CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project(gymidgard)

file(
  GLOB_RECURSE
  SRC
  src/*.c
)

add_library (gymidgard ${SRC})
target_link_libraries (gymidgard -lpthread -lgc)
install (TARGETS gymidgard DESTINATION /usr/lib/)
include_directories(${CMAKE_SOURCE_DIR}/include/)













