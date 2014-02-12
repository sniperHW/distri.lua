#include "core/kn_string.h"
#include <stdio.h>

int main(){

	string_t s1 = new_string("hello");

	string_append(s1,"hahaha");
	printf("%s\n",to_cstr(s1));

	string_t s2 = new_string("");
	string_copy(s2,s1,5);
	printf("%s\n",to_cstr(s2));

	string_append(s1,"hahahafasfasdfasfasfasdfasdfasdfasdfasfasfasfasdfasdfdf");
	printf("%s\n",to_cstr(s1));

	string_append(s1,"hahahafasfasdfasfasfasdfasdfasdfasdfasfasfasfasdfasdfdfhahahafasfasdfasfasfasdfasdfasdfasdfasfasfasfasdfasdfdfhahahafasfasdfasfasfasdfasdfasdfasdfasfasfasfasdfasdfdf");
	printf("%s\n",to_cstr(s1));

	string_copy(s2,s1,80);
	printf("%s\n",to_cstr(s2));
	printf("%d\n",string_len(s2));
	return 0;
}