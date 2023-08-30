include_directories(
	${CMAKE_SOURCE_DIR}
	)
link_directories (
	${CMAKE_BINARY_DIR}
	)

if (WITH_TESTS)
	add_custom_target(example_semaphore)
	add_dependencies(example_semaphore tests_semaphore)
endif()

add_subdirectory(example1)
add_subdirectory(example2)
add_subdirectory(opencv)
add_subdirectory(find_frame)

install(TARGETS example1 example2
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	)
