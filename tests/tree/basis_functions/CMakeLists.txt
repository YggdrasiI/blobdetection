set(TEST_APP_NAME tree_basis_functions)
set(TEST_APP_SOURCES test_basis_functions.cpp)
set(TEST_APP_LIBS _tree)

build_and_run_test(${TEST_APP_NAME} "${TEST_APP_SOURCES}" "${TEST_APP_LIBS}")


