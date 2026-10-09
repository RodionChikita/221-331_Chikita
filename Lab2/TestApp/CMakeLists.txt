cmake_minimum_required(VERSION 3.16)
project(Lab2TestApp C)

set(CMAKE_C_STANDARD 11)

add_executable(test_file_io test_file_io.c)

if(MSVC)
    target_compile_definitions(test_file_io PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(test_file_io PRIVATE /W4)
else()
    target_compile_options(test_file_io PRIVATE -Wall -Wextra)
endif()
