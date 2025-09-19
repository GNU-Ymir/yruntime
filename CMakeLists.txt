CMAKE_MINIMUM_REQUIRED(VERSION 3.22)
PROJECT(Midgard C)

set(CMAKE_YMIR_COMPILER "gyc")
set(CMAKE_YMIR_RELEASE_FLAGS "-fPIC" "-nostdinc" "-nomidgardlib" "-fv" "-I${CMAKE_CURRENT_SOURCE_DIR}/midgard/")
set(CMAKE_YMIR_DEBUG_FLAGS "${CMAKE_YMIR_RELEASE_FLAGS}" "-fversion=DEBUG_LIB" "-g" "-fv")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -fPIC -O3 -g")


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
  rt/*.c
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


add_custom_target (lib_release ALL
  COMMENT "Building __lib_release_"
  COMMAND ${CMAKE_YMIR_COMPILER} ${CMAKE_YMIR_RELEASE_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/midgard/__lib__.yr -c -o ${CMAKE_CURRENT_BINARY_DIR}/__lib_release_.o
)

add_custom_target (lib_tests ALL
  COMMENT "Building __lib_tests_"
  COMMAND ${CMAKE_YMIR_COMPILER} ${CMAKE_YMIR_RELEASE_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/test-rt/__lib__.yr  -c  -o ${CMAKE_CURRENT_BINARY_DIR}/__lib_test_.o
)

add_custom_target (lib_debug ALL
  COMMENT "Building __lib_debug_"
  COMMAND ${CMAKE_YMIR_COMPILER} ${CMAKE_YMIR_DEBUG_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/midgard/__lib__.yr  -c  -o ${CMAKE_CURRENT_BINARY_DIR}/__lib_debug_.o
)

add_custom_target (midgard_tests ALL
  COMMENT "Building midgard tests"
  COMMAND ${CMAKE_YMIR_COMPILER} ${CMAKE_YMIR_DEBUG_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/tests/__lib__.yr -fversion=DEBUG_LIB  -o ${CMAKE_CURRENT_BINARY_DIR}/midgard_tests -funittest -nostdinc -nomidgardlib -I ${CMAKE_CURRENT_SOURCE_DIR}/midgard -lgc  ${CMAKE_CURRENT_BINARY_DIR}/libgymidgard-debug.a ${CMAKE_CURRENT_BINARY_DIR}/libgymidgard-tests.a  ${CMAKE_CURRENT_BINARY_DIR}/libgymidgard-debug.a
)


add_library (gymidgard-release STATIC ${SRC_CORE_C} ${SRC_STD_C} ${SRC_RUNTIME} ${CMAKE_CURRENT_BINARY_DIR}/__lib_release_.o)
add_library (gymidgard-debug STATIC ${SRC_CORE_C} ${SRC_STD_C} ${SRC_RUNTIME} ${CMAKE_CURRENT_BINARY_DIR}/__lib_debug_.o)
add_library (gymidgard-tests STATIC ${SRC_UNITTEST} ${CMAKE_CURRENT_BINARY_DIR}/__lib_test_.o)

add_library (runtime STATIC ${SRC_RUNTIME})

add_dependencies (gymidgard-release lib_release)
add_dependencies (gymidgard-debug lib_debug)
add_dependencies (gymidgard-tests lib_tests)
add_dependencies (midgard_tests gymidgard-debug gymidgard-tests gymidgard-release)

target_include_directories (gymidgard-release PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/)
target_include_directories (gymidgard-debug PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/)
target_include_directories (runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/)

install (TARGETS gymidgard-debug DESTINATION /usr/lib/)
install (TARGETS gymidgard-release DESTINATION /usr/lib/)
install (TARGETS gymidgard-tests DESTINATION /usr/lib/)
