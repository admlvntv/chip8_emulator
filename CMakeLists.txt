cmake_minimum_required(VERSION 3.28)
project(chip8_emulator)

set(CMAKE_CXX_STANDARD 20)

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
)

include(FetchContent)
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
