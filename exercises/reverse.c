

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <math.h>

struct coord {
  float x;
  float y;
  float z;
};

int main(int argc, char** argv) {
  // Create an array to hold pipe file descriptors
  int pipefds[2];
  //  int pipefds2[2];
  // Create a pipe
  if(pipe(pipefds)) {
    perror("pipe failed");
    exit(2);
  }
  
  // At this point, pipefds[0] holds the output end of the pipe and pipefds[1] holds the input end. Split them out for easier code reading
  int from_child = pipefds[0];
  int to_parent = pipefds[1];
  
  // Now call fork to create a child process
  pid_t child_pid = fork();
  if(child_pid == -1) {
    perror("fork failed");
    exit(2);
  } else if(child_pid == 0) {
    // In the child process
    
    // Close the end of the pipe the child does not use
    close(from_child);
    
    // Make space for a coordinate value
    dup2(STDOUT_FILENO,to_parent);
    exec(args[1], NULL);
    
  } else {
    // In the parent process
    
    // Close the end of the pipe the parent does not use
    close(to_child);
    
    // Send ten structs to the child process
    
      // Create a struct value to send over the pipe
 
      int size = 0;
      int currentSize = 20;
      char * input = (char *)malloc(sizeof(char)*currentSize);
      int prevSize = 0;
      while((size+=read(from_child, &input, 10*sizeof(char))) > prevSize){

        if(size >= currentSize - 10){
          currentSize*=2;
          char * temp;
          temp = (char *)malloc(sizeof(char)*currentSize);
          strcpy(input, temp, currentSize/2);
        }

        

        prevSize = size;
      }
      input[size] = '\0';
      char ouput[strlen(input)];

      for(int i = size - 1; i > 0; i--){
        ouput[size - 1 - i] = input[i];
      } 

      printf("%s\n", output);
      // Pause for half a second
    
    
    // Close the pipe input
    close(from_child);
  }
  
  return 0;
}

