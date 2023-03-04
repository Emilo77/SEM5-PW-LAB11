cmake_minimum_required (VERSION 3.1)
project (MIMUW-SIGNALS C)

set(CMAKE_CXX_STANDARD "17")
set(CMAKE_C_STANDARD "11")
set(CMAKE_C_FLAGS "-g -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter")

add_library(err err.c)

function(add_example_program target_name)
  add_executable(${target_name} "${target_name}.c")
  target_link_libraries(${target_name} err)
endfunction()

add_example_program(simple_handler)
add_example_program(signal_mask)
add_example_program(more_info)
add_example_program(terminator)
