cmake_minimum_required(VERSION 3.27)
project(PPPJP)

set(CMAKE_CXX_STANDARD 23)

add_executable(PPPJP src/main.cpp
        src/tokenizer.hpp
        src/parser.hpp
        src/ir_generator.hpp
        src/arena_allocator.hpp
        src/asm_generator.hpp
)

configure_file(printer.asm printer.asm COPYONLY)

add_subdirectory(tests)
