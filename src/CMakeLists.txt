set(THRESH_SOURCES blob.c threshtree.c tree.c threshtree_old.c output.c output_threshtree.c)
add_library(threshtree SHARED ${THRESH_SOURCES} )

set(DEPTH_SOURCES blob.c depthtree.c tree.c output.c output_depthtree.c)
add_library(depthtree SHARED ${DEPTH_SOURCES} )

target_link_libraries(threshtree m)
target_link_libraries(depthtree m)

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
