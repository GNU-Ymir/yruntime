CMAKE_MINIMUM_REQUIRED(VERSION 3.22)
PROJECT(Midgard C)

# The static lib name suffix and the install include dir name midgard's *own* release - VERSION
# at the repo root, which is also the git tag each release is cut under - and have always tracked
# only its major.minor (e.g. "1.2"), not the full patch version. It has to be this and not
# YMIR_BOOTSTRAP_VERSION from YMIR_VERSION (the gyc release this library is compiled *with*, a
# separate fact that drifts ahead of it): gyc resolves -lgymidgard-* and its bundled include dir
# from the midgard version it was built against, so naming these after anything else produces a
# library no compiler looks for.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" MIDGARD_VERSION LIMIT_COUNT 1)
string(STRIP "${MIDGARD_VERSION}" MIDGARD_VERSION)
if(NOT MIDGARD_VERSION MATCHES "^[0-9]+\\.[0-9]+")
  message(FATAL_ERROR "${CMAKE_CURRENT_SOURCE_DIR}/VERSION does not hold a <major>.<minor>... version: '${MIDGARD_VERSION}'")
endif()
string(REGEX MATCH "^[0-9]+\\.[0-9]+" MIDGARD_SHORT_VERSION "${MIDGARD_VERSION}")

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
  COMMAND ${CMAKE_YMIR_COMPILER} ${CMAKE_YMIR_DEBUG_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/tests/__lib__.yr -fversion=DEBUG_LIB  -o ${CMAKE_CURRENT_BINARY_DIR}/midgard_tests -funittest -nostdinc -nomidgardlib -I ${CMAKE_CURRENT_SOURCE_DIR}/midgard -I ${CMAKE_CURRENT_SOURCE_DIR}/test-rt -lgc  ${CMAKE_CURRENT_BINARY_DIR}/libgymidgard-debug_${MIDGARD_SHORT_VERSION}.a ${CMAKE_CURRENT_BINARY_DIR}/libgymidgard-tests_${MIDGARD_SHORT_VERSION}.a  ${CMAKE_CURRENT_BINARY_DIR}/libgymidgard-debug_${MIDGARD_SHORT_VERSION}.a
)


set_source_files_properties (
  ${CMAKE_CURRENT_BINARY_DIR}/__lib_release_.o
  ${CMAKE_CURRENT_BINARY_DIR}/__lib_debug_.o
  ${CMAKE_CURRENT_BINARY_DIR}/__lib_test_.o
  PROPERTIES GENERATED TRUE
)

add_library (gymidgard-release_${MIDGARD_SHORT_VERSION} STATIC ${SRC_CORE_C} ${SRC_STD_C} ${SRC_RUNTIME} ${CMAKE_CURRENT_BINARY_DIR}/__lib_release_.o)
add_library (gymidgard-debug_${MIDGARD_SHORT_VERSION} STATIC ${SRC_CORE_C} ${SRC_STD_C} ${SRC_RUNTIME} ${CMAKE_CURRENT_BINARY_DIR}/__lib_debug_.o)
add_library (gymidgard-tests_${MIDGARD_SHORT_VERSION} STATIC ${SRC_UNITTEST} ${CMAKE_CURRENT_BINARY_DIR}/__lib_test_.o)

add_library (runtime STATIC ${SRC_RUNTIME})

add_dependencies (gymidgard-release_${MIDGARD_SHORT_VERSION} lib_release)
add_dependencies (gymidgard-debug_${MIDGARD_SHORT_VERSION} lib_debug)
add_dependencies (gymidgard-tests_${MIDGARD_SHORT_VERSION} lib_tests)
add_dependencies (midgard_tests gymidgard-debug_${MIDGARD_SHORT_VERSION} gymidgard-tests_${MIDGARD_SHORT_VERSION} gymidgard-release_${MIDGARD_SHORT_VERSION})

target_include_directories (gymidgard-release_${MIDGARD_SHORT_VERSION} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/)
target_include_directories (gymidgard-debug_${MIDGARD_SHORT_VERSION} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/)
target_include_directories (runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/)

install (TARGETS gymidgard-debug_${MIDGARD_SHORT_VERSION} DESTINATION /usr/lib/)
install (TARGETS gymidgard-release_${MIDGARD_SHORT_VERSION} DESTINATION /usr/lib/)
install (TARGETS gymidgard-tests_${MIDGARD_SHORT_VERSION} DESTINATION /usr/lib/)
