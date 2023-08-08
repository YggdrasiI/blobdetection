#ifndef TREE_INTERN_H
#define TREE_INTERN_H

#ifndef INLINE
#ifdef VISUAL_STUDIO
#define INLINE extern __inline
#else
#define INLINE extern inline
#endif
#endif

/*
 * Helper functions for sorting
 * */
// Swap nodes with same parent
INLINE void swap_silbings(Node *a, Node *b);


/* returns 1, if a>b, and 0 otherwise. (do not use (1, 0, -1) like strcmp ) */
INLINE int cmp(Node *a, Node *b);

INLINE void swap_pnode(Node **a, Node **b);

INLINE int successor(Node *parent, Node *child);

void quicksort_silbings(Node **begin, Node **end);

/*
 * End Helper functions for sorting
 * */

INLINE
void swap_silbings(Node *a, Node *b)
{
  Node *p = a->parent;
  Node *c = p->child;
  Node *d = p->child;
  //serach c, d with ..., c, a, ..., d, b order
  if(c==a) c = NULL;
  else{
    while(c->silbing!=a) c=c->silbing;
  }
  if(d==b) d = NULL;
  else{
    while(d->silbing!=b) d=d->silbing;
  }

  //swap anchor of a and b
  if( c == NULL ) p->child = b;
  else c->silbing = b;
  if( d == NULL ) p->child = a;
  else d->silbing = a;
  
  //at least, swap silbings
  d = a->silbing;
  a->silbing = b->silbing;
  b->silbing = d;
}

INLINE
int cmp(Node *a, Node *b)
{
  if( a->height < b->height) return 0;
  if( a->height > b->height) return 1;
  if( a->width < b->width) return 0;
  if( a->width > b->width) return 1;

  /* Now assme, that children already sorted.
   * Then for topological equalness only i-th child of a needs
   * compared with i-th child of b.
   * */
  int ret=0;
  Node *ca = a->child;
  Node *cb = b->child;
  while( ret == 0 && ca!=NULL ){
    ret = cmp(ca, cb);
    ca=ca->silbing;
    cb=cb->silbing;
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
int successor(Node *parent, Node *child){
  while( child != NULL ){
    child = child->parent;
    if( child == parent ) return 1;
  }
  return 0;
}
#endif
