#ifndef _TEST_H
#define _TEST_H

typedef int (*TestFunc)(void);

int call_test(TestFunc func, const char * desc);

#endif
