#!/bin/bash

set -e

TESTS=(
  "10_basic"
  "15_basic_mt"
  "20_stress_test_st"
  "25_stress_test_mt"
  "30_cxx11"
  "35_cxx17"
  "37_cxx20"
)

CC="cc"
CXX="c++"
CFLAGS="-Wall -Wextra -ggdb -O0 -I../include"
LDFLAGS="-L../build/linux-x64"
LIBS="-l:libtimber.a"
BUILD_FOLDER="build"

function detect_std() {
  if [[ $1 =~ cxx([0-9]+)$ ]]; then
    IS_CXX=1
    CXX_STD="${BASH_REMATCH[1]}"
  else
    IS_CXX=0
    C_STD="11"
  fi
}

mkdir -p $BUILD_FOLDER
for t in "${TESTS[@]}"; do
  detect_std "$t"
  if [[ $IS_CXX == 1 ]]; then
    $CXX $CFLAGS -I../bindings/c++ -std=c++$CXX_STD -o $BUILD_FOLDER/$t $t.cpp $LDFLAGS $LIBS
  else
    $CC $CFLAGS -o $BUILD_FOLDER/$t $t.c $LDFLAGS $LIBS
  fi
done
