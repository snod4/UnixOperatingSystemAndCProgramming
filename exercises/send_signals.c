#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>

int main(int argv, char * args[]){

  if(argv != 4){
    perror("./send_signals <pid> <signal> <number of times>\n");
    exit(2);
  }
  
  for(int i = 0; i < atoi(args[3]); i++){
    if(strcmp(args[2], "SIGINT") == 0){
      if(kill(atoi(args[1]),2 ) == -1){
        perror("Failed to kill process");
        exit(2);
      }
    }

    else if(strcmp(args[2], "SIGUSR1") == 0){
      if(kill(atoi(args[1]),10 ) == -1){
        perror("Failed to kill process");
        exit(2);
      }
    }

    else if(strcmp(args[2], "SIGALRM") == 0){
      if(kill(atoi(args[1]), 14 ) == -1){
        perror("Failed to kill process");
        exit(2);
      }
    }
    else{
      perror("Use a proper signal\n");
      exit(2);
    }
    sleep(1);
  }
  return 0;
}
