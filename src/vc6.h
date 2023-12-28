#ifndef __VC6_H
#define __VC6_H

/* VC6 Compiler issues */
#if defined(_MSC_VER)
/* Disable "decorated name length exceeded,
   name was truncated" compiler warning */
#pragma warning(disable:4503)
/* Disable "symbol truncated to 255 characters" compiler warning */
#pragma warning(disable:4786)
/* Disable "conversion from int to bool" compiler performance warning */
#pragma warning(disable:4800)
/* VC8:
   Disable "'stricmp' was declared deprecated" compiler warning */
#pragma warning(disable:4996)

#if _MSC_VER > 1600
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define snprintf _snprintf_s
#endif

/* all:
   VC++ has no real equivalent of GCC's __builtin_expect, so NOP it */
#define __builtin_expect(x, y) (x)
#endif

#if defined(__WIN32__) || defined(WIN32) || defined (_WIN32) \
  || defined(__CYGWIN__)
/* also nop sync and fsync - on a PC, they're not as necessary */
#define fsync(x)
#define sync()
#endif

#endif
