project(ai)

file(GLOB_RECURSE HEADERS *.hpp)

source_group("Header Files" FILES ${HEADERS})

add_executable(bot eval.cpp bot.cpp)
add_executable(solve3 eval.cpp solve3.cpp)
target_link_libraries(bot tak)
target_link_libraries(solve3 tak)
