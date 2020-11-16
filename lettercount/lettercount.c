#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>

#define PAGE_SIZE 0x1000
#define ROUND_UP(x,y) ((x) % (y) == 0 ? (x) : (x) + ((y) - (x) % (y)))

/// The number of times we've seen each letter in the input, initially zero
size_t letter_counts[26] = {0};
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct args{ //packs up the arguments of countFunction so they can be passed into the new thread
  int startIndex; 
  char * file_data;
  int partition;
}args_t;


void * countFunction(void *  arg){

  //unpacks arg to get arguments
  int partition = ((args_t *)arg)->partition; //the number of items to read
  int startIndex =  ((args_t *)arg)->startIndex; //the start of the section this thread is supposed to read
  char * file_data =  ((args_t *)arg)->file_data; //the file data 
  

  size_t local_letter_counts[26] = {0}; //initalizes a local count so the thread does not have to lock the global count often
  size_t i = startIndex;
  size_t pos = startIndex; //current position in file_data
  while(i < startIndex + partition){ //go unitl the thread passes the part it is supposed to read
    //count the number of each character
    for(; i< pos + 100 && i<startIndex + partition; i++){ //read 100 characters max before updating global count
      char c = file_data[i];
      if(c >= 'a' && c <= 'z') {
      local_letter_counts[c - 'a']++;
      } else if(c >= 'A' && c <= 'Z') {
        local_letter_counts[c - 'A']++;
      }
    }

    //start updating the global letter count array
    pthread_mutex_lock(&lock); 

    for(int g = 0; g < 26; g++){
      letter_counts[g] += local_letter_counts[g];
      local_letter_counts[g] = 0;
    }
    
    pthread_mutex_unlock(&lock);
    //finish updating global letter count array

    
    pos = i; //shift pos by the amount already traversed
  }

  return NULL;
}

/**
 * This function should divide up the file_data between the specified number of
 * threads, then have each thread count the number of occurrences of each letter
 * in the input data. Counts should be written to the letter_counts array. Make
 * sure you count only the 26 different letters, treating upper- and lower-case
 * letters as the same. Skip all other characters.
 *
 * \param num_threads   The number of threads your program must use to count
 *                      letters. Divide work evenly between threads
 * \param file_data     A pointer to the beginning of the file data array
 * \param file_size     The number of bytes in the file_data array
 */
void count_letters(int num_threads, char* file_data, off_t file_size) {
  // TODO: Implement this function. You will have to write at least one
  //       other function to run in each of your threads



  int partition = file_size/num_threads; //determine size of what each thread should read

  pthread_t Arr[num_threads]; //keeps track of threads
  args_t arg[num_threads]; //keeps track of arguments for threads

  for(int i = 0; i < num_threads -1; i++){
    //intializes each argument and creates each thread but the last one
    arg[i].startIndex = partition * i;
    arg[i].file_data = file_data;
    arg[i].partition = partition;
    pthread_create(&Arr[i], NULL, countFunction, &arg[i]);
  }
  
  int comparisonSize = partition * num_threads; //used to see if one thread needs to have a larger partition than the others

  if(partition * num_threads < file_size){ //the case where each partition put together does not sum up to the file size, one partition needs to be larger
    arg[num_threads-1].startIndex = partition * (num_threads - 1);
    arg[num_threads-1].file_data = file_data;
    arg[num_threads-1].partition = partition + (file_size - comparisonSize);
    pthread_create(&Arr[num_threads-1], NULL, countFunction, &arg[num_threads-1]);
  }
  else{ //the optimal final case where the partitions summed up equal the file size
    arg[num_threads-1].startIndex = partition * (num_threads - 1);
    arg[num_threads-1].file_data = file_data;
    arg[num_threads-1].partition = partition - (comparisonSize - file_size);
    pthread_create(&Arr[num_threads-1], NULL, countFunction, &arg[num_threads-1]);
  }

  for(int i = 0; i < num_threads; i++){ //joins each thread to the main 
    pthread_join(Arr[i], NULL);
  }
  
}

/**
 * Show instructions on how to run the program.
 * \param program_name  The name of the command to print in the usage info
 */
void show_usage(char* program_name) {
  fprintf(stderr, "Usage: %s <N> <input file>\n", program_name);
  fprintf(stderr, "    where <N> is the number of threads (1, 2, 4, or 8)\n");
  fprintf(stderr, "    and <input file> is a path to an input text file.\n");
}

int main(int argc, char** argv) {
  // Check parameter count
  if(argc != 3) {
    show_usage(argv[0]);
    exit(1);
  }
  
  // Read thread count
  int num_threads = atoi(argv[1]);
  if(num_threads != 1 && num_threads != 2 && num_threads != 4 && num_threads != 8) {
    fprintf(stderr, "Invalid number of threads: %s\n", argv[1]);
    show_usage(argv[0]);
    exit(1);
  }
  
  // Open the input file
  int fd = open(argv[2], O_RDONLY);
  if(fd == -1) {
    fprintf(stderr, "Unable to open input file: %s\n", argv[2]);
    show_usage(argv[0]);
    exit(1);
  }
  
  // Get the file size
  off_t file_size = lseek(fd, 0, SEEK_END);
  if(file_size == -1) {
    fprintf(stderr, "Unable to seek to end of file\n");
    exit(2);
  }

  // Seek back to the start of the file
  if(lseek(fd, 0, SEEK_SET)) {
    fprintf(stderr, "Unable to seek to the beginning of the file\n");
    exit(2);
  }
  
  // Load the file with mmap
  char* file_data = mmap(NULL, ROUND_UP(file_size, PAGE_SIZE), PROT_READ, MAP_PRIVATE, fd, 0);
  if(file_data == MAP_FAILED) {
    fprintf(stderr, "Failed to map file\n");
    exit(2);
  }
  
  // Call the function to count letter frequencies
  count_letters(num_threads, file_data, file_size);
  
  // Print the letter counts
  for(int i=0; i<26; i++) {
    printf("%c: %lu\n", 'a' + i, letter_counts[i]);
  }
  
  return 0;
}
