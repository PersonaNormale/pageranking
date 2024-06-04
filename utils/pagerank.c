#include "pagerank.h"
#include "graph.h"
#include "threadpool.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static bool signal_received = false;

double first_term(grafo *g, double d) {
  double numeratore = 1 - d;
  double denominatore = (float)g->N;

  double calcolo = numeratore / denominatore;
  return calcolo;
}

typedef struct first_term_args {
  grafo *g;
  double d;
  double *res;
} first_term_args_t;

void first_term_thread(void *arg) {
  first_term_args_t *args = (first_term_args_t *)arg;
  *(args->res) = first_term(args->g, args->d);
}

double second_term(grafo *g, int node, double d, double *Y) {
  double sum_in_node = 0;

  int *arr = g->in->edges_array[node]->array;
  int size = g->in->edges_array[node]->edges_num;

  for (int i = 0; i < size; i++) {
    sum_in_node += Y[arr[i]]; // Qua posso fare direttamente prima il calcolo
                              // senza calcolare Y
                              // (Valido non usare Y)?
  }

  return sum_in_node * d;
}

typedef struct second_term_args {
  grafo *g;
  int node;
  double d;
  double *Y;
  double *res;
} second_term_args_t;

void second_term_thread(void *arg) {
  second_term_args_t *args = (second_term_args_t *)arg;
  *(args->res) = second_term(args->g, args->node, args->d, args->Y);
}

double third_term(grafo *g, double d, double S) {
  double res = d / (float)g->N;

  res *= S;

  return res;
}

double calcolo_S(grafo *g, double *X) {
  double sum = 0;

  for (int i = 0; i < g->N; i++) {
    if (!g->out[i])
      sum += X[i];
  }

  return sum;
}

typedef struct calc_S_args {
  grafo *g;
  double *X;
  double *res;
} calc_S_args_t;

void calcolo_S_thread(void *arg) {
  calc_S_args_t *args = (calc_S_args_t *)arg;
  *(args->res) = calcolo_S(args->g, args->X);
}

void calcolo_Y(grafo *g, double *X, double *Y) {
  for (int i = 0; i < g->N; i++) {
    if (!g->out[i])
      continue;
    Y[i] = X[i] / (float)g->out[i];
  }
}

typedef struct calc_Y_args {
  grafo *g;
  double *X;
  double *res;
} calc_Y_args_t;

void calcolo_Y_thread(void *arg) {
  calc_Y_args_t *args = (calc_Y_args_t *)arg;
  calcolo_Y(args->g, args->X, args->res);
}

void calcolo_errore(grafo *g, double *X_t, double *X_t_1, double *err) {
  double temp = 0;
  // RAM
  *err = 0;

  for (int i = 0; i < g->N; i++) {
    temp = X_t_1[i] - X_t[i];
    if (temp < 0)
      temp = -temp;
    *err += temp;
  }
}

typedef struct calc_errore_args {
  grafo *g;
  double *X_t;
  double *X_t_1;
  double *res;
} calc_errore_args_t;

void calcolo_errore_thread(void *arg) {
  calc_errore_args_t *args = (calc_errore_args_t *)arg;

  calcolo_errore(args->g, args->X_t, args->X_t_1, args->res);
}

double calcolo_X_j_t_1(grafo *g, double d, int node, double *X, double *Y,
                       double S) {
  double first = first_term(g, d);
  double second = second_term(g, node, d, Y);
  double third = third_term(g, d, S);

  return first + second + third;
}

typedef struct calcolo_args {
  grafo *g;
  double d;
  int node;
  double *Y;
  double first;
  double third;
  double *res;
} calcolo_args_t;

void calcolo_X_j_t_1_thread(void *arg) {
  calcolo_args_t *args = (calcolo_args_t *)arg;

  double second = second_term(args->g, args->node, args->d, args->Y);

  *(args->res) = args->first + second + args->third;
  free(args);
}

void handle_sigusr1() { signal_received = true; }

void *sigusr1_thread(void *arg) {

  int sig;

  // Faccio il setup del signal handler
  struct sigaction sa;
  sa.sa_handler = handle_sigusr1;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGUSR1, &sa, NULL);

  // Sblocco SIGUSR1 in questo thread
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGUSR1);
  pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);

  while (true) {
    sigwait(&sigset, &sig);
    if (sig == SIGUSR1) {
      handle_sigusr1();
    }
  }

  return NULL;
}

int find_max_array(double *X, int len) {
  double max = -1;
  int idx = 0;

  for (int i = 0; i < len; i++) {
    if (X[i] > max) {
      max = X[i];
      idx = i;
    }
  }

  return idx;
}

double *pagerank(grafo *g, double d, double eps, int maxiter, int taux,
                 int *numiter) {

  thread_pool_t *tpool;
  tpool = tp_create(taux);

  double *X_t = (double *)calloc(g->N, sizeof(double)); // X(t)
  double *Y = (double *)calloc(g->N, sizeof(double));
  double *X_t_1 = (double *)calloc(g->N, sizeof(double)); // X(t+1)
  double *S = (double *)calloc(1, sizeof(double));
  double *errore = (double *)calloc(1, sizeof(double));
  double first = first_term(g, d);
  double *temp;

  for (int i = 0; i < g->N; i++)
    X_t[i] = 1.0 / (float)g->N;

  calcolo_Y(g, X_t, Y); // Y(t)

  *S = calcolo_S(g, X_t);
  int iter = 0;

  calc_S_args_t *calc_S = (calc_S_args_t *)calloc(1, sizeof(calc_S_args_t));
  calc_Y_args_t *calc_Y = (calc_Y_args_t *)calloc(1, sizeof(calc_Y_args_t));
  calc_errore_args_t *calc_errore =
      (calc_errore_args_t *)calloc(1, sizeof(calc_errore_args_t));

  // Parte segnali
  pthread_t signal_thread;
  pthread_create(&signal_thread, NULL, sigusr1_thread, NULL);

  do {
    double third = third_term(g, d, *S);

    for (int j = 0; j < g->N; j++) {
      calcolo_args_t *calc =
          (calcolo_args_t *)calloc(1, sizeof(calcolo_args_t));
      calc->g = g;
      calc->d = d;
      calc->node = j;
      calc->Y = Y;
      calc->first = first;
      calc->third = third;
      calc->res = &X_t_1[j];
      tp_add_work(tpool, calcolo_X_j_t_1_thread, calc);
    }

    tp_wait(tpool);
    // printf("Finito di aspettare iter: %d\n", iter);

    temp = X_t;
    X_t = X_t_1;
    X_t_1 = temp;

    calc_S->g = g;
    calc_S->X = X_t;
    calc_S->res = S;
    // S = calcolo_S(g, X_t);

    calc_Y->g = g;
    calc_Y->X = X_t;
    calc_Y->res = Y;
    // Y = calcolo_Y(g, X_t);

    calc_errore->g = g;
    calc_errore->X_t = X_t;
    calc_errore->X_t_1 = X_t_1;
    calc_errore->res = errore;

    tp_add_work(tpool, calcolo_S_thread, calc_S);
    tp_add_work(tpool, calcolo_Y_thread, calc_Y);
    tp_add_work(tpool, calcolo_errore_thread, calc_errore);

    tp_wait(tpool);
    // errore = calcolo_errore(g, X_t, X_t_1);
    iter++;

    if (signal_received) {
      signal_received = false;
      int max = find_max_array(X_t, g->N);
      fprintf(stderr, "%d %d %lf\n", iter, max, X_t[max]);
    }

  } while (*errore > eps && iter < maxiter);

  pthread_cancel(signal_thread); // Mi serve, pure usando sig_atomic la sigwait
                                 // sta in attesa e devo fare l'handling così
  pthread_join(signal_thread, NULL);

  free(calc_S);
  free(calc_Y);
  free(calc_errore);
  free(X_t_1);
  free(Y);
  free(S);
  free(errore);
  tp_destroy(tpool);

  *numiter = iter;

  return X_t;
}
