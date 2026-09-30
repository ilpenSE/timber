#include <timber.hpp>

int main() {
  timber::Instance lg;
  lg.add_stdout();
  if (!lg.init()) return 1;
  lg.info("Hello, {}!", "Formatted");
}
