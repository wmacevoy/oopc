#include <stdio.h>

#include "Trace.h"

#ifdef __cplusplus
extern "C" {
#endif

  static void Trace_stdout(const char *format, va_list args)
  {
    vprintf(format, args);
  }

  static TraceSink sink = Trace_stdout;

  void Trace_to(TraceSink new_sink)
  {
    sink = (new_sink != 0) ? new_sink : Trace_stdout;
  }

  void Trace(const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    sink(format, args);
    va_end(args);
  }

#ifdef __cplusplus
} /* extern "C" */
#endif
