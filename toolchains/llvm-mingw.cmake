# toolchains/llvm-mingw
# llvm-mingw: https://github.com/mstorsjo/llvm-mingw
#
# Default values: (Define them according to your environment)
# ARCH = x86_64
# ROOT = $HOME/.local/llvm-mingw
#
# Define ARCH with these: x86_64, x86_64h -> amd64; arm64, o64, arm64e, aarch64 -> arm64
# Define ROOT to your llvm-mingw path like: $HOME/.local/llvm-mingw
# it must contain bin, include and lib folders.

if (NOT DEFINED ARCH)
  set(ARCH "x86_64" CACHE STRING "cpu architecture")
endif()

if (NOT DEFINED ROOT)
  set(ROOT "$ENV{HOME}/.local/llvm-mingw" CACHE STRING "llvm-mingw installation path")
endif()

set(LLVM_MINGW_TRIPLE "${ARCH}-w64-mingw32")

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR ${ARCH})
set(CMAKE_C_COMPILER   ${ROOT}/bin/${LLVM_MINGW_TRIPLE}-clang)
set(CMAKE_CXX_COMPILER ${ROOT}/bin/${LLVM_MINGW_TRIPLE}-clang++)
set(CMAKE_RC_COMPILER  ${ROOT}/bin/${LLVM_MINGW_TRIPLE}-windres)
set(CMAKE_LINKER       ${ROOT}/bin/${LLVM_MINGW_TRIPLE}-ld)
set(CMAKE_AR           ${ROOT}/bin/${LLVM_MINGW_TRIPLE}-ar)
set(CMAKE_STRIP        ${ROOT}/bin/${LLVM_MINGW_TRIPLE}-strip)
set(CMAKE_RANLIB       ${ROOT}/bin/${LLVM_MINGW_TRIPLE}-ranlib)

set(CMAKE_FIND_ROOT_PATH ${ROOT}/${LLVM_MINGW_TRIPLE})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_CROSSCOMPILING_EMULATOR wine)
