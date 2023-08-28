#ifndef TREE_LOOPS_H
#define TREE_LOOPS_H

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

//#include "enums.h"

#include "blobdetection/tree.h"

#ifndef INLINE
#define UNDEF_INLINE
#ifdef VISUAL_STUDIO
//#define INLINE extern __inline 
#define INLINE extern __inline __forceinline
#else
//#define INLINE extern inline
#define INLINE extern inline __attribute__((always_inline))
// This throws an error at compile time if no extern variant exists
// (=> Inlining failed at a position it shouldn't)
#endif
#endif

/*
 * Return value: Status code of abort condition:
 *  > 0 : Handler returned value != 0. If more details needed,
 *                                     write them into data.
 *  = 0 : All nodes visited.
 *  < 0 : Error code
 */
INLINE
int _tree_depth_first_search(
        Tree * const tree,
        node_func_t * const on_leaf, // Same event in Pre-, In- and Post-Order
        child_node_func_t * const on_non_leaf_pre_order,
        child_node_func_t * const on_non_leaf_in_order,
        child_node_func_t * const on_non_leaf_post_order,
        void *data);

/* Traversal over two trees of the
 * same structure.
 * Function aborts if differnce is detetced in tree structures 
 * or one of the handler returns != 0.
 *
 * Return value: Status code of abort condition:
 *  -1: tree1 != NULL, tree2 == NULL
 *   1: tree2 != NULL, tree1 == NULL
 *  -2: tree1 has more nodes.
 *   2: tree2 has more nodes.
 *  -3: tree1 is inconsistend (loop never reached root node)
 *   3: tree2 is inconsistent (loop never reached root node)
 *  > 0 : Handler returned positive value
 *  = 0 : All nodes visited.
 *  < -1024 : Error code
 */
INLINE
int _trees_depth_first_search(
        Tree * const tree1,
        Tree * const tree2,
        node2_func_t * const on_leaf, // Same event in Pre-, In- and Post-Order
        child_node2_func_t * const on_non_leaf_pre_order,
        child_node2_func_t * const on_non_leaf_in_order,
        child_node2_func_t * const on_non_leaf_post_order,
        void *data);


// =========================================

INLINE
int _tree_depth_first_search(
        Tree * const tree,
        node_func_t * const on_leaf, // Same event in Pre-, In- and Post-Order
        child_node_func_t * const on_non_leaf_pre_order,
        child_node_func_t * const on_non_leaf_in_order,
        child_node_func_t * const on_non_leaf_post_order,
        void *data)
{

  if (tree == NULL) return -1;
  assert(tree != NULL);

  const Node * const root = tree->root;
  const Node *cur = root;
  // Loop through tree until an handler indicates an hit/error.
  while(cur) {

    if (cur->child) {
      // Pre-Order-Positon, non-leafs
      // Event for cur->parent, but argument is cur!
      if (on_non_leaf_pre_order != NULL) {
        if (on_non_leaf_pre_order(cur->child, data)) return 1;
      }

      // Alfter on_non_leaf_in_order call to allow child-resort before.
      cur = cur->child;
      continue;
    }

    // (Pre)+In+Post-Order-Position, leafs
    if (on_leaf != NULL) {
      if (on_leaf(cur, data)) return 1;
    }

    if (cur->sibling) {
      // In-Order-Postion for parent of leafs (cur->parent). Note that
      // it will be called multiple times if num_childs(cur->parent) > 2.
      if (on_non_leaf_in_order != NULL) {
        // Event for cur->parent, but argument is cur!
        if (on_non_leaf_in_order(cur, data)) return 1;
      }

      cur = cur->sibling;
      continue;
    }

    while(cur!=root){
      // Here, cur is always the rightmost child.

      // Post-Order-Position for non-leafs (cur->parent)
      if (on_non_leaf_post_order != NULL) {
        // Event for cur->parent, but argument is cur!
        if (on_non_leaf_post_order(cur, data)) return 1;
      }

      cur=cur->parent;
      if(cur==NULL){ // tree is inconsistent if we reach NULL before root
        assert(cur!=NULL);
        return -3;
      }

      if (cur->sibling) {
        // In-Order-Postion for parent of non-leafs (cur->parent).
        // Note that it will be called multiple times if num_childs(cur->parent) > 2.
        if (on_non_leaf_in_order != NULL) {
          // Event for cur->parent, but argument is cur!
          if (on_non_leaf_in_order(cur, data)) return 1;
        }

        cur=cur->sibling;
        break;
      }
    }
  }
  return 0;
}


INLINE
int _trees_depth_first_search(
        Tree * const tree1,
        Tree * const tree2,
        node2_func_t * const on_leaf, // Same event in Pre-, In- and Post-Order
        child_node2_func_t * const on_non_leaf_pre_order,
        child_node2_func_t * const on_non_leaf_in_order,
        child_node2_func_t * const on_non_leaf_post_order,
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
  const Node *cur1 = root1;
  const Node *cur2 = root2;
  // Loop through tree1 + tree2 and until handler returned an hit/error
  // or the structure of both trees doesn't match anymore

  if (cur1 && cur2==NULL) // tree1 has more nodes on certain place
    return -2;

  while(cur1) {
    // Commented out because the NULL checks are already done before loop restarts
    //if (cur2==NULL) // tree1 has more nodes on certain place
    //  return -2;

    if (cur1->child) {
      if (cur2->child==NULL) { // tree1 has more nodes on certain place
        return -12; //Child missing
      }

      // Pre-Order-Positon, non-leafs
      // Event for cur1->parent, but argument is cur1!
      if (on_non_leaf_pre_order != NULL) {
        int32_t d = on_non_leaf_pre_order(cur1->child, cur2->child, data);
        if (d) return d;
      }

      // Alfter on_non_leaf_in_order call to allow child-resort before.
      cur1 = cur1->child;
      cur2 = cur2->child;
      assert(cur1!= NULL && cur2 != NULL); // guaranteed due previous if's.
      continue;
    }else if (cur2->child!=NULL) { // tree2 has more nodes on certain place
      return 12; //Child missing
    }

    // (Pre)+In+Post-Order-Position, leafs
    if (on_leaf != NULL) {
      int32_t d = on_leaf(cur1, cur2, data);
      if (d) return d;
    }

    if (cur1->sibling) {
      if (cur2->sibling==NULL) { // tree1 has more nodes on certain place
        return -13; //Sibling missing
      }

      // In-Order-Postion for parent of leafs (cur1->parent). Note that
      // it will be called multiple times if num_childs(cur1->parent) > 2.
      if (on_non_leaf_in_order != NULL) {
        // Event for cur1->parent, but argument is cur1!
        int32_t d = on_non_leaf_in_order(cur1, cur2, data);
        if (d) return d;
      }

      cur1 = cur1->sibling;
      cur2 = cur2->sibling;
      assert(cur1!= NULL && cur2 != NULL); // guaranteed due previous if's.
      continue;
    }else if (cur2->sibling!=NULL) { // tree2 has more nodes on certain place
      return 13; //Sibling missing
    }

    while(cur1){
      // Here, cur1 is always the rightmost child or root1.
      // We need to check if its root1 to prevent call of on_non_leaf_post_order
      // for cur1->parent == NULL.
      if(cur1->parent){
        if (cur2->parent==NULL) { // tree2 ending too early + is inconsistent
          return -14; //Sibling missing
        }

        // Post-Order-Position for non-leafs (cur1->parent)
        if (on_non_leaf_post_order != NULL) {
          // Event for cur1->parent, but argument is cur1!
          int32_t d = on_non_leaf_post_order(cur1, cur2, data);
          if (d) return d;
        }
        cur1=cur1->parent;
        cur2=cur2->parent;
        assert(cur1!= NULL && cur2 != NULL); // guaranteed due previous if's.
      }else {
        if(cur2->parent){ // tree1 is inconsistent if we reach NULL before tree2 ends 
          assert(cur1!=NULL);
          return 14;
        }
        cur1=cur1->parent;
        cur2=cur2->parent;
        // Here, NULL is a possible value,
        if( cur1== NULL){
          break;
        }
      }

      if (cur1->sibling){
        if (cur2->sibling==NULL) { // tree1 has more nodes on certain place
          return -13; //Sibling missing
        }
        // In-Order-Postion for parent of non-leafs (cur1->parent).
        // Note that it will be called multiple times if num_childs(cur1->parent) > 2.
        if (on_non_leaf_in_order != NULL) {
          // Event for cur1->parent, but argument is cur1!
          int32_t d = on_non_leaf_in_order(cur1, cur2, data);
          if (d) return d;
        }

        cur1=cur1->sibling;
        cur2=cur2->sibling;
        assert(cur1!= NULL && cur2 != NULL); // guaranteed due previous if's.
        break;
      }else if (cur2->sibling!=NULL) { // tree2 has more nodes on certain place
        return 13; //Sibling missing
      }

    }
  }

  if (cur2 != NULL) // => Extra node
    return 2;

  return 0;
}

#ifdef UNDEF_INLINE
#undef INLINE
#undef UNDEF_INLINE
#endif

#endif
