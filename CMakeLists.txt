CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(Midgard C)
SET(CMAKE_YMIR_COMPILER "gyc")	
set(CMAKE_YMIR_COMPILE_OBJECT "${CMAKE_YMIR_COMPILER} ${CMAKE_YMIR_FLAGS} <FLAGS> -I<INCLUDES> -c <SOURCE> -o <OBJECT>")
set(CMAKE_YMIR_CREATE_STATIC_LIBRARY "<CMAKE_AR> rc <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_YMIR_FLAGS "${CMAKE_C_FLAGS} -g -nostdinc -nomidgardlib")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")


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


FILE (
  GLOB_RECURSE
  SRC_RUNTIME
  c/*.c
  )


SET_SOURCE_FILES_PROPERTIES(${SRC_CORE} ${SRC_STD} ${SRC_ETC} PROPERTIES LANGUAGE YMIR)

add_library (gymidgard STATIC ${SRC_CORE} ${SRC_STD} ${SRC_ETC} ${SRC_CORE_C} ${SRC_STD_C} ${SRC_RUNTIME})
target_link_libraries (gymidgard -lpthread -lgc -lgmp -lm)

install (TARGETS gymidgard DESTINATION /usr/lib/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})














