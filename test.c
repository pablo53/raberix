#include "test.h"

#include <stdio.h>

int call_test(TestFunc func, const char * desc)
{
  int res;
  fprintf(stdout, " >>> Testing '%s':\n", desc);
  res = func();
  fprintf(stdout, " <<< '%s' %s", desc, res ? "failed.\n" : "done.\n");
  return res;
}
