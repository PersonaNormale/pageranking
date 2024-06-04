#include "graph.h"

double first_term(grafo *g, double d);
double second_term(grafo *g, int node, double d, double *Y);
double third_term(grafo *g, double d, double S);
double calcolo_S(grafo *g, double *X);
void calcolo_Y(grafo *g, double *X, double *Y);
void calcolo_errore(grafo *g, double *X_t, double *X_t_1, double *err);
double calcolo_X_j_t_1(grafo *g, double d, int node, double *X, double *Y,
                       double S);
double *pagerank(grafo *g, double d, double eps, int maxiter, int taux,
                 int *numiter);
