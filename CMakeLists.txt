CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
PROJECT(Midgard C)

set(CMAKE_YMIR_COMPILER "/home/emile/ymir/bootstrap/gyc/gcc-install/bin/gyc")
set(CMAKE_YMIR_RELEASE_FLAGS "-fPIC" "-nostdinc" "-nomidgardlib" "-fdoc" "-g" "-I${CMAKE_CURRENT_SOURCE_DIR}/midgard/")
set(CMAKE_YMIR_DEBUG_FLAGS "${CMAKE_YMIR_RELEASE_FLAGS}" "-fversion=DEBUG_LIB")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -fPIC -g")


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
  SRC_UNITTEST
  test-rt/*.c
  )


add_custom_target (lib_release
  COMMENT "Building __lib_release_"
  COMMAND ${CMAKE_YMIR_COMPILER} ${CMAKE_YMIR_DEBUG_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/midgard/__lib__.yr -c -o ${CMAKE_CURRENT_BINARY_DIR}/__lib_release_.o
)

add_custom_target (lib_tests
  COMMENT "Building __lib_tests_"
  COMMAND ${CMAKE_YMIR_COMPILER} ${CMAKE_YMIR_RELEASE_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/test-rt/__lib__.yr  -c -o ${CMAKE_CURRENT_BINARY_DIR}/__lib_test_.o
)

add_custom_target (lib_debug
  COMMENT "Building __lib_debug_"
  COMMAND ${CMAKE_YMIR_COMPILER} ${CMAKE_YMIR_RELEASE_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/midgard/__lib__.yr  -c -o ${CMAKE_CURRENT_BINARY_DIR}/__lib_debug_.o
)

add_custom_target (midgard_tests
  COMMENT "Building midgard tests"
  COMMAND ${CMAKE_YMIR_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}/tests/__lib__.yr -fversion=DEBUG_LIB -g -o ${CMAKE_CURRENT_BINARY_DIR}/midgard_tests -funittest -nostdinc -nomidgardlib -I ${CMAKE_CURRENT_SOURCE_DIR}/midgard -lgc  ${CMAKE_CURRENT_BINARY_DIR}/libgymidgard-bootstrap-debug.a ${CMAKE_CURRENT_BINARY_DIR}/libgymidgard-bootstrap-tests.a  ${CMAKE_CURRENT_BINARY_DIR}/libgymidgard-bootstrap-debug.a
)


add_library (gymidgard-bootstrap-release STATIC ${SRC_CORE_C} ${SRC_STD_C} ${SRC_RUNTIME} __lib_release_.o)
add_library (gymidgard-bootstrap-debug STATIC ${SRC_CORE_C} ${SRC_STD_C} ${SRC_RUNTIME} __lib_debug_.o)
add_library (gymidgard-bootstrap-tests STATIC ${SRC_UNITTEST} __lib_test_.o)

add_dependencies (gymidgard-bootstrap-release lib_release)
add_dependencies (gymidgard-bootstrap-debug lib_debug)
add_dependencies (gymidgard-bootstrap-tests lib_tests)
add_dependencies (midgard_tests gymidgard-bootstrap-debug gymidgard-bootstrap-tests gymidgard-bootstrap-release)


install (TARGETS gymidgard-bootstrap-debug DESTINATION /usr/lib/)
install (TARGETS gymidgard-bootstrap-release DESTINATION /usr/lib/)
install (TARGETS gymidgard-bootstrap-tests DESTINATION /usr/lib/)
