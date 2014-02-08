#ifndef _IDMGR_H
#define _IDMGR_H
#include <stdint.h>
#include "core/llist.h"

typedef struct idnode
{
	lnode    node;	
	int32_t  id;
}idnode;


typedef struct idmgr{
	struct llist idpool;
	uint32_t beginid
	uint32_t endid;
}*idmgr_t;

static inline idmgr_t new_idmgr(int32_t beginid,int32_t endid)
{
	if(beginid < 0 || endid < 0 || beginid > endid)
		return NULL;
	idmgr_t _idmgr = calloc(1,sizeof(*_idmgr));
	_idmgr->beginid = beginid;
	_idmgr->endid = endid;
	llist_init(&_idmgr->idpool);
	int32_t i = beginid;
	for(; i <= endid; ++i){
		idnode *id = calloc(1,sizeof(*id));
		id->id = i;
		LLIST_PUSHBACK(&_idmgr->idpool,(lnode*)id);
	}
	return _idmgr;	
}

static inline void destroy_idmgr(idmgr_t _idmgr)
{
	idnode *id;
	while((id = LLIST_POP((idnode*),&_idmgr->idpool))!= NULL)
		free(id);
	free(_idmgr);
}

static inline int32_t get_id(idmgr_t _idmgr)
{
	idnode *id = LLIST_POP((idnode*),&_idmgr->idpool);
	if(!id) return -1;
	int32_t ret = id->id;
	free(id);
	return ret;
}

static inline void release_id(idmgr_t _idmgr,int32_t id)
{
	if(id >= _idmgr->beginid && id <= _idmgr->endid)
	{
		idnode *_idnode = calloc(1,sizeof(*_idnode));
		_idnode->id = id;
		LLIST_PUSHBACK(&_idmgr->idpool,_idnode);
	}
}



#endif