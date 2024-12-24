CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(Midgard C)
SET(CMAKE_C_COMPILER "/usr/bin/gyc")
SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -fPIC -nostdinc -nomidgardlib")

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


SET_SOURCE_FILES_PROPERTIES(${SRC_CORE} ${SRC_STD} ${SRC_ETC} ${SRC_UNITTEST_YR} overview.yr PROPERTIES LANGUAGE C)

add_library (gymidgard-release STATIC ${SRC_CORE} ${SRC_STD} ${SRC_ETC} ${SRC_CORE_C} ${SRC_STD_C} ${SRC_RUNTIME} overview.yr)

add_library (gymidgard-debug STATIC ${SRC_CORE} ${SRC_STD} ${SRC_ETC} ${SRC_CORE_C} ${SRC_STD_C} ${SRC_RUNTIME} overview.yr)
target_compile_options (gymidgard-debug PRIVATE -fversion=DEBUG_LIB)

add_library (gymidgard-tests STATIC ${SRC_UNITTEST} ${SRC_UNITTEST_YR})

IF(UNIX) 
    install (TARGETS gymidgard-debug DESTINATION /usr/lib/)
    install (TARGETS gymidgard-release DESTINATION /usr/lib/)
    install (TARGETS gymidgard-tests DESTINATION /usr/lib/)
ELSE(UNIX)
    install (TARGETS gymidgard-debug DESTINATION /mingw64/lib/)
    install (TARGETS gymidgard-release DESTINATION /mingw64/lib/)
    install (TARGETS gymidgard-tests DESTINATION /mingw64/lib/)
ENDIF(UNIX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})














