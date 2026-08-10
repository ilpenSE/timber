# Timber - A Logging Library

- It's asynchronous, thread-safe, portable, language-agnostic logger library written in C.
- You can have syntax sugar and custom formatting for C++ in timber.hpp
- This library is revamped version of [logger.h](https://github.com/ilpenSE/logger.git)

## Quick Start

- Build the project for your system:
```bash
cmake -B build
cmake --build build
```

- Now your static and dynamic library is in build folder
- Copy them into your library folder
- Copy the header located in include folder to your include folder
- Build your executable with these:
```bash
# Dynamic linking
cc -Iinclude/ main.c -o main -Llib/ -ltimber -Wl,-rpath,'lib'
# Static linking
cc -Iinclude/ main.c -o main -Llib/ -l:libtimber.a
```

- If you wanna use it stb-style single-header run this bash script: `aux/generate.sh`
- It will generate stb-style header in build folder.
- You can use it like this:
```bash
#define TIMBER_IMPLEMENTATION
#include "timber.h"

int main(void) {
  Timber *t = timber_alloc();
  if (!timber_init(t)) return 1;
  timber_info(t, "Hello!");
  if (!timber_destroy(t)) return 2;
  timber_free(t);
}
```
