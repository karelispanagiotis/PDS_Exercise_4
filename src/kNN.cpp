#include "kNN.h"
#include <queue>
#include <math.h>
#include <cstdio>
#include <cstdlib>

#include <cilk/cilk.h>
#include <cilk/cilk_api_linux.h>

using namespace std;

// Helper struct used in kNN search algorithm
typedef struct neighbour
{
    float dist; //!< Distance to some query point 
    int index;  //!< Index of neighbour in the original set

}neighbour;
bool operator<(const neighbour &n1, const neighbour &n2){ return n1.dist < n2.dist; }

float dist(float *X, float *Y, int dim)
{
    float dist = 0.0;
    for(int i=0; i<dim; i++)
        dist += (X[i] - Y[i])*(X[i] - Y[i]);
    return sqrtf(dist);
}

// The top of queue always keeps the farthest neighbour
void vpt_search(priority_queue<neighbour> &q, vptree *node, float *query, int d, int k, int &nodesVisited)
{
    if(node == NULL) return;

    nodesVisited++;
    neighbour x = {dist(node->vp, query, d), node->idx};

    if(q.size() < k)
        q.push(x);
    else if(x.dist < q.top().dist)
    {
        q.pop();
        q.push(x);
    }
    
    // The order of branching in the case of intersecting "mu" and "tau" is crucial for performance
    if(x.dist < node->md)
    {
        // Inner first 
        vpt_search(q, node->inner, query, d, k, nodesVisited);
        
        // Outer second (if there is an intersection)
        if(x.dist >= node->md - q.top().dist) 
            vpt_search(q, node->outer, query, d, k, nodesVisited);
    }
    else
    {
        // Outer first
        vpt_search(q, node->outer, query, d, k, nodesVisited); 

        // Inner second (if there is an intersection)
        if(x.dist <= q.top().dist + node->md)
            vpt_search(q, node->inner, query, d, k, nodesVisited);
    }
}

knnresult vptree_kNN(vptree *root, float *query, int n, int d, int k)
{
    knnresult result;
    result.k = k; result.m = n;
    result.ndist = (float *)malloc(n*k*sizeof(float));
    result.nidx  = (int *)  malloc(n*k*sizeof(int));

    int *visits = (int *)malloc(n*sizeof(int));
    cilk_for(int i=0; i<n; i++)
    {
        int nodesVisited = 0;  
        priority_queue<neighbour> q;

        vpt_search(q, root, query + i*d, d, k, nodesVisited);
        visits[i] = nodesVisited;

        for(int j=k-1; j>=0; j--)
        {
            neighbour x = q.top(); q.pop();
            result.nidx[i*k + j] = x.index;
            result.ndist[i*k + j] = x.dist;
        }
    }

    int totalNodesVisited = 0;
    for(int i=0; i<n; i++) totalNodesVisited += visits[i]; 
    printf("Average nodes: %lf\n", (double)totalNodesVisited/n);

    free(visits);
    return result;
}

////////////////////////////////////////////////////////////////////////////

void kdt_search(priority_queue<neighbour> &q, kdtree *node, float *query, int d, int k, int &nodesVisited)
{
    if(node==NULL) return;

    nodesVisited++;
    neighbour x = {dist(node->p, query, d), node->idx};

    if(q.size() < k)
        q.push(x);
    else if(x.dist < q.top().dist)
    {
        q.pop();
        q.push(x);
    }

    int axis = getAxis(node);
    if(query[axis] < node->mc)
    {
        kdt_search(q, node->left, query, d, k, nodesVisited);
        if(node->mc <= q.top().dist + query[axis])
            kdt_search(q, node->right, query, d, k, nodesVisited);
    }
    else
    {
        kdt_search(q, node->right, query, d, k, nodesVisited);
        if(node->mc >= query[axis] - q.top().dist)
            kdt_search(q, node->left, query, d, k, nodesVisited);
    }
}

knnresult kdtree_kNN(kdtree *root, float *query, int n, int d, int k)
{
    knnresult result;
    result.k = k; result.m = n;
    result.ndist = (float *)malloc(n*k*sizeof(float));
    result.nidx  = (int *)  malloc(n*k*sizeof(int));

    int *visits = (int *)malloc(n*sizeof(int));
    cilk_for(int i=0; i<n; i++)
    {
        int nodesVisited = 0;
        priority_queue<neighbour> q;

        kdt_search(q, root, query + i*d, d, k, nodesVisited);
        visits[i] = nodesVisited;

        for(int j=k-1; j>=0; j--)
        {
            neighbour x = q.top(); q.pop();
            result.nidx[i*k + j] = x.index;
            result.ndist[i*k + j] = x.dist;
        }
    }
    
    int totalNodesVisited = 0;
    for(int i=0; i<n; i++) totalNodesVisited += visits[i]; 
    printf("Average nodes: %lf\n", (double)totalNodesVisited/n);

    free(visits);
    return result;
}
