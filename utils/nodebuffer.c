#include "nodebuffer.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

void buffer_init(buffer_t *buffer, int size) {
  buffer->size = size;
  buffer->in = 0;
  buffer->out = 0;
  buffer->finished = 0;
  buffer->buffer = (tupla *)calloc(size, sizeof(tupla));
  sem_init(&buffer->empty, 0, size);
  sem_init(&buffer->full, 0, 0);
  pthread_mutex_init(&buffer->mutex, NULL);
}

void buffer_destroy(buffer_t *buffer) {
  free(buffer->buffer);
  sem_destroy(&buffer->empty);
  sem_destroy(&buffer->full);
  pthread_mutex_destroy(&buffer->mutex);
}

void buffer_produce(buffer_t *buffer, tupla item) {
  // printf("Producer \n");
  sem_wait(&buffer->empty); // Attende che ci sia spazio vuoto nel buffer
  pthread_mutex_lock(&buffer->mutex); // Acquisisce il lock sul buffer

  buffer->buffer[buffer->in] = item;
  buffer->in = (buffer->in + 1) % buffer->size;

  pthread_mutex_unlock(&buffer->mutex); // Rilascia il lock sul buffer_producer
  sem_post(&buffer->full); // Incrementa il conteggio degli elementi presenti
}

tupla buffer_consume(buffer_t *buffer) {
  sem_wait(&buffer->full); // Attende che ci sia almeno un elemento nel buffer
  pthread_mutex_lock(&buffer->mutex); // Acquisisce il lock sul buffer

  //  Verifica se il buffer è vuoto e la produzione è terminata

  tupla item = buffer->buffer[buffer->out];
  buffer->out = (buffer->out + 1) % buffer->size;

  pthread_mutex_unlock(&buffer->mutex); // Rilascia il lock sul buffer
  sem_post(&buffer->empty); // Incrementa il conteggio degli spazi vuoti

  if (item.IN == -1) {
    // printf("Yeah\n");
    pthread_exit(0);
  }

  return item;
}
