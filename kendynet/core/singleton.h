#ifndef _SINGLETON_H
#define _SINGLETON_H

#include <pthread.h>

#define DECLARE_SINGLETON(TYPE)\
        extern pthread_key_t  TYPE##_key;\
        extern pthread_once_t TYPE##_key_once;\
        extern TYPE*          TYPE##_instance;\
        extern void (*TYPE_fn_destroy)(void*);


#define IMPLEMENT_SINGLETON(TYPE,CREATE_FUNCTION,DESTYOY_FUNCTION)\
        pthread_key_t  TYPE##_key;\
        pthread_once_t TYPE##_key_once = PTHREAD_ONCE_INIT;\
        TYPE*          TYPE##_instance = NULL;\
        void (*TYPE_fn_destroy)(void*) = DESTYOY_FUNCTION;\
        static inline  void TYPE##_on_process_end(){\
        	TYPE_fn_destroy((void*)TYPE##_instance);\
        }\
        static inline  void TYPE##_once_routine(){\
            pthread_key_create(&TYPE##_key,NULL);\
            TYPE##_instance = CREATE_FUNCTION();\
            if(TYPE_fn_destroy) atexit(TYPE##_on_process_end);\
        }

#define GET_INSTANCE(TYPE)\
        ({do pthread_once(&TYPE##_key_once,TYPE##_once_routine);\
            while(0);\
            TYPE##_instance;})

#endif