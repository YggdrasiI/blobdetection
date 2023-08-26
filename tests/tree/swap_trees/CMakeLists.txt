set(TEST_APP_NAME swap_trees)
set(TEST_APP_SOURCES test_swap_trees.cpp)
set(TEST_APP_LIBS _tree)

build_and_run_test(${TEST_APP_NAME} "${TEST_APP_SOURCES}" "${TEST_APP_LIBS}")


