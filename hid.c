#include "hid.h"

#include <stdio.h>
#include <stdlib.h>
//#include <sys/types.h>
//#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include <dirent.h>
#include <string.h>

#include "structure.h"

DECLARE_LINKED_LIST_OPERATIONS(RbxHidDevice*,HidDevice);

int create_hid(void)
{
  fprintf(stderr, "Raberix: starting HID... ");
  // TODO
  fprintf(stderr, "done\n");
  return 1;
}

void destroy_hid(void)
{
  fprintf(stderr, "Raberix: stopping HID... ");
  // TODO
  fprintf(stderr, "done\n");
}

LINKED_LIST_TYPE(HidDevice) get_hid_device_list(void)
{
  DIR *dev_dir = opendir("/dev");
  struct dirent *dent;
  DECLARE_LINKED_LIST(HidDevice,devices);
  RbxHidDevice *device;

  while ((dent = readdir(dev_dir)))
  {
    if (strncmp(dent->d_name, "rawhid", 6))
      continue;
    device = malloc(sizeof(RbxHidDevice));
    if (!device)
    {
      fprintf(stderr, "Could not allocate memory for more elements in raw HID device list.\n");
      return LINKED_LIST(devices);
    }
    device->name = "";
    device->device_id = 0x0000;
    device->vendor_id = 0x0000;
    LINKED_LIST_ADD(HidDevice,devices,device)
  }

  if (closedir(dev_dir) < 0)
    fprintf(stderr, "Error closing /dev directory.\n");
  
  return LINKED_LIST(devices);
}

void delete_hid_device_list(LINKED_LIST_TYPE(HidDevice) *devices_ref)
{
  LINKED_LIST_TYPE(HidDevice) LINKED_LIST(devices) = *devices_ref;
  LINKED_LIST_DESTROY(HidDevice,devices);
  (*devices_ref) = LINKED_LIST(devices);
}

RbxHidReport* get_hid_report_desc(RbxHidDevice* device)
{
  return NULL; // TODO
}

void delete_hid_report_desc(RbxHidReport* report_desc)
{
  // TODO
}
