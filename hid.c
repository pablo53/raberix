#include "hid.h"

#include <stdio.h>
#include <stdlib.h>
//#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "structure.h"
#include "util.h"

DECLARE_LINKED_LIST_OPERATIONS(HidDevice);

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
  DECLARE_LINKED_LIST(HidDevice,devices) = NULL;
  RbxHidDevice *device;
  int fd, res;
  char *buf = malloc(4096);

  if (!buf)
  {
    fprintf(stderr, "Could not allocate memory for HID data buffer.\n");
    return LINKED_LIST(devices);
  }
  while ((dent = readdir(dev_dir)))
  {
    if (strncmp(dent->d_name, "hidraw", 6))
      continue;
    device = malloc(sizeof(RbxHidDevice));
    if (!device)
    {
      fprintf(stderr, "Could not allocate memory for more elements in raw HID device list.\n");
      free(buf);
      return LINKED_LIST(devices);
    }
    LINKED_LIST_ADD(HidDevice,devices,device)
    device->name = NULL;
    device->file_name = sconcatenate2("/dev/", dent->d_name);
    device->device_id = 0x0000;
    device->vendor_id = 0x0000;
    fd = open(device->file_name, O_RDWR|O_NONBLOCK);
    if (fd < 0)
    {
      fprintf(stderr, "Could not open the device '%s'.\n", device->file_name);
      if (errno == EACCES)
        fprintf(stderr, "You do not have enough permissions to open '%s'. "
                        "If You are Ubuntu user, consider adding Your user to group 'plugdev' (if not yet done so) and "
                        "adding as root the following line to a rules file in /etc/udev/rules.d:\n"
                        "  KERNEL==\"hidraw*\", SUBSYSTEM==\"hidraw\", MODE=\"0664\", GROUP=\"plugdev\"\n"
                        "Next, unplug and plug again Your device(s).\n", device->file_name);
      continue;
    }
    res = ioctl(fd, HIDIOCGRAWNAME(4096), buf);
    if (res >= 0)
      device->name = sduplicate1(buf);
    else
      fprintf(stderr, "Could not read the device '%s' name.\n", device->file_name);
    close(fd);
  }
  if (closedir(dev_dir) < 0)
    fprintf(stderr, "Error closing '/dev' directory.\n");
  
  free(buf);
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
