#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

#include "../tree_test_setup.hpp"

// Tests of tree_swap_nodes() function

//Naming shortcut
#define n1 (t1->nodes)
#define n2 (t2->nodes)
#define c1 (t1clone->nodes)
int verbose = 0;

TEST_GROUP(TestSwapNodes) {
	Tree *t1, *t2 = NULL;
	void setup() {
		t1 = tree_create(10, 1); // Inits nodes as Leafs
		t2 = tree_create(10, 1);
	}

	void teardown() {
		tree_destroy(&t1);
		tree_destroy(&t2);
	}

};


TEST(TestSwapNodes, Test_for_direct_related_nodes) {
	int print_tree = verbose;

	/* Generate following structuere:
	 * 0 —> 1 —> 3
	 *   —> 2
	 */
  tree_add_child(&n1[0], &n1[1]);
  tree_add_child(&n1[0], &n1[2]);
  tree_add_child(&n1[1], &n1[3]);
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}

	uint32_t num_nodes = tree_number_of_nodes(t1);
	// Copy t1 to check if swapped arguments leads to same result.
	Tree *t1clone = tree_clone(t1, NULL, NULL);

	// Swap nodes with different parents
	LONGS_EQUAL(0, tree_swap_nodes(t1, &n1[1], &n1[3], 1, 1));
	LONGS_EQUAL(0, tree_swap_nodes(t1clone, &c1[3], &c1[1], 1, 1));
	/* Expected result after operation:
	 * 0 —> 3 —> 1
	 *   —> 2
	 */
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}
	LONGS_EQUAL(num_nodes, tree_number_of_nodes(t1));
	LONGS_EQUAL(0, tree_cmp(t1, t1clone, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));

	// Swap nodes with same parents
	LONGS_EQUAL(0, tree_swap_nodes(t1, &n1[3], &n1[2], 1, 1));
	LONGS_EQUAL(0, tree_swap_nodes(t1clone, &c1[2], &c1[3], 1, 1));
	/* Expected result after operation:
	 * 0 —> 2 —> 1
	 *   —> 3
	 */
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}
	LONGS_EQUAL(num_nodes, tree_number_of_nodes(t1));
	LONGS_EQUAL(0, tree_cmp(t1, t1clone, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));

	// Check structure
	POINTERS_EQUAL(  NULL, n1[0].parent);
	POINTERS_EQUAL(&n1[2], n1[0].child);
	POINTERS_EQUAL(  NULL, n1[0].sibling);
	POINTERS_EQUAL(&n1[2], n1[1].parent);
	POINTERS_EQUAL(  NULL, n1[1].sibling);
	POINTERS_EQUAL(  NULL, n1[1].child);
	POINTERS_EQUAL(&n1[0], n1[2].parent);
	POINTERS_EQUAL(&n1[3], n1[2].sibling);
	POINTERS_EQUAL(&n1[1], n1[2].child);
	POINTERS_EQUAL(&n1[0], n1[3].parent);
	POINTERS_EQUAL(  NULL, n1[3].sibling);
	POINTERS_EQUAL(  NULL, n1[3].child);

	tree_destroy(&t1clone);
}

TEST(TestSwapNodes, Test_for_indirect_related_nodes) {
	int print_tree = verbose;

	/* Generate following structuere:
	 * 0 —> 1 —> 2 —> 3 —> 4 —> 5
	 *   —> 6 —> 7
	 *   —> 8
	 *   —> 9
	 */
  tree_add_child(&n1[0], &n1[1]);
  tree_add_child(&n1[1], &n1[2]);
  tree_add_child(&n1[2], &n1[3]);
  tree_add_child(&n1[3], &n1[4]);
  tree_add_child(&n1[4], &n1[5]);
  tree_add_child(&n1[0], &n1[6]);
  tree_add_child(&n1[6], &n1[7]);
  tree_add_child(&n1[0], &n1[8]);
  tree_add_child(&n1[0], &n1[9]);
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}

	uint32_t num_nodes = tree_number_of_nodes(t1);
	// Copy t1 to check if swapped arguments leads to same result.
	Tree *t1clone = tree_clone(t1, NULL, NULL);

	// Swap nodes on same branch, one node between.
	LONGS_EQUAL(0, tree_swap_nodes(t1, &n1[1], &n1[3], 1, 1));
	LONGS_EQUAL(0, tree_swap_nodes(t1clone, &c1[3], &c1[1], 1, 1));
	/* Expected result after operation:
	 * 0 —> 3 —> 2 —> 1 —> 4 —> 5
	 *   —> 6 —> 7
	 *   —> 8
	 *   —> 9
	 */
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}
	LONGS_EQUAL(num_nodes, tree_number_of_nodes(t1));
	LONGS_EQUAL(0, tree_cmp(t1, t1clone, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));

	// Swap nodes on same branch, two nodes between
	LONGS_EQUAL(0, tree_swap_nodes(t1, &n1[3], &n1[4], 1, 1));
	LONGS_EQUAL(0, tree_swap_nodes(t1clone, &c1[4], &c1[3], 1, 1));
	/* Expected result after operation:
	 * 0 —> 4 —> 2 —> 1 —> 3 —> 5
	 *   —> 6 —> 7
	 *   —> 8
	 *   —> 9
	 */
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}
	LONGS_EQUAL(num_nodes, tree_number_of_nodes(t1));
	LONGS_EQUAL(0, tree_cmp(t1, t1clone, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));

	// Swap nodes on same branch, tree nodes between
	LONGS_EQUAL(0, tree_swap_nodes(t1, &n1[4], &n1[5], 1, 1));
	LONGS_EQUAL(0, tree_swap_nodes(t1clone, &c1[5], &c1[4], 1, 1));
	/* Expected result after operation:
	 * 0 —> 5 —> 2 —> 1 —> 3 —> 4
	 *   —> 6 —> 7
	 *   —> 8
	 *   —> 9
	 */
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}
	LONGS_EQUAL(num_nodes, tree_number_of_nodes(t1));
	LONGS_EQUAL(0, tree_cmp(t1, t1clone, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));

	// Swap nodes on different branches, both with childs.
	LONGS_EQUAL(0, tree_swap_nodes(t1, &n1[3], &n1[6], 1, 1));
	LONGS_EQUAL(0, tree_swap_nodes(t1clone, &c1[6], &c1[3], 1, 1));
	/* Expected result after operation:
	 * 0 —> 5 —> 2 —> 1 —> 6 —> 4
	 *   —> 3 —> 7
	 *   —> 8
	 *   —> 9
	 */
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}
	LONGS_EQUAL(num_nodes, tree_number_of_nodes(t1));
	LONGS_EQUAL(0, tree_cmp(t1, t1clone, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));

	// Finally, test swap of root node
	// Here, we has to update the root information manually
	// because called function had no acces on Tree struct...
	LONGS_EQUAL(0, tree_swap_nodes(t1, &n1[0], &n1[9], 1, 1));
	LONGS_EQUAL(0, tree_swap_nodes(t1clone, &c1[9], &c1[0], 1, 1));
	tree_set_root(t1, &n1[9]);
	tree_set_root(t1clone, &c1[9]);
	/* Expected result after operation:
	 * 9 —> 5 —> 2 —> 1 —> 6 —> 4
	 *   —> 3 —> 7
	 *   —> 8
	 *   —> 0 
	 */
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}
	LONGS_EQUAL(num_nodes, tree_number_of_nodes(t1));
	LONGS_EQUAL(0, tree_cmp(t1, t1clone, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));

	// Check structure
	POINTERS_EQUAL(&n1[9], n1[0].parent);
	POINTERS_EQUAL(  NULL, n1[0].sibling);
	POINTERS_EQUAL(  NULL, n1[0].child);
	POINTERS_EQUAL(&n1[2], n1[1].parent);
	POINTERS_EQUAL(  NULL, n1[1].sibling);
	POINTERS_EQUAL(&n1[6], n1[1].child);
	POINTERS_EQUAL(&n1[5], n1[2].parent);
	POINTERS_EQUAL(  NULL, n1[2].sibling);
	POINTERS_EQUAL(&n1[1], n1[2].child);
	POINTERS_EQUAL(&n1[9], n1[3].parent);
	POINTERS_EQUAL(&n1[8], n1[3].sibling);
	POINTERS_EQUAL(&n1[7], n1[3].child);
	POINTERS_EQUAL(&n1[6], n1[4].parent);
	POINTERS_EQUAL(  NULL, n1[4].sibling);
	POINTERS_EQUAL(  NULL, n1[4].child);
	POINTERS_EQUAL(&n1[9], n1[5].parent);
	POINTERS_EQUAL(&n1[3], n1[5].sibling);
	POINTERS_EQUAL(&n1[2], n1[5].child);
	POINTERS_EQUAL(&n1[1], n1[6].parent);
	POINTERS_EQUAL(  NULL, n1[6].sibling);
	POINTERS_EQUAL(&n1[4], n1[6].child);
	POINTERS_EQUAL(&n1[3], n1[7].parent);
	POINTERS_EQUAL(  NULL, n1[7].sibling);
	POINTERS_EQUAL(  NULL, n1[7].child);
	POINTERS_EQUAL(&n1[9], n1[8].parent);
	POINTERS_EQUAL(&n1[0], n1[8].sibling);
	POINTERS_EQUAL(  NULL, n1[8].child);
	POINTERS_EQUAL(  NULL, n1[9].parent);
	POINTERS_EQUAL(  NULL, n1[9].sibling);
	POINTERS_EQUAL(&n1[5], n1[9].child);

	tree_destroy(&t1clone);
}

TEST(TestSwapNodes, Test_child_node_rule_fail) {
	int print_tree = verbose;

	/* Generate following structuere:
	 * 0 —> 1 —> 3
	 *   —> 2
	 */
  tree_add_child(&n1[0], &n1[1]);
  tree_add_child(&n1[0], &n1[2]);
  tree_add_child(&n1[1], &n1[3]);

  // Operation not allowed because n1[3] is direct child of n1[1]
	LONGS_EQUAL(-1, tree_swap_nodes(t1, &n1[1], &n1[3], 0, 0));
}

TEST(TestSwapNodes, Test_for_non_related_nodes) {
}

//=================================================


int main(int ac, char** av)
{
	if (ac>1){
		verbose = 1;
	}
	return CommandLineTestRunner::RunAllTests(ac, av);
}
