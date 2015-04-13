#include "kn_stream_socket.h"

void ShowCerts(SSL * ssl)
{
    X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl);
    if (cert != NULL) {
        printf("数字证书信息:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("证书: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("颁发者: %s\n", line);
        free(line);
        X509_free(cert);
    } else
        printf("无证书信息！\n");
}

static void on_events(handle_t h,int events);

static void on_destroy(void *_){
	kn_socket *s = (kn_socket*)_;
	st_io *io_req;
	if(s->clear_func){
        		while((io_req = (st_io*)kn_list_pop(&s->pending_send))!=NULL)
            			s->clear_func(io_req);
        		while((io_req = (st_io*)kn_list_pop(&s->pending_recv))!=NULL)
            			s->clear_func(io_req);
	}
	kn_stream_socket *ss = (kn_stream_socket*)_;
	if(ss->ctx){
		SSL_CTX_free(ss->ctx);
	}
	if(ss->ssl){
		if(s->comm_head.status == SOCKET_ESTABLISH)
        			SSL_shutdown(ss->ssl);
        		SSL_free(ss->ssl);
	}
	if(s->e){
		kn_event_del(s->e,(handle_t)s);
	}
	close(s->comm_head.fd);
	free(s);
}

int stream_socket_close(handle_t h){
	if(h->type != KN_SOCKET && ((kn_socket*)h)->type != SOCK_STREAM)
		return -1;
	kn_socket *s = (kn_socket*)h;
	if(h->status != SOCKET_CLOSE){
		if(h->inloop){
			h->status = SOCKET_CLOSE;
			//shutdown(h->fd,SHUT_WR);
			//kn_push_destroy(s->e,h);
		}else
			on_destroy(s);				
		return 0;
	}
	return -1;	
}

static int stream_socket_associate(engine_t e,handle_t h,void (*callback)(handle_t,void*,int,int)){
	if(((handle_t)h)->type != KN_SOCKET) return -1;						  
	kn_socket *s = (kn_socket*)h;
	if(!callback) return -1;
	if(s->e){
		kn_event_del(s->e,h);
		s->e = NULL;
	}	
	if(h->status == SOCKET_ESTABLISH){
		kn_set_noblock(h->fd,0);
#ifdef _LINUX
		kn_event_add(e,h,EPOLLRDHUP);
#elif   _BSD
		kn_event_add(e,h,EVFILT_READ);
		kn_event_add(e,h,EVFILT_WRITE);
		kn_disable_read(e,h);
		kn_disable_write(e,h);
#else
		return -1;
#endif
	}else if(h->status == SOCKET_LISTENING){
#ifdef _LINUX	
		kn_event_add(e,h,EPOLLIN);
#elif _BSD
		kn_event_add(e,EVFILT_READ);
#else
		return -1;
#endif		
	}else if(h->status == SOCKET_CONNECTING){
#ifdef _LINUX			
		kn_event_add(e,h,EPOLLIN | EPOLLOUT);
#elif _BSD
		kn_event_add(e,h,EVFILT_READ);
		kn_event_add(e,h,EVFILT_WRITE);		
#else
		return -1;
#endif
	}
	else{
		return -1;
	}
	s->callback = callback;
	s->e = e;	
	return 0;
}

handle_t new_stream_socket(int fd,int domain){
	kn_stream_socket *ss = calloc(1,sizeof(*ss));
	if(!ss){
		return NULL;
	}	
	((handle_t)ss)->fd = fd;
	((handle_t)ss)->type = KN_SOCKET;
	((kn_socket*)ss)->domain = domain;
	((kn_socket*)ss)->type = SOCK_STREAM;
	((handle_t)ss)->on_events = on_events;
	//((handle_t)ss)->on_destroy = on_destroy;
	((handle_t)ss)->associate = stream_socket_associate;
	return (handle_t)ss; 
}

static void process_read(kn_socket *s){
	st_io* io_req = 0;
	int bytes_transfer = 0;
	while((io_req = (st_io*)kn_list_pop(&s->pending_recv))!=NULL){
		errno = 0;
		if(((kn_stream_socket*)s)->ssl){
			bytes_transfer = TEMP_FAILURE_RETRY(SSL_read(((kn_stream_socket*)s)->ssl,io_req->iovec[0].iov_base,io_req->iovec[0].iov_len));
			int ssl_error = SSL_get_error(((kn_stream_socket*)s)->ssl,bytes_transfer);
			if(bytes_transfer < 0 && (ssl_error == SSL_ERROR_WANT_WRITE ||
						ssl_error == SSL_ERROR_WANT_READ ||
						ssl_error == SSL_ERROR_WANT_X509_LOOKUP)){
				errno = EAGAIN;
			}			
		}
		else
			bytes_transfer = TEMP_FAILURE_RETRY(readv(s->comm_head.fd,io_req->iovec,io_req->iovec_count));	
		
		if(bytes_transfer < 0 && errno == EAGAIN){
				//将请求重新放回到队列
				kn_list_pushback(&s->pending_recv,(kn_list_node*)io_req);
				break;
		}else{
			s->callback((handle_t)s,io_req,bytes_transfer,errno);
			if(s->comm_head.status == SOCKET_CLOSE)
				return;			
		}
	}	
	if(!kn_list_size(&s->pending_recv)){
		//没有接收请求了,取消EPOLLIN
		kn_disable_read(s->e,(handle_t)s);
	}	
}

static void process_write(kn_socket *s){
	st_io* io_req = 0;
	int bytes_transfer = 0;
	while((io_req = (st_io*)kn_list_pop(&s->pending_send))!=NULL){
		errno = 0;
		if(((kn_stream_socket*)s)->ssl){
			bytes_transfer = TEMP_FAILURE_RETRY(SSL_write(((kn_stream_socket*)s)->ssl,io_req->iovec[0].iov_base,io_req->iovec[0].iov_len));
			int ssl_error = SSL_get_error(((kn_stream_socket*)s)->ssl,bytes_transfer);
			if(bytes_transfer < 0 && (ssl_error == SSL_ERROR_WANT_WRITE ||
						ssl_error == SSL_ERROR_WANT_READ ||
						ssl_error == SSL_ERROR_WANT_X509_LOOKUP)){
				errno = EAGAIN;
			}			
		}
		else	
			bytes_transfer = TEMP_FAILURE_RETRY(writev(s->comm_head.fd,io_req->iovec,io_req->iovec_count));
		
		if(bytes_transfer < 0 && errno == EAGAIN){
				//将请求重新放回到队列
				kn_list_pushback(&s->pending_send,(kn_list_node*)io_req);
				break;
		}else{
			s->callback((handle_t)s,io_req,bytes_transfer,errno);
			if(s->comm_head.status == SOCKET_CLOSE)
				return;
		}
	}
	if(!kn_list_size(&s->pending_send)){
		//没有接收请求了,取消EPOLLOUT
		kn_disable_write(s->e,(handle_t)s);
	}		
}

static int _accept(kn_socket *a,kn_sockaddr *remote){
	int fd;
	socklen_t len;
	int domain = a->domain;
again:
	if(domain == AF_INET){
		len = sizeof(remote->in);
		fd = accept(a->comm_head.fd,(struct sockaddr*)&remote->in,&len);
	}else if(domain == AF_INET6){
		len = sizeof(remote->in6);
		fd = accept(a->comm_head.fd,(struct sockaddr*)&remote->in6,&len);
	}else if(domain == AF_LOCAL || domain == AF_UNIX){
		len = sizeof(remote->un);
		fd = accept(a->comm_head.fd,(struct sockaddr*)&remote->un,&len);
	}else{
		return -1;
	}

	if(fd < 0){
#ifdef EPROTO
		if(errno == EPROTO || errno == ECONNABORTED)
#else
		if(errno == ECONNABORTED)
#endif
			goto again;
		else
			return -errno;
	}
	int flags;
	int dummy = 0;
	if ((flags = fcntl(fd, F_GETFL, dummy)) < 0){
		printf("fcntl get error\n");
    		close(fd);
    		return -1;
	}
	if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) <0){
    		printf("fcntl set  FD_CLOEXEC error\n");
    		close(fd);
    		return -1;	
	}
	
	return fd;
}

static void process_accept(kn_socket *s){
    int fd;
    kn_sockaddr remote;
    for(;;)
    {
    	fd = _accept(s,&remote);
    	if(fd < 0)
    		break;
    	else{
		   handle_t h = new_stream_socket(fd,s->domain);	
		   ((kn_socket*)h)->addr_local = s->addr_local;
		   ((kn_socket*)h)->addr_remote = remote;
		   if(((kn_stream_socket*)s)->ctx){
		   	((kn_stream_socket*)h)->ssl = SSL_new(((kn_stream_socket*)s)->ctx);
		   	SSL_set_fd(((kn_stream_socket*)h)->ssl, fd);
        			if (SSL_accept(((kn_stream_socket*)h)->ssl) == -1) {
            				printf("SSL_accept error\n");
            				stream_socket_close(h);
            				continue;
        			}
		   }
		   h->status = SOCKET_ESTABLISH;
		   ((kn_socket*)s)->callback(h,s,0,0);
    	}      
    }
}

static void process_connect(kn_socket *s,int events){
	int err = 0;
	socklen_t len = sizeof(err);    
	//kn_event_del(s->e,(handle_t)s);
	//s->e = NULL;
	if(getsockopt(s->comm_head.fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1) {
	    if(s->callback){
	    	s->callback((handle_t)s,&s->addr_remote,0,err);
	    }else{
	    	kn_close_sock((handle_t)s);
	    }
	    return;
	}
	if(err){
	    errno = err;
	    if(s->callback){
	    	s->callback((handle_t)s,&s->addr_remote,0,errno);
	    }else{
	    	kn_close_sock((handle_t)s);
	    }	    
	    return;
	}
	//connect success
	    if(s->callback){
	    	s->comm_head.status = SOCKET_ESTABLISH;
	    	s->callback((handle_t)s,&s->addr_remote,0,0);
	    }else{
	    	kn_close_sock((handle_t)s);
	    }			
}

static void on_events(handle_t h,int events){	
	kn_socket *s = (kn_socket*)h;
	if(h->status == SOCKET_CLOSE)
		return;
	do{
		h->inloop = 1;
		if(h->status == SOCKET_LISTENING){
			process_accept(s);
		}else if(h->status == SOCKET_CONNECTING){
			process_connect(s,events);
		}else if(h->status == SOCKET_ESTABLISH){
			if(events & EVENT_READ){
				if(kn_list_size(&s->pending_recv) == 0){
					s->callback((handle_t)s,NULL,-1,0);
					break;
				}else{
					process_read(s);	
					if(h->status == SOCKET_CLOSE) 
						break;								
				}
			}		
			if(events & EVENT_WRITE)
				process_write(s);			
		}
		h->inloop = 0;
	}while(0);
	if(h->status == SOCKET_CLOSE)
		on_destroy(s);
}

static int _bind(int fd,kn_sockaddr *addr_local){
	assert(addr_local);
	int ret = -1;
	if(addr_local->addrtype == AF_INET)
		ret = bind(fd,(struct sockaddr*)&addr_local->in,sizeof(addr_local->in));
	else if(addr_local->addrtype == AF_INET6)
		ret = bind(fd,(struct sockaddr*)&addr_local->in6,sizeof(addr_local->in6));
	else if(addr_local->addrtype == AF_LOCAL)
		ret = bind(fd,(struct sockaddr*)&addr_local->un,sizeof(addr_local->un));
	return ret;	
}

int stream_socket_listen(kn_stream_socket *ss,kn_sockaddr *local)
{	
	if(((handle_t)ss)->status != SOCKET_NONE) 
		return -1;
	int fd = ((handle_t)ss)->fd;
	kn_set_noblock(fd,0);
	int32_t yes = 1;
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)))
		return -1;
	
	if(_bind(fd,local) < 0){
		 return -1;
	}	
	if(listen(fd,SOMAXCONN) < 0){
		return -1;
	}

	((kn_socket*)ss)->addr_local = *local;
	((handle_t)ss)->status = SOCKET_LISTENING;	
	return 0;
}

int stream_socket_connect(kn_stream_socket *ss,
			      kn_sockaddr *local,
			      kn_sockaddr *remote)
{
	int fd = ((handle_t)ss)->fd;
	if(!ss->ctx) kn_set_noblock(fd,0);
	socklen_t len;	
	if(local){
		if(_bind(fd,local) < 0){
			return -1;
		}
	}
	int ret;
	if(((kn_socket*)ss)->domain == AF_INET)
		ret = connect(fd,(const struct sockaddr *)&remote->in,sizeof(remote->in));
	else if(((kn_socket*)ss)->domain == AF_INET6)
		ret = connect(fd,(const struct sockaddr *)&remote->in6,sizeof(remote->in6));
	else if(((kn_socket*)ss)->domain == AF_LOCAL)
		ret = connect(fd,(const struct sockaddr *)&remote->un,sizeof(remote->un));
	else{
		return -1;
	}
	if(ret < 0 && errno != EINPROGRESS){
		return -1;
	}
	if(ret == 0){		
		if(!local){		
			((kn_socket*)ss)->addr_local.addrtype = ((kn_socket*)ss)->domain;
			if(((kn_socket*)ss)->addr_local.addrtype == AF_INET){
				len = sizeof(((kn_socket*)ss)->addr_local.in);
				getsockname(fd,(struct sockaddr*)&((kn_socket*)ss)->addr_local.in,&len);
			}else if(((kn_socket*)ss)->addr_local.addrtype == AF_INET6){
				len = sizeof(((kn_socket*)ss)->addr_local.in6);
				getsockname(fd,(struct sockaddr*)&((kn_socket*)ss)->addr_local.in6,&len);
			}else{
				len = sizeof(((kn_socket*)ss)->addr_local.un);
				getsockname(fd,(struct sockaddr*)&((kn_socket*)ss)->addr_local.un,&len);
			}
    		}
    		((handle_t)ss)->status = SOCKET_ESTABLISH;
		return 1;
	}
	((handle_t)ss)->status = SOCKET_CONNECTING;
	return 0;	
}


void SSL_init(){
    /* SSL 库初始化 */
    SSL_library_init();
    /* 载入所有 SSL 算法 */
    OpenSSL_add_all_algorithms();
    /* 载入所有 SSL 错误消息 */
    SSL_load_error_strings();
}

int      kn_sock_ssllisten(handle_t h,
		             kn_sockaddr *addr,
		             const char *certificate,
		             const char *privatekey
		             ){
	   if(h->type != KN_SOCKET && ((kn_socket*)h)->type != SOCK_STREAM)
		return 0;	
	   kn_stream_socket *ss = (kn_stream_socket*)h;
	   if(h->status != SOCKET_NONE) return -1;
	    /* 以 SSL V2 和 V3 标准兼容方式产生一个 SSL_CTX ，即 SSL Content Text */
	    SSL_CTX *ctx = SSL_CTX_new(SSLv23_server_method());
	    /* 也可以用 SSLv2_server_method() 或 SSLv3_server_method() 单独表示 V2 或 V3标准 */
	    if (ctx == NULL) {
	        ERR_print_errors_fp(stdout);
	        return -1;
	    }
	    /* 载入用户的数字证书， 此证书用来发送给客户端。 证书里包含有公钥 */
	    if (SSL_CTX_use_certificate_file(ctx,certificate, SSL_FILETYPE_PEM) <= 0) {
	        ERR_print_errors_fp(stdout);
	        SSL_CTX_free(ctx);
	        return -1;
	    }
	    /* 载入用户私钥 */
	    if (SSL_CTX_use_PrivateKey_file(ctx, privatekey, SSL_FILETYPE_PEM) <= 0) {
	        ERR_print_errors_fp(stdout);
	        SSL_CTX_free(ctx);
	        return -1;
	    }
	    /* 检查用户私钥是否正确 */
	    if (!SSL_CTX_check_private_key(ctx)) {
	        ERR_print_errors_fp(stdout);
	        SSL_CTX_free(ctx);
	        return -1;
	    }
	    kn_set_noblock(((handle_t)ss)->fd,0);    
	    ss->ctx = ctx;
	    return stream_socket_listen(ss,addr);    
}

int      kn_sock_sslconnect(//engine_t e,
		                  handle_t h,
		                  kn_sockaddr *remote,
		                  kn_sockaddr *local){
	if(h->type != KN_SOCKET && ((kn_socket*)h)->type != SOCK_STREAM)
		return 0;
	kn_socket *s = (kn_socket*)h;
	if(h->status != SOCKET_NONE) return -1;
	//if(s->e) return -1;
	SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL) {
	    ERR_print_errors_fp(stdout);
	    return -1;
	}
    	((kn_stream_socket*)s)->ctx = ctx;		
	s->addr_remote = *remote;
	int ret = stream_socket_connect((kn_stream_socket*)s,local,remote);
	if(ret == 1){
		((kn_stream_socket*)s)->ssl = SSL_new(ctx);
		SSL_set_fd(((kn_stream_socket*)s)->ssl,h->fd);
		if (SSL_connect(((kn_stream_socket*)s)->ssl) == -1){
		        ERR_print_errors_fp(stderr);
		        ret = -1;
		}
		else {
		        kn_set_noblock(h->fd,0);
		        h->status = SOCKET_ESTABLISH;
		        printf("Connected with %s encryption/n", SSL_get_cipher(((kn_stream_socket*)s)->ssl));
		        ShowCerts(((kn_stream_socket*)s)->ssl);
		}		
	}else{
		ret = -1;
	}	
	return ret;	
}

int      kn_is_ssl_socket(handle_t h){
	if(h->type != KN_SOCKET && ((kn_socket*)h)->type != SOCK_STREAM)
		return 0;
	kn_stream_socket *ss = (kn_stream_socket*)h;
	return (ss->ssl || ss->ctx) ? 1 : 0;
}

int      kn_get_ssl_error(handle_t h,int ret){
	if(h->type != KN_SOCKET && ((kn_socket*)h)->type != SOCK_STREAM)
		return 0;	
	kn_stream_socket *ss = (kn_stream_socket*)h;
	if(ss->ssl)
		return SSL_get_error(ss->ssl,ret);
	else
		return 0;
}