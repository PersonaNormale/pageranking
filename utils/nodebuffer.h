#ifndef BUFFER_H
#define BUFFER_H

#include <pthread.h>
#include <semaphore.h>

typedef struct {
  int IN;
  int OUT;
} tupla;

typedef struct buffer {
  int size;
  tupla *buffer;
  int in;                // Indice per il prossimo elemento da produrre
  int out;               // Indice per il prossimo elemento da consumare
  sem_t empty;           // Conta gli spazi vuoti nel buffer
  sem_t full;            // Conta gli elementi presenti nel buffer
  pthread_mutex_t mutex; // Protegge l'accesso al buffer
  int finished;          // Indica se il produttore ha finito
} buffer_t;

// Inizializza il buffer
void buffer_init(buffer_t *buffer, int size);

// Distrugge il buffer
void buffer_destroy(buffer_t *buffer);

// Aggiunge un elemento al buffer
void buffer_produce(buffer_t *buffer, tupla item);

// Rimuove un elemento dal buffer
tupla buffer_consume(buffer_t *buffer);

#endif // BUFFER_H
