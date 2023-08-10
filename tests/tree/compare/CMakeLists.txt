set(TEST_APP_NAME tree_compare)

add_executable(${TEST_APP_NAME} test_tree_compare.cpp )
target_link_libraries(${TEST_APP_NAME} _tree ${CPPUTEST_LDFLAGS}) 


#Run the test once the build is done
#add_custom_command(TARGET ${TEST_APP_NAME} COMMAND ./${TEST_APP_NAME} POST_BUILD)

