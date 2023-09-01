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
 *            *out_label will be free'd, even if the function will fail.
 */
int tree_sort_canonical_order_inplace(
        Tree *tree,
        char **out_label);

/* 
 * Like tree_canonical_order_inplace,
 * but ordered tree will retuned as copy of the given tree.
 * */
int tree_sort_canonical_order(
        const Tree *tree,
        Tree **out_tree,
        char **out_label);

/*
 * This just returns the canonical label but does not
 * sort the given input.
 *
 * Delete returned char after usage. */
char *tree_canonical_label(
        const Tree *tree);
#endif
