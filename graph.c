#include "graph.h"

int initialize(graph *g, char *filename, char format)
{
    if (strlen(filename) > FILENAMELEN)
    {
        fprintf(stderr, "Max file name length exceeded.\n");
        return 1;
    }

    strcpy(g->filename, filename);
    
    if (format != BADJGZ)
    {
        g->stream = (FILE *) fopen64(g->filename, "r");
    
        if (g->stream == NULL)
        {
            fprintf(stderr, "Could not open file.\n");
            return 1;
        }
    }
    
    g->format = format;

    if (g->format == SMAT)
    {
        char buff[BUFFSIZE];
    
        fscanf(g->stream, "%s", buff);
        fscanf(g->stream, "%s", buff);
        g->n = strtoull(buff, NULL, 0);
    
        fscanf(g->stream, "%s", buff);
        g->m = strtoull(buff, NULL, 0);
    }
    else if (g->format == BSMAT)
    {
        int n, m;

        fread(&n, sizeof(int), 1, g->stream);
        fread(&n, sizeof(int), 1, g->stream);
        fread(&m, sizeof(int), 1, g->stream);

        g->n = (unsigned long long) n;
        g->m = (unsigned long long) m;
    }
    else if (g->format == BADJ)
    {
        unsigned long long n;
        unsigned long long m;

        fread(&g->n, sizeof(unsigned long long), 1, g->stream);
        fread(&g->m, sizeof(unsigned long long), 1, g->stream);
    }
    else
    {
        g->gzstream = (gzFile) gzopen64(g->filename, "r");
    
        if (g->gzstream == NULL)
        {
            fprintf(stderr, "Could not open file.\n");
            return 1;
        }

        unsigned long long n;
        unsigned long long m;

        gzread(g->gzstream, &g->n, sizeof(unsigned long long));
        gzread(g->gzstream, &g->m, sizeof(unsigned long long));
    }

    recentedge.src = -1;

    return 0;
}

int nextedge(graph *g, edge *e)
{
    if (feof(g->stream))
    {
        return 1;
    }
   
    if (g->format == SMAT)
    {
        char buff[BUFFSIZE];
    
        fscanf(g->stream, "%s", buff);
        e->src = strtoull(buff, NULL, 0);
    
        fscanf(g->stream, "%s", buff);
        e->dest = strtoull(buff, NULL, 0);

        fscanf(g->stream, "%s", buff);
    }
    else if (g->format == BSMAT)
    {
        double weight;

        fread(&e->src, sizeof(unsigned long long), 1, g->stream);
        fread(&e->dest, sizeof(unsigned long long), 1, g->stream);
        fread(&weight, sizeof(double), 1, g->stream);
    }
    else
    {
        return 1;
    }

    return 0;
}

int nextnode(graph *g, node *v, unsigned long long i)
{
    if (g->format == SMAT || g->format == BSMAT)
    {
        unsigned long long size = 64;

        v->deg = 0;
        v->adj = malloc(size*sizeof(unsigned long long));

        edge e;

        while (1)
        {
            if (v->deg == 0 && recentedge.src == i)
            {
                e = recentedge;
            }
            else if (recentedge.src != -1 && recentedge.src > i)
            {
                break;
            }
            else if (nextedge(g, &e))
            {
                recentedge.src = -1;
                break;
            }
            else if (e.src != i)
            {
                recentedge = e;
                break;
            }
        
            if (v->deg == size)
            {
                size = 2*size;
                unsigned long long *tmp = malloc(size*sizeof(unsigned long long));
                unsigned long long j;
                for (j = 0; j < size / 2; j++)
                {
                    tmp[j] = v->adj[j];
                }
                free(v->adj);
                v->adj = tmp;
            }

            v->adj[v->deg] = e.dest;
            v->deg++;
        }
    }
    else if (g->format == BADJ)
    {
        if (g->m > 4294967296)
        {
            fread(&v->deg, sizeof(unsigned long long), 1, g->stream);
            
            v->adj = malloc(v->deg*sizeof(unsigned long long));
            fread(v->adj, sizeof(unsigned long long), v->deg, g->stream);
        }
        else
        {
            unsigned int val;
            fread(&val, sizeof(unsigned int), 1, g->stream);
            v->deg = (unsigned long long) val;
           
            char *buff = malloc(v->deg*sizeof(unsigned long long));
            fread(buff, sizeof(unsigned int), v->deg, g->stream);

            long long i;
            for (i = v->deg-1; i >= 0; i--)
            {
                if (i == 0)
                {
                    buff[i*sizeof(unsigned long long)+4] = buff[i*sizeof(unsigned int)];
                    buff[i*sizeof(unsigned long long)+5] = buff[i*sizeof(unsigned int)+1];
                    buff[i*sizeof(unsigned long long)+6] = buff[i*sizeof(unsigned int)+2];
                    buff[i*sizeof(unsigned long long)+7] = buff[i*sizeof(unsigned int)+3];

                    buff[i*sizeof(unsigned long long)] = buff[i*sizeof(unsigned long long)+4];
                    buff[i*sizeof(unsigned long long)+1] = buff[i*sizeof(unsigned long long)+5];
                    buff[i*sizeof(unsigned long long)+2] = buff[i*sizeof(unsigned long long)+6];
                    buff[i*sizeof(unsigned long long)+3] = buff[i*sizeof(unsigned long long)+7];
                }
                else
                {
                    buff[i*sizeof(unsigned long long)] = buff[i*sizeof(unsigned int)];
                    buff[i*sizeof(unsigned long long)+1] = buff[i*sizeof(unsigned int)+1];
                    buff[i*sizeof(unsigned long long)+2] = buff[i*sizeof(unsigned int)+2];
                    buff[i*sizeof(unsigned long long)+3] = buff[i*sizeof(unsigned int)+3];
                }

                buff[i*sizeof(unsigned long long)+4] = 0;
                buff[i*sizeof(unsigned long long)+5] = 0;
                buff[i*sizeof(unsigned long long)+6] = 0;
                buff[i*sizeof(unsigned long long)+7] = 0;
            }

            v->adj = (unsigned long long *) buff;
        }
    }
    else
    {
        if (g->m > 4294967296)
        {
            gzread(g->gzstream, &v->deg, sizeof(unsigned long long));

            v->adj = malloc(v->deg*sizeof(unsigned long long));
            gzread(g->gzstream, &v->adj, v->deg*sizeof(unsigned long long));
        }
        else
        {
            unsigned int val;
            gzread(g->gzstream, &val, sizeof(unsigned int));
            v->deg = (unsigned long long) val;
           
            char *buff = malloc(v->deg*sizeof(unsigned long long));
            gzread(g->gzstream, buff, v->deg*sizeof(unsigned int));

            long long i;
            for (i = v->deg-1; i >= 0; i--)
            {
                if (i == 0)
                {
                    buff[i*sizeof(unsigned long long)+4] = buff[i*sizeof(unsigned int)];
                    buff[i*sizeof(unsigned long long)+5] = buff[i*sizeof(unsigned int)+1];
                    buff[i*sizeof(unsigned long long)+6] = buff[i*sizeof(unsigned int)+2];
                    buff[i*sizeof(unsigned long long)+7] = buff[i*sizeof(unsigned int)+3];

                    buff[i*sizeof(unsigned long long)] = buff[i*sizeof(unsigned long long)+4];
                    buff[i*sizeof(unsigned long long)+1] = buff[i*sizeof(unsigned long long)+5];
                    buff[i*sizeof(unsigned long long)+2] = buff[i*sizeof(unsigned long long)+6];
                    buff[i*sizeof(unsigned long long)+3] = buff[i*sizeof(unsigned long long)+7];
                }
                else
                {
                    buff[i*sizeof(unsigned long long)] = buff[i*sizeof(unsigned int)];
                    buff[i*sizeof(unsigned long long)+1] = buff[i*sizeof(unsigned int)+1];
                    buff[i*sizeof(unsigned long long)+2] = buff[i*sizeof(unsigned int)+2];
                    buff[i*sizeof(unsigned long long)+3] = buff[i*sizeof(unsigned int)+3];
                }

                buff[i*sizeof(unsigned long long)+4] = 0;
                buff[i*sizeof(unsigned long long)+5] = 0;
                buff[i*sizeof(unsigned long long)+6] = 0;
                buff[i*sizeof(unsigned long long)+7] = 0;
            }

            v->adj = (unsigned long long *) buff;
        }
    }

    return 0;
}

int rewindedges(graph *g)
{
    if (g->format == SMAT)
    {
        fseek(g->stream, 0, SEEK_SET);
        
        char buff[BUFFSIZE];

        fscanf(g->stream, "%s", buff);
        fscanf(g->stream, "%s", buff);
        fscanf(g->stream, "%s", buff);
    }
    else if (g->format == BSMAT)
    {
        fseek(g->stream, 3*sizeof(int), SEEK_SET);
    }
    else if (g->format == BADJ)
    {
        fseek(g->stream, 2*sizeof(unsigned long long), SEEK_SET);
    }
    else
    {
        gzseek(g->gzstream, 2*sizeof(unsigned long long), SEEK_SET);
    }

    return 0;
}
