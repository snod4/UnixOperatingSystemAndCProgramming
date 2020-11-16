#include <gtest/gtest.h>

#include<pthread.h>
#include "queue.hh"
#include<stdio.h>
#include<stdlib.h>
#include<time.h>

/****** Queue Invariants ******/

// Invariant 1: For every value V enqueued p times and dequeued q times,
//              where p >= q, there are p-q copies of V in the queue,

// Invariant 2: Every value dequeued must have once been enqueued

// Invariant 3: Items are dequeued in the order they were enqueued


/****** Begin Tests ******/

typedef struct myargs {
  int * onArr;
  int * offArr;
  my_queue_t * q;
  bool odd;
} myargs_t;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// Basic queue functionality
TEST(QueueTest, BasicQueueOps) {
  my_queue_t q;
  queue_init(&q);
  
  // Make sure the queue is empty
  ASSERT_TRUE(queue_empty(&q));
  
  // Make sure taking from the queue returns -1
  ASSERT_EQ(-1, queue_take(&q));
  
  // Add some items to the queue
  queue_put(&q, 1);
  queue_put(&q, 2);
  queue_put(&q, 3);
  
  // Make sure the queue is not empty
  ASSERT_FALSE(queue_empty(&q));
  
  // Take the values from the queue and check them
  ASSERT_EQ(1, queue_take(&q));
  ASSERT_EQ(2, queue_take(&q));
  ASSERT_EQ(3, queue_take(&q));
  
  // Make sure the queue is empty
  ASSERT_TRUE(queue_empty(&q));
  
  // Clean up
  queue_destroy(&q);
}

void * enqueueAndDequeue(void * args) {
  //Decompose the myargs_t struct passed in as a parameter
  int * globalNumAdded = ((myargs_t *)args)->onArr;
  int * globalNumRemoved = ((myargs_t *)args)->offArr;
  my_queue_t * q = ((myargs_t *)args)->q;

  // Enqueue some random integers between 0 and 100
  for(int i = 0; i < 1000; i++) {
    int adder = rand() % 100;
    queue_put(q, adder);
    pthread_mutex_lock(&lock);
    globalNumAdded[adder]++;
    pthread_mutex_unlock(&lock);
  }

  // Dequeue some of the objects in threads
  for(int j = 0; j < 500; j++) {
    int removed = queue_take(q);
    pthread_mutex_lock(&lock);
    if (removed == -1) {
      globalNumRemoved[100]++;
    } else {
    globalNumRemoved[removed]++;
    }
    pthread_mutex_unlock(&lock);
  }
  return NULL;
}


// Testing Invariants 1 and 2
TEST(QueueTest, Invariants1And2) {
  // Initialize variables
  my_queue_t *  q = (my_queue_t *)malloc(sizeof(my_queue_t));
  int * globalNumAdded = (int *)malloc(100 * sizeof(int));
  int * globalNumRemoved = (int *)malloc(101 * sizeof(int));
  time_t t;
  srand((unsigned) time(&t));

  // Initialize the arguments struct to pass to the threads
  myargs_t * args = (myargs_t *) malloc(sizeof(myargs_t));
  args->q = q;
  args->onArr = globalNumAdded;
  args->offArr = globalNumRemoved;
  queue_init(q);

  // Initialize the arrays of pushed and popped values
  for(int k = 0; k < 100; k++) {
    globalNumAdded[k] = 0;
    globalNumRemoved[k] = 0;
  }
  globalNumRemoved[100] = 0;
                           

  //Run threads                                    
  pthread_t first;
  pthread_t second;

  pthread_create(&first, NULL, enqueueAndDequeue, args);
  pthread_create(&second, NULL, enqueueAndDequeue, args);

  pthread_join(first, NULL);
  pthread_join(second, NULL);

  
  // Dequeue the remaining 
  for(int i = 0; i < 1000; i++) {
    int removed = queue_take(q);
    if (removed == -1) {
      globalNumRemoved[100]++;
    } else {
      globalNumRemoved[removed]++;
    }
  }

  //Make Sure every value was pushed and popped the same number of times
  for(int j = 0; j < 100; j++) {
    ASSERT_EQ(globalNumAdded[j], globalNumRemoved[j]);
  }
  ASSERT_EQ(globalNumRemoved[100], 0);
  
  //Clean Up
  queue_destroy(q);
  free(globalNumAdded);
  free(globalNumRemoved);
}

void* enqueue(void * args) {
  my_queue_t * q = ((myargs_t *) args)->q;
  bool odd = ((myargs_t *) args)->odd;

  for(int i = odd; i < 1000; i+=2) {
    queue_put(q, i);
  }
  return NULL;
}


//checks to make sure that items are popped off in some order
//returns NULL if the order is violated, otherwise it returns a int
void * takeOrder(void * args) {
  my_queue_t * stack = (my_queue_t*) args;
  int *  prev = (int *) malloc(sizeof(int));
  *prev = -1;

  //dequeues items and ensures that the current item is larger than the previous
  for(int i = 0; i < 50; i++){
    int cur = queue_take(stack);
    if(cur > *prev) {
      *prev = cur;
    } else {
      return NULL;
    }
  }
  
  return (void *) prev;
}



TEST(QueueTest, Invariant3) {
  my_queue_t * q = (my_queue_t *)malloc(sizeof(my_queue_t));
  queue_init(q);
  pthread_t first;
  pthread_t second;

  myargs_t args1;
  myargs_t args2;

  args1.q = q;
  args2.q = q;
  args1.odd = 0;
  args2.odd = 1;
  //checks to ensure parallel enqueuing does not compromise the order of dequeuing 
  pthread_create(&first, NULL, enqueue, &args1);
  pthread_create(&second, NULL, enqueue, &args2);

  pthread_join(first, NULL);
  pthread_join(second, NULL);

  //checks that order was maintained within the enqueuing threads
  int lastOdd = -1;
  int lastEven = -1;
  int mostRecent;
  for(int i = 0; i < 1000; i++) {
    mostRecent = queue_take(q);
    if(mostRecent%2 == 0){
      ASSERT_TRUE(mostRecent > lastEven);
      lastEven = mostRecent;
    }
    else{
      ASSERT_TRUE(mostRecent > lastOdd);
      lastOdd = mostRecent;
    }
  }

  ASSERT_TRUE(queue_empty(q));
    //Tests for parallel dequeues 
  for(int i = 0; i < 1000; i++) {
    queue_put(q, i);
  }
  pthread_create(&first, NULL, takeOrder, (void *) q);
  pthread_create(&second, NULL, takeOrder, (void *) q);

  //when not NULL, these variables imply that takeOrder maintained the queue order
  void * thing1 = NULL;
  void * thing2 = NULL;
  pthread_join(first, &thing1);
  pthread_join(second, &thing2);

  //check that parallel dequeuing maintains order
  ASSERT_TRUE(thing1 != NULL);
  ASSERT_TRUE(thing2 != NULL);
  //Clean up
  queue_destroy(q);
}
