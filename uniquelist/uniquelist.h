#ifndef UNIQUELIST_H
#define UNIQUELIST_H


typedef struct uniquelist {
  int val; //integer value
  struct uniquelist * next; //points to the next element in linked list
  struct uniquelist * last; //points to last element in list, also provides
                            //way to check if the list has any values in it
                            // only the head of the list has a non-null value to
                            // the last pointer
} uniquelist_t;

/// Initialize a new uniquelist
void uniquelist_init(uniquelist_t* s);

/// Destroy a uniquelist
void uniquelist_destroy(uniquelist_t* s);

/// Add an element to a uniquelist, unless it's already in the uniquelist
void uniquelist_insert(uniquelist_t* s, int n);

/// Print all the numbers in a uniquelist
void uniquelist_print_all(uniquelist_t* s);

#endif
