cmake_minimum_required(VERSION 3.6)
project(tsp VERSION 0.1 LANGUAGES CXX)

add_executable(two_level_tree_test 
	../src/two_level_tree.cpp
	src/test_two_level_tree.cpp
	src/test_main.cpp
)

# C++ 11
target_compile_features(two_level_tree_test PUBLIC cxx_std_11)
set_target_properties(two_level_tree_test PROPERTIES CXX_EXTENSIONS OFF)
# include path
target_include_directories(two_level_tree_test PRIVATE  lib ../include )

# macro for fmt and msvc
target_compile_definitions(two_level_tree_test PRIVATE FMT_HEADER_ONLY _CRT_SECURE_NO_WARNINGS)
target_compile_definitions(two_level_tree_test PRIVATE "$<$<CONFIG:RELEASE>:NDEBUG>")

# set the build type to default release if not specified by the user
if (NOT EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt)
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
	message(STATUS "Setting build type to ${CMAKE_BUILD_TYPE} since none was specified")
  endif()
endif()