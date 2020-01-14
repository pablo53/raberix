#include "commandref.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structure.h"

#include "XPLM/XPLMUtilities.h"


typedef struct {
  RbxCmdHandler cmd_handler;
  RbxCmdHandlerDestructor cmd_destr;
  void *cmd_ref;
  int cmd_ref_index;
  int cmd_hdl_id;
  int is_before;
  void *handler_data;
} RbxCmdCallbackData;

static int cmd_callback(XPLMCommandRef cmd_ref, XPLMCommandPhase cmd_phase, void *data);

DECLARE_SYNC_LIST(XPLMCommandRef,cmdref)
DECLARE_SYNC_LIST(RbxCmdCallbackData*,cmdhdldat)

DECLARE_INSERT_INTO_SYNC_LIST(XPLMCommandRef,cmdref)
DECLARE_INSERT_INTO_SYNC_LIST(RbxCmdCallbackData*,cmdhdldat)

int create_commandref(void)
{
  fflush(stdout);
  fprintf(stderr, "Raberix: preparing XPLM command ref structures... ");
  int mres;
  mres = INIT_SYNC_LIST(cmdref);
  if (mres)
  {
    fprintf(stderr, "failed (cannot init command refs, err %i)\n", mres);
    return 0;
  }
  mres = INIT_SYNC_LIST(cmdhdldat);
  if (mres)
  {
    fprintf(stderr, "failed (cannot init command handlers, err %i)\n", mres);
    BREAK_SYNC_LIST(cmdref);
    return 0;
  }
  fprintf(stderr, "done\n");
  return 1;
}

void destroy_commandref(void)
{
  DESTROY_SYNC_LIST(cmdhdldat);
  DESTROY_SYNC_LIST(cmdref);
}

int find_cmd_ref(const char * cmd_ref_name)
{
  int idx = -1;
  LOCK_SYNC_LIST(cmdref);
  XPLMCommandRef cmd_ref = XPLMFindCommand(cmd_ref_name);
  if (cmd_ref)
  {
    idx = INSERT_INTO_SYNC_LIST(cmdref,cmd_ref);
    if (idx >= 0)
      fprintf(stderr, "Raberix: XPLM Command ref '%s' found.\n", cmd_ref_name);
    else
      fprintf(stderr, "Raberix: XPLM Command ref '%s' not found!\n", cmd_ref_name);
  }
  UNLOCK_SYNC_LIST(cmdref);
  return idx;  
}

XPLMCommandRef get_cmd_ref(int cmd_ref_index)
{
  XPLMCommandRef cmd_ref = NULL;
  LOCK_SYNC_LIST(cmdref);
  if (cmd_ref_index >= 0 && cmd_ref_index < SYNC_LIST_LENGTH(cmdref) && SYNC_LIST(cmdref))
    cmd_ref = SYNC_LIST(cmdref)[cmd_ref_index];
  else
    fprintf(stderr, "Raberix: Cannot find command ref. %d.\n", cmd_ref_index);
  UNLOCK_SYNC_LIST(cmdref);
  return cmd_ref;
}

int put_cmd_ref(const char * cmd_ref_name, const char * cmd_ref_desc)
{
  int idx;
  XPLMCommandRef new_cmd_ref = XPLMCreateCommand(cmd_ref_name, cmd_ref_desc);
  LOCK_SYNC_LIST(cmdref);
  idx = INSERT_INTO_SYNC_LIST(cmdref,new_cmd_ref);
  UNLOCK_SYNC_LIST(cmdref);
  return idx;
}

int add_cmd_handler(RbxCmdHandler cmd_handler, RbxCmdHandlerDestructor cmd_destr, int cmd_ref_index, int is_before, void *handler_data)
{
  if (!cmd_handler)
  {
    fprintf(stderr, "Raberix: No handler for Command ref. %d.\n", cmd_ref_index);
    return -1;
  }
  XPLMCommandRef cmd_ref = get_cmd_ref(cmd_ref_index);
  if (cmd_ref < 0)
  {
    fprintf(stderr, "Raberix: XPLM Command Ref %d.\n", cmd_ref_index);
    return -1;
  }
  RbxCmdCallbackData *data = malloc(sizeof(RbxCmdCallbackData));
  if (!data)
  {
    fprintf(stderr, "Raberix: Could not allocate memory for XPLM Command Ref %d handler data.\n", cmd_ref_index);
    return -1;
  }
  data->cmd_ref = (void*)cmd_ref; // in fact, XPLMCommandRef is void*
  data->cmd_handler = cmd_handler;
  data->cmd_destr = cmd_destr;
  data->cmd_ref_index = cmd_ref_index;
  data->is_before = is_before;
  data->handler_data = handler_data;
  LOCK_SYNC_LIST(cmdhdldat);
  data->cmd_hdl_id = INSERT_INTO_SYNC_LIST(cmdhdldat,data);
  UNLOCK_SYNC_LIST(cmdhdldat);
  if (data->cmd_hdl_id < 0)
  {
    fprintf(stderr, "Raberix: Could not save XPLM Command handler ID %d data.\n", cmd_ref_index);
    free(data);
    return -1;
  }

  XPLMRegisterCommandHandler(cmd_ref, cmd_callback, is_before, data);

  return data->cmd_hdl_id;
}

static RbxCmdCallbackData* get_hdldat(int cmd_hdl_id);

void remove_cmd_handler(int cmd_hdl_id)
{
  RbxCmdCallbackData *data = get_hdldat(cmd_hdl_id);
  if (data)
  {
    XPLMUnregisterCommandHandler(
      (XPLMCommandRef)(data->cmd_ref), // in fact, XPLMCommandRef is void*
      cmd_callback,
      data->is_before,
      data // implicit cast to void*
    );
    if (data->cmd_destr)
    {
      data->cmd_destr(
        data->cmd_ref_index,
        data->cmd_hdl_id,
        data->is_before,
        data->handler_data
      );
    }
  }
}

/* Auxiliary functions: */

static int cmd_callback(XPLMCommandRef cmd_ref, XPLMCommandPhase cmd_phase, void *data)
{
  return ((RbxCmdCallbackData*)data)->cmd_handler(
    ((RbxCmdCallbackData*)data)->cmd_ref_index,
    ((RbxCmdCallbackData*)data)->cmd_hdl_id,
    ((RbxCmdCallbackData*)data)->is_before,
    (int)cmd_phase,
    ((RbxCmdCallbackData*)data)->handler_data
  );
}

static RbxCmdCallbackData* get_hdldat(int cmd_hdl_id)
{
  RbxCmdCallbackData *data = NULL;
  LOCK_SYNC_LIST(cmdhdldat);
  if (cmd_hdl_id >= 0 && cmd_hdl_id < SYNC_LIST_LENGTH(cmdhdldat) && SYNC_LIST(cmdhdldat))
    data = SYNC_LIST(cmdhdldat)[cmd_hdl_id];
  else
    fprintf(stderr, "Raberix: Cannot find command handler ID %d.\n", cmd_hdl_id);
  UNLOCK_SYNC_LIST(cmdhdldat);
  return data;
}
