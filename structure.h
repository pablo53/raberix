#ifndef _STRUCTURE_H
#define _STRUCTURE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pthread.h>

/* Synchronized dynamically allocated list: */

#define DECLARE_SYNC_LIST(elemtype,name) \
static pthread_mutex_t name ## _lock; \
static elemtype *name ## s = NULL; \
static int no_ ## name = 0; \
static int name ## _capacity = 0; \

#define INIT_SYNC_LIST(name) pthread_mutex_init(&name ## _lock, NULL);

#define BREAK_SYNC_LIST(name) pthread_mutex_destroy(&name ## _lock);

#define DESTROY_SYNC_LIST(name) \
  pthread_mutex_lock(&name ## _lock); \
  if (name ## s) \
  { \
    free(name ## s); \
    name ## s = NULL; \
  } \
  no_ ## name = 0; \
  name ## _capacity = 0; \
  pthread_mutex_unlock(&name ## _lock); \
  pthread_mutex_destroy(&name ## _lock); \

#define DECLARE_INSERT_INTO_SYNC_LIST(elemtype,name) \
static int insert_ ## name ## _into_tab(elemtype name) \
{ \
  /* This function is not thread-safe, thus should be called from a one that has acquired a lock. */ \
  if (name ## _capacity <= no_ ## name) \
  { \
    int new_ ## name ## _capacity = name ## _capacity + 10; /* this is arbitrary (a magic number) */ \
    elemtype *new_ ## name ## s = calloc(new_## name ## _capacity, sizeof(elemtype)); \
    if (!new_ ## name ## s) \
    { \
      fprintf(stderr, "Raberix: insert_" #name "_to_tab() cannot allocate more memory for command refs!\n"); \
      return -1; \
    } \
    if (name ## s) \
    { \
      memcpy(new_ ## name ## s, name ## s, no_ ## name * sizeof(elemtype)); \
      free(name ## s); \
    } \
    name ## s = new_ ## name ## s; \
    name ## _capacity = new_ ## name ## _capacity; \
  } \
  name ## s[no_ ## name] = name; \
  return no_ ## name++; \
} \

#define SYNC_LIST_CAPACITY(name) name ## _capacity

#define SYNC_LIST_LENGTH(name) no_ ## name

#define LOCK_SYNC_LIST(name) pthread_mutex_lock(&name ## _lock)

#define UNLOCK_SYNC_LIST(name) pthread_mutex_unlock(&name ## _lock)

#define INSERT_INTO_SYNC_LIST(name,elem) insert_ ## name ## _into_tab(elem)

#define SYNC_LIST(name) name ## s

#endif
