#include "menu.h"

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "XPLM/XPLMMenus.h"

#include "structure.h"


typedef struct {
  char *title;
  RbxMenuItemHandler handler;
  RbxMenuItemDataDestructor ddest;
  int menuItemId;
  void *data;
  int xplm_item; // to be used by init_menu(..)
} RbxMenuItemRef;


static void rbx_menu_hdl(void *inMenuRef, void *inItemRef);

static int plugin_menu_item = 0;
static XPLMMenuID rbx_menu = NULL;

DECLARE_SYNC_LIST(RbxMenuItemRef*,menuitem)
DECLARE_INSERT_INTO_SYNC_LIST(RbxMenuItemRef*,menuitem)

int create_menu(const char *title, ...)
{
  va_list argp;
  
  fprintf(stderr, "Raberix: starting menu... ");
  XPLMMenuID xpl_plugins_menu = XPLMFindPluginsMenu();
  plugin_menu_item = XPLMAppendMenuItem(xpl_plugins_menu, "Raberix", NULL, 0);
  rbx_menu = XPLMCreateMenu("Raberix", xpl_plugins_menu, plugin_menu_item, rbx_menu_hdl, NULL);
  
  va_start(argp, title);
  const char * new_title = title;
  RbxMenuItemHandler new_handler = NULL;
  while (new_title)
  {
    new_handler = va_arg(argp, RbxMenuItemHandler);
    add_menu_item(new_title, new_handler, NULL, NULL);
    new_title = va_arg(argp, const char *);
  }
  va_end(argp);

  fprintf(stderr, "done\n");

  return rbx_menu != NULL;
}

void destroy_menu(void)
{
  fprintf(stderr, "Raberix: stopping menu... ");
  for (int i = 0; i < SYNC_LIST_LENGTH(menuitem); i++)
  {
    XPLMRemoveMenuItem(rbx_menu, SYNC_LIST(menuitem)[i]->xplm_item);
    free(SYNC_LIST(menuitem)[i]->title);
    if (SYNC_LIST(menuitem)[i]->ddest)
      (SYNC_LIST(menuitem)[i]->ddest)(SYNC_LIST(menuitem)[i]->data);
  }
  DESTROY_SYNC_LIST(menuitem);
  XPLMDestroyMenu(rbx_menu);
  rbx_menu = NULL;
  XPLMRemoveMenuItem(XPLMFindPluginsMenu(), plugin_menu_item);
  plugin_menu_item = 0;
  fprintf(stderr, " done\n");
}

static void rbx_menu_hdl(void *inMenuRef, void *inItemRef)
{
  if (!inItemRef)
    return;
  if (((RbxMenuItemRef *)inItemRef)->handler != NULL)
    (((RbxMenuItemRef *)inItemRef)->handler)(
      ((RbxMenuItemRef *)inItemRef)->menuItemId,
      ((RbxMenuItemRef *)inItemRef)->data
    );
  fprintf(stderr, "Raberix menu item %s handled.\n", ((RbxMenuItemRef *)inItemRef)->title);
}

int add_menu_item(const char *title, RbxMenuItemHandler handler, void *data, RbxMenuItemDataDestructor ddest)
{
  RbxMenuItemRef *rbx_menu_item = malloc(sizeof(RbxMenuItemRef));
  if (!rbx_menu_item)
  {
    fprintf(stderr, "Raberix: Could not allocate memory for new menu item!\n");
    return -1;
  }
  rbx_menu_item->title = strdup(title);
  if (!(rbx_menu_item->title))
  {
    fprintf(stderr, "Raberix: Could not allocate memory for new menu item title!\n");
    free(rbx_menu_item);
    return -1;
  }
  rbx_menu_item->handler = handler;
  rbx_menu_item->data = data;
  rbx_menu_item->ddest = ddest;
  LOCK_SYNC_LIST(menuitem);
  rbx_menu_item->menuItemId = INSERT_INTO_SYNC_LIST(menuitem,rbx_menu_item);
  UNLOCK_SYNC_LIST(menuitem);
  if (rbx_menu_item->menuItemId < 0)
  {
    fprintf(stderr, "Raberix: Could not add new menu item! [%d]\n", rbx_menu_item->menuItemId);
    free(rbx_menu_item->title);
    free(rbx_menu_item);
    return -1;
  }
  rbx_menu_item->xplm_item = XPLMAppendMenuItem(rbx_menu, rbx_menu_item->title, rbx_menu_item, 0);

  return rbx_menu_item->menuItemId;
}
