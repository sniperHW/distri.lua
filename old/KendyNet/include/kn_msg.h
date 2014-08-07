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
#include "kn_list.h"
#include "kn_ref.h"
#include <stdint.h>

typedef struct msg
{
    kn_list_node node;
    uint8_t      type;
    void         *ud;
    void (*fn_destroy)(void*);
}*msg_t;

#define MSG_TYPE(MSG) ((msg_t)MSG)->type
#define MSG_NEXT(MSG) ((msg_t)MSG)->node.next
#define MSG_UD(MSG) ((msg_t)MSG)->ud
#define MSG_FN_DESTROY(MSG) ((msg_t)MSG)->fn_destroy

#endif
