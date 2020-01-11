#include "menu.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>


typedef struct {
  const char *title;
  RbxMenuItemHandler handler;
  int xplm_item; // to be used by init_menu(..)
} RbxMenuItemRef;


static void rbx_menu_hdl(void *inMenuRef, void *inItemRef);

static int plugin_menu_item = 0;
static XPLMMenuID rbx_menu = NULL;
static int rbx_no_menu_items = 0;
static RbxMenuItemRef *rbx_menu_items = NULL;

int create_menu(int no_items, ...)
{
  va_list argp;
  
  fprintf(stderr, "Raberix: starting menu...");
  XPLMMenuID xpl_plugins_menu = XPLMFindPluginsMenu();
  plugin_menu_item = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "Raberix", NULL, 0);
  rbx_menu = XPLMCreateMenu("Raberix", xpl_plugins_menu, plugin_menu_item, rbx_menu_hdl, NULL);
  
  rbx_menu_items = calloc(no_items, sizeof(RbxMenuItemRef));
  rbx_no_menu_items = no_items;
  va_start(argp, no_items);
  for (int i = 0; i < rbx_no_menu_items; i++)
  {
    rbx_menu_items[i].title = va_arg(argp, const char *);
    rbx_menu_items[i].handler = va_arg(argp, RbxMenuItemHandler);
    rbx_menu_items[i].xplm_item = XPLMAppendMenuItem(rbx_menu, rbx_menu_items[i].title, (void *)&rbx_menu_items[i], 0);
  }
  va_end(argp);

  fprintf(stderr, " done\n");

  return rbx_menu != NULL;
}

void destroy_menu(void)
{
  for (int i = 0; i < rbx_no_menu_items; i++)
  {
    XPLMRemoveMenuItem(rbx_menu, rbx_menu_items[i].xplm_item);
    rbx_menu_items[i].xplm_item = 0;
  }
  rbx_no_menu_items = 0;
  free(rbx_menu_items);
  rbx_menu_items = NULL;
  XPLMDestroyMenu(rbx_menu);
  rbx_menu = NULL;
  XPLMRemoveMenuItem(XPLMFindPluginsMenu(), plugin_menu_item);
  plugin_menu_item = 0;
}

static void rbx_menu_hdl(void *inMenuRef, void *inItemRef)
{
  RbxMenuItemRef *ref = (RbxMenuItemRef *)inItemRef;
  if (ref->handler != NULL)
    (ref->handler)();
  fprintf(stderr, "Raberix menu item %s handled.\n", ref->title);
}
