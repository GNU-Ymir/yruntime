CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project(gyruntime)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")

file(  
  GLOB_RECURSE
  SRC
  src/*.c
)

add_compile_definitions(DEBUG_RUNTIME=1)


add_library (gyruntime SHARED ${SRC})
target_link_libraries(gyruntime PUBLIC -lbfd)
install (TARGETS gyruntime DESTINATION /usr/lib/)
include_directories(${CMAKE_SOURCE_DIR}/include/)










