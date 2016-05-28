project(util)

file(GLOB_RECURSE HEADERS include/*.hpp)

source_group("Header Files" ${HEADERS})

cuda_add_library(${PROJECT_NAME} src/util.cu ${HEADERS})
#target_include_directories(${PROJECT_NAME} PUBLIC include)
