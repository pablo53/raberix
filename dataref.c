#include "dataref.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "XPLM/XPLMDataAccess.h"


static pthread_mutex_t datarefs_lock;
static XPLMDataRef *datarefs = NULL;
static int no_datarefs = 0;
static int datarefs_capacity = 0;

static int insert_dataref_to_tab(XPLMDataRef data_ref); // internal function


int create_dataref(void)
{
  fflush(stdout);
  fprintf(stderr, "Raberix: preparing XPLM data ref structures... ");
  int mres = pthread_mutex_init(&datarefs_lock, NULL);
  if (mres)
  {
    fprintf(stderr, "failed (cannot create mutex, err %i)\n", mres);
    return 0;
  }
  fprintf(stderr, "done\n");
  return 1;
}

void destroy_dataref(void)
{
  pthread_mutex_lock(&datarefs_lock);
  if (datarefs)
  {
    free(datarefs);
    datarefs = NULL;
  }
  no_datarefs = 0;
  datarefs_capacity = 0;
  pthread_mutex_unlock(&datarefs_lock);
  pthread_mutex_destroy(&datarefs_lock);
}


int add_data_ref(const char * data_ref_name)
{
  int idx = -1;
  pthread_mutex_lock(&datarefs_lock);
  XPLMDataRef data_ref = XPLMFindDataRef(data_ref_name);
  if (data_ref)
  {
    idx = insert_dataref_to_tab(data_ref);
    if (idx >= 0)
      fprintf(stderr, "Raberix: XPLM Data ref '%s' found.\n", data_ref_name);
  }
  pthread_mutex_unlock(&datarefs_lock);
  return idx;
}

XPLMDataRef get_data_ref(int data_ref_index)
{
  XPLMDataRef data_ref = NULL;
  pthread_mutex_lock(&datarefs_lock);
  if (data_ref_index >= 0 && data_ref_index < no_datarefs && datarefs)
    data_ref = datarefs[data_ref_index];
  pthread_mutex_unlock(&datarefs_lock);
  return data_ref;
}

/* Auxiliary ("private") functions: */

static int insert_dataref_to_tab(XPLMDataRef data_ref)
{
  /* This function is not thread-safe, thus should be called from a one that has acquired a lock. */
  
  if (datarefs_capacity <= no_datarefs)
  {
    int new_datarefs_capacity = datarefs_capacity + 10; // this is arbitrary (a magic number)
    XPLMDataRef *new_datarefs = calloc(new_datarefs_capacity, sizeof(XPLMDataRef));
    if (!new_datarefs)
    {
      fprintf(stderr, "Raberix: Cannot allocate more memory for data refs!\n");
      return -1;
    }
    if (datarefs)
    {
      memcpy(new_datarefs, datarefs, no_datarefs * sizeof(XPLMDataRef));
      free(datarefs);
    }
    datarefs = new_datarefs;
    datarefs_capacity = new_datarefs_capacity;
  }
  datarefs[no_datarefs] = data_ref;

  return no_datarefs++;
}
