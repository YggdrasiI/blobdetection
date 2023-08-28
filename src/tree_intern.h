#ifndef TREE_INTERN_H
#define TREE_INTERN_H

#include <stddef.h>
#include <stdint.h>

#include <assert.h>

#include "blobdetection/tree.h"

#ifndef INLINE
#define UNDEF_INLINE
#ifdef VISUAL_STUDIO
#define INLINE __inline 
//#define INLINE extern __inline 
//#define INLINE extern __inline __forceinline
#else
//#define INLINE extern inline
//#define INLINE extern inline __attribute__((always_inline))
#define INLINE inline 
//#define INLINE inline __attribute__((always_inline))
// This throws an error at compile time if no extern variant exists
// (=> Inlining failed at a position it shouldn't)
#endif
#endif

#define _unused(x) ((void)(x)) // For asserts and -Wunused-but-set-variable warning

/* realloc did not free array on source location on error. 
 * Let's free it in this wrapper function before the pointer will be overwritten.
 * */
static inline
void *_reallocarray_or_free(void *ptr, size_t nmemb, size_t size){
    void *ptr_new = reallocarray(ptr, nmemb, size);
    if(ptr_new == NULL) {
        assert(0);
        free(ptr);
    }
    return ptr_new;
}


/*
 * Helper functions for sorting
 * */
// Swap nodes with same parent
  INLINE
void swap_siblings(Node *a, Node *b)
{
  Node *p = a->parent;
  Node *c = p->child;
  Node *d = p->child;
  //serach c, d with ..., c, a, ..., d, b order
  if(c==a) c = NULL;
  else{
    while(c->sibling!=a) c=c->sibling;
  }
  if(d==b) d = NULL;
  else{
    while(d->sibling!=b) d=d->sibling;
  }

  //swap anchor of a and b
  if( c == NULL ) p->child = b;
  else c->sibling = b;
  if( d == NULL ) p->child = a;
  else d->sibling = a;

  //at least, swap siblings
  d = a->sibling;
  a->sibling = b->sibling;
  b->sibling = d;
}

/* returns 1, if a>b, and 0 otherwise. (do not use (1, 0, -1) like strcmp ) */
// This function can not be inlined due its recurcive call.
static inline
int32_t cmp(Node *a, Node *b)
{
  if( a->height < b->height) return 0;
  if( a->height > b->height) return 1;
  if( a->width < b->width) return 0;
  if( a->width > b->width) return 1;

  /* Now assume, that children already sorted.
   * Then for topological equalness only i-th child of a needs
   * compared with i-th child of b.
   * */
  int32_t ret=0;
  Node *ca = a->child;
  Node *cb = b->child;
  while( ret == 0 && ca!=NULL ){
    ret = cmp(ca, cb);
    ca=ca->sibling;
    cb=cb->sibling;
  }
  return ret;
}

  INLINE
void swap_pnode(Node **a, Node **b)
{
  Node *tmp = *a;
  *a = *b;
  *b = tmp;
}

  INLINE
int32_t successor(Node *parent, Node *child)
{
  while( child != NULL ){
    child = child->parent;
    if( child == parent ) return 1;
  }
  return 0;
}

void *quicksort_siblings(Node **begin, Node **end);
/*
 * END Helper functions for sorting
 * */

/*
 * Helper functions for printing images on terminal 
 * */
uint8_t * debug_getline(void);
void debug_print_matrix(
        uint32_t* data,
        uint32_t w, uint32_t h,
        BlobtreeRect roi,
        uint32_t gridw, uint32_t gridh);
void debug_print_matrix2(
        uint32_t* ids,
        uint32_t* data,
        uint32_t w, uint32_t h,
        BlobtreeRect roi,
        uint32_t gridw, uint32_t gridh,
        int8_t twice);
void debug_print_matrix_char(
        uint8_t * data,
        uint32_t w, uint32_t h,
        BlobtreeRect roi,
        uint32_t gridw, uint32_t gridh);
/*
 * END Helper functions for printing images on terminal 
 * */


// Function used for intern hashing of trees
uint32_t qhashmurmur3_32(const void *data, size_t nbytes);
int qhashmurmur3_128(const void *data, size_t nbytes, void *retbuf);


#ifdef UNDEF_INLINE
#undef INLINE
#undef UNDEF_INLINE
#endif
#endif
