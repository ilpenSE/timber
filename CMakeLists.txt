cmake_minimum_required(VERSION 3.15)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

project(Timber LANGUAGES C)

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
