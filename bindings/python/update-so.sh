#!/bin/bash

HERE="$(readlink -f $(dirname $0))"

# Linux
cp -v "$HERE/../../build/linux-x64/libtimber.so" "$HERE/timber/libtimber-x86_64.so"
cp -v "$HERE/../../build/linux-aarch64/libtimber.so" "$HERE/timber/libtimber-aarch64.so"

# MacOS
cp -v "$HERE/../../build/osx-x64/libtimber.dylib" "$HERE/timber/libtimber-x86_64.dylib"
cp -v "$HERE/../../build/osx-aarch64/libtimber.dylib" "$HERE/timber/libtimber-aarch64.dylib"

# Windows (MSVC)
cp -v "$HERE/../../build/msvc-x64/timber.dll" "$HERE/timber/timber-x86_64.dll"
cp -v "$HERE/../../build/msvc-aarch64/timber.dll" "$HERE/timber/timber-aarch64.dll"
