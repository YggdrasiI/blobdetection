#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

#include "../tree_test_setup.hpp"


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
};

TEST(TestTreeCompare, CompareType1_EqualStructureRoot) {
	// Compare with just one root node
	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));
}

TEST(TestTreeCompare, CompareType1_EqualStructure1) {
	// Compare with just one child
	init_tree_structure1(t1);
	init_tree_structure1(t2);
	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));
}

TEST(TestTreeCompare, CompareType1_InequalStructure1vsRoot) {
	// Compare trees with different one and no child
	init_tree_structure1(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, CompareType1_InequalStructure1vs2) {
	// Compare trees with different one and no child
	init_tree_structure1(t1);
	init_tree_structure2(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, CompareType1_InequalStructure3vs4) {
	// Compare trees with 3 and 6 children
	init_tree_structure3(t1);
	init_tree_structure4(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, CompareType1_EqualStructure2) {
	// Compare trees with same 3 Nodes
	init_tree_structure2(t1);
	init_tree_structure2(t2);

	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));
	LONGS_EQUAL(0, tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));
}

TEST(TestTreeCompare, CompareType1_InequalStructure2vs3) {
	// Compare trees with different order of 3 Nodes
	init_tree_structure2(t1);
	init_tree_structure3(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, CompareType1_EqualStructure8) {
	// Compare trees with identical nodes
	init_tree_structure8(t1);
	init_tree_structure8(t2);

	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));
	LONGS_EQUAL(0, tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));
}

TEST(TestTreeCompare, CompareType1_EqualStructure8B) {
	// Compare trees with nodes shuffeld children order (Assumed inequal for this compare type)
	init_tree_structure8(t1);
	init_tree_structure8B(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, CompareType1_InequalStructure7vs8) {
	// Compare trees with different structure
	init_tree_structure7(t1);
	init_tree_structure8(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

// =============================================================

TEST(TestTreeCompare, CompareType2_EqualStructureRoot) {
	// Compare with just one root node
	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED));
}

TEST(TestTreeCompare, CompareType2_EqualStructure1) {
	// Compare with just one child
	init_tree_structure1(t1);
	init_tree_structure1(t2);
	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED));
}

TEST(TestTreeCompare, CompareType2_InequalStructure1vsRoot) {
	// Compare trees with different one and no child
	init_tree_structure1(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, CompareType2_InequalStructure1vs2) {
	// Compare trees with different one and no child
	init_tree_structure1(t1);
	init_tree_structure2(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, CompareType2_InequalStructure3vs4) {
	// Compare trees with 3 and 6 children
	init_tree_structure3(t1);
	init_tree_structure4(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, CompareType2_EqualStructure2) {
	// Compare trees with same 3 Nodes
	init_tree_structure2(t1);
	init_tree_structure2(t2);

	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED));
	LONGS_EQUAL(0, tree_cmp(t2, t1, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED));
}

TEST(TestTreeCompare, CompareType2_InequalStructure2vs3) {
	// Compare trees with different order of 3 Nodes
	init_tree_structure2(t1);
	init_tree_structure3(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, CompareType2_EqualStructure8) {
	// Compare trees with identical nodes
	init_tree_structure8(t1);
	init_tree_structure8(t2);

	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED));
	LONGS_EQUAL(0, tree_cmp(t2, t1, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED));
}

TEST(TestTreeCompare, CompareType2_EqualStructure8B) {
	// Compare trees with nodes shuffeld children order 
	init_tree_structure8(t1);
	init_tree_structure8B(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, CompareType2_InequalStructure7vs8) {
	// Compare trees with different structure
	init_tree_structure7(t1);
	init_tree_structure8(t2);
	
	int ret = tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}


// =============================================================

TEST(TestTreeCompare, CompareType3_EqualStructure1) {
	// Compare trees with identical nodes
	init_tree_structure8(t1);
	init_tree_structure8(t2);

	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_IF_NODES_ISOMORPH));
	LONGS_EQUAL(0, tree_cmp(t2, t1, TREE_COMPARE_IF_NODES_ISOMORPH));
}

TEST(TestTreeCompare, CompareType3_EqualStructure2) {
	// Compare trees with some swapped nodes, where nodes are on the same level
	// Thus, it should be isomorph
	init_tree_structure8(t1);
	init_tree_structure8(t2);

	/* t1, t2:
	 * 0 —> 1 —> 2 —> 8
	 *       ——> 3
	 *  ——> 4 —> 5
	 *       ——> 6
	 *       ——> 7
	 *  ——> 9
	 */
	tree_swap_subtrees(t1, &t1->nodes[1], &t1->nodes[9], 0, 0);
	tree_swap_nodes(t1, &t1->nodes[6], &t1->nodes[8], 0, 0);
	tree_swap_nodes(t1, &t1->nodes[3], &t1->nodes[5], 0, 0);
  //tree_print(t1, t1->root, 0);
	/* t1':
	 * 0 —> 9
	 *  ——> 4 —> 3
	 *       ——> 8
	 *       ——> 7
	 *  ——> 1 —> 2 —> 6
	 *       ——> 5
	 */

	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_IF_NODES_ISOMORPH));
	LONGS_EQUAL(0, tree_cmp(t2, t1, TREE_COMPARE_IF_NODES_ISOMORPH));
}
// =============================================================

// =============================================================

TEST(TestTreeCompare, CompareType6_EqualStructure8_Null_data) {
	// Compare trees with identical nodes
	init_tree_structure8(t1);
	init_tree_structure8(t2);

	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_IF_DATA_ISOMORPH));
	LONGS_EQUAL(0, tree_cmp(t2, t1, TREE_COMPARE_IF_DATA_ISOMORPH));
}

TEST(TestTreeCompare, CompareType6_EqualStructure8) {
	// Compare trees with identical nodes/data
	init_tree_structure8(t1);
	init_tree_structure8(t2);

	char *offset = (char *)0x4141414141414141; //shift label chars into printable range.
	for(int i=0; i<t1->size; ++i){
		t1->nodes[i].data = offset + 0x0102030405060708 + i; //* 16;
		t2->nodes[i].data = offset + 0x0102030405060708 + i; //* 16;
	}

	// Swap two values
	void *tmp = t2->nodes[5].data;
	t2->nodes[5].data = t2->nodes[7].data;
	t2->nodes[7].data = tmp;

	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_IF_DATA_ISOMORPH));
	LONGS_EQUAL(0, tree_cmp(t2, t1, TREE_COMPARE_IF_DATA_ISOMORPH));
}
// =============================================================

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
