cmake_minimum_required(VERSION 3.16)
project(dungeon CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vector2D, TileMap, Collision2D, PathFind, NPC... come from the
# disciplina-de-jogos repo, expected next to this one.
set(LIB_DIR "${CMAKE_SOURCE_DIR}/../disciplina-de-jogos/source"
    CACHE PATH "source folder of the disciplina-de-jogos repo")

# The game uses the SFML 2 API. Also look in ~/.local/opt/sfml-2.6, for
# systems that only package SFML 3.
list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.local/opt/sfml-2.6")
find_package(SFML 2.5 COMPONENTS graphics window audio system REQUIRED)

file(GLOB LIB_SOURCES CONFIGURE_DEPENDS "${LIB_DIR}/*.cpp")
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

# third_party/ holds single-file libraries, unmodified, with their licenses.
set(INCLUDES src "${LIB_DIR}" third_party)

add_executable(dungeon ${GAME_SOURCES} ${LIB_SOURCES})
target_include_directories(dungeon PRIVATE ${INCLUDES})
target_link_libraries(dungeon PRIVATE sfml-graphics sfml-window sfml-audio sfml-system)
target_compile_definitions(dungeon PRIVATE
    ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets/")

# Tests for the rules and the synthesizer (no window, no sound card), if
# GoogleTest is installed.
find_package(GTest)
if(GTest_FOUND)
    set(TESTED_SOURCES ${GAME_SOURCES})
    list(FILTER TESTED_SOURCES EXCLUDE REGEX "/(Main|Music|Screen)\\.cpp$")

    add_executable(tests test/GameTest.cpp test/SynthTest.cpp
                   ${TESTED_SOURCES} ${LIB_SOURCES})
    target_include_directories(tests PRIVATE ${INCLUDES})
    target_link_libraries(tests PRIVATE GTest::gtest_main
                          sfml-graphics sfml-window sfml-system)
    target_compile_definitions(tests PRIVATE
        ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets/")
endif()
