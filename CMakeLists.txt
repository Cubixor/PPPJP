cmake_minimum_required(VERSION 3.27)
project(PPPJP)

set(CMAKE_CXX_STANDARD 23)

add_executable(PPPJP src/main/main.cpp
        src/main/tokenizer.hpp
        src/main/parser.hpp
        src/main/ir_generator.hpp
        src/main/arena_allocator.hpp
        src/main/asm_generator.hpp
)

configure_file(printer.asm printer.asm COPYONLY)

add_subdirectory(src/tests)
