CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(Midgard C)
SET(CMAKE_C_COMPILER "gyc")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -nostdinc -fdoc")

FILE (
  GLOB_RECURSE
  SRC_CORE
  core/*.yr
  )

FILE (
  GLOB_RECURSE
  SRC_STD
  std/*.yr
  )


FILE (
  GLOB_RECURSE
  SRC_ETC
  etc/*.yr
  )


FILE (
  GLOB_RECURSE
  SRC_CORE_C
  core/*.c
  )

FILE (
  GLOB_RECURSE
  SRC_STD_C
  std/*.c
  )

SET_SOURCE_FILES_PROPERTIES(${SRC_CORE} ${SRC_STD} ${SRC_ETC} PROPERTIES LANGUAGE C)

add_library (gymidgard ${SRC_CORE} ${SRC_STD} ${SRC_ETC} ${SRC_CORE_C} ${SRC_STD_C})
target_link_libraries (gymidgard -lpthread -lgc -lgyruntime -lgmp)
install (TARGETS gymidgard DESTINATION /usr/lib/)
include_directories(${CMAKE_SOURCE_DIR})














