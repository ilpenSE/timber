# Timber - A Logging Library

- It's asynchronous, thread-safe, portable, language-agnostic logger library written in C.
- You can have syntax sugar and custom formatting for C++ in timber.hpp
- This library is revamped version of [logger.h](https://github.com/ilpenSE/logger.git)

## Usage

> [!WARNING]
> You have to use min C11 or C++11 to use it like stb-style

- This is so called stb-style header-only library

- Define `TIMBER_IMPLEMENTATION` macro then include the header like this:
```c
#define TIMBER_IMPLEMENTATION
#include "timber.h"
```
- In C++, you can include C++ "bindings" it's recommended to use it
- But you have to have timber.h with timber.hpp in same folder
  (or you can change the include in timber.hpp or maybe bake C header inside hpp)
```cpp
#define TIMBER_IMPLEMENTATION
#include "timber.hpp"
```


- Allocate an instance then initialize it:
```c
int main() {
  Timber
}
```
