#include <pthread.h>
#include "stack.hh"

#include <stdlib.h>

//node for each element in stack
typedef struct stack_node {
  int value;
  struct stack_node * next;
} stack_node_t;


// Initialize a stack
void stack_init(my_stack_t* stack) {
  pthread_mutex_init(&stack->lock, NULL);
  stack->head = NULL;
}

// Destroy a stack
void stack_destroy(my_stack_t* stack) {
  //this should only be alllowed by one thread at a time 
  pthread_mutex_lock(&stack->lock);
  stack_node_t* temp = stack->head;
  while(stack->head != NULL) {
    stack->head = stack->head->next;
    free(temp);
    temp = stack->head;
  }
  pthread_mutex_unlock(&stack->lock);
}

// Push an element onto a stack
void stack_push(my_stack_t* stack, int element) {
  //create node for element and initialize the node
  stack_node_t * temp = (stack_node_t *) malloc(sizeof(stack_node_t));
  temp->value = element;
  //add node to stack
  pthread_mutex_lock(&stack->lock);
  temp->next = stack->head;
  stack->head = temp;
  pthread_mutex_unlock(&stack->lock);
}

// Check if a stack is empty
bool stack_empty(my_stack_t* stack) {
  if(stack->head == NULL) {
    return true;
  }
  return false;
}

// Pop an element off of a stack
int stack_pop(my_stack_t* stack) {
  //checks for empty stack
  if(stack_empty(stack)){
    return -1;
  }
  //removes node off the top of the stack and returns the element in that node
  pthread_mutex_lock(&stack->lock);
  stack_node_t * temp = stack->head;
  stack->head = stack->head->next;
  int value = temp->value;
  pthread_mutex_unlock(&stack->lock);

  //frees the node removed
  free(temp);
  return value;
}
