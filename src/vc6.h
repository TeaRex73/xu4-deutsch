#ifndef VC6_H
#define VC6_H

#ifdef __cplusplus
extern "C" {
#include <cstdio>
#define STD_FILE std::FILE
#else
#include <stdio.h>
#define STD_FILE FILE
#endif

// IWYU pragma: always_keep

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

#if (defined(_WIN32) || defined(_WIN64))

#include <io.h>

#if defined(_MSC_VER)

static inline int fileno(STD_FILE *f)
{
    return _fileno(f);
}


#elif defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#include <unistd.h>
#else
#error "Unsupported Windows platform"
#endif

static inline int fsync(int fd)
{
   return _commit(fd);
}

static inline void sync(void)
{
}

#else
#include <unistd.h>
#endif

#ifdef __cplusplus
}
#endif

#endif
