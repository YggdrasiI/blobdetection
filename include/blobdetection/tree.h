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

// Like opencvs Rect_<int32_t>
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

/* ===  Consistent tree operations ===
 *
 * This functions are more conservative and preserving a consistent state of tree.
 */
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
 * is like a root node, but shares the memory of same Tree struct.
 *
 */
int tree_release_child(
        Node * const child,
        int leaf_only);

/* Swap positions of two children of the same parent. */
int tree_swap_siblings(
        Node * const child1,
        Node * const child2);

/* Replaces node1 by node2 and vice versa. */
int tree_swap_nodes(
        Node * const node1,
        Node * const node2);

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

static inline Node * tree_right_sibling(
        Node *n)
{
    return n->sibling;
}

Node * tree_left_sibling(
        Node *n);

uint32_t tree_number_of_nodes(
        Node *root);

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
    const Tree * tree,
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

/* Generate unique id for sorting trees.
 * [DE] Wird ein Baum durchlaufen und für jeden Schritt angegeben, ob als nächstes ein
 * Kindknoten oder der nächste Geschwisterknoten auf x-ter Ebene ist, so entsteht eine
 * eindeutige Id eines Baumes. Diese kann später für vergleiche genutzt werdne.
 * Kann man das noch komprimieren, wenn man als Basis die maximale Tiefe wählt?!
 *
 * */
void _gen_tree_id(
        Node *root,
        uint32_t **id,
        uint32_t *d);


/* Generate Unique Number [xyz...] for node
 * Preallocate id-array with #nodes(root).
 * */
void tree_generate_id(
        Node *root,
        uint32_t* id,
        uint32_t size);

// Debug/Helper-Functions
int8_t * debug_getline(void);
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
