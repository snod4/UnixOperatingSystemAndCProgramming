#include <gtest/gtest.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "stack.hh"

typedef struct myargs{
  my_stack_t * stack;
  int * pushArr;
  int * popArr;
  bool odd;
}myargs_t;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/****** Stack Invariants ******/

// Invariant 1
// For every value V that has been pushed onto the stack p times and returned by pop q times, there must be p-q copies of this value on the stack. This only holds if p >= q.

// Invariant 2
// No value should ever be returned by pop if it was not first passed to push by some thread.

// Invariant 3
// If a thread pushes value A and then pushes value B, and no other thread pushes these specific values, A must not be popped from the stack before popping B.

/****** Begin Tests ******/

// A simple test of basic stack functionality
TEST(StackTest, BasicStackOps) {
  // Create a stack
  my_stack_t s;
  stack_init(&s);
  
  // Push some values onto the stack
  stack_push(&s, 1);
  stack_push(&s, 2);
  stack_push(&s, 3);
  
  // Make sure the elements come off the stack in the right order
  ASSERT_EQ(3, stack_pop(&s));
  ASSERT_EQ(2, stack_pop(&s));
  ASSERT_EQ(1, stack_pop(&s));
  
  // Clean up
  stack_destroy(&s);
}

//Pushes and pops random values into the stack.
void * pushAndPopRandom(void * args) {

  //gets the references to the global pop and push arrays
  int * globalNumPop = ((myargs_t *)args)->popArr;
  int * globalNumPush = ((myargs_t *)args)->pushArr;
  my_stack_t * stack = ((myargs_t *)args)->stack;

  //creates local push and pop arrays
  int numPop[101] = {0};
  int numPush[100] = {0};
  int val;

  //pushes random values
  for(int i = 0; i < 100; i++) {
    val = rand() % 100;
    numPush[val]++;
    stack_push(stack,val); 
  }
  
  //pops random values
  for(int i = 0; i < 100; i++) {
    val = stack_pop(stack);
    if(val == -1) {
      numPop[100]++;
    }
    else{
      numPop[val]++;
    }
  }

  
  pthread_mutex_lock(&lock);
  //updates global push and pop counts
  for(int i = 0; i < 100; i++){
    globalNumPop[i] += numPop[i];
    globalNumPush[i] += numPush[i];
  }
  
  globalNumPop[100] += numPop[100];
  
  pthread_mutex_unlock(&lock);
 
  return NULL;
}

TEST(StackTest, FirstInvariant) {
  // Create a stack
  my_stack_t s;
  stack_init(&s);

  //Initialize a random number generator
  time_t t;
  srand((unsigned) time(&t));
  
  // Declare two threads
  pthread_t first;
  pthread_t second;

  //create global push and pop arrays, zeros them
  int numPush[100] = {0};
  int numPop[101] = {0};
  
  //initalizes the argument to pass into the thread function
  myargs_t args1;
  args1.stack = &s;
  args1.popArr = numPop;
  args1.pushArr = numPush;
  
  //Create two threads
  pthread_create(&first, NULL, pushAndPopRandom, &args1);
  pthread_create(&second, NULL, pushAndPopRandom, &args1);

  //join the two threads once they are finished
  pthread_join(first, NULL);
  pthread_join(second, NULL);

  //check to see that the values pushed onto the stack are popped off in equal amounts
  for(int i = 0; i < 100; i++){
    ASSERT_EQ(numPush[i], numPop[i]);
  }
  //ensures that the stack never returned -1 for a pop because equal numbers were pushed and popped. Somewhat redundant but here just to be safe
  ASSERT_EQ(numPop[100], 0);

  stack_destroy(&s);
}

//pushes even or odd values in ascending order
void * justPushInOrder(void * args) {
  //unpack arguements
  my_stack_t * stack = ((myargs_t*) args)->stack;
  //determine whether this thread pushes even or odd values
  int odd = ((myargs_t*) args)->odd;
  //push 25 even or odd values in ascending order
  for(int i = odd; i < 50; i += 2) {
    stack_push(stack, i);
  }
  return NULL;
}

//checks to make sure that items are popped off in some order
//returns null if the order is violated, otherwise it returns a int
void * popOrder(void * args) {
  my_stack_t * stack = (my_stack_t*) args;
  int *  prev = (int *) malloc(sizeof(int));
  *prev = 101;

  //pops items off and ensures that the current item is smaller than the previous
  for(int i = 0; i < 50; i++){
    int cur = stack_pop(stack);
    if(cur < *prev) {
      *prev = cur;
    } else {
      return NULL;
    }
  }
  
  return (void *) prev;
}

TEST(StackTest, ThirdInvariant) {
 // Create a stack
  my_stack_t s;
  stack_init(&s);

  //Initialize a random number generator
  time_t t;
  srand((unsigned) time(&t));
  
  // Declare two threads
  pthread_t first;
  pthread_t second;

  //Declare and initialize arguements for thread functions
  myargs_t args1;
  args1.stack = &s;
  args1.odd = 1;

  myargs_t args2;
  args2.stack = &s;
  args2.odd = 0;

  //Tests for parallel pushes
  // Create the threads
  pthread_create(&first, NULL, justPushInOrder, &args1);
  pthread_create(&second, NULL, justPushInOrder, &args2);

  // Join the threads back together
  pthread_join(first, NULL);
  pthread_join(second, NULL);


  //Tests to see that the intended order of the stack is maintained
  int lastEven = INT_MAX;
  int lastOdd = INT_MAX;
  int mostRecent;
  for(int i = 0; i < 100; i++) {
    mostRecent = stack_pop(&s);
    if(mostRecent%2 == 0){
      ASSERT_TRUE(mostRecent <= lastEven);
      lastEven = mostRecent;
    }
    else{
      ASSERT_TRUE(mostRecent <= lastOdd);
      lastOdd = mostRecent;
    }
  }

  //Ensures the number of items pushed on are popped off the stack
  ASSERT_TRUE(stack_empty(&s));

  //Tests for parallel pops 
  for(int i = 0; i < 100; i++) {
    stack_push(&s, i);
  }
  pthread_create(&first, NULL, popOrder, (void *) &s);
  pthread_create(&second, NULL, popOrder, (void *) &s);

  //when not NULL, these variables imply that popOrder maintained the stack order
  void * thing1;
  void * thing2;
  pthread_join(first, &thing1);
  pthread_join(second, &thing2);

  //check that parallel popping maintains order
  ASSERT_TRUE(thing1 != NULL);
  ASSERT_TRUE(thing2 != NULL);

  //Clean up
  stack_destroy(&s);
}

// Another test case
TEST(StackTest, EmptyStack) {
  // Create a stack
  my_stack_t s;
  stack_init(&s); 
  
  // The stack should be empty
  ASSERT_TRUE(stack_empty(&s));
  
  // Popping an empty stack should return -1
  ASSERT_EQ(-1, stack_pop(&s));
  
  // Put something on the stack
  stack_push(&s, 0);
  
  // The stack should not be empty
  ASSERT_FALSE(stack_empty(&s));
  
  // Pop the element off the stack.
  // We're just testing empty stack behavior, so there's no need to check the resulting value
  stack_pop(&s);
  
  // The stack should be empty now
  ASSERT_TRUE(stack_empty(&s));
  
  // Clean up
  stack_destroy(&s);
}
