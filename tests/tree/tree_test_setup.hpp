#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "blobdetection/tree.h"
#include "tree_loops.h"
}

int child_compare_func1(const Node *n1, const Node *n2, void *compare_data) {
	int dw = n1->width - n2->width;
	int dh = n1->height - n2->height;
	if (dw) return dw;
	if (dh) return dh;
	return 0;
}

int parent_compare_func1(const Node *n1, const Node *n2, void *compare_data) {
	return child_compare_func1(n1->parent, n2->parent, compare_data);
}

void init_tree_structure1(Tree *t){
	/* 0 —> 9
	*/
	tree_add_child(&t->nodes[0], &t->nodes[9]);
}

void init_tree_structure2(Tree *t){
	/* 0 —> 1
	 * |
	 *  ——> 2
	 */
	tree_add_child(&t->nodes[0], &t->nodes[1]);
	tree_add_child(&t->nodes[0], &t->nodes[2]);
}

void init_tree_structure3(Tree *t){
	/* 0 —> 2
	 * |
	 *  ——> 1
	 */
	tree_add_child(&t->nodes[0], &t->nodes[2]);
	tree_add_child(&t->nodes[0], &t->nodes[1]);
}

void init_tree_structure4(Tree *t){
	/* 0 —> 1 —> 2
	 * |
	 *  ——> 3 —> 4
	 *      |
	 *       ——> 5
	 */
	tree_add_child(&t->nodes[0], &t->nodes[1]);
	tree_add_child(&t->nodes[0], &t->nodes[3]);
	tree_add_child(&t->nodes[1], &t->nodes[2]);
	tree_add_child(&t->nodes[3], &t->nodes[4]);
	tree_add_child(&t->nodes[3], &t->nodes[5]);
}

void init_tree_structure5(Tree *t){
	/* 0 —> 1 —> 2
	 * |    |
	 * |     ——> 3
	 * |
	 *  ——> 4 —> 5
	 */
	tree_add_child(&t->nodes[0], &t->nodes[1]);
	tree_add_child(&t->nodes[0], &t->nodes[4]);
	tree_add_child(&t->nodes[1], &t->nodes[2]);
	tree_add_child(&t->nodes[1], &t->nodes[3]);
	tree_add_child(&t->nodes[4], &t->nodes[5]);

}

void init_tree_structure6(Tree *t){
	/* 0 —> 1 —> 2
	 * |    |
	 * |     ——> 3
	 * |
	 *  ——> 4 —> 5
	 *      |
	 *       ——> 6
	 */
	tree_add_child(&t->nodes[0], &t->nodes[1]);
	tree_add_child(&t->nodes[0], &t->nodes[4]);
	tree_add_child(&t->nodes[1], &t->nodes[2]);
	tree_add_child(&t->nodes[1], &t->nodes[3]);
	tree_add_child(&t->nodes[4], &t->nodes[5]);
	tree_add_child(&t->nodes[4], &t->nodes[6]);
}

void init_tree_structure7(Tree *t){
	/* 0 —> 1 —> 2 —> 8
	 * |    |
	 * |     ——> 3
	 * |
	 *  ——> 4 —> 5
	 *      |
	 *       ——> 6
	 *      |
	 *       ——> 7
	 */
	tree_add_child(&t->nodes[0], &t->nodes[1]);
	tree_add_child(&t->nodes[0], &t->nodes[4]);
	tree_add_child(&t->nodes[1], &t->nodes[2]);
	tree_add_child(&t->nodes[1], &t->nodes[3]);
	tree_add_child(&t->nodes[4], &t->nodes[5]);
	tree_add_child(&t->nodes[4], &t->nodes[6]);
	tree_add_child(&t->nodes[4], &t->nodes[7]);
	tree_add_child(&t->nodes[2], &t->nodes[8]);
}

void init_tree_structure8(Tree *t){
	/* 0 —> 1 —> 2 —> 8
	 * |    |
	 * |     ——> 3
	 * |
	 *  ——> 4 —> 5
	 * |    |
	 * |     ——> 6
	 * |    |
	 * |     ——> 7
	 * |
	 *  ——> 9
	 */
	tree_add_child(&t->nodes[0], &t->nodes[1]);
	tree_add_child(&t->nodes[0], &t->nodes[4]);
	tree_add_child(&t->nodes[1], &t->nodes[2]);
	tree_add_child(&t->nodes[1], &t->nodes[3]);
	tree_add_child(&t->nodes[4], &t->nodes[5]);
	tree_add_child(&t->nodes[4], &t->nodes[6]);
	tree_add_child(&t->nodes[4], &t->nodes[7]);
	tree_add_child(&t->nodes[2], &t->nodes[8]);
	tree_add_child(&t->nodes[0], &t->nodes[9]);
}

void init_tree_structure8B(Tree *t){
	/* 0 —> 9
	 * |
	 *  ——> 4 —> 7
	 * |    |
	 * |     ——> 6
	 * |    |
	 * |     ——> 5
	 * |
	 *  ——> 1 —> 2 —> 8
	 *      |
	 *       ——> 3
	 */
	tree_add_child(&t->nodes[0], &t->nodes[9]);
	tree_add_child(&t->nodes[0], &t->nodes[4]);
	tree_add_child(&t->nodes[4], &t->nodes[7]);
	tree_add_child(&t->nodes[4], &t->nodes[6]);
	tree_add_child(&t->nodes[4], &t->nodes[5]);
	tree_add_child(&t->nodes[0], &t->nodes[1]);
	tree_add_child(&t->nodes[1], &t->nodes[2]);
	tree_add_child(&t->nodes[1], &t->nodes[3]);
	tree_add_child(&t->nodes[2], &t->nodes[8]);
}

void gen_invalid_tree_structure1(Tree *t){
	/* 0 ———
	 * ^    |
	 * |————
	 */
	tree_add_child(&t->nodes[0], &t->nodes[1]);
	tree_add_child(&t->nodes[1], &t->nodes[2]);
	tree_add_child(&t->nodes[2], &t->nodes[7]);
	tree_add_child(&t->nodes[7], &t->nodes[0]);
	tree_add_child(&t->nodes[1], &t->nodes[3]);
}

void gen_invalid_tree_structure2(Tree *t){
	/* 0 —> 1 —> 2 —> 7
	 * ^    |         |
	 * |——————————————
	 * |    |
	 * |     ——> 3
	 */
	tree_add_child(&t->nodes[0], &t->nodes[1]);
	tree_add_child(&t->nodes[1], &t->nodes[2]);
	tree_add_child(&t->nodes[2], &t->nodes[7]);
	tree_add_child(&t->nodes[7], &t->nodes[0]);
	tree_add_child(&t->nodes[1], &t->nodes[3]);
}
