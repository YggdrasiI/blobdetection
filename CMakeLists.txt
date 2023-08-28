cmake_minimum_required(VERSION 3.6)
project(Blobdetection)

SET(CMAKE_C_STANDARD 11)

OPTION(WITH_TESTS "Compile tests" OFF)
OPTION(WITH_EXAMPLES "Compile examples" ON)

include_directories(
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/src
	)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fpic -O3" )
#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -g -O0 -fmax-errors=3 -w" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c18 -Wall " )
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -std=c18 -Wall -Werror " )

# For clang/lsp
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)

if(WITH_TESTS)
	add_subdirectory(tests)
else(WITH_TESTS)
	message(STATUS "Skip tests")
endif(WITH_TESTS)

if(WITH_EXAMPLES)
	add_subdirectory(examples)
else(WITH_EXAMPLES)
	message(STATUS "Skip examples")
endif(WITH_EXAMPLES)
