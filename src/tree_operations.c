// TODO Collecting operations like tree_add_child here.
//
#include <assert.h>
#include <string.h>
#include <stddef.h>

//#define INLINE inline
#include "tree.h"
#include "tree_intern.h"

Tree *tree_clone(
        const Tree * source,
        const void * data,
        const void * cloned_data)
{
  if(!source) return NULL;

  // Without knowlege about the data we cannot duplicate it ourself.
  // Clone data yourself. This method will just update the pointers.
  if ((data == NULL && cloned_data != NULL) ||
      (data != NULL && cloned_data == NULL))
  {
    return NULL;
  }
  Tree *clone = tree_create(source->size, 0);
  if (clone == NULL ) return NULL;

  assert(source->size == clone->size);
  memcpy(clone->nodes, source->nodes, clone->size*sizeof(Node));
  clone->root = source->root;

  // Fixing pointer by adding offsets.
  ptrdiff_t d_tree = clone->nodes - source->nodes;
  ptrdiff_t d_data = cloned_data - data;
  uint32_t s = clone->size;

  clone->root += d_tree;
  Node *cur = clone->nodes;
  while(s){
    if(cur->parent) cur->parent += d_tree;
    if(cur->child) cur->child += d_tree;
    if(cur->sibling) cur->sibling += d_tree;
    if(cur->data) cur->data += d_data;

    --s;++cur;
  }
  return clone;
}


/* ====================  Sorting  ====================*/



#ifdef OLD_STUFF
// TODO: Der Sortieralgo ist merkwürdig und nicht nach topologischen Kriterien
// wie es aber im Header beschrieben ist. Dazu kommt die dumme Rekurion.
// Ab besten noch mal neu beginnen.
void _quicksort_siblings(
    Node **begin,
    Node **end)
{
  Node **ptr;
  Node **split;
  if (end - begin <= 1)
    return;
  ptr = begin;
  split = begin + 1;
  while (++ptr != end) {
    //if ( **ptr < **begin ) {
    if ( cmp(*ptr, *begin)  ) {
      swap_pnode(ptr, split);
      ++split;
    }
  }
  swap_pnode(begin, split - 1);
  _quicksort_siblings(begin, split - 1);
  _quicksort_siblings(split, end);
  }


void tree_sort(
    Node *root)
{
     if( root->width == 0) return; //leaf reached
    /* Sort children and store pointer to this children */
    Node** children = (Node**) malloc( root->width*sizeof(Node*) );
    Node** next = children;
    Node* c = root->child;
    while( c != NULL ){
      tree_sort(c);
      *next=c;
      c = c->sibling;
      next++;
    }
    //now, next points behind children array
    if( root->width > 1){
      Node** end = next;

      _quicksort_siblings(children, end);

      //rearange children of root like sorted array propose.
      c = *children;
      root->child = c;
      next = children+1;
      while(next<end){
        c->sibling = *next;
        c=*next;
        next++;
      }
      c->sibling = NULL; //remove previous anchor in last child.
    }

    free( children );
 }
#endif
