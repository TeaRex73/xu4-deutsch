#ifndef __VC6_H
#define __VC6_H

// VC6 Compiler issues
#if defined(_MSC_VER)
// Disable "decorated name length exceeded,
// name was truncated" compiler warning
#pragma warning(disable:4503)
// Disable "symbol truncated to 255 characters" compiler warning
#pragma warning(disable:4786)
// Disable "conversion from int to bool" compiler performance warning
#pragma warning(disable:4800)
// VC8:
// Disable "'stricmp' was declared deprecated" compiler warning
#pragma warning(disable:4996)
// all:
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define snprintf _snprintf
#define strdup _strdup
// VC++ has no real equivalent of GCC's __builtin_expect, so NOP it
#define __builtin_expect(x, y) (x)
#endif

#endif
