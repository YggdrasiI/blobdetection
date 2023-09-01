#include <assert.h>

#include <string.h>

#include "blobdetection/tree.h"
#include "quicksort.h"

int _cmp_strings_by_value(void **a, void **b)
{
  if (strcmp(*a, *b) < 0) return 1;
  return 0;
}

/* typeof(*a,b) will be (Node *), which points into tree->nodes */
int _cmp_pointer_by_position(void **a, void **b)
{
  return (*a < *b?1:0);
}

int _cmp_nodes_by_data_pointer(void **a, void **b)
{
  Node *n1 = *((Node **)a);
  Node *n2 = *((Node **)b);
  return ((n1->data < n2->data)?1:0);
}

void _swap_pointers(void **a, void **b)
{
  void *tmp = *a;
  *a = *b;
  *b = tmp;
}

void _quicksort_pointer(
    void **begin,
    void **end,
    pointer_compare_func *cmp)
{
  void **ptr;
  void **split;
  if (end - begin <= 1)
    return;
  ptr = begin;
  split = begin + 1;
  while (++ptr != end) {
    if ( cmp(ptr, begin) ) 
    {
      _swap_pointers(ptr, split);
      ++split;
    }
  }
  _swap_pointers(begin, split - 1);
  _quicksort_pointer(begin, split - 1, cmp);
  _quicksort_pointer(split, end, cmp);
}


