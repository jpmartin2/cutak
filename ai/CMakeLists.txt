project(ai)

file(GLOB_RECURSE HEADERS *.hpp)

source_group("Header Files" FILES ${HEADERS})

add_executable(bot eval.cpp bot.cpp)
target_link_libraries(bot tak)
