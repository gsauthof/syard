cmake_minimum_required(VERSION 3.0.2)

project(syard CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

add_executable(ut
  test/main.cc
  test/syard.cc
  syard/syard.cc
  )
set_property(TARGET ut PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

add_custom_target(check COMMAND ut)
