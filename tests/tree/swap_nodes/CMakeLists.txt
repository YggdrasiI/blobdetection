set(TEST_APP_NAME swap_nodes)
set(TEST_APP_SOURCES test_swap_nodes.cpp)
set(TEST_APP_LIBS _tree)

build_and_run_test(${TEST_APP_NAME} "${TEST_APP_SOURCES}" "${TEST_APP_LIBS}")


