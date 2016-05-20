project(ai)

file(GLOB_RECURSE HEADERS *.hpp)

source_group("Header Files" FILES ${HEADERS})

#add_executable(ai ai.cpp ${HEADERS})
add_executable(client client.cpp)
#target_link_libraries(ai tak util)
target_link_libraries(client tak)
