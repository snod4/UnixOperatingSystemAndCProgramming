#include "uniquelist.h"

#include <stdio.h>
#include <stdlib.h>



/// Initialize a new uniquelist
// sets last to NULL to signify that the list has not been given any values yet
void uniquelist_init(uniquelist_t* s) {
  s->last = NULL;
  
}

/// Destroy a uniquelist
// frees all 'malloc'ed memory used in the linked list
void uniquelist_destroy(uniquelist_t* s) {
  uniquelist_t * temp = s->next;
  uniquelist_t * current;
  while(temp != NULL){
    current = temp;
    temp = temp->next;
    free(current);
  }
 
  s = NULL; //sets the uniquelist head that does not reside in the heap to NULL
  
  
  
}

/// Add an element to a uniquelist, unless it's already in the uniquelist
//inserts values into the uniquelist that have not been inserted before, also maintains the
//order of insertion
void uniquelist_insert(uniquelist_t* s, int n) {
  uniquelist_t * temp = malloc(sizeof(uniquelist_t));
  uniquelist_t * loopTemp = s; //need to maintatin s so that the its last pointer can be called
  int found = 0; //determines whether or not 'n' has already been inserted into the list
  if(s->last == NULL){
    s->val = n;
    s->last = s;
    s->next = NULL;
    
  }
  while(loopTemp != NULL){//checks for matches to 'n'
    if(loopTemp->val == n){
      found = 1;
      break;
    }
    loopTemp = loopTemp->next;
  }
  
  if(!found){//inserts 'n' otherwise at the end of the list
  temp->val = n;
  temp->next = NULL;
  temp->last = NULL;
  (s->last)->next = temp;
  s->last = temp;
  }
}

/// Print all the numbers in a uniquelist
void uniquelist_print_all(uniquelist_t* s) {
  while(s != NULL){
    printf("%d ", s->val);
    s = s->next;
  }
}
