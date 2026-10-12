#!/bin/bash

HERE="$(readlink -f $(dirname $0))"
mkdir -p "$HERE/timber/lib"

# Linux
cp -v "$HERE/../../build/linux-x64/libtimber.so" "$HERE/timber/lib/libtimber-x86_64.so"
cp -v "$HERE/../../build/linux-aarch64/libtimber.so" "$HERE/timber/lib/libtimber-aarch64.so"

# MacOS
cp -v "$HERE/../../build/osx-x64/libtimber.dylib" "$HERE/timber/lib/libtimber-x86_64.dylib"
cp -v "$HERE/../../build/osx-aarch64/libtimber.dylib" "$HERE/timber/lib/libtimber-aarch64.dylib"

# Windows (MSVC)
cp -v "$HERE/../../build/msvc-x64/timber.dll" "$HERE/timber/lib/timber-x86_64.dll"
cp -v "$HERE/../../build/msvc-aarch64/timber.dll" "$HERE/timber/lib/timber-aarch64.dll"
