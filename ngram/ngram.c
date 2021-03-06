#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
  // Make sure the program is run with an N parameter
  if(argc != 2) {
    fprintf(stderr, "Usage: %s N (N must be >= 1)\n", argv[0]);
    exit(1);
  }
  
  // Convert the N parameter to an integer
  int N = atoi(argv[1]);
  
  // Make sure N is >= 1
  if(N < 1) {
    fprintf(stderr, "Invalid N value %d\n", N);
    exit(1);
  }
  
  // TODO: read from standard input and print out ngrams until we reach the end of the input

  char strChunk[N+1];//creates space for an ngram including a null character
  strChunk[N] = '\0';
  int counter = 0; //exists to differentiate between the first reading of N
                   //characters and subsequent readings
  
  char readChar = fgetc(stdin); 
  while(!feof(stdin)){
      if(counter >= N){
        //after first read-in of N characters, each subsequent character read-in
        //requires a shift of all previous characters one space backward in the
        //char array to create space
        for(int i = 1; i < N; i++){
          strChunk[i-1] = strChunk[i];
        }
        strChunk[N-1] = readChar;
      }
      else{
        //This portion exists for the first N characters read in
        strChunk[counter] = readChar;
        counter++;
      }
      if(counter >= N){
        //prints only when the strChunk guaranteed to be full
        printf("%s\n", strChunk);
      }
    
    readChar = fgetc(stdin);
    
 
  }
  
  return 0;
}
