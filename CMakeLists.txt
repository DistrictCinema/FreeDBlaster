cmake_minimum_required(VERSION 3.4)

project(FreeDBlaster)

file(GLOB_RECURSE MainSrc
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
add_executable(${PROJECT_NAME} ${MainSrc})
