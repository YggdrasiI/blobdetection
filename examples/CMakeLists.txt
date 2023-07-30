include_directories(
	${CMAKE_SOURCE_DIR}
	)
link_directories (
	${CMAKE_BINARY_DIR}
	)

add_subdirectory(example1)
add_subdirectory(example2)

install(TARGETS example1 example2
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	)
