#ifndef _DEF_H
#define _DEF_H

#if defined(WIN)
#  define EXPORT_SYMBOL __declspec(dllexport)
#elif defined(LIN)
//#  error Linux not supported Yet
//#  define EXPORT_SYMBOL
#  define EXPORT_SYMBOL __attribute__ ((visibility ("default")))
#elif defined(MAC)
#  error Mac OS not supported yet.
#  define EXPORT_SYMBOL
#endif

#endif