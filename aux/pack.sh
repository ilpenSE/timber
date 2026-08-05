#!/bin/bash
# Usage:
# ./build.sh --target linux --arch x64 --release
# ./build.sh -t linux -a x64 -r

set -e

RELEASE=0
ARCH="x64"
TARGET="linux-gcc"
declare -A TARGETS=( ["mingw"]="win-mingw"
                     ["msvc"]="win-msvc"
                     ["darwin"]="darwin-clang"
                     ["osx"]="darwin-clang"
                     ["apple"]="darwin-clang"
                     ["linux"]="linux-gcc"
                     ["all"]="all" )

while [[ $# -gt 0 ]]; do
  case "$1" in
    --release | -r) RELEASE=1 ;;
    --target | -t)
      if [[ -v TARGETS[$2] ]]; then
        TARGET="${TARGETS[$2]}"
      else
        echo "Invalid target: '$2', defaulting to linux"
        echo "Available targets:"
        echo "For Linux (GCC): linux"
        echo "For Apple's OS (MacOS/OSX) (Clang): darwin or apple or osx"
        echo "For Windows (MinGW): mingw"
        echo "For Windows (MSVC): msvc"
      fi
      ;;
    --arch | -a)
      case "$2" in
        x64 | x86_64 | amd64) ARCH="x64" ;;
        arm64 | aarch64) ARCH="arm64" ;;
        *)
          echo "Invalid architecture: '$2', defaulting to x64..."
          echo "Available options:"
          echo "For x64: x64 or x86_64 or amd64"
          echo "For ARM64: aarch64 or arm64"
          ;;
      esac
      shift
      ;;
  esac
  shift
done

CWD="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$CWD")"
BUILD_FOLDER="$PROJECT_ROOT/build/"

PLATFORMS=( "${TARGETS[@]/#/${ARCH}-}" )
PLATFORM=${ARCH}-$TARGET
bash "${CWD}/generate.sh"

# build_for <platform = x64-linux-gcc>
build_for() {
  build_folder="${BUILD_FOLDER}$1"
  toolchain_file="${PROJECT_ROOT}/toolchains/$1.cmake"
  config=Debug
  [[ $RELEASE == 1 ]] && config=Release

  cmake -B "$build_folder" -S "$PROJECT_ROOT" \
        -DCMAKE_TOOLCHAIN_FILE="$toolchain_file" \
        -DCMAKE_BUILD_TYPE="$config"
  cmake --build "$build_folder" --config "$config"
}

# pack_for <platform = x64-linux-gcc>
pack_for() {
  local platform="$1"
  local build_folder="${BUILD_FOLDER}${platform}"
  local stage="${BUILD_FOLDER}/stage/${platform}"

  rm -rf "$stage"
  mkdir -p "$stage/include" "$stage/lib"

  # Headers
  cp "${PROJECT_ROOT}/include/timber.h" "$stage/include/"
  cp "${PROJECT_ROOT}/bindings/c++/timber.hpp" "$stage/include/" 2>/dev/null || true

  # Libraries
  find "$build_folder" -maxdepth 1 -type f \
       \( -name '*.so' -o -name '*.dll' -o -name '*.a' \
       -o -name '*.dylib' -o -name '*.lib' -o -name '*.pdb' \) \
       -exec cp {} "$stage/lib/" \;

  if [[ $platform == *-win-* ]]; then
    (
      cd "${BUILD_FOLDER}/stage"
      zip -r "${BUILD_FOLDER}/${platform}.zip" "$platform"
    )
  else
    tar -C "${BUILD_FOLDER}/stage" \
        -czf "${BUILD_FOLDER}/${platform}.tar.gz" \
        "$platform"
  fi
}

# pack_for() {
#   bash "${CWD}/generate.sh"
#   build_folder="${BUILD_FOLDER}$1"
#   files="$(realpath --relative-to="$BUILD_FOLDER" \
#        $(find "$build_folder" -maxdepth 1 -type f \( -name '*.so' -o -name '*.dll' -o -name '*.a' \
#        -o -name '*.dylib' -o -name '*.lib' \)))"
#   tar -C "$BUILD_FOLDER" \
#     -czf "${BUILD_FOLDER}/$1.tar.gz" \
#     $files
# }

if [[ $TARGET == "all" ]]; then
  for platform in "${PLATFORMS[@]}"; do
    build_for $platform
    pack_for $platform
  done
else
  build_for $PLATFORM
  pack_for $PLATFORM
fi
