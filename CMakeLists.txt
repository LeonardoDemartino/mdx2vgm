
cmake_minimum_required(VERSION 2.8)

project(mdx2vgm)

set(CMAKE_C_FLAGS_RELEASE "-Wall -O3")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O3")
set(CMAKE_C_FLAGS_DEBUG "-Wall -g")
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g")

add_executable(mdx2vgm
	src/mdx2vgm.cpp
)

