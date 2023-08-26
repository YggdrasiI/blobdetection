#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

#include "../tree_test_setup.hpp"

TEST_GROUP(TestTreeCreation) {
	void setup() {
	}

	void teardown() {
	}
};

TEST_GROUP(TestAddingNodes) {
	Tree *t1;
	Node * nodes1;

	void setup() {
		// This gets run before every test
		t1 = tree_create(10, 1);
		nodes1= t1->root;
	}

	void teardown() {
		// This gets run after every test
		nodes1 = NULL;
		tree_destroy(&t1);
	}
};

TEST_GROUP(TestRootOffset) {
	// Test if algorithms still working if root is not first node of tree->nodes
	void setup() {
	}

	void teardown() {
	}
};

TEST_GROUP(TestTreeCloning) {
	Tree *t1;
	void setup() {
		t1 = NULL;
	}

	void teardown() {
		tree_destroy(&t1);
	}

};



TEST(TestTreeCreation, Test_TreeCreation) {
	size_t N = 10;
	Tree *t2 = tree_create(N, 0);
	CHECK( t2 != NULL );
	CHECK( t2->root != NULL );
  LONGS_EQUAL(N, t2->size);
	POINTERS_EQUAL(t2->root, t2->nodes);
	tree_destroy(&t2);
}

TEST(TestTreeCreation, Test_TreeCreationSize0) {
	size_t N = 0;
	Tree *t2 = tree_create(N, 0);
	CHECK( t2 == NULL );
	tree_destroy(&t2);
}

TEST(TestTreeCreation, Test_TreeCreationAsLeafs) {
	size_t N = 3;
	Tree *t2 = tree_create(N, 1);
	CHECK( t2 != NULL );
	for(size_t i=0; i<N; ++i){
		POINTERS_EQUAL(NULL, t2->nodes[i].parent);
		POINTERS_EQUAL(NULL, t2->nodes[i].child);
		POINTERS_EQUAL(NULL, t2->nodes[i].sibling);
#ifdef TREE_REDUNDANT_INFOS
		LONGS_EQUAL(0, t2->nodes[i].width);
		LONGS_EQUAL(0, t2->nodes[i].height);
#endif
	}
  LONGS_EQUAL(N, t2->size);
	tree_destroy(&t2);
}

TEST(TestAddingNodes, Test_RootNodeIsLeafAtStartup) {
	POINTERS_EQUAL(NULL, nodes1[0].parent);
	POINTERS_EQUAL(NULL, nodes1[0].child);
	POINTERS_EQUAL(NULL, nodes1[0].sibling);
#ifdef TREE_REDUNDANT_INFOS
  LONGS_EQUAL(0, nodes1[0].width);
  LONGS_EQUAL(0, nodes1[0].height);
#endif
}

TEST(TestAddingNodes, Test_AddingOneNodeToRoot) {
	t1->nodes[1] = Leaf;
  LONGS_EQUAL(1, tree_number_of_nodes(t1));
  tree_add_child(t1->root, &nodes1[1]);
  LONGS_EQUAL(2, tree_number_of_nodes(t1));

	POINTERS_EQUAL(&nodes1[0], nodes1[1].parent);
	POINTERS_EQUAL(&nodes1[1], nodes1[0].child);
	POINTERS_EQUAL(NULL, nodes1[0].sibling);
	POINTERS_EQUAL(NULL, nodes1[1].sibling);
#ifdef TREE_REDUNDANT_INFOS
  LONGS_EQUAL(1, nodes1[0].width);
  LONGS_EQUAL(1, nodes1[0].height);
  LONGS_EQUAL(0, nodes1[1].width);
  LONGS_EQUAL(0, nodes1[1].height);
#endif
}

TEST(TestAddingNodes, Test_AddingTwoNodesToRoot) {
	t1->nodes[1] = Leaf;
	t1->nodes[2] = Leaf;
  tree_add_child(&nodes1[0], &nodes1[1]);
  tree_add_child(&nodes1[0], &nodes1[2]);
  LONGS_EQUAL(3, node_number_of_successors(&nodes1[0]));
  LONGS_EQUAL(1, node_number_of_successors(&nodes1[1]));
  LONGS_EQUAL(1, node_number_of_successors(&nodes1[2]));

	// Check root
	POINTERS_EQUAL(NULL, nodes1[0].parent);
	POINTERS_EQUAL(NULL, nodes1[0].sibling);
	POINTERS_EQUAL(&nodes1[1], nodes1[0].child);
#ifdef TREE_REDUNDANT_INFOS
  LONGS_EQUAL(2, nodes1[0].width);
  LONGS_EQUAL(1, nodes1[0].height);
#endif

	// Check child 1
	POINTERS_EQUAL(&nodes1[0], nodes1[1].parent);
	POINTERS_EQUAL(&nodes1[2], nodes1[1].sibling);
	POINTERS_EQUAL(NULL, nodes1[1].child);

	// Check child 2
	POINTERS_EQUAL(&nodes1[0], nodes1[2].parent);
	POINTERS_EQUAL(NULL, nodes1[2].sibling);
	POINTERS_EQUAL(NULL, nodes1[2].child);
}

TEST(TestAddingNodes, Test_AddingGrandchild) {
	t1->nodes[1] = Leaf;
	t1->nodes[2] = Leaf;
  tree_add_child(&nodes1[0], &nodes1[1]);
  tree_add_child(&nodes1[1], &nodes1[2]);
  LONGS_EQUAL(3, node_number_of_successors(&nodes1[0]));
  LONGS_EQUAL(2, node_number_of_successors(&nodes1[1]));
  LONGS_EQUAL(1, node_number_of_successors(&nodes1[2]));

	// Check root
	POINTERS_EQUAL(&nodes1[1], nodes1[0].child);
	POINTERS_EQUAL(NULL, nodes1[0].sibling);
	POINTERS_EQUAL(NULL, nodes1[1].sibling);
#ifdef TREE_REDUNDANT_INFOS
  LONGS_EQUAL(1, nodes1[0].width);
  LONGS_EQUAL(2, nodes1[0].height);
#endif

	// Check child 1
	POINTERS_EQUAL(&nodes1[0], nodes1[1].parent);
	POINTERS_EQUAL(NULL, nodes1[1].sibling);
	POINTERS_EQUAL(&nodes1[2], nodes1[1].child);
#ifdef TREE_REDUNDANT_INFOS
  LONGS_EQUAL(1, nodes1[1].width);
  LONGS_EQUAL(1, nodes1[1].height);
#endif

	// Check child 2
	POINTERS_EQUAL(&nodes1[1], nodes1[2].parent);
	POINTERS_EQUAL(NULL, nodes1[2].sibling);
	POINTERS_EQUAL(NULL, nodes1[2].child);
#ifdef TREE_REDUNDANT_INFOS
  LONGS_EQUAL(0, nodes1[2].width);
  LONGS_EQUAL(0, nodes1[2].height);
#endif
}


//=================================================

TEST(TestTreeCloning, Test_RootNodeSet) {
	t1 = tree_create(10, 0);
	init_tree_structure8(t1);
	Tree* clone1 = tree_clone(t1, NULL, NULL);

  LONGS_EQUAL(tree_number_of_nodes(t1),
			tree_number_of_nodes(clone1));
	POINTERS_EQUAL(NULL, clone1->nodes[0].sibling);
	POINTERS_EQUAL(&clone1->nodes[1], clone1->nodes[0].child);

	tree_destroy(&clone1);
}

TEST(TestTreeCloning, Test_NullTree) {
	Tree* clone1 = tree_clone(NULL, (void *)0x1, (void *)0x2);
	POINTERS_EQUAL(NULL, clone1);
	tree_destroy(&clone1);
}

TEST(TestTreeCloning, Test_WrongDataPointer) {
	t1 = tree_create(10, 1);
	Tree* clone1 = tree_clone(t1, (void *)0x1, NULL);
	Tree* clone2 = tree_clone(t1, NULL, (void *)0x1);
	POINTERS_EQUAL(NULL, clone1);
	POINTERS_EQUAL(NULL, clone2);

	tree_destroy(&clone1);
	tree_destroy(&clone2);
}

TEST(TestTreeCloning, Test_RedundantInformationFunctions) {
	t1 = tree_create(10, 1);
	init_tree_structure8(t1);
	Tree* clone1 = tree_clone(t1, NULL, NULL);
	Tree* clone2 = tree_clone(t1, NULL, NULL);

	tree_gen_redundant_information(clone1->root);
	tree_gen_redundant_information_recursive(clone2->root);
	// Compare values of all tree variants. Assume the simplest algorithm
	// (clone2) evaluated the correct values.
	
  int test1 =  _trees_depth_first_search(clone2, t1,
      child_compare_func1, parent_compare_func1,
      NULL, NULL, NULL);
  int test2 =  _trees_depth_first_search(clone2, clone1,
      child_compare_func1, parent_compare_func1,
      NULL, NULL, NULL);

	LONGS_EQUAL(test1, test2);

	tree_destroy(&clone1);
	tree_destroy(&clone2);
}

TEST(TestTreeCloning, Test_RedundantInformationFunctionsSubtree) {
	t1 = tree_create(10, 1);
	init_tree_structure8(t1);
	Tree* clone1 = tree_clone(t1, NULL, NULL);
	Tree* clone2 = tree_clone(t1, NULL, NULL);

	tree_gen_redundant_information(&clone1->nodes[2]);
	tree_gen_redundant_information_recursive(&clone2->nodes[2]);
	// Compare values of all tree variants. 
	
  int test1 =  _trees_depth_first_search(t1, clone1,
      child_compare_func1, parent_compare_func1,
      NULL, NULL, NULL);
  int test2 =  _trees_depth_first_search(t1, clone2,
      child_compare_func1, parent_compare_func1,
      NULL, NULL, NULL);

	LONGS_EQUAL(test1, test2);

	tree_destroy(&clone1);
	tree_destroy(&clone2);
}

int main(int ac, char** av)
{
	   return CommandLineTestRunner::RunAllTests(ac, av);
}
