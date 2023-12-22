/**
 * $Id$
 */

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <list>
#include <string>



/**
 * The following code was taken from the Boost filesystem libraries
 * (http://www.boost.org)
 */
#if !defined(FS_WINDOWS) && !defined(FS_POSIX)
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) \
    || defined(__CYGWIN__)
#define FS_WINDOWS
#else
#define FS_POSIX
#endif
#endif

/**
 * Provides cross-platform functionality for representing and validating
 * paths.
 */
class Path {
public:
    explicit Path(const std::string &p);
    bool exists() const;
    bool isFile() const;
    bool isDir() const;

    std::string getPath() const
    {
        return path; /**< Returns the full translated path */
    }

    std::list<std::string> *getDirTree()
    {
        return &dirs; /**< Returns the list of directories in path */
    }

    std::string getDir() const;
    std::string getFilename() const;
    std::string getBaseFilename() const;
    std::string getExt() const;
    static bool exists(const std::string &path);
    static const char delim;

private:
    std::string path;
    std::list<std::string> dirs;
    std::string file, ext;
};


/**
 * Provides cross-platform functionality for file and directory operations.
 * It currently only supports directory creation, but other operations
 * will be added as support is needed.
 */
class FileSystem {
public:
    static std::FILE *openFile(
        const std::string &filepath, const char *mode = "wrb"
    );
    static void createDirectory(Path &path);
    static void createDirectory(const std::string &filepath);
};

#endif // ifndef FILESYSTEM_H
