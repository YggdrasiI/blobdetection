#ifndef QUICKSORT_H
#define QUICKSORT_H

#include <limits.h>
#include <stddef.h>

/* Compare function for _quicksort_pointer function.
 *
 * Returns 1, if a>b, and 0 otherwise.
 * Do not use (1, 0, -1) like strcmp for sorting stability.
 */
typedef int pointer_compare_func(void **a, void **b);

int _cmp_strings_by_value(void **a, void **b);
int _cmp_pointer_by_position(void **a, void **b);
int _cmp_nodes_by_data_pointer(void **a, void **b);

void _swap_pointers(void **a, void **b);

/* Sorting for Elements with sizeof(...) = Pointer-size */
void _quicksort_pointer(
    void **begin,
    void **end,
    pointer_compare_func *cmp);


/* Like _quicksort_pointer but with correct pointer types instead of void *
 * Example usage:
 *
 *  typedef struct {
 *    int x;
 *    double y;
 *  } Foo_t;
 *
 * int cmp(Foo_t *a, Foo_t *b){
 *    if ( (a)->x > (b)->x ) return 1;
 *    return 0;
 * }
 *
 * QUICKSORT_TEMPLATE(Foo_t);
 * [...]
 *
 * Foo_t begin[num_elements]; // Pointers on elements.
 * Call sorting by
 * _quicksort_Foo_t(begin, begin+num_elements, cmp);
 */
#define QUICKSORT_TEMPLATE(NAME, STRUCTNAME) \
    typedef int compare_func_##STRUCTNAME (STRUCTNAME *a, STRUCTNAME *b);\
    void _swap_##STRUCTNAME (STRUCTNAME **a, STRUCTNAME **b)\
    {\
      STRUCTNAME *tmp = *a;\
      *a = *b;\
      *b = tmp;\
    }\
    void _quicksort_##NAME (\
        STRUCTNAME **begin,\
        STRUCTNAME **end,\
        compare_func_##STRUCTNAME *cmp)\
    {\
      STRUCTNAME **ptr;\
      STRUCTNAME **split;\
      if (end - begin <= 1)\
        return;\
      ptr = begin;\
      split = begin + 1;\
      while (++ptr != end) {\
        if ( cmp(*ptr, *begin) ) \
        {\
          _swap_##STRUCTNAME (ptr, split);\
          ++split;\
        }\
      }\
      _swap_##STRUCTNAME (begin, split - 1);\
      _quicksort_##NAME (begin, split - 1, cmp);\
      _quicksort_##NAME (split, end, cmp);\
    }

#endif
