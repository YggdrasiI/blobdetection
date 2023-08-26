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

TEST(TestTreeCompare, Test_CompareType1_EqualStructureRoot) {
	// Compare with just one root node
	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));
}

TEST(TestTreeCompare, Test_CompareType1_EqualStructure1) {
	// Compare with just one child
	init_tree_structure1(t1);
	init_tree_structure1(t2);
	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));
}

TEST(TestTreeCompare, Test_CompareType1_InequalStructure1vsRoot) {
	// Compare trees with different one and no child
	init_tree_structure1(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, Test_CompareType1_InequalStructure1vs2) {
	// Compare trees with different one and no child
	init_tree_structure1(t1);
	init_tree_structure2(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, Test_CompareType1_InequalStructure3vs4) {
	// Compare trees with 3 and 6 children
	init_tree_structure3(t1);
	init_tree_structure4(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, Test_CompareType1_EqualStructure2) {
	// Compare trees with same 3 Nodes
	init_tree_structure2(t1);
	init_tree_structure2(t2);

	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));
	LONGS_EQUAL(0, tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));
}

TEST(TestTreeCompare, Test_CompareType1_InequalStructure2vs3) {
	// Compare trees with different order of 3 Nodes
	init_tree_structure2(t1);
	init_tree_structure3(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, Test_CompareType1_EqualStructure8) {
	// Compare trees with identical nodes
	init_tree_structure8(t1);
	init_tree_structure8(t2);

	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));
	LONGS_EQUAL(0, tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT));
}

TEST(TestTreeCompare, Test_CompareType1_EqualStructure8B) {
	// Compare trees with nodes shuffeld children order (Assumed inequal for this compare type)
	init_tree_structure8(t1);
	init_tree_structure8B(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, Test_CompareType1_InequalStructure7vs8) {
	// Compare trees with different structure
	init_tree_structure7(t1);
	init_tree_structure8(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_SAME_NODE_MEMORY_LAYOUT);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

// =============================================================

TEST(TestTreeCompare, Test_CompareType2_EqualStructureRoot) {
	// Compare with just one root node
	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED));
}

TEST(TestTreeCompare, Test_CompareType2_EqualStructure1) {
	// Compare with just one child
	init_tree_structure1(t1);
	init_tree_structure1(t2);
	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED));
}

TEST(TestTreeCompare, Test_CompareType2_InequalStructure1vsRoot) {
	// Compare trees with different one and no child
	init_tree_structure1(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, Test_CompareType2_InequalStructure1vs2) {
	// Compare trees with different one and no child
	init_tree_structure1(t1);
	init_tree_structure2(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, Test_CompareType2_InequalStructure3vs4) {
	// Compare trees with 3 and 6 children
	init_tree_structure3(t1);
	init_tree_structure4(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, Test_CompareType2_EqualStructure2) {
	// Compare trees with same 3 Nodes
	init_tree_structure2(t1);
	init_tree_structure2(t2);

	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED));
	LONGS_EQUAL(0, tree_cmp(t2, t1, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED));
}

TEST(TestTreeCompare, Test_CompareType2_InequalStructure2vs3) {
	// Compare trees with different order of 3 Nodes
	init_tree_structure2(t1);
	init_tree_structure3(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, Test_CompareType2_EqualStructure8) {
	// Compare trees with identical nodes
	init_tree_structure8(t1);
	init_tree_structure8(t2);

	LONGS_EQUAL(0, tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED));
	LONGS_EQUAL(0, tree_cmp(t2, t1, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED));
}

TEST(TestTreeCompare, Test_CompareType2_EqualStructure8B) {
	// Compare trees with nodes shuffeld children order 
	init_tree_structure8(t1);
	init_tree_structure8B(t2);

	int ret = tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}

TEST(TestTreeCompare, Test_CompareType2_InequalStructure7vs8) {
	// Compare trees with different structure
	init_tree_structure7(t1);
	init_tree_structure8(t2);
	
	int ret = tree_cmp(t1, t2, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	int retSwap = tree_cmp(t2, t1, TREE_COMPARE_CHILD_NODE_ORDER_SCRAMBLED);
	CHECK(ret != 0);
	LONGS_EQUAL(ret, -retSwap);
}


// =============================================================

// =============================================================

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
