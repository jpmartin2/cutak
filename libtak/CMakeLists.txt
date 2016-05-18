cmake_minimum_required(VERSION 2.8)

project(libtak)

add_executable(gen gen.cpp)

#add_custom_target(make-include-dir ALL
#                  COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/include)

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/table.cu ${PROJECT_BINARY_DIR}/include/table.hpp
                   COMMAND gen
                   DEPENDS make-include-dir gen
                   COMMENT "Generating table.cpp")

message(STATUS "PROJECT_BINARY_DIR: " ${PROJECT_BINARY_DIR})

file(GLOB_RECURSE HEADERS include/*.hpp)
file(GLOB_RECURSE GEN_HEADERS ${PROJECT_BINARY_DIR}/include/*.hpp)
set(GEN_SOURCE ${PROJECT_BINARY_DIR}/table.cu)

source_group("Header Files" FILES ${HEADERS})
source_group("Generated Files" FILES ${GEN_HEADERS} ${GEN_SOURCE})

include_directories(${PROJECT_BINARY_DIR}/include)
cuda_add_library(tak STATIC ${GEN_SOURCE} ${HEADERS} ${GEN_HEADERS})
target_link_libraries(tak util)
target_include_directories(tak PUBLIC ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)

set (LIBTAK_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include PARENT_SCOPE)
