#include "test.h"

#include <stdio.h>

int call_test(TestFunc func, const char * desc)
{
  int res;
  fprintf(stdout, " *** Testing %s... ", desc);
  res = func();
  fprintf(stdout, res ? "failed.\n" : "done.\n");
  return res;
}
