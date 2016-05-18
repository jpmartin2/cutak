cmake_minimum_required(VERSION 2.8)

project(libtak)

add_executable(gen gen.cpp)

add_custom_target(make-include-dir ALL
                  COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/include)

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/table.cu ${PROJECT_BINARY_DIR}/include/table.hpp
                   COMMAND gen
                   DEPENDS make-include-dir gen
                   COMMENT "Generating table.cpp")

message(STATUS "PROJECT_BINARY_DIR: " ${PROJECT_BINARY_DIR})

include_directories(${PROJECT_BINARY_DIR}/include)
cuda_add_library(tak STATIC ${PROJECT_BINARY_DIR}/table.cu)
target_include_directories(tak PUBLIC ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)

set (LIBTAK_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include PARENT_SCOPE)
