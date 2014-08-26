#include "http_parser.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h> /* rand */
#include <string.h>
#include <stdarg.h>


int on_message_begin (http_parser *parser){
	printf("on_message_begin\n");
	return 0;
}

int on_url(http_parser *parser, const char *at, size_t length){
	char *url = calloc(1,length+1);
	memcpy(url,at,length);
	url[length] = 0;
	printf("on_url:%s\n",url);	
	return 0;
}

int on_status(http_parser *parser, const char *at, size_t length){
	char *status = calloc(1,length+1);
	memcpy(status,at,length);
	status[length] = 0;
	printf("on_status:%s\n",status);	
	return 0;
}

int on_header_field(http_parser *parser, const char *at, size_t length){
	char *field = calloc(1,length+1);
	memcpy(field,at,length);
	field[length] = 0;
	printf("on_header_field:%s\n",field);	
	return 0;
}

int on_header_value(http_parser *parser, const char *at, size_t length){
	char *value = calloc(1,length+1);
	memcpy(value,at,length);
	value[length] = 0;
	printf("on_header_value:%s\n",value);	
	return 0;
}

int on_headers_complete(http_parser *parser){
	printf("on_headers_complete\n");		
	return 0;
}

int on_body(http_parser *parser, const char *at, size_t length){
	char *body = calloc(1,length+1);
	memcpy(body,at,length);
	body[length] = 0;
	printf("on_body:%s\n",body);	
	return 0;
}

int on_message_complete(http_parser *parser){
	printf("on_message_complete\n");		
	return 0;
}

int main(){
	
	http_parser_settings settings = {
		.on_message_begin = on_message_begin,
		.on_url = on_url,
		.on_status = on_status,
		.on_header_field = on_header_field,
		.on_header_value = on_header_value,
		.on_headers_complete = on_headers_complete,
		.on_body = on_body,
		.on_message_complete = on_message_complete,
	};	
	http_parser parser;
	http_parser_init(&parser, HTTP_REQUEST);
	
	
	char buf[] = "GET /demo HTTP/1.1\r\n"
				 "Host: www.baidu.com\r\n"				 
				 "Content-Length: 5\r\n"
				  "\r\n"
				  "hello";
	
	http_parser_execute(&parser,&settings,buf,strlen("GET /demo HTTP/1.1\r\n"));
	http_parser_execute(&parser,&settings,buf+strlen("GET /demo HTTP/1.1\r\n"),sizeof(buf) - strlen("GET /demo HTTP/1.1\r\n"));
	
	return 0;
}
