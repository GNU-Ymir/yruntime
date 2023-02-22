CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(Midgard C)
SET(CMAKE_YMIR_COMPILER "gyc")	
set(CMAKE_YMIR_COMPILE_OBJECT "${CMAKE_YMIR_COMPILER} ${CMAKE_YMIR_FLAGS} <FLAGS> -I<INCLUDES> -c <SOURCE> -o <OBJECT>")
set(CMAKE_YMIR_CREATE_STATIC_LIBRARY "<CMAKE_AR> rc <TARGET> <LINK_FLAGS> <OBJECTS>")

IF (WIN32) 
  set(CMAKE_YMIR_FLAGS "${CMAKE_C_FLAGS} -O3 -fPIC -nostdinc -nomidgardlib -fdoc -fno-reflect")
ELSE()
  set(CMAKE_YMIR_FLAGS "${CMAKE_C_FLAGS} -O3 -fPIC -nostdinc -nomidgardlib -fdoc")
ENDIF()

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -fPIC")


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

FILE (
  GLOB_RECURSE
  SRC_UNITTEST
  test-rt/*.c
  )

FILE (
  GLOB_RECURSE
  SRC_UNITTEST_YR
  test-rt/*.yr
  )


SET_SOURCE_FILES_PROPERTIES(${SRC_CORE} ${SRC_STD} ${SRC_ETC} ${SRC_UNITTEST_YR} overview.yr PROPERTIES LANGUAGE YMIR)

add_library (gymidgard STATIC ${SRC_CORE} ${SRC_STD} ${SRC_ETC} ${SRC_CORE_C} ${SRC_STD_C} ${SRC_RUNTIME} overview.yr)
add_library (gymidgard-tests STATIC ${SRC_UNITTEST} ${SRC_UNITTEST_YR})

IF(UNIX) 
    install (TARGETS gymidgard DESTINATION /usr/lib/)
    install (TARGETS gymidgard-tests DESTINATION /usr/lib/)
ELSE(UNIX)
    install (TARGETS gymidgard DESTINATION /mingw64/lib/)
    install (TARGETS gymidgard-tests DESTINATION /mingw64/lib/)
ENDIF(UNIX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})














