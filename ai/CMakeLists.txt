project(ai)

file(GLOB_RECURSE HEADERS *.hpp)

source_group("Header Files" FILES ${HEADERS})

add_executable(ai ai.cpp ${HEADERS})
target_link_libraries(ai tak util)
