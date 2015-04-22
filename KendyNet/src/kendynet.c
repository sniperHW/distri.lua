#include "kendynet.h"
#include "kn_type.h"

int kn_engine_associate(engine_t e,handle_t h,void (*callback)(handle_t,void*,int,int)){

	if (!h->associate) 
		return -1;
	else 
		return h->associate(e,h,callback);
}
