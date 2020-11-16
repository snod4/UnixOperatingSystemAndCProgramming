#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int main() {
  // Create an empty signal set
  sigset_t signals;
  if(sigemptyset(&signals)) {
    perror("sigemptyset failed");
    exit(2);
  }

  // Add SIGINT to the signal set
  if(sigaddset(&signals, SIGINT)) {
    perror("sigaddset failed");
    exit(2);
  }

  // Mask the signals in our set so they do not run signal handlers
  if(sigprocmask(SIG_BLOCK, &signals, NULL)) {
    perror("sigprocmask failed");
    exit(2);
  }

  // Loop ten times
  for(int i=0; i<10; i++) {
    // Wait for a signal in the set
    int signal;
    if(sigwait(&signals, &signal)) {
      perror("sigwait failed");
      exit(2);
    }

    // Print a message
    printf(" %d: Received signal %d\n", i, signal);
  }

  return 0;
}
