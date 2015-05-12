#include <random.h>
#include "tests/lib.h"
#include "tests/main.h"

int
main (void) 
{

  test_name = "test";

  msg ("begin");
  random_init (0);
  test_main ();
  msg ("end");
  return 0;
}
