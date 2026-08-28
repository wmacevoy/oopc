#pragma once

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

  /*
    Tracing.

    TRACE(...) is OFF unless you compile with -DDEBUG.  When it is off it still
    parses and type-checks its arguments -- it expands to `if (0) Trace(...)` --
    so a stale format string is a compile error rather than a surprise the day
    someone turns tracing on.  The optimizer deletes the dead branch, and the
    library keeps no trace strings.

    Where traces GO is a runtime choice, so a host can capture them:

      - By default they go to stdout via vprintf.
      - Under Emscripten that is already wired to console.log (and stderr to
        console.error), so a wasm build traces into the JS console with no work.
      - Anywhere else -- a browser build, a test harness that wants to assert on
        the trace, a log file -- call Trace_to() with your own sink.

    Note for hosts that buffer their own stdout (CPython, notably): these writes
    go to file descriptor 1 directly and do NOT pass through sys.stdout, so they
    interleave out of order with Python's prints, and under Jupyter they land in
    the kernel's terminal rather than the cell output.  Call Trace_to() from the
    host if that matters.
  */

  typedef void (*TraceSink)(const char *format, va_list args);

  /* Redirect tracing.  Passing 0 restores the default stdout sink. */
  void Trace_to(TraceSink sink);

#if defined(__GNUC__)
  void Trace(const char *format, ...) __attribute__((format(printf,1,2)));
#else
  void Trace(const char *format, ...);
#endif

#if defined(DEBUG)
#  define TRACE(...) Trace(__VA_ARGS__)
#else
#  define TRACE(...) do { if (0) Trace(__VA_ARGS__); } while (0)
#endif

#ifdef __cplusplus
}
#endif
