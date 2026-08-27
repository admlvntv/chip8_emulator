cmake_minimum_required(VERSION 3.28)
project(chip8_emulator)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.14
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

add_executable(chip8_emulator
        src/main.cpp
        src/CPU.cpp
        src/CPU.h
        src/Display.cpp
        src/Display.h
        src/Keypad.cpp
        src/Keypad.h
        src/TerminalDisplay.cpp
        src/TerminalDisplay.h
        src/SDLDisplay.cpp
        src/SDLDisplay.h
)
target_link_libraries(chip8_emulator PRIVATE SDL3::SDL3)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(tests
        tests/CPU_test.cpp
        tests/Display_test.cpp
        tests/Keypad_test.cpp
        src/CPU.cpp
        src/Display.cpp
        src/Keypad.cpp
)
target_link_libraries(tests gtest_main)

include(GoogleTest)
gtest_discover_tests(tests)
