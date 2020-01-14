#ifndef _COMMANDREF_H
#define _COMMANDREF_H

#include "XPLM/XPLMUtilities.h"

typedef int (*RbxCmdHandler)(int cmd_ref_index, int cmd_hdl_id, int is_before, int phase, void *handler_data); // the handler returns 1 to let processing the command, or 0 otherwise
typedef void (*RbxCmdHandlerDestructor)(int cmd_ref_index, int cmd_hdl_id, int is_before, void *handler_data); // it should free the necessary memeory

int create_commandref(void);
void destroy_commandref(void);

int find_cmd_ref(const char * cmd_ref_name); // returns command ref index to be used next in get_cmd_ref() or -1 on error (e.g. not found)
XPLMCommandRef get_cmd_ref(int cmd_ref_index);
int put_cmd_ref(const char * cmd_ref_name, const char * cmd_ref_desc);
int add_cmd_handler(RbxCmdHandler cmd_handler, RbxCmdHandlerDestructor cmd_destr, int cmd_ref_index, int is_before, void *handler_data); // returns command handler reference ID (not command ref index!)
void remove_cmd_handler(int cmd_hdl_id);

#endif
