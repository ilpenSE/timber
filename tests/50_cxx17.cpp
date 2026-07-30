#define TIMBER_IMPLEMENTATION
#include "../timber.hpp"

int main() {
  timber::Timber inst(TIMBER_BLOCK_POLICY);
  inst.add_sink(stdout);
  if (!inst.init()) {
    return 1;
  }
  inst.info() << "Hello, Stream!";
  inst.info("Hello, World!");
  inst.warning() << "Hello" << " World!";
}
