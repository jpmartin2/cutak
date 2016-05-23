cmake_minimum_required(VERSION 2.8)

project(cutak)

find_package(CUDA QUIET REQUIRED)
set(CUDA_SEPARABLE_COMPILATION OFF)
set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS}; --expt-extended-lambda -std=c++11 -gencode arch=compute_52,code=sm_52 --expt-extended-lambda -rdc=true)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(external/asio/asio/include)
include_directories(external/cub)

add_subdirectory(util)
add_subdirectory(libtak)
add_subdirectory(ai)
include_directories(${LIBTAK_INCLUDE_DIRS})
get_property(UTIL_INCLUDES TARGET util PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${UTIL_INCLUDES})

add_executable(main main.cpp)
target_link_libraries(main tak)
