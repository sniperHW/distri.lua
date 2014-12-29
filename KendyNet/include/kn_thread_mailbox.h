#ifndef _KN_THREAD_MAILBOX_H
#define _KN_THREAD_MAILBOX_H

#include "kn_refobj.h"
#include "kendynet.h"

//线程邮箱,每个线程有一个唯一的线程邮箱用于接收其它线程发过来的消息
typedef ident kn_thread_mailbox_t;

typedef void (*cb_on_mail)(kn_thread_mailbox_t *from,void *);

void kn_setup_mailbox(engine_t,cb_on_mail);

int  kn_send_mail(kn_thread_mailbox_t,void*,void (*fn_destroy)(void*));

//获得当前线程的mailbox
kn_thread_mailbox_t kn_self_mailbox();

//根据线程id查询mailbox
kn_thread_mailbox_t kn_query_mailbox(pthread_t);

#endif
