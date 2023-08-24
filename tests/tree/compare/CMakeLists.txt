set(TEST_APP_NAME tree_compare)
set(TEST_APP_SOURCES test_tree_compare.cpp)
set(TEST_APP_LIBS _tree)

build_and_run_test(${TEST_APP_NAME} "${TEST_APP_SOURCES}" "${TEST_APP_LIBS}")


