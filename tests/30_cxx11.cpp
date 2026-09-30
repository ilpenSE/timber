#include <timber.hpp>

int main() {
  timber::Instance inst(TIMBER_BLOCK_POLICY);
  inst.add_stderr();
  if (!inst.init()) {
    return 1;
  }
  inst.info() << "Hello, Stream!";
  inst.info("Hello, World!");
  inst.warning() << "Hello" << " World!";
} // RAII calls timber_destroy automatically here
