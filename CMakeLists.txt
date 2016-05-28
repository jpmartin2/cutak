cmake_minimum_required(VERSION 2.8)

project(cutak)

#set(CUDA_ARCH -gencode arch=compute_30,code=sm_30 -gencode arch=compute_35,code=sm_35 -gencode arch=compute_50,code=sm_50 -gencode arch=compute_52,code=sm_52)
set(CUDA_ARCH -gencode arch=compute_52,code=sm_52)

find_package(CUDA QUIET REQUIRED)
set(CUDA_SEPARABLE_COMPILATION ON)
set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS}; -D_FORCE_INLINES -std=c++11 ${CUDA_ARCH} --expt-extended-lambda)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(external/asio/asio/include)
include_directories(external/cub)
include_directories(external/libcuckoo/src)
include_directories(util/include)

add_subdirectory(util)
add_subdirectory(libtak)
add_subdirectory(ai)
include_directories(${LIBTAK_INCLUDE_DIRS})
get_property(UTIL_INCLUDES TARGET util PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${UTIL_INCLUDES})

add_executable(main main.cpp)
target_link_libraries(main tak)
