cmake_minimum_required(VERSION 3.16)
project(dungeon CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# As classes da disciplina (Vector2D, TileMap, NPC...) vêm do repo
# disciplina-de-jogos, que fica ao lado deste.
set(LIB_DIR "${CMAKE_SOURCE_DIR}/../disciplina-de-jogos/source"
    CACHE PATH "Pasta source do repo disciplina-de-jogos")

# O jogo usa a API da SFML 2 (o Arch só empacota a 3; ver README).
list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.local/opt/sfml-2.6")
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

file(GLOB LIB_SOURCES CONFIGURE_DEPENDS "${LIB_DIR}/*.cpp")
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(dungeon ${GAME_SOURCES} ${LIB_SOURCES})
target_include_directories(dungeon PRIVATE src "${LIB_DIR}")
target_link_libraries(dungeon PRIVATE sfml-graphics sfml-window sfml-system)
target_compile_definitions(dungeon PRIVATE
    ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets/")

# Testes das regras (sem janela), se o GoogleTest estiver instalado.
find_package(GTest)
if(GTest_FOUND)
    set(RULE_SOURCES ${GAME_SOURCES})
    list(FILTER RULE_SOURCES EXCLUDE REGEX "/Main\\.cpp$")

    add_executable(testes test/JogoTest.cpp ${RULE_SOURCES} ${LIB_SOURCES})
    target_include_directories(testes PRIVATE src "${LIB_DIR}")
    target_link_libraries(testes PRIVATE GTest::gtest_main
                          sfml-graphics sfml-window sfml-system)
    target_compile_definitions(testes PRIVATE
        ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets/")
endif()
