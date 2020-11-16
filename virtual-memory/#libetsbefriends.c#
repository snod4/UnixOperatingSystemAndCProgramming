#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>

const char * const messages[] = {"You got this?\n",
                                 "Better luck next time...\n",
                                 "Good try :)\n", "Alas...\n",
                                 "Bummer :(\n", "Bummer, good try\n",
                                 "Nth time's the charm\n",
                                 "Take your time, that memory isn't going anywhere >:(\n",
                                 "This handler believes in you!\n"};

void signalHandler(int signal, siginfo_t* info, void*ctx){
  printf("%s", messages[rand()%9]);
  exit(2);
}

// TODO: Implement your signal handling code here!
__attribute__((constructor)) void init() {
  srand(time(0)); 
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_sigaction = signalHandler; //assign the action of the sigaction struct to our signalHandler method
  sa.sa_flags = SA_SIGINFO;
  if(sigaction(SIGSEGV, &sa, NULL) != 0){ //check for error in setting the signal handler
    perror("sigaction failed");
    exit(2);
  }
}


