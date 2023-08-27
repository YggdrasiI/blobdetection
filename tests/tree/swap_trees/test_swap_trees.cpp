#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

#include "../tree_test_setup.hpp"

// Tests of tree_swap_subtrees() function

//Naming shortcut
#define n1 (t1->nodes)
#define n2 (t2->nodes)
#define c1 (t1clone->nodes)
int verbose = 0;

TEST_GROUP(TestSwapTrees) {
	Tree *t1, *t2 = NULL;
	void setup() {
		t1 = tree_create(10, 1); // Inits nodes as Leafs
		t2 = tree_create(10, 1);
	}

	void teardown() {
		tree_destroy(&t1);
		tree_destroy(&t2);
	}

	void gen2(Tree *t) {
		/* Generate following structuere:
		 * 0 —> 1 —> 2 —> 3 —> 4 —> 5
		 *                  —> 8
		 *   —> 6 —> 7
		 *   —> 9
		 */
		tree_add_child(&t->nodes[0], &t->nodes[1]);
		tree_add_child(&t->nodes[1], &t->nodes[2]);
		tree_add_child(&t->nodes[2], &t->nodes[3]);
		tree_add_child(&t->nodes[3], &t->nodes[4]);
		tree_add_child(&t->nodes[4], &t->nodes[5]);
		tree_add_child(&t->nodes[0], &t->nodes[6]);
		tree_add_child(&t->nodes[6], &t->nodes[7]);
		tree_add_child(&t->nodes[3], &t->nodes[8]);
		tree_add_child(&t->nodes[0], &t->nodes[9]);
	}
};


TEST(TestSwapTrees, Direct_related_nodes) {
	int print_tree = verbose;
	int rule = 1; // descendant_node_rule

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
	LONGS_EQUAL(0, tree_swap_subtrees(t1, &n1[1], &n1[3], rule, 1));
	LONGS_EQUAL(0, tree_swap_subtrees(t1clone, &c1[3], &c1[1], rule, 1));
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
	LONGS_EQUAL(0, tree_swap_subtrees(t1, &n1[3], &n1[2], rule, 1));
	LONGS_EQUAL(0, tree_swap_subtrees(t1clone, &c1[2], &c1[3], rule, 1));
	/* Expected result after operation:
	 * 0 —> 2
	 *   —> 3 —> 1
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
	POINTERS_EQUAL(&n1[3], n1[1].parent);
	POINTERS_EQUAL(  NULL, n1[1].sibling);
	POINTERS_EQUAL(  NULL, n1[1].child);
	POINTERS_EQUAL(&n1[0], n1[2].parent);
	POINTERS_EQUAL(&n1[3], n1[2].sibling);
	POINTERS_EQUAL(  NULL, n1[2].child);
	POINTERS_EQUAL(&n1[0], n1[3].parent);
	POINTERS_EQUAL(  NULL, n1[3].sibling);
	POINTERS_EQUAL(&n1[1], n1[3].child);

	tree_destroy(&t1clone);
}

TEST(TestSwapTrees, Indirect_related_nodes_13) {
	int print_tree = verbose;
	int rule = 1; // descendant_node_rule

	/* Generate following structuere:
	 * 0 —> 1 —> 2 —> 3 —> 4 —> 5
	 *                  —> 8
	 *   —> 6 —> 7
	 *   —> 9
	 */
	gen2(t1);
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}

	uint32_t num_nodes = tree_number_of_nodes(t1);
	// Copy t1 to check if swapped arguments leads to same result.
	Tree *t1clone = tree_clone(t1, NULL, NULL);

	// Swap nodes on same branch, one node between.
	LONGS_EQUAL(0, tree_swap_subtrees(t1, &n1[1], &n1[3], rule, 1));
	LONGS_EQUAL(0, tree_swap_subtrees(t1clone, &c1[3], &c1[1], rule, 1));
	/* Expected result after operation:
	 * 0 —> 3 —> 2 —> 1
	 *        —> 4 —> 5
	 *        —> 8
	 *   —> 6 —> 7
	 *   —> 9
	 */
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}
	LONGS_EQUAL(num_nodes, tree_number_of_nodes(t1));
	LONGS_EQUAL(0, tree_cmp(t1, t1clone, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));

	// Check structure
	// TODO: String to Tree parser needed

	tree_destroy(&t1clone);
}

TEST(TestSwapTrees, Indirect_related_nodes_14) {
	int print_tree = verbose;
	int rule = 1; // descendant_node_rule

	/* Generate following structuere:
	 * 0 —> 1 —> 2 —> 3 —> 4 —> 5
	 *                  —> 8
	 *   —> 6 —> 7
	 *   —> 9
	 */
	gen2(t1);
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}

	uint32_t num_nodes = tree_number_of_nodes(t1);
	// Copy t1 to check if swapped arguments leads to same result.
	Tree *t1clone = tree_clone(t1, NULL, NULL);

	// Swap nodes on same branch, two node between.
	LONGS_EQUAL(0, tree_swap_subtrees(t1, &n1[1], &n1[4], rule, 1));
	LONGS_EQUAL(0, tree_swap_subtrees(t1clone, &c1[4], &c1[1], rule, 1));
	/* Expected result after operation:
	 * 0 —> 4 —> 2 —> 3 —> 1
	 *                  —> 8
	 *        —> 5
	 *   —> 6 —> 7
	 *   —> 9
	 */
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}
	LONGS_EQUAL(num_nodes, tree_number_of_nodes(t1));
	LONGS_EQUAL(0, tree_cmp(t1, t1clone, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));

	// Check structure
	// TODO: String to Tree parser needed

	tree_destroy(&t1clone);
}

TEST(TestSwapTrees, Indirect_related_nodes_14flip) {
	int print_tree = verbose;
	int rule = 2; // descendant_node_rule

	/* Generate following structuere:
	 * 0 —> 1 —> 2 —> 3 —> 4 —> 5
	 *                  —> 8
	 *   —> 6 —> 7
	 *   —> 9
	 */
	gen2(t1);
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}

	uint32_t num_nodes = tree_number_of_nodes(t1);
	// Copy t1 to check if swapped arguments leads to same result.
	Tree *t1clone = tree_clone(t1, NULL, NULL);

	// Swap nodes on same branch, two node between.
	LONGS_EQUAL(0, tree_swap_subtrees(t1, &n1[1], &n1[4], rule, 1));
	LONGS_EQUAL(0, tree_swap_subtrees(t1clone, &c1[4], &c1[1], rule, 1));
	/* Expected result after operation:
	 * 0 —> 4 —> 3 —> 2 —> 1
	 *             —> 8
	 *        —> 5
	 *   —> 6 —> 7
	 *   —> 9
	 */
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}
	LONGS_EQUAL(num_nodes, tree_number_of_nodes(t1));
	LONGS_EQUAL(0, tree_cmp(t1, t1clone, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));

	// Check structure
	// TODO: String to Tree parser needed

	tree_destroy(&t1clone);
}

TEST(TestSwapTrees, Indirect_related_nodes_15) {
	int print_tree = verbose;
	int rule = 1; // descendant_node_rule

	/* Generate following structuere:
	 * 0 —> 1 —> 2 —> 3 —> 4 —> 5
	 *                  —> 8
	 *   —> 6 —> 7
	 *   —> 9
	 */
	gen2(t1);
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}

	uint32_t num_nodes = tree_number_of_nodes(t1);
	// Copy t1 to check if swapped arguments leads to same result.
	Tree *t1clone = tree_clone(t1, NULL, NULL);

	// Swap nodes on same branch, two node between.
	LONGS_EQUAL(0, tree_swap_subtrees(t1, &n1[1], &n1[5], rule, 1));
	LONGS_EQUAL(0, tree_swap_subtrees(t1clone, &c1[5], &c1[1], rule, 1));
	/* Expected result after operation:
	 * 0 —> 5 —> 2 —> 3 —> 4 —> 1
	 *                  —> 8
	 *   —> 6 —> 7
	 *   —> 9
	 */
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}
	LONGS_EQUAL(num_nodes, tree_number_of_nodes(t1));
	LONGS_EQUAL(0, tree_cmp(t1, t1clone, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));

	// Check structure
	// TODO: String to Tree parser needed

	tree_destroy(&t1clone);
}

TEST(TestSwapTrees, Indirect_related_nodes_15flip) {
	int print_tree = verbose;
	int rule = 2; // descendant_node_rule

	/* Generate following structuere:
	 * 0 —> 1 —> 2 —> 3 —> 4 —> 5
	 *                  —> 8
	 *   —> 6 —> 7
	 *   —> 9
	 */
	gen2(t1);
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}

	uint32_t num_nodes = tree_number_of_nodes(t1);
	// Copy t1 to check if swapped arguments leads to same result.
	Tree *t1clone = tree_clone(t1, NULL, NULL);

	// Swap nodes on same branch, two node between.
	LONGS_EQUAL(0, tree_swap_subtrees(t1, &n1[1], &n1[5], rule, 1));
	LONGS_EQUAL(0, tree_swap_subtrees(t1clone, &c1[5], &c1[1], rule, 1));
	/* Expected result after operation:
	 * 0 —> 5 —> 4 —> 3 —> 2 —> 1
	 *             —> 8
	 *   —> 6 —> 7
	 *   —> 9
	 */
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}
	LONGS_EQUAL(num_nodes, tree_number_of_nodes(t1));
	LONGS_EQUAL(0, tree_cmp(t1, t1clone, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));

	// Check structure
	// TODO: String to Tree parser needed

	tree_destroy(&t1clone);
}

TEST(TestSwapTrees, Non_related_nodes) {
	int print_tree = verbose;
	int rule = 0; // descendant_node_rule

	/* Generate following structuere:
	 * 0 —> 1 —> 2 —> 3 —> 4 —> 5
	 *                  —> 8
	 *   —> 6 —> 7
	 *   —> 9
	 */
	gen2(t1);
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}

	uint32_t num_nodes = tree_number_of_nodes(t1);
	// Copy t1 to check if swapped arguments leads to same result.
	Tree *t1clone = tree_clone(t1, NULL, NULL);

	// Swap nodes on same branch, two node between.
	LONGS_EQUAL(0, tree_swap_subtrees(t1, &n1[3], &n1[6], rule, 1));
	LONGS_EQUAL(0, tree_swap_subtrees(t1clone, &c1[6], &c1[3], rule, 1));
	/* Expected result after operation:
	 * 0 —> 1 —> 2 —> 6 —> 7
   *   —> 3 —> 4 —> 5
	 *        —> 8
	 *   —> 9
	 */
	if (print_tree) {
		printf("\n");
		tree_print(t1, NULL, 0);
	}
	LONGS_EQUAL(num_nodes, tree_number_of_nodes(t1));
	LONGS_EQUAL(0, tree_cmp(t1, t1clone, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));

	// Check structure
	// TODO: String to Tree parser needed

	tree_destroy(&t1clone);

}

//=================================================


int main(int ac, char** av)
{
	if (ac>1){
		verbose = 1;
	}
	return CommandLineTestRunner::RunAllTests(ac, av);
}

