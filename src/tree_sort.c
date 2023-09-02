#define _GNU_SOURCE // for mempcpy
#include <string.h>
#include <assert.h>

//#include "blobdetection/tree_sort.h"

#include "tree_intern.h"

// Force gcc to inline looping over trees.
// This probably removes the 'if (function_handler == NULL)' checks.
#define INLINE inline __attribute__((always_inline))
#include "tree_loops.h"

#include "quicksort.h"

#define POINTER_LEN sizeof(void *)
#define ADD_NULL_TERMINAL_TO_LABEL

typedef struct {
    char *label_ref;
    Node *node_ref;
} label_node_ref1_t;

typedef struct {
    const Node *anchor;
    uint32_t depth;
    size_t len_of_label_caches;
    char *label_cache[2];
    label_node_ref1_t *refs; // node-id -> pLabel map, where *pLabel is in label_cache
    char *first_free_char_in_label_cache[2];

    uint32_t num_sorting_siblings;
    label_node_ref1_t **sorting_siblings; // pointer into refs
    //Tree *tree; // for debugging
} sort_func1_data_t;

/* Compare function for _quicksort_pointer function.
 *
 * Returns 1, if a>b, and 0 otherwise.
 * Do not use (1, 0, -1) like strcmp for sorting stability.
 */
int _cmp_ref_by_label(void **a, void **b)
{
  label_node_ref1_t *refA = (label_node_ref1_t *)(*a);
  label_node_ref1_t *refB = (label_node_ref1_t *)(*b);
  if (strcmp(refA->label_ref, refB->label_ref) < 0) return 1;
  return 0;
}

// compare function for QUICKSORT_TEMPLATE
int _cmp_for_sort1(label_node_ref1_t *a, label_node_ref1_t *b)
{
  if (strcmp(a->label_ref, b->label_ref) < 0) return 1;
  return 0;
}
// Define _quicksort_for_sort1
QUICKSORT_TEMPLATE(for_sort1, label_node_ref1_t);

void destroy_sort_func1_data(sort_func1_data_t *d, int keep_label0){
  if( !keep_label0 ){
    free(d->label_cache[0]);
  }
  free(d->label_cache[1]);
  free(d->refs);
  free(d->sorting_siblings);
}

sort_func1_data_t create_sort_func1_data(const Tree *tree){

  const uint32_t number_of_labels = tree->size;
  const uint32_t len_of_label_caches = (2 * number_of_labels + 1) * sizeof(char);

  sort_func1_data_t data = {
    .anchor = tree->nodes /* first node of array needed here*/,
    .depth = 0,
    .len_of_label_caches = len_of_label_caches,
    .label_cache = {
      malloc(len_of_label_caches),
      malloc(len_of_label_caches)
    },
    // Each reference is a (label,node) pair
    .refs = calloc(number_of_labels, sizeof(label_node_ref1_t)),
    .first_free_char_in_label_cache = {NULL, NULL},
    .num_sorting_siblings = 0,
    .sorting_siblings = NULL,
    //.tree = (Tree *)tree,
  };
  // Check allocations for failure
  if (data.label_cache[0] == NULL || data.label_cache[0] == NULL
      || data.refs == NULL ){
    assert(0);
    destroy_sort_func1_data(&data, 0);
    sort_func1_data_t fail_data = {
      .anchor = NULL,
      .depth = 0,
      .len_of_label_caches = 0,
      .label_cache = { NULL, NULL },
      .refs = NULL,
      .first_free_char_in_label_cache = {NULL, NULL},
      .num_sorting_siblings = 0,
      .sorting_siblings = NULL,
      //.tree = NULL,
    };
    return fail_data;
  }
  data.label_cache[0][data.len_of_label_caches] = '\0';
  data.label_cache[1][data.len_of_label_caches] = '\0';
  data.first_free_char_in_label_cache[0] = data.label_cache[0];
  data.first_free_char_in_label_cache[1] = data.label_cache[1];

  return data;
}

int sort_func1_on_leaf(Node *n, void *_data) {
  // Init labels on leafs
  sort_func1_data_t * data = (sort_func1_data_t *)_data;

  //Flipping in and out avoids nesting with data->depth++/data->depth--
  //at begin and end of this function
  const uint32_t idx_out = data->depth % 2;
  //const uint32_t idx_in = 1-idx_out;

  // Eval index in label arrays
  const ptrdiff_t d = n - data->anchor;
  assert( d>=0 && d<=data->len_of_label_caches/2); //=> n pointer ok
  assert( data->first_free_char_in_label_cache[idx_out] + 2 < data->label_cache[idx_out] + data->len_of_label_caches);

  // Save start of label for this node
  data->refs[d].label_ref = data->first_free_char_in_label_cache[idx_out];
  data->refs[d].node_ref = n;
  // insert LEAF_CHAR
  *(data->first_free_char_in_label_cache[idx_out])++ = LEAF_CHAR;
  *(data->first_free_char_in_label_cache[idx_out])++ = '\0';

  return 0;
}
int sort_func1_on_nonleaf_preorder(Node *n, void *_data) {
  // n = First child node.
  sort_func1_data_t * data = (sort_func1_data_t *)_data;
  data->depth++;

  return 0;
}

int sort_func1_on_nonleaf_postorder(Node *n, void *_data) {
  // n = Last child node.
  sort_func1_data_t * data = (sort_func1_data_t *)_data;

  const uint32_t idx_in = data->depth % 2;
  const uint32_t idx_out = 1-idx_in;

  // Put label refs into array
#ifdef TREE_REDUNDANT_INFOS
  const uint32_t num_nodes = n->parent->width;
  // Guarantee enough space.
  if (num_nodes > data->num_sorting_siblings) {
    free(data->sorting_siblings);
    data->num_sorting_siblings = MAX(num_nodes+8,
        2 * data->num_sorting_siblings);
    data->sorting_siblings = calloc(
        data->num_sorting_siblings, sizeof(label_node_ref1_t*));
    if (data->sorting_siblings == NULL) {
      data->num_sorting_siblings = 0;
      assert(0);
      return -4;
    }
  }

  // Fill array
  Node *cur = n->parent->child;
  label_node_ref1_t **array_cur = data->sorting_siblings;
  while(cur){
    const ptrdiff_t d = cur - data->anchor;
    // Link ref-array entry in sorting array. The values of refs[d]
    // was set in leaf-handler or previous call of this handler.
    *array_cur = &data->refs[d];

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
          data->num_sorting_siblings, sizeof(label_node_ref1_t*));
      if (data->sorting_siblings == NULL){
        data->num_sorting_siblings = 0;
        assert(0);
        return -4;
      }
    }

    // Fill array
    const ptrdiff_t d = cur - data->anchor;
    data->sorting_siblings[node_index] = &data->refs[d];

    cur = cur->sibling;
    ++node_index;
  }
  uint32_t num_nodes = node_index;
#endif

  // Backup pointer to leftmost used label before sorting. All labels fullfill the
  // condition &label_child_1 < ... &label_child_K.
  assert(num_nodes > 0);
  label_node_ref1_t * const leftmost_pointer_input_labels = data->sorting_siblings[0];

  // Sorting the label refs
  _quicksort_for_sort1(
          data->sorting_siblings,
          data->sorting_siblings+num_nodes,
          _cmp_for_sort1);

  // Merging labels into label of parent node
  // and re-sort children nodes
  label_node_ref1_t **pRef = data->sorting_siblings;
  label_node_ref1_t ** const pRefEnd = pRef + num_nodes;
  char *cur_pos = data->first_free_char_in_label_cache[idx_out];
  char * const leftmost_pointer_output_labels = cur_pos;
  if(cur_pos == NULL){
    assert(0);
    return -5;
  }

  assert( (*pRef)->node_ref->parent->child->parent == (*pRef)->node_ref->parent);
  (*pRef)->node_ref->parent->child = (*pRef)->node_ref; // Leftmost child
  *cur_pos++ = LEFT_NON_LEAF_CHAR;

  while (pRef < pRefEnd-1) {
    assert( (*(pRef+1))->node_ref->parent == (*pRef)->node_ref->parent);
    assert(cur_pos >= data->label_cache[idx_out]);
    assert(cur_pos + strlen((*pRef)->label_ref)
        < data->label_cache[idx_out] + data->len_of_label_caches);

    (*pRef)->node_ref->sibling = (*(pRef+1))->node_ref; // not (*pRef)+1…
    cur_pos = stpcpy(cur_pos, (*pRef)->label_ref);
    ++pRef;
  }
  if (pRef < pRefEnd) { // Last node
    (*pRef)->node_ref->sibling = NULL; // Rightmost child

    assert(cur_pos >= data->label_cache[idx_out]);
    assert(cur_pos + strlen((*pRef)->label_ref)
        < data->label_cache[idx_out] + data->len_of_label_caches);
    cur_pos = stpcpy(cur_pos, (*pRef)->label_ref);
    ++pRef;
  }

  *cur_pos++ = RIGHT_NON_LEAF_CHAR;
  *cur_pos = '\0';
  //printf("Label: %s\n", data->first_free_char_in_label_cache[idx_out]);

  // Lock memory in idx_out-cache
  data->first_free_char_in_label_cache[idx_out] = cur_pos+1; /* One after last '\0' */
  // Unlock memory in idx_in-cache
  data->first_free_char_in_label_cache[idx_in] = leftmost_pointer_input_labels->label_ref;

  // Link to new label in parent node
  const ptrdiff_t dParent = n->parent - data->anchor;
  data->refs[dParent].label_ref = leftmost_pointer_output_labels;
  data->refs[dParent].node_ref = n->parent;

  // We're done :-)
  data->depth--;
  return 0;
}
//================================================

// Variant which just generates labels, but does not change tree.
typedef struct {
  const Node *anchor;
  uint32_t depth;
  size_t len_of_label_caches;
  char *label_cache[2];
  char **label_refs;
  char *first_free_char_in_label_cache[2];

  uint32_t num_sorting_siblings;
  char **sorting_siblings;
  //Tree *tree; // for debugging
} sort_func2_data;

void destroy_sort_func2_data(sort_func2_data *d, int keep_label0){
  if( !keep_label0 ){
    free(d->label_cache[0]);
  }
  free(d->label_cache[1]);
  free(d->label_refs);
  free(d->sorting_siblings);
}

sort_func2_data create_sort_func2_data(const Tree *tree){

  const uint32_t number_of_labels = tree->size;
  const uint32_t len_of_label_caches = (2 * number_of_labels + 1) * sizeof(char);

  sort_func2_data data = {
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
    .sorting_siblings = NULL,
    //.tree = (Tree *)tree,
  };
  // Check allocations for failure
  if (data.label_cache[0] == NULL || data.label_cache[0] == NULL
      || data.label_refs == NULL ){
    assert(0);
    destroy_sort_func2_data(&data, 0);
    sort_func2_data fail_data = {
      .anchor = NULL,
      .depth = 0,
      .len_of_label_caches = 0,
      .label_cache = { NULL, NULL },
      .label_refs = NULL,
      .first_free_char_in_label_cache = {NULL, NULL},
      .num_sorting_siblings = 0,
      .sorting_siblings = NULL,
      //.tree = NULL,
    };
    return fail_data;
  }
  data.label_cache[0][data.len_of_label_caches] = '\0';
  data.label_cache[1][data.len_of_label_caches] = '\0';
  data.first_free_char_in_label_cache[0] = data.label_cache[0];
  data.first_free_char_in_label_cache[1] = data.label_cache[1];

  return data;
}

int sort_func2_on_leaf(Node *n, void *_data) {
  // Init labels on leafs
  sort_func2_data * data = (sort_func2_data *)_data;

  //Flipping in and out avoids nesting with data->depth++/data->depth--
  //at begin and end of this function
  const uint32_t idx_out = data->depth % 2;
  //const uint32_t idx_in = 1-idx_out;

  // Eval index in label arrays
  const ptrdiff_t d = n - data->anchor;
  assert( d>=0 && d<=data->len_of_label_caches/2); //=> n pointer ok
  assert( data->first_free_char_in_label_cache[idx_out] + 2 < data->label_cache[idx_out] + data->len_of_label_caches);

  // Save start of label for this node
  data->label_refs[d] = data->first_free_char_in_label_cache[idx_out];
  // insert "|"
  *(data->first_free_char_in_label_cache[idx_out])++ = LEAF_CHAR;
  *(data->first_free_char_in_label_cache[idx_out])++ = '\0';

  return 0;
}
int sort_func2_on_nonleaf_preorder(Node *n, void *_data) {
  // n = First child node.
  sort_func2_data * data = (sort_func2_data *)_data;
  data->depth++;

  return 0;
}

int sort_func2_on_nonleaf_postorder(Node *n, void *_data) {
  // n = Last child node.
  sort_func2_data * data = (sort_func2_data *)_data;

  const uint32_t idx_in = data->depth % 2;
  const uint32_t idx_out = 1-idx_in;

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
  *cur_pos++ = LEFT_NON_LEAF_CHAR;
  while (plabel < plabelEnd) {
    assert(cur_pos >= data->label_cache[idx_out]);
    assert(cur_pos + strlen(*plabel)
        < data->label_cache[idx_out] + data->len_of_label_caches);
    cur_pos = stpcpy(cur_pos, *plabel);
    ++plabel;
  }
  *cur_pos++ = RIGHT_NON_LEAF_CHAR;
  *cur_pos = '\0';
  //printf("Label: %s\n", data->first_free_char_in_label_cache[idx_out]);

  // Lock memory in idx_out-cache
  data->first_free_char_in_label_cache[idx_out] = cur_pos+1; /* One after last '\0' */
  // Unlock memory in idx_in-cache
  data->first_free_char_in_label_cache[idx_in] = leftmost_pointer_input_labels;

  // Link to new label in parent node
  const ptrdiff_t dParent = n->parent - data->anchor;
  data->label_refs[dParent] = leftmost_pointer_output_labels;

  // We're done :-)
  data->depth--;
  return 0;
}


//================================================
// Variant for TREE_COMPARE_IF_DATA_ISOMORPH. Here, the labels
// are arrays, containing '\0' which will be compared by memcmp instead
// of strcmp.
// This is required because the labels will containing pointers/numbers.
//
typedef struct {
    void *label_ref;
    uint32_t label_len;
    Node *node_ref;
} label_node_ref3_t;

typedef struct {
    const Node *anchor;
    uint32_t depth;
    size_t len_of_label_caches;
    char *label_cache[2];
    label_node_ref3_t *refs; // node-id -> pLabel map, where *pLabel is in label_cache
    char *first_free_char_in_label_cache[2];

    uint32_t num_sorting_siblings;
    label_node_ref3_t **sorting_siblings; // pointer into refs
    //Tree *tree; // for debugging
} sort_func3_data;


/* Compare function for _quicksort_pointer function.
 *
 * Returns 1, if a>b, and 0 otherwise.
 * Do not use (1, 0, -1) like strcmp for sorting stability.
 */
int _cmp_ref_by_array(void **a, void **b)
{
  label_node_ref3_t *refA = (label_node_ref3_t *)(*a);
  label_node_ref3_t *refB = (label_node_ref3_t *)(*b);
  int dLen = refA->label_len - refB->label_len;
  if (dLen) return 1;
  if (memcmp(refA->label_ref, refB->label_ref, refA->label_len) < 0) return 1;
  return 0;
}
// compare function for QUICKSORT_TEMPLATE
int _cmp_for_sort3(label_node_ref3_t *a, label_node_ref3_t *b)
{
  int dLen = a->label_len - b->label_len;
  if (dLen) return 1;
  if (memcmp(a->label_ref, b->label_ref, a->label_len) < 0) return 1;
  return 0;
}

// Define _quicksort_for_sort3
QUICKSORT_TEMPLATE(for_sort3, label_node_ref3_t);

void destroy_sort_func3_data(sort_func3_data *d, int keep_label0){
  if( !keep_label0 ){
    free(d->label_cache[0]);
  }
  free(d->label_cache[1]);
  free(d->refs);
  free(d->sorting_siblings);
}

sort_func3_data create_sort_func3_data(const Tree *tree){

  const uint32_t number_of_labels = tree->size;
  const uint32_t len_of_label_caches = ((2+POINTER_LEN) * number_of_labels + 1) * sizeof(char);
  // Leaf-Label: node->data
  // NonLeaf-Label: '('node->data[…]')'

  sort_func3_data data = {
    .anchor = tree->nodes /* first node of array needed here*/,
    .depth = 0,
    .len_of_label_caches = len_of_label_caches,
    .label_cache = {
      malloc(len_of_label_caches),
      malloc(len_of_label_caches)
    },
    // Each reference is a (label,node) pair
    .refs = calloc(number_of_labels, sizeof(label_node_ref3_t)),
    .first_free_char_in_label_cache = {NULL, NULL},
    .num_sorting_siblings = 0,
    .sorting_siblings = NULL,
    //.tree = (Tree *)tree,
  };
  // Check allocations for failure
  if (data.label_cache[0] == NULL || data.label_cache[0] == NULL
      || data.refs == NULL ){
    assert(0);
    destroy_sort_func3_data(&data, 0);
    sort_func3_data fail_data = {
      .anchor = NULL,
      .depth = 0,
      .len_of_label_caches = 0,
      .label_cache = { NULL, NULL },
      .refs = NULL,
      .first_free_char_in_label_cache = {NULL, NULL},
      .num_sorting_siblings = 0,
      .sorting_siblings = NULL,
      //.tree = NULL,
    };
    return fail_data;
  }
  data.label_cache[0][data.len_of_label_caches] = '\0';
  data.label_cache[1][data.len_of_label_caches] = '\0';
  data.first_free_char_in_label_cache[0] = data.label_cache[0];
  data.first_free_char_in_label_cache[1] = data.label_cache[1];

  return data;
}

int sort_func3_on_leaf(Node *n, void *_data) {
  // Init labels on leafs
  sort_func3_data * data = (sort_func3_data *)_data;

  //Flipping in and out avoids nesting with data->depth++/data->depth--
  //at begin and end of this function
  const uint32_t idx_out = data->depth % 2;
  //const uint32_t idx_in = 1-idx_out;

  // Eval index in label arrays
  const ptrdiff_t d = n - data->anchor;
  assert( d>=0 && d<=data->len_of_label_caches/(2+POINTER_LEN)); //=> n pointer ok
  assert( data->first_free_char_in_label_cache[idx_out] + 2 < data->label_cache[idx_out] + data->len_of_label_caches);

  // Save start of label for this node
  data->refs[d].label_ref = data->first_free_char_in_label_cache[idx_out];
  data->refs[d].node_ref = n;
  // insert LEAF_CHAR
  //*(data->first_free_char_in_label_cache[idx_out])++ = LEAF_CHAR;
  //*(data->first_free_char_in_label_cache[idx_out])++ = '\0';
  // insert pointer (without \0)
  *((void **)data->first_free_char_in_label_cache[idx_out]) = n->data;
  *(data->first_free_char_in_label_cache[idx_out]) += POINTER_LEN;
  return 0;
}
int sort_func3_on_nonleaf_preorder(Node *n, void *_data) {
  // n = First child node.
  sort_func3_data * data = (sort_func3_data *)_data;
  data->depth++;

  return 0;
}

int sort_func3_on_nonleaf_postorder(Node *n, void *_data) {
  // n = Last child node.
  sort_func3_data * data = (sort_func3_data *)_data;

  const uint32_t idx_in = data->depth % 2;
  const uint32_t idx_out = 1-idx_in;

  // Put label refs into array
  //uint32_t sum_child_label_len = 0;
#ifdef TREE_REDUNDANT_INFOS
  const uint32_t num_nodes = n->parent->width;
  // Guarantee enough space.
  if (num_nodes > data->num_sorting_siblings) {
    free(data->sorting_siblings);
    data->num_sorting_siblings = MAX(num_nodes+8,
        2 * data->num_sorting_siblings);
    data->sorting_siblings = calloc(
        data->num_sorting_siblings, sizeof(label_node_ref3_t*));
    if (data->sorting_siblings == NULL) {
      data->num_sorting_siblings = 0;
      assert(0);
      return -4;
    }
  }

  // Fill array
  Node *cur = n->parent->child;
  label_node_ref3_t **array_cur = data->sorting_siblings;
  while(cur){
    const ptrdiff_t d = cur - data->anchor;
    // Link ref-array entry in sorting array. The values of refs[d]
    // was set in leaf-handler or previous call of this handler.
    *array_cur = &data->refs[d];

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
          data->num_sorting_siblings, sizeof(label_node_ref3_t*));
      if (data->sorting_siblings == NULL){
        data->num_sorting_siblings = 0;
        assert(0);
        return -4;
      }
    }

    // Fill array
    const ptrdiff_t d = cur - data->anchor;
    data->sorting_siblings[node_index] = &data->refs[d];

    //sum_child_label_len += data->refs[d].label_len;
    cur = cur->sibling;
    ++node_index;
  }
  uint32_t num_nodes = node_index;
#endif

  // Backup pointer to leftmost used label before sorting. All labels fullfill the
  // condition &label_child_1 < ... &label_child_K.
  assert(num_nodes > 0);
  label_node_ref3_t * const leftmost_pointer_input_labels = data->sorting_siblings[0];

  // Sorting the label refs
  _quicksort_for_sort3(
          data->sorting_siblings,
          data->sorting_siblings+num_nodes,
          _cmp_for_sort3);

  // Merging labels into label of parent node
  // and re-sort children nodes
  label_node_ref3_t **pRef = data->sorting_siblings;
  label_node_ref3_t ** const pRefEnd = pRef + num_nodes;
  char *cur_pos = data->first_free_char_in_label_cache[idx_out];
  char * const leftmost_pointer_output_labels = cur_pos;
  if(cur_pos == NULL){
    assert(0);
    return -5;
  }

  assert( (*pRef)->node_ref->parent->child->parent == (*pRef)->node_ref->parent);
  (*pRef)->node_ref->parent->child = (*pRef)->node_ref; // Leftmost child

  *cur_pos++ = LEFT_NON_LEAF_CHAR;
  *((void **)cur_pos) = n->data;
  cur_pos += POINTER_LEN;

  while (pRef < pRefEnd-1) {
    assert( (*(pRef+1))->node_ref->parent == (*pRef)->node_ref->parent);
    assert(cur_pos >= data->label_cache[idx_out]);
    assert(cur_pos + strlen((*pRef)->label_ref)
        < data->label_cache[idx_out] + data->len_of_label_caches);

    (*pRef)->node_ref->sibling = (*(pRef+1))->node_ref; // not (*pRef)+1…
    cur_pos = mempcpy(cur_pos, (*pRef)->label_ref, (*pRef)->label_len);
    //sum_child_label_len += (*pRef)->label_len;
    ++pRef;
  }
  if (pRef < pRefEnd) { // Last node
    (*pRef)->node_ref->sibling = NULL; // Rightmost child

    assert(cur_pos >= data->label_cache[idx_out]);
    assert(cur_pos + strlen((*pRef)->label_ref)
        < data->label_cache[idx_out] + data->len_of_label_caches);
    cur_pos = mempcpy(cur_pos, (*pRef)->label_ref, (*pRef)->label_len);
    //sum_child_label_len += (*pRef)->label_len;
    ++pRef;
  }

  *cur_pos = RIGHT_NON_LEAF_CHAR;
  // Eval number of consumed bytes
  uint32_t label_len = cur_pos - data->first_free_char_in_label_cache[idx_out];

  // Lock memory in idx_out-cache
  data->first_free_char_in_label_cache[idx_out] = cur_pos+1; /* One after last consumed byte */
  // Unlock memory in idx_in-cache
  data->first_free_char_in_label_cache[idx_in] = leftmost_pointer_input_labels->label_ref;

  // Link to new label in parent node
  const ptrdiff_t dParent = n->parent - data->anchor;
  data->refs[dParent].label_ref = leftmost_pointer_output_labels;
  //data->refs[dParent].label_len = sum_child_label_len + (2+POINTER_LEN);
  data->refs[dParent].label_len = label_len;
  data->refs[dParent].node_ref = n->parent;

  // We're done :-)
  data->depth--;
  return 0;
}

//================================================
// Unification of code for all variants...
// sort_nodes flag: Changing tree. Otherwise just the label will be generated.
// label_handler: Function which writes label and returned size of written bytes.

/* Return position after written bytes. */
typedef char * write_leaf_label_t(
    const Node *node,
    char *out, void *data /*, ssize_t avail_chars*/);
typedef char * write_nonleaf_label_t(
    const Node *node,
    int num_childs,
    char *out, void *data /*, ssize_t avail_chars*/);


#define POINTER_LEN sizeof(void *)
typedef struct {
    char *label_ref;
    uint32_t label_len;
    Node *node_ref;
} label_node_ref4_t;

typedef struct {
    const Node *anchor;
    uint32_t depth;
    size_t len_of_label_caches;
    char *label_cache[2];
    label_node_ref4_t *refs; // node-id -> pLabel map, where *pLabel is in label_cache
    char *first_free_char_in_label_cache[2];

    uint32_t num_sorting_siblings;
    label_node_ref4_t **sorting_siblings; // pointer into refs

    write_leaf_label_t *write_leaf_f;
    write_nonleaf_label_t *write_nonleaf_f;
    int sort_nodes;
    Tree *tree; // for debugging

} sort_func4_data_t;


// Label writer handlers
// Canonical label
char * write_leaf(
    const Node *node,
    char *out, void *data)
{
  *out++ = LEAF_CHAR;
#ifdef ADD_NULL_TERMINAL_TO_LABEL
  *out++ = '\0';
#else
  *out = '\0'; // Written, but pointer not incremeted.
#endif
  return out;
}

char * write_nonleaf(
    const Node *node,
    int num_childs,
    char *out, void *_data)

{
  sort_func4_data_t * data = (sort_func4_data_t *)_data;

  //const char *out_begin = out;
  *out++ = LEFT_NON_LEAF_CHAR;

  label_node_ref4_t **pRef = data->sorting_siblings;
  label_node_ref4_t ** const pRefEnd = pRef + num_childs;
  while (pRef < pRefEnd) {
    printf(" cL: %d '%s'", (*pRef)->label_len, (*pRef)->label_ref);
    out = mempcpy(out, (*pRef)->label_ref, (*pRef)->label_len
#ifdef ADD_NULL_TERMINAL_TO_LABEL
        -1
#endif
        );
    ++pRef;
  }
  printf("\n");
  *out++ = RIGHT_NON_LEAF_CHAR;
#ifdef ADD_NULL_TERMINAL_TO_LABEL
  *out++ = '\0'; // Written, but pointer not incremented
#else
  *out = '\0'; // Written, but pointer not incremented
#endif

  return out;
}

// label with data pointer
char * enrich_leaf_by_pointer(
    const Node *node,
    char *out, void *data)
{
  *((void **)out) = node->data;
  out += POINTER_LEN;
  *out = '\0'; // Add optional string end after memory block. (Not counted)
  return out; 
}

char * enrich_nonleaf_by_pointer(
    const Node *node,
    int num_childs,
    char *out, void *_data)
{
  sort_func4_data_t * data = (sort_func4_data_t *)_data;

  //const char *out_begin = out;
  *out++ = LEFT_NON_LEAF_CHAR;
  *((void **)out) = node->data;
  out += POINTER_LEN;

  label_node_ref4_t **pRef = data->sorting_siblings;
  label_node_ref4_t ** const pRefEnd = pRef + num_childs;
  while (pRef < pRefEnd) {
    out = mempcpy(out, (*pRef)->label_ref, (*pRef)->label_len);
    ++pRef;
  }
  *out++ = RIGHT_NON_LEAF_CHAR;
  *out = '\0'; // Add optional string end after memory block. (Not counted)

  return out; //-out_begin;
}

// label with hash value
// TODO: Maybe this should be salted by some fixed value to hide pointer?!
char * hash_leaf(
    const Node *node,
    char *out, void *data)
{
  *((void **)out) = node->data;
  out += POINTER_LEN;
  *out = '\0'; // Add optional string end after memory block. (Not counted)
  return out; 
}

char * hash_nonleaf(
    const Node *node,
    int num_childs,
    char *out, void *_data)
{
  //sort_func4_data_t * data = (sort_func4_data_t *)_data;

  char *out_begin_stage1 = out;

  // Stage 1: Write label like enrich_nonleaf_by_pointer()
  char *out_end_stage1 = enrich_nonleaf_by_pointer(node, num_childs, out_begin_stage1, _data);

  // Stage 2: Convert this new label into hash. 
#ifdef HASH_128BIT
   unsigned char hash[16];
   qhashmurmur3_128((void*)out_begin_stage1, out_end_stage1 - out_begin_stage1, hash);

  *out++ = LEFT_NON_LEAF_CHAR;
  out = mempcpy(out, hash, 16);
  *out++ = RIGHT_NON_LEAF_CHAR;
  *out = '\0'; // Add optional string end after memory block. (Not counted)
#else
   uint32_t hashval = qhashmurmur3_32((void*)out_begin_stage1, out_end_stage1 - out_begin_stage1);

  *out++ = LEFT_NON_LEAF_CHAR;

  *((uint32_t *)out) = hashval;
  out += POINTER_LEN;
  *out++ = RIGHT_NON_LEAF_CHAR;
  *out = '\0'; // Add optional string end after memory block. (Not counted)
#endif

  return out; //-out_begin;
}

// compare function for QUICKSORT_TEMPLATE
int _cmp_for_sort4(label_node_ref4_t *a, label_node_ref4_t *b)
{
  int dLen = b->label_len - a->label_len;
  //printf("Compare '%s' vs '%s' D:%d C:%d\n", a->label_ref, b->label_ref,
  //  dLen, memcmp(a->label_ref, b->label_ref, a->label_len)); 
  if (dLen < 0) return 1;
  if (memcmp(a->label_ref, b->label_ref, a->label_len) < 0) return 1;
  return 0;
}

// Define _quicksort_for_sort4
QUICKSORT_TEMPLATE(for_sort4, label_node_ref4_t);

void destroy_sort_func4_data(sort_func4_data_t *d, int keep_label0){
  if( !keep_label0 ){
    free(d->label_cache[0]);
  }
  free(d->label_cache[1]);
  free(d->refs);
  free(d->sorting_siblings);
}

int create_sort_func4_data(const Tree *tree,
    int sort_nodes,
    write_leaf_label_t* write_leaf,
    write_nonleaf_label_t *write_nonleaf,
    sort_func4_data_t *out)
{

  const uint32_t number_of_labels = tree->size;
  const uint32_t len_of_label_caches = ((2+POINTER_LEN) * number_of_labels + 1) * sizeof(char);
  // Leaf-Label: node->data
  // NonLeaf-Label: '('node->data[…]')'

  sort_func4_data_t data = {
    .anchor = tree->nodes /* first node of array needed here*/,
    .depth = 0,
    .len_of_label_caches = len_of_label_caches,
    .label_cache = {
      malloc(len_of_label_caches),
      malloc(len_of_label_caches)
    },
    // Each reference is a (label,node) pair
    .refs = calloc(number_of_labels, sizeof(label_node_ref4_t)),
    .first_free_char_in_label_cache = {NULL, NULL},
    .num_sorting_siblings = 0,
    .sorting_siblings = NULL,
    .sort_nodes = sort_nodes,
    .write_leaf_f = write_leaf,
    .write_nonleaf_f = write_nonleaf,
    .tree = (Tree *)tree,
  };
  // Check allocations for failure
  if (data.label_cache[0] == NULL || data.label_cache[0] == NULL
      || data.refs == NULL ){
    assert(0);
    destroy_sort_func4_data(&data, 0);
    sort_func4_data_t fail_data = {
      .anchor = NULL,
      .depth = 0,
      .len_of_label_caches = 0,
      .label_cache = { NULL, NULL },
      .refs = NULL,
      .first_free_char_in_label_cache = {NULL, NULL},
      .num_sorting_siblings = 0,
      .sorting_siblings = NULL,
      .sort_nodes = 0,
      .write_leaf_f = NULL,
      .write_nonleaf_f = NULL,
      .tree = NULL,
    };
    *out = fail_data;
    return -1;
  }

  data.label_cache[0][data.len_of_label_caches] = '\0';
  data.label_cache[1][data.len_of_label_caches] = '\0';
  data.first_free_char_in_label_cache[0] = data.label_cache[0];
  data.first_free_char_in_label_cache[1] = data.label_cache[1];

  *out = data;
  return 0;
}

int sort_func4_on_leaf(Node *n, void *_data) {
  // Init labels on leafs
  sort_func4_data_t * data = (sort_func4_data_t *)_data;

  //Flipping in and out avoids nesting with data->depth++/data->depth--
  //at begin and end of this function
  const uint32_t idx_out = data->depth % 2;
  //const uint32_t idx_in = 1-idx_out;

  // Eval index in label arrays
  const ptrdiff_t d = n - data->anchor;
  assert( d>=0 && d<=data->tree->size); //=> n pointer ok
  assert( data->first_free_char_in_label_cache[idx_out] + 2 < data->label_cache[idx_out] + data->len_of_label_caches);

  // Write label for leaf node
  char *node_label = data->first_free_char_in_label_cache[idx_out];
  char *char_after_node_label = data->write_leaf_f(n, node_label, data);
  printf("Leaflabel: %s\n", node_label);

  // Eval number of consumed bytes
  assert(char_after_node_label >= node_label);
  uint32_t label_len = char_after_node_label - node_label;

  // Lock memory in idx_out-cache
  data->first_free_char_in_label_cache[idx_out] = char_after_node_label;

  // Save start of label for this node
  data->refs[d].label_ref = node_label;
  data->refs[d].label_len = label_len;
  data->refs[d].node_ref = n;



  return 0;
}
int sort_func4_on_nonleaf_preorder(Node *n, void *_data) {
  // n = First child node.
  sort_func4_data_t * data = (sort_func4_data_t *)_data;
  data->depth++;

  return 0;
}

int sort_func4_on_nonleaf_postorder(Node *n, void *_data) {
  // n = Last child node.
  sort_func4_data_t * data = (sort_func4_data_t *)_data;

  const uint32_t idx_in = data->depth % 2;
  const uint32_t idx_out = 1-idx_in;

  // Put label refs into array
  //uint32_t sum_child_label_len = 0;
#ifdef TREE_REDUNDANT_INFOS
  const uint32_t num_nodes = n->parent->width;
  // Guarantee enough space.
  if (num_nodes > data->num_sorting_siblings) {
    free(data->sorting_siblings);
    data->num_sorting_siblings = MAX(num_nodes+8,
        2 * data->num_sorting_siblings);
    data->sorting_siblings = calloc(
        data->num_sorting_siblings, sizeof(label_node_ref4_t*));
    if (data->sorting_siblings == NULL) {
      data->num_sorting_siblings = 0;
      assert(0);
      return -4;
    }
  }

  // Fill array
  Node *cur = n->parent->child;
  label_node_ref4_t **array_cur = data->sorting_siblings;
  while(cur){
    const ptrdiff_t d = cur - data->anchor;
    // Link ref-array entry in sorting array. The values of refs[d]
    // was set in leaf-handler or previous call of this handler.
    *array_cur = &data->refs[d];

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
          data->num_sorting_siblings, sizeof(label_node_ref4_t*));
      if (data->sorting_siblings == NULL){
        data->num_sorting_siblings = 0;
        assert(0);
        return -4;
      }
    }

    // Fill array
    const ptrdiff_t d = cur - data->anchor;
    data->sorting_siblings[node_index] = &data->refs[d];

    //sum_child_label_len += data->refs[d].label_len;
    cur = cur->sibling;
    ++node_index;
  }
  uint32_t num_nodes = node_index;
#endif

  // Backup pointer to leftmost used label before sorting. All labels fullfill the
  // condition &label_child_1 < ... &label_child_K.
  assert(num_nodes > 0);
  label_node_ref4_t * const leftmost_pointer_input_labels = data->sorting_siblings[0];

  // Sorting the label refs
  _quicksort_for_sort4(
          data->sorting_siblings,
          data->sorting_siblings+num_nodes,
          _cmp_for_sort4);

  // Write label for leaf node by merging labels into label of parent node
  char *node_label = data->first_free_char_in_label_cache[idx_out];
  char *char_after_node_label = data->write_nonleaf_f(n, num_nodes,
      node_label, data);
  printf("Label: %s\n", node_label);

  // Re-sort children nodes
  if (data->sort_nodes) {
    assert(num_nodes > 0);
    label_node_ref4_t **pRef = data->sorting_siblings;
    label_node_ref4_t ** const pRefEnd = pRef + num_nodes;

    (*pRef)->node_ref->parent->child = (*pRef)->node_ref; // Leftmost child
    while (pRef < pRefEnd-1) {
      (*pRef)->node_ref->sibling = (*(pRef+1))->node_ref; // not (*pRef)+1…
      ++pRef;
    }
    (*pRef)->node_ref->sibling = NULL; // Rightmost child without sibling
  }

  // Eval number of consumed bytes
  assert(char_after_node_label >= node_label);
  uint32_t label_len = char_after_node_label - node_label;

  // Lock memory in idx_out-cache
  data->first_free_char_in_label_cache[idx_out] = char_after_node_label;
  // Unlock memory in idx_in-cache
  data->first_free_char_in_label_cache[idx_in] = leftmost_pointer_input_labels->label_ref;

  // Link to new label in parent node
  const ptrdiff_t dParent = n->parent - data->anchor;
  data->refs[dParent].label_ref = node_label;
  data->refs[dParent].label_len = label_len;
  data->refs[dParent].node_ref = n->parent;

  // We're done :-)
  data->depth--;
  return 0;
}



//================================================
// Public functions

#if 0 // Old variant
int tree_sort_canonical_order_inplace(
    Tree *tree,
    char **out_label)
{
  if( *out_label != NULL ){
    free(*out_label); *out_label = NULL;
  }

  sort_func1_data_t data = create_sort_func1_data(tree);
  if (data.anchor == NULL){ // Check allocations for failure
    assert(0);
    destroy_sort_func1_data(&data, 0);
    return -1;
  }

  int ret = _tree_depth_first_search(
      tree,
      sort_func1_on_leaf, sort_func1_on_nonleaf_preorder,
      NULL, sort_func1_on_nonleaf_postorder,
      &data);

  if (ret == 0){
    // Cleanup, but hold result
    destroy_sort_func1_data(&data, 1);
    *out_label = data.label_cache[0];
    return 0;
  }else{
    // Cleanup all
    destroy_sort_func1_data(&data, 0);
    return -1;
  }

}

/* Bring all isomorph trees into same order.
 * The label will represent the order of the nodes
 * and all isomorph trees have the same label.
 */
int tree_sort_canonical_order(
    const Tree *tree,
    Tree **out_tree,
    char **out_label)
{
  if( *out_tree != NULL ){
    tree_destroy(out_tree);
  }

  Tree *copied_tree = tree_clone(tree, NULL, NULL);
  tree_print(copied_tree, NULL, 0);
  tree_print_integrity_check(copied_tree->root);
  int ret = tree_sort_canonical_order_inplace(copied_tree, out_label);

  if (ret) { // error occoured, return NULL for out_tree and out_label
    tree_destroy(&copied_tree); *out_tree = NULL; // redundant.
    free(*out_label); *out_label = NULL;
    return ret;
  }
  *out_tree = copied_tree;
  return ret;
}

char *tree_canonical_label(
    const Tree *tree)
{

  sort_func2_data data = create_sort_func2_data(tree);
  if (data.anchor == NULL){ // Check allocations for failure
    assert(0);
    destroy_sort_func2_data(&data, 0);
    return NULL;
  }

  int ret = _tree_depth_first_search(
      (Tree *)tree /* Discarding const is ok for this function handlers */,
      sort_func2_on_leaf, sort_func2_on_nonleaf_preorder,
      NULL, sort_func2_on_nonleaf_postorder,
      &data);

  if (ret == 0){
    // Cleanup, but hold result
    destroy_sort_func2_data(&data, 1);
    return data.label_cache[0];
  }else{
    // Cleanup all
    destroy_sort_func2_data(&data, 0);
    return NULL;
  }
}
#else // New variant using unified code (see definition of sort_func4_data_t, pp )

int tree_sort_canonical_order_inplace(
        Tree *tree,
        char **out_label)
{
  if( *out_label != NULL ){
    free(*out_label); *out_label = NULL;
  }

  sort_func4_data_t data;
  if (create_sort_func4_data(tree, 1, write_leaf, write_nonleaf, &data)){
    return -1; // Allocation error
  }

  int ret = _tree_depth_first_search(
      tree,
      sort_func4_on_leaf, sort_func4_on_nonleaf_preorder,
      NULL, sort_func4_on_nonleaf_postorder,
      &data);

  if (ret == 0){
    // Cleanup, but hold result
    destroy_sort_func4_data(&data, 1);
    *out_label = data.label_cache[0];
    return 0;
  }else{
    // Cleanup all
    destroy_sort_func4_data(&data, 0);
    return -1;
  }

}

int tree_sort_canonical_order(
        const Tree *tree,
        Tree **out_tree,
        char **out_label)
{
  if( *out_tree != NULL ){
    tree_destroy(out_tree);
  }

  Tree *copied_tree = tree_clone(tree, NULL, NULL);
  int ret = tree_sort_canonical_order_inplace(copied_tree, out_label);

  if (ret) { // error occoured, return NULL for out_tree and out_label
    tree_destroy(&copied_tree); *out_tree = NULL; // redundant.
    free(*out_label); *out_label = NULL;
    return ret;
  }
  *out_tree = copied_tree;
  return ret;
}


char *tree_canonical_label(
        const Tree *tree)
{
  sort_func4_data_t data;
  if (create_sort_func4_data(tree, 0, write_leaf, write_nonleaf, &data)){
    return NULL; // Allocation error
  }

  int ret = _tree_depth_first_search(
      (Tree *)tree,
      sort_func4_on_leaf, sort_func4_on_nonleaf_preorder,
      NULL, sort_func4_on_nonleaf_postorder,
      &data);

  if (ret == 0){
    // Cleanup, but hold result
    destroy_sort_func4_data(&data, 1);
    char *out_label = data.label_cache[0];
    return out_label;
  }else{
    // Cleanup all
    destroy_sort_func4_data(&data, 0);
    return NULL;
  }
}


int tree_sort_by_hash_data_inplace(
        Tree *tree,
        char **out_label)
{
  if( *out_label != NULL ){
    free(*out_label); *out_label = NULL;
  }

  sort_func4_data_t data;
  if (create_sort_func4_data(tree, 1, hash_leaf, hash_nonleaf, &data)){
    return -1; // Allocation error
  }

  int ret = _tree_depth_first_search(
      tree,
      sort_func4_on_leaf, sort_func4_on_nonleaf_preorder,
      NULL, sort_func4_on_nonleaf_postorder,
      &data);

  if (ret == 0){
    // Cleanup, but hold result
    destroy_sort_func4_data(&data, 1);
    *out_label = data.label_cache[0];
    return 0;
  }else{
    // Cleanup all
    destroy_sort_func4_data(&data, 0);
    return -1;
  }
}


int tree_sort_by_hash_data(
        const Tree *tree,
        Tree **out_tree,
        char **out_label)
{
  if( *out_tree != NULL ){
    tree_destroy(out_tree);
  }

  Tree *copied_tree = tree_clone(tree, NULL, NULL);
  int ret = tree_sort_by_hash_data_inplace(copied_tree, out_label);

  if (ret) { // error occoured, return NULL for out_tree and out_label
    tree_destroy(&copied_tree); *out_tree = NULL; // redundant
    free(*out_label); *out_label = NULL;
    return ret;
  }
  *out_tree = copied_tree;
  return ret;
}


char *tree_hash_label(
        const Tree *tree)
{
  sort_func4_data_t data;
  if (create_sort_func4_data(tree, 0, hash_leaf, hash_nonleaf, &data)){
    return NULL; // Allocation error
  }

  int ret = _tree_depth_first_search(
      (Tree *)tree,
      sort_func4_on_leaf, sort_func4_on_nonleaf_preorder,
      NULL, sort_func4_on_nonleaf_postorder,
      &data);

  if (ret == 0){
    // Cleanup, but hold result
    destroy_sort_func4_data(&data, 1);
    char *out_label = data.label_cache[0];
    return out_label;
  }else{
    // Cleanup all
    destroy_sort_func4_data(&data, 0);
    return NULL;
  }
}


int tree_sort_by_data_pointer_inplace(
        Tree *tree,
        char **out_label)
{
  if( *out_label != NULL ){
    free(*out_label); *out_label = NULL;
  }

  sort_func4_data_t data;
  if (create_sort_func4_data(tree, 1, enrich_leaf_by_pointer, enrich_nonleaf_by_pointer, &data)){
    return -1; // Allocation error
  }

  int ret = _tree_depth_first_search(
      tree,
      sort_func4_on_leaf, sort_func4_on_nonleaf_preorder,
      NULL, sort_func4_on_nonleaf_postorder,
      &data);

  if (ret == 0){
    // Cleanup, but hold result
    destroy_sort_func4_data(&data, 1);
    *out_label = data.label_cache[0];
    return 0;
  }else{
    // Cleanup all
    destroy_sort_func4_data(&data, 0);
    return -1;
  }

}

int tree_sort_by_data_pointer(
        const Tree *tree,
        Tree **out_tree,
        char **out_label)
{
  if( *out_tree != NULL ){
    tree_destroy(out_tree);
  }

  Tree *copied_tree = tree_clone(tree, NULL, NULL);
  int ret = tree_sort_by_data_pointer_inplace(copied_tree, out_label);

  if (ret) { // error occoured, return NULL for out_tree and out_label
    tree_destroy(&copied_tree); *out_tree = NULL; // redundant
    free(*out_label); *out_label = NULL;
    return ret;
  }
  *out_tree = copied_tree;
  return ret;
}


#endif
