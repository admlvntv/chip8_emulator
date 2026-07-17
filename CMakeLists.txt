cmake_minimum_required(VERSION 4.2)
project(chip8_emulator)

set(CMAKE_CXX_STANDARD 20)

add_executable(chip8_emulator src/main.cpp
        src/cpu.cpp
        src/cpu.h
        src/display.cpp
        src/display.h
)
