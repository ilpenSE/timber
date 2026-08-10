#!/bin/bash

set -e

if [[ $1 == "help" ]]; then
  echo "Usage: $0 <target> <variant>"
  echo "Targets: linux-x64 osx-x64 mingw-x64"
  echo "         linux-aarch64 osx-aarch64 mingw-aarch64"
  echo ""
  echo "Variants:"
  echo "O0: No optimization, with debug info"
  echo "O3: Full optimizations, without debug info"
  echo "ASAN: No optimization, ASan + UBSan"
  echo "TSAN: No optimization, TSan + UBSan"
  exit 0
fi

TESTS=(
  "10_basic"
  "15_basic_mt"
  "20_stress_test_st"
  "25_stress_test_mt"
  "30_cxx11"
  "35_cxx17"
  "37_cxx20"
)

TARGET="${1:-linux-x64}"
VARIANT="${2:-O0}"

INCLUDE_FLAGS=(
  "-I../include"
)

CXX_INCLUDE_FLAGS=(
  "-I../bindings/c++"
)

COMMON_CFLAGS=(
  "-Wall"
  "-Wextra"
  "-ggdb"
)

COMMON_LDFLAGS=()

BUILD_FOLDER="build/$TARGET/$VARIANT"

# ------------------------------------------------------------
# Target
# ------------------------------------------------------------

case "$TARGET" in
  linux-x64)
    CC="cc"
    CXX="c++"
    LIB_DIR="../build/linux-x64"
    ;;

  osx-x64)
    CC="x86_64-apple-darwin25.1-clang"
    CXX="x86_64-apple-darwin25.1-clang++"
    LIB_DIR="../build/osx-x64"
    ;;

  osx-aarch64)
    CC="aarch64-apple-darwin25.1-clang"
    CXX="aarch64-apple-darwin25.1-clang++"
    LIB_DIR="../build/osx-aarch64"
    ;;

  mingw-x64)
    CC="x86_64-w64-mingw32-clang"
    CXX="x86_64-w64-mingw32-clang++"
    LIB_DIR="../build/mingw-x64"
    ;;

  mingw-aarch64)
    CC="aarch64-w64-mingw32-clang"
    CXX="aarch64-w64-mingw32-clang++"
    LIB_DIR="../build/mingw-aarch64"
    ;;

  *)
    echo "Unknown target: $TARGET"
    exit 1
    ;;
esac

COMMON_LDFLAGS+=(
  "-L$LIB_DIR"
)

LIBS=(
  "-l:libtimber.a"
)

# ------------------------------------------------------------
# Variant
# ------------------------------------------------------------

case "$VARIANT" in
  O0)
    CFLAGS=(
      "${COMMON_CFLAGS[@]}"
      "-O0"
    )
    LDFLAGS=()
    ;;

  O3)
    CFLAGS=(
      "${COMMON_CFLAGS[@]}"
      "-O3"
    )
    LDFLAGS=()
    ;;

  ASAN)
    CFLAGS=(
      "${COMMON_CFLAGS[@]}"
      "-O0"
      "-fsanitize=address,undefined"
      "-fno-omit-frame-pointer"
      "-fno-sanitize-recover=undefined"
    )
    LDFLAGS=(
      "-fsanitize=address,undefined"
    )
    ;;

  TSAN)
    CFLAGS=(
      "${COMMON_CFLAGS[@]}"
      "-O0"
      "-fsanitize=thread,undefined"
      "-fno-omit-frame-pointer"
      "-fno-sanitize-recover=undefined"
    )
    LDFLAGS=(
      "-fsanitize=thread,undefined"
    )
    ;;

  *)
    echo "Unknown variant: $VARIANT"
    exit 1
    ;;
esac

# ------------------------------------------------------------
# Standard detection
# ------------------------------------------------------------

detect_std() {
  if [[ $1 =~ cxx([0-9]+)$ ]]; then
    IS_CXX=1
    CXX_STD="${BASH_REMATCH[1]}"
  else
    IS_CXX=0
    C_STD="11"
  fi
}

# ------------------------------------------------------------
# Build
# ------------------------------------------------------------

mkdir -p "$BUILD_FOLDER"

for t in "${TESTS[@]}"; do
  detect_std "$t"

  if [[ $IS_CXX == 1 ]]; then
    echo "==> $TARGET / $VARIANT / C++$CXX_STD / $t"

    "$CXX" \
      "${INCLUDE_FLAGS[@]}" \
      "${CXX_INCLUDE_FLAGS[@]}" \
      "${CFLAGS[@]}" \
      "-std=c++$CXX_STD" \
      -o "$BUILD_FOLDER/$t" \
      "$t.cpp" \
      "${COMMON_LDFLAGS[@]}" \
      "${LDFLAGS[@]}" \
      "${LIBS[@]}"
  else
    echo "==> $TARGET / $VARIANT / C11 / $t"

    "$CC" \
      "${INCLUDE_FLAGS[@]}" \
      "${CFLAGS[@]}" \
      -o "$BUILD_FOLDER/$t" \
      "$t.c" \
      "${COMMON_LDFLAGS[@]}" \
      "${LDFLAGS[@]}" \
      "${LIBS[@]}"
  fi
done
