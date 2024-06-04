#include "threadpool.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// INIZIO STRUMENTI CODA

thread_pool_work_t *create_work(thread_func_t func, void *arg) {
  thread_pool_work_t *work =
      (thread_pool_work_t *)calloc(1, sizeof(thread_pool_work_t));

  work->func = func;
  work->arg = arg;

  return work;
}

void queue_init(thread_pool_queue_t *queue) {
  queue->head = NULL;
  queue->tail = NULL;
}

bool is_empty(thread_pool_queue_t *queue) {
  if (queue->head == NULL) {
    return true;
  }

  return false;
}

bool enqueue(thread_pool_queue_t *queue, thread_func_t func, void *arg) {
  thread_pool_work_t *work = create_work(func, arg);

  if (work == NULL) {
    return false;
  }

  if (is_empty(queue)) {
    queue->head = work;
    queue->tail = work;
    return true;
  }

  queue->tail->next = work;
  queue->tail = work;
  return true;
}

thread_pool_work_t *dequeue(thread_pool_queue_t *queue) {
  if (is_empty(queue)) {
    return NULL;
  }

  thread_pool_work_t *work = queue->head;

  queue->head = queue->head->next;

  return work;
}

// FINE STRUMENTI CODA

// INIZIO THREAD POOL

void tp_work_destroy(thread_pool_work_t *work) {
  if (work != NULL) {
    free(work);
  }
}

bool tp_add_work(thread_pool_t *tpool, thread_func_t func, void *arg) {
  if (tpool == NULL) {
    return false;
  }

  pthread_mutex_lock(&(tpool->work_mutex));

  bool res = enqueue(tpool->work_queue, func, arg);

  if (res) {
    pthread_cond_broadcast(&(tpool->work_cond));
  }

  pthread_mutex_unlock(&(tpool->work_mutex));

  return res;
}

void tp_wait(thread_pool_t *tpool) {
  if (tpool == NULL) {
    return;
  }

  pthread_mutex_lock(&(tpool->work_mutex));

  while (true) {
    bool first_cond = !is_empty(tpool->work_queue);
    bool second_cond = !tpool->stop && tpool->working_counter != 0;
    bool third_cond = tpool->stop && tpool->thread_counter != 0;
    bool cond = first_cond || second_cond || third_cond;

    if (cond) {
      pthread_cond_wait(&(tpool->working_cond), &(tpool->work_mutex));
    } else {
      break;
    }
  }

  pthread_mutex_unlock(&(tpool->work_mutex));
}

void *tp_worker(void *arg) {
  thread_pool_t *tpool = arg;
  thread_pool_work_t *work;

  while (1) {
    pthread_mutex_lock(&(tpool->work_mutex));

    while (is_empty(tpool->work_queue) && !tpool->stop) {
      pthread_cond_wait(&(tpool->work_cond), &(tpool->work_mutex));
    }

    if (tpool->stop) {
      break;
    }

    work = dequeue(tpool->work_queue);
    tpool->working_counter++;

    pthread_mutex_unlock(&(tpool->work_mutex));

    if (work != NULL) {
      // printf("Partito Lavoro Nodo\n");
      work->func(work->arg);
      tp_work_destroy(work);
    }

    pthread_mutex_lock(&(tpool->work_mutex));
    tpool->working_counter--;

    bool cond = !tpool->stop && tpool->working_counter == 0 &&
                is_empty(tpool->work_queue);

    if (cond) {
      pthread_cond_signal(&(tpool->working_cond));
    }

    pthread_mutex_unlock(&(tpool->work_mutex));
  }

  tpool->thread_counter--;
  pthread_cond_signal(&(tpool->working_cond));
  pthread_mutex_unlock(&(tpool->work_mutex));
  return (void *)0;
}

thread_pool_t *tp_create(int thread_num) {
  thread_pool_t *tpool;
  pthread_t *threads;

  if (thread_num == 0) {
    perror("Valore 0 non valido per num thread.");
    exit(1);
  }

  threads = (pthread_t *)calloc(thread_num, sizeof(pthread_t));
  tpool = (thread_pool_t *)calloc(1, sizeof(thread_pool_t));
  tpool->work_queue =
      (thread_pool_queue_t *)calloc(1, sizeof(thread_pool_queue_t));

  queue_init(tpool->work_queue);
  tpool->thread_counter = thread_num;
  pthread_mutex_init(&(tpool->work_mutex), NULL);
  pthread_cond_init(&(tpool->work_cond), NULL);
  pthread_cond_init(&(tpool->working_cond), NULL);

  for (int i = 0; i < thread_num; i++) {
    pthread_create(&threads[i], NULL, tp_worker, tpool);
    pthread_detach(threads[i]);
  }

  free(threads);

  return tpool;
}

void tp_destroy(thread_pool_t *tpool) {
  if (tpool == NULL) {
    return;
  }

  pthread_mutex_lock(&(tpool->work_mutex));

  while (!is_empty(tpool->work_queue)) {
    tp_work_destroy(dequeue(tpool->work_queue));
  }

  tpool->stop = true;

  pthread_cond_broadcast(&(tpool->work_cond));
  pthread_mutex_unlock(&(tpool->work_mutex));

  tp_wait(tpool);

  pthread_mutex_destroy(&(tpool->work_mutex));
  pthread_cond_destroy(&(tpool->work_cond));
  pthread_cond_destroy(&(tpool->working_cond));

  free(tpool->work_queue);

  free(tpool);
}
