//Problem 1



int list_peek(list_t* lst) {
  pthread_mutex_lock(&lst->mutex);
  node_t* first = lst->head;
  pthread_mutex_unlock(&lst->mutex);
  
  if(first == NULL) {
    return -1;
  } else {
    return first->data;
  }
}

