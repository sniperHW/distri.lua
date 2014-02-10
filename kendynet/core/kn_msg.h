#ifndef _KN_MSG_H
#define _KN_MSG_H

typedef struct msg
{
    lnode   node;
    uint8_t type;
    union{
        uint64_t  usr_data;
        void*     usr_ptr;
        ident     _ident;
    };
    void (*fn_destroy)(void*);
}*msg_t;

#define MSG_TYPE(MSG) ((msg_t)MSG)->type
#define MSG_NEXT(MSG) ((msg_t)MSG)->node.next
#define MSG_USRDATA(MSG) ((msg_t)MSG)->usr_data
#define MSG_USRPTR(MSG)  ((msg_t)MSG)->usr_ptr
#define MSG_IDENT(MSG)   ((msg_t)MSG)->_ident
#define MSG_FN_DESTROY(MSG) ((msg_t)MSG)->fn_destroy

typedef struct msg_do_function
{
    struct msg base;
    void (*fn_function)(void*);

}*msg_do_function_t;

#endif
