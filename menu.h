#ifndef _MENU_H
#define _MENU_H

#include "XPLM/XPLMMenus.h"

typedef void (*RbxMenuItemHandler)(void);

int create_menu(int no_items, ...); // varargs of type const char * interleaved by RbxMenuItemHandler
void destroy_menu(void);

#endif
