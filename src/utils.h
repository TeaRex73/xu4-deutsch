/*
 * $Id$
 */

#ifndef UTILS_H
#define UTILS_H

#include <cstdio>
#include <ctime>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

#include <unistd.h>

#include "filesystem.h"




/* The AdjustValue functions used to be #define'd macros, but these are
 * evil for several reasons, *especially* when they contain multiple
 * statements, and have if statements in them. The macros did both.
 * See http://www.parashift.com/c++-faq-lite/inline-functions.html#faq-9.5
 * for more information.
 */

// for unsigned types, checks for overflow/underflow
template<typename T, typename U> inline void AdjustValueMax_helper(
    T &v, typename std::make_signed<T>::type val, U max, std::true_type
)
{
    T old_v = v;
    T maxt = static_cast<T>(max);
    v += val;
    if (v > maxt || (val > 0 && v < old_v)) {
        v = maxt;
    }
}

// for signed types, no check since signed overflow is undefined behaviour
template<typename T, typename U> inline void AdjustValueMax_helper(
    T &v, typename std::make_signed<T>::type val, U max, std::false_type
)
{
    T maxt = static_cast<T>(max);
    v += val;
    if (v > maxt) {
        v = maxt;
    }
}

// use tag dispatch to chose the right one
template<typename T, typename U> inline void AdjustValueMax(
    T &v, typename std::make_signed<T>::type val, U max
)
{
    AdjustValueMax_helper(
        v, val, max, typename std::is_unsigned<T>::type()
    );
}

// for unsigned types, checks for overflow/underflow
template<typename T, typename U> inline void AdjustValueMin_helper(
    T &v, typename std::make_signed<T>::type val, U min, std::true_type
)
{
    T old_v = v;
    T mint = static_cast<T>(min);
    v += val;
    if (v < mint || (val < 0 && v > old_v)) {
        v = mint;
    }
}

// for signed types, no check since signed overflow is undefined behaviour
template<typename T, typename U> inline void AdjustValueMin_helper(
    T &v, typename std::make_signed<T>::type val, U min, std::false_type
)
{
    T mint = static_cast<T>(min);
    v += val;
    if (v < mint) {
        v = mint;
    }
}

// use tag dispatch to chose the right one
template<typename T, typename U> inline void AdjustValueMin(
    T &v, typename std::make_signed<T>::type val, U min
)
{
    AdjustValueMin_helper(
        v, val, min, typename std::is_unsigned<T>::type()
    );
}

// for unsigned types, checks for overflow/underflow
template<typename T, typename U> inline void AdjustValue_helper(
    T &v, typename std::make_signed<T>::type val, U max, U min, std::true_type
)
{
    T old_v = v;
    T maxt = static_cast<T>(max);
    T mint = static_cast<T>(min);
    v += val;
    if (v > maxt || (val > 0 && v < old_v)) {
        v = maxt;
    }
    if (v < mint || (val < 0 && v > old_v)) {
        v = mint;
    }
}

// for signed types, no check since signed overflow is undefined behaviour
template<typename T, typename U> inline void AdjustValue_helper(
    T &v, typename std::make_signed<T>::type val, U max, U min, std::false_type
)
{
    T maxt = static_cast<T>(max);
    T mint = static_cast<T>(min);
    v += val;
    if (v > maxt) {
        v = maxt;
    }
    if (v < mint) {
        v = mint;
    }
}

// use tag dispatch to chose the right one
template<typename T, typename U> inline void AdjustValue(
    T &v, typename std::make_signed<T>::type val, U max, U min
)
{
    AdjustValue_helper(
        v, val, max, min, typename std::is_unsigned<T>::type()
    );
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
std::string xu4_to_string(int val);
std::vector<std::string> split(
    const std::string &s, const std::string &separators
);

class Performance {
private:
    typedef std::map<std::string, std::clock_t> TimeMap;

public:
    Performance(const std::string &
#ifndef NPERF
                                s
#endif
                           )
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

    void init(const std::string &
#ifndef NPERF
                          s
#endif
                         )
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

    void end(const std::string &
#ifndef NPERF
                         funcName
#endif
                        )
    {
#ifndef NPERF
        e = std::clock();
        times[funcName] = e - s;
#endif
    }

    void report(const char *
#ifndef NPERF
                                pre
#endif
                            = nullptr)
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
        std::fflush(log);
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
