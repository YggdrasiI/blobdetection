# (1) Look for installed version of CppUTest
if(DEFINED ENV{CPPUTEST_HOME})
    message(STATUS "Using CppUTest home: $ENV{CPPUTEST_HOME}")
    set(CPPUTEST_INCLUDE_DIRS $ENV{CPPUTEST_HOME}/include)
    set(CPPUTEST_LIBRARIES $ENV{CPPUTEST_HOME}/lib)
    set(CPPUTEST_LDFLAGS CppUTest CppUTestExt)
else()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(CPPUTEST REQUIRED cpputest>=3.8)
		message(STATUS "Found CppUTest version ${CPPUTEST_VERSION}")
endif()
include_directories(${CPPUTEST_INCLUDE_DIRS})
link_directories(${CPPUTEST_LIBRARIES})

#include_directories(
#	${CMAKE_SOURCE_DIR}/include/blobdetection
#	${CMAKE_SOURCE_DIR}/src
#	)
#link_directories (
#	${CMAKE_BINARY_DIR}
#	)

macro(build_and_run_test TEST_NAME SOURCE_FILES LIBS)

	add_executable(${TEST_NAME} ${SOURCE_FILES})
	target_link_libraries(${TEST_NAME} ${CPPUTEST_LDFLAGS} ${LIBS}) 

	### Run the test once the build is done

	# 1)
	# Simple template which deletes the binary on failure.
	# Drawback: Deletion of binary is anoying for debugging.
	#
	#add_custom_command(TARGET ${TEST_APP_NAME} COMMAND ./${TEST_APP_NAME} POST_BUILD)

	# 2)
	# This variant interleaves a target between build and running test.
	# It will keep the binary.
	# Drawback: Test called on every 'make' even if sources or test file 
	#           has not changed.
	#
	#add_custom_target(${TEST_NAME}_run ALL DEPENDS ${TEST_NAME})
	#add_custom_command(TARGET ${TEST_NAME}_run COMMAND ./${TEST_NAME} POST_BUILD)

	# 3)
	# Like (2), but test just runs if source code change led to new test binary.
	#
	# This variant creates a file if the test completes successful and removes
	# it when a new test binary was build.
	#
  set(TEST_SUCC_FILE ./.${TEST_NAME}_succeded)
	add_custom_target(${TEST_NAME}_run ALL DEPENDS ${TEST_NAME})
	add_custom_command(TARGET ${TEST_NAME}_run
		COMMAND test -e "${TEST_SUCC_FILE}" || ./${TEST_NAME}
		) 
	add_custom_command(TARGET ${TEST_NAME}_run
		POST_BUILD
		COMMAND touch "${TEST_SUCC_FILE}"
		)
	add_custom_command(TARGET ${TEST_NAME}
		POST_BUILD
		COMMAND test -e "${TEST_SUCC_FILE}" && rm "${TEST_SUCC_FILE}" || true
		)
endmacro()

function(loop_over_test_dirs PARENT_FOLDER BLACKLIST)
	# message(STATUS "Parent folder: ${PARENT_FOLDER}")
	# message(STATUS "Blacklist: ${BLACKLIST}")
	FILE(GLOB FOLDERS RELATIVE ${PARENT_FOLDER} ${PARENT_FOLDER}/*)
	FOREACH(SUBFOLDER ${FOLDERS})
		IF(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SUBFOLDER}")
			#IF (NOT "${SUBFOLDER}" IN_LIST ${TESTS_BACKLIST})  # Requires cmake 3.6
			LIST(FIND BLACKLIST ${SUBFOLDER} BLACKLIST_POS)
			# message("${SUBFOLDER} pos? ${BLACKLIST_POS} ${BLACKLIST_POSB}")
			IF( "-1" EQUAL ${BLACKLIST_POS} )
				message(STATUS "Include '${PARENT_FOLDER}/${SUBFOLDER}'")
				add_subdirectory(${SUBFOLDER})
			ELSE()
				message(STATUS "Skip blacklisted test '${SUBFOLDER}'")
			ENDIF()
    ENDIF()
  ENDFOREACH()
endfunction()

LIST(APPEND TESTGROUPS_BLACKLIST "none")

message(STATUS "Tests:")
list(APPEND CMAKE_MESSAGE_INDENT "  ")

#add_subdirectory(tree)
loop_over_test_dirs(${CMAKE_CURRENT_SOURCE_DIR} ${TESTGROUPS_BLACKLIST})

list(POP_BACK CMAKE_MESSAGE_INDENT)
