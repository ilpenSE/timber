#!/bin/bash
# In order to run this script without failing,
# you need to have aarch64-apple-darwin25.1-* and mingw toolchain in your path
# It's designed for my environment if you wanna compile for your machine, use
# ./buic build system and provide target (specify prefixes)

set -xe

mkdir -p artifacts

# Linux/BSD x64
rm -rf .build/
./buic posix
tar czf artifacts/x86_64-linux-gnu.tar.gz --transform 's,^.build,timber,' .build/*

# Apple aarch64
setup_cc osx
rm -rf .build/
./buic apple
tar czf artifacts/aarch64-apple.tar.gz --transform 's,^.build,timber,' .build/*

# MinGW x64
rm -rf .build/
./buic mingw
zip artifacts/x86_64-win32-mingw.tar.gz .build/*

# MSVC x64
rm -rf .build/
./buic mingw
zip artifacts/x86_64-win32-msvc.tar.gz .build/*
