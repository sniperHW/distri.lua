/*
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _KN_MSG_H
#define _KN_MSG_H
#include "llist.h"
#include "refbase.h"
#include <stdint.h>

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

#endif
