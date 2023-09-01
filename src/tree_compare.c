#include <assert.h>
#include <limits.h>
#include <stddef.h>

#include <string.h>

#include "blobdetection/enums.h"
#include "blobdetection/tree.h"
//#include "blobdetection/tree_sort.h"

#include "tree_intern.h"

// Force gcc to inline looping over trees.
// This probably removes the 'if (function_handler == NULL)' checks.
//#define INLINE inline __attribute__((always_inline))
#include "tree_loops.h"

#include "quicksort.h"


/* This changes the order of some operations.
 *
 * If nodes were resorted in some of the functions
 * compare this siblings directly and not after traversing normally 
 * over the tree.
 */
#define DIRECT_CHECK_AFTER_SORTING

// Collection of comparator functions for Tree struct.


#if 0
/* Define type for compare functions.
 *
 * • Both nodes an be assumed as != NULL (Nodes already checked by outer loop.)
 * • Data holds struct with specific data needed for the different
 * compare types, e.g pointer to the root nodes.
 * */
typedef int node_compare_func(Node *n1, Node *n2, void *compare_data);

/* Tree looping function for node_compare_func handlers*/
int _tree_cmp(
        Tree * const tree1,
        Tree * const tree2,
        node_compare_func *cmp_on_enter_node,
        node_compare_func *cmp_on_leaf,
        node_compare_func *cmp_on_leave_last_child,
        void *compare_data);
#endif

// ==============================================
/* Relation of compare functions:
 *
 * 1: TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT
 * 2: TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED
 * 3: TREE_COMPARE_IF_NODES_ISOMORPH
 * 4: TREE_COMPARE_SAME_DATA_MEMORY_LAYOUT
 * 5: TREE_COMPARE_CHILD_DATA_ORDER_SCRAMBLED
 * 6: TREE_COMPARE_IF_DATA_ISOMORPH
 *
 * TODO
 * 7: TREE_COMPARE_ISOMORPH_NODE_HASH
 * 8: TREE_COMPARE_ISOMORPH_DATA_HASH
 * 9: TREE_COMPARE_IF_NODES_ISOMORPH_FILTERED
 * 10: TREE_COMPARE_IF_DATA_ISOMORPH_FILTERED
 *
 * 1 implies 2, 2 implies 3
 * 4 implies 5, 5 implies 6
 */
// Begin variants

/* Variant 1: TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT
 *
 * Check if all node pointer have the same distance to the root nodes.
 * */
typedef struct {
    const Node *anchor1;
    const Node *anchor2;
} compare_func1_data;


int child_compare_func1(Node *n1, Node *n2, void *_compare_data) {
  const compare_func1_data * compare_data = (const compare_func1_data *)_compare_data;
  ptrdiff_t d1 = n1 - compare_data->anchor1;
  ptrdiff_t d2 = n2 - compare_data->anchor2;
  if (d2-d1) return d2-d1;

  return 0;
}

int parent_compare_func1(Node *n1, Node *n2, void *_compare_data) {
  const compare_func1_data * compare_data = (const compare_func1_data *)_compare_data;
  ptrdiff_t d1 = n1->parent - compare_data->anchor1;
  ptrdiff_t d2 = n2->parent - compare_data->anchor2;
  if (d2-d1) return d2-d1;

  return 0;
}

/* Variant 2: TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED
 *
 * Assume that s:tree2->tree2 is a bijection with 
 *  s(n)->parent == n->parent. (Thus it re-orders siblings)
 *
 * Check an re-ordering s:Nodes(tree2)->Nodes(tree2) exists which fulfill
 * TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT for (tree1, s(tree2))
 * */
typedef struct {
    const Node *anchor1;
    const Node *anchor2;
    // Up to here, the same structure as compare_func1_data
    size_t num_sorting_siblings;
    Node **sorting_siblings1;
    Node **sorting_siblings2;
} compare_func2_data;

void destroy_compare_func2_data(compare_func2_data *d){
  free(d->sorting_siblings1);
  free(d->sorting_siblings2);
}

//int compare_func2(Node *n1, Node *n2, void *compare_data){}
#define child_compare_func2 child_compare_func1

int parent_compare_func2(Node *n1, Node *n2, void *_compare_data) {
  compare_func2_data * compare_data = (compare_func2_data *)_compare_data;

  // This function re-sorts child on left-most child and just compare results
  // on all other nodes. (Maybe it's better to check all siblings directly in first call?!)
  if( n1->parent->child != n1 ) return child_compare_func2(n1->parent, n2->parent, _compare_data);

  // Search bijection between nodes of n1->parent and n2->parent by sorting both sets
#ifdef TREE_REDUNDANT_INFOS
  const uint32_t num_nodes = n1->parent->width;
  const uint32_t num_nodes2 = n2->parent->width;
  if(num_nodes != num_nodes2) return num_nodes2-num_nodes;

  // Guarantee enough space.
  if (num_nodes > compare_data->num_sorting_siblings) {
    free(compare_data->sorting_siblings1);
    free(compare_data->sorting_siblings2);
    compare_data->num_sorting_siblings = MAX(num_nodes+8,
        2 * compare_data->num_sorting_siblings);
    compare_data->sorting_siblings1 = malloc(
        compare_data->num_sorting_siblings*sizeof(Node *));
    compare_data->sorting_siblings2 = malloc(
        compare_data->num_sorting_siblings*sizeof(Node *));
    if (compare_data->sorting_siblings1 == NULL || compare_data->sorting_siblings2 == NULL ){
      compare_data->num_sorting_siblings = 0;
      assert(0);
      return -4;
    }
  }

  // Fill arrays
  Node *cur1 = n1->parent->child;
  Node *cur2 = n2->parent->child;
  Node **array_cur1 = compare_data->sorting_siblings1;
  Node **array_cur2 = compare_data->sorting_siblings2;
  while(cur1){
    *array_cur1 = cur1;
    *array_cur2 = cur2;
    cur1 = cur1->sibling;
    cur2 = cur2->sibling;
    ++array_cur1;
    ++array_cur2;
  }
#else
  // Combined step to count array size and filling.
  Node *cur1 = n1->parent->child;
  Node *cur2 = n2->parent->child;
  uint32_t node_index = 0;
  while(cur1 && cur2){
    // Realloc destination array if too small
    if (node_index >= compare_data->num_sorting_siblings) {
      compare_data->num_sorting_siblings = MAX(node_index+8,
          2 * compare_data->num_sorting_siblings);
      compare_data->sorting_siblings1 = _reallocarray_or_free(compare_data->sorting_siblings1,
          compare_data->num_sorting_siblings, sizeof(Node *));
      compare_data->sorting_siblings2 = _reallocarray_or_free(compare_data->sorting_siblings1,
          compare_data->num_sorting_siblings, sizeof(Node *));
      if (compare_data->sorting_siblings1 == NULL || compare_data->sorting_siblings2 == NULL ){
        compare_data->num_sorting_siblings = 0;
        assert(0);
        return -4;
      }
    }

    // Fill arrays
    compare_data->sorting_siblings1[node_index] = cur1;
    compare_data->sorting_siblings2[node_index] = cur2;

    cur1 = cur1->sibling;
    cur2 = cur2->sibling;
    ++node_index;
  }
  if( cur1 || cur2){ //One parent node had an extra sibling.
    return cur2 - cur1;
  }
  uint32_t num_nodes = node_index;
#endif

  // Sorting
  _quicksort_pointer(
      (void **)compare_data->sorting_siblings1,
      (void **)compare_data->sorting_siblings1+num_nodes,
      _cmp_pointer_by_position);
  _quicksort_pointer(
      (void **)compare_data->sorting_siblings2,
      (void **)compare_data->sorting_siblings2+num_nodes,
      _cmp_pointer_by_position);

#ifdef DIRECT_CHECK_AFTER_SORTING
  // Check if canonical form is the same.
  uint32_t idx = num_nodes;
  while(idx--){
    Node *n1 = compare_data->sorting_siblings1[idx];
    Node *n2 = compare_data->sorting_siblings2[idx];
    int d = child_compare_func2(n1, n2, _compare_data);
    if (d) return d;
  }

  // Update node order in both trees to canonical form.
  Node **pn1 = compare_data->sorting_siblings1;
  Node **pn2 = compare_data->sorting_siblings2;
  for(idx = 0; idx<num_nodes-1; ++idx){
    (*pn1)->sibling = *(pn1+1);
    (*pn2)->sibling = *(pn2+1);
    ++pn1; ++pn2;
  }
  (*pn1)->sibling = NULL;
  (*pn2)->sibling = NULL;
#else // Just check leftmost node.

  // Update node order in both trees to canonical form.
  Node **pn1 = compare_data->sorting_siblings1;
  Node **pn2 = compare_data->sorting_siblings2;
  for(idx = 0; idx<num_nodes-1; ++idx){
    (*pn1)->sibling = *(pn1+1);
    (*pn2)->sibling = *(pn2+1);
    ++pn1; ++pn2;
  }
  (*pn1)->sibling = NULL;
  (*pn2)->sibling = NULL;

  int d = child_compare_func2(compare_data->sorting_siblings1[0],
      compare_data->sorting_siblings2[0], compare_data);
  if (d) return d;
#endif

  return 0;
}

/* Variant 3: TREE_COMPARE_IF_NODES_ISOMORPH
 *
 * Check if an ismorphism iso:tree1 -> tree2 exists. 
 *
 * In contrast to TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED
 * this variant ignores the node positions in the underlying array.
 * This is probably the most practical variant for users.
 * */
// See tree_sort.c

/* Variant 4: TREE_COMPARE_SAME_DATA_MEMORY_LAYOUT
 *
 * Check if trees had the same structure and 
 * for all nodes with data nA_i and nB_i of tree i holds:
 *     nB_0.data - nA_0.data == nB_1.data - nB_1.data

 * Note that data pointer of both trees can differ and interal
 * nodes during the path traversal of both trees can differ, too.
 * //Check if nodes of both trees have same distance to each root node.
 * //and made the same check for data handlers, if both given.
 *
 * */
typedef struct {
    //const Node *anchor1;
    //const Node *anchor2;
    void *data1_anchor;
    void *data2_anchor;
} compare_func4_data;


int child_compare_func4(Node *n1, Node *n2, void *_compare_data) {
  compare_func4_data * compare_data = (compare_func4_data *)_compare_data;
  if (n1->data){
    if (compare_data->data1_anchor == NULL){
        compare_data->data1_anchor = n1->data;
    }
    if (n2->data == NULL) return -1; // Second data pointer NULL
  }
  else if (n2->data){
    if (compare_data->data2_anchor == NULL){
        compare_data->data2_anchor = n2->data;
    }
    return 1; // First data pointer NULL
  }
  else { // Both data pointers NULL
    return 0;
  }

  // Previous if's guarantees that both ->data entries are != NULL
  assert(n1->data != NULL && n2->data != NULL);

  ptrdiff_t d1 = n1->data - compare_data->data1_anchor;
  ptrdiff_t d2 = n2->data - compare_data->data2_anchor;
  return d2-d1;
}

int parent_compare_func4(Node *n1, Node *n2, void *compare_data) {
    return child_compare_func4(n1->parent, n1->parent, compare_data);
}

/* Variant 5: TREE_COMPARE_CHILD_DATA_ORDER_SCRAMBLED
 *
 * LIKE TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED but compare data pointer
 * instead of node's own position.
 *
 * Attention:
 *    • All ->data pointers needs to be set. 
 *      Otherwise False-Negative results are possible.
 *    • It just checks the pointer position, not the content.
 * */
typedef struct {
    //const Node *anchor1;
    //const Node *anchor2;
    void *data1_anchor;
    void *data2_anchor;

    size_t num_sorting_siblings;
    Node **sorting_siblings1;
    Node **sorting_siblings2;
} compare_func5_data;

void destroy_compare_func5_data(compare_func5_data *d){
  free(d->sorting_siblings1);
  free(d->sorting_siblings2);
}

/* This function will be called for leafs or
 * already sorted(!) siblings of its parent node. */
int child_compare_func5(Node *n1, Node *n2, void *_compare_data) {
  compare_func5_data * compare_data = (compare_func5_data *)_compare_data;
  if (n1->data){
    if (compare_data->data1_anchor == NULL){
        compare_data->data1_anchor = n1->data;
    }
    if (n2->data == NULL) return -1; // Second data pointer NULL
  }
  else if (n2->data){
    if (compare_data->data2_anchor == NULL){
        compare_data->data2_anchor = n2->data;
    }
    return 1; // First data pointer NULL
  }
  else { // Both data pointers NULL
    return 0;
  }

  // Previous if's guarantees that both ->data entries are != NULL
  assert(n1->data != NULL && n2->data != NULL);

  ptrdiff_t d1 = n1->data - compare_data->data1_anchor;
  ptrdiff_t d2 = n2->data - compare_data->data2_anchor;
  return d2-d1;
}

int parent_compare_func5(Node *n1, Node *n2, void *_compare_data) {
  compare_func5_data * compare_data = (compare_func5_data *)_compare_data;

  // This function re-sorts child on left-most child and just compare results
  // on all other nodes. (Maybe it's better to check all siblings directly in first call?!)
#ifdef DIRECT_CHECK_AFTER_SORTING
  if( n1->parent->child != n1 ) return 0; // We had already checked.
#else
  if( n1->parent->child != n1 ) return child_compare_func5(n1->parent, n2->parent, compare_data);
#endif

  // Search bijection between nodes of n1->parent and n2->parent by sorting both sets
#ifdef TREE_REDUNDANT_INFOS
  const uint32_t num_nodes = n1->parent->width;
  const uint32_t num_nodes2 = n2->parent->width;
  if(num_nodes != num_nodes2) return num_nodes2-num_nodes;

  // Guarantee enough space.
  if (num_nodes > compare_data->num_sorting_siblings) {
    free(compare_data->sorting_siblings1);
    free(compare_data->sorting_siblings2);
    compare_data->num_sorting_siblings = MAX(num_nodes+8,
        2 * compare_data->num_sorting_siblings);
    compare_data->sorting_siblings1 = malloc(
        compare_data->num_sorting_siblings*sizeof(Node *));
    compare_data->sorting_siblings2 = malloc(
        compare_data->num_sorting_siblings*sizeof(Node *));
    if (compare_data->sorting_siblings1 == NULL || compare_data->sorting_siblings2 == NULL ){
      compare_data->num_sorting_siblings = 0;
      assert(0);
      return -4;
    }
  }

  // Fill arrays
  Node *cur1 = n1->parent->child;
  Node *cur2 = n2->parent->child;
  Node **array_cur1 = compare_data->sorting_siblings1;
  Node **array_cur2 = compare_data->sorting_siblings2;
  while(cur1){
    *array_cur1 = cur1;
    *array_cur2 = cur2;
    cur1 = cur1->sibling;
    cur2 = cur2->sibling;
    ++array_cur1;
    ++array_cur2;
  }
#else
  // Combined step to count array size and filling.
  Node *cur1 = n1->parent->child;
  Node *cur2 = n2->parent->child;
  uint32_t node_index = 0;
  while(cur1 && cur2){
    // Realloc destination array if too small
    if (node_index >= compare_data->num_sorting_siblings) {
      compare_data->num_sorting_siblings = MAX(node_index+8,
          2 * compare_data->num_sorting_siblings);
      compare_data->sorting_siblings1 = _reallocarray_or_free(compare_data->sorting_siblings1,
          compare_data->num_sorting_siblings, sizeof(Node *));
      compare_data->sorting_siblings2 = _reallocarray_or_free(compare_data->sorting_siblings1,
          compare_data->num_sorting_siblings, sizeof(Node *));
      if (compare_data->sorting_siblings1 == NULL || compare_data->sorting_siblings2 == NULL ){
        compare_data->num_sorting_siblings = 0;
        assert(0);
        return -4;
      }
    }

    // Fill arrays
    compare_data->sorting_siblings1[node_index] = cur1;
    compare_data->sorting_siblings2[node_index] = cur2;

    cur1 = cur1->sibling;
    cur2 = cur2->sibling;
    ++node_index;
  }
  if( cur1 || cur2){ //One parent node had an extra sibling.
    return cur2 - cur1;
  }
  uint32_t num_nodes = node_index;
#endif

  // Sorting
  _quicksort_pointer(
      (void **)compare_data->sorting_siblings1,
      (void **)compare_data->sorting_siblings1+num_nodes,
      _cmp_nodes_by_data_pointer);
  _quicksort_pointer(
      (void **)compare_data->sorting_siblings2,
      (void **)compare_data->sorting_siblings2+num_nodes,
      _cmp_nodes_by_data_pointer);

#ifdef DIRECT_CHECK_AFTER_SORTING
  // Check if canonical form is the same.
  uint32_t idx = num_nodes;
  while(idx--){
    Node *n1 = compare_data->sorting_siblings1[idx];
    Node *n2 = compare_data->sorting_siblings2[idx];
    int d = child_compare_func5(n1, n2, compare_data);
    if (d) return d;
  }

  // Update node order in both trees to canonical form.
  Node **pn1 = compare_data->sorting_siblings1;
  Node **pn2 = compare_data->sorting_siblings2;
  for(idx = 0; idx<num_nodes-1; ++idx){
    (*pn1)->sibling = *(pn1+1);
    (*pn2)->sibling = *(pn2+1);
    ++pn1; ++pn2;
  }
  (*pn1)->sibling = NULL;
  (*pn2)->sibling = NULL;
#else // Just check leftmost node.

  // Update node order in both trees to canonical form.
  Node **pn1 = compare_data->sorting_siblings1;
  Node **pn2 = compare_data->sorting_siblings2;
  for(idx = 0; idx<num_nodes-1; ++idx){
    (*pn1)->sibling = *(pn1+1);
    (*pn2)->sibling = *(pn2+1);
    ++pn1; ++pn2;
  }
  (*pn1)->sibling = NULL;
  (*pn2)->sibling = NULL;

  int d = child_compare_func5(compare_data->sorting_siblings1[0],
      compare_data->sorting_siblings2[0], compare_data);
  if (d) return d;
#endif

  return 0;
}

/* Variant 6: TREE_COMPARE_IF_DATA_ISOMORPH
 *
 * LIKE TREE_COMPARE_IF_NODES_ISOMORPH but compare data pointer
 * instead of node's own position.
 *
 * Attention: It just checks the pointer position, not the content.
 * */
typedef struct {
  const Node *anchor1;
  const Node *anchor2;
  const void *tree1_data;
  const void *tree2_data;
} compare_func6_data;


int compare_func6(Node *n1, Node *n2, void *compare_data) {
  return 0;
}

/* Variant 7: TREE_COMPARE_ISOMORPH_NODE_HASH
 *
 * Check if an ismorphism iso:tree1 -> tree2 exists. 
 * Alternative approach of TREE_COMPARE_IF_NODES_ISOMORPH
 * which combines hashes of subtrees such that
 * isomorph trees got the same root hash. 
 *
 * Attention, the case of a hash collision will be not catched.
 *  Nevertheless, a hash collision is unlikely enough in most application.
 */
typedef struct {
  const Node *anchor1;
  const Node *anchor2;

  const void *tree1_hash;
  const void *tree2_hash;
} compare_func7_data;

int compare_func7(Node *n1, Node *n2, void *_compare_data) {
  //const compare_func7_data * compare_data = (const compare_func7_data *)_compare_data;

  return 0;
}

/* Variant 8: TREE_COMPARE_ISOMORPH_DATA_HASH
 *
 * Check if an ismorphism iso:tree1 -> tree2 exists, which
 * also respects the data pointer.
 * Alternative approach of TREE_COMPARE_IF_DATA_ISOMORPH
 *
 * Attention, the case of a hash collision will be not catched.
 *  Nevertheless, a hash collision is unlikely enough in most application.
 */
typedef struct {
  const Node *anchor1;
  const Node *anchor2;

  const void *tree1_hash;
  const void *tree2_hash;
} compare_func8_data;

int compare_func8(Node *n1, Node *n2, void *_compare_data) {
  //const compare_func8_data * compare_data = (const compare_func8_data *)_compare_data;

  return 0;
}


// End variants
// ==============================================

/*
 * Check if the parent-child-relation matches
 * and if
 *    (node_i^1 - root^1) == (node_i^2 - root^2) ∀node_i^j∈ tree^j 
 *
 * Returns 0 if distance of child pointers to root node is 
 * the same for all nodes.
 *
 * Elements of node array which are not connected to the
 * root node will be ignored.
 */
int tree_cmp_same_node_memory_layout(
    const Tree * const tree1,
    const Tree * const tree2)
{
  assert(tree1 != NULL && tree2 != NULL);

  // Non-const pointers, but underlaying handlers will not alter trees.
  Tree *_tree1 = (Tree *)tree1;
  Tree *_tree2 = (Tree *)tree2;

  compare_func1_data data = {
    .anchor1 = _tree1->nodes,
    .anchor2 = _tree2->nodes,
  };
  return _trees_depth_first_search(_tree1, _tree2,
      child_compare_func1, parent_compare_func1,
      NULL, NULL, &data);
}


int tree_cmp_child_node_order_scrambled(
    const Tree * const tree1,
    const Tree * const tree2)
{
  assert(tree1 != NULL && tree2 != NULL);

  // Create copies because we will re-sort childs during the operation.
  // (It would be possible to shrink this on one clone.)
  Tree *_tree1 = tree_clone(tree1, NULL, NULL);
  Tree *_tree2 = tree_clone(tree2, NULL, NULL);

  compare_func2_data data = {
    .anchor1 = _tree1->nodes,
    .anchor2 = _tree2->nodes,
    .num_sorting_siblings = 0,
    .sorting_siblings1 = NULL,
    .sorting_siblings2 = NULL,
  };
  int ret = _trees_depth_first_search(_tree1, _tree2,
      child_compare_func2, parent_compare_func2,
      NULL, NULL, &data);

  // Cleanup
  destroy_compare_func2_data(&data);
  tree_destroy(&_tree1);
  tree_destroy(&_tree2);

  return ret;
}

int tree_cmp_if_nodes_isomorph(
    const Tree * const tree1,
    const Tree * const tree2)
{
  assert(tree1 != NULL && tree2 != NULL);

  /* The data to compare the trees will be build 'down to top'.
   *
   * Thus, we can overwrite data from child nodes after we reached
   * a node in post-order. Nevertheless we use two buffers and
   * nodes of depth N will read from buffer (N+1)%2
   * and write into buffer (N+0)%2.
   * This preventing overlapping of strings during writing the node label.
   */
  /* 
   * We do not clone the input trees and for this reason we can not use
   * the data pointer without overwriting user data.
   * Instead, we're using 'nodeX - treeX->root' as index into an new data array.
   * 
   * (Cloning the input trees looks more practical, but we want use this function 
   * in the TREE_COMPARE_IF_DATA_ISOMORPH algorithm and there,
   * we need the input data pointer.
   */
  /* One terminal char, 'o', for each leaf and
   * two Terminals '(',')'  for each non-leaf will be used.
   * Thus 2*num_nodes(t)+1 chars can store the whole string,
   *  where num_nodes(t) <= t->size.
   */
  char *label_tree1 = tree_canonical_label(tree1);
  if (label_tree1 == NULL) return -1;
  char *label_tree2 = tree_canonical_label(tree2);
  if (label_tree2 == NULL) {
    free(label_tree1);
    return 1;
  }

  int ret = strcmp(label_tree1, label_tree2);

  // Cleanup
  free(label_tree1);
  free(label_tree2);
  return ret;
}

int tree_cmp_same_data_memory_layout(
    const Tree * const tree1,
    const Tree * const tree2)
{
  assert(tree1 != NULL && tree2 != NULL);

  // Non-const pointers, but underlaying handlers will not alter trees.
  Tree *_tree1 = (Tree *)tree1;
  Tree *_tree2 = (Tree *)tree2;

  compare_func4_data data = {
    .data1_anchor = NULL, /* Set on first found data_poiter != NULL */
    .data2_anchor = NULL,
  };
  return _trees_depth_first_search(_tree1, _tree2,
      child_compare_func4, parent_compare_func4,
      NULL, NULL, &data);
}


int tree_cmp_child_data_order_scrambled(
    const Tree * const tree1,
    const Tree * const tree2)
{
  assert(tree1 != NULL && tree2 != NULL);

  // Create copies because we will re-sort childs during the operation.
  // (It would be possible to shrink this on one clone.)
  Tree *_tree1 = tree_clone(tree1, NULL, NULL);
  Tree *_tree2 = tree_clone(tree2, NULL, NULL);

  compare_func5_data data = {
    .data1_anchor = NULL, /* Set on first found data_poiter != NULL */
    .data2_anchor = NULL,
    .num_sorting_siblings = 0,
    .sorting_siblings1 = NULL,
    .sorting_siblings2 = NULL,
  };
  int ret = _trees_depth_first_search(_tree1, _tree2,
      child_compare_func5, parent_compare_func5,
      NULL, NULL, &data);

  // Cleanup
  destroy_compare_func5_data(&data);
  tree_destroy(&_tree1);
  tree_destroy(&_tree2);

  return ret;
}

int tree_cmp_if_data_isomorph(
    const Tree * const tree1,
    const Tree * const tree2)
{
  assert(tree1 != NULL && tree2 != NULL);

  /* The data to compare the trees will be build 'down to top'.
   *
   * Thus, we can overwrite data from child nodes after we reached
   * a node in post-order. Nevertheless we use two buffers and
   * nodes of depth N will read from buffer (N+1)%2
   * and write into buffer (N+0)%2.
   * This preventing overlapping of strings during writing the node label.
   */
  /* 
   * We do not clone the input trees and for this reason we can not use
   * the data pointer without overwriting user data.
   * Instead, we're using 'nodeX - treeX->root' as index into an new data array.
   * 
   * (Cloning the input trees looks more practical, but we want use this function 
   * in the TREE_COMPARE_IF_DATA_ISOMORPH algorithm and there,
   * we need the input data pointer.
   */
  /* One terminal char, 'o', for each leaf and
   * two Terminals '(',')'  for each non-leaf will be used.
   * Thus 2*num_nodes(t)+1 chars can store the whole string,
   *  where num_nodes(t) <= t->size.
   */
  Tree *tree1_ordered = NULL, *tree2_ordered = NULL;
  char *label_tree1 = NULL, *label_tree2 = NULL;
  int ret;

  ret = tree_sort_canonical_order(tree1, &tree1_ordered, &label_tree1);
  if (ret) return -1;
  ret = tree_sort_canonical_order(tree2, &tree2_ordered, &label_tree2);
  if (ret){
    tree_destroy(&tree1_ordered);
    free(label_tree1);
    return 1;
  }

  ret = strcmp(label_tree1, label_tree2);

  // Cleanup
  free(label_tree1);
  free(label_tree2);
  return ret;
}


int tree_cmp(
    const Tree * const tree1,
    const Tree * const tree2,
    enum tree_compare_types compare_type)
{
  if (tree1 == NULL) return (tree2==NULL)?0:1;
  if (tree2 == NULL) return -1;
  assert(tree1 != NULL && tree2 != NULL);

  switch (compare_type) {
    case TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT:
      return tree_cmp_same_node_memory_layout(tree1, tree2);
    case TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED:
      return tree_cmp_child_node_order_scrambled(tree1, tree2);
    case TREE_COMPARE_IF_NODES_ISOMORPH:
      return tree_cmp_if_nodes_isomorph(tree1, tree2);
    case TREE_COMPARE_SAME_DATA_MEMORY_LAYOUT:
      return tree_cmp_same_data_memory_layout(tree1, tree2);
    case TREE_COMPARE_CHILD_DATA_ORDER_SCRAMBLED:
      return tree_cmp_child_data_order_scrambled(tree1, tree2);
    case TREE_COMPARE_IF_DATA_ISOMORPH:
      return tree_cmp_if_data_isomorph(tree1, tree2);
    default:
      fprintf(stderr, "Unsupported compare type: ");
      fprintf_enum_name(stderr, compare_type);
      fprintf(stderr, "\n");
  } 

  assert(0);
  return 0;
}

