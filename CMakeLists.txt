CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(Midgard C)
SET(CMAKE_YMIR_COMPILER "/home/emile/ymir/bootstrap/gyc/gcc-install/bin/gyc")
set(CMAKE_YMIR_COMPILE_OBJECT "${CMAKE_YMIR_COMPILER} ${CMAKE_YMIR_FLAGS} <FLAGS>  -c <SOURCE> -o <OBJECT>")
set(CMAKE_YMIR_CREATE_STATIC_LIBRARY "<CMAKE_AR> rc <TARGET> <LINK_FLAGS> <OBJECTS>")

IF (WIN32) 
  set(CMAKE_YMIR_FLAGS "${CMAKE_C_FLAGS} -O3 -fPIC -nostdinc -nomidgardlib -fdoc -fno-reflect")
ELSE()
  set(CMAKE_YMIR_FLAGS "${CMAKE_C_FLAGS} -fPIC -nostdinc -nomidgardlib -fdoc -g")
ENDIF()

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -fPIC -g")

FILE (
  GLOB_RECURSE
  SRC_LIB
  midgard/__lib__.yr
  )

FILE (
  GLOB_RECURSE
  SRC_TESTS
  tests/__lib__.yr
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


SET_SOURCE_FILES_PROPERTIES(${SRC_LIB} ${SRC_UNITTEST_YR} ${SRC_TESTS} PROPERTIES LANGUAGE YMIR)

add_library (gymidgard-bootstrap-release STATIC ${SRC_LIB} ${SRC_CORE_C} ${SRC_STD_C} ${SRC_RUNTIME})
add_library (gymidgard-bootstrap-debug STATIC ${SRC_LIB} ${SRC_CORE_C} ${SRC_STD_C} ${SRC_RUNTIME})
target_compile_options (gymidgard-bootstrap-debug PRIVATE -fversion=DEBUG_LIB)


add_custom_command(TARGET gymidgard-bootstrap-release
  PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E touch_nocreate ${CMAKE_CURRENT_SOURCE_DIR}/midgard/__lib__.yr)


add_custom_command(TARGET gymidgard-bootstrap-debug
  PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E touch_nocreate ${CMAKE_CURRENT_SOURCE_DIR}/midgard/__lib__.yr)

add_library (gymidgard-bootstrap-tests STATIC ${SRC_UNITTEST} ${SRC_UNITTEST_YR})
target_compile_options (gymidgard-bootstrap-tests PRIVATE -I ${CMAKE_CURRENT_SOURCE_DIR}/midgard/__lib__.yr)
target_link_libraries (gymidgard-bootstrap-tests gymidgard-bootstrap-release)
IF(UNIX) 
    install (TARGETS gymidgard-bootstrap-debug DESTINATION /usr/lib/)
    install (TARGETS gymidgard-bootstrap-release DESTINATION /usr/lib/)
    install (TARGETS gymidgard-bootstrap-tests DESTINATION /usr/lib/)
ELSE(UNIX)
    install (TARGETS gymidgard-bootstrap-debug DESTINATION /mingw64/lib/)
    install (TARGETS gymidgard-bootstrap-release DESTINATION /mingw64/lib/)
    install (TARGETS gymidgard-bootstrap-tests DESTINATION /mingw64/lib/)
ENDIF(UNIX)

add_library (midgard_tests STATIC ${SRC_LIB})
add_custom_command(TARGET midgard_tests
  PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E touch_nocreate ${CMAKE_CURRENT_SOURCE_DIR}/tests/__lib__.yr
  COMMAND ${CMAKE_YMIR_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}/tests/__lib__.yr -o ${CMAKE_CURRENT_BINARY_DIR}/midgard_tests -funittest -nomidgardlib -nostdinc -I ${CMAKE_CURRENT_SOURCE_DIR}/midgard -lgymidgard-bootstrap-debug -lgc -lgymidgard-bootstrap-tests)

add_custom_command(TARGET midgard_tests
  POST_BUILD
  COMMAND rm libmidgard_tests.a)
