#include <timber.hpp>

int main() {
  timber::Timber lg;
  lg.add_sink(stdout);
  if (!lg.init()) return 1;
  lg.info("Hello, {}!", "Formatted");
}
