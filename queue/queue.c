#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define QUEUE_END -1



typedef struct node{
  int value;
  struct node * next;
} node_t;

// This struct is where you should define your queue datatype. You may need to
// add another struct (e.g. a node struct) depending on how you choose to
// implement your queue.

typedef struct queue {
  node_t * head;
  node_t * tail;
} queue_t;

/**
 * Initialize a queue pointed to by the parameter q.
 * \param q  This points to allocated memory that should be initialized to an
 *           empty queue.
 */
void queue_init(queue_t* q) {
  q->head = NULL;
  q->tail = NULL;
}

/**
 * Add a value to a queue.
 * \param q       Points to a queue that has been initialized by queue_init.
 * \param value   The integer value to add to the queue
 */
void queue_put(queue_t* q, int value) {
  node_t * temp = malloc(sizeof(node_t));
  temp->value = value;
  temp->next = NULL;
  if(q->head == NULL){
    q->head = temp;
    q->tail = temp;
  }
  else{
  (q->tail)->next = temp;
  q->tail = temp;
  }

}

/**
 * Check if a queue is empty.
 * \param q   Points to a queue initialized by queue_init.
 * \returns   True if the queue is empty, otherwise false.
 */
bool queue_empty(queue_t* q) {
  
  return q->head == NULL;
}

/**
 * Take a value from a queue.
 * \param q   Points to a queue initialized by queue_init.
 * \returns   The value that has been in the queue the longest time. If the
 *            queue is empty, return QUEUE_END.
 */
int queue_take(queue_t* q) {
  if(queue_empty(q)){
  return QUEUE_END;
    }
  else{
    int val = (q->head)->value;
    node_t * tempPtr = q->head;
    q->head = (q->head)->next;
    free(tempPtr); 
    return val;
    }
}

/**
 * Free any memory allocated inside the queue data structure.
 * \param q   Points to a queue initialized by queue_init. The memory referenced
 *            by q should *NOT* be freed.
 */
void queue_destroy(queue_t* q) {
  node_t * tempPtr = q->head;
  node_t * freeTempPtr = tempPtr; //frees node after its next value has been obtained
  while(tempPtr != NULL){
    tempPtr = tempPtr->next;
    free(freeTempPtr);
    freeTempPtr = tempPtr;
  }
  q->head = NULL;
  q->tail = NULL;
}

int main(int argc, char** argv) {
  // Set up and initialize a queue
  queue_t q;
  queue_init(&q);
  
  // Read lines until the end of stdin
  char* line = NULL;
  size_t line_size = 0;
  while(getline(&line, &line_size, stdin) != EOF) {
    int num;
    
    // If the line has a take command, take a value from the queue
    if(strcmp(line, "take\n") == 0) {
      if(queue_empty(&q)) {
        printf("The queue is empty.\n");
      } else {
        printf("%d\n", queue_take(&q));
      }
    } else if(sscanf(line, "put %d\n", &num) == 1) {
      queue_put(&q, num);
    } else {
      printf("unrecognized command.\n");
    }
  }
  
  // Free the space allocated by getline
  free(line);
  
  // Clean up the queue
  queue_destroy(&q);
  
  return 0;
}
