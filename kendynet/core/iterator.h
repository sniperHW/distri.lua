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
#ifndef _ITERATOR_H
#define _ITERATOR_H

struct base_iterator
{
	void   (*next)(struct base_iterator*);
	void   (*get_key)(struct base_iterator*,void*);
	void   (*get_val)(struct base_iterator*,void*);
	void   (*set_val)(struct base_iterator*,void*);
	int8_t (*is_equal)(struct base_iterator*,struct base_iterator*);
};

/*
#ifndef IT_NEXT
#define IT_NEXT(IT_TYPE,ITER)\
   ({ IT_TYPE __result;\
       do ITER.base.next((struct base_iterator*)&ITER,(struct base_iterator*)&__result);\
       while(0);\
       __result;})
#endif*/

#ifndef IT_NEXT
#define IT_NEXT(ITER)\
	ITER.base.next((struct base_iterator*)&ITER)
#endif

#ifndef IT_EQ
#define IT_EQ(IT1,IT2)\
	IT1.base.is_equal((struct base_iterator*)&IT1,(struct base_iterator*)&IT2)
#endif

#ifndef IT_GET_KEY
#define IT_GET_KEY(TYPE,ITER)\
   ({ TYPE __result;\
       do ITER.base.get_key((struct base_iterator*)&ITER,&__result);\
       while(0);\
       __result;})
#endif

#ifndef IT_GET_VAL
#define IT_GET_VAL(TYPE,ITER)\
   ({ TYPE __result;\
       do ITER.base.get_val((struct base_iterator*)&ITER,&__result);\
       while(0);\
       __result;})
#endif

#ifndef IT_SET_VAL
#define IT_SET_VAL(TYPE,ITER,VAL)\
	{TYPE __val=VAL;ITER.base.set_val((struct base_iterator*)&ITER,&__val);}
#endif

#endif