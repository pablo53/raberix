#ifndef _MENU_H
#define _MENU_H

typedef void (*RbxMenuItemHandler)(int menuItemId);

int create_menu(const char * title, ...); // varargs of type const char * interleaved by RbxMenuItemHandler, terminated by NULL; returns menuItemId
void destroy_menu(void);

int add_menu_item(const char *title, RbxMenuItemHandler handler);

#endif
