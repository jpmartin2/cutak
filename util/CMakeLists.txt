project(util)

file(GLOB_RECURSE HEADERS include/*.hpp)

source_group("Header Files" ${HEADERS})

add_library(${PROJECT_NAME} INTERFACE)
target_sources(${PROJECT_NAME} INTERFACE ${HEADERS})
target_include_directories(${PROJECT_NAME} INTERFACE include)
