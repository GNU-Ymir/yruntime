CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project(gyruntime)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -ggdb -Wall -fno-exceptions")

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
)

add_library (gyruntime ${SRC})
# target_link_libraries (gyruntime INTERFACE) #
install (TARGETS gyruntime DESTINATION /usr/lib/)
include_directories(${CMAKE_SOURCE_DIR}/include/)













