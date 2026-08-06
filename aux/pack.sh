#!/bin/bash
# Usage:
# ./build.sh --target linux --arch x64 --release
# ./build.sh -t linux -a x64 -r

set -e

RELEASE=0
ARCH="x64"
TARGET="linux"
declare -A TARGETS=( ["linux"]="linux"
                     ["osx"]="osx"
                     ["msvc"]="msvc"
                     ["mingw"]="mingw" )

while [[ $# -gt 0 ]]; do
  case "$1" in
    --release | -r) RELEASE=1 ;;
    --target | -t)
      if [[ -v TARGETS[$2] ]]; then
        TARGET="${TARGETS[$2]}"
      elif [[ $2 == "all" ]]; then
        TARGET="all"
      else
        echo "Invalid target: '$2', defaulting to linux"
        echo "Available targets:"
        echo "For Linux (GCC): linux"
        echo "For MacOS (OSXCross): osx"
        echo "For Windows (LLVM MinGW): mingw"
        echo "For Windows (MSVC Wine): msvc"
      fi
      ;;
    --arch | -a)
      case "$2" in
        x64 | x86_64 | amd64) ARCH="x64" ;;
        arm64 | aarch64) ARCH="aarch64" ;;
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

bash_clr_red='\e[0;32m'
bash_clr_rst='\e[0m'

info() {
  echo -e "${bash_clr_red}==> [INFO] ${bash_clr_rst}$1"
}

CWD="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$CWD")"
BUILD_FOLDER="$PROJECT_ROOT/build/"

PRESETS=( "${TARGETS[@]/%/-${ARCH}}" )
PRESET=${TARGET}-$ARCH
bash "${CWD}/generate.sh"

# build_for <preset = linux-x64>
build_for() {
  local preset="$1"
  local build_folder="${BUILD_FOLDER}${preset}"
  local config=Debug
  [[ $RELEASE == 1 ]] && config=Release

  info "Building for preset: $preset"
  info "Building into: $build_folder"
  info "Config type: $config"
  cmake -B "$build_folder" -S "$PROJECT_ROOT" \
        --preset "$preset" \
        -DCMAKE_BUILD_TYPE="$config"
  cmake --build "$build_folder" --preset "$preset"
}

# pack_for <preset = linux-x64>
pack_for() {
  local preset="$1"
  local build_folder="${BUILD_FOLDER}${preset}"
  local stage="${BUILD_FOLDER}/stage/${preset}"

  rm -rf "$stage"
  mkdir -p "$stage/include" "$stage/lib"

  # Headers
  cp -v "${PROJECT_ROOT}/include/timber.h" "$stage/include/"
  cp -v "${PROJECT_ROOT}/bindings/c++/timber.hpp" "$stage/include/" 2>/dev/null || true

  # Libraries
  find "$build_folder" -maxdepth 1 -type f \
       \( -name '*.so' -o -name '*.dll' -o -name '*.a' \
       -o -name '*.dylib' -o -name '*.lib' -o -name '*.pdb' \) \
       -exec cp {} "$stage/lib/" \;

  if [[ $preset =~ '(msvc|mingw)-.*' ]]; then
    (
      cd "${BUILD_FOLDER}/stage"
      local output="${BUILD_FOLDER}${preset}.zip"
      zip -r "$output" "$preset"
      info "Created zip into: $output"
    )
  else
    local output="${BUILD_FOLDER}${preset}.tar.gz"
    tar -C "${BUILD_FOLDER}/stage" \
        -czf "$output" "$preset"
    info "Created tarball into: $output"
  fi
}

if [[ $TARGET == "all" ]]; then
  for preset in "${PRESETS[@]}"; do
    build_for $preset
    pack_for $preset
  done
else
  build_for $PRESET
  pack_for $PRESET
fi
