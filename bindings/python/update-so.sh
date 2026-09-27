#!/bin/bash

HERE="$(readlink -f $(dirname $0))"
cp -v "$HERE/../../build/linux-x64/libtimber.so" "$HERE/timber"
