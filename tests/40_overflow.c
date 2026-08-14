#include <timber.h>

int main(void) {
  Timber *t = timber_alloc();
  timber_add_stdout(t);
  timber_set_format(t, "$$ ====> $T [$L/$L] $M");
  if (!timber_init(t)) return 1;

  // Plain
  // 256
  timber_info(t, "PLAIN 256 BYTESlo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hall");

  // 255
  timber_info(t, "PLAIN 255 BYTESlo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hal");

  // 257
  timber_info(t, "PLAIN 257 BYTESlo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo");

  // Variadics
  // 256
  timber_infof(t, "%s", "VARIADIC 256 BYTESHallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hall");

  // 255
  timber_infof(t, "%s", "VARIADIC 255 BYTESHallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hal");

  // 257
  timber_infof(t, "%s", "VARIADIC 257 BYTESHallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo!Hallo");

  if (!timber_destroy(t)) return 2;
  timber_free(t);
}
