#include <timber.h>

int main(void)
{
  Timber *timber = timber_alloc();
  // Chain API
  timber_set_format(timber_add_stdout_sink(timber), "$T [$L] $M");

  if (!timber_init(timber)) return 1;
  timber_info(timber, "Hello, World!");
  timber_infof(timber, "Hello, %s!", "World");
  if (!timber_destroy(timber)) return 2;
  timber_free(timber);
}
