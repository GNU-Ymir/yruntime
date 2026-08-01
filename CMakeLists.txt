CMAKE_MINIMUM_REQUIRED(VERSION 3.22)
PROJECT(Midgard C)

# Single source of truth for the ymir bootstrap version this library is built/versioned
# against (see YMIR_VERSION at the repo root - also read by the release/CI workflows and the
# `install` script). The static lib name suffix and install include dir have always tracked
# only major.minor (e.g. "1.1"), not the full patch version.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/YMIR_VERSION" YMIR_VERSION_LINES)
foreach(_line ${YMIR_VERSION_LINES})
  if(_line MATCHES "^YMIR_BOOTSTRAP_VERSION=(.*)$")
    set(YMIR_BOOTSTRAP_VERSION "${CMAKE_MATCH_1}")
  endif()
endforeach()
if(NOT DEFINED YMIR_BOOTSTRAP_VERSION)
  message(FATAL_ERROR "YMIR_BOOTSTRAP_VERSION not found in ${CMAKE_CURRENT_SOURCE_DIR}/YMIR_VERSION")
endif()
string(REGEX MATCH "^[0-9]+\\.[0-9]+" YMIR_SHORT_VERSION "${YMIR_BOOTSTRAP_VERSION}")

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
  COMMAND ${CMAKE_YMIR_COMPILER} ${CMAKE_YMIR_RELEASE_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/midgard/__lib__.yr -c -o ${CMAKE_CURRENT_BINARY_DIR}/__lib_release_.o -fdoc midgard.json
)

add_custom_target (lib_tests ALL
  COMMENT "Building __lib_tests_"
  COMMAND ${CMAKE_YMIR_COMPILER} ${CMAKE_YMIR_RELEASE_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/test-rt/__lib__.yr  -c  -g -o ${CMAKE_CURRENT_BINARY_DIR}/__lib_test_.o
)

add_custom_target (lib_debug ALL
  COMMENT "Building __lib_debug_"
  COMMAND ${CMAKE_YMIR_COMPILER} ${CMAKE_YMIR_DEBUG_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/midgard/__lib__.yr  -c  -o ${CMAKE_CURRENT_BINARY_DIR}/__lib_debug_.o
)

add_custom_target (midgard_tests ALL
  COMMENT "Building midgard tests"
  COMMAND ${CMAKE_YMIR_COMPILER} ${CMAKE_YMIR_DEBUG_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/tests/__lib__.yr -fversion=DEBUG_LIB  -o ${CMAKE_CURRENT_BINARY_DIR}/midgard_tests -funittest -nostdinc -nomidgardlib -I ${CMAKE_CURRENT_SOURCE_DIR}/midgard -lgc  ${CMAKE_CURRENT_BINARY_DIR}/libgymidgard-debug_${YMIR_SHORT_VERSION}.a ${CMAKE_CURRENT_BINARY_DIR}/libgymidgard-tests_${YMIR_SHORT_VERSION}.a  ${CMAKE_CURRENT_BINARY_DIR}/libgymidgard-debug_${YMIR_SHORT_VERSION}.a
)


set_source_files_properties (
  ${CMAKE_CURRENT_BINARY_DIR}/__lib_release_.o
  ${CMAKE_CURRENT_BINARY_DIR}/__lib_debug_.o
  ${CMAKE_CURRENT_BINARY_DIR}/__lib_test_.o
  PROPERTIES GENERATED TRUE
)

add_library (gymidgard-release_${YMIR_SHORT_VERSION} STATIC ${SRC_CORE_C} ${SRC_STD_C} ${SRC_RUNTIME} ${CMAKE_CURRENT_BINARY_DIR}/__lib_release_.o)
add_library (gymidgard-debug_${YMIR_SHORT_VERSION} STATIC ${SRC_CORE_C} ${SRC_STD_C} ${SRC_RUNTIME} ${CMAKE_CURRENT_BINARY_DIR}/__lib_debug_.o)
add_library (gymidgard-tests_${YMIR_SHORT_VERSION} STATIC ${SRC_UNITTEST} ${CMAKE_CURRENT_BINARY_DIR}/__lib_test_.o)

add_library (runtime STATIC ${SRC_RUNTIME})

add_dependencies (gymidgard-release_${YMIR_SHORT_VERSION} lib_release)
add_dependencies (gymidgard-debug_${YMIR_SHORT_VERSION} lib_debug)
add_dependencies (gymidgard-tests_${YMIR_SHORT_VERSION} lib_tests)
add_dependencies (midgard_tests gymidgard-debug_${YMIR_SHORT_VERSION} gymidgard-tests_${YMIR_SHORT_VERSION} gymidgard-release_${YMIR_SHORT_VERSION})

target_include_directories (gymidgard-release_${YMIR_SHORT_VERSION} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/)
target_include_directories (gymidgard-debug_${YMIR_SHORT_VERSION} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/)
target_include_directories (runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/)

install (TARGETS gymidgard-debug_${YMIR_SHORT_VERSION} DESTINATION /usr/lib/)
install (TARGETS gymidgard-release_${YMIR_SHORT_VERSION} DESTINATION /usr/lib/)
install (TARGETS gymidgard-tests_${YMIR_SHORT_VERSION} DESTINATION /usr/lib/)
