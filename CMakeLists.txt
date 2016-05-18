cmake_minimum_required(VERSION 2.8)

find_package(CUDA QUIET REQUIRED)

set(CUDA_SEPARABLE_COMPILATION ON)

set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS}; -std=c++11)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(util)
add_subdirectory(libtak)
include_directories(${LIBTAK_INCLUDE_DIRS})
add_subdirectory(ai)

add_executable(main main.cpp)
target_link_libraries(main tak)

cuda_add_executable(testing test.cu)
target_link_libraries(testing tak)
