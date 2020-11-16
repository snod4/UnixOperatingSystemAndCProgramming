#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_contents(uint8_t* data, size_t size);

int main(int argc, char** argv) {
  // Make sure we have a file input
  if(argc != 2) {
    fprintf(stderr, "Please specify an input filename.\n");
    exit(1);
  }
  
  // Try to open the file
  FILE* input = fopen(argv[1], "r");
  if(input == NULL) {
    perror("Unable to open input file");
    exit(1);
  }
  
  // Seek to the end of the file so we can get its size
  if(fseek(input, 0, SEEK_END) != 0) {
    perror("Unable to seek to end of file");
    exit(2);
  }
  
  // Get the size of the file
  size_t size = ftell(input);
  
  // Seek back to the beginning of the file
  if(fseek(input, 0, SEEK_SET) != 0) {
    perror("Unable to seek to beginning of file");
    exit(2);
  }
  
  // Allocate a buffer to hold the file contents. We know the size in bytes, so
  // there's no need to multiply to get the size we pass to malloc in this case.
  uint8_t* data = malloc(size);
  
  // Read the file contents
  if(fread(data, 1, size, input) != size) {
    fprintf(stderr, "Failed to read entire file\n");
    exit(2);
  }
  
  // Make sure the file starts with the .ar file signature
  if(memcmp(data, "!<arch>\n", 8) != 0) {
    fprintf(stderr, "Input file is not in valid .ar format\n");
    exit(1);
  }
  
  // Call the code to print the archive contents
  print_contents(data, size);
  
  // Clean up
  free(data);
  fclose(input);
  
  return 0;
}

/**
 * This function should print the name of each file in the archive followed by its contents.
 *
 * \param data This is a pointer to the first byte in the file.
 * \param size This is the number of bytes in the file.
 */
void print_contents(uint8_t* data, size_t size) {
  int currentSize = 8; //skips the firsts 8 bits
  
  while(currentSize < size){ //loops through each sub .ar file
    char fileSizeStr[11]; //size of the file will be at most 10 bytes
    char fileName[16]; //the file name will be at most 15 bytes long
    int fileSize;
    int i; //denotes byte location in the file
    for(i = currentSize; i < 16+currentSize; i++){
      if(data[i] != '/'){ //goes unitl the ending character '/' for file name
        fileName[i-currentSize] = data[i];
      }
      else{
        fileName[i-currentSize] = '\0';
        break;
      }
      
    }
    
    for(i = 48+currentSize; i < 58+currentSize; i++){ //48 is the start position of the size info relative to the start of that .ar file, 58 is the end
      if(data[i] == ' '){
        i--;
        break;
      }
      else{
        fileSizeStr[i-currentSize - 48] = data[i];
      }
    }

    fileSizeStr[i-currentSize +1 - 48] = '\0'; 
    fileSize = atoi(fileSizeStr);
    

    char fileData[fileSize+1];
    for(i = 60+currentSize; i < 60 + fileSize+currentSize; i++){
      fileData[i - 60 - currentSize] = data[i];
        }
    fileData[fileSize] = '\0';
    if(data[i] == '\n'){ //accounts for new-line character when updating currentSize in order to skip reading it
      fileSize++;
    }
    
    

    printf("%s\n", fileName);
    printf("%s\n", fileData);
    currentSize += fileSize + 60; //adds 60 to currentSize to account for the size of the file header and fileSize to account for the size of the file contents
  
  }
  
}
