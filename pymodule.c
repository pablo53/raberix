#include "pymodule.h"

#include <stdio.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>


static PyObject * python_init_raberix(void);
static int python_initialized = 0;

int create_python(void)
{
  fprintf(stderr, "Raberix: starting python interpreter...");

  python_initialized = !PyImport_AppendInittab("raberix", &python_init_raberix);
  Py_Initialize();

  fprintf(stderr, " done\n");

  return python_initialized;
}

void destroy_python(void)
{
  int exit_code = Py_FinalizeEx();
  if (exit_code < 0)
    fprintf(stderr, "Python interpreter for Raberix returned exit code %d.\n", exit_code);
}

static PyObject* python_echo(PyObject *self, PyObject *args)
{
  return PyUnicode_FromString("Raberix");
}

static PyMethodDef python_methods[] =
{
  {
    .ml_name = "echo",
    .ml_meth = python_echo,
    .ml_flags = METH_VARARGS,
    .ml_doc = "Returns echo."
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
