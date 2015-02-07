#include "top.h"
#include <stdio.h>
#include "kn_time.h"

int main(){
	struct top *_top = new_top();
	add_filter(_top,"top");
	while(1){
		kn_string_t str = get_top(_top);
		printf("%s",kn_to_cstr(str));
		kn_sleepms(1000);
		kn_release_string(str);
		system("clear");
	};
	return 0;
}