/*
 * $Id$
 */

#ifndef DEBUG_H
#define DEBUG_H


/**
 * Define XU4_FUNC as the function name.  Most compilers define
 * __FUNCTION__.  GCC provides __FUNCTION__ as a variable, not as a
 * macro, so detecting with #if __FUNCTION__ doesn't work.
 */
#if defined(__GNUC__) || defined(__FUNCTION__)
#define XU4_FUNC __FUNCTION__
#else
#define XU4_FUNC ""
#endif

#undef TRACE
#define TRACE(dbg, msg)                             \
    (dbg).trace(msg, __FILE__, XU4_FUNC, __LINE__)
#define TRACE_LOCAL(dbg, msg)                               \
    (dbg).trace(msg, __FILE__, XU4_FUNC, __LINE__, false);

#include <string>
#include <cstdio>
#include <cstdlib>


/*
 * Derived from XINE_ASSERT in the xine project.  I've updated it to
 * be C99 compliant, to use stderr rather than stdout, and to compile
 * out when NDEBUG is set, like a regular assert.  Finally, an
 * alternate ASSERT stub is provided for pre C99 systems.
 */
void print_trace(std::FILE *file);

#if HAVE_VARIADIC_MACROS
#ifdef NDEBUG
#define U4ASSERT(exp, desc, ...) /* nothing */
#else
#define U4ASSERT(exp, ...)                             \
    do {                                               \
        if (!(exp)) {                                  \
            std::fprintf(                              \
                stderr,                                \
                "%s:%s:%d: assert `%s' gescheitert. ", \
                __FILE__, XU4_FUNC, __LINE__, #exp     \
            );                                         \
            std::fprintf(stderr, __VA_ARGS__);         \
            std::fprintf(stderr, "\n\n");              \
            print_trace(stderr);                       \
            exit(EXIT_FAILURE);                        \
        }                                              \
    }                                                  \
    while (0)
#endif /* ifdef NDEBUG */
#else
void U4ASSERT(bool exp, const char *desc, ...);
#endif /* if HAVE_VARIADIC_MACROS */


/**
 * A debug class that uses the TRACE() and TRACE_LOCAL() macros.
 * It writes debug info to the filename provided, creating
 * any directory structure it needs to ensure the file will
 * be created successfully.
 */
class Debug {
public:
    // disallow assignments, copy contruction
    Debug(const Debug &) = delete;
    Debug(Debug &&) = delete;
    const Debug &operator=(const Debug &) = delete;
    const Debug &operator=(Debug &&) = delete;

    explicit Debug(
        const std::string &fn,
        const std::string &nm = "",
        bool append = false
    );
    static void initGlobal(const std::string &filename);
    void trace(
        const std::string &msg,
        const std::string &fn = "",
        const std::string &func = "",
        const int line = -1,
        bool glbl = true
    );

private:
    static bool loggingEnabled(const std::string &name);
    bool disabled;
    std::string filename, name;
    std::FILE *file;
    static std::FILE *global;
    std::string l_filename, l_func;
    int l_line;
};

#endif /* ifndef DEBUG_H */
