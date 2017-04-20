/*
 * $Id$
 */

#ifndef UTILS_H
#define UTILS_H

#include <cstdio>
#include <ctime>
#include <map>
#include <string>
#include <vector>

#include <unistd.h>

#include "filesystem.h"




/* The AdjustValue functions used to be #define'd macros, but these are
 * evil for several reasons, *especially* when they contain multiple
 * statements, and have if statements in them. The macros did both.
 * See http://www.parashift.com/c++-faq-lite/inline-functions.html#faq-9.5
 * for more information.
 */
inline void AdjustValueMax(int &v, int val, int max)
{
    v += val;
    if (v > max) {
        v = max;
    }
}

inline void AdjustValueMin(int &v, int val, int min)
{
    v += val;
    if (v < min) {
        v = min;
    }
}

inline void AdjustValue(int &v, int val, int max, int min)
{
    v += val;
    if (v > max) {
        v = max;
    }
    if (v < min) {
        v = min;
    }
}

inline void AdjustValueMax(short &v, int val, int max)
{
    v += val;
    if (v > max) {
        v = max;
    }
}

inline void AdjustValueMin(short &v, int val, int min)
{
    v += val;
    if (v < min) {
        v = min;
    }
}

inline void AdjustValue(short &v, int val, int max, int min)
{
    v += val;
    if (v > max) {
        v = max;
    }
    if (v < min) {
        v = min;
    }
}

inline void AdjustValueMax(unsigned short &v, int val, int max)
{
    v += val;
    if (v > max) {
        v = max;
    }
}

inline void AdjustValueMin(unsigned short &v, int val, int min)
{
    v += val;
    if (v < min) {
        v = min;
    }
}

inline void AdjustValue(unsigned short &v, int val, int max, int min)
{
    v += val;
    if (v > max) {
        v = max;
    }
    if (v < min) {
        v = min;
    }
}

void xu4_srandom();
int xu4_random(int upperval);
int xu4_islower(int c);
int xu4_toupper(int c);
int xu4_tolower(int c);
int xu4_strcasecmp(const char *s1, const char *s2);
int xu4_strncasecmp(const char *s1, const char *s2, std::size_t n);
char *xu4_strdup(const char *s);
std::string &trim(
    std::string &val, const std::string &chars_to_trim = "\t\013\014 \n\r"
);
std::string lowercase(std::string val);
std::string uppercase(std::string val);
std::string deumlaut(std::string val);
std::string to_string(int val);
std::vector<std::string> split(
    const std::string &s, const std::string &separators
);

class Performance {
private:
    typedef std::map<std::string, std::clock_t> TimeMap;

public:
    Performance(const std::string &s)
		:log(), filename(), s(), e(), times()
    {
#ifndef NPERF
        init(s);
#endif
    }
    Performance(const Performance &) = delete;
	Performance(Performance &&) = delete;
	Performance &operator=(const Performance &) = delete;
	Performance &operator=(Performance &&) = delete;
    void init(const std::string &s)
    {
#ifndef NPERF
        Path path(s);
        FileSystem::createDirectory(path);
        filename = path.getPath();
        log = std::fopen(filename.c_str(), "wt");
        if (!log) {
            // FIXME: throw exception
            return;
        }
#endif
    }
    
    void reset()
    {
#ifndef NPERF
        if (!log) {
            log = std::fopen(filename.c_str(), "at");
            if (!log) {
                // FIXME: throw exception
                return;
            }
        }
#endif
    }

    void start()
    {
#ifndef NPERF
        s = std::clock();
#endif
    }
    
    void end(const std::string &funcName)
    {
#ifndef NPERF
        e = std::clock();
        times[funcName] = e - s;
#endif
    }
    
    void report(const char *pre = nullptr)
    {
#ifndef NPERF
        static const double msec = double(CLOCKS_PER_SEC) / double(1000);
        TimeMap::const_iterator i;
        std::clock_t total = 0;
        std::map<double, std::string> percentages;
        std::map<double, std::string>::iterator perc;
        if (pre) { std::fprintf(log, "%s", pre); }
        for (i = times.begin(); i != times.end(); i++) {
            std::fprintf(
                log,
                "%s [%0.2f msecs]\n",
                i->first.c_str(),
                double(i->second) / msec
            );
            total += i->second;
        }
        for (i = times.begin(); i != times.end(); i++) {
            double perc = 100.0 * double(i->second) / total;
            percentages[perc] = i->first;
        }
        std::fprintf(log, "\n");
        for (perc = percentages.begin(); perc != percentages.end(); perc++) {
            std::fprintf(
                log, "%0.1f%% - %s\n", perc->first, perc->second.c_str()
            );
        }
        std::fprintf(log, "\nTotal [%0.2f msecs]\n", double(total) / msec);
        fsync(fileno(log));
        std::fclose(log);
        sync();
        log = nullptr;
        times.clear();
#endif // ifndef NPERF
    } // report

private:
    std::FILE *log;
    std::string filename;
    std::clock_t s, e;
    TimeMap times;
};

#endif // ifndef UTILS_H
