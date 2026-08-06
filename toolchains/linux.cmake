# toolchains/linux.cmake
#
# Default values: (Define them according to your environment)
# ARCH = x86_64
# ROOT = /
#
# Define ARCH with these: x86_64, x86_64h -> amd64; arm64, o64, arm64e, aarch64 -> arm64
# Define ROOT if you're not using linux machine to something else
# that directory must contain bin/ folder which contains <x86_64|aarch64>-linux-gnu-* utils
# and also include/ and lib/ folders for system includes and libraries

if (NOT DEFINED ARCH)
  set(ARCH "x86_64" CACHE STRING "cpu architecture")
endif()

if (NOT DEFINED ROOT)
  set(ROOT "/usr" CACHE STRING "linux utils installation path (root directory)")
endif()

set(LINUX_TRIPLE "${ARCH}-linux-gnu")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR ${ARCH})

set(CMAKE_C_COMPILER   ${ROOT}/bin/${LINUX_TRIPLE}-gcc)
set(CMAKE_CXX_COMPILER ${ROOT}/bin/${LINUX_TRIPLE}-g++)
set(CMAKE_LINKER       ${ROOT}/bin/${LINUX_TRIPLE}-ld)
set(CMAKE_AR           ${ROOT}/bin/${LINUX_TRIPLE}-ar)
set(CMAKE_STRIP        ${ROOT}/bin/${LINUX_TRIPLE}-strip)
set(CMAKE_RANLIB       ${ROOT}/bin/${LINUX_TRIPLE}-ranlib)

set(CMAKE_FIND_ROOT_PATH "${ROOT}/${LINUX_TRIPLE}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
