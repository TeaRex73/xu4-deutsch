/**
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>

#include "filesystem.h"

#ifdef FS_WINDOWS
#include <windows.h>
const char Path::delim = '\\';
#else
const char Path::delim = '/';
#endif

#if !defined(S_IFDIR)
#define S_IFDIR _S_IFDIR
#endif


/**
 * Creates a path to a directory or file
 */
Path::Path(const std::string &p)
    :path(p), dirs(), file(), ext()
{
    struct stat path_stat;
    char src_char, dest_char;
    unsigned int pos;
    bool _exists = false, _isDir = false;
    /* first, we need to translate the path into something the
       filesystem will recognize */
    dest_char = delim;
    if (delim == '\\') {
        src_char = '/';
    } else {
        src_char = '\\';
    }
    /* make the path work well with our OS */
    while ((pos = path.find(src_char)) < path.size()) {
        path[pos] = dest_char;
    }
    /* determine if the path really exists */
    _exists = (stat(path.c_str(), &path_stat) == 0);
    /* if so, let's glean more information */
    if (_exists) {
        _isDir = (path_stat.st_mode & S_IFDIR);
    }
    /* find the elements of the path that involve directory structure */
    std::string dir_path =
        _isDir ? path : path.substr(0, path.find_last_of(dest_char));
    /* Add the trailing / or \ to the end, if it doesn't exist */
    if (dir_path[dir_path.size() - 1] != dest_char) {
        dir_path += dest_char;
    }
    /* Get a list of directories for this path */
    while ((pos = dir_path.find(dest_char)) < dir_path.size()) {
        dirs.push_back(dir_path.substr(0, pos));
        dir_path = dir_path.substr(pos + 1);
    }
    /* If it's for sure a file, get file information! */
    if (_exists && !_isDir) {
        file = dirs.size() ?
            path.substr(path.find_last_of(dest_char) + 1) :
            path;
        if ((pos = file.find_last_of(".")) < file.size()) {
            ext = file.substr(pos + 1);
            file.resize(pos);
        }
    }
}


/**
 * Returns true if the path exists in the filesystem
 */
bool Path::exists() const
{
    struct stat path_stat;
    return stat(path.c_str(), &path_stat) == 0;
}


/**
 * Returns true if the path points to a file
 */
bool Path::isFile() const
{
    struct stat path_stat;
    if ((stat(path.c_str(), &path_stat) == 0)
        && ((path_stat.st_mode & S_IFDIR) == 0)) {
        return true;
    }
    return false;
}


/**
 * Returns true if the path points to a directory
 */
bool Path::isDir() const
{
    struct stat path_stat;
    if ((stat(path.c_str(), &path_stat) == 0)
        && (path_stat.st_mode & S_IFDIR)) {
        return true;
    }
    return false;
}


/**
 * Returns the directory indicated in the path.
 */
std::string Path::getDir() const
{
    std::list<std::string>::const_iterator i;
    std::string dir;
    for (i = dirs.cbegin(); i != dirs.cend(); ++i) {
        dir += *i + delim;
    }
    return dir;
}


/** Returns the full filename of the file designated in the path */
std::string Path::getFilename() const
{
    return (ext.empty()) ? file : file + std::string(".") + ext;
}

std::string Path::getBaseFilename() const
{
    return file; /**< Returns the base filename of the file */
}

std::string Path::getExt() const
{
    return ext; /**< Returns the extension of the file (if it exists) */
}


/**
 * Returns true if the given path exists in the filesystem
 */
bool Path::exists(const std::string &path)
{
    struct stat path_stat;
    return stat(path.c_str(), &path_stat) == 0;
}


/**
 * Opens a file and attempts to create the directory structure beneath it
 * before opening the file.
 */
std::FILE *FileSystem::openFile(const std::string &filepath, const char *mode)
{
    Path path(filepath);
    createDirectory(filepath);
    return std::fopen(path.getPath().c_str(), mode);
}


/**
 * Create the directory that composes the path.
 * If any directories that make up the path do not exist,
 * they are created.
 */
void FileSystem::createDirectory(Path &path)
{
    std::list<std::string>::const_iterator i;
    std::list<std::string> *dirs = path.getDirTree();
    std::string dir;
    for (i = dirs->cbegin(); i != dirs->cend(); ++i) {
        dir += *i;
        /* create each directory leading up to our path */
        if (!Path::exists(dir)) {
#ifdef FS_WINDOWS
            CreateDirectoryA(dir.c_str(), 0);
#else
            mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
#endif
        }
        dir += Path::delim;
    }
}


/**
 * Create a directory that composes the path
 */
void FileSystem::createDirectory(const std::string &filepath)
{
    Path path(filepath);
    createDirectory(path);
}
