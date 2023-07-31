project(Blobdetection)
cmake_minimum_required(VERSION 2.8)

SET(CMAKE_C_STANDARD 11)

OPTION(WITH_EXAMPLES "Compile examples" ON)

include_directories(
	${CMAKE_SOURCE_DIR}/include/blobdetection
	)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fpic -O3" )
#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -g -O0 -fmax-errors=3 -w" )

add_subdirectory(src)

if(WITH_EXAMPLES)
	add_subdirectory(examples)
else(WITH_EXAMPLES)
	message(STATUS "Skip examples")
endif(WITH_EXAMPLES)

