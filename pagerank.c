#include "graph.h"

#define FPTYPE float

/* Perform one iteration of PowerIteration. */
int poweriterate(graph *g, FPTYPE alpha, FPTYPE *x, FPTYPE *y)
{
    // Initialize y to 0
    unsigned int i;
    for (i = 0; i < g->n; i++)
    {
        y[i] = 0.0;
    }

    #pragma omp parallel
    {
        unsigned int threadno = omp_get_thread_num();
        while (1)
        {
            while (1)
            {
                // Get the next node
                node v;
                unsigned int i = nextnode(g, &v, threadno);
                if (i == (unsigned int) -1)
                {
                    break;
                }

                // Compute update for neighbors
                if (v.deg != 0)
                {
                    FPTYPE update = alpha * x[i] / v.deg;
                    
                    unsigned int j;
                    for (j = 0; j < v.deg; j++)
                    {
                        #pragma omp atomic
                        y[v.adj[j]] += update;
                    }
                }
                free(v.adj);
            }

            // Get the next block
            nextblock(g, threadno);

            // Check if iteration is over
            if (g->currblockno[threadno] <= NTHREADS)
            {
                break;
            }
        }
    }

    // Distribute remaining weight among the nodes
    FPTYPE remainder = 1.0;
    for (i = 0; i < g->n; i++)
    {
        remainder -= y[i];
    }
    remainder /= (FPTYPE) g->n;
    for (i = 0; i < g->n; i++)
    {
        y[i] += remainder;
    }

    return 0;
}

/* Perform PowerIteration. */
int power(graph *g, FPTYPE alpha, FPTYPE tol, int maxit, FPTYPE *x, FPTYPE *y)
{
    // Initialize x to e/n
    FPTYPE init = 1.0 / (FPTYPE) g->n;
    unsigned int i;
    for (i = 0; i < g->n; i++)
    {
        x[i] = init;
    }

    // Get next blocks
    for (i = 0; i < NTHREADS; i++)
    {
        nextblock(g, i);
    }

    // For each iteration
    int iter = 0;
    while (iter < maxit)
    {
        // Perform iteration
        poweriterate(g, alpha, x, y);
        iter++;
        
        // Compute residual norm
        FPTYPE norm = 0.0;
        for (i = 0; i < g->n; i++)
        {
            norm += fabs(x[i] - y[i]);
        }
        
        // Print residual norm
        fprintf(stderr, "%d: %e\n", iter, norm);
        
        // Copy y to x
        for (i = 0; i < g->n; i++)
        {
            x[i] = y[i];
        }

        // Stop iterating if residual norm is within tolerance
        if (norm < tol)
        {
            break;
        }
    }

    return 0;
}

/* Computes the PageRank vector of a directed graph in
 * BADJBLK format using PowerIteration. */
int main(int argc, char *argv[])
{
    // Check arguments
    if (argc < 3)
    {
        fprintf(stderr, "Usage: ./pagerank [BADJBLK file] [maxiter]\n");
        return 1;
    }
    
    // Initialize graph
    graph g;
    if (initialize(&g, argv[1], BADJBLK))
    {
        return 1;
    }

    // Print numbers of nodes and edges
    fprintf(stderr, "Nodes: %llu\n", g.n);
    fprintf(stderr, "Edges: %llu\n\n", g.m);

    // Set PageRank parameters
    FPTYPE alpha = 0.85;
    FPTYPE tol = 1e-8;
    int maxit = atoi(argv[2]);

    // Initialize PageRank vectors
    FPTYPE *x = malloc(g.n * sizeof(FPTYPE));
    FPTYPE *y = malloc(g.n * sizeof(FPTYPE));

    // Perform PowerIteration
    power(&g, alpha, tol, maxit, x, y);

    // Test
    fprintf(stderr, "\n");
    unsigned int i;
    for (i = 0; i < 10; i++)
    {
        fprintf(stderr, "%d: %e\n", i, x[i]);
    }
   
    // Destroy vectors
    free(x);
    free(y);

    // Destroy graph
    destroy(&g);

    return 0;
}
