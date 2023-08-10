#ifndef ENUMS_H
#define ENUMS_H

// Defines enumes and helper functions to print its variable names
//
// See https://en.wikipedia.org/wiki/X_macro#Example
// for usage of macros for variable name expansions.

#include <stdio.h>

#define FOR_LIST_OF_TREE_ERRORS(DO) \
  DO(  -1, TREE_ERROR_ROOT_HAS_SIBLING) \
  DO(  -2, TREE_ERROR_DUPLICATE) \
  DO(  -3, TREE_ERROR_NODE_IS_PARENT_FOR_NONSIBLING) \
  DO(  -4, TREE_ERROR_TOO_MANY_CHILDREN) \
  DO(  -5, TREE_ERROR_TOO_MANY_LEVELS) \
  DO(  -6, TREE_ERROR_NODE_HAS_WRONG_PARENT) \
  DO( -30, TREE_ERROR_NODE_WIDTH_WRONG) \
  DO( -31, TREE_ERROR_NODE_WIDTH_EXCEEDED) \
  DO( -35, TREE_ERROR_NODE_HEIGHT_WRONG) \
  DO( -36, TREE_ERROR_NODE_HEIGHT_EXCEEDED) \
  DO( -99, TREE_ERROR_PARENT_NODE_IS_NULL) \
  DO(-127, TREE_ERROR_NODE_IS_NULL) \


#define FOR_LIST_OF_TREE_COMPARE_TYPES(DO) \
  DO(1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT) \
  DO(2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED) \
  DO(3, TREE_COMPARE_IF_NODES_ISOMORPH) \
  DO(4, TREE_COMPARE_SAME_DATA_MEMORY_LAYOUT) \
  DO(5, TREE_COMPARE_CHILD_DATA_ORDER_SCRAMBLED) \
  DO(6, TREE_COMPARE_IF_DATA_ISOMORPH) \
  DO(7, TREE_COMPARE_ISOMORPH_NODE_HASH) \
  DO(8, TREE_COMPARE_ISOMORPH_DATA_HASH) \


#define FOR_LIST_OF_TREE_SORTING_TYPES(DO) \
  DO(1, TREE_SORT_NUM_CHILDS) \
  DO(2, TREE_SORT_INT_DATA) \
  DO(3, TREE_SORT_BY_COMPARE_HANDLE) \

// ========================================================f

#define DEFINE_ENUMERATION(id, name, ...) name = id,
enum tree_error_consistent_checks {
  FOR_LIST_OF_TREE_ERRORS( DEFINE_ENUMERATION )
};
enum tree_compare_types {
  FOR_LIST_OF_TREE_COMPARE_TYPES( DEFINE_ENUMERATION )
};
enum tree_sorting_types {
  FOR_LIST_OF_TREE_SORTING_TYPES( DEFINE_ENUMERATION )
};
#undef DEFINE_ENUMERATION

/* Print all defined enums on stdout */
void printf_enums(void);

/* Print enum name into stream */
void fprintf_enum_name(FILE *stream, int id);

/* Print '{enum_name} ({enum_id})' for error types.*/
void fprintf_err(FILE *stream, int err); 


#endif
