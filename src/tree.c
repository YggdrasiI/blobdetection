#include <stdio.h>
#include <math.h>

#include <assert.h>

//#define INLINE inline
#include "blobdetection/tree.h"
#include "tree_intern.h"

/* Allocate tree struct. If you use
 * the data pointer of the nodes you has to
 * setup/handle the storage for this data
 * separatly.
 */
Tree *tree_create(
    uint32_t size,
    int init_nodes)
{
  // malloc/calloc can return pointer !=NULL for size=0.
  if(size == 0) return NULL;

  Tree *tree = (Tree*) malloc(sizeof(Tree));
  if(tree == NULL) return NULL;

  Node *nodes;
  if (init_nodes == 0) {
    nodes = (Node*) malloc( size*sizeof(Node));
  }else{
    nodes = (Node*) calloc( size, sizeof(Node));
  }
  if(nodes == NULL) { // malloc failed
      free(tree);
      return NULL;
  }

  // Only position to setup read-only variables
  *(Node **)&tree->nodes = nodes;
  *(uint32_t *)&tree->size = size;

  tree->root = tree->nodes;
  if (init_nodes == 0) {
    // Root node initialized as leaf, all other nodes unintialized.
    *tree->root = Leaf;
  }else{
    for( uint32_t i=0; i<size; ++i ) {
      // Setup any non-null values. Currently above calloc is fine…
    }
  }

  return tree;
}


/* Dealloc tree. Attention, target of data pointer is not free'd. */
void tree_destroy(
    Tree **ptree)
{
  if( *ptree == NULL ) return;
  Tree *tree = *ptree;
  if( tree->nodes != NULL ){
    free( tree->nodes );
  }
  free(tree);
  *ptree = NULL;
}


#ifdef TREE_REDUNDANT_INFOS
/* Eval height and number of children for each Node */
void tree_gen_redundant_information(
    Node * const root)
{
  // Remove height information about this subtree from parent
  Node *node = root;
  if( node->parent ){
      --node->parent->width; // Will be increased again later.
  }
  while( node->parent ){
    node->parent->height = 1; // Not 0… node is still present.
    Node *cur = node->parent->child;
    while(cur){
      if(cur!= node){
        if( node->parent->height < cur->height+1 ){
          node->parent->height = cur->height+1;
        }
      }
      cur = cur->sibling;
    }
    node = node->parent;
  }

  // Now handle this subtree.
  node = root;
  do{
    /* Reset values */
    node->height = 0;
    node->width = 0;

    if( node->parent ){
      ++node->parent->width;
      if( node->parent->height < node->height+1 ){
        node->parent->height = node->height+1;
      }
    }

    /* To to next node. update parent node if possible */
    if( node->child != NULL ){
      node = node->child;
      continue;
    }
    if( node->sibling != NULL ){
      node = node->sibling;
      continue;
    } 
    while( node->parent != NULL ){
      node = node->parent;
      if( node->parent ){
        if( node->parent->height < node->height+1 ){
          node->parent->height = node->height+1;
        }
      }
      if( node == root) break;
      if( node->sibling != NULL ){
        node = node->sibling;
        break;
      }
    }
  }while( node != root );

}


void _tree_gen_redundant_information_recursive(
    Node* root,
    uint32_t *pheight,
    uint32_t *psiblings);

/* Eval height and number of children for each Node */
void tree_gen_redundant_information_recursive(
    Node* root)
{
  if(root->parent == NULL){
    _tree_gen_redundant_information_recursive(root, NULL, NULL);
  }else{
    // Remove height information about this subtree from parent
    Node *node = root;
    --node->parent->width; // Will be increased again later.
    while( node->parent ){
      node->parent->height = 1; // Not 0… node is still present.
      Node *cur = node->parent->child;
      while(cur){
        if(cur!= node){
          if( node->parent->height < cur->height+1 ){
            node->parent->height = cur->height+1;
          }
        }
        cur = cur->sibling;
      }
      node = node->parent;
    }
    _tree_gen_redundant_information_recursive(root,
        &root->parent->height, &root->parent->width);
  }
}

void _tree_gen_redundant_information_recursive(
    Node* root,
    uint32_t *pheight,
    uint32_t *psiblings)
{
  root->width = 0;
  if ( psiblings != NULL ){
    (*psiblings)++; //update number of children for parent node
  }
  if( root->child != NULL ){
    uint32_t height2=0;
    _tree_gen_redundant_information_recursive(root->child, &height2, &root->width);
    if( pheight != NULL) { 
      if( *pheight < height2+1 ) *pheight = height2+1; //update height of parent node
    }
  }
  if( root->sibling != NULL ){
    _tree_gen_redundant_information_recursive(root->sibling, pheight, psiblings);
  }
}
#endif

// Update of height after one branch grows
// Here, c has to be new/updated child of p!
static inline void _update_heights_after_add(
    Node *p,
    uint32_t h /* Increased height of existing node or height of new node */)
{
  ++h;
  while( p != NULL && p->height < h ){
    p->height = h;
    p=p->parent;
    ++h;
  }
}

// Update of height after one branch shrinked
// Here, c has to be old/outdated child of p! (To check
// if p->height was ruled by c's values.)
static inline void _update_heights_after_shrink(
    Node *p,
    uint32_t previous_height_of_removed_or_changed_child)
{

  // New height is maximum of all children except the removed/shrinked
  // one. We need to go upwards until the node is not
  // in the critical branch.
  while( p != NULL && p->height == previous_height_of_removed_or_changed_child+1 ){
    uint32_t new_max_height_child = 0;
    Node *c=p->child;
    while (c != NULL){
      if (new_max_height_child < c->height){
        new_max_height_child = c->height;
      }
      c = c->sibling;
    }

    previous_height_of_removed_or_changed_child = p->height; // for next loop step
    p->height = new_max_height_child + 1;
    p=p->parent;
  }
}

int tree_add_child(
    Node * const parent,
    Node * const node)
{
  if (parent == NULL) return -2;
  if (node == NULL) return -1;
  assert(node->sibling == NULL);

  if( parent->child == NULL ){
    parent->child = node;
  }else{
    Node *cur = parent->child;
    while( cur->sibling != NULL ){
      cur = cur->sibling;
    }
    cur->sibling = node;
  }
  node->parent = parent;

#ifdef TREE_REDUNDANT_INFOS
  //update redundant information
  parent->width++;
  _update_heights_after_add(parent, node->height);
#endif
  return 0;
}

int tree_set_root(
    Tree *tree,
    Node *node){
  ptrdiff_t d = node - tree->nodes;
  if (d >= tree->size) return -1;
  if (d < 0) return -2;
  if (node->parent != NULL) return -3;
  if (node->sibling != NULL) return -4;

  tree->root = node;
  return 0;
}

int tree_add_siblings(
    Node * const parent,
    Node * const sibling)
{
  if (parent == NULL) return -2; // Use tree_release_child()
  if (sibling == NULL) return -1;


  // Backup leftmost sibling before releasing previous parent info
  Node * const leftmost_sib=sibling->parent==NULL?sibling:sibling->parent->child;

  Node *prev_parent = leftmost_sib->parent;
#ifdef TREE_REDUNDANT_INFOS
    // Backup values (**)
    const uint32_t num_siblings_prev_parent=prev_parent?prev_parent->width:0;
    const uint32_t height_prev_parent=prev_parent?prev_parent->height:0;
#endif

  // Removing child info in previous parent node
  if (prev_parent != NULL){
    // Note that we cannot use tree_release_child() because it would clear
    // the sibling releation between the looped nodes!

    // Loop commented out, because we will set the parent again, see below.
#if 0
    Node *sib = leftmost_sib;
    while (sib != NULL){
      sib->parent = NULL;
      sib = sib->sibling;
    }
#endif
    prev_parent->child = NULL;

#ifdef TREE_REDUNDANT_INFOS
    // update redundant information
    prev_parent->width = 0;
    prev_parent->height = 0;
    _update_heights_after_shrink(prev_parent->parent, height_prev_parent);
#endif
  }

  // Attach leftmost new sibling to rightmost old sibling
  if( parent->child == NULL ){
    parent->child = leftmost_sib;
  }else{
    Node *cur = parent->child;
    while( cur->sibling != NULL ){
      cur = cur->sibling;
    }
    cur->sibling = leftmost_sib;
  }

  //set new parent of children
  Node *sib = leftmost_sib;
  while( sib ){
    sib->parent = parent;
    sib=sib->sibling;
  }

#ifdef TREE_REDUNDANT_INFOS
  /*
  // Counting New child and get maximal height
  uint32_t num_siblings=parent->width;
  uint32_t max_new_height=parent->height;
  Node *sib = leftmost_sib;
  while( sib ){
    sib->parent = parent;
    num_siblings++;
    if(sib->height > max_new_height){
      max_new_height=sib->height;
    }
    sib=sib->sibling;
  }

  //update redundant information
  parent->width+=num_siblings;
  parent->height = max_new_height;
  _update_heights_after_add(p->parent, p->height);
  */
  // Relying on (**) saves some operation
  parent->width+=num_siblings_prev_parent;
  if (parent->height<height_prev_parent) parent->height=height_prev_parent;
  _update_heights_after_add(parent->parent, parent->height);
#endif

  return 0;
}

/* Remove child from its parent node.
 *
 */
int tree_release_child(
    Node * const child,
    int leaf_only)
{
  if (child == NULL) return -2;
  if (leaf_only && child->child) return -3;
  Node *parent = child->parent;
  if (parent == NULL){
    if (child->sibling){
#ifndef NDEBUG
      fprintf(stderr, "tree_release_child() called for root node with sibling"
          "This indicates that tree has no integrity!");
#endif
      return -1;
    }
    return 0; // nothing to do for root nodes without sibling.
  }
  /*if (parent == NULL){
      // No parent node. We do not return here because
      // the (root) node can still have siblings
  }else */if (parent->child == child) {
      // first child case
     parent->child = child->sibling;
  }else{
      // other child cases
      Node *sibling = parent->child;
      while (sibling){
          if (sibling->sibling == child) {
              sibling->sibling = child->sibling;
              break;
          }
          sibling = sibling->sibling;
      }
  }

#ifdef TREE_REDUNDANT_INFOS
  //update redundant information
  parent->width--;
  _update_heights_after_shrink(parent, child->height);
#endif

  // Release links from child to it parent/right siblings, too.
  child->parent = NULL;
  child->sibling = NULL;
  return 0;
}

#define SWAP(A, B) {\
  typeof(A) _t = A;\
  A = B;\
  B = _t;\
}

/* Swapping position of two nodes under the condition that
 * node1 is sibling of node2.
 * We assume here that the root node never owns a sibling. Thus
 * both input nodes will have a parent.
 */
int _tree_swap_siblings(
    Tree * const tree,
    Node * const node1,
    Node * const node2,
    int children_swap,
    int data_pointer_swap)
{
  /*
  if (node1 == NULL) return -4; // use tree_release_child()
  if (node2 == NULL) return -3; // use tree_release_child()
  Node *parent = node1->parent;
  if (parent == NULL) return -2;
  if (parent != node2->parent) return -1; // not the same parent.
  */
  Node *parent = node1->parent;
  assert(node1 != node2);
  assert (node1->parent == node2->parent);
  assert (node1->parent != NULL);

  if (node1 == node2) return 0; // Avoids checks for some cycle references below

  // Search left siblings. If NULL after search childX is leftmost node
#if 0
  Node *left_sib1=left_sibling(node1);
  Node *left_sib2=left_sibling(node2);
#else
  Node *left_sib1 = NULL, *left_sib2 = NULL;
  Node *sib = node1->parent->child;
  Node *ref, **refB;
  while (sib){
    // Search for node1 and node2
    if (sib->sibling == node1){
      left_sib1=sib;
      ref=node2; refB=&left_sib2; // References for second search
      break;
    }
    if (sib->sibling == node2){
      left_sib2=sib;
      ref=node1; refB=&left_sib1; // References for second search
      break;
    }
    sib = sib->sibling;
  }
  while (sib){
    // Search for the other element
    if (sib->sibling == ref){
      *refB=sib;
      break;
    }
    sib = sib->sibling;
  }
#endif

  // Swap right siblings, which also can be NULL
  if (node1->sibling == node2) {       /* (1) */
    //    <node1> —> <node2> —> <right_c2>
    // => <node2> —> <node1> —> <right_c2>
    node1->sibling = node2->sibling;
    node2->sibling = node1;
  }else if (node2->sibling == node1) { /* (2) */
    //    <node2> —> <node1> —> <right_c1>
    // => <node1> —> <node2> —> <right_c1>
    node2->sibling = node1->sibling;
    node1->sibling = node2;
  }else {
    //    <node1> —> <right_c1> + <node2> —> <right_c2>
    // => <node1> —> <right_c2> + <node2> —> <right_c1>
    SWAP(node1->sibling, node2->sibling);
  }

  // Swap references in left siblings/parent
  if (left_sib1){
    if (left_sib1 != node2) { // Otherwise it would be a self reference 
                               // This case was already handled in (2)
      left_sib1->sibling = node2;
    }
  }else{
    parent->child = node2;
  }
  if (left_sib2){
    if (left_sib2 != node1) { // Otherwise it would be a self reference 
                               // This case was already handled in (1)
      left_sib2->sibling = node1;
    }
  }else{
    parent->child = node1;
  }

  if (children_swap){
    //Anchors on first child.
    Node *c1 = node1->child;
    Node *c2 = node2->child;

    // Swap Children
    node1->child=c2;
    node2->child=c1;

    // Swap parent relation of children
    while (c1) {
      c1->parent = node2;
      c1 = c1->sibling;
    }
    while (c2) {
      c2->parent = node1;
      c2 = c2->sibling;
    }
  }

  if (data_pointer_swap) {
    SWAP(node1->data, node2->data);
  }

  // Update tree root
  if (node1 == tree->root){
    tree_set_root(tree, node2);
  }else if(node2 == tree->root){
    tree_set_root(tree, node1);
  }

#ifdef TREE_REDUNDANT_INFOS
  // Nothing to do in this case/operation.
#endif
  return 0;
}

/* Swapping position of two nodes under the condition that
 * node1 is parent of node2.
 *
 * Valid input has following structure:
 *    <parent1> —> <node1> —> <node2> —> CN2:= {children node2}
 *                         —> CN1' := {other children node1}
 *              —> CP1':= {other children parent1}
 *
 * Target is following structure:
 *    <parent1> —> <node2> —> <node1> -> CN2
 *                         —> CN1'
 *              —> CP1'
 *
 */
int  _tree_swap_parent_child_tuple(
    Tree * const tree,
    Node *node1,
    Node *node2,
    int data_pointer_swap)
{
  assert(node1 != NULL);
  assert(node2 != NULL);
  assert(node1 != node2);
  assert(node2->parent == node1);

  Node *parent1 = node1->parent;

  // Search left siblings of both nodes
  Node *left_sib1 = tree_left_sibling(node1); // Element of CP1' or NULL
  Node *left_sib2 = tree_left_sibling(node2); // Element of CN1' or NULL

  // Change parent relation of CN1' ⋃ {node2}
  // (Change of node2 will be fixed later (**))
  Node *c1 = node1->child;
  while (c1) {
    c1->parent = node2;
    c1 = c1->sibling;
  }

  // Change parent relation of CN2
  Node *c2 = node2->child;
  while (c2) {
    c2->parent = node1;
    c2 = c2->sibling;
  }

  // Change parents
  node1->parent = node2;
  node2->parent = parent1; // Fixes (**)

  // Swap child relations
  Node *tmp = node1->child;
  node1->child=node2->child;
  node2->child=tmp; // (***) Will be fixed later if tmp==node2.


  // Change left siblings or child anchor in it's parents
  if(left_sib1){
    left_sib1->sibling = node2;
  }else{
    if (parent1) {
      parent1->child = node2;
    }
  }
  if(left_sib2){
    left_sib2->sibling = node1;
  }else{
    node2->child = node1; // Fixes (***)
  }

  // Swap right siblings. No interleaving possible.
  SWAP(node1->sibling, node2->sibling);

  if (data_pointer_swap) {
    SWAP(node1->data, node2->data);
  }

  // Update tree root
  if (node1 == tree->root){
    tree_set_root(tree, node2);
  }else if(node2 == tree->root){
    tree_set_root(tree, node1);
  }

  // TODO: Update width/height?!

 return 0;
}

/* Swapping subtrees of two nodes under the condition that
 * node1 is parent of node2.
 *
 * Valid input has following structure:
 *    <parent1> —> <node1> —> <node2> —> CN2:= {children node2}
 *                         —> CN1' := {other children node1}
 *              —> CP1':= {other children parent1}
 *
 * Target is following structure:
 *    <parent1> —> <node2> —> <node1> -> CN1'
 *                         —> CN2
 *              —> CP1'
 *
 *
 * exclude_child2: (For _tree_swap_subtrees_descendant())
 *   Used in recursion steps to mark the path from node1 to node2.
 *   This path is excluded from swapping.
 *   The end-node on the other side of the path will be 'tl_node2'
 *
 */
int  _tree_swap_subtrees_direct_parent(
    Tree * const tree,
    Node *node1,
    Node *node2,
    Node *exclude_child2,
    int data_pointer_swap)
{
  assert(node1 != NULL);
  assert(node2 != NULL);
  assert(node1 != node2);
  assert(node2->parent == node1);

  Node *parent1 = node1->parent;

  // Search left siblings of both nodes
  Node *left_sib1 = tree_left_sibling(node1); // Element of CP1' or NULL
  Node *left_sib2 = tree_left_sibling(node2); // Element of CN1' or NULL

  Node *left_exclude_child2 = tree_left_sibling(exclude_child2); // Element of CN2' or NULL

  /* Change of siblings:
   * 1. Remove node2 from CN1
   * 1b Remove exclude_child2 from CN2
   * 2. Replace node1 in CP1' by node2
   * 3. Add node1 to CN2 as leftmost sibling
   * 3b Add exclude_child2 to CN1 as leftmost sibling if !=NULL
   * Order of this operations is important!
   */

  // 1) Remove node2 from CN1
  if(left_sib2){
    left_sib1->sibling = node2->sibling;
  }else{
    assert(node1->child == node2);
    node1->child = node2->sibling;
  }
  // Final 'node2->sibling = ...' done in (2).

  // 1a) Remove exclude_child2 from CN2
  if(left_exclude_child2){
    left_exclude_child2->sibling = exclude_child2->sibling;
  }else if(exclude_child2){
    assert(node2->child == exclude_child2);
    node2->child = exclude_child2->sibling;
  }
  // Final 'exclude_child2->sibling = ...' done in (3b).

  // 2) Replace node1 in CP1' by node2
  if(left_sib1){
    left_sib1->sibling = node2;
  }else{
    if (parent1) {
      parent1->child = node2;
    }
  }
  node2->sibling = node1->sibling;
  // Final 'node1->sibling = ...' done in (3).

  // 3) Add node1 to CN2 as leftmost sibling
  node1->sibling = node2->child;
  node2->child = node1;

  // 3b
  if (exclude_child2){
    exclude_child2->sibling = node1->child;
    node1->child = exclude_child2;
    exclude_child2->parent = node1;
  }

  // Change parents
  node1->parent = node2;
  node2->parent = parent1;

  // Child relation need no further changes except above in this variant.

  if (data_pointer_swap) {
    SWAP(node1->data, node2->data);
  }

  // Update tree root
  if (node1 == tree->root){
    tree_set_root(tree, node2);
  }else if(node2 == tree->root){
    tree_set_root(tree, node1);
  }

 return 0;
}

/* Swapping position of two subtrees under the condition that
 * subtree(node1) containing subtree(node2)
 *
 * Valid input has following structure:
 *    <parent1> —> <node1> —> … —> <parent2> —> <node2> —> CN2:= {children node2}
 *                                           —> CP2' := {other children parent1}
 *                         —> CN1' := {other children node1}
 *              —> CP1':= {other children parent1}
 *
 * Target is following structure:
 *    <parent1> —> <node2> —>(… —> <parent2>)—> <node1> -> CN1'
 *                            ‾‾‾‾‾‾‾‾‾‾‾‾‾‾ —> CP2'
 *                         —> CN2
 *              —> CP2'
 *              
 * exclude_child2: 
 *   Used in recursion steps to mark the path from node1 to node2.
 *   This path is excluded from swapping.
 *   The end-node on the other side of the path will be 'tl_node2'
 *
 * flip_direction:
 *   0: Nodes on path between node1 and node2 will not be changed.
 *   1: The order of nodes in bath will be inverted. (_-range in sketch)
 *
 */
int  _tree_swap_subtrees_descendant(
    Tree * const tree,
    Node *node1,
    Node *node2,
    Node *exclude_child2,
    int flip_direction,
    int data_pointer_swap)
{
  assert(node1 != NULL);
  assert(node2 != NULL);
  assert(node1 != node2);

  if (node2->parent == node1) {
    return _tree_swap_subtrees_direct_parent(tree, node1, node2, exclude_child2, data_pointer_swap);
  }

  assert(node2->parent != node1);
  Node *parent1 = node1->parent;
  Node *parent2 = node2->parent;
  // At this point, we know already that node1, node2, parent1,
  // and parent2 all differs.
  // Thus, node_get_node_before_descendant() should not return NULL
  // tl_node2 := top level node on the branch from node1 to node2.
  Node *tl_node2 = node_get_node_before_descendant(node1, node2);
  assert(tl_node2 != NULL);
  assert(tl_node2->parent == node1);

  // Search left siblings of both nodes
  Node *left_sib1 = tree_left_sibling(node1); // Element of CP1' or NULL
  Node *left_sib2 = tree_left_sibling(node2); // Element of CP2' or NULL
  // Left siblings of the critical childs of node1/node2
  Node *left_tl_node2 = tree_left_sibling(tl_node2); // Element of CN1' or NULL
  Node *left_exclude_child2 = tree_left_sibling(exclude_child2); // Element of CN2' or NULL

  /* Change of siblings:
   * 1a Remove tl_node2 (=path to node2) from CN1
   * 1b Remove exclude_child2 from CN2
   * 2. Swap node1 with node2 in CP1/CP2
   * 3a Add tl_node2 (=path to node1) to CN2 as leftmost sibling
   * 3b Add exclude_child2 to CN1 as leftmost sibling if !=NULL
   */

  // 1a) Remove tl_node2 from CN1
  if(left_tl_node2){
    left_tl_node2->sibling = tl_node2->sibling;
  }else{
    assert(node1->child == tl_node2);
    node1->child = tl_node2->sibling;
  }
  // Final 'tl_node2->sibling = ...' done in (3a).

  // 1a) Remove exclude_child2 from CN2
  if(left_exclude_child2){
    left_exclude_child2->sibling = exclude_child2->sibling;
  }else if(exclude_child2){
    assert(node2->child == exclude_child2);
    node2->child = exclude_child2->sibling;
  }
  // Final 'exclude_child2->sibling = ...' done in (3b).

  // 2) Replace node1 by node2 in CP1
  //    and replace node2 by node1 in CP2
  if(left_sib1){
    left_sib1->sibling = node2;
  }else{
    if (parent1) {
      parent1->child = node2;
    }
  }
  if(left_sib2){
    left_sib2->sibling = node1;
  }else{
    parent2->child = node1;
  }
  SWAP(node2->sibling, node1->sibling);

  // 3a) Add tl_node2 to CN2 as leftmost sibling
  tl_node2->sibling = node2->child;
  node2->child = tl_node2;
  tl_node2->parent = node2;

  // 3b
  if (exclude_child2){
    exclude_child2->sibling = node1->child;
    node1->child = exclude_child2;
    exclude_child2->parent = node1;
  }

  // Change parents
  node1->parent = parent2;
  node2->parent = parent1;


  if (data_pointer_swap) {
    SWAP(node1->data, node2->data);
  }

  // Update tree root
  if (node1 == tree->root){
    tree_set_root(tree, node2);
  }

  // Recursion 
  if( flip_direction ) {
    if( parent2 == node1){ // Directly connected.
    }
    else if ( parent2 == tl_node2) {
      // <node1> —> <lt_node2> —> <node2>  => No flipping of paths needed.
    }else{
      // At least two nodes between input nodes. Recurivly flipping them.
      int _ret = _tree_swap_subtrees_descendant(
          tree, tl_node2, parent2,
          node1 /*Now at position of node2 */, 1 /*flip_direction*/,
          data_pointer_swap); 
      if(_ret) {
        assert(0);
        return _ret;
      }
    }
  }

 return 0;
}

/* The uncritical case:
 * Nodes are not on the same branch and no siblings.
 *
 * Without the possibility of interleaving we could just swap all
 * reference pointers.
 */
int _tree_swap_unconnected_nodes(
    Tree * const tree,
    Node * const node1,
    Node * const node2,
    int children_swap,
    int data_pointer_swap)
{
  // Check parents
  Node *parent1 = node1->parent;
  Node *parent2 = node2->parent;

  // Search left siblings of both nodes
  Node *left_sib1 = tree_left_sibling(node1);
  Node *left_sib2 = tree_left_sibling(node2);

  // Swap parents
  node1->parent = parent2;
  node2->parent = parent1;

  // Swap right siblings, no interleaving possible
  SWAP(node1->sibling, node2->sibling);

  // Swap references in left siblings/parent
  if (left_sib1){
    left_sib1->sibling = node2;
  }else if(parent1){
    parent1->child = node2;
  }
  if (left_sib2){
    left_sib2->sibling = node1;
  }else if(parent2){
    parent2->child = node1;
  }

  if (children_swap) {
    //Anchors on first child.
    Node *c1 = node1->child;
    Node *c2 = node2->child;

    // Swap Children
    node1->child=c2;
    node2->child=c1;

    // Swap parent relation of children
    while (c1) {
      c1->parent = node2;
      c1 = c1->sibling;
    }
    while (c2) {
      c2->parent = node1;
      c2 = c2->sibling;
    }
  }

  if (data_pointer_swap) {
    SWAP(node1->data, node2->data);
  }

  // Update tree root
  if (node1 == tree->root){
    tree_set_root(tree, node2);
  }else if(node2 == tree->root){
    tree_set_root(tree, node1);
  }

#ifdef TREE_REDUNDANT_INFOS
  //update redundant information
  if (!children_swap) {
    if( node1->height < node2->height ){
      _update_heights_after_add(parent1, node2->height);
      _update_heights_after_shrink(parent2, node2->height);
    }
    else if( node1->height > node2->height ){
      _update_heights_after_shrink(parent1, node1->height);
      _update_heights_after_add(parent2, node1->height);
    }
  }else{
    SWAP(node1->width, node2->width);
    SWAP(node1->height, node2->height);
  }
#endif

  return 0;
}

int tree_swap_data(
    Node * const node1,
    Node * const node2)
{
  SWAP(node1->data, node2->data);
  return 0;
}

/* Replaces node1 by node2 and vice versa. */
int tree_swap_nodes(
    Tree * const tree,
    Node * const node1,
    Node * const node2,
    int child_node_rule,
    int data_pointer_swap)
{
  if (node1 == NULL) return -2; // use tree_release_child()
  if (node2 == NULL) return -3; // use tree_release_child()
  if (node1 == node2) return 0; // Nothing to do

  // Check parents
  Node *parent1 = node1->parent;
  Node *parent2 = node2->parent;
  if (parent1 == parent2
    /* swapping siblings just works for non-root nodes! */
    && parent1 != NULL ){
    return _tree_swap_siblings(tree, node1, node2, 1, data_pointer_swap);
  }

  if (node_is_parent(node1, node2)){
    if (child_node_rule == 1){
      return _tree_swap_parent_child_tuple(tree, node1, node2, data_pointer_swap);
    }
    return -1; // Operation not allowed
  }
  else if (node_is_parent(node2, node1)){
    if (child_node_rule == 1){
      return _tree_swap_parent_child_tuple(tree, node2, node1, data_pointer_swap);
    }
    return -1; // Operation not allowed
  }

  // Here, we know that nodes are not directly connected
  // and no siblings. This eliminates most corner cases.
  return _tree_swap_unconnected_nodes(tree, node1, node2, 1, data_pointer_swap);
}

int tree_swap_subtrees(
    Tree * const tree,
    Node * const node1,
    Node * const node2,
    int descendant_node_rule,
    int data_pointer_swap)
{
  if (node1 == NULL) return -2; // use tree_release_child()
  if (node2 == NULL) return -3; // use tree_release_child()

  // Check parents
  Node *parent1 = node1->parent;
  Node *parent2 = node2->parent;

  if (parent1 == parent2
      /* swapping siblings just works for non-root nodes! */
      && parent1 != NULL ){
    return _tree_swap_siblings(tree, node1, node2, 0, data_pointer_swap);
  }

  if (node_is_descendant(node1, node2)){
    if (descendant_node_rule == 1){
      return _tree_swap_subtrees_descendant(tree, node1, node2, NULL, 0, data_pointer_swap);
    }
    if (descendant_node_rule == 2){
      return _tree_swap_subtrees_descendant(tree, node1, node2, NULL, 1, data_pointer_swap);
    }
    return -1; // Operation not allowed
  }
  else if (node_is_descendant(node2, node1)){
    if (descendant_node_rule == 1){
      return _tree_swap_subtrees_descendant(tree, node2, node1, NULL, 0, data_pointer_swap);
    }
    if (descendant_node_rule == 2){
      return _tree_swap_subtrees_descendant(tree, node2, node1, NULL, 1, data_pointer_swap);
    }
    return -1; // Operation not allowed
  }

  // Here, we know that nodes are in different branches.
  // This eliminates most corner cases.
  // We can handle it like the uncritical case of
  // tree_swap_nodes, without applying changes on children.
  return _tree_swap_unconnected_nodes(tree, node1, node2, 0, data_pointer_swap);
}

uint32_t tree_number_of_nodes(
    const Tree * const tree)
{
  // TODO: Caching number(?)
  // There are many operations which invalidates this number?!
  // Das würde aber das Debugging erschweren, wobei ich ja 
  // da dann immer node_number_of_successors nehmen könnte...
  return 1 + node_number_of_successors(tree->root);
}

uint32_t node_number_of_successors(
        const Node * node)
{
  uint32_t n = 0;
  Node *cur = node->child;
  while (cur != NULL) {
    n++;
    if( cur->child != NULL ) cur = cur->child;
    else if( cur->sibling != NULL ) cur = cur->sibling;
    else{
      cur = cur->parent;
      while (cur != NULL) {
        if (cur==node) return n;
        if (cur->sibling) {
          cur=cur->sibling;
          break;
        }
        cur = cur->parent;
      }
    }
  }
  return n;
}

void _tree_print(
    const Tree * tree,
    const Node *node,
    int32_t shift)
{
  int32_t i;
  int32_t shift2=0;
  int32_t id = tree_get_node_id(tree, node);

  //printf(id>99?"<%3i> ":" <%2i>", id);
  //shift2+=6;
  printf(id>99?"◖%3i◗ ":(id>9?"◖■%2i◗":" ◖■%1i■◗"), id);
  shift2+=7;

  // More ident for higher ids(?!)
  if(id>999) shift2 += ((int)log10(id)) - 3;


  if( node->child != NULL){
    printf("→");
    _tree_print(tree, node->child, shift+shift2);
  }else{
    printf("\n");
  }

  if( node->sibling != NULL){
  //  printf("\n");
    for(i=0; i<shift-1; i++) printf(" ");
    printf("↘");
    _tree_print(tree, node->sibling, shift);
  }
}

void tree_print(
    const Tree * tree,
    const Node *subtree_root,
    int32_t shift)
{
  if( subtree_root != NULL ){
    assert(subtree_root >= tree->nodes);
    assert(subtree_root < tree->nodes + tree->size);
  }
  _tree_print(tree, subtree_root?subtree_root:tree->root, shift);
}

/* Generate trees from strings like tree_print(…). 
 *
 * Example input strings:
 *
 * 0 -> 3
 * -------------------------
 * 0 —> 1 —> 2 —> 3 —> 4 —> 5
 *                  —> 8
 *   —> 6 —> 7
 *   —> 9
 * -------------------------
 * ◖■0■◗→ ◖■5■◗→ ◖■2■◗→ ◖■1■◗→ ◖■3■◗→ ◖■4■◗
 *      ↘ ◖■6■◗→ ◖■7■◗          
 *      ↘ ◖■8■◗                 
 *      ↘ ◖■9■◗
 */
Tree *tree_loads(
        const char *tree_input,
        const char **node_formats,
        const char **arrow_formats)
{
  /* Parsing rules:
   *   0)  Indention of leftmost arrow defines parent node.
   *   1)  Nodes with same parent orderd from top to bottom.
   *   2a) Everything betweeen detected arrow is a node.
   *   2b) Node can contain at most one id.
   *   3)  If more than one root node is given, an extra layer will be added.
   */
  return NULL;
}

int32_t tree_get_node_id(
    const Tree *tree,
    const Node *node)
{
  /* Relative to root and modulu
  return (node>=tree->root)?
    ((node - tree->root)% tree->size):
    ((tree->root - node)% tree->size + tree->size);
    */
  assert(node>= tree->nodes);
  assert(node<tree->nodes + tree->size);
  return (node - tree->nodes);
}


Node * tree_left_sibling(
    const Node *n)
{
  if (n == NULL) return NULL;
  if (n->parent == NULL) return NULL;

  Node *sib = n->parent->child;
  while (sib){
    if (sib->sibling == n) return sib;
    sib = sib->sibling;
  }
  return NULL;
}

/* Shortcut for 
 * int err = foobar(…)
 * if (err<0) return err;
 *
 * For debugging:
 *   Insert assert(0) call to find the position where the error occours.
 */
#define RETURN_ON_INTEGRITY_ERR(code) \
{ int _err = code; \
    if (_err<0) { \
      assert(_err == 0); \
      return _err; \
    } \
}

/* Return value:
 *  >= 0: Number of children <= TREE_CHILDREN_MAX
 *  < 0: Error status
 */
int _check_too_many_children(Node * const parent){
  if (parent == NULL) RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_IS_NULL);

  Node *n = parent->child;
  uint32_t num_siblings=0;
  while (n) {
    ++num_siblings;
    if(num_siblings > TREE_MAX_CHILDREN)
      RETURN_ON_INTEGRITY_ERR(TREE_ERROR_TOO_MANY_CHILDREN);
    n=n->sibling;
  }
  return num_siblings;
}

/* Returns 0 if at most TREE_MAX_CHILDREN (right) siblings have same parent.*/
int _check_parent_relation(Node *n){
  Node * const parent = n->parent;
  uint32_t num_siblings=0;
  while (n) {
    ++num_siblings;
    if(num_siblings > TREE_MAX_CHILDREN){
      RETURN_ON_INTEGRITY_ERR(TREE_ERROR_TOO_MANY_CHILDREN);
    }
    if(n->parent != parent){
      RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_HAS_WRONG_PARENT);
    }
    n=n->sibling;
  }
  return 0;
}

/* Searches first entry of a silbing sequence which is 
 * linked as silbing twice. (Thus, looping over silbings never ends).
 *
 * Return value:
 *  ret < 0: Error occoured.
 *  ret = 0: if no duplicate can be found (best case).
 *  ret > 0: cycle length until *pOut will be reached again.
 *
 * Assumptions:
 *   • First TREE_MAX_CHILDREN siblings share same parent (or NULL).
 *
 * Task: Find X with two predecessors N_a, N_b
 *
 *    N_0 →    …  → N_a → X → …
 *  (Non-cycle prelude)   ↑   ↓  (Cycle of length L)
 *                      N_b ← …
 *
 * Solution: 
 *  0. N_0 is start->parent->child.
 *  1. Assume parent-property is constant for input.
 *  2. Loop trough nodes and save predecessor relation in node.parent.
 *  3. If node is reached which was already changed, we found N_b, N_a and X.
 *  4. Reverse loop from N_b to N_0 and restore original parent node.
 *     We we reaching N_a, we also got L.
 *
 *
 *  If N_a is its own sibling, then N_a != X, X = N_b.
 */
int _search_leftmost_silbing_duplicate(
    Node * const node,
    Node **pOut)
{
  *pOut = NULL;
  if (node == NULL) RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_IS_NULL);

  /* Shortcut for trivial case
   * This avoids recursion problem of line (***) for step 4
   * if starting node is its own sibling, node->sibling == node.
   */
  if (node->sibling == node) {
    *pOut = node;
    return 1; // cycle length/distance to first duplicate
  }

  Node * const parent = node->parent; // Backup for step 4. Can be NULL
  Node * const leftmost_child = node->parent?node->parent->child:node;
  Node *N_a = leftmost_child->parent->child;
  Node *N_b = NULL, *X = NULL;
  Node *cur = leftmost_child;
  uint32_t num_siblings=0;
  leftmost_child->parent = NULL; // Endpoint of step 4
  while(cur->sibling) {
    ++num_siblings;
    if(num_siblings > TREE_MAX_CHILDREN){
      RETURN_ON_INTEGRITY_ERR(TREE_ERROR_TOO_MANY_CHILDREN);
    }
    if( cur->sibling->parent != parent ){
      // Step 3
      N_a = cur->sibling->parent;
      X = cur->sibling;
      N_b = cur;
      break;
    }
    // Step 2
    cur->sibling->parent = cur; // Save left sibling, (***)
    cur=cur->sibling;
  }
  // Step 4
  int cycle_len=0;
  {
    assert(cur == N_b);
    _unused(N_b);
    // Reset values in cycle
    while(cur) {
      Node *tmp_left=cur->parent;
      cur->parent = parent;
      cur = tmp_left;
      ++cycle_len;

      if (cur == N_a){
        break;
      }
    }
    // Reset values in prelude.
    assert(cur == N_a);
    while(cur) {
      Node *tmp_left=cur->parent;
      cur->parent = parent;
      cur = tmp_left;
    }
  }

  *pOut = X;
  return cycle_len;
}

int _integrity_check_for_leftmost_nodes(
    Node * const node)
{
  /* Non-root nodes have always a parent */
  if( node->parent == NULL)
    RETURN_ON_INTEGRITY_ERR(TREE_ERROR_PARENT_NODE_IS_NULL);

  /* Check up level reference.
   *
   *  ↗ C_1, …, C_n
   * P
   *  ↘ C'_1, …, C'_m
   */
  if(node != node->parent->child)
    RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_IS_PARENT_FOR_NONSIBLING);

  // Count siblings of 'node'
  int num_siblings_or_err = _check_too_many_children(node->parent);

  // Siblings share same parent.
  int err_parent_relation = _check_parent_relation(node);

  if (num_siblings_or_err < 0 && !err_parent_relation){
    // Check why it's too many children. Just too many elements or
    // a cycle?!
    Node *duplicate;
    int duplicate_err = _search_leftmost_silbing_duplicate(node, &duplicate);
    if (duplicate_err) {
      return duplicate_err;
    }
    return num_siblings_or_err;
  }

  if (err_parent_relation) return err_parent_relation;


#ifdef TREE_REDUNDANT_INFOS
  if (num_siblings_or_err >= 0){
    if (node->parent->width != num_siblings_or_err) {
      RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_WIDTH_WRONG);
    }
    if (node->parent->width > TREE_MAX_CHILDREN) {
      RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_WIDTH_EXCEEDED);
    }
  }
#endif

  return 0;
}

int _compare_height_of_parent_and_its_children(
    Node * const parent)
{
  uint32_t max_height = 0;
  Node *cur = parent->child;
  while(cur){
    if (max_height < cur->height + 1){
      max_height = cur->height + 1;
    }
    cur = cur->sibling;
  }
  if (max_height != parent->height){
    RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_HEIGHT_WRONG);
  }

  if (max_height > TREE_MAX_HEIGHT){
    RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_HEIGHT_EXCEEDED);
  }

  return 0;
}

int _integrity_check_for_rightmost_nodes(
    Node * const node)
{
#ifdef TREE_REDUNDANT_INFOS
  //Here, all child nodes node->parent were
  //already checked. We can rely on their ->height value.
  RETURN_ON_INTEGRITY_ERR(_compare_height_of_parent_and_its_children(node->parent));

  return 0;
#endif
}

int _leaf_integrety_check(
    Node * const leaf,
    uint32_t depth)
{
  assert(leaf->child == NULL);
#ifdef TREE_REDUNDANT_INFOS
  if (leaf->width != 0)
    RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_WIDTH_WRONG);
  if (leaf->height != 0)
    RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_HEIGHT_WRONG);
#endif

  return 0;
}

int tree_integrity_check(
    Node *node)
{
  if (node == NULL) RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_IS_NULL);
  if (node->parent == NULL && node->sibling){
    RETURN_ON_INTEGRITY_ERR(TREE_ERROR_ROOT_HAS_SIBLING);
  }

  /* With this variable we track if the complete tree
   * exceeded the maximal depth.
   * This counting is orthogonal to (optional) Node.height values.
   */
  uint32_t depth = 0;

  // If we starting in a subtree add the actual depth.
  Node *p = node->parent;
  while(p && depth < TREE_MAX_HEIGHT){
    ++depth;
    p = node->parent;
  }
  if( depth >= TREE_MAX_HEIGHT ){
    RETURN_ON_INTEGRITY_ERR(TREE_ERROR_TOO_MANY_LEVELS); // or cycle in parent-relation
  }
  const uint32_t start_node_depth = depth;

  Node *cur = node->child;
  ++depth;
  while(cur){
    // Checks on leftmost children 
    if( cur == cur->parent->child ){
      RETURN_ON_INTEGRITY_ERR(_integrity_check_for_leftmost_nodes(cur));
    }

#ifdef TREE_REDUNDANT_INFOS
    // Other checks.
    if (cur->height > TREE_MAX_HEIGHT)
      RETURN_ON_INTEGRITY_ERR(TREE_ERROR_TOO_MANY_LEVELS);
    if (cur->height + 1 > cur->parent->height)
      RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_HEIGHT_WRONG);
#endif

    // Find next node
    if(cur->child){
      cur = cur->child;

      ++depth;
      if(depth >= TREE_MAX_HEIGHT)
        RETURN_ON_INTEGRITY_ERR(TREE_ERROR_TOO_MANY_LEVELS);

      continue;
    }

    // Here, cur has to be a leaf
    RETURN_ON_INTEGRITY_ERR(_leaf_integrety_check(cur, depth));

    if(cur->sibling){
      cur = cur->sibling;
      continue;
    }

    // Go upwards
    while(cur->parent){
      RETURN_ON_INTEGRITY_ERR(_integrity_check_for_rightmost_nodes(cur));

      cur = cur->parent;

      --depth;
      uint32_t relative_depth = depth - start_node_depth; // Check for underflow
      assert(relative_depth <= TREE_MAX_HEIGHT); // Failing indicates error in this function, but probably not tree structure.
      _unused(relative_depth);

      if(cur == node){ // Loop has visited all nodes of (sub-)tree.
        cur = NULL;
        break; 
      }
      if(cur->sibling){
        cur = cur->sibling;
        break;
      }
    }
  }

  return 0;
}

void tree_print_integrity_check(
    Node *root)
{
  int err = tree_integrity_check(root);
  if (err == 0){
    fprintf(stderr, "Tree has integrity. :-)\n");
    return;
  }

  // Print error message
  fprintf(stderr, "Tree does not have integrity!\n Error: ");
  fprintf_enum_name(stderr, err);
  fprintf(stderr, "\n");

  /*
  switch( err ) {
    case TREE_ERROR_ROOT_HAS_SIBLING:
      fprintf(stderr,
          "Error %d: Tree contains node with too many children.\n",
          err);
      break;
    case TREE_ERROR_NODE_IS_NULL:
      fprintf(stderr,
          "Error %d: Unexpected NULL reference.\n", 
          err);
      break;
    default:
      fprintf(stderr, "Tree does not have integrity. Error: %d",
          err);
      break;
  }
  */
}
