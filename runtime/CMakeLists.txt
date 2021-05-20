CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project(gyruntime)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

file(  
  GLOB_RECURSE
  SRC
  src/*.c
)

add_compile_definitions(DEBUG_RUNTIME=1)


add_library (gyruntime SHARED ${SRC})

install (TARGETS gyruntime DESTINATION /usr/lib/)
include_directories(${CMAKE_SOURCE_DIR}/include/)
add_custom_command (TARGET gyruntime POST_BUILD
  COMMAND patchelf --add-needed libbfd.so libgyruntime.so
  COMMAND echo "[101%] Patched"
  )









