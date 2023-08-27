#ifndef TREE_H
#define TREE_H

/*
 * Simple Treestructure with Nodes N={parent, next_sibling, child, data}
 *
 * */

#include <stdlib.h>

// Do not use bool to prevent conflicts with c++'s bool
//#ifdef VISUAL_STUDIO
//#include "vc_stdbool.h"
//#else
//#include <stdbool.h>
//#endif

#include <stdint.h>
#include <limits.h>
#include <stdio.h>

#if __has_include("settings.h")
#include "settings.h"
#endif
#include "settings.default.h"

#include "enums.h"

// Like opencvs Rect_<int> (but unsigned…)
typedef struct BlobtreeRect BlobtreeRect;
struct BlobtreeRect{
  uint32_t x, y, width, height;
} ;

/* Minimal Node struct
 * Note: The height value stores the length of the longest path of successors, not
 * the depth to a (unknown) root node.
 * */
typedef struct Node {
  struct Node *parent;
  struct Node *sibling;
  struct Node *child; /* second child reachable as sibling of first child and so on… */
#ifdef TREE_REDUNDANT_INFOS
  uint32_t height; /* height = maximal reachable depth */
  uint32_t width; /* number of children */
#endif
  void *data;
} Node;
static const struct Node Leaf = { NULL, NULL, NULL,
#ifdef TREE_REDUNDANT_INFOS
  0, 0,
#endif
  NULL };

/* The data struct for both algorithms.
 * Some functions, which operates on a tree
 * expect this data struct. (This functions
 * should be recognizeable by its names.) */
typedef struct Blob Blob;
struct Blob{
  uint32_t id;
  BlobtreeRect roi;
  uint32_t area;
#ifdef SAVE_DEPTH_MAP_VALUE
  uint8_t depth_level;  // != level in tree structure
#endif
#ifdef BLOB_BARYCENTER
  int32_t barycenter[2];
#endif
};

typedef struct Tree Tree;
struct Tree{
  Node * const nodes; // Points on begin of allocated space. Required to release mem in tree_destroy().
  const uint32_t size; //length of node array.
  Node * root; // root of tree.
};

/* Allocate tree struct. If you use
 * the data pointer of the nodes you has to
 * setup/handle the storage for this data
 * separatly.
 *
 * size:       Maximal available space for nodes.
 * init_nodes: If != 0 all node will initialized as leafs.
 *             If == 0 you has to setup the nodes on your own,
 *             e.g. with ``nodes[i] = Leaf``.
 * */
Tree *tree_create(
    uint32_t size,
    int init_nodes);

/* Dealloc tree. Attention, target of data pointer is not free'd. */
void tree_destroy(
    Tree **tree);

#ifdef TREE_REDUNDANT_INFOS
/* Eval height and number of children for each Node */
void tree_gen_redundant_information(
    Node * const root);

/* Eval height and number of children for each Node */
void tree_gen_redundant_information_recursive(
    Node* root);
#endif

/* === Read-only operations ===
*/

/* Gen unique and stable id for nodes in this tree-struct array. */
int32_t tree_get_node_id(
    const Tree *tree,
    const Node *node);

static inline Node * tree_right_sibling(
    const Node *n)
{
  if (n == NULL) return NULL;
  return n->sibling;
}

/* Returns left sibling of node or NULL. */
Node * tree_left_sibling(
    const Node *n);

/* Going upwards from 'successor' until child of descendant is reached.
 *
 * Note: Returns NULL if successor is direct child.
 */
static inline Node * node_get_node_before_descendant(const Node *descendant, const Node *successor)
{
  if (successor == NULL ) return NULL;
  Node * s = successor->parent;
  while(s){
    if(s->parent == descendant) return s;
    s = s->parent;
  }
  return NULL;
}

/* Returns:
 *    1: If subtree(descendant) contains successor.
 *    0: Otherwise.
 */
static inline int node_is_descendant(const Node *descendant, const Node *successor)
{
  while(successor->parent){
    if(successor == descendant) return 1;
    successor = successor->parent;
  }
  return 0;
}
static inline int node_is_successor(const Node *successor, const Node *descendant)
{
 return node_is_descendant(descendant, successor);
}

static inline int node_is_parent(const Node *parent, const Node *child){
  return (child->parent == parent)?1:0;
}

static inline int node_is_child(const Node *child, const Node *parent){
  return node_is_parent(parent, child);
}

uint32_t tree_number_of_nodes(
    const Tree * const tree);

uint32_t node_number_of_successors(
    const Node * node);

static inline uint32_t node_number_of_descendants(
    const Node * node){
  uint32_t r=0;
  while(node->parent){
    ++r;
    node = node->parent;
  }
  return r;
}

static inline uint32_t node_number_of_children(
    const Node * node){
#ifdef TREE_REDUNDANT_INFOS
  return node->width;
#endif
  uint32_t c=0;
  node=node->child;
  while(node){
    ++c;
    node = node->sibling;
  }
  return c;
}

static inline uint32_t node_get_depth(
    const Node * node){
  return node_number_of_descendants(node);
}

static inline uint32_t node_get_height(
    const Node * node){
#ifdef TREE_REDUNDANT_INFOS
  return node->height;
#else
#error("Not implemented")
  return -1;
#endif
}

static inline Node * node_get_root(
    Node *node)
{
  while(node->parent) {
    node = node->parent;
  }
  return node;
}

static inline Node * tree_get_root(
    Tree *tree)
{
  return tree->root;
}

/* ===  Consistent tree operations ===
 *
 * This functions should preserve a consistent state of tree.
 */
int tree_set_root(
    Tree *tree,
    Node *node);

/* Adding node (and it's siblings) to parent node.
 *
 * Note that you will also transfer the siblings of 'child'.
 * Release child from its previous location by tree_release_child()
 * if you want just transfer a single node.
 */
int tree_add_siblings(
    Node * const parent,
    Node * const child);

/* Release child from its parent. After this child
 * is like a root node (no parent and no siblings), but is
 * not connected to the main tree.
 *
 * It still shares the memory of Tree struct.
 */
int tree_release_child(
    Node * const child,
    int leaf_only);


/* Swap data pointers of nodes.
 *
 * This operation is cheaper then tree_swap_nodes(node1, node2, 1, 1).
 * but has alsmost the same effect.
 * */
int tree_swap_data(
    Node * const node1,
    Node * const node2);

/* Replaces node1 by node2 and vice versa.
 *
 * The children of the nodes will be swapped. To keep them
 * use tree_swap_subtrees(…).
 *
 * If node_i is child of node_(1-i), the set of childs can
 * not be keept:
 *
 * child_node_rule == 0: Reject swap of nodes if one node is parent of the other.
 * child_node_rule == 1: Allow change of childs.
 *
 *
 * TODO: Define names in enums.h
 * */
int tree_swap_nodes(
    Tree * const tree,
    Node * const node1,
    Node * const node2,
    int child_node_rule,
    int data_pointer_swap);

/* Replaces node1 by node2 and vice versa, but both nodes
 * keeping its children.
 *
 * If subtree(node_i) is containing node_(1-i), the path between
 * both nodes has to be excluded from the swap and has to be
 * handled differently:
 *
 * descendant_node_rule == 0: Reject swap of nodes
 * descendant_node_rule == 1: Keep order of nodes between both nodes.
 * descendant_node_rule == 2: Reverse order of nodes between both nodes.
 *
 * TODO: Define names in enums.h
 * */
int tree_swap_subtrees(
    Tree * const tree,
    Node * const node1,
    Node * const node2,
    int descendant_node_rule,
    int data_pointer_swap);

/* == Dangerous tree operations ===
 *
 */

/* Add 'node' as child of given parent node 'parent'.
 * Attention,  this functions may lead to inconsistent state of tree
 * if 'child'
 *   a) Already owns a parent node
 *   b) Already owns a sibling.
 */
int tree_add_child(
    Node * const parent,
    Node * const node);


/* Copy tree
 *
 * Args:
 *    source:            Tree to clone
 *    data, cloned_data: Optional pointers to arrays
 *                       with identical content.
 *
 *                       If not NULL the Node.data pointer
 *                       will mapped into cloned_data.
 *
 * Returns:
 *    New tree with same structure as origin.
 */
Tree *tree_clone(
    const Tree * source,
    const void * data,
    const void * cloned_data);


/* ======  For traversal over trees ================= */

/* Type for functions called on each node
 * during a traversal of a tree.
 *
 * Return value != 0 breaks loop over tree */
typedef int node_func_t(const Node *n, void *data);

/* Same as node_func_t but type indicates that n->parent is the target
 * node of the event.
 * We can not simply propagate n->parent instead of n because
 * a) In-Order calls can be done multiple times for nodes with n>2 childs.
 * b) Post-Order calls can be handled better if we know the right-most child.
 * c) Pre-Order calls do not profit, but to normalize the behavour to the other
 *    handlers. We propagate the child, too.
 *
 * Alternativly, one could propagate n AND the redundant n->parent.
 * */
typedef node_func_t child_node_func_t;

/* Handlers for double tree traversal.
 * Here, we propagate nodes of both trees for comparsion.
 *
 * Note that this is just useful for trees of the same structure.
 *
 * It would fail if the trees are just isomorph. It this case you
 * hat to convert the isomorpth trees in the (shared) canonical form.
 * * */
typedef int node2_func_t(const Node *n1, const Node *n2, void *data);
typedef node2_func_t child_node2_func_t;


//=======================================================
/* Compare trees with different metrics.
*/
int tree_cmp(
    const Tree * const tree1,
    const Tree * const tree2,
    enum tree_compare_types compare_type);

//=======================================================

/* Textual output of tree.
 * subtree_root is NULL or defining the root of printed (sub-)tree.
 * Shift defines number of spaces at line beginning. */
void tree_print(
    const Tree *tree,
    const Node *subtree_root,
    int32_t shift);


/* Sorting the nodes such that topological equal trees got
 * the same result. The algorithm sort rekursivly all children
 * and then the siblings with quicksort.
 *
 * TODO: FIX THIS STUFF
 * */
void tree_sort(
    Node *root);


/* => see enums.h
   typedef enum {
   TREE_ERROR_ROOT_HAS_SIBLING=-1,
//…
} TREE_ERROR_CONSISTENT_CHECKS;
*/

/* Returns 0 if given tree or subtree is in consistent state
 *
 * Rules:
 * 1. Only root nodes has NULL parent.
 * 2. root nodes have no sibling.
 * 2. Siblings had same parent node.
 *
 * If Nodes tracking the width and height value:
 * 3. Check if values are tracked correctly.
 *
 * */
int tree_integrity_check(
    Node *root);

void tree_print_integrity_check(
    Node *root);

#endif
