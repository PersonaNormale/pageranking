#ifndef THREADPOOL_1_H
#define THREADPOOL_1_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

typedef void (*thread_func_t)(void *arg);

typedef struct thread_pool_work {
  thread_func_t func;
  void *arg;
  struct thread_pool_work *next;
} thread_pool_work_t;

typedef struct thread_pool_queue {
  thread_pool_work_t *head;
  thread_pool_work_t *tail;
} thread_pool_queue_t;

typedef struct thread_pool {
  pthread_mutex_t work_mutex;
  pthread_cond_t work_cond; // Segnala se c'è lavoro da fare ai threads
  pthread_cond_t
      working_cond; // Segnala quando non ci sono threads che lavorano
  thread_pool_queue_t *work_queue;

  int working_counter; // Quanti threads stanno lavorando
  int thread_counter;
  bool stop;
  pthread_t *threads;
} thread_pool_t;

// Prototipi delle funzioni per la gestione del thread pool
void tp_work_destroy(thread_pool_work_t *work);
bool tp_add_work(thread_pool_t *tpool, thread_func_t func, void *arg);
void tp_wait(thread_pool_t *tpool);
void *tp_worker(void *arg);
thread_pool_t *tp_create(int thread_num);
void tp_destroy(thread_pool_t *tpool);

#endif // !THREADPOOL_1_H
