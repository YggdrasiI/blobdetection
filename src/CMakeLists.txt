set(TREE_INLINE_SOURCES tree_intern.c)  # Code just added if some function calls were not inlined.
add_library(_tree_inline
	STATIC ${TREE_INLINE_SOURCES})

set(TREE_SOURCES tree.c tree_intern.c enums.c tree_operations.c tree_compare.c tree_hash.c io_for_debugging.c)
add_library(_tree SHARED ${TREE_SOURCES})
#add_library(_tree STATIC ${TREE_SOURCES})
target_link_libraries(_tree _tree_inline)

set(THRESH_SOURCES blob.c threshtree.c threshtree_old.c output.c output_threshtree.c )
add_library(threshtree SHARED ${THRESH_SOURCES} ${TREE_SOURCES})

set(DEPTH_SOURCES blob.c depthtree.c output.c output_depthtree.c)
add_library(depthtree SHARED ${DEPTH_SOURCES} ${TREE_SOURCES})

# target_link_libraries(threshtree PUBLIC m _tree)
# target_link_libraries(depthtree PUBLIC m _tree)
target_link_libraries(threshtree PUBLIC m _tree_inline)
target_link_libraries(depthtree PUBLIC m _tree_inline)

# Define set of public header files for install
file(GLOB_RECURSE BLOBDETECTION_INCLUDE_FILES "${CMAKE_SOURCE_DIR}/include/*.h")
set_target_properties(threshtree
	PROPERTIES
	PUBLIC_HEADER "${BLOBDETECTION_INCLUDE_FILES}"
	)

install(TARGETS threshtree depthtree
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include/blobdetection
	)
