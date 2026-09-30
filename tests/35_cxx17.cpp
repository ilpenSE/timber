#include <timber.hpp>

int main() {
  timber::Instance inst(TIMBER_BLOCK_POLICY);
  inst.add_stdout()->set_format("$T $L: $M");
  if (!inst.init()) {
    return 1;
  }
  inst.info() << "Hello, Stream!";
  inst.info("Hello, World!");
  inst.warning() << "Hello" << " World!";
}
