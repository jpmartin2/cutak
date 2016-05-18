project(util)

file(GLOB_RECURSE HEADERS include/*.hpp)

source_group("Header Files" ${HEADERS})

add_library(${PROJECT_NAME} STATIC ${HEADERS})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(${PROJECT_NAME} INTERFACE include)