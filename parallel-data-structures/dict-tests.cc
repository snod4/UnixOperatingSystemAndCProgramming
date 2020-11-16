#include <gtest/gtest.h>

#include "dict.hh"
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

/****** Dictionary Invariants ******/

// Invariant 1: Added keys should be found, removed keys should not be found

// Invariant 2: Changed keys should return their new value from dict_get

// Invariant 3: Keys that were not added should not be found

// Invariant 4: Values with different initial characters for keys should be able to be inserted at the same time

/****** Synchronization ******/

// Under what circumstances can accesses to your dictionary structure can proceed in parallel? Answer below.
// When the keys of the different accesses begin with different ASCII characters
//    (not including \0)
// 

// When will two accesses to your dictionary structure be ordered by synchronization? Answer below.
// When the two accesses have keys that begin with the same ASCII character
// 
//

typedef struct argStruct {
  my_dict_t * d;
  int offset;
} argStruct_t;

/****** Begin Tests ******/

// Basic functionality for the dictionary
TEST(DictionaryTest, BasicDictionaryOps) {
  my_dict_t d;
  dict_init(&d);
  
  // Make sure the dictionary does not contain keys A, B, and C
  ASSERT_FALSE(dict_contains(&d, "A"));
  ASSERT_FALSE(dict_contains(&d, "B"));
  ASSERT_FALSE(dict_contains(&d, "C"));
  
  // Add some values
  dict_set(&d, "A", 1);
  dict_set(&d, "B", 2);
  dict_set(&d, "C", 3);
  
  // Make sure these values are contained in the dictionary
  ASSERT_TRUE(dict_contains(&d, "A"));
  ASSERT_TRUE(dict_contains(&d, "B"));
  ASSERT_TRUE(dict_contains(&d, "C"));
  
  // Make sure these values are in the dictionary
  ASSERT_EQ(1, dict_get(&d, "A"));
  ASSERT_EQ(2, dict_get(&d, "B"));
  ASSERT_EQ(3, dict_get(&d, "C"));
  
  // Set some new values
  dict_set(&d, "A", 10);
  dict_set(&d, "B", 20);
  dict_set(&d, "C", 30);
  
  // Make sure these values are contained in the dictionary
  ASSERT_TRUE(dict_contains(&d, "A"));
  ASSERT_TRUE(dict_contains(&d, "B"));
  ASSERT_TRUE(dict_contains(&d, "C"));
  
  // Make sure the new values are in the dictionary
  ASSERT_EQ(10, dict_get(&d, "A"));
  ASSERT_EQ(20, dict_get(&d, "B"));
  ASSERT_EQ(30, dict_get(&d, "C"));
  
  // Remove the values
  dict_remove(&d, "A");
  dict_remove(&d, "B");
  dict_remove(&d, "C");
  
  // Make sure these values are not contained in the dictionary
  ASSERT_FALSE(dict_contains(&d, "A"));
  ASSERT_FALSE(dict_contains(&d, "B"));
  ASSERT_FALSE(dict_contains(&d, "C"));
  
  // Make sure we get -1 for each value
  ASSERT_EQ(-1, dict_get(&d, "A"));
  ASSERT_EQ(-1, dict_get(&d, "B"));
  ASSERT_EQ(-1, dict_get(&d, "C"));
  
  // Clean up
  dict_destroy(&d);
}

void * addFixed (void * args) {
  // Decode the argument to a my_dict_t and an integer
  my_dict_t * d = ((argStruct_t *) args)->d;
  int offset = ((argStruct_t *) args)->offset;

  // Create two keys 
  char random[2];
  random[0]= 'A'+offset;
  random[1] = '\0';


  char A[3];
  A[0] = 'A';
  A[1] = 'A' + offset;
  A[2] = '\0';

  // Add values for A and random to the dictionary
  dict_set(d, random, 2);

  dict_set(d, A, 3);
 
  return NULL;
}

void * removeAndChange (void * args) {
  // Decode the argument to a my_dict_t and an integer
  my_dict_t * d = ((argStruct_t *) args)->d;
  int offset = ((argStruct_t *) args)->offset;

   // Create two keys 
  char random[2];
  random[0]= 'A'+offset;
  random[1] = '\0';


  char A[3];
  A[0] = 'A';
  A[1] = 'A' + offset;
  A[2] = '\0';

  // Remove one value and edit the other
  dict_remove(d, random);
  dict_set(d, A, 24);
  
  
  return NULL;
}

TEST(DictionaryTest, Invariants) {
  // Create two threads
  pthread_t thread1;
  pthread_t thread2;

  // Create and initialize a dictionary
  my_dict_t d;
  dict_init(&d);

  //Initialize a random number generator
  time_t t;
  srand((unsigned) time(&t));

  // Initialize some structs to send to threads
  int offset1 = 18;
  int offset2 = 19;

  argStruct_t * first = (argStruct_t *)malloc(sizeof(argStruct_t));
  first->d = &d;
  first->offset = offset1;

  argStruct_t * second = (argStruct_t *)malloc(sizeof(argStruct_t));
  second->d = &d;
  second->offset = offset2;
  

  // Create and rejoin the threads
  pthread_create(&thread1, NULL, addFixed, (void*) first);
  pthread_create(&thread2, NULL, addFixed, (void*) second);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  // Make sure the threads added the expected values and did not add any others
  ASSERT_TRUE(dict_contains(&d, "AT"));
  ASSERT_TRUE(dict_contains(&d, "AS"));
  ASSERT_TRUE(dict_contains(&d, "T"));
  ASSERT_TRUE(dict_contains(&d, "S"));
  ASSERT_FALSE(dict_contains(&d, "B"));
  ASSERT_FALSE(dict_contains(&d, "Z"));


  // Make sure the threads added the correct values
  ASSERT_EQ(dict_get(&d, "AT"), 3);
  ASSERT_EQ(dict_get(&d, "AS"), 3);
  ASSERT_EQ(dict_get(&d, "T"), 2);
  ASSERT_EQ(dict_get(&d, "S"), 2);


  
  // Create more threads to modify and remove elements
  pthread_create(&thread1, NULL, removeAndChange, (void*) first);
  pthread_create(&thread2, NULL, removeAndChange, (void*) second);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  // Check the modifying and removing
  ASSERT_FALSE(dict_contains(&d, "T"));
  ASSERT_FALSE(dict_contains(&d, "S"));
  ASSERT_EQ(dict_get(&d, "AS"), 24);
  ASSERT_EQ(dict_get(&d, "AT"), 24);
               
  
  // Clean Up
  free(first);
  free(second);

  dict_destroy(&d);
}




