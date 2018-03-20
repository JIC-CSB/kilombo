cmake_minimum_required(VERSION 2.8)

project(ckbsim)

# On some systems, e.g. Ubuntu and debian, libcheck depends on libsubunit
# we try to find libsubunit here, if found it is linked to in
# src/tests/CMakeLists.txt

# uses FindSubunit, located in cmake-modules/FindSubunit.cmake
# from https://github.com/babelouest/ulfius

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)
include(FindSubunit)
find_package(Subunit)

enable_testing()

add_subdirectory (src)


