#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

extern "C" {
#include "tree.h"
}


TEST_GROUP(TestTreeCompare) {
	Tree *t1, *t2;
	const size_t num_nodes = 10;
	void setup() {
		// This gets run before every test
		t1 = tree_create(num_nodes, 1);
		t2 = tree_create(num_nodes, 1);
	}

	void teardown() {
		// This gets run after every test
		tree_destroy(&t1);
		tree_destroy(&t2);
	}

	void gen_tree_structure1(Tree *t){
		/* 0 —> 1
		 * |
		 *  ——> 2
		 */
		tree_add_child(&t->nodes[0], &t->nodes[1]);
		tree_add_child(&t->nodes[0], &t->nodes[2]);
	}

	void gen_tree_structure2(Tree *t){
		/* 0 —> 2
		 * |
		 *  ——> 1
		 */
		tree_add_child(&t->nodes[0], &t->nodes[2]);
		tree_add_child(&t->nodes[0], &t->nodes[1]);
	}

	void gen_tree_structure3(Tree *t){
		/* 0 —> 1 —> 2
		 * |
		 * ———> 3 —> 4
		 *      |
		 *       ——> 5
		 */
		tree_add_child(&t->nodes[0], &t->nodes[1]);
		tree_add_child(&t->nodes[0], &t->nodes[3]);
		tree_add_child(&t->nodes[1], &t->nodes[2]);
		tree_add_child(&t->nodes[3], &t->nodes[4]);
		tree_add_child(&t->nodes[3], &t->nodes[5]);
	}

	void gen_tree_structure4(Tree *t){
		/* 0 —> 1 —> 2
		 * |    |
		 * |     ——> 3
		 * |
		 * ———> 4 —> 5
		 */
		tree_add_child(&t->nodes[0], &t->nodes[1]);
		tree_add_child(&t->nodes[0], &t->nodes[4]);
		tree_add_child(&t->nodes[1], &t->nodes[2]);
		tree_add_child(&t->nodes[1], &t->nodes[3]);
		tree_add_child(&t->nodes[4], &t->nodes[5]);

	}

	void gen_tree_structure5(Tree *t){
		/* 0 —> 1 —> 2
		 * |    |
		 * |     ——> 3
		 * |
		 * ———> 4 —> 5
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
};


TEST(TestTreeCompare, Test_CompareType1_EqualStructure0) {
	// Compare with just one root node
	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));
}

TEST(TestTreeCompare, Test_CompareType1_EqualStructure1) {
	// Compare trees with same 3 Nodes
	gen_tree_structure1(t1);
	gen_tree_structure1(t2);

	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));
	LONGS_EQUAL(0, tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));
}

TEST(TestTreeCompare, Test_CompareType1_InequalStructure12) {
	// Compare trees with different order of 3 Nodes
	gen_tree_structure1(t1);
	gen_tree_structure2(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

int main(int ac, char** av)
{
	   return CommandLineTestRunner::RunAllTests(ac, av);
}

#if 0
int main(){
  Tree *t1 = tree_create(10);
  Node * const nodes= t1->root;
  tree_add_child(&nodes[0], &nodes[1]);
  tree_add_child(&nodes[0], &nodes[9]);
  tree_add_child(&nodes[9], &nodes[8]);
  tree_add_child(&nodes[8], &nodes[7]);
  tree_add_child(&nodes[9], &nodes[6]);
  tree_add_child(&nodes[1], &nodes[2]);
  tree_add_child(&nodes[1], &nodes[3]);
  tree_add_child(&nodes[1], &nodes[4]);
  tree_add_child(&nodes[1], &nodes[5]);

  tree_print(t1, t1->root, 0);

  tree_destroy(&t1);

  return 0;
}
#endif
