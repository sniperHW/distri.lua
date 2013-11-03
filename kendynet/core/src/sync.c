#include "sync.h"
#include <stdlib.h>
#include <stdio.h>

mutex_t mutex_create()
{
	mutex_t m = malloc(sizeof(*m));
	pthread_mutexattr_init(&m->m_attr);
	pthread_mutexattr_settype(&m->m_attr,PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&m->m_mutex,&m->m_attr);
	return m;
}

void mutex_destroy(mutex_t *m)
{
	pthread_mutexattr_destroy(&(*m)->m_attr);
	pthread_mutex_destroy(&(*m)->m_mutex);
	free(*m);
	*m=0;
}

condition_t condition_create()
{
	condition_t c = malloc(sizeof(*c));
	pthread_cond_init(&c->cond,NULL);
	return c;
}

void condition_destroy(condition_t *c)
{
	pthread_cond_destroy(&(*c)->cond);
	free(*c);
	*c = 0;
}



barrior_t barrior_create(int waitcount)
{
	barrior_t b = malloc(sizeof(*b));
	b->wait_count = waitcount;
	b->mtx = mutex_create();
	b->cond = condition_create();
	return b;
}

void barrior_destroy(barrior_t *b)
{
	mutex_destroy(&(*b)->mtx);
	condition_destroy(&(*b)->cond);
	free(*b);
	*b = 0;
}

