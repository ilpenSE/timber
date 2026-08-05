#!/bin/bash

set -e

PROJECT_ROOT="$(dirname $(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd))"
HEADER="${PROJECT_ROOT}/include/timber.h"
SOURCE="${PROJECT_ROOT}/src/timber.c"
OUTPUT="${PROJECT_ROOT}/build/timber.h"

mkdir -p "${PROJECT_ROOT}/build/"

cp "$HEADER" "$OUTPUT"
printf "\n#ifdef TIMBER_IMPLEMENTATION\n\n" >> "$OUTPUT"
cat "$SOURCE" >> "$OUTPUT"
printf "\n#endif /* TIMBER_IMPLEMENTATION */\n" >> "$OUTPUT"

sed -i '/^#include <timber\.h>$/d' "$OUTPUT"
echo "${OUTPUT}:0: generated single-header"
