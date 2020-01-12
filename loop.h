#ifndef _LOOP_H
#define _LOOP_H

typedef void (*RbxLoopHandler)(void);

int create_loop(void);
void destroy_loop(void);

void set_loop_handler(RbxLoopHandler handler); // pass NULL to unset

#endif

