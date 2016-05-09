// Copyright 2013 15418 Course Staff.

#ifndef __WORKER_WORK_STACK_H__
#define __WORKER_WORK_STACK_H__


#include <vector>


template <class T>
class MStack {
private:
  std::vector<T> storage;
  pthread_mutex_t stack_lock;
  pthread_cond_t stack_cond;

public:

  MStack() {
    pthread_cond_init(&stack_cond, NULL);
    pthread_mutex_init(&stack_lock, NULL);
  }

  T pop() {
    pthread_mutex_lock(&stack_lock);
    while (storage.size() == 0) {
      pthread_cond_wait(&stack_cond, &stack_lock);
    }

    T item = storage.back();
    //storage.pop_front();
    storage.erase(storage.end());

    pthread_mutex_unlock(&stack_lock);
    return item;
  }

  void push(const T& item) {
    pthread_mutex_lock(&stack_lock);
    storage.push_back(item);
    pthread_mutex_unlock(&stack_lock);
    pthread_cond_signal(&stack_cond);
  }
};

#endif  // WORKER_WORK_STACK_H_
