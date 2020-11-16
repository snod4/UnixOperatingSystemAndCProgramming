#ifndef DICT_H
#define DICT_H
#define CHARS 256
#include <pthread.h>
#include <stdbool.h>

//node for each linked_list in the dictionary
typedef struct node {
  int value;
  char * key;
  struct node * next;
} node_t;
//a bin for each index of the dictionary
typedef struct linked_list {
  pthread_mutex_t lock;
  node_t * head;
} linked_list_t;

//dictionary structure
//indexed by the first character in the key
typedef struct my_dict {
  struct linked_list array[CHARS];
} my_dict_t;

// Initialize a dictionary
void dict_init(my_dict_t* dict);

// Destroy a dictionary
void dict_destroy(my_dict_t* dict);

// Set a value in a dictionary
void dict_set(my_dict_t* dict, const char* key, int value);

// Check if a dictionary contains a key
bool dict_contains(my_dict_t* dict, const char* key);

// Get a value in a dictionary
int dict_get(my_dict_t* dict, const char* key);

// Remove a value from a dictionary
void dict_remove(my_dict_t* dict, const char* key);

#endif
