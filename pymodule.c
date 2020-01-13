#include "pymodule.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "loop.h"
#include "dataref.h"


/* Variables for function linked to the python interpreter: */
static PyObject *flight_loop_handler = NULL;

/* other declarations: */
static PyObject * python_init_raberix(void);
static int python_initialized = 0;

static char * find_python_home(void);
static float loop_handler_wrapper(void);

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
      fprintf(stderr, "Illegal value return from flight loop handler! Assuming -1.0 instead.");
    Py_DECREF(res);
  }
  return ret_val;
}

/* Auxiliary functions for Python: */

static char* pyobj2str(PyObject *obj)
{
  /* Warning: This function does not acquire GIL!                               */
  /* It return a string that needs to be deallocated with free() by the caller. */

  char *s = NULL;
  if (!PyUnicode_Check(obj))
  {
    fprintf(stderr, "The argument passed is not a string.\n");
    return s;
  }
  PyObject *u_obj = PyUnicode_AsUTF8String(obj);
  if (!PyBytes_Check(u_obj))
  {
    fprintf(stderr, "Internal error on decoding string.\n");
    Py_DECREF(u_obj);
    return s;
  }
  const char *s_obj = PyBytes_AsString(u_obj);
  s = strdup(s_obj);
  Py_DECREF(u_obj);

  return s;
}

static long pyobj2long(PyObject *obj, long default_value)
{
  long ret_val = default_value;

  if (!PyLong_Check(obj))
    fprintf(stderr, "The argument passed is not an integer. Assuming %ld.\n", default_value);
  else
    ret_val = PyLong_AsLong(obj);
  
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
    fprintf(stderr, "Method 'set_flight_loop_handler' takes exactly 1 argument.");
    ret_val = PyLong_FromLong((long)0);
    PyGILState_Release(gil_state);
    return ret_val;
  }
  if (!PyCallable_Check(handler))
  {
    fprintf(stderr, "The argument passed to method 'set_flight_loop_handler' is not callable.");
    Py_DECREF(handler);
    ret_val = PyLong_FromLong((long)0);
    PyGILState_Release(gil_state);
    return ret_val;
  }
  Py_INCREF(handler);
  old_handler = flight_loop_handler;
  flight_loop_handler = handler;
  if (old_handler)
    Py_DECREF(old_handler);

  fprintf(stderr, "Registered flight loop handler.");
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
    PyGILState_Release(gil_state);
    return Py_None;
  }
  Py_INCREF(data_ref_name);
  char * s_data_ref_name = pyobj2str(data_ref_name);
  Py_DECREF(data_ref_name);
  if (!s_data_ref_name)
  {
    fprintf(stderr, "The argument passed to method 'find_dataref' cannot be read as string.\n");
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
  PyObject *ret_val = Py_None;
  PyObject *data_ref_idx = NULL;
  if (!PyArg_UnpackTuple(args, "get_dataref", 1, 1, &data_ref_idx))
  {
    fprintf(stderr, "Method 'get_dataref' takes exactly 1 argument.\n");
    PyGILState_Release(gil_state);
    return ret_val;
  }
  Py_INCREF(data_ref_idx);
  int i_data_ref_idx = (int)pyobj2long(data_ref_idx, -1l);
  Py_DECREF(data_ref_idx);
  if (i_data_ref_idx == -1)
  {
    fprintf(stderr, "The argument passed to method 'get_dataref' cannot be read as long integer.\n");
    PyGILState_Release(gil_state);
    return ret_val;
  }
  
  XPLMDataRef data_ref = get_data_ref(i_data_ref_idx);
  if (!data_ref)
  {
    fprintf(stderr, "Data Ref no. %i not found.\n", i_data_ref_idx);
    PyGILState_Release(gil_state);
    return ret_val;
  }

  switch(XPLMGetDataRefTypes(data_ref))
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

  PyGILState_Release(gil_state);
  return ret_val; // no err
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
    .ml_doc = "Get XPLM data ref value by data ref handler ID (see: find_dataref() method). None, if not found, or error occured."
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
