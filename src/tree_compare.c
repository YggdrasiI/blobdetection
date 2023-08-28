#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "blobdetection/enums.h"
#include "blobdetection/tree.h"

#include "tree_intern.h"

// Force gcc to inline looping over threes.
// This probably removes the 'if (function_handler == NULL)' checks.
//#define INLINE inline __attribute__((always_inline))
#include "tree_loops.h"

#define MAX(A,B) ((A>B)?(A):(B))

// Collection of comparator functions for Tree struct.

/* Compare function for _quicksort_pointer function.
 *
 * Returns 1, if a>b, and 0 otherwise.
 * Do not use (1, 0, -1) like strcmp for sorting stability.
 */
typedef int pointer_compare_func(const void **a, const void **b);

int _cmp_strings_by_value(const void **a, const void **b)
{
  if (strcmp(*a, *b) > 0) return 1;
  return 0;
}

int _cmp_strings_by_position(const void **a, const void **b)
{
  return (*a < *b?1:0);
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
    //if ( **ptr < **begin )
    if ( cmp(*ptr, *begin) ) 
    {
      _swap_pointers(ptr, split);
      ++split;
    }
  }
  _swap_pointers(begin, split - 1);
  _quicksort_pointer(begin, split - 1, cmp);
  _quicksort_pointer(split, end, cmp);
}

#if 0
/* Define type for compare functions.
 *
 * • Both nodes an be assumed as != NULL (Nodes already checked by outer loop.)
 * • Data holds struct with specific data needed for the different
 * compare types, e.g pointer to the root nodes.
 * */
typedef int node_compare_func(const Node *n1, const Node *n2, void *compare_data);

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


int child_compare_func1(const Node *n1, const Node *n2, void *compare_data) {
  const compare_func1_data * cmp = (const compare_func1_data *)compare_data;
  ptrdiff_t d1 = n1 - cmp->anchor1;
  ptrdiff_t d2 = n2 - cmp->anchor2;
  if (d2-d1) return d2-d1;

  return 0;
}

int parent_compare_func1(const Node *n1, const Node *n2, void *compare_data) {
  const compare_func1_data * cmp = (const compare_func1_data *)compare_data;
  ptrdiff_t d1 = n1->parent - cmp->anchor1;
  ptrdiff_t d2 = n2->parent - cmp->anchor2;
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

//int compare_func2(const Node *n1, const Node *n2, void *compare_data){}
#define child_compare_func2 child_compare_func1

int parent_compare_func2(const Node *n1, const Node *n2, void *compare_data) {
  compare_func2_data * cmp = (compare_func2_data *)compare_data;

  // This function re-sorts child on left-most child and just compare results
  // on all other nodes. (Maybe it's better to check all siblings directly in first call?!)
  if( n1->parent->child != n1 ) return child_compare_func2(n1->parent, n2->parent, compare_data);

  // Search bijection between nodes of n1->parent and n2->parent by sorting both sets
#ifdef TREE_REDUNDANT_INFOS
  const uint32_t num_nodes = n1->parent->width;
  const uint32_t num_nodes2 = n2->parent->width;
  if(num_nodes != num_nodes2) return num_nodes2-num_nodes;

  // Guarantee enough space.
  if (num_nodes > cmp->num_sorting_siblings) {
    free(cmp->sorting_siblings1);
    free(cmp->sorting_siblings2);
    cmp->num_sorting_siblings = MAX(num_nodes+8,
        2 * cmp->num_sorting_siblings);
    cmp->sorting_siblings1 = malloc(
        cmp->num_sorting_siblings*sizeof(Node *));
    cmp->sorting_siblings2 = malloc(
        cmp->num_sorting_siblings*sizeof(Node *));
    if (cmp->sorting_siblings1 == NULL || cmp->sorting_siblings2 == NULL ){
      cmp->num_sorting_siblings = 0;
      assert(0);
      return -4;
    }
  }

  // Fill arrays
  Node *cur1 = n1->parent->child;
  Node *cur2 = n2->parent->child;
  Node **array_cur1 = cmp->sorting_siblings1;
  Node **array_cur2 = cmp->sorting_siblings2;
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
    if (node_index >= cmp->num_sorting_siblings) {
      cmp->num_sorting_siblings = MAX(node_index+8,
          2 * cmp->num_sorting_siblings);
      cmp->sorting_siblings1 = _reallocarray_or_free(cmp->sorting_siblings1,
          cmp->num_sorting_siblings, sizeof(Node *));
      cmp->sorting_siblings2 = _reallocarray_or_free(cmp->sorting_siblings1,
          cmp->num_sorting_siblings, sizeof(Node *));
      if (cmp->sorting_siblings1 == NULL || cmp->sorting_siblings2 == NULL ){
        cmp->num_sorting_siblings = 0;
        assert(0);
        return -4;
      }
    }

    // Fill arrays
    cmp->sorting_siblings1[node_index] = cur1;
    cmp->sorting_siblings2[node_index] = cur2;

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
      (void **)cmp->sorting_siblings1,
      (void **)cmp->sorting_siblings1+num_nodes,
      _cmp_strings_by_position);

  // Check if canonical form is the same.
  uint32_t idx = num_nodes;
  while(idx--){
    Node *n1 = cmp->sorting_siblings1[idx];
    Node *n2 = cmp->sorting_siblings2[idx];
    int d = child_compare_func2(n1, n2, compare_data);
    if (d) return d;
  }

  // Update node order in both trees to canonical form.
  Node **pn1 = cmp->sorting_siblings1;
  Node **pn2 = cmp->sorting_siblings2;
  for(idx = 1 /* not 0 */; idx<num_nodes; ++idx){
    (*pn1)->sibling = *(pn1+1);
    (*pn2)->sibling = *(pn2+1);
    ++pn1; ++pn2;
  }
  (*pn1)->sibling = NULL;
  (*pn2)->sibling = NULL;

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
typedef struct {
    const Node *anchor;
    uint32_t depth;
    size_t len_of_label_caches;
    char *label_cache[2];
    char **label_refs;
    char *first_free_char_in_label_cache[2];

    uint32_t num_sorting_siblings;
    char **sorting_siblings;
} compare_func3_data;

void destroy_compare_func3_data(compare_func3_data *d, int keep_label0){
  if( !keep_label0 ){
    free(d->label_cache[0]);
  }
  free(d->label_cache[1]);
}

compare_func3_data create_compare_func3_data(const Tree *tree){

  const uint32_t number_of_labels = tree->size;
  const uint32_t len_of_label_caches = (2 * number_of_labels + 1) * sizeof(char);

  compare_func3_data data = {
    .anchor = tree->nodes /* first node of array needed here*/,
    .depth = 0,
    .len_of_label_caches = len_of_label_caches,
    .label_cache = {
      malloc(len_of_label_caches),
      malloc(len_of_label_caches)
    },
    .label_refs = calloc(number_of_labels, sizeof(char *)),
    .first_free_char_in_label_cache = {NULL, NULL},
    .num_sorting_siblings = 0,
    .sorting_siblings = NULL
  };
  // Check allocations for failure
  if (data.label_cache[0] == NULL || data.label_cache[0] == NULL
      || data.label_refs == NULL ){
    assert(0);
    destroy_compare_func3_data(&data, 0);
    compare_func3_data fail_data = {
      .anchor = NULL,
      .depth = 0,
      .len_of_label_caches = 0,
      .label_cache = { NULL, NULL },
      .label_refs = NULL,
      .first_free_char_in_label_cache = {NULL, NULL},
      .num_sorting_siblings = 0,
      .sorting_siblings = NULL
    };
    return fail_data;
  }
  data.label_cache[0][data.len_of_label_caches] = '\0';
  data.label_cache[1][data.len_of_label_caches] = '\0';
  data.first_free_char_in_label_cache[0] = data.label_cache[0];
  data.first_free_char_in_label_cache[1] = data.label_cache[1];

  return data;
}

int func3_on_leaf(const Node *n, void *_data) {
  // Init labels on leafs
  compare_func3_data * data = (compare_func3_data *)_data;

  //Flipping in and out avoids nesting with data->depth++ ;data->depth--;
  const uint32_t idx_in = data->depth % 2;
  const uint32_t idx_out = 1-idx_in;

  // Eval index in label arrays
  const ptrdiff_t d = n - data->anchor;
  assert( d>=0 && d<data->len_of_label_caches/2); //=> n pointer ok
  assert( data->first_free_char_in_label_cache[idx_out] + 3 < data->label_cache[idx_out] + data->len_of_label_caches);

  // Save start of label for this node
  data->label_refs[d] = data->first_free_char_in_label_cache[idx_out];
  // insert "()"
  *(data->first_free_char_in_label_cache[idx_out])++ = '(';
  *(data->first_free_char_in_label_cache[idx_out])++ = ')';
  *(data->first_free_char_in_label_cache[idx_out])++ = '\0';

  return 0;
}
int func3_on_nonleaf_preorder(const Node *n, void *_data) {
  // n = First child node.
  compare_func3_data * data = (compare_func3_data *)_data;
  data->depth++;

  return 0;
}

int func3_on_nonleaf_postorder(const Node *n, void *_data) {
  // n = Last child node.
  compare_func3_data * data = (compare_func3_data *)_data;

  const uint32_t idx_out = data->depth % 2;
  const uint32_t idx_in = 1-idx_out;

  // Put label refs into array
#ifdef TREE_REDUNDANT_INFOS
  const uint32_t num_nodes = n->parent->width;
  // Guarantee enough space.
  if (num_nodes > data->num_sorting_siblings) {
    free(data->sorting_siblings);
    data->num_sorting_siblings = MAX(num_nodes+8,
        2 * data->num_sorting_siblings);
    data->sorting_siblings = malloc(
        data->num_sorting_siblings*sizeof(char *));
    if (data->sorting_siblings == NULL) {
      data->num_sorting_siblings = 0;
      assert(0);
      return -4;
    }
  }

  // Fill array
  Node *cur = n->parent->child;
  char **array_cur = data->sorting_siblings;
  while(cur){
    const ptrdiff_t d = cur - data->anchor;
    *array_cur = data->label_refs[d];
    cur = cur->sibling;
    ++array_cur;
  }
#else
  // Combined step to count array size and filling.
  Node *cur = n->parent->child;
  uint32_t node_index = 0;
  while(cur){
    // Realloc destination array if too small
    if (node_index >= data->num_sorting_siblings) {
      data->num_sorting_siblings = MAX(node_index+8,
          2 * data->num_sorting_siblings);
      data->sorting_siblings = _reallocarray_or_free(data->sorting_siblings,
          data->num_sorting_siblings, sizeof(char *));
      if (data->sorting_siblings == NULL){
        data->num_sorting_siblings = 0;
        assert(0);
        return -4;
      }
    }

    // Fill array
    const ptrdiff_t d = cur - data->anchor;
    data->sorting_siblings[node_index] = data->label_refs[d];

    cur = cur->sibling;
    ++node_index;
  }
  uint32_t num_nodes = node_index;
#endif

  // Backup pointer to leftmost used label before sorting. All labels fullfill the 
  // condition &label_child_1 < ... &label_child_K.
  assert(num_nodes > 0);
  char * const leftmost_pointer_input_labels = data->sorting_siblings[0];

  // Sorting the label refs
  _quicksort_pointer(
      (void **)data->sorting_siblings,
      (void **)data->sorting_siblings+num_nodes,
      _cmp_strings_by_value);

  // Merging labels into label of parent node
  char **plabel = data->sorting_siblings;
  char ** const plabelEnd = plabel + num_nodes;
  char *cur_pos = data->first_free_char_in_label_cache[idx_out];
  char * const leftmost_pointer_output_labels = cur_pos;
  if(cur_pos == NULL){
    assert(0);
    return -5;
  }
  while (plabel < plabelEnd) {
    assert(cur_pos > data->label_cache[idx_out]);
    assert(cur_pos + strlen(*plabel) 
        < data->label_cache[idx_out] + data->len_of_label_caches);
    cur_pos = strcpy(cur_pos, *plabel);
    ++plabel;
  }

  // Lock memory in idx_out-cache
  data->first_free_char_in_label_cache[idx_out] = cur_pos;
  // Unlock memory in idx_in-cache
  data->first_free_char_in_label_cache[idx_in] = leftmost_pointer_input_labels;

  // Link to new label in parent node
  const ptrdiff_t dParent = cur->parent - data->anchor;
  data->label_refs[dParent] = leftmost_pointer_output_labels;

  // We're done :-)
  data->depth--;
  return 0;
}


/* Variant 4: TREE_COMPARE_SAME_DATA_MEMORY_LAYOUT
 *
 * Check if nodes of both trees have same distance to each root node.
 * and made the same check for data handlers, if both given.
 *
 * */
typedef struct {
    const Node *anchor1;
    const Node *anchor2;
    const void *tree1_data;
    const void *tree2_data;
} compare_func4_data;


int compare_func4(const Node *n1, const Node *n2, void *compare_data) {
  const compare_func4_data * cmp = (const compare_func4_data *)compare_data;
  if (n1->data){
    if (n2->data == NULL) return -1;
    ptrdiff_t d1 = n1->data - cmp->tree1_data;
    ptrdiff_t d2 = n2->data - cmp->tree2_data;
    return d2-d1;
  }
  if (n2->data){
    if (n1->data == NULL) return 1;
    ptrdiff_t d1 = n1->data - cmp->tree1_data;
    ptrdiff_t d2 = n2->data - cmp->tree2_data;
    return d2-d1;
  }
  return 0;
}

/* Variant 5: TREE_COMPARE_CHILD_DATA_ORDER_SCRAMBLED
 *
 * LIKE TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED but compare data pointer
 * instead of node's own position.
 *
 * Attention: It just checks the pointer position, not the content.
 * */
typedef struct {
    const Node *anchor1;
    const Node *anchor2;
    const void *tree1_data;
    const void *tree2_data;
} compare_func5_data;


int compare_func5(const Node *n1, const Node *n2, void *compare_data) {
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


int compare_func6(const Node *n1, const Node *n2, void *compare_data) {
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

int compare_func7(const Node *n1, const Node *n2, void *compare_data) {
  //const compare_func7_data * cmp = (const compare_func7_data *)compare_data;

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

int compare_func8(const Node *n1, const Node *n2, void *compare_data) {
  //const compare_func8_data * cmp = (const compare_func8_data *)compare_data;

  return 0;
}

// End variants
// ==============================================

#if 0
int _tree_cmp(
        Tree * const tree1,
        Tree * const tree2,
        node_compare_func *cmp_on_enter_node,
        node_compare_func *cmp_on_leaf,
        node_compare_func *cmp_on_leave_last_child,
        void *data)
{

    if (tree1 == NULL) return (tree2==NULL)?0:1;
    if (tree2 == NULL) return -1;
    assert(tree1 != NULL && tree2 != NULL);

    // This would be wrong because the size
    // is just the number of allocated space. 
    // Trees can just be the same for different sizes.
    //int32_t d = tree2->size - tree1->size;
    //if (d) return d;

    const Node * const root1 = tree1->root;
    const Node * const root2 = tree2->root;
    const Node *cur1 = root1->child;
    const Node *cur2 = root2->child;
    int32_t d;
    // Loop through tree1 and check if tree2 condition matches.
    while(cur1) {
      if (cur2==NULL) // tree1 has more nodes
          return -1;

      // Check of nodes on entering
      if (cmp_on_enter_node != NULL) {
        d = cmp_on_enter_node(cur1, cur2, data);
        if(d) return d;
      }

      if (cur1->child) {
          cur1 = cur1->child;
          cur2 = cur2->child;
          continue;
      }

      // Check/Initialization of leafs
      if (cmp_on_leaf != NULL) {
        d = cmp_on_leaf(cur1, cur2, data);
        if(d) return d;
      }

      if (cur1->sibling) {
          cur1 = cur1->sibling;
          cur2 = cur2->sibling;
          continue;
      }
      while(cur1!=root1){

        // Check siblings on leaving of right most child
        if (cmp_on_leave_last_child != NULL ) {
          d = cmp_on_leave_last_child(cur1, cur2, data);
          if(d) return d;
        }

          cur1=cur1->parent;
          cur2=cur2->parent;

          if(cur1==NULL){ // tree1 is inconsistent if we reach NULL before root1
              assert(cur1!=NULL);
              return INT_MAX;
          }
          if(cur2==NULL){ // tree2 ending too early
              return INT_MIN;
          }

          if (cur1->sibling){
              cur1=cur1->sibling;
              cur2=cur2->sibling;
              break;
          }
      }
    }

    if (cur2 != NULL) // => Extra node
      return 1;

    return 0;
}
#endif


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
#if 1
int tree_cmp_same_memory_layout(
        const Tree * const tree1,
        const Tree * const tree2)
{
  assert(tree1 != NULL && tree2 != NULL);

  // Non-const pointers, but underlaying handlers will not alter trees.
  Tree *_tree1 = (Tree *)tree1;
  Tree *_tree2 = (Tree *)tree2;

  compare_func1_data data = {_tree1->nodes, _tree2->nodes};
  return _trees_depth_first_search(_tree1, _tree2,
      child_compare_func1, parent_compare_func1,
      NULL, NULL, &data);
}

#else // Old version without function handlers
int tree_cmp_same_memory_layout(
        const Tree * const tree1,
        const Tree * const tree2)
{
    assert(tree1 != NULL && tree2 != NULL);

    // This would be wrong because the size
    // is just the number of allocated space. 
    // Trees can just be the same for different sizes.
    //int32_t d = tree2->size - tree1->size;
    //if (d) return d;

    const Node * const root1 = tree1->root;
    const Node * const root2 = tree2->root;
    const Node *cur1 = root1->child;
    const Node *cur2 = root2->child;
    int32_t d1, d2;
    // Loop through tree1 and check if tree2 condition matches.
    while(cur1) {
        if (cur2==NULL) // tree1 has more nodes
            return -1;

        d1 = cur1-root1;
        d2 = cur2-root2;
        if(d1 != d2) return d2-d1;


        if (cur1->child) {
            cur1 = cur1->child;
            cur2 = cur2->child;
            continue;
        }
        if (cur1->sibling) {
            cur1 = cur1->sibling;
            cur2 = cur2->sibling;
            continue;
        }
        while(cur1!=root1){
            cur1=cur1->parent;
            cur2=cur2->parent;

            if(cur1==NULL){ // tree1 is inconsistent if we reach NULL before root1
                assert(cur1!=NULL);
                return INT_MAX;
            }
            if(cur2==NULL){ // tree2 ending too early
                return INT_MIN;
            }

            if (cur1->sibling){
                cur1=cur1->sibling;
                cur2=cur2->sibling;
                break;
            }
        }
    }

    if (cur2 != NULL) // => Extra node
        return 1;

    return 0;
}
#endif

int tree_cmp_child_node_order_scrambled(
        const Tree * const tree1,
        const Tree * const tree2)
{
  assert(tree1 != NULL && tree2 != NULL);

  // Create copies because we will re-sort childs during the operation.
  // (It would be possible to shrink this on one clone.)
  Tree *_tree1 = tree_clone(tree1, NULL, NULL);
  Tree *_tree2 = tree_clone(tree2, NULL, NULL);

  compare_func2_data data = {_tree1->nodes, _tree2->nodes, 0, NULL, NULL};
  int ret = _trees_depth_first_search(_tree1, _tree2,
      child_compare_func2, parent_compare_func2,
      NULL, NULL, &data);

  // Cleanup
  destroy_compare_func2_data(&data);
  tree_destroy(&_tree1);
  tree_destroy(&_tree2);

  return ret;
}

/* Delete returned char after usage! */
char *_gen_canonical_label(const Tree *tree){

  compare_func3_data data = create_compare_func3_data(tree);
  if (data.anchor == NULL){ // Check allocations for failure
    assert(0);
    destroy_compare_func3_data(&data, 0);
    return NULL;
  }
  data.label_cache[0][data.len_of_label_caches] = '\0';
  data.label_cache[1][data.len_of_label_caches] = '\0';
  data.first_free_char_in_label_cache[0] = data.label_cache[0];
  data.first_free_char_in_label_cache[1] = data.label_cache[1];
  
  int ret = _tree_depth_first_search(
      (Tree *)tree /* Discarding const is ok for this function handlers */,
      func3_on_leaf, func3_on_nonleaf_preorder,
      NULL, func3_on_nonleaf_postorder,
      &data);

  if (ret == 0){
    // Cleanup, but hold result
    destroy_compare_func3_data(&data, 1);
    return data.label_cache[0]; // TODO: Unklar ob letztes Label ganz vorne. Mindestens nen Assert einbauen.
  }else{
    // Cleanup all
    destroy_compare_func3_data(&data, 0);
    return NULL;
  }
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
   * We do not clone the input trees and for thes reason we can not use
   * the data pointer without overwriting user data.
   * Instead, we're using 'nodeX - treeX->root' as index into an new data array.
   * the ->data pointer.
   * (Cloning the input trees looks more practival, but we want use the 
   * same structure as the TREE_COMPARE_IF_DATA_ISOMORPH algorithm and there,
   * we need the input data pointer.
   */
  /*
   * Two Terminals '(',')' for each node will be used, to 2*num_nodes(t)+1
   * can store the whole string and num_nodes(t) <= t->size.
   */

  char *label_tree1 = _gen_canonical_label(tree1);
  if (label_tree1 == NULL) return -1;
  char *label_tree2 = _gen_canonical_label(tree2);
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
      return tree_cmp_same_memory_layout(tree1, tree2);
    case TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED:
      return tree_cmp_child_node_order_scrambled(tree1, tree2);
    default:
      fprintf(stderr, "Unsupported compare type: ");
      fprintf_enum_name(stderr, compare_type);
      fprintf(stderr, "\n");
  } 

  assert(0);
  return 0;
}

