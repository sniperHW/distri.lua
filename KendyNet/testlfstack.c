//#include "lf_stack.h"
#include "kn_list.h"
#include "kn_time.h"
#include "kn_thread.h"
#include "kn_atomic.h"

typedef struct lockfree_stack
{
	kn_list_node * volatile head;
}lockfree_stack,*lockfree_stack_t;

static  void lfstack_push(lockfree_stack_t ls,kn_list_node *n)
{
	for( ; ;){	
		kn_list_node *lhead = ls->head;
		n->next = lhead;
		if(COMPARE_AND_SWAP(&ls->head,lhead,n))//if head unchange,set n to be the new head
			break;
	}
}

static  kn_list_node* lfstack_pop(lockfree_stack_t ls)
{
	for( ; ;){	
		kn_list_node *lhead = ls->head;
		if(!lhead) return NULL;
		kn_list_node *next = lhead->next;				
		if(COMPARE_AND_SWAP(&ls->head,lhead,next))
		{
			lhead->next = NULL;
			return lhead;
		}
	}
}


volatile int count = 0;
atomic_32_t c1 = 0;
atomic_32_t c2 = 0;

struct element{
	kn_list_node node;
	int value;
};

struct element *element_pool1;
struct element *element_pool2;

lockfree_stack lf_stack;

void *producer1(void *arg)
{
	printf("producer1\n");
	int i;
	while(1){
		for(i = 0; i < 10000000; ++i){
			struct element *ele =  &element_pool1[i];
			ATOMIC_INCREASE(&c1);
			lfstack_push(&lf_stack,(kn_list_node*)ele);
		}
		while(c1 > 0){
			FENCE();
			kn_sleepms(0);
		}
	}
	printf("producer1 end\n");
    	return NULL;
}

void *producer2(void *arg)
{
	printf("producer2\n");
	int i;
	while(1){
		for(i = 0; i < 10000000; ++i){
			struct element *ele =  &element_pool2[i];
			ATOMIC_INCREASE(&c2);	
			lfstack_push(&lf_stack,(kn_list_node*)ele);

		}
		while(c2 > 0){
			FENCE();
			kn_sleepms(0);
		}
	}
    return NULL;
}


void *consumer(void *arg)
{
	printf("consumer\n");
	volatile struct element *ele;
	uint32_t tick = 0;
	while(1){
		if((ele = (struct element*)lfstack_pop(&lf_stack))){
			if(count == 0){
				 tick = kn_systemms();
			}
			if(++count  == 5000000) {
				uint32_t now = kn_systemms();
            				uint32_t elapse = (uint32_t)(now-tick);
				printf("pop %d/ms\n",count/elapse*1000);
				tick = now;
				count = 0;				
			}
			if(ele->value == 1)
				ATOMIC_DECREASE(&c1);
			else if(ele->value == 2)
				ATOMIC_DECREASE(&c2);
			else
				printf("%d\n",ele->value);	
		}
	}
    return NULL;
}

int main(){
	element_pool1 = calloc(10000000,sizeof(*element_pool1));
	element_pool2 = calloc(10000000,sizeof(*element_pool2));
	int i;
	for(i = 0; i < 10000000; ++i) element_pool1[i].value = 1;
	for(i = 0; i < 10000000; ++i) element_pool2[i].value = 2;		
	lf_stack.head = NULL;
	kn_thread_t t1 = kn_create_thread(THREAD_JOINABLE);
	kn_thread_t t2 = kn_create_thread(THREAD_JOINABLE);	
	kn_thread_t t3 = kn_create_thread(THREAD_JOINABLE);	
	kn_thread_start_run(t1,producer1,NULL);
	kn_thread_start_run(t2,producer2,NULL);	
	kn_thread_start_run(t3,consumer,NULL);		
 	getchar();
	return 0;
}
