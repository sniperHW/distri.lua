#include "kendynet.h"
#include "kn_thread.h"
#include "kn_refobj.h"


struct testobj{
	refobj   refobj;	
};

static void on_destroy(void *ptr)
{
	printf("testobj,destroy:%u\n",pthread_self());
	free(ptr);				
}

static struct testobj* new_testobj(){
	struct testobj *obj =calloc(1,sizeof(*obj));
	refobj_init((refobj*)obj,on_destroy);
	return obj;
}

ident g_ident;
struct testobj *g_obj;


void *routine1(void *arg)
{
    struct testobj *obj = (struct testobj *)cast2refobj(g_ident);
    if(obj){
    	printf("routine1,got:%u\n",pthread_self());
    	refobj_dec((refobj*)obj);
    }
    return NULL;
}

void *routine2(void *arg)
{
    struct testobj *obj = (struct testobj *)cast2refobj(g_ident);
    if(obj){
    	printf("routine2,got:%u\n",pthread_self());
    	refobj_dec((refobj*)obj);
    }
    return NULL;
}

void *routine3(){
	refobj_dec((refobj*)g_obj);
	return NULL;
}

typedef void* (*thread_fn)(void*);

thread_fn _thread_fn[][3] ={
 	{&routine1,&routine2,&routine3},
 	{&routine1,&routine3,&routine2},
 	{&routine2,&routine1,&routine3},
 	{&routine2,&routine3,&routine1},
 	{&routine3,&routine2,&routine1},
 	{&routine3,&routine1,&routine2},
 };

int main(){
	int i = 0;
	for(; i < 10000;++i){
		printf("---------------------------%d--------------------------------\n",i+1);
		g_obj = new_testobj();
		g_ident = make_ident((refobj*)g_obj);
		kn_thread_t t1 = kn_create_thread(THREAD_JOINABLE);
		kn_thread_t t2 = kn_create_thread(THREAD_JOINABLE);	
		kn_thread_t t3 = kn_create_thread(THREAD_JOINABLE);	
		kn_thread_start(t1,_thread_fn[i%3][0],NULL);
		kn_thread_start(t3,_thread_fn[i%3][1],NULL);
		kn_thread_start(t2,_thread_fn[i%3][2],NULL);			
		kn_thread_join(t1);
		kn_thread_join(t2);
		kn_thread_join(t3);
	}
	return 0;
}