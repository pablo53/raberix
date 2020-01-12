#include "loop.h"

#include <stdio.h>
#include <stdlib.h>

#include "XPLM/XPLMProcessing.h"

static float flight_loop_callback(float inElapsedSinceLastCall,
                                  float inElapsedTimeSinceLastFlightLoop,
                                  int inCounter,
                                  void *inRefcon);

static XPLMCreateFlightLoop_t flight_loop_data =
{
  .structSize = sizeof(XPLMCreateFlightLoop_t),
  .phase = xplm_FlightLoop_Phase_AfterFlightModel,
  .callbackFunc = flight_loop_callback,
  .refcon = NULL
};

static RbxLoopHandler flight_loop_handler = NULL;
static XPLMFlightLoopID flight_loop_id = NULL;

int create_loop(void)
{
  fflush(stdout);
  fprintf(stderr, "Raberix: starting flight loop... ");

  flight_loop_id = XPLMCreateFlightLoop(&flight_loop_data);
  XPLMScheduleFlightLoop(flight_loop_id, 0.1, 0); // Schedule the loop to run every 0.1 sec.

  fprintf(stderr, "done\n");
  return 1;
}


void destroy_loop(void)
{
  XPLMDestroyFlightLoop(flight_loop_id);
  flight_loop_id = NULL;
}

void set_loop_handler(RbxLoopHandler handler)
{
  flight_loop_handler = handler;
}

static float flight_loop_callback(float inElapsedSinceLastCall,
                                  float inElapsedTimeSinceLastFlightLoop,
                                  int inCounter,
                                  void *inRefcon)
{
  if (flight_loop_handler)
    flight_loop_handler();
  return -1.0; // by default, call back every flight loop iteration
}
