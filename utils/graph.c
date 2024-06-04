#include "graph.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

edges_array_t *create_edges_array() {
  // Allocate memory for the edges_array structure
  edges_array_t *edges = (edges_array_t *)calloc(1, sizeof(edges_array_t));
  if (edges == NULL) {
    fprintf(stderr, "Failed to allocate memory for edges_array structure.\n");
    return NULL;
  }

  // Allocate memory for the array
  edges->array = (int *)calloc(4, sizeof(int));
  if (edges->array == NULL) {
    fprintf(stderr, "Failed to allocate memory for array.\n");
    free(edges);
    return NULL;
  }

  // Initialize the mutex
  if (pthread_mutex_init(&edges->mutex, NULL) != 0) {
    fprintf(stderr, "Failed to initialize mutex.\n");
    free(edges->array);
    free(edges);
    return NULL;
  }

  edges->size = 4;
  edges->edges_num = 0;

  return edges;
}

void destroy_edges_array(edges_array_t *edges) {
  if (edges == NULL) {
    return;
  }
  pthread_mutex_destroy(&edges->mutex);
  free(edges->array);
  free(edges);
}

void insert_edges_array(edges_array_t *edges, int data, outgoing_edges_t *out) {
  pthread_mutex_lock(&(edges->mutex));

  if (edges->edges_num == edges->size) {
    edges->size *= 2;
    edges->array = (int *)realloc(edges->array, edges->size * sizeof(int));
    if (edges->array == NULL) {
      fprintf(stderr, "Failed to reallocate memory for array.\n");
      pthread_mutex_unlock(&(edges->mutex));
      return;
    }
  }

  for (int i = 0; i < edges->edges_num; i++) {
    if (edges->array[i] == data) {
      pthread_mutex_unlock(&(edges->mutex));
      return;
    }
  }

  edges->array[edges->edges_num] = data;

  edges->edges_num++;

  pthread_mutex_unlock(&(edges->mutex));

  insert_outgoing_edges(out, data);
}

outgoing_edges_t *create_outgoing_edges(int size) {
  outgoing_edges_t *out =
      (outgoing_edges_t *)calloc(1, sizeof(outgoing_edges_t));

  if (out == NULL) {
    perror("Errore di allocazione array g->out.");
    exit(EXIT_FAILURE);
  }

  if (pthread_mutex_init(&(out->mutex), NULL) != 0) {
    perror("Errore inizializzazione mutex.");
    free(out);
    exit(EXIT_FAILURE);
  }

  out->outgoing_edges = (int *)calloc(size, sizeof(int));

  if (out->outgoing_edges == NULL) {
    perror("Errore di allocazione array g->out.");
    exit(EXIT_FAILURE);
  }

  return out;
}

void insert_outgoing_edges(outgoing_edges_t *out, int node) {
  pthread_mutex_lock(&(out->mutex));

  out->outgoing_edges[node]++;

  pthread_mutex_unlock(&(out->mutex));
}

// Funzione per inizializzare l'inmap
inmap *create_inmap(int size) {
  inmap *map = (inmap *)calloc(1, sizeof(inmap));
  if (map == NULL) {
    perror("Errore allocazione memoria mappa.");
    exit(EXIT_FAILURE);
  }
  map->size = size;

  map->edges_array = (edges_array_t **)calloc(size, sizeof(edges_array_t *));
  if (map->edges_array == NULL) {
    perror("Errore allocazione memoria edges array.");
    free(map);
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < size; i++) {
    map->edges_array[i] = create_edges_array();
    if (map->edges_array[i] == NULL) {
      for (int j = 0; j < i; j++) {
        destroy_edges_array(map->edges_array[j]);
      }
      free(map->edges_array);
      free(map);
      exit(EXIT_FAILURE);
    }
  }
  return map;
}

void insert_inmap(inmap *map, int entering, int exiting,
                  outgoing_edges_t *out) {
  if (exiting >= 0 && exiting < map->size && entering != exiting &&
      entering >= 0 && entering < map->size) {
    // printf("Inserted node %d in array_node %d\n", entering, exiting);
    insert_edges_array(map->edges_array[(exiting)], entering, out);
  }
}

// Funzione per deallocare la memoria occupata dall'inmap
void free_inmap(inmap *map) {
  for (int i = 0; i < map->size; i++) {
    destroy_edges_array(map->edges_array[i]);
  }
  free(map->edges_array);
  free(map);
}

void free_outgoing_edges(outgoing_edges_t *out) {
  pthread_mutex_lock(&(out->mutex));

  free(out->outgoing_edges);
  pthread_mutex_unlock(&(out->mutex));
  pthread_mutex_destroy(&(out->mutex));
  free(out);
}
