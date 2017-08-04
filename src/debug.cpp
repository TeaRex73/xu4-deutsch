/*
 * $Id$
 */

#ifdef MACOSX
#include <CoreServices/CoreServices.h>
#endif

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "debug.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "debug.h"
#include "filesystem.h"
#include "settings.h"
#include "utils.h"

#if HAVE_BACKTRACE
#include <execinfo.h>

/**
 * Get a backtrace and print it to the file.  Note that gcc requires
 * the -rdynamic flag to have access to the actual backtrace symbols;
 * otherwise they will be simple hex offsets.
 */
void print_trace(std::FILE *file)
{
    /* Code Taken from GNU C Library manual */
    void *array[10];
    int size;
    char **strings;
    int i;
    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);
    std::fprintf(file, "Stack trace:\n");
    /* start at one to omit print_trace */
    for (i = 1; i < size; i++) {
        std::fprintf(file, "%s\n", strings[i]);
    }
    std::free(strings);
}

#else // if HAVE_BACKTRACE

/**
 * Stub for systems without access to the stack backtrace.
 */
void print_trace(std::FILE *file)
{
    std::fprintf(file, "Stack trace not available\n");
}

#endif // if HAVE_BACKTRACE


#if !HAVE_VARIADIC_MACROS
#include <cstdarg>

/**
 * Stub for systems without variadic macros.  Unfortunately, this
 * assert won't be very useful.
 */
void ASSERT(bool exp, const char *desc, ...)
{
#ifndef NDEBUG
    std::va_list args;
    va_start(args, desc);
    if (!exp) {
        std::fprintf(stderr, "Assert fehlgeschlagen: ");
        std::vfprintf(stderr, desc, args);
        std::fprintf(stderr, "\n");
        std::abort();
    }
    va_end(args);
#endif
}

#endif // if !HAVE_VARIADIC_MACROS


std::FILE *Debug::global = nullptr;


/**
 * A debug class that uses the TRACE() and TRACE_LOCAL() macros.
 * It writes debug info to the filename provided, creating
 * any directory structure it needs to ensure the file will
 * be created successfully.
 *
 * @param fn        The file path used to write debug info
 * @param nm        The name of this debug object, used to
 *                  identify it in the global debug file.
 * @param append    If true, appends to the debug file
 *                  instead of overwriting it.
 */
Debug::Debug(const std::string &fn, const std::string &nm, bool append)
    :disabled(false),
     filename(fn),
     name(nm),
     file(nullptr),
     l_filename(),
     l_func(),
     l_line(0)
{
    if (!loggingEnabled(name)) {
        disabled = true;
        return;
    }
#ifdef MACOSX
    /* In Mac OS X store debug files in a user-specific location */
    FSRef folder;
    OSErr err = FSFindFolder(
        kUserDomain, kApplicationSupportFolderType, kCreateFolder, &folder
    );
    if (err == noErr) {
        UInt8 path[2048];
        if (FSRefMakePath(&folder, path, 2048) == noErr) {
            filename = reinterpret_cast<const char *>(path);
            filename += "/xu4/" + fn;
        }
    }
#endif
    if (append) {
        file = FileSystem::openFile(filename, "at");
    } else {
        file = FileSystem::openFile(filename, "wt");
    }
    if (!file)
    {
        // FIXME: throw exception here
    } else if (!name.empty()) {
        std::fprintf(file, "=== %s ===\n", name.c_str());
    }
}


/**
 * Initializes a global debug file, if desired.
 * This file will contain the results of any TRACE()
 * macro used, whereas TRACE_LOCAL() only captures
 * the debug info in its own debug file.
 */
void Debug::initGlobal(const std::string &filename)
{
    if (settings.logging.empty()) {
        return;
    }
    if (global) {
        std::fclose(global);
    }
#ifdef MACOSX
    /* In Mac OS X store debug files in a user-specific location */
    std::string osxfname;
    osxfname.reserve(2048);
    FSRef folder;
    OSErr err = FSFindFolder(
        kUserDomain, kApplicationSupportFolderType, kCreateFolder, &folder
    );
    if (err == noErr) {
        UInt8 path[2048];
        if (FSRefMakePath(&folder, path, 2048) == noErr) {
            osxfname.append(reinterpret_cast<const char *>(path));
            osxfname += "/xu4/";
            osxfname += filename;
        }
    }
    global = osxfname.empty() ? nullptr : FileSystem::openFile(osxfname, "wt");
#else
    global = FileSystem::openFile(filename, "wt");
#endif
    if (!global) {
        // FIXME: throw exception here
    }
} // Debug::initGlobal


/**
 * Traces information into the debug file.
 * This function is used by the TRACE() and TRACE_LOCAL()
 * macros to provide trace functionality.
 */
void Debug::trace(
    const std::string &msg,
    const std::string &fn,
    const std::string &func,
    const int line,
    bool glbl
)
{
    if (disabled) {
        return;
    }
    bool brackets = false;
    std::string message, filename;
    Path path(fn);
    filename = path.getFilename();
    if (!file) {
        return;
    }
    if (!msg.empty()) {
        message += msg;
    }
    if (!filename.empty() || (line > 0)) {
        brackets = true;
        message += " [";
    }
    if ((l_filename == filename)
        && (l_func == func)
        && (l_line == line)) {
        message += "...";
    } else {
        if (!func.empty()) {
            l_func = func;
            message += func + "() - ";
        } else {
            l_func.erase();
        }
        if (!filename.empty()) {
            l_filename = filename;
            message += filename + ": ";
        } else {
            l_filename.erase();
        }
        if (line > 0) {
            l_line = line;
            char ln[8];
            std::sprintf(ln, "%d", line);
            message += "line ";
            message += ln;
        } else {
            l_line = -1;
        }
    }
    if (brackets) {
        message += "]";
    }
    message += "\n";
    std::fprintf(file, "%s", message.c_str());
    if (global && glbl) {
        std::fprintf(global, "%12s: %s", name.c_str(), message.c_str());
    }
} // Debug::trace


/**
 * Determines whether or not this debug element is enabled in our
 * game settings.
 */
bool Debug::loggingEnabled(const std::string &name)
{
    if (settings.logging == "all") {
        return true;
    }
    std::vector<std::string> enabledLogs = split(settings.logging, ", ");
    if (std::find(enabledLogs.begin(), enabledLogs.end(), name)
        != enabledLogs.end()) {
        return true;
    }
    return false;
}


#if defined(_WIN32)
#include <windows.h>

class ExceptionHandler {
public:
    ExceptionHandler()
        {
            LoadLibrary("exchndl.dll");
        }
};

static ExceptionHandler gExceptionHandler; // global instance of class
#endif
