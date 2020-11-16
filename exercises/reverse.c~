

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
  int pipefds2[2];
  // Create a pipe
  if(pipe(pipefds)) {
    perror("pipe failed");
    exit(2);
  }

  if(pipe(pipefds2)) {
    perror("pipe failed");
    exit(2);
  }
  
  
  // At this point, pipefds[0] holds the output end of the pipe and pipefds[1] holds the input end. Split them out for easier code reading
  int from_parent = pipefds[0];
  int to_child = pipefds[1];
  int from_child = pipefds2[0];
  int to_parent = pipefds2[1];
  
  // Now call fork to create a child process
  pid_t child_pid = fork();
  if(child_pid == -1) {
    perror("fork failed");
    exit(2);
  } else if(child_pid == 0) {
    // In the child process
    
    // Close the end of the pipe the child does not use
    close(to_child);
    close(from_child);
    
    // Make space for a coordinate value
    struct coord c;
    
    // Now read in a loop
    while(read(from_parent, &c, sizeof(struct coord)) == sizeof(struct coord)) {
      // Display the value received
      printf("Child: Received coordinate <%.2f, %.2f, %.2f>\n", c.x, c.y, c.z);
      
      double  magnitude = sqrt((c.x*c.x + c.y *c.y +c.z*c.z));
      if(write(to_parent, &magnitude, sizeof(double)) != sizeof(double)) {
        perror("Failed to write to pipe");
        exit(2);
      }
    }
    
    // Close the pipe output
    close(to_parent);
    close(from_parent);
    
  } else {
    // In the parent process
    
    // Close the end of the pipe the parent does not use
    close(from_parent);
    
    // Send ten structs to the child process
    for(int i=0; i<10; i++) {
      // Create a struct value to send over the pipe
      struct coord c;
      c.x = i;
      c.y = i*2;
      c.z = 3*i/2;
      
      // Print the value we're sending
      printf("Parent: Sending coordinate <%.2f, %.2f, %.2f>\n", c.x, c.y, c.z);
      
      // Send the struct
      
      if(write(to_child, &c, sizeof(struct coord)) != sizeof(struct coord)) {
        perror("Failed to write to pipe");
        exit(2);
      }
      double parent_magnitude;
      if(read(from_child, &parent_magnitude, sizeof(double)) != sizeof(double)){
        perror("Failed to read from child\n");
        exit(2);
      }
      printf("Magnitude received from child: %lf\n",parent_magnitude);
      
      // Pause for half a second
      usleep(500000);
    }
    
    // Close the pipe input
    close(from_child);
    close(to_child);
  }
  
  return 0;
}

