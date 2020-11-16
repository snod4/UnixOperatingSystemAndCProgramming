#include <stdio.h>
#include <string.h>


int main(){

   char * test = "This is a test";
  char * str =  strsep(&test," ");
  printf("%s\n", test);
  //  printf("%s\n", test);
  // printf("HELLO");
  return 0;
}
