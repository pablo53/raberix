#ifndef _DATAREF_H
#define _DATAREF_H

/* This is an implementation of cache for XPLMDataRefs. Searching them is very expensive, */
/* hence, it is advisable to hold them here once they have been found.                    */

#include "XPLM/XPLMDataAccess.h"

int create_dataref(void);
void destroy_dataref(void);

int add_data_ref(const char * data_ref_name); // returns data ref index to be used next in get_data_ref() or -1 on error (e.g. not found)
XPLMDataRef get_data_ref(int data_ref_index);

#endif
