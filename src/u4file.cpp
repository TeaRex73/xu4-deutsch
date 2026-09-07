/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cctype> // IWYU pragma: keep
#include <climits> // IWYU pragma: keep
#include <cstdint> // IWYU pragma: keep
#include <cstdio>
#include <cwchar>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "u4file.h"

#include "debug.h"
#include "unzip.h"
#include "xordata.h"


namespace {
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
        void close() override;
        int seek(long offset, int whence) override;
        long tell() override;
        std::size_t read(
            void *ptr, std::size_t size, std::size_t nmemb
        ) override;
        int getc() override;
        int putc(int c) override;
        long length() override;
    private:
        std::FILE *file;
    };
}

namespace {
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
        void close() override;
        int seek(long offset, int whence) override;
        long tell() override;
        std::size_t read(
            void *ptr, std::size_t size, std::size_t nmemb
        ) override;
        int getc() override;
        int putc(int) override;
        long length() override;
    private:
        unzFile zfile;
        long length_cached;
    };
}

namespace {
    /**
     * A specialization of U4FILE that uses XOR data over another U4FILE.
     */
    class U4FILE_xor:public U4FILE {
    public:
        U4FILE_xor()
            :xor_data(nullptr),
             pos(0),
             file(nullptr)
        {
        }

        U4FILE_xor(const U4FILE_xor &) = delete;
        U4FILE_xor(U4FILE_xor &&) = delete;
        U4FILE_xor &operator=(const U4FILE_xor &) = delete;
        U4FILE_xor &operator=(U4FILE_xor &&) = delete;

        static U4FILE *open(const std::string &fname);
        static U4FILE *open(const std::string &fname, const U4ZipPackage *package);
        void close() override;
        int seek(long offset, int whence) override;
        long tell() override;
        std::size_t read(
            void *ptr, std::size_t size, std::size_t nmemb
        ) override;
        int getc() override;
        int putc(int) override;
        long length() override;
    private:
        const ByteVector *xor_data;
        std::size_t pos;
        U4FILE *file;
    };
}

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
    if (__builtin_expect(!instance, false)) {
        instance = new U4PATH();
        instance->initDefaultPaths();
    }
    return instance;
}

void U4PATH::initDefaultPaths()
{
    if (defaultsHaveBeenInited) {
        return;
    }
    // The first part of the path searched will be one of these
    // root directories
    /*Try to cover all root possibilities. These can be added to
      by separate modules*/
    rootResourcePaths.emplace_back(".");
    rootResourcePaths.emplace_back("");
    rootResourcePaths.emplace_back("./ultima4");
    rootResourcePaths.emplace_back("/usr/lib/u4");
    rootResourcePaths.emplace_back("/usr/local/lib/u4");
    // The second (specific) part of the path searched will be these
    // various subdirectories
    /* the possible paths where u4 for DOS can be installed */
    u4ForDOSPaths.emplace_back("");
    u4ForDOSPaths.emplace_back("./u4");
    u4ForDOSPaths.emplace_back("./ultima4");
    /* the possible paths where the u4 zip files can be installed */
    u4ZipPaths.emplace_back("");
    u4ZipPaths.emplace_back("./u4");
    /* the possible paths where the u4 music files can be installed */
    musicPaths.emplace_back("./music");
    musicPaths.emplace_back("../music");
    musicPaths.emplace_back("./mid");
    musicPaths.emplace_back("../mid");
    musicPaths.emplace_back("");
    /* the possible paths where the u4 sound files can be installed */
    soundPaths.emplace_back("./sound");
    soundPaths.emplace_back("../sound");
    soundPaths.emplace_back("");
    /* the possible paths where the u4 config files can be installed */
    configPaths.emplace_back("./conf");
    configPaths.emplace_back("../conf");
    configPaths.emplace_back("");
    /* the possible paths where the u4 graphics files can be installed */
    graphicsPaths.emplace_back("./graphics");
    graphicsPaths.emplace_back("../graphics");
    graphicsPaths.emplace_back("");
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
    bool result = false;
    /* FIXME: Is there a better way to determine this? */
    U4FILE *u4f = u4fopen("ega.drv");
    if (u4f) {
        const long file_length = u4f->length();
        u4fclose(u4f);
        /* see if (ega.drv > 5k).  If so, the upgrade is installed */
        if (file_length > 5 * 1024) {
            result = true;
        }
    }
    if (verbose) {
        std::printf("u4isUpgradeInstalled %d\n", static_cast<int>(result));
    }
    return result;
}


/**
 * Creates a new zip package.
 */
U4ZipPackage::U4ZipPackage(
    std::string name, std::string path, const bool extension
)
    :name(std::move(name)), path(std::move(path)), extension(extension)
{
}

void U4ZipPackage::addTranslation(
    const std::string &value, const std::string &translation
)
{
    translations[value] = translation;
}

const std::string &U4ZipPackage::translate(const std::string &nameToFind) const
{
    const auto i = translations.find(nameToFind);
    if (i != translations.cend()) {
        return i->second;
    }
    return nameToFind;
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
{
    std::string upg_pathname(u4find_path("u4upgrad.zip", u4Path.u4ZipPaths));
    if (!upg_pathname.empty()) {
        /* upgrade zip is present */
        auto *upgrade = new U4ZipPackage(upg_pathname, "", false);
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
    bool flag = false;
    std::string pathname;
    do {
        // Check for the upgraded package once.
        // unlikely it'll be renamed.
        pathname = u4find_path("ultima4-1.01.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = true;
            break;
        }
        // We check for all manner of generic packages, though.
        pathname = u4find_path("ultima4.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = true;
            break;
        }
        pathname = u4find_path("Ultima4.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = true;
            break;
        }
        pathname = u4find_path("ULTIMA4.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = true;
            break;
        }
        pathname = u4find_path("u4.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = true;
            break;
        }
        pathname = u4find_path("U4.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = true;
            break;
        }
        // search for the ultimaforever.com zip and variations
        pathname = u4find_path("UltimaIV.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = true;
            break;
        }
        pathname = u4find_path("Ultimaiv.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = true;
            break;
        }
        pathname = u4find_path("ULTIMAIV.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = true;
            break;
        }
        pathname = u4find_path("ultimaIV.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = true;
            break;
        }
        pathname = u4find_path("ultimaiv.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = true;
            break;
        }
        // If it's not found by this point, give up.
        break;
    } while (!flag);
    if (flag) {
        unzFile f = unzOpen(pathname.c_str());
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
    for (const auto *package: packages) {
        delete package;
    }
}

int U4FILE::get_short()
{
    const int byteLow = getc();
    return byteLow | (getc() << 8);
}

U4FILE *U4FILE_stdio::open(const std::string &fname)
{
    std::FILE *f = std::fopen(fname.c_str(), "rb");
    if (!f) {
        return nullptr;
    }
    auto *u4f = new U4FILE_stdio;
    u4f->file = f;
    return u4f;
}

void U4FILE_stdio::close()
{
    std::fclose(file);
}

int U4FILE_stdio::seek(const long offset, const int whence)
{
    return std::fseek(file, offset, whence);
}

long U4FILE_stdio::tell()
{
    return std::ftell(file);
}

std::size_t U4FILE_stdio::read(
    void *ptr, const std::size_t size, const std::size_t nmemb
)
{
    return std::fread(ptr, size, nmemb, file);
}

int U4FILE_stdio::getc()
{
    return std::fgetc(file);
}

int U4FILE_stdio::putc(const int c)
{
    return std::fputc(c, file);
}

long U4FILE_stdio::length()
{
    const long curr = std::ftell(file);
    std::fseek(file, 0L, SEEK_END);
    const long len = std::ftell(file);
    std::fseek(file, curr, SEEK_SET);
    return len;
}

U4FILE *U4FILE_xor::open(const std::string &fname)
{
    U4FILE *u4f_stdio;
    const auto i = xorDataMap.find(fname);
    if (i == xorDataMap.end()) {
        u4f_stdio = U4FILE_stdio::open(fname);
        if (!u4f_stdio) {
            return nullptr;
        }
        return u4f_stdio;
    }
    u4f_stdio = U4FILE_stdio::open(i->second.name);
    if (!u4f_stdio) {
        return nullptr;
    }
    auto *u4f = new U4FILE_xor;
    u4f->file = u4f_stdio;
    u4f->pos = 0;
    u4f->xor_data = &i->second.contents;
    return u4f;
}

U4FILE *U4FILE_xor::open(const std::string &fname, const U4ZipPackage *package)
{
    U4FILE *u4f_zip;
    const auto i = xorDataMap.find(fname);
    if (i == xorDataMap.end()) {
        u4f_zip = U4FILE_zip::open(fname, package);
        if (!u4f_zip) {
            return nullptr;
        }
        return u4f_zip;
    }
    u4f_zip = U4FILE_zip::open(i->second.name, package);
    if (!u4f_zip) {
        return nullptr;
    }
    auto *u4f = new U4FILE_xor;
    u4f->file = u4f_zip;
    u4f->pos = 0;
    u4f->xor_data = &i->second.contents;
    return u4f;
}

void U4FILE_xor::close()
{
    file->close();
    delete file;
    file = nullptr;
    xor_data = nullptr;
    pos = 0;
}

int U4FILE_xor::seek(const long offset, const int whence)
{
    long new_pos;
    switch (whence) {
    case SEEK_SET:
        new_pos = offset;
        break;
    case SEEK_CUR:
        new_pos = static_cast<long>(pos) + offset;
        break;
    case SEEK_END:
        new_pos = static_cast<long>(xor_data->size()) + offset;
        break;
    default:
        return -1;
    }
    if (new_pos < 0 || new_pos >= static_cast<long>(xor_data->size())) {
        return -1;
    }
    pos = static_cast<std::size_t>(new_pos);
    return file->seek(static_cast<long>(pos) % file->length(), SEEK_SET);
}

long U4FILE_xor::tell()
{
    return static_cast<long>(pos);
}

int U4FILE_xor::getc()
{
    if (pos >= xor_data->size()) return EOF;
    int c = file->getc();
    if (c == EOF) {
        file->seek(0, SEEK_SET);
        c = file->getc();
        if (c == EOF) return EOF;
    }
    return c ^ (*xor_data)[pos++];
}

int U4FILE_xor::putc(int)
{
    U4ASSERT(0, "xor files must be read-only!");
    return EOF;
}

std::size_t U4FILE_xor::read(
    void *ptr, const std::size_t size, const std::size_t nmemb
)
{
    for (std::size_t i = 0; i < size * nmemb; i++) {
        const int c = this->getc();
        if (c == EOF) return i / size;
        static_cast<std::uint8_t *>(ptr)[i] = c;
    }
    return nmemb;
}

long U4FILE_xor::length()
{
    return static_cast<long>(xor_data->size());
}


/**
 * Opens a file from within a zip archive.
 */
U4FILE *U4FILE_zip::open(const std::string &fname, const U4ZipPackage *package)
{
    const unzFile f = unzOpen(package->getFilename().c_str());
    if (!f) {
        return nullptr;
    }
    const std::string pathname =
        package->getInternalPath() + package->translate(fname);
    if (unzLocateFile(f, pathname.c_str(), 2) == UNZ_END_OF_LIST_OF_FILE) {
        unzClose(f);
        return nullptr;
    }
    unzOpenCurrentFile(f);
    auto *u4f = new U4FILE_zip;
    u4f->zfile = f;
    u4f->length_cached = -1;
    return u4f;
}

void U4FILE_zip::close()
{
    unzClose(zfile);
}

int U4FILE_zip::seek(long offset, const int whence)
{
    U4ASSERT(
        whence != SEEK_END,
        "seeking with whence == SEEK_END not allowed with zip files"
    );
    long pos = unztell(zfile);
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
    U4ASSERT(offset - pos >= 0, "error in U4FILE_zip::seek");
    if (offset > pos) {
        auto *buf = new char[offset - pos];
        unzReadCurrentFile(zfile, buf, offset - pos);
        delete[] buf;
    }
    return 0;
} // U4FILE_zip::seek

long U4FILE_zip::tell()
{
    return unztell(zfile);
}

std::size_t U4FILE_zip::read(
    void *ptr, const std::size_t size, const std::size_t nmemb
)
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

int U4FILE_zip::putc(int)
{
    U4ASSERT(0, "zip files must be read-only!");
    return EOF;
}

long U4FILE_zip::length()
{
    unz_file_info fileinfo;
    if (length_cached == -1) {
        unzGetCurrentFileInfo(
            zfile, &fileinfo, nullptr, 0, nullptr, 0, nullptr, 0
        );
        length_cached = static_cast<long>(fileinfo.uncompressed_size);
    }
    return length_cached;
}


/**
 * Open a data file from the Ultima 4 for DOS installation.  This
 * function checks the various places where it can be installed, and
 * maps the filenames to uppercase if necessary.  The files are always
 * opened for reading only.
 *
 * First, it looks in the zip files.  Next, it tries FILENAME, Filename
 * and filename in up to four paths, meaning up to twelve or more
 * opens per file.  Seems to be ok for performance, but could be
 * getting excessive.  The presence of the zip files should probably be
 * cached.
 */
U4FILE *u4fopen(const std::string &fname)
{
    U4FILE *u4f = nullptr;
    if (verbose) {
        std::printf("looking for %s\n", fname.c_str());
    }
    /**
     * search for file within zipfiles (ultima4.zip, u4upgrad.zip, etc.)
     */
    const std::vector<U4ZipPackage *> &packages =
        U4ZipPackageMgr::getInstance()->getPackages();
    for (auto j = packages.crbegin(); j != packages.crend(); ++j) {
        u4f = U4FILE_xor::open(fname, *j);
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
            fname_copy[0] = static_cast<char>(std::toupper(fname_copy[0]));
            pathname = u4find_path(fname_copy, u4Path.u4ForDOSPaths);
        }
        if (pathname.empty()) {
            for (int i = 0; fname_copy[i] != '\0'; i++) {
                if (std::islower(fname_copy[i])) {
                    fname_copy[i] =
                        static_cast<char>(std::toupper(fname_copy[i]));
                }
            }
            pathname = u4find_path(fname_copy, u4Path.u4ForDOSPaths);
        }
    }
    if (!pathname.empty()) {
        u4f = U4FILE_xor::open(pathname);
        if (verbose && u4f != nullptr) {
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
 * Opens a file from XOR data and a standard file and wraps it in a U4FILE.
 */
U4FILE *u4fopen_xor(const std::string &fname)
{
    return U4FILE_xor::open(fname);
}

/**
 * Opens a file from XOR data and a zip file and wraps it in a U4FILE.
 */
U4FILE *u4fopen_xor(const std::string &fname, const U4ZipPackage *package)
{
    return U4FILE_xor::open(fname, package);
}


/**
 * Closes a data file from the Ultima 4 for DOS installation.
 */
void u4fclose(U4FILE *f)
{
    f->close();
    delete f;
}

int u4fseek(U4FILE *f, const long offset, const int whence)
{
    return f->seek(offset, whence);
}

long u4ftell(U4FILE *f)
{
    return f->tell();
}

std::size_t u4fread(
    void *ptr, const std::size_t size, const std::size_t nmemb, U4FILE *f
)
{
    return f->read(ptr, size, nmemb);
}

int u4fgetc(U4FILE *f)
{
    return f->getc();
}

int u4fget_short(U4FILE *f)
{
    return f->get_short();
}

int u4fputc(const int c, U4FILE *f)
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
std::vector<std::string> u4read_string_table(
    U4FILE *f, const long offset, const int n_strings
)
{
    std::string buffer;
    std::vector<std::string> strs;
    U4ASSERT(offset < u4flength(f), "offset begins beyond end of file");
    if (offset != -1) {
        f->seek(offset, SEEK_SET);
    }
    for (int i = 0; i < n_strings; i++) {
        char c;
        buffer.erase();
        while ((c = static_cast<char>(f->getc())) != '\0') {
            buffer += c;
        }
        strs.push_back(buffer);
    }
    return strs;
}

#ifndef PATH_MAX
#   define PATH_MAX 4096 //FIXME: There is no real maximum on some OS
#endif

std::string u4find_path(
    const std::string &fname, const std::list<std::string> &specificSubPaths
)
{
    std::FILE *f = nullptr;
    // Try absolute first
    char path[PATH_MAX];

    f = std::fopen(fname.c_str(), "rb");
    if (f) std::snprintf(path, PATH_MAX, "%s", fname.c_str());

    // Try 'file://' protocol if specified
    if (f == nullptr) {
        const std::string file_url_prefix("file://");

        if(fname.compare(0, file_url_prefix.length(), file_url_prefix) == 0) {
            std::snprintf(
                path,
                PATH_MAX,
                "%s",
                fname.substr(file_url_prefix.length()).c_str()
            );
            if (verbose) {
                std::printf("trying to open %s\n", path);
            }
            f = std::fopen(path, "rb");
        }
    }

    // Try paths
    if (f == nullptr) {
        for (auto rootItr = u4Path.rootResourcePaths.cbegin();
             rootItr != u4Path.rootResourcePaths.cend() && !f;
             ++rootItr) {
            for (auto subItr = specificSubPaths.cbegin();
                 subItr != specificSubPaths.cend() && !f;
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
    }
    return "";
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
