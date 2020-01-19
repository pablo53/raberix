#include "util.h"

#include <stdlib.h>
#include <string.h>

char * sduplicate1(const char *s)
{
  return strdup(s); // TODO: POSIX only - implement it for other systems
}

char * sconcatenate2(const char * s1, const char * s2)
{
  char * s = malloc(strlen(s1) + strlen(s2) + 1);
  if (s)
    strcat(strcpy(s, s1), s2);
  return s;
}
