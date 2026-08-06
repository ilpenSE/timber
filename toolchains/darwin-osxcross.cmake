# toolchains/amd64-darwin-osxcross
# osxcross: https://github.com/tpoechtrager/osxcross
#
# Default values: (Define them according to your environment)
# ARCH = x86_64
# ROOT = $HOME/.local/osxcross
# SDK_VERSION = 26.1
# DARWIN_VERSION = 25.1
#
# Define ARCH with these: x86_64, x86_64h -> amd64; arm64, o64, arm64e, aarch64 -> arm64
# Define ROOT to your osxcross path like: $HOME/.local/osxcross
# This ROOT must contain SDK folder with inside: 'MacOSX26.1.sdk' for MacOS 26 for example
# and bin/, include/ and lib/ folders

if (NOT DEFINED ARCH)
  set(ARCH "x86_64" CACHE STRING "osxcross architecture (x86_64 / arm64)")
endif()

if (NOT DEFINED ROOT)
  set(ROOT "$ENV{HOME}/.local/osxcross" CACHE PATH "osxcross installation path")
endif()

if (NOT DEFINED SDK_VERSION)
  set(SDK_VERSION "26.1" CACHE STRING "macos/osx version")
endif()

if (NOT DEFINED DARWIN_VERSION)
  set(DARWIN_VERSION "25.1" CACHE STRING "darwin version")
endif()

set(ENV{OSXCROSS_HOST} "${ARCH}-apple-darwin${DARWIN_VERSION}")
set(ENV{OSXCROSS_TARGET_DIR} "${ROOT}")
set(ENV{OSXCROSS_TARGET} "darwin${DARWIN_VERSION}")
set(ENV{OSXCROSS_SDK} "${ROOT}/SDK/MacOSX${SDK_VERSION}.sdk")

include("${ROOT}/toolchain.cmake")
