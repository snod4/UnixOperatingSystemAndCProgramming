#include "queue.hh"
#include <pthread.h>
#include <stdlib.h>



// Initialize a new queue
void queue_init(my_queue_t* queue) {
  queue->head = NULL;
  queue->tail = NULL;
  pthread_mutex_init(&queue->lock, NULL);
}

// Destroy a queue
void queue_destroy(my_queue_t* queue) {
  //only one thread should be able to destroy a queue
  pthread_mutex_lock(&queue->lock);
  node_t * cur = queue->head;
  //free each node in the queue
  while(queue->head != NULL) {
    queue->head = queue->head->next;
    free(cur);
    cur = queue->head;
  }
  //ensure tail is NULL as well
  queue->tail = NULL;
  pthread_mutex_unlock(&queue->lock);
}

// Put an element at the end of a queue
void queue_put(my_queue_t* queue, int element) {
  //create and initialize the new node
  node_t * newNode = (node_t *) malloc(sizeof(node_t));
  newNode->data = element;
  newNode->next = NULL;

  //lock the writing to the queue
  pthread_mutex_lock(&queue->lock);

  //deals with the case where the queue is empty
  if(queue_empty(queue)) {
    queue->head = newNode;
    queue->tail = newNode;
  } else {
    queue->tail->next = newNode;
    queue->tail = queue->tail->next;
  }
   pthread_mutex_unlock(&queue->lock);

}

// Check if a queue is empty
bool queue_empty(my_queue_t* queue) {
  if(queue->tail == NULL) {
    return true;
  } else {
    return false;
  }
}

// Take an element off the front of a queue
int queue_take(my_queue_t* queue) {
  pthread_mutex_lock(&queue->lock);
  if(queue_empty(queue)) {
    pthread_mutex_unlock(&queue->lock);
    return -1;
  }
  int retVal = queue->head->data;
  queue->head = queue->head->next;
  if (queue->head == NULL) {
    queue->tail = NULL;
  }
  pthread_mutex_unlock(&queue->lock);
  
  
  return retVal;
}
