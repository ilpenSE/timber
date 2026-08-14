#include <timber.h>

int main(void) {
  Timber *t = timber_alloc();
  timber_set_format(t, "");
  if (!timber_init(t)) return 1;

  timber_info(t, "");
  timber_infof(t, "");
  timber_infof(t, "%s", "");

  if (!timber_destroy(t)) return 2;
  timber_free(t);
}
