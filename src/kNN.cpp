#include "kNN.h"
#include <queue>
#include <math.h>
#include <cstdio>

int nodesVisited;

typedef struct neighbour
{
    float dist; //!< Distance to some query point 
    int index;  //!< Index of neighbour in the original set

}neighbour;
bool operator<(const neighbour &n1, const neighbour &n2){ return n1.dist < n2.dist; }

std::priority_queue<neighbour> queue;

float dist(float *X, float *Y, int dim)
{
    float dist = 0.0;
    for(int i=0; i<dim; i++)
        dist += (X[i] - Y[i])*(X[i] - Y[i]);
    return sqrtf(dist);
}

// The top of queue always keeps the farthest neighbour
void vpt_search(vptree *node, float *query, int d, int k)
{
    if(node == NULL)
        return;
    
    neighbour x = {dist(node->vp, query, d), node->idx};

    if(queue.size() < k)
        queue.push(x);
    else if(x.dist < queue.top().dist)
    {
        queue.pop();
        queue.push(x);
    }

    if(x.dist <= queue.top().dist + node->md)
        vpt_search(node->inner, query, d, k);
    
    if(x.dist >= node->md - queue.top().dist)
        vpt_search(node->outer, query, d, k);
}


knnresult vptree_kNN(vptree *root, float *query, int n, int d, int k)
{
    knnresult result;
    result.k = k; result.m = n;
    result.ndist = (float *)malloc(n*k*sizeof(float));
    result.nidx  = (int *)  malloc(n*k*sizeof(int));

    neighbour x;
    for(int i=0; i<n; i++)
    {
        vpt_search(root, query + i*d, d, k);
        for(int j=k-1; j>=0; j--)
        {
            x = queue.top();
            result.nidx[i*k + j] = x.index;
            result.ndist[i*k + j] = x.dist;
            queue.pop();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////

void kdt_search(kdtree *node, float *query, int d, int k)
{
    if(node==NULL)
        return;
    nodesVisited++;
    neighbour x = {dist(node->p, query, d), node->idx};

    if(queue.size() < k)
        queue.push(x);
    else if(x.dist < queue.top().dist)
    {
        queue.pop();
        queue.push(x);
    }

    int axis = getAxis(node);
    if(node->mc <= queue.top().dist + query[axis])
        kdt_search(getRight(node), query, d, k);
    if(node->mc >= query[axis] - queue.top().dist)
        kdt_search(getLeft(node), query, d, k);
}

knnresult kdtree_kNN(kdtree *root, float *query, int n, int d, int k)
{
    knnresult result;
    result.k = k; result.m = n;
    result.ndist = (float *)malloc(n*k*sizeof(float));
    result.nidx  = (int *)  malloc(n*k*sizeof(int));

    neighbour x;
    for(int i=0; i<n; i++)
    {
        kdt_search(root, query + i*d, d, k);
        for(int j=k-1; j>=0; j--)
        {
            x = queue.top();
            result.nidx[i*k + j] = x.index;
            result.ndist[i*k + j] = x.dist;
            queue.pop();
        }
    }
    printf("Total %d nodes visited\n", nodesVisited);

    return result;
}
