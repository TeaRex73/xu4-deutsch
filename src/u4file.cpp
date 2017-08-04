/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "u4file.h"
#include "unzip.h"
#include "debug.h"
#include "utils.h"

#ifdef MACOSX
#include <libgen.h>
#endif




/**
 * A specialization of U4FILE that uses C stdio internally.
 */
class U4FILE_stdio:public U4FILE {
public:
    U4FILE_stdio()
        :file(nullptr)
    {
    }

    U4FILE_stdio(const U4FILE_stdio &) = delete;
    U4FILE_stdio(U4FILE_stdio &&) = delete;
    U4FILE_stdio &operator=(const U4FILE_stdio &) = delete;
    U4FILE_stdio &operator=(U4FILE_stdio &&) = delete;
    
    static U4FILE *open(const std::string &fname);
    virtual void close();
    virtual int seek(long offset, int whence);
    virtual long tell();
    virtual std::size_t read(void *ptr, std::size_t size, std::size_t nmemb);
    virtual int getc();
    virtual int putc(int c);
    virtual long length();
private:
    std::FILE *file;
};


/**
 * A specialization of U4FILE that reads files out of zip archives
 * automatically.
 */
class U4FILE_zip:public U4FILE {
public:
    U4FILE_zip()
        :zfile(nullptr),
         length_cached(0)
    {
    }

    U4FILE_zip(const U4FILE_zip &) = delete;
    U4FILE_zip(U4FILE_zip &&) = delete;
    U4FILE_zip &operator=(const U4FILE_zip &) = delete;
    U4FILE_zip &operator=(U4FILE_zip &&) = delete;
        
    static U4FILE *open(const std::string &fname, const U4ZipPackage *package);
    virtual void close();
    virtual int seek(long offset, int whence);
    virtual long tell();
    virtual std::size_t read(void *ptr, std::size_t size, std::size_t nmemb);
    virtual int getc();
    virtual int putc(int c);
    virtual long length();
private:
    unzFile zfile;
    long length_cached;
};

extern bool verbose;

U4PATH *U4PATH::instance = nullptr;

U4PATH::~U4PATH()
{
    rootResourcePaths.clear();
    u4ForDOSPaths.clear();
    u4ZipPaths.clear();
    musicPaths.clear();
    soundPaths.clear();
    configPaths.clear();
    graphicsPaths.clear();
}

U4PATH *U4PATH::getInstance()
{
    if (!instance) {
        instance = new U4PATH();
        instance->initDefaultPaths();
    }
    return instance;
}

void U4PATH::initDefaultPaths()
{
    if (defaultsHaveBeenInitd) {
        return;
    }
    // The first part of the path searched will be one of these
    // root directories
    /*Try to cover all root possibilities. These can be added to
      by separate modules*/
    rootResourcePaths.push_back("./");
    rootResourcePaths.push_back("./ultima4g/");
    rootResourcePaths.push_back("/usr/lib/u4g/");
    rootResourcePaths.push_back("/usr/local/lib/u4g/");
    // The second (specific) part of the path searched will be these
    // various subdirectories
    /* the possible paths where u4 for DOS can be installed */
    u4ForDOSPaths.push_back("");
    u4ForDOSPaths.push_back("u4g/");
    u4ForDOSPaths.push_back("ultima4g/");
    /* the possible paths where the u4 zipfiles can be installed */
    u4ZipPaths.push_back("");
    u4ZipPaths.push_back("u4g/");
    /* the possible paths where the u4 music files can be installed */
    musicPaths.push_back("");
    musicPaths.push_back("mid/");
    musicPaths.push_back("../mid/");
    musicPaths.push_back("music/");
    musicPaths.push_back("../music/");
    /* the possible paths where the u4 sound files can be installed */
    soundPaths.push_back("");
    soundPaths.push_back("./sound/");
    soundPaths.push_back("../sound/");
    /* the possible paths where the u4 config files can be installed */
    configPaths.push_back("");
    configPaths.push_back("conf/");
    configPaths.push_back("../conf/");
    /* the possible paths where the u4 graphics files can be installed */
    graphicsPaths.push_back("");
    graphicsPaths.push_back("./graphics/");
    graphicsPaths.push_back("../graphics/");
} // U4PATH::initDefaultPaths


/**
 * Returns true if the upgrade is present.
 */
bool u4isUpgradeAvailable()
{
    bool avail = false;
    U4FILE *pal;
    if ((pal = u4fopen("u4vga.pal")) != nullptr) {
        avail = true;
        u4fclose(pal);
    }
    return avail;
}


/**
 * Returns true if the upgrade is not only present, but is installed
 * (switch.bat or setup.bat has been run)
 */
bool u4isUpgradeInstalled()
{
    U4FILE *u4f = nullptr;
    long filelength;
    bool result = false;
    /* FIXME: Is there a better way to determine this? */
    u4f = u4fopen("ega.drv");
    if (u4f) {
        filelength = u4f->length();
        u4fclose(u4f);
        /* see if (ega.drv > 5k).  If so, the upgrade is installed */
        if (filelength > (5 * 1024)) {
            result = true;
        }
    }
    if (verbose) {
        std::printf("u4isUpgradeInstalled %d\n", (int)result);
    }
    return result;
}


/**
 * Creates a new zip package.
 */
U4ZipPackage::U4ZipPackage(
    const std::string &name, const std::string &path, bool extension
)
    :name(name), path(path), extension(extension), translations()
{
}

void U4ZipPackage::addTranslation(
    const std::string &value, const std::string &translation
)
{
    translations[value] = translation;
}

const std::string &U4ZipPackage::translate(const std::string &name) const
{
    std::map<std::string, std::string>::const_iterator i =
        translations.find(name);
    if (i != translations.end()) {
        return i->second;
    } else {
        return name;
    }
}

U4ZipPackageMgr *U4ZipPackageMgr::instance = nullptr;

U4ZipPackageMgr *U4ZipPackageMgr::getInstance()
{
    if (instance == nullptr) {
        instance = new U4ZipPackageMgr();
    }
    return instance;
}

void U4ZipPackageMgr::destroy()
{
    delete instance;
    instance = nullptr;
}

void U4ZipPackageMgr::add(U4ZipPackage *package)
{
    packages.push_back(package);
}

U4ZipPackageMgr::U4ZipPackageMgr()
    :packages()
{
    unzFile f;
    std::string upg_pathname(u4find_path("u4upgrad.zip", u4Path.u4ZipPaths));
    if (!upg_pathname.empty()) {
        /* upgrade zip is present */
        U4ZipPackage *upgrade = new U4ZipPackage(upg_pathname, "", false);
        upgrade->addTranslation("compassn.ega", "compassn.old");
        upgrade->addTranslation("courage.ega", "courage.old");
        upgrade->addTranslation("cove.tlk", "cove.old");
        // the next one is not actually used
        upgrade->addTranslation("ega.drv", "ega.old");
        upgrade->addTranslation("honesty.ega", "honesty.old");
        upgrade->addTranslation("honor.ega", "honor.old");
        upgrade->addTranslation("humility.ega", "humility.old");
        upgrade->addTranslation("key7.ega", "key7.old");
        upgrade->addTranslation("lcb.tlk", "lcb.old");
        upgrade->addTranslation("love.ega", "love.old");
        upgrade->addTranslation("love.ega", "love.old");
        upgrade->addTranslation("minoc.tlk", "minoc.old");
        upgrade->addTranslation("rune_0.ega", "rune_0.old");
        upgrade->addTranslation("rune_1.ega", "rune_1.old");
        upgrade->addTranslation("rune_2.ega", "rune_2.old");
        upgrade->addTranslation("rune_3.ega", "rune_3.old");
        upgrade->addTranslation("rune_4.ega", "rune_4.old");
        upgrade->addTranslation("rune_5.ega", "rune_5.old");
        upgrade->addTranslation("sacrific.ega", "sacrific.old");
        upgrade->addTranslation("skara.tlk", "skara.old");
        upgrade->addTranslation("spirit.ega", "spirit.old");
        upgrade->addTranslation("start.ega", "start.old");
        upgrade->addTranslation("stoncrcl.ega", "stoncrcl.old");
        upgrade->addTranslation("truth.ega", "truth.old");
        // the next one is not actually used
        upgrade->addTranslation("ultima.com", "ultima.old");
        upgrade->addTranslation("valor.ega", "valor.old");
        upgrade->addTranslation("yew.tlk", "yew.old");
        add(upgrade);
    }
    // Check for the default zip packages
    int flag = 0;
    std::string pathname;
    do                                {
        // Check for the upgraded package once.
        // unlikely it'll be renamed.
        pathname = u4find_path("ultima4g-1.01.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }
        // We check for all manner of generic packages, though.
        pathname = u4find_path("ultima4g.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }
        pathname = u4find_path("Ultima4g.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }
        pathname = u4find_path("ULTIMA4G.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }
        pathname = u4find_path("u4g.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }
        pathname = u4find_path("U4G.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }
        // search for the ultimaforever.com zip and variations
        pathname = u4find_path("UltimaIVg.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }
        pathname = u4find_path("Ultimaivg.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }
        pathname = u4find_path("ULTIMAIVG.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }
        pathname = u4find_path("ultimaIVg.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }
        pathname = u4find_path("ultimaivg.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }
        // If it's not found by this point, give up.
        break;
    } while (flag == 0);
    if (flag) {
        f = unzOpen(pathname.c_str());
        if (!f) {
            return;
        }
        // Now we detect the folder structure inside the zipfile.
        if (unzLocateFile(f, "charset.ega", 2) == UNZ_OK) {
            add(new U4ZipPackage(pathname, "", false));
        } else if (unzLocateFile(f, "ultima4/charset.ega", 2) == UNZ_OK) {
            add(new U4ZipPackage(pathname, "ultima4/", false));
        } else if (unzLocateFile(f, "Ultima4/charset.ega", 2) == UNZ_OK) {
            add(new U4ZipPackage(pathname, "Ultima4/", false));
        } else if (unzLocateFile(f, "ULTIMA4/charset.ega", 2) == UNZ_OK) {
            add(new U4ZipPackage(pathname, "ULTIMA4/", false));
        } else if (unzLocateFile(f, "u4/charset.ega", 2) == UNZ_OK) {
            add(new U4ZipPackage(pathname, "u4/", false));
        } else if (unzLocateFile(f, "U4/charset.ega", 2) == UNZ_OK) {
            add(new U4ZipPackage(pathname, "U4/", false));
        }
        unzClose(f);
    }
    /* scan for extensions */
}

U4ZipPackageMgr::~U4ZipPackageMgr()
{
    for (std::vector<U4ZipPackage *>::iterator i = packages.begin();
         i != packages.end();
         i++) {
        delete *i;
    }
}

int U4FILE::getshort()
{
    int byteLow = getc();
    return byteLow | (getc() << 8);
}

U4FILE *U4FILE_stdio::open(const std::string &fname)
{
    U4FILE_stdio *u4f;
    std::FILE *f;
    f = std::fopen(fname.c_str(), "rb");
    if (!f) {
        return nullptr;
    }
    u4f = new U4FILE_stdio;
    u4f->file = f;
    return u4f;
}

void U4FILE_stdio::close()
{
    std::fclose(file);
}

int U4FILE_stdio::seek(long offset, int whence)
{
    return std::fseek(file, offset, whence);
}

long U4FILE_stdio::tell()
{
    return std::ftell(file);
}

std::size_t U4FILE_stdio::read(void *ptr, std::size_t size, std::size_t nmemb)
{
    return std::fread(ptr, size, nmemb, file);
}

int U4FILE_stdio::getc()
{
    return std::fgetc(file);
}

int U4FILE_stdio::putc(int c)
{
    return std::fputc(c, file);
}

long U4FILE_stdio::length()
{
    long curr, len;
    curr = std::ftell(file);
    std::fseek(file, 0L, SEEK_END);
    len = std::ftell(file);
    std::fseek(file, curr, SEEK_SET);
    return len;
}


/**
 * Opens a file from within a zip archive.
 */
U4FILE *U4FILE_zip::open(const std::string &fname, const U4ZipPackage *package)
{
    U4FILE_zip *u4f;
    unzFile f;
    f = unzOpen(package->getFilename().c_str());
    if (!f) {
        return nullptr;
    }
    std::string pathname =
        package->getInternalPath() + package->translate(fname);
    if (unzLocateFile(f, pathname.c_str(), 2) == UNZ_END_OF_LIST_OF_FILE) {
        unzClose(f);
        return nullptr;
    }
    unzOpenCurrentFile(f);
    u4f = new U4FILE_zip;
    u4f->zfile = f;
    u4f->length_cached = -1;
    return u4f;
}

void U4FILE_zip::close()
{
    unzClose(zfile);
}

int U4FILE_zip::seek(long offset, int whence)
{
    char *buf;
    long pos;
    ASSERT(
        whence != SEEK_END,
        "seeking with whence == SEEK_END not allowed with zipfiles"
    );
    pos = unztell(zfile);
    if (whence == SEEK_CUR) {
        offset = pos + offset;
    }
    if (offset == pos) {
        return 0;
    }
    if (offset < pos) {
        unzCloseCurrentFile(zfile);
        unzOpenCurrentFile(zfile);
        pos = 0;
    }
    ASSERT(offset - pos > 0, "error in U4FILE_zip::seek");
    buf = new char[offset - pos];
    unzReadCurrentFile(zfile, buf, offset - pos);
    delete[] buf;
    return 0;
} // U4FILE_zip::seek

long U4FILE_zip::tell()
{
    return unztell(zfile);
}

std::size_t U4FILE_zip::read(void *ptr, std::size_t size, std::size_t nmemb)
{
    std::size_t retval = unzReadCurrentFile(zfile, ptr, size * nmemb);
    if (retval > 0) {
        retval = retval / size;
    }
    return retval;
}

int U4FILE_zip::getc()
{
    int retval;
    unsigned char c;
    if (unzReadCurrentFile(zfile, &c, 1) > 0) {
        retval = c;
    } else {
        retval = EOF;
    }
    return retval;
}

int U4FILE_zip::putc(int c)
{
    ASSERT(0, "zipfiles must be read-only!");
    return c;
}

long U4FILE_zip::length()
{
    unz_file_info fileinfo;
    if (length_cached == -1) {
        unzGetCurrentFileInfo(
            zfile, &fileinfo, nullptr, 0, nullptr, 0, nullptr, 0
        );
        length_cached = fileinfo.uncompressed_size;
    }
    return length_cached;
}


/**
 * Open a data file from the Ultima 4 for DOS installation.  This
 * function checks the various places where it can be installed, and
 * maps the filenames to uppercase if necessary.  The files are always
 * opened for reading only.
 *
 * First, it looks in the zipfiles.  Next, it tries FILENAME, Filename
 * and filename in up to four paths, meaning up to twelve or more
 * opens per file.  Seems to be ok for performance, but could be
 * getting excessive.  The presence of the zipfiles should probably be
 * cached.
 */
U4FILE *u4fopen(const std::string &fname)
{
    U4FILE *u4f = nullptr;
    unsigned int i;
    if (verbose) {
        std::printf("looking for %s\n", fname.c_str());
    }
    /**
     * search for file within zipfiles (ultima4.zip, u4upgrad.zip, etc.)
     */
    const std::vector<U4ZipPackage *> &packages =
        U4ZipPackageMgr::getInstance()->getPackages();
    for (std::vector<U4ZipPackage *>::const_reverse_iterator j =
             packages.rbegin();
         j != packages.rend();
         j++) {
        u4f = U4FILE_zip::open(fname, *j);
        if (u4f) {
            return u4f;  /* file was found, return it! */
        }
    }
    /*
     * file not in a zipfile; check if it has been unzipped
     */
    std::string fname_copy(fname + '\0');
    std::string pathname = u4find_path(fname_copy, u4Path.u4ForDOSPaths);
    if (pathname.empty()) {
        if (std::islower(fname_copy[0])) {
            fname_copy[0] = std::toupper(fname_copy[0]);
            pathname = u4find_path(fname_copy, u4Path.u4ForDOSPaths);
        }
        if (pathname.empty()) {
            for (i = 0; fname_copy[i] != '\0'; i++) {
                if (std::islower(fname_copy[i])) {
                    fname_copy[i] = std::toupper(fname_copy[i]);
                }
            }
            pathname = u4find_path(fname_copy, u4Path.u4ForDOSPaths);
        }
    }
    if (!pathname.empty()) {
        u4f = U4FILE_stdio::open(pathname);
        if (verbose && (u4f != nullptr)) {
            std::printf("%s successfully opened\n", pathname.c_str());
        }
    }
    return u4f;
} // u4fopen


/**
 * Opens a file with the standard C stdio facilities and wrap it in a
 * U4FILE.
 */
U4FILE *u4fopen_stdio(const std::string &fname)
{
    return U4FILE_stdio::open(fname);
}


/**
 * Opens a file from a zipfile and wraps it in a U4FILE.
 */
U4FILE *u4fopen_zip(const std::string &fname, U4ZipPackage *package)
{
    return U4FILE_zip::open(fname, package);
}


/**
 * Closes a data file from the Ultima 4 for DOS installation.
 */
void u4fclose(U4FILE *f)
{
    f->close();
    delete f;
}

int u4fseek(U4FILE *f, long offset, int whence)
{
    return f->seek(offset, whence);
}

long u4ftell(U4FILE *f)
{
    return f->tell();
}

std::size_t u4fread(void *ptr, std::size_t size, std::size_t nmemb, U4FILE *f)
{
    return f->read(ptr, size, nmemb);
}

int u4fgetc(U4FILE *f)
{
    return f->getc();
}

int u4fgetshort(U4FILE *f)
{
    return f->getshort();
}

int u4fputc(int c, U4FILE *f)
{
    return f->putc(c);
}


/**
 * Returns the length in bytes of a file.
 */
long u4flength(U4FILE *f)
{
    return f->length();
}


/**
 * Read a series of zero terminated strings from a file.  The strings
 * are read from the given offset, or the current file position if
 * offset is -1.
 */
std::vector<std::string> u4read_stringtable(
    U4FILE *f, long offset, int nstrings
)
{
    std::string buffer;
    int i;
    std::vector<std::string> strs;
    ASSERT(offset < u4flength(f), "offset begins beyond end of file");
    if (offset != -1) {
        f->seek(offset, SEEK_SET);
    }
    for (i = 0; i < nstrings; i++) {
        char c;
        buffer.erase();
        while ((c = f->getc()) != '\0') {
            buffer += c;
        }
        strs.push_back(buffer);
    }
    return strs;
}

std::string u4find_path(
    const std::string &fname, std::list<std::string> specificSubPaths
)
{
    std::FILE *f = nullptr;
    char path[2048]; // Sometimes paths get big.
    for (std::list<std::string>::iterator rootItr =
             u4Path.rootResourcePaths.begin();
         rootItr != u4Path.rootResourcePaths.end() && !f;
         rootItr++) {
        for (std::list<std::string>::iterator subItr =
                 specificSubPaths.begin();
             subItr != specificSubPaths.end() && !f;
             ++subItr) {
            std::snprintf(
                path,
                sizeof(path),
                "%s/%s/%s",
                rootItr->c_str(),
                subItr->c_str(),
                fname.c_str()
            );
            if (verbose) {
                std::printf("trying to open %s\n", path);
            }
            if ((f = std::fopen(path, "rb")) != nullptr) {
                break;
            }
        }
    }
    if (verbose) {
        if (f != nullptr) {
            std::printf("%s successfully found\n", path);
        } else {
            std::printf("%s not found\n", fname.c_str());
        }
    }
    if (f) {
        std::fclose(f);
        return path;
    } else {
        return "";
    }
} // u4find_path

std::string u4find_music(const std::string &fname)
{
    return u4find_path(fname, u4Path.musicPaths);
}

std::string u4find_sound(const std::string &fname)
{
    return u4find_path(fname, u4Path.soundPaths);
}

std::string u4find_conf(const std::string &fname)
{
    return u4find_path(fname, u4Path.configPaths);
}

std::string u4find_graphics(const std::string &fname)
{
    return u4find_path(fname, u4Path.graphicsPaths);
}
