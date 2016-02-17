project(profiling_tool)
cmake_minimum_required(VERSION 2.8)

set(
	ROOT
	${CMAKE_CURRENT_LIST_DIR}
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY 
	${ROOT}/bin
)

include_directories(
	include
)

if(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
endif(UNIX)

add_subdirectory(src)
add_subdirectory(sample)