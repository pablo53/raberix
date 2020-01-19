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

/* Lists: */

#define DECLARE_LINKED_LIST_TYPE(elemtype,typename) \
  \
  typedef void (*ListElementDestructor ## typename)(elemtype element); \
  \
  typedef struct ListElementContainer ## typename ListElementContainer ## typename; \
  \
  struct ListElementContainer ## typename { \
    elemtype element; \
    ListElementContainer ## typename *next; \
    ListElementDestructor ## typename ddest; \
  }; \

#define DECLARE_LINKED_LIST_OPERATIONS(elemtype,typename) \
  int add_element_to_linked_list_ ## typename(elemtype new_element, ListElementContainer ## typename **head_ref) \
  { \
    ListElementContainer ## typename *tail = *head_ref; \
    if (!(*head_ref)) \
      *head_ref = tail = malloc(sizeof(ListElementContainer ## typename)); \
    else \
    { \
      while (tail->next) \
        tail = tail->next; \
      tail->next = malloc(sizeof(ListElementContainer ## typename)); \
      tail = tail->next; \
    } \
    if (!tail) \
    { \
      fprintf(stderr, "Could not allocate memory for a list element!\n"); \
      return -1; \
    } \
    tail->next = NULL; \
    tail->element = new_element; \
    return 0; \
  } \
  \
  void destroy_linked_list ## typename(ListElementContainer ## typename **head_ref) \
  { \
    ListElementContainer ## typename *curr; \
    while ((curr = *head_ref)) \
    { \
      if ((*head_ref)->ddest) \
        ((*head_ref)->ddest)((*head_ref)->element); \
      (*head_ref) = (*head_ref)->next; \
      free(curr); \
    } \
  } \

#define LINKED_LIST_TYPE(typename) ListElementContainer ## typename *

#define LINKED_LIST(name) head ## name

#define DECLARE_LINKED_LIST(typename,name) \
  LINKED_LIST_TYPE(typename) LINKED_LIST(name) = NULL; \

#define LINKED_LIST_ADD(typename,name,element) \
  add_element_to_linked_list_ ## typename((element), &LINKED_LIST(name)); \

#define LINKED_LIST_DESTROY(typename,name) \
  destroy_linked_list ## typename(&LINKED_LIST(name)); \

#define LINKED_LIST_FOREACH(typename,name,element) \
  for (ListElementContainer ## typename container = LINKED_LIST(name), if (container) element = LINKED_LIST(name)->element; \
       container; \
       container = container->next, if (container) element = container->element) \

#endif
