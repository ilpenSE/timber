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
  timber_add_stdout(t);
  if (!timber_init(t)) return 1;
  timber_info(t, "Hello!");
  if (!timber_destroy(t)) return 2;
  timber_free(t);
}
```

## Documentation

- This project uses [Doxygen](https://www.doxygen.nl/) for documentation
- Install `doxygen` tool to your system then run this command on project root:
```bash
doxygen
```

- Doxygen'll create docs folder with html, latex, xml and manpages inside.
- For HTML: Open `docs/html/index.html` in your browser.
- For PDF: You have to have pdflatex and texlive tools installed in your system. Then run Makefile in `docs/latex`
- For XML: Open `docs/html/index.xml` in your browser or any XML editor.

## Tests

- Tests are located in `tests` folder.
- It has `test.py` script which compiles then runs tests.
- It has linux and mingw target for x86_64.
- Use MinGW on Windows, use linux on Linux/BSD
- There're `thread.h` and `benchmark.h` mini-libraries for
abstractions (time.h and pthreads/winthreads)

- Run using linux target and O3 variant:
```bash
./test.py -p linux -v O3
```

- There're `O0` (default one), `O3`, `ASAN` and `TSAN` variants
- These variants determines compilation flags
- Binaries are located in `tests/build/platform/variant/` (e.g.: `tests/build/linux/ASAN`)
- To add a new test, just add a new entry to `TESTS` dictionary with specifying standart C or C++ version.
- Python script compiles then runs all tests sequentially.

## About

- Author: [ilpeN](https://github.com/ilpenSE)
- Used min C11 standart, compatible with >=C++11
- Version: 1.3.0
