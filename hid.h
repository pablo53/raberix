#ifndef _HID_H
#define _HID_H

#include <stddef.h>

#include "structure.h"
#include "def.h"

EXPORT_SYMBOL int create_hid(void);
EXPORT_SYMBOL void destroy_hid(void);

typedef struct
{
  size_t size;
  char *data;
} RbxHidReport;

typedef struct
{
  char *name;
  char *file_name;
  unsigned short int vendor_id;
  unsigned short int device_id;
} RbxHidDevice;

DECLARE_LINKED_LIST_TYPE(RbxHidDevice*,HidDevice);

EXPORT_SYMBOL LINKED_LIST_TYPE(HidDevice) get_hid_device_list(void); // the caller is the owner of the returning (null-terminated) list and must destroy it with destroy_hid_device_list()
EXPORT_SYMBOL void delete_hid_device_list(LINKED_LIST_TYPE(HidDevice) *devices_ref);
EXPORT_SYMBOL RbxHidReport* get_hid_report_desc(RbxHidDevice* device); // the caller is the owner and must destroy it with delete_hid_report_desc()
EXPORT_SYMBOL void delete_hid_report_desc(RbxHidReport* report_desc);

#endif
