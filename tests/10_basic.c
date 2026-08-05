#include <timber.h>

int main(void) {
  Timber timber = {0};
  timber_add_stdout_sink(&timber);
  if (!timber_init(&timber)) return 1;
  timber_info(&timber, "Hello, World!");
  timber_infof(&timber, "Hello, %s!", "World");
  if (!timber_destroy(&timber)) return 2;
}
