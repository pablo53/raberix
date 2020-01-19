#ifndef _UTIL_H
#define _UTIL_H

/* All the following functions return a new pointer. The caller is its owner */
/* and must release memory with free(). Returns NULL on error.               */
char * sduplicate1(const char *s); // duplicates the string
char * sconcatenate2(const char * s1, const char * s2); // concatenates two strings

#endif
