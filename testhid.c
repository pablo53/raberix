

#include "hid.h"
#include "test.h"

int test_create_destroy(void)
{
  int res = create_hid();
  if (!res)
    return res;
  destroy_hid();
  return 0;
}

int main(int args, char *argv[])
{
  return call_test(test_create_destroy, "Create/Destroy HID");
}
