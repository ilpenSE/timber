# toolchains/msvc-wine
# msvc-wine: https://github.com/mstorsjo/msvc-wine
#
# Default values: (Define them according to your environment)
# ARCH = x86_64
# ROOT = $HOME/.local/msvc-wine
#
# Define ARCH with these: x64 -> amd64; arm64 -> arm64
# Define ROOT to your msvc-wine path like: $HOME/.local/msvc-wine
# it must contain bin, include and lib folders.

if (NOT DEFINED ARCH)
  set(ARCH "x64" CACHE STRING "cpu architecture")
endif()

if (NOT DEFINED ROOT)
  set(ROOT "$ENV{HOME}/.local/msvc-wine" CACHE STRING "msvc-wine installation path")
endif()

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR ${ARCH})
set(CMAKE_C_COMPILER   ${ROOT}/bin/${ARCH}/cl)
set(CMAKE_CXX_COMPILER ${ROOT}/bin/${ARCH}/cl)
set(CMAKE_RC_COMPILER  ${ROOT}/bin/${ARCH}/rc)
set(CMAKE_LINKER       ${ROOT}/bin/${ARCH}/link)
set(CMAKE_AR           ${ROOT}/bin/${ARCH}/lib)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_CROSSCOMPILING_EMULATOR wine)
