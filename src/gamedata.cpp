/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <sys/stat.h>
#include <sys/types.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

#if defined(__APPLE__) && defined(__MACH__)
#include <CommonCrypto/CommonDigest.h>
#include <dirent.h>
#include <mach-o/dyld.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <climits>
#endif

#include "gamedata.h"

#include "u4file.h"


/**
 * Add the per-user data directory to the list of roots searched for
 * game data.  It is appended last so files shipped with the app still
 * take precedence.  Only the macOS bundle downloads game data, so this
 * is a no-op elsewhere.
 */
#if defined(__APPLE__) && defined(__MACH__)
void addUserDataSearchPath(const std::string &path)
{
    if (path.empty()) {
        return;
    }
    u4Path.rootResourcePaths.push_back(path);
}
#else
void addUserDataSearchPath(const std::string &)
{
}
#endif

#if defined(__APPLE__) && defined(__MACH__)

extern char **environ;

namespace {
    /* size and checksum of the ultima4.zip that the Gentoo mirrors
       have been serving for years; anything else is rejected so an
       http:// mirror cannot inject a modified game */
    const std::int64_t GAME_DATA_SIZE = 529099;
    const char *const GAME_DATA_FILE = "ultima4.zip";
    const char *const GAME_DATA_SHA256 =
        "94aa748cfa1d0e7aa2e518abebb994f3c18acf7edb78c3bd37cd0a4404e6ba74";
    const char *const GAME_DATA_URL =
        "https://ultima.thatfleminggent.com/ultima4.zip";
    const char *const DOCS_DIR = "Ultima IV Dokumentation";
    const char *const DOCS_TARGET_DIR = "Ultima IV Deutsch";
    const char *const DOCS_SUFFIX = ".pdf.xor";
    const char *const DOCS_XOR_SUFFIX = ".xor";
    const char *const DOCS_README = "LIESMICH.TXT";

    /**
     * Path of the running app bundle, or an empty string when u4 was
     * not started from a bundle.
     */
    std::string bundlePath()
    {
        char exe[PATH_MAX];
        uint32_t size = static_cast<uint32_t>(sizeof(exe));
        if (_NSGetExecutablePath(exe, &size) != 0) {
            return "";
        }
        char resolved[PATH_MAX];
        if (realpath(exe, resolved) == nullptr) {
            return "";
        }
        const std::string path(resolved);
        const std::string suffix("/Contents/MacOS/");
        const std::size_t pos = path.rfind(suffix);
        if (pos == std::string::npos
            || path.find('/', pos + suffix.size()) != std::string::npos) {
            return "";
        }
        return path.substr(0, pos);
    }

    std::string parentDirectory(const std::string &path)
    {
        const std::size_t pos = path.rfind('/');
        if (pos == std::string::npos) {
            return "";
        }
        if (pos == 0) {
            return "/";
        }
        return path.substr(0, pos);
    }

    std::string userDataDirectory(const std::string &userPath)
    {
        if (userPath.empty()) {
            return "";
        }
        if (userPath[userPath.size() - 1] == '/') {
            return userPath;
        }
        return userPath + '/';
    }

    std::string homeDirectory()
    {
        const char *home = std::getenv("HOME");
        if (home == nullptr || home[0] == '\0') {
            return "";
        }
        return std::string(home);
    }

    bool fileExists(const std::string &path)
    {
        struct stat st {};
        return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
    }

    bool directoryExists(const std::string &path)
    {
        struct stat st {};
        return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
    }

    std::string trim(const std::string &text)
    {
        const std::string whitespace(" \t\r\n");
        const std::size_t begin = text.find_first_not_of(whitespace);
        if (begin == std::string::npos) {
            return "";
        }
        const std::size_t end = text.find_last_not_of(whitespace);
        return text.substr(begin, end - begin + 1);
    }

    /**
     * Run a program without a shell, so paths with spaces need no
     * quoting.  Returns true when it exits successfully.
     */
    bool runProcess(const std::vector<std::string> &args)
    {
        std::vector<char *> argv;
        argv.reserve(args.size() + 1);
        for (const auto &arg: args) {
            argv.push_back(const_cast<char *>(arg.c_str()));
        }
        argv.push_back(nullptr);
        pid_t pid = 0;
        if (posix_spawn(
                &pid, argv[0], nullptr, nullptr, argv.data(), environ
            ) != 0) {
            return false;
        }
        int status = 0;
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                return false;
            }
        }
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }

    std::string appleScriptString(const std::string &text)
    {
        std::string quoted("\"");
        for (const char c: text) {
            if (c == '\\' || c == '"') {
                quoted += '\\';
            }
            if (c == '\n') {
                quoted += "\\n";
            } else {
                quoted += c;
            }
        }
        quoted += '"';
        return quoted;
    }

    void showAlert(const std::string &title, const std::string &message)
    {
        const std::string script =
            "display alert " + appleScriptString(title)
            + " message " + appleScriptString(message)
            + " as critical buttons {\"OK\"} default button 1";
        runProcess({"/usr/bin/osascript", "-e", script});
    }

    void reportDownloadFailure(
        const std::string &dir, const bool interactive
    )
    {
        std::string message(
            "Die Spieldaten von Ultima IV konnten nicht heruntergeladen "
            "werden.\n\nBitte lade die Datei\n"
        );
        message += GAME_DATA_URL;
        message += "\nmit dem Browser herunter und lege sie, ohne sie zu "
                   "entpacken, hier ab:\n\n";
        message += dir;
        message += GAME_DATA_FILE;
        message += "\n\nBeim naechsten Start findet das Spiel die Daten.";
        if (interactive && std::getenv("U4_NO_DIALOG") == nullptr) {
            showAlert("Ultima IV", message);
        } else {
            std::fprintf(stderr, "%s\n", message.c_str());
        }
    }

    void addMirror(
        std::vector<std::string> &mirrors, const std::string &value
    )
    {
        if (value.empty() || value[0] == '#') {
            return;
        }
        if (value.compare(0, 7, "http://") != 0
            && value.compare(0, 8, "https://") != 0) {
            return;
        }
        std::string mirror = value;
        if (mirror[mirror.size() - 1] != '/') {
            mirror += '/';
        }
        for (const auto &existing: mirrors) {
            if (existing == mirror) {
                return;
            }
        }
        mirrors.push_back(mirror);
    }

    void readMirrorFile(
        const std::string &path, std::vector<std::string> &mirrors
    )
    {
        std::FILE *f = std::fopen(path.c_str(), "r");
        if (f == nullptr) {
            return;
        }
        char line[512];
        while (std::fgets(line, static_cast<int>(sizeof(line)), f)
               != nullptr) {
            addMirror(mirrors, trim(line));
        }
        std::fclose(f);
    }

    /**
     * Mirror base URLs, one per line, from Resources/mirrors.txt.
     * Falls back to a few known good mirrors when the file is absent.
     */
    std::vector<std::string> readMirrors()
    {
        std::vector<std::string> mirrors;
        readMirrorFile("mirrors.txt", mirrors);
        const std::string bundle = bundlePath();
        if (!bundle.empty()) {
            readMirrorFile(
                bundle + "/Contents/Resources/mirrors.txt", mirrors
            );
        }
        if (mirrors.empty()) {
            addMirror(mirrors, "https://ftp.fau.de/gentoo/");
            addMirror(
                mirrors, "https://ftp.halifax.rwth-aachen.de/gentoo/"
            );
            addMirror(mirrors, "https://mirror.dogado.de/gentoo/");
        }
        return mirrors;
    }

    bool gameDataSizeIsValid(const std::string &path)
    {
        struct stat st {};
        if (stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
            return false;
        }
        return static_cast<std::int64_t>(st.st_size) == GAME_DATA_SIZE;
    }

    std::string sha256(const std::string &path)
    {
        std::FILE *f = std::fopen(path.c_str(), "rb");
        if (f == nullptr) {
            return "";
        }
        CC_SHA256_CTX ctx;
        CC_SHA256_Init(&ctx);
        char buffer[65536];
        std::size_t n;
        while ((n = std::fread(buffer, 1, sizeof(buffer), f)) > 0) {
            CC_SHA256_Update(
                &ctx, buffer, static_cast<CC_LONG>(n)
            );
        }
        const bool readOk = std::ferror(f) == 0;
        std::fclose(f);
        if (!readOk) {
            return "";
        }
        unsigned char digest[CC_SHA256_DIGEST_LENGTH];
        CC_SHA256_Final(digest, &ctx);
        std::string hex;
        for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) {
            char part[3];
            std::snprintf(part, sizeof(part), "%02x", digest[i]);
            hex += part;
        }
        return hex;
    }

    bool verifyGameData(const std::string &path)
    {
        return gameDataSizeIsValid(path)
            && sha256(path) == GAME_DATA_SHA256;
    }

    bool readFile(const std::string &path, std::vector<char> &data)
    {
        std::FILE *f = std::fopen(path.c_str(), "rb");
        if (f == nullptr) {
            return false;
        }
        if (std::fseek(f, 0L, SEEK_END) != 0) {
            std::fclose(f);
            return false;
        }
        const long size = std::ftell(f);
        if (size <= 0 || std::fseek(f, 0L, SEEK_SET) != 0) {
            std::fclose(f);
            return false;
        }
        data.resize(static_cast<std::size_t>(size));
        const std::size_t n = std::fread(&data[0], 1, data.size(), f);
        std::fclose(f);
        return n == data.size();
    }

    bool copyFile(const std::string &inPath, const std::string &outPath)
    {
        std::FILE *in = std::fopen(inPath.c_str(), "rb");
        if (in == nullptr) {
            return false;
        }
        const std::string temp = outPath + ".tmp";
        std::FILE *out = std::fopen(temp.c_str(), "wb");
        if (out == nullptr) {
            std::fclose(in);
            return false;
        }
        char buffer[4096];
        std::size_t n;
        bool ok = true;
        while ((n = std::fread(buffer, 1, sizeof(buffer), in)) > 0) {
            if (std::fwrite(buffer, 1, n, out) != n) {
                ok = false;
                break;
            }
        }
        if (std::ferror(in) != 0) {
            ok = false;
        }
        std::fclose(in);
        if (std::fclose(out) != 0) {
            ok = false;
        }
        if (!ok || std::rename(temp.c_str(), outPath.c_str()) != 0) {
            std::remove(temp.c_str());
            return false;
        }
        return true;
    }

    /**
     * XOR one file with the (repeating) game data zip, like xorbin.
     */
    bool decodeFile(
        const std::string &inPath, const std::vector<char> &key,
        const std::string &outPath
    )
    {
        std::FILE *in = std::fopen(inPath.c_str(), "rb");
        if (in == nullptr) {
            return false;
        }
        const std::string temp = outPath + ".tmp";
        std::FILE *out = std::fopen(temp.c_str(), "wb");
        if (out == nullptr) {
            std::fclose(in);
            return false;
        }
        unsigned char buffer[65536];
        std::size_t keyPos = 0;
        std::size_t n;
        bool ok = true;
        while ((n = std::fread(buffer, 1, sizeof(buffer), in)) > 0) {
            for (std::size_t i = 0; i < n; i++) {
                buffer[i] = static_cast<unsigned char>(
                    buffer[i] ^ static_cast<unsigned char>(key[keyPos])
                );
                keyPos++;
                if (keyPos == key.size()) {
                    keyPos = 0;
                }
            }
            if (std::fwrite(buffer, 1, n, out) != n) {
                ok = false;
                break;
            }
        }
        if (std::ferror(in) != 0) {
            ok = false;
        }
        std::fclose(in);
        if (std::fclose(out) != 0) {
            ok = false;
        }
        if (!ok || std::rename(temp.c_str(), outPath.c_str()) != 0) {
            std::remove(temp.c_str());
            return false;
        }
        return true;
    }

    /**
     * The documentation folder next to the app or in the working
     * directory, or an empty string when neither exists.
     */
    std::string documentationSource()
    {
        std::vector<std::string> candidates;
        const std::string bundle = bundlePath();
        if (!bundle.empty()) {
            const std::string parent = parentDirectory(bundle);
            if (!parent.empty()) {
                candidates.push_back(
                    parent + "/" + DOCS_DIR
                );
            }
        }
        candidates.push_back(DOCS_DIR);
        for (const auto &candidate: candidates) {
            if (directoryExists(candidate)) {
                return candidate;
            }
        }
        return "";
    }
}


/**
 * Download and verify ultima4.zip.  A verified copy that is already
 * present is accepted without touching the network.
 */
bool downloadGameData(const std::string &userPath, const bool interactive)
{
    const std::string dir = userDataDirectory(userPath);
    if (dir.empty()) {
        return false;
    }
    const std::string target = dir + GAME_DATA_FILE;
    if (verifyGameData(target)) {
        return true;
    }
    /* an incomplete or corrupted download is useless, and would keep
       failing the size check on every start */
    std::remove(target.c_str());
    const std::string temp = target + ".tmp";
    const std::vector<std::string> mirrors = readMirrors();
    const std::size_t count = mirrors.size();
    /* like the Windows launcher, do not try every mirror forever */
    const std::size_t attempts = count < 10 ? count : 10;
    const std::size_t start = count == 0 ? 0
        : static_cast<std::size_t>(std::time(nullptr)) % count;
    bool downloaded = false;
    for (std::size_t i = 0; i < attempts && !downloaded; i++) {
        const std::string &mirror = mirrors[(start + i) % count];
        const std::string url =
            mirror + "distfiles/59/" + GAME_DATA_FILE;
        std::remove(temp.c_str());
        if (!runProcess(
                {"/usr/bin/curl", "--fail", "--location", "--silent",
                 "--show-error", "--connect-timeout", "10",
                 "--max-time", "120", "--output", temp, url}
            )) {
            continue;
        }
        if (verifyGameData(temp)
            && std::rename(temp.c_str(), target.c_str()) == 0) {
            downloaded = true;
        }
    }
    std::remove(temp.c_str());
    if (!downloaded) {
        reportDownloadFailure(dir, interactive);
    }
    return downloaded;
}


/**
 * Decode the *.pdf.xor files shipped next to the app into
 * ~/Documents/Ultima IV Deutsch.  The folder is only created when at
 * least one encoded file still has to be decoded, so an already
 * extracted documentation costs nothing on later starts.
 */
std::string decodeDocumentation()
{
    const std::string source = documentationSource();
    if (source.empty()) {
        return "";
    }
    const std::string suffix(DOCS_SUFFIX);
    std::vector<std::string> encoded;
    DIR *dir = opendir(source.c_str());
    if (dir == nullptr) {
        return "";
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        const std::string name(entry->d_name);
        if (name.size() > suffix.size()
            && name.compare(
                   name.size() - suffix.size(), suffix.size(), suffix
               ) == 0) {
            encoded.push_back(name);
        }
    }
    closedir(dir);
    if (encoded.empty()) {
        return "";
    }
    const std::string key =
        u4find_path(GAME_DATA_FILE, u4Path.u4ZipPaths);
    if (key.empty()) {
        return "";
    }
    std::vector<char> keyData;
    if (!readFile(key, keyData)) {
        return "";
    }
    const std::string home = homeDirectory();
    if (home.empty()) {
        return "";
    }
    const std::string documents = home + "/Documents";
    const std::string target = documents + "/" + DOCS_TARGET_DIR;
    /* both may already exist; creating the folder can raise the
       one-time "Files and Folders" permission prompt */
    if (mkdir(documents.c_str(), 0755) != 0 && errno != EEXIST) {
        return "";
    }
    if (mkdir(target.c_str(), 0755) != 0 && errno != EEXIST) {
        return "";
    }
    const std::string xorSuffix(DOCS_XOR_SUFFIX);
    int decoded = 0;
    for (const auto &name: encoded) {
        const std::string outName =
            name.substr(0, name.size() - xorSuffix.size());
        const std::string outPath = target + "/" + outName;
        if (fileExists(outPath)) {
            continue;
        }
        if (decodeFile(source + "/" + name, keyData, outPath)) {
            decoded++;
        }
    }
    if (decoded == 0) {
        return "";
    }
    const std::string readme(DOCS_README);
    if (!fileExists(target + "/" + readme)) {
        copyFile(source + "/" + readme, target + "/" + readme);
    }
    /* reveal once, without activating Finder: this runs before the
       game takes over the screen, so fullscreen is not disturbed */
    runProcess({"/usr/bin/open", "-g", target});
    return target;
}

#else

bool downloadGameData(const std::string &, const bool)
{
    return false;
}

std::string decodeDocumentation()
{
    return "";
}

#endif
