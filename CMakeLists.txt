cmake_minimum_required(VERSION 3.16)
project(BreakoutGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(BUILD_TESTING "Build tests" ON)
option(BUILD_APP "Build Qt GUI application" OFF)

if(BUILD_APP)
    find_package(Qt6 COMPONENTS Core Gui Widgets Multimedia REQUIRED)
endif()

add_library(breakout_core
    src/core/utils/vector2d.cpp
    src/core/utils/collision.cpp
    src/core/utils/random.cpp
    src/core/entities/ball.cpp
    src/core/entities/paddle.cpp
    src/core/entities/brick.cpp
    src/core/game/physics_engine.cpp
    src/core/game/level_manager.cpp
    src/core/game/game_engine.cpp
)

target_include_directories(breakout_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(breakout_core PRIVATE -Wall -Wextra -Wpedantic)

if(BUILD_APP)
    add_executable(breakout_app
        src/main.cpp
        src/ui/main_window.cpp
        src/ui/main_window.h
        src/ui/game_widget.cpp
        src/ui/game_widget.h
        src/ui/endgame_editor.cpp
        src/ui/endgame_editor.h
        src/data/config_manager.cpp
        src/data/config_manager.h
        src/data/endgame_manager.cpp
        src/data/endgame_manager.h
    )

    target_link_libraries(breakout_app PRIVATE
        breakout_core
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Multimedia
    )

    target_compile_options(breakout_app PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(breakout_core_tests
        tests/core_tests.cpp
    )

    target_link_libraries(breakout_core_tests PRIVATE
        breakout_core
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(breakout_core_tests)
endif()
