#ifndef _GRAPH_
#define _GRAPH_
#include <pthread.h>

typedef struct edges_array {
  int *array;
  pthread_mutex_t mutex;
  int size;
  int edges_num;
} edges_array_t;

typedef struct {
  int *outgoing_edges;
  pthread_mutex_t mutex;
} outgoing_edges_t;

// Definizione della struttura inmap
typedef struct {
  edges_array_t **edges_array;
  int size;
} inmap;

typedef struct {
  int N;
  int *out;
  inmap *in;
} grafo;

// Funzione per inizializzare una linked list
outgoing_edges_t *create_outgoing_edges(int size);

// Funzione per inserire un elemento in ordine crescente nella linked list
// (senza duplicati)
void insert_outgoing_edges(outgoing_edges_t *out, int node);

// Funzione per deallocare la memoria occupata dalla linked list
void free_outgoing_edges(outgoing_edges_t *out);

// Funzione per inizializzare l'inmap
inmap *create_inmap(int size);
void print_inmap(inmap *map);

void insert_inmap(inmap *map, int entering, int exiting, outgoing_edges_t *out);

// Funzione per deallocare la memoria occupata dall'inmap
void free_inmap(inmap *map);

void free_outgoing_edges(outgoing_edges_t *out);

#endif
