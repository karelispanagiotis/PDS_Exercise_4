/*!
  \file   tester.c
  \brief  Validate Vantage Point implementation.

  \author Dimitris Floros
  \date   2019-10-19
*/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>
#include "vptree.h"

// #define VERBOSE 

static char * STR_CORRECT_WRONG[] = {"WRONG", "CORRECT"};


// =========================================================
// === STACK IMPLEMENTATION TO TEST RECUSRION INVARIANCE ===
// =========================================================

// Declare linked list node 
  
typedef struct node { 
  vptree *T;
  int isInner;
  float md;
  struct node* link; 
} node;

  
// Utility function to add an element data in the stack 
// insert at the beginning 
void push(node **top, vptree *T, float md, int isInner) 
{ 
  // create new node temp and allocate memory 
  node *temp = (node *) malloc( sizeof(node) ); 

#ifdef VERBOSE
  fprintf( stdout, "PUSH ENTERED\n");
#endif
  
  // check if stack (heap) is full. Then inserting an element would 
  // lead to stack overflow 
  if (!temp) { 
    fprintf( stderr, "\npush: Heap Overflow\n" );
    exit(1);
  } 
  
  // initialize data into temp data field 
  temp->T       = T; 
  temp->isInner = isInner;
  temp->md      = md;
  
  // put top pointer reference into temp link 
  temp->link = *top;
  
  // make temp as top of Stack 
  *top = temp;
  
}

// Remove top element from stack
void pop(node **top) 
{ 

#ifdef VERBOSE
    fprintf( stdout, "POP ENTERED\n");
#endif
    
    // check for stack underflow 
    if (*top == NULL) { 
      fprintf( stderr, "\npop: Stack Underflow\n" );
      exit(1); 
    } 

    // top assign into temp 
    struct node* temp = *top; 

    // return second as top
    *top = (*top)->link; 
    
    // destroy connection between first and second 
    temp->link = NULL; 
  
    // release memory of top node 
    free(temp);

}




// =================
// === UTILITIES ===
// =================

float dist(float *X, float *Y, int d){
  float dist2 = 0;
  for (int i = 0; i < d; i++){
    dist2 += (X[i] - Y[i])*(X[i] - Y[i]);
  }
  return sqrt(dist2);
}

// Function to print all the  
// elements of the stack  
int verifyLeafPlace(node **top, float *X, int d)
{ 

  
  // check for stack underflow 
  if (*top == NULL) { 
    fprintf( stderr, "\nverifyLeafPlace: Stack Underflow\n" );
    exit(1); 
  }
  
  struct node* temp = *top;

  // iterate the ancestors in stack
  while (temp != NULL) { 

#ifdef VERBOSE
    fprintf( stdout, "%f | %f | %d == %d\n", dist(X, getVP(temp->T), d), temp->md, temp->isInner, dist(X, getVP(temp->T), d) <= temp->md );
#endif

    // check whether point should be inside or outside
    int isInner =  dist(X, getVP(temp->T), d) <= temp->md;
    
    // if the direction is invalid, break and return false
    if ( isInner != temp->isInner )
    {
      fprintf( stdout, "%f | %f | %d == %d\n", dist(X, getVP(temp->T), d), temp->md, temp->isInner, dist(X, getVP(temp->T), d) <= temp->md );
      return 0;
    } 
    
    // assign temp link to temp 
    temp = temp->link; 
  }

  return 1;
  
} 




// ==================
// === VALIDATION ===
// ==================

int *foundInTree;

int verifyTree(vptree *T, float *vp, node **stack, float md, int isInner,
               int n, int d){

  int isValid = 1;
  
  // if empty, return
  if (T == NULL) return isValid;

  int isValidAncestor = 1;
  int isLeaf = (getInner(T) == NULL && getOuter(T) == NULL);

#ifdef VERBOSE
  fprintf( stdout, "%x %x\n", stack, *stack);
#endif  

  // if leaf check ancestor path
  if (isLeaf) isValidAncestor = verifyLeafPlace(stack, getVP(T), d);


  // if inner, radius must be smaller than parent's diameter
  if (isInner && getMD(T) > 2*md) return 0;

  // update list of indices
  int idx = getIDX(T);
  if (idx < n)
    foundInTree[ idx ] = 1;

  // validate distance to parent
  if (isInner)
    isValid = dist(vp, getVP(T), d) <= md;
  else
    isValid = dist(vp, getVP(T), d) > md;

  // recurse if not leaf
  if (!isLeaf) {

    // add to stack as inner and recurse, then pop
    push( stack, T, getMD(T), 1 );
    int isValidInn = verifyTree( getInner( T ), getVP(T), stack, getMD(T), 1, n, d );
    pop( stack );

    // add to stack as outer and recurse, then pop
    push( stack, T, getMD(T), 0 );
    int isValidOut = verifyTree( getOuter( T ), getVP(T), stack, getMD(T), 0, n, d );
    pop( stack );
  
    // all conditions must be true
    isValid = isValid && isValidInn && isValidOut;

  } else {

    // make sure ancestory path is correct
    isValid = isValid && isValidAncestor;
    
  }

  // return 
  return isValid;
  
}



int main(int argc, char *argv[])
{

  int n=atoi(argv[1]);//data
  int d=atoi(argv[2]);//dimensions

  float  * dataArr = (float * ) malloc( n*d * sizeof(float) );
  float  * zeros   = (float * ) calloc( d   , sizeof(float) );

  foundInTree = (int *) calloc( n, sizeof(int) );
  
  for (int i=0;i<n*d;i++)
    dataArr[i]=(float)rand()/RAND_MAX;

  printf("Initialized CPU data...\n");
  // Time Measurement
  struct timeval start, end;
  gettimeofday(&start, NULL);
  vptree *root=buildvp(dataArr,n,d);
  gettimeofday(&end, NULL);
  
  node *stack = NULL;
  
  int isValid = verifyTree(root, zeros, &stack, 1.0/0.0, 1, n, d );
  int foundAll = 1;

  assert( stack == NULL );
  
  for (int i = 0; i<n; i++)
    if (!foundInTree[i]){
      foundAll = 0;
      break;
    }
  
  long time_usec = (end.tv_sec - start.tv_sec)*1000000 + end.tv_usec - start.tv_usec;
  printf("Tester validation: %s PROPERTIES | %s INDICES\n",
           STR_CORRECT_WRONG[isValid], STR_CORRECT_WRONG[foundAll]);
  printf("%s:Construction of tree took %lf\n", argv[0], (double)time_usec/1000000);

  return 0;
  
}
