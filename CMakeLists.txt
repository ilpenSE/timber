cmake_minimum_required(VERSION 3.15)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

project(Timber LANGUAGES C CXX)
enable_testing()

set(TEST_SOURCES
  tests/10_basic.c
  tests/15_basic_mt.c
  tests/20_stress_test_st.c
  tests/25_stress_test_mt.c
  tests/30_cxx11.cpp
  tests/35_cxx17.cpp
  tests/37_cxx20.cpp
)

# Shared (dynamic) library
add_library(timber_shared SHARED src/timber.c)
target_include_directories(timber_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_options(timber_shared PRIVATE
  $<$<C_COMPILER_ID:MSVC>:/experimental:c11atomics>
)

target_compile_definitions(timber_shared
  PRIVATE TIMBER_BUILD
  PUBLIC  TIMBER_SHARED
)

set_target_properties(timber_shared PROPERTIES
  OUTPUT_NAME timber
  C_VISIBILITY_PRESET hidden
)

# Static library
add_library(timber_static STATIC src/timber.c)
target_include_directories(timber_static PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_compile_options(timber_static PRIVATE
  $<$<C_COMPILER_ID:MSVC>:/experimental:c11atomics>
)

if (MSVC)
  set_target_properties(timber_static PROPERTIES
    OUTPUT_NAME timber_static
  )
else()
  set_target_properties(timber_static PROPERTIES
    OUTPUT_NAME timber
  )
endif()

# Tests
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(VARIANT_NAMES O0 O3 asan)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(VARIANT_NAMES O0 O3)
else()
  set(VARIANT_NAMES O0 O3 asan tsan)
endif()

set(FLAGS_O0   "-O0")
set(FLAGS_O3   "-O3")
set(FLAGS_asan "-O0;-fsanitize=address,undefined;-fno-omit-frame-pointer;-fno-sanitize-recover=undefined")
set(FLAGS_tsan "-O0;-fsanitize=thread,undefined;-fno-omit-frame-pointer;-fno-sanitize-recover=undefined")

function(detect_standard SRC_FILE OUT_STD OUT_IS_CXX)
  get_filename_component(NAME ${SRC_FILE} NAME_WE)
  if(NAME MATCHES "cxx([0-9]+)$")
    set(${OUT_STD} ${CMAKE_MATCH_1} PARENT_SCOPE)
    set(${OUT_IS_CXX} TRUE PARENT_SCOPE)
  else()
    set(${OUT_STD} 11 PARENT_SCOPE)
    set(${OUT_IS_CXX} FALSE PARENT_SCOPE)
  endif()
endfunction()

foreach(SRC ${TEST_SOURCES})
  get_filename_component(BASE_NAME ${SRC} NAME_WE)
  detect_standard(${SRC} STD IS_CXX)

  foreach(VARIANT ${VARIANT_NAMES})
    set(TARGET_NAME ${BASE_NAME}_${VARIANT})
    add_executable(${TARGET_NAME} ${SRC})
    target_link_libraries(${TARGET_NAME} PRIVATE timber_static)

    target_compile_options(${TARGET_NAME} PRIVATE ${FLAGS_${VARIANT}})
    target_link_options(${TARGET_NAME} PRIVATE ${FLAGS_${VARIANT}})

    if(IS_CXX)
      set_target_properties(${TARGET_NAME} PROPERTIES
        CXX_STANDARD ${STD} CXX_STANDARD_REQUIRED ON)
      target_include_directories(${TARGET_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bindings/c++/>
      )
    else()
      set_target_properties(${TARGET_NAME} PROPERTIES
        C_STANDARD ${STD} C_STANDARD_REQUIRED ON)
    endif()

    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
    set_tests_properties(${TARGET_NAME} PROPERTIES LABELS ${VARIANT})
  endforeach()
endforeach()
