#include "pymodule.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "loop.h"
#include "dataref.h"
#include "commandref.h"


/* Variables for function linked to the python interpreter: */
static PyObject *flight_loop_handler = NULL;

/* other declarations: */
static PyObject * python_init_raberix(void);
static int python_initialized = 0;

static char * find_python_home(void);
static float loop_handler_wrapper(void);
static int command_handler_wrapper(int cmd_ref_index, int cmd_hdl_id, int is_before, int phase, void *handler_data); // handler_data is ref to callable PyObject

int create_python(void)
{
  fflush(stdout);
  fprintf(stderr, "Raberix: starting python interpreter... ");
  set_loop_handler(loop_handler_wrapper);
  char * python_home = find_python_home();
  if (!python_home)
  {
    fprintf(stderr, "failed.\n");
    return 0;
  }
  fprintf(stderr, "python home: '%s', ", python_home);
    
  wchar_t *loc_python_home = Py_DecodeLocale(python_home, NULL);
  if (!loc_python_home)
  {
    fprintf(stderr, "failed (cannot decode locale for python home).\n");
    return 0;
  }
  Py_SetPythonHome(loc_python_home);
  fprintf(stderr, "python home set, ");
  Py_SetProgramName(L"raberix");
  fprintf(stderr, "program name set, ");
  python_initialized = !PyImport_AppendInittab("raberix", &python_init_raberix);
  Py_Initialize();
  PyMem_RawFree(loc_python_home);
  fprintf(stderr, " done\n");
  PyRun_SimpleString("import sys\n"
                     "sys.stderr.write('Raberix: Python interpreter path:\\n')\n"
                     "for path in sys.path:\n"
                     "    sys.stderr.write('  ' + path + '\\n')\n"
                     "import os\n"
                     "print('Current dir: ', os.getcwd())\n"
                     "script_name = os.getenv('RABERIX_SCRIPT')\n"
                     "if script_name == None or script_name == '':\n"
                     "  script_name = './Resources/plugins/raberix.py'\n"
                     "with open(script_name) as script_file:\n"
                     "  exec(script_file.read())\n");

  return python_initialized;
}

void destroy_python(void)
{
  int exit_code = Py_FinalizeEx();
  if (exit_code < 0)
    fprintf(stderr, "Python interpreter for Raberix returned exit code %d.\n", exit_code);
  if (flight_loop_handler)
    Py_DECREF(flight_loop_handler);
  flight_loop_handler = NULL;
}

static char * find_python_home(void)
{
  char *python_home = getenv("RABERIX_PYTHON_HOME");
  #define no_python_home() (!python_home || !strlen(python_home))
  if (no_python_home())
  {
    fprintf(stderr, "$RABERIX_PYTHON_HOME environment variable not set, ");
    python_home = getenv("CONDA_PREFIX");
    if (no_python_home())
    {
      fprintf(stderr, "$CONDA_PREFIX environment variable not set, ");
      python_home = getenv("PYTHON_HOME");
      if (no_python_home())
      {
        fprintf(stderr, "$PYTHON_HOME environment variable not set, ");
        python_home = getenv("PYTHONHOME");
        if (no_python_home())
        {
          fprintf(stderr, "$PYTHON_HOME environment variable not set, ");
          // We cannot do anything more about it...
        }
      }
    }
  }
  return no_python_home() ? NULL : python_home;
  #undef no_python_home
}

static float loop_handler_wrapper(void)
{
  float ret_val = -1.0f;
  if (flight_loop_handler)
  {
    PyObject *res = PyObject_CallFunctionObjArgs(flight_loop_handler, NULL);
    if PyFloat_Check(res)
      ret_val = (float)PyFloat_AsDouble(res);
    else
      fprintf(stderr, "Illegal value return from flight loop handler! Assuming -1.0 instead.\n");
    Py_DECREF(res);
  }
  return ret_val;
}

/* Auxiliary functions for Python: */

static char* pyobj2str(PyObject *obj, int *err)
{
  /* Warning: This function does not acquire GIL!                               */
  /* It return a string that needs to be deallocated with free() by the caller. */

  char *s = NULL;

  Py_INCREF(obj);
  if (!PyUnicode_Check(obj))
  {
    Py_DECREF(obj);
    fprintf(stderr, "The argument passed is not a string.\n");
    if (err)
      *err = -1;
    return s;
  }
  PyObject *u_obj = PyUnicode_AsUTF8String(obj);
  if (!PyBytes_Check(u_obj))
  {
    Py_DECREF(obj);
    Py_DECREF(u_obj);
    fprintf(stderr, "Internal error on decoding string.\n");
    if (err)
      *err = -1;
    return s;
  }
  const char *s_obj = PyBytes_AsString(u_obj);
  s = strdup(s_obj);
  Py_DECREF(obj);
  Py_DECREF(u_obj);
  if (err)
    *err = 0;

  return s;
}

static long pyobj2long(PyObject *obj, long default_value, int *err)
{
  long ret_val = default_value;
  
  Py_INCREF(obj);
  if (!PyLong_Check(obj))
  {
    fprintf(stderr, "The argument passed is not an integer. Assuming %ld.\n", default_value);
    if (err)
      *err = -1;
  }
  else
  {
    ret_val = PyLong_AsLong(obj);
    if (err)
      *err = 0;
  }
  Py_DECREF(obj);
  
  return ret_val;
}

static double pyobj2double(PyObject *obj, double default_value, int *err)
{
  long ret_val = default_value;
  
  Py_INCREF(obj);
  if (!PyFloat_Check(obj))
  {
    fprintf(stderr, "The argument passed is not a float. Assuming %f.\n", default_value);
    if (err)
      *err = -1;
  }
  else
  {
    ret_val = PyFloat_AsDouble(obj);
    if (err)
      *err = 0;
  }
  Py_DECREF(obj);
  
  return ret_val;
}

/* Functions to be used from inside Python scripts: */

static PyObject* python_echo(PyObject *self, PyObject *args)
{
  return PyUnicode_FromString("Raberix");
}

static PyObject* python_set_flight_loop_handler(PyObject *self, PyObject *args)
{
  PyObject *handler, *old_handler;
  PyObject *ret_val;
  PyGILState_STATE gil_state = PyGILState_Ensure(); // it's better to acquire a lock on python interperter to be thread-safe on XPLM callbacks
  if (!PyArg_UnpackTuple(args, "set_flight_loop_handler", 1, 1, &handler))
  {
    fprintf(stderr, "Method 'set_flight_loop_handler' takes exactly 1 argument.\n");
    ret_val = PyLong_FromLong((long)0);
    PyGILState_Release(gil_state);
    return ret_val;
  }
  Py_INCREF(handler);
  if (!PyCallable_Check(handler))
  {
    fprintf(stderr, "The argument passed to method 'set_flight_loop_handler' is not callable.\n");
    ret_val = PyLong_FromLong((long)0);
    Py_DECREF(handler);
    PyGILState_Release(gil_state);
    return ret_val;
  }
  old_handler = flight_loop_handler;
  flight_loop_handler = handler;
  if (old_handler)
    Py_DECREF(old_handler);

  fprintf(stderr, "Registered flight loop handler.\n");
  ret_val = PyLong_FromLong((long)1);
  PyGILState_Release(gil_state);
  return ret_val; // no err
}

static PyObject* python_find_dataref(PyObject *self, PyObject *args)
{
  PyObject *data_ref_name = NULL; 
  PyGILState_STATE gil_state = PyGILState_Ensure();

  if (!PyArg_UnpackTuple(args, "find_dataref", 1, 1, &data_ref_name))
  {
    fprintf(stderr, "Method 'find_dataref' takes exactly 1 argument.\n");
    Py_INCREF(Py_None);
    PyGILState_Release(gil_state);
    return Py_None;
  }
  Py_INCREF(data_ref_name);
  char * s_data_ref_name = pyobj2str(data_ref_name, NULL);
  Py_DECREF(data_ref_name);
  if (!s_data_ref_name)
  {
    fprintf(stderr, "The argument passed to method 'find_dataref' cannot be read as string.\n");
    Py_INCREF(Py_None);
    PyGILState_Release(gil_state);
    return Py_None;
  }
  
  int data_ref_idx = add_data_ref(s_data_ref_name);
  if (data_ref_idx < 0)
    fprintf(stderr, "Cannot find XPLM Data Ref '%s'.\n", s_data_ref_name);
  free(s_data_ref_name);

  PyObject *ret_val = PyLong_FromLong((long)data_ref_idx);
  PyGILState_Release(gil_state);
  return ret_val; // no err
}

static PyObject* python_get_dataref(PyObject *self, PyObject *args)
{
  PyGILState_STATE gil_state = PyGILState_Ensure();
  PyObject *ret_val = NULL;
  PyObject *data_ref_idx = NULL;
  if (!PyArg_UnpackTuple(args, "get_dataref", 1, 1, &data_ref_idx))
  {
    fprintf(stderr, "Method 'get_dataref' takes exactly 1 argument.\n");
    Py_INCREF(Py_None);
    PyGILState_Release(gil_state);
    return Py_None;
  }
  Py_INCREF(data_ref_idx);
  int err;
  int i_data_ref_idx = (int)pyobj2long(data_ref_idx, -1l, &err);
  Py_DECREF(data_ref_idx);
  if (err)
  {
    fprintf(stderr, "The argument passed to method 'get_dataref' cannot be read as long integer.\n");
    Py_INCREF(Py_None);
    PyGILState_Release(gil_state);
    return Py_None;
  }
  
  XPLMDataRef data_ref = get_data_ref(i_data_ref_idx);
  if (!data_ref)
  {
    fprintf(stderr, "Data Ref no. %i not found.\n", i_data_ref_idx);
    Py_INCREF(Py_None);
    PyGILState_Release(gil_state);
    return Py_None;
  }

  switch (XPLMGetDataRefTypes(data_ref))
  {
    case xplmType_Unknown:
      fprintf(stderr, "Unknown data type of data ref. %i.\n", i_data_ref_idx);
      break;
    case xplmType_Int:
      ret_val = PyLong_FromLong(XPLMGetDatai(data_ref));
      break;
    case xplmType_Float:
      ret_val = PyFloat_FromDouble((double)XPLMGetDataf(data_ref));
      break;
    case xplmType_Double:
      ret_val = PyFloat_FromDouble(XPLMGetDatad(data_ref));
      break;
    case xplmType_FloatArray:
      fprintf(stderr, "Float array type - not implemented (data ref. %i).\n", i_data_ref_idx);
      break;
    case xplmType_IntArray:
      fprintf(stderr, "Integer array type - not implemented (data ref. %i).\n", i_data_ref_idx);
      break;
    case xplmType_Data:
      fprintf(stderr, "Data type - not implemented (data ref. %i).\n", i_data_ref_idx);
      break;
    default:
      fprintf(stderr, "Unrecognized data type of data ref. %i.\n", i_data_ref_idx);
      break;
  }

  if (!ret_val)
  {
    Py_INCREF(Py_None);
    ret_val = Py_None;
  }
  PyGILState_Release(gil_state);
  return ret_val;
}

static PyObject* python_set_dataref(PyObject *self, PyObject *args)
{
  PyGILState_STATE gil_state = PyGILState_Ensure();
  PyObject *data_ref_idx = NULL;
  PyObject *value = NULL;

  if (!PyArg_UnpackTuple(args, "set_dataref", 2, 2, &data_ref_idx, &value))
  {
    fprintf(stderr, "Method 'set_dataref' takes exactly 2 arguments: <data ref index>, <value>.\n");
    Py_INCREF(Py_None);
    PyGILState_Release(gil_state);
    return Py_None;
  }

#define INCREF_ALL() Py_INCREF(data_ref_idx); Py_INCREF(value)
#define DECREF_ALL() Py_DECREF(data_ref_idx); Py_DECREF(value)

  INCREF_ALL();

  int err;
  int i_data_ref_idx = (int)pyobj2long(data_ref_idx, -1l, &err);
  if (err)
  {
    fprintf(stderr, "The 1st argument passed to method 'set_dataref' cannot be read as long integer.\n");
    DECREF_ALL();
    Py_INCREF(Py_None);
    PyGILState_Release(gil_state);
    return Py_None;
  }
  
  XPLMDataRef data_ref = get_data_ref(i_data_ref_idx);
  if (!data_ref)
  {
    fprintf(stderr, "Data Ref no. %i not found.\n", i_data_ref_idx);
    DECREF_ALL();
    Py_INCREF(Py_None);
    PyGILState_Release(gil_state);
    return Py_None;
  }

  int i_value;
  double d_value;
  switch (XPLMGetDataRefTypes(data_ref))
  {
    case xplmType_Unknown:
      fprintf(stderr, "Unknown data type of data ref. %i.\n", i_data_ref_idx);
      break;
    case xplmType_Int:
      i_value = (int)pyobj2long(value, 0, &err);
      if (err)
        fprintf(stderr, "The 2nd argument of 'set_dataref' method should be integer for data ref. %i.\n", i_data_ref_idx);
      else
        XPLMSetDatai(data_ref, i_value);
      break;
    case xplmType_Float:
      d_value = pyobj2double(value, NAN, &err);
      if (err)
        fprintf(stderr, "The 2nd argument of 'set_dataref' method should be Float for data ref. %i.\n", i_data_ref_idx);
      else
        XPLMSetDataf(data_ref, (float)d_value);
      break;
    case xplmType_Double:
      d_value = (double)pyobj2double(value, NAN, &err);
      if (err)
        fprintf(stderr, "The 2nd argument of 'set_dataref' method should be Float for data ref. %i.\n", i_data_ref_idx);
      else
        XPLMSetDatad(data_ref, d_value);
      break;
    case xplmType_FloatArray:
      fprintf(stderr, "Float array type - not implemented (data ref. %i).\n", i_data_ref_idx);
      break;
    case xplmType_IntArray:
      fprintf(stderr, "Integer array type - not implemented (data ref. %i).\n", i_data_ref_idx);
      break;
    case xplmType_Data:
      fprintf(stderr, "Data type - not implemented (data ref. %i).\n", i_data_ref_idx);
      break;
    default:
      fprintf(stderr, "Unrecognized data type of data ref. %i.\n", i_data_ref_idx);
      break;
  }

  DECREF_ALL();
  Py_INCREF(Py_None);
  PyGILState_Release(gil_state);
  return Py_None;

#undef INCREF_ALL
#undef DECREF_ALL

}

static PyObject* python_find_commandref(PyObject *self, PyObject *args)
{
  PyObject *cmd_ref_name = NULL; 
  PyGILState_STATE gil_state = PyGILState_Ensure();

  if (!PyArg_UnpackTuple(args, "find_commandref", 1, 1, &cmd_ref_name))
  {
    fprintf(stderr, "Method 'find_commandref' takes exactly 1 argument.\n");
    Py_INCREF(Py_None);
    PyGILState_Release(gil_state);
    return Py_None;
  }
  Py_INCREF(cmd_ref_name);
  char * s_cmd_ref_name = pyobj2str(cmd_ref_name, NULL);
  Py_DECREF(cmd_ref_name);
  if (!s_cmd_ref_name)
  {
    fprintf(stderr, "The argument passed to method 'find_commandref' cannot be read as string.\n");
    Py_INCREF(Py_None);
    PyGILState_Release(gil_state);
    return Py_None;
  }
  
  int cmd_ref_idx = find_cmd_ref(s_cmd_ref_name);
  if (cmd_ref_idx < 0)
    fprintf(stderr, "Cannot find XPLM Command Ref '%s'.\n", s_cmd_ref_name);
  free(s_cmd_ref_name);

  PyObject *ret_val = PyLong_FromLong((long)cmd_ref_idx);
  PyGILState_Release(gil_state);
  return ret_val; // no err
}

static PyObject* python_do_command(PyObject *self, PyObject *args)
{
  PyGILState_STATE gil_state = PyGILState_Ensure();
  PyObject *cmd_ref_idx = NULL;
  PyObject *cmd_type = NULL;

  Py_INCREF(Py_None); // We will always return Py_None;
  
  if (!PyArg_UnpackTuple(args, "do_command", 1, 2, &cmd_ref_idx, &cmd_type))
  {
    fprintf(stderr, "Method 'do_command' takes 1 mandatory argument and 1 optional argument.\n");
    PyGILState_Release(gil_state);
    return Py_None;
  }
  Py_INCREF(cmd_ref_idx);
  int err;
  int i_cmd_ref_idx = (int)pyobj2long(cmd_ref_idx, -1l, &err);
  Py_DECREF(cmd_ref_idx);
  if (err)
  {
    fprintf(stderr, "The 1st argument passed to method 'do_command' cannot be read as long integer.\n");
    PyGILState_Release(gil_state);
    return Py_None;
  }
  int i_cmd_type = 0;
  if (cmd_type)
  {
    Py_INCREF(cmd_type);
    i_cmd_type = (int)pyobj2long(cmd_type, -1l, &err);
    Py_DECREF(cmd_type);
  }
  if (i_cmd_type < 0 || i_cmd_type > 2 || err)
  {
    fprintf(stderr, "The 2nd argument passed to method 'do_command' must be one of: 0 (=\"do command once\"), 1 (\"start command\") or 2 (\"end command\").\n");
    PyGILState_Release(gil_state);
    return Py_None;
  }
  XPLMCommandRef cmd_ref = get_cmd_ref(i_cmd_ref_idx);
  if (!cmd_ref)
  {
    fprintf(stderr, "Data Ref no. %i not found.\n", i_cmd_ref_idx);
    PyGILState_Release(gil_state);
    return Py_None;
  }

  switch (i_cmd_type)
  {
    case 0:
      XPLMCommandOnce(cmd_ref);
      break;
    case 1:
      XPLMCommandBegin(cmd_ref);
      break;
    case 2:
      XPLMCommandEnd(cmd_ref);
      break;
  }

  PyGILState_Release(gil_state);
  return Py_None; // function returns no meaningful value
}

static PyObject* python_create_command(PyObject *self, PyObject *args)
{
  PyObject *cmd_ref_name = NULL; 
  PyObject *cmd_ref_desc = NULL; 
  PyGILState_STATE gil_state = PyGILState_Ensure();

  if (!PyArg_UnpackTuple(args, "create_command", 2, 2, &cmd_ref_name, &cmd_ref_desc))
  {
    fprintf(stderr, "Method 'create_command' takes exactly 2 arguments: <command name>, <command description>.\n");
    Py_INCREF(Py_None);
    PyGILState_Release(gil_state);
    return Py_None;
  }
  Py_INCREF(cmd_ref_name);
  char * s_cmd_ref_name = pyobj2str(cmd_ref_name, NULL);
  Py_DECREF(cmd_ref_name);
  if (!s_cmd_ref_name)
  {
    fprintf(stderr, "The 1st argument passed to method 'create_command' cannot be read as string.\n");
    Py_INCREF(Py_None);
    PyGILState_Release(gil_state);
    return Py_None;
  }
  Py_INCREF(cmd_ref_desc);
  char * s_cmd_ref_desc = pyobj2str(cmd_ref_desc, NULL);
  Py_DECREF(cmd_ref_desc);
  if (!s_cmd_ref_desc)
  {
    fprintf(stderr, "The 2nd argument passed to method 'create_command' cannot be read as string.\n");
    Py_INCREF(Py_None);
    PyGILState_Release(gil_state);
    free(s_cmd_ref_name);
    return Py_None;
  }
  
  fprintf(stderr, "Creating command '%s' with description: '%s'.\n", s_cmd_ref_name, s_cmd_ref_desc);
  int cmd_ref_idx = put_cmd_ref(s_cmd_ref_name, s_cmd_ref_desc);
  
  //free(s_cmd_ref_name); // TODO: Not sure, if we can free this memory after creating XPLM command.
  //free(s_cmd_ref_desc); // TODO: Not sure, if we can free this memory after creating XPLM command.

  PyObject *ret_val = PyLong_FromLong((long)cmd_ref_idx);
  PyGILState_Release(gil_state);
  return ret_val; // no err
}

static int command_handler_wrapper(int cmd_ref_index, int cmd_hdl_id, int is_before, int phase, void *handler_data)
{
  PyGILState_STATE gil_state = PyGILState_Ensure();

  PyObject *py_cmd_ref_index = PyLong_FromLong((long)cmd_ref_index);
  PyObject *py_cmd_hdl_id = PyLong_FromLong((long)cmd_hdl_id);
  PyObject *py_is_before = PyBool_FromLong((long)is_before);
  PyObject *py_phase = PyLong_FromLong((long)phase);
  
  // python_add_command_handler() has already checked that ((PyObject*)handler_data) is callable:
  PyObject *res = PyObject_CallFunctionObjArgs((PyObject*)handler_data, py_cmd_ref_index, py_cmd_hdl_id, py_is_before, py_phase, NULL);

  Py_DECREF(py_cmd_ref_index);
  Py_DECREF(py_cmd_hdl_id);
  Py_DECREF(py_is_before);
  Py_DECREF(py_phase);

  int ret_val = 1;
  if (PyLong_Check(res))
    ret_val = (int)PyLong_AsLong(res);
  else
    fprintf(stderr, "No integer value returned from command handler id %d, called from command ref. %d.\n", cmd_hdl_id, cmd_ref_index);
  Py_DECREF(res);

  PyGILState_Release(gil_state);
  return ret_val;
}

static void command_handler_data_destructor(int cmd_ref_index, int cmd_hdl_id, int is_before, void *handler_data)
{
  PyGILState_STATE gil_state = PyGILState_Ensure();

  Py_XDECREF((PyObject*)handler_data);

  PyGILState_Release(gil_state);
}

static PyObject* python_add_command_handler(PyObject *self, PyObject *args)
{
  PyObject *cmd_ref_index;
  PyObject *handler;
  PyObject *is_before;
  PyObject *ret_val;
  PyGILState_STATE gil_state = PyGILState_Ensure(); // it's better to acquire a lock on python interperter to be thread-safe on XPLM callbacks
  
  if (!PyArg_UnpackTuple(args, "add_command_handler", 3, 3, &cmd_ref_index, &handler, &is_before))
  {
    fprintf(stderr, "Method 'add_command_handler' takes exactly 3 arguments: <command ref id>, <handler>, <is before>.\n");
    ret_val = PyLong_FromLong((long)-1l);
    PyGILState_Release(gil_state);
    return ret_val;
  }

#define INCREF_ALL() Py_INCREF(cmd_ref_index); Py_INCREF(handler); Py_INCREF(is_before)
#define DECREF_ALL() Py_DECREF(cmd_ref_index); Py_DECREF(handler); Py_DECREF(is_before)

  INCREF_ALL();

  if (!PyLong_Check(cmd_ref_index))
  {
    DECREF_ALL();
    fprintf(stderr, "The 1st argument passed to method 'add_command_handler' is not a long integer.\n");
    ret_val = PyLong_FromLong((long)-1);
    PyGILState_Release(gil_state);
    return ret_val;
  }

  if (!PyCallable_Check(handler))
  {
    DECREF_ALL();
    fprintf(stderr, "The 2nd argument passed to method 'add_command_handler' is not callable.\n");
    ret_val = PyLong_FromLong((long)-1);
    PyGILState_Release(gil_state);
    return ret_val;
  }

  if (!PyBool_Check(is_before))
  {
    DECREF_ALL();
    fprintf(stderr, "The 3rd argument passed to method 'add_command_handler' is not boolean.\n");
    ret_val = PyLong_FromLong((long)-1);
    PyGILState_Release(gil_state);
    return ret_val;
  }

  int i_cmd_ref_index = (int)PyLong_AsLong(cmd_ref_index);
  int i_is_before = PyObject_IsTrue(is_before);

  Py_DECREF(cmd_ref_index);
  Py_DECREF(is_before);
  // we still need to own callable PyObject* handler - it will be dereferenced in command_handler_data_destructor()

#undef INCREF_ALL
#undef DECREF_ALL

  int cmd_hdl_id = add_cmd_handler(command_handler_wrapper, command_handler_data_destructor, i_cmd_ref_index, i_is_before, (void*)handler); // ...this is where the handler is passed
  if (cmd_hdl_id < 0)
  {
    Py_DECREF(handler); // the handler will not be called anyway, so we do not need it any more.
    fprintf(stderr, "Failed to register command handler!\n");
  }
  else
    fprintf(stderr, "Registered command handler.\n");

  ret_val = PyLong_FromLong((long)cmd_hdl_id);
  PyGILState_Release(gil_state);
  return ret_val;
}

static PyObject* python_remove_command_handler(PyObject *self, PyObject *args)
{
  PyObject *cmd_hdl_id;
  PyGILState_STATE gil_state = PyGILState_Ensure();
  
  if (!PyArg_UnpackTuple(args, "remove_command_handler", 1, 1, &cmd_hdl_id))
  {
    fprintf(stderr, "Method 'remove_command_handler' takes exactly 1 argument: <command handler id>.\n");
    Py_INCREF(Py_None);
    PyGILState_Release(gil_state);
    return Py_None;
  }

  Py_INCREF(cmd_hdl_id);

  if (!PyLong_Check(cmd_hdl_id))
  {
    Py_DECREF(cmd_hdl_id);
    fprintf(stderr, "The argument passed to method 'remove_command_handler' is not a long integer.\n");
    Py_INCREF(Py_None);
    PyGILState_Release(gil_state);
    return Py_None;
  }

  int i_cmd_hdl_id = (int)PyLong_AsLong(cmd_hdl_id);

  remove_cmd_handler(i_cmd_hdl_id);

  Py_INCREF(Py_None);
  PyGILState_Release(gil_state);
  return Py_None;
}

/* Registering functions as a module in Python interpreter: */

static PyMethodDef python_methods[] =
{
  {
    .ml_name = "echo",
    .ml_meth = python_echo,
    .ml_flags = METH_VARARGS,
    .ml_doc = "Return echo."
  },
  {
    .ml_name = "set_flight_loop_handler",
    .ml_meth = python_set_flight_loop_handler,
    .ml_flags = METH_VARARGS,
    .ml_doc = "Register the flight loop callback handler."
  },
  {
    .ml_name = "find_dataref",
    .ml_meth = python_find_dataref,
    .ml_flags = METH_VARARGS,
    .ml_doc = "Find XPLM Data Ref and return data ref handler ID for it (>=0). Negative number means an error."
  },
  {
    .ml_name = "get_dataref",
    .ml_meth = python_get_dataref,
    .ml_flags = METH_VARARGS,
    .ml_doc = "Get XPLM data ref value by data ref handler ID (see: find_dataref() method). None, if not found or error occured."
  },
  {
    .ml_name = "set_dataref",
    .ml_meth = python_set_dataref,
    .ml_flags = METH_VARARGS,
    .ml_doc = "Set XPLM data ref value by data ref handler ID (see: find_dataref() method)."
  },
  {
    .ml_name = "find_commandref",
    .ml_meth = python_find_commandref,
    .ml_flags = METH_VARARGS,
    .ml_doc = "Find XPLM Command Ref and return command ref handler ID for it (>=0). Negative number means an error."
  },
  {
    .ml_name = "do_command",
    .ml_meth = python_do_command,
    .ml_flags = METH_VARARGS,
    .ml_doc = "Do once/begin/end XPLM Command with given command ref handler ID."
  },
  {
    .ml_name = "create_command",
    .ml_meth = python_create_command,
    .ml_flags = METH_VARARGS,
    .ml_doc = "Create new XPLM Command and return command ref handler ID."
  },
  {
    .ml_name = "add_command_handler",
    .ml_meth = python_add_command_handler,
    .ml_flags = METH_VARARGS,
    .ml_doc = "Add XPLM Command handler and return its reference ID."
  },
  {
    .ml_name = "remove_command_handler",
    .ml_meth = python_remove_command_handler,
    .ml_flags = METH_VARARGS,
    .ml_doc = "Remove XPLM Command handler, previously added, by its reference ID."
  },
  {
    .ml_name = NULL,
    .ml_meth = NULL,
    .ml_flags = 0,
    .ml_doc = NULL
  }
};

static PyModuleDef python_module =
{
  .m_base = PyModuleDef_HEAD_INIT,
  .m_name = "raberix",
  .m_doc = "Raberix Addon for X-Plane integration module.",
  .m_size = -1,
  .m_methods = python_methods,
  .m_slots = NULL,
  .m_traverse = NULL,
  .m_clear = NULL,
  .m_free = NULL
};

static PyObject* python_init_raberix(void)
{
  return PyModule_Create(&python_module);
}
