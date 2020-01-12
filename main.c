#include "main.h"

#include "XPLM/XPLMDefs.h"

#include <string.h>
#include <GL/gl.h>

#include "pymodule.h"
#include "menu.h"
#include "loop.h"


PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc)
{
  int test = 1;

  strcpy(outName, "XPRaberixPlugin");
  strcpy(outSig, "raberix.plugin");
  strcpy(outDesc, "Raberix Plugin for X-Plane.");

  test = test && create_python();
  test = test && create_menu(1, "About", (RbxMenuItemHandler)NULL);
  test = test && create_loop();

  return test;
}

PLUGIN_API int XPluginEnable(void)
{
  return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * inParam)
{
}

PLUGIN_API void XPluginDisable(void)
{
}

PLUGIN_API void XPluginStop(void)
{
  destroy_loop();
  destroy_menu();
  destroy_python();
}
