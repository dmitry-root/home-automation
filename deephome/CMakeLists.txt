cmake_minimum_required(VERSION 3.5)
project(deephome)

set(deephome_VERSION_MAJOR 0)
set(deephome_VERSION_MINOR 1)

if ("$ENV{DEEPHOME_INSTALL_PREFIX}" STREQUAL "")
	set(INSTALL_PATH "${PROJECT_SOURCE_DIR}/../output/deephome")
else()
	set(INSTALL_PATH "$ENV{DEEPHOME_INSTALL_PREFIX}")
endif()
message("Install path: ${INSTALL_PATH}")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INSTALL_PREFIX "${INSTALL_PATH}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG -O2")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O2")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

include_directories("${PROJECT_SOURCE_DIR}/gtest/include")
include_directories("${PROJECT_SOURCE_DIR}/../firmware/shared/include")
include_directories("${PROJECT_SOURCE_DIR}/common/include")

add_subdirectory(gtest)
add_subdirectory(common)
add_subdirectory(router)
add_subdirectory(test)

