#ifndef _MENU_H
#define _MENU_H

typedef void (*RbxMenuItemHandler)(void);

int create_menu(int no_items, ...); // varargs of type const char * interleaved by RbxMenuItemHandler
void destroy_menu(void);

#endif
