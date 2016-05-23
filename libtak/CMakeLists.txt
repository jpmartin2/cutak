cmake_minimum_required(VERSION 2.8)

project(libtak)

add_executable(gen gen.cpp)

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include)

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/table.cu ${PROJECT_BINARY_DIR}/include/table.hpp
                   COMMAND gen
                   DEPENDS gen
                   COMMENT "Generating table.cu")

file(GLOB_RECURSE HEADERS include/*.hpp)
file(GLOB_RECURSE GEN_HEADERS ${PROJECT_BINARY_DIR}/include/*.hpp)
set(SOURCE src/tak/net/message.cpp src/tak/dynamic.cpp)
set(GEN_SOURCE ${PROJECT_BINARY_DIR}/table.cu)

source_group("Header Files" FILES ${HEADERS})
source_group("Generated Files" FILES ${GEN_HEADERS} ${GEN_SOURCE})

# Shouldn't have to do this, but cuda_add_library seems to mess things up
get_property(UTIL_INCLUDES TARGET util PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${UTIL_INCLUDES})

include_directories(${PROJECT_BINARY_DIR}/include)
include_directories(include)

cuda_add_executable(alphabeta ${GEN_SOURCE} ${HEADERS} ${GEN_HEADERS} alphabeta.cu)

cuda_add_library(tak STATIC ${SOURCE} ${GEN_SOURCE} ${HEADERS} ${GEN_HEADERS})
target_link_libraries(tak util)
target_include_directories(tak PUBLIC ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include ${UTIL_INCLUDES})

set (LIBTAK_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include PARENT_SCOPE)
