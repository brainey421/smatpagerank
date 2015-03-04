#include "graph.h"

int iterate(graph *g, double alpha, double *x, double *y)
{
    node v;
    unsigned long long i;
    double prob;
    unsigned long long j;

    for (i = 0; i < g->n; i++)
    {
        y[i] = 0.0;
    }

    for (i = 0; i < g->n; i++)
    {
        nextnode(g, &v, i);
        
        if (v.deg != 0)
        {
            prob = 1.0 / (double) v.deg;

            for (j = 0; j < v.deg; j++)
            {
                y[v.adj[j]] += x[i]*prob;
            }
        }

        free(v.adj);
    }

    for (i = 0; i < g->n; i++)
    {
        y[i] *= alpha;
    }

    double sum = 0.0;
    for (i = 0; i < g->n; i++)
    {
        sum += y[i];
    }
    double remainder = (1.0 - sum) / (double) g->n;
    for (i = 0; i < g->n; i++)
    {
        y[i] += remainder;
    }

    rewindedges(g);

    return 0;
}

int power(graph *g, double alpha, double tol, int maxit, double *x, double *y)
{
    unsigned long long i;
    double init = 1.0 / (double) g->n;
    for (i = 0; i < g->n; i++)
    {
        x[i] = init;
    }

    int iter = 0;
    double norm;
    double diff;
    double *tmp;
    while (iter < maxit)
    {
        iterate(g, alpha, x, y);
        iter++;

        norm = 0.0;
        for (i = 0; i < g->n; i++)
        {
            diff = x[i] - y[i];
            if (diff > 0.0)
            {
                norm += diff;
            }
            else
            {
                norm -= diff;
            }
        }
        
        tmp = x;
        x = y;
        y = tmp;

        fprintf(stderr, "%d: %e\n", iter, norm);

        if (norm < tol)
        {
            break;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: ./pagerank [graphfile] [badj|bsmat|smat]\n");
        return 1;
    }
    
    char format;
    if (!strcmp(argv[2], "bsmat"))
    {
        format = BSMAT;
    }
    else if (!strcmp(argv[2], "smat"))
    {
        format = SMAT;
    }
    else if (!strcmp(argv[2], "badj"))
    {
        format = BADJ;
    }
    else
    {
        fprintf(stderr, "Unknown format.\n");
        return 1;
    }

    graph g;
    if (initialize(&g, argv[1], format))
    {
        return 1;
    }

    printf("Nodes: %llu\n", g.n);
    printf("Edges: %llu\n\n", g.m);

    double alpha = 0.85;
    double tol = 1e-8;
    int maxit = 1000;

    double *x = malloc(g.n * sizeof(double));
    double *y = malloc(g.n * sizeof(double));
    
    power(&g, alpha, tol, maxit, x, y);
    
    free(x);
    free(y);

    return 0;
}
