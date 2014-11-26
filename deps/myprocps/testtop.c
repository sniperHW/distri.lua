#include "mytop.h"

int main (int dont_care_argc, char *argv[]){   
   addfilter("distrilua");
   addfilter("gateserverd");
   addfilter("groupserverd");
   addfilter("ssdb-server");		
   for (;;) {
	  system("clear");
     	  printf("%s\n",top());
	  sleep(1);
   }
   return 0;
}
