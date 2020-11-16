#define _GNU_SOURCE
#include "lazycopy.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
// a node of the tree we use to do bookkeeping
typedef struct tree {
  void *chunkStart;
  struct tree *left, *right;
} Tree;

Tree *  chunkList = NULL; //declares a global binary tree that keeps track of lazy-copied chunks
void ** chunkArr = NULL;
int chunkArrSize = 0;
void add(void * ptr){
  printf("Adding %d\n", chunkArrSize);
  chunkArr = realloc(chunkArr,(size_t) (chunkArrSize + 1));
  if(chunkArr == NULL){
    perror("Realloc failed");
    exit(2);
  }
  chunkArr[chunkArrSize++] = ptr;
  
}

void * getChunkStartPtr(void * ptr){
  for(int i = 0; i < chunkArrSize; i++){
    if((intptr_t) ptr >= (intptr_t) chunkArr[i] && (intptr_t) ptr < (intptr_t) (chunkArr[i] + CHUNKSIZE)){
      return chunkArr[i];
    }
  }

  perror("Seg fault not in allocated chunk");
  exit(2);
}
// add a chunk of memory to the tree
Tree *addChunk(Tree *root, void* chunkStart){
  if(root == NULL){
    root = (Tree*)malloc(sizeof(Tree));
    root->chunkStart = chunkStart;
    root->left = NULL;
    root->right = NULL;
  } else if ((intptr_t)chunkStart <(intptr_t) (root->chunkStart)){
    // Branch Left
    root->left = addChunk(root->left, chunkStart);
  } else {
    // Branch Right, ignore duplicates as that makes no sense
    root->right = addChunk(root->right, chunkStart);
  }
  return root;
}

// get the start position of a chunk from a pointer into that chunk,
// using our bookkeeping tree
void *getChunkStart(void *ptr, Tree *root){
  if((intptr_t) ptr < (intptr_t) (root->chunkStart)){
    // Branch Left
    return getChunkStart(ptr, root->left);
  } else if ((intptr_t)ptr >= (intptr_t)((root->chunkStart)+CHUNKSIZE)){
    // Branch Right
    return getChunkStart(ptr, root->right);
  } else {
    // We are in the right chunk
    return root->chunkStart;
  }
}  

void signalHandler(int signal, siginfo_t* info, void*ctx){
  // Later, if either copy is written to you will need to:
  // 1. Save the contents of the chunk elsewhere (a local array works well)
  // 2. Use mmap to make a writable mapping at the location of the chunk that was written
  // 3. Restore the contents of the chunk to the new writable mapping
   void* chunkStart = getChunkStart(info->si_addr, chunkList);
  //  void * chunkStart = getChunkStartPtr(info->si_addr);
  
 unsigned char byteArr[CHUNKSIZE];
 memmove(byteArr, chunkStart, CHUNKSIZE);
 mmap(chunkStart, CHUNKSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED | MAP_FIXED, -1, 0);
 memmove(chunkStart, byteArr, CHUNKSIZE);
 
}
/**
 * This function will be called at startup so you can set up a signal handler.
 */
void chunk_startup() {
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_sigaction = signalHandler; //assign the action of the sigaction struct to our signalHandler method
  sa.sa_flags = SA_SIGINFO;
  if(sigaction(SIGSEGV, &sa, NULL) != 0){ //check for error in setting the signal handler
    perror("sigaction failed");
    exit(2);
  }
}

/**
 * This function should return a new chunk of memory for use.
 *
 * \returns a pointer to the beginning of a 64KB chunk of memory that can be read, written, and copied
 */
void* chunk_alloc() {
  // Call mmap to request a new chunk of memory. See comments below for description of arguments.
  void* result = mmap(NULL, CHUNKSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
  // Arguments:
  //   NULL: this is the address we'd like to map at. By passing null, we're asking the OS to decide.
  //   CHUNKSIZE: This is the size of the new mapping in bytes.
  //   PROT_READ | PROT_WRITE: This makes the new reading readable and writable
  //   MAP_ANONYMOUS | MAP_SHARED: This mapes a new mapping to cleared memory instead of a file,
  //                               which is another use for mmap. MAP_SHARED makes it possible for us
  //                               to create shared mappings to the same memory.
  //   -1: We're not connecting this memory to a file, so we pass -1 here.
  //   0: This doesn't matter. It would be the offset into a file, but we aren't using one.
  
  // Check for an error
  if(result == MAP_FAILED) {
    perror("mmap failed in chunk_alloc");
    exit(2);
  }
  
  // Everything is okay. Return the pointer.
  return result;
}

/**
 * Create a copy of a chunk by copying values eagerly.
 *
 * \param chunk This parameter points to the beginning of a chunk returned from chunk_alloc()
 * \returns a pointer to the beginning of a new chunk that holds a copy of the values from
 *   the original chunk.
 */
void* chunk_copy_eager(void* chunk) {
  // First, we'll allocate a new chunk to copy to
  void* new_chunk = chunk_alloc();
  
  // Now copy the data
  memcpy(new_chunk, chunk, CHUNKSIZE);
  
  // Return the new chunk
  return new_chunk;
}

/**
 * Create a copy of a chunk by copying values lazily.
 *
 * \param chunk This parameter points to the beginning of a chunk returned from chunk_alloc()
 * \returns a pointer to the beginning of a new chunk that holds a copy of the values from
 *   the original chunk.
 */
void* chunk_copy_lazy(void* chunk) {
  // Just to make sure your code works, this implementation currently calls the eager copy version
  //return chunk_copy_eager(chunk);

  void * ptr =  mremap(chunk, 0, CHUNKSIZE, MREMAP_MAYMOVE); //MREMAP_MAYMOVE where to put it
  mprotect(chunk, CHUNKSIZE, PROT_READ);
  mprotect(ptr, CHUNKSIZE, PROT_READ);
  //add(chunk);
  // add(ptr);
    chunkList = addChunk(chunkList, chunk);
    chunkList = addChunk(chunkList, ptr);
  return ptr;
  
  // Your implementation should do the following:
  // 1. Use mremap to create a duplicate mapping of the chunk passed in
  // 2. Mark both mappings as read-only
  // 3. Keep some record of both lazy copies so you can make them writable later.
  //    At a minimum, you'll need to know where the chunk begins and ends.
  
}
