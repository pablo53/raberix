

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

int test_retrieve_hid_device_list(void)
{
  int res = create_hid();
  if (!res)
    return res;
  DECLARE_LINKED_LIST(HidDevice,hid_devices) = get_hid_device_list();
  DECLARE_LINKED_LIST_LOOP(HidDevice);

  LINKED_LIST_FOREACH(HidDevice,hid_devices,element)
  {
    printf("Found Raw HID device file: '%s'.", element->file_name);
  }

  delete_hid_device_list(&LINKED_LIST(hid_devices));
  destroy_hid();
  return 0;
}

int main(int args, char *argv[])
{
  return call_test(test_create_destroy, "Create/Destroy HID")
      || call_test(test_retrieve_hid_device_list, "Retrieve HID device list");
}
