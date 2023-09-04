#ifndef TREE_SORT_H
#define TREE_SORT_H

#include <limits.h>
#include <stddef.h>

#include "blobdetection/enums.h"
#include "blobdetection/tree.h"

#define LEAF_CHAR 'o'
#define LEFT_NON_LEAF_CHAR '('
#define RIGHT_NON_LEAF_CHAR ')'


/* Bring all isomorph trees into same order.
 *
 * out_label: The label will represent the order of the nodes.
 *            All isomorph trees have the same label.
 *
 *            Free out_label after usage!
 */
int tree_sort_canonical_order_inplace(
        Tree *tree,
        char **out_label,
        size_t *out_label_size);

/* 
 * Like tree_canonical_order_inplace,
 * but ordered tree will returned as copy of the given tree.
 *
 *            Free out_tree after usage!
 *            Free out_label after usage!
 * */
int tree_sort_canonical_order(
        const Tree *tree,
        Tree **out_tree,
        char **out_label,
        size_t *out_label_size);


/* 
 * Like tree_canonical_order,
 * but label will respect data pointer.
 *
 * The returned label is no string, because it can contain null-bytes!
 *
 *            Free out_label after usage!
 * */
int tree_sort_by_data_pointer_inplace(
        Tree *tree,
        char **out_label,
        size_t *out_label_size);

/* 
 * Like tree_canonical_order_inplace,
 * but ordered tree will returned as copy of the given tree.
 *
 *            Free out_tree after usage!
 *            Free out_label after usage!
 * */
int tree_sort_by_data_pointer(
        const Tree *tree,
        Tree **out_tree,
        char **out_label,
        size_t *out_label_size);


/* Avoids long labels of nodes due hashing of them. Could be faster for
 * wide/depth trees.
 * Collisions will not be tracked.
 *
 * Isomorph trees will generate the same hash.
 */
int tree_sort_by_hash_inplace(
        Tree *tree,
        char **out_label,
        size_t *out_label_size);

int tree_sort_by_hash(
        const Tree *tree,
        Tree **out_tree,
        char **out_label,
        size_t *out_label_size);

/* 
 * Like tree_sort_by_hash_data_inplace, but label will respect data pointer.
 * Note that not the content of node->data will be hashed but the pointer value!
 *
 *            Free out_label after usage!
 * */
int tree_sort_by_hash_data_inplace(
        Tree *tree,
        char **out_label,
        size_t *out_label_size);

/* 
 * Like tree_sort_by_hash_data_inplace,
 * but ordered tree will returned as copy of the given tree.
 *
 *            Free out_tree after usage!
 *            Free out_label after usage!
 * */
int tree_sort_by_hash_data(
        const Tree *tree,
        Tree **out_tree,
        char **out_label,
        size_t *out_label_size);

#endif
