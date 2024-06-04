#include <assert.h>
#include <errno.h>
#define _GNU_SOURCE
#include "utils/graph.h"
#include "utils/nodebuffer.h"
#include "utils/pagerank.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct value_node {
  double value;
  int index;
} value_node_t;

int cmp(const void *a, const void *b) {
  value_node_t *a1 = (value_node_t *)a;
  value_node_t *a2 = (value_node_t *)b;
  if (a1->value > a2->value)
    return -1;
  else if (a1->value < a2->value)
    return 1;
  else
    return 0;
}

void read_from_grafo(char *filename, buffer_t *buf, int thread_num, inmap *map,
                     pthread_t *aux_threads) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    perror("Errore lettura file.");
    exit(EXIT_FAILURE);
  }

  // Buffer per leggere
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  int start_reading_nodes = 0;

  while ((read = getline(&line, &len, file)) != -1) {
    if (!start_reading_nodes) {
      if (line[0] == '%') {
        continue;
      } else {
        start_reading_nodes = 1;
        continue;
      }
    }

    if (line[0] == '%') {
      continue;
    }

    char *ptr;
    int in = strtol(line, &ptr, 10) - 1;
    int out = strtol(ptr, NULL, 10) - 1;

    tupla t = {.IN = in, .OUT = out};
    buffer_produce(buf, t);
  }

  for (int i = 0; i < thread_num; i++) {
    tupla t = {.IN = -1, .OUT = -1};
    buffer_produce(buf, t);
  }

  free(line);
  fclose(file);
}

int read_size_from_file(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    perror("Error opening file");
    exit(EXIT_FAILURE);
  }

  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  while ((read = getline(&line, &len, file)) != -1) {
    if (line[0] == '%') {
      continue;
    } else {
      char *token = strtok(line, " ");
      if (token != NULL) {
        int size = atoi(token);
        free(line);
        fclose(file);
        return size;
      } else {
        perror("Errore: Impossibile leggere la dimensione della mappa");
        free(line);
        fclose(file);
        exit(EXIT_FAILURE);
      }
    }
  }

  free(line);
  fclose(file);
  perror("Errore: Dimensione della mappa non trovata");
  exit(EXIT_FAILURE);
}

typedef struct {
  buffer_t *buffer;
  inmap *map;
  outgoing_edges_t *out;
} consumer_args_t;

void *consumer(void *arg) {
  // printf("Consumer\n");
  consumer_args_t *args = (consumer_args_t *)arg;
  buffer_t *buf = args->buffer;
  inmap *map = args->map;
  outgoing_edges_t *out = args->out;

  while (1) {
    tupla t = buffer_consume(buf);
    // printf("Cons: %d %d \n", t.IN, t.OUT);
    if (t.IN == -1 && t.OUT == -1) {
      return (void *)0;
    }

    // printf("Consumer Inmap-> %d\n", map);
    // printf("Inserted %d %d \n", t.IN, t.OUT);
    insert_inmap(map, t.IN, t.OUT, out);
  }
}

static int compare(const void *a, const void *b) {
  if (*(double *)a < *(double *)b)
    return 1;
  else if (*(double *)a > *(double *)b)
    return -1;
  else
    return 0;
}

// Funzione per ordinare un array di double
void sort_double_array(double *array, int size) {
  qsort(array, size, sizeof(double), compare);
}

int main(int argc, char *argv[]) {

  // Parte segnali
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGUSR1);
  pthread_sigmask(SIG_BLOCK, &sigset, NULL);

  int K = 3;           // default per top K nodes
  int M = 100;         // default per max iterations
  double D = 0.9;      // default per damping factor
  double E = 1.0e-7;   // default per max error
  int T = 3;           // default per threads
  char *infile = NULL; // input file

  int opt;
  while ((opt = getopt(argc, argv, "k:m:d:e:t:")) != -1) {
    switch (opt) {
    case 'k':
      K = atoi(optarg);
      break;
    case 'm':
      M = atoi(optarg);
      break;
    case 'd':
      D = atof(optarg);
      break;
    case 'e':
      E = atof(optarg);
      break;
    case 't':
      T = atoi(optarg);
      break;
    default:
      fprintf(stderr, "Usage: %s [-k K] [-m M] [-d D] [-e E] [-t T] infile\n",
              argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  if (K <= 0) {
    errno = 1;
    perror("Invalid K value.");
    exit(1);
  }
  if (M <= 0) {
    errno = 1;
    perror("Invalid M value.");
    exit(1);
  }
  if (D <= 0 || D >= 1) {
    errno = 1;
    perror("Invalid D value.");
    exit(1);
  }
  if (T <= 0) {
    errno = 1;
    perror("Invalid T value.");
    exit(1);
  }

  if (optind < argc) {
    infile = argv[optind];
  } else {
    fprintf(stderr, "Expected infile argument after options\n");
    fprintf(stderr, "Usage: %s [-k K] [-m M] [-d D] [-e E] [-t T] infile\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  int size = read_size_from_file(infile);
  buffer_t *cb = (buffer_t *)calloc(1, sizeof(buffer_t));
  inmap *map = create_inmap(size);
  outgoing_edges_t *out = create_outgoing_edges(size);
  consumer_args_t *ca = (consumer_args_t *)calloc(1, sizeof(consumer_args_t));
  ca->buffer = cb;
  ca->map = map;
  ca->out = out;
  pthread_t *threads = (pthread_t *)calloc(T, sizeof(pthread_t));

  buffer_init(cb, 2048);

  for (int i = 0; i < T; i++) {
    pthread_create(&threads[i], NULL, consumer, (void *)ca);
  }

  read_from_grafo(infile, cb, T, map, threads);

  for (int i = 0; i < T; i++) {
    pthread_join(threads[i], NULL);
  }

  free(threads);
  buffer_destroy(cb);
  free(cb);
  free(ca);

  grafo *g = (grafo *)calloc(1, sizeof(grafo));
  g->N = size;
  g->in = map;
  g->out = out->outgoing_edges;

  int *num = (int *)calloc(1, sizeof(int));

  double *p = pagerank(g, D, E, M, T, num);

  value_node_t *vn = (value_node_t *)calloc(g->N, sizeof(value_node_t));

  for (int i = 0; i < g->N; i++) {
    vn[i].value = p[i];
    vn[i].index = i;
  }

  // sort_double_array(p, size);
  qsort(vn, g->N, sizeof(value_node_t), cmp);

  int dead_end = 0;
  double ranks_sum = 0;
  int valid_edges = 0;
  for (int i = 0; i < g->N; i++) {
    if (g->out[i] == 0)
      dead_end++;
    ranks_sum += p[i];
    valid_edges += g->in->edges_array[i]->edges_num;
  }

  fprintf(stdout, "Number of nodes: %d\n", g->N);
  fprintf(stdout, "Number of dead-end nodes: %d\n", dead_end);
  fprintf(stdout, "Number of valid arcs: %d\n", valid_edges);
  if (*num < M) {
    fprintf(stdout, "Converged after %d iterations\n", *num);
  } else {
    fprintf(stdout, "Did not converge after %d iterations\n", M);
  }
  fprintf(stdout, "Sum of ranks: %0.4f   (should be 1)\n", ranks_sum);
  if (K > g->N)
    exit(0);
  fprintf(stdout, "Top %d nodes:\n", K);
  for (int i = 0; i < K; i++) {
    fprintf(stdout, "  %d %lf\n", vn[i].index, vn[i].value);
  }

  free_inmap(map);
  free_outgoing_edges(out);
  free(g);
  free(num);
  free(p);
  free(vn);

  sleep(1); // Toglie le segnalazione dei thread su valgrind
            // Gli da il tempo al O.S. di killarli

  return 0;
}
