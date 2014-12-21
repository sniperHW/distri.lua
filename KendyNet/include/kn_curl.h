#ifndef _KN_CURL_H
#define _KN_CURL_H

#include <curl/curl.h>


typedef struct kn_CURLM *kn_CURLM_t;
typedef struct kn_CURL *kn_CURL_t;


kn_CURL_t        kn_curl_easy_init();
CURL            *kn_curl_easy_get(kn_CURL_t);


#ifndef 	  kn_curl_easy_setopt	
//CURLOPT_PRIVATE有特殊用处,所以禁止使用CURLOPT_PRIVATE
#define       kn_curl_easy_setopt(__curl,__option,...)({\
					   CURLcode __result;\
					   if(__option == CURLOPT_PRIVATE)\
							__result = CURL_LAST;\
					   else \
							__result = curl_easy_setopt(kn_curl_easy_get(__curl),__option,__VA_ARGS__);\
					   __result;})
#endif

void          kn_curl_easy_cleanup(kn_CURL_t curl);

#ifndef       kn_curl_easy_getinfo
#define       kn_curl_easy_getinfo(__curl,__info, ...)\
					({ CURLcode __result;\
					   if(__info == CURLOPT_PRIVATE)\
							__result = CURL_LAST;\
					   else \
							__result = curl_easy_getinfo(kn_curl_easy_get(__curl),__info,__VA_ARGS__);\
					   __result;})
#endif

kn_CURLM_t    kn_CURLM_init(engine_t);
CURLM*        kn_CURLM_get(kn_CURLM_t);

#ifndef       kn_CURLM_setopt
#define       kn_CURLM_setopt(h,option, ...)\
					({ CURLcode __result;\
					   __result = curl_multi_setopt(kn_curl_get(h),__VA_ARGS__);\
					   __result;})
#endif

CURLMcode     kn_CURLM_add(kn_CURLM_t,kn_CURL_t,void (*cb)(kn_CURL_t,CURLMsg *message,void*),void*ud);
void          kn_CURLM_cleanup(kn_CURLM_t);


#endif
