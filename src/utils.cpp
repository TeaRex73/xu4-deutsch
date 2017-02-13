/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>
#endif

#include "utils.h"
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ctime>




/**
 * Seed the random number generator.
 */
void xu4_srandom()
{
    std::srand((unsigned int)std::time(nullptr));
}


/**
 * Generate a random number between 0 and (upperRange - 1).
 */
int xu4_random(int upperRange)
{
    int r;
    int rand_limit = RAND_MAX - (((RAND_MAX % upperRange) + 1) % upperRange);
    do {
        r = std::rand();
    } while (r > rand_limit);
    return r % upperRange; 
}


/**
 * Trims whitespace from a string
 * @param val The string you are trimming
 * @param chars_to_trim A list of characters that will be trimmed
 */
std::string &trim(std::string &val, const std::string &chars_to_trim)
{
    std::string::iterator i;
    if (val.size()) {
        std::string::size_type pos;
        for (i = val.begin();
             (i != val.end()) &&
                 (pos = chars_to_trim.find(*i)) != std::string::npos;) {
            i = val.erase(i);
        }
        for (i = val.end() - 1;
             (i != val.begin()) &&
                 (pos = chars_to_trim.find(*i)) != std::string::npos;) {
            i = val.erase(i) - 1;
        }
    }
    return val;
}

int xu4_islower(int c)
{
    return ((c >= 'a') && (c <= '~'));
}

int xu4_toupper(int c)
{
    if ((c >= 'a') && (c <= '}')) {
        return c - 32;
    }
    return c;
}

int xu4_tolower(int c)
{
    if ((c >= 'A') && (c <= ']')) {
        return c + 32;
    }
    return c;
}

int xu4_strcasecmp(const char *s1, const char *s2)
{
    unsigned char c1,c2;
    if (s1 == s2) {
        return 0;
    }
    do {
        c1 = xu4_tolower(*s1++);
        c2 = xu4_tolower(*s2++);
        if (c1 == '\0') {
            break;
        }
    } while (c1 == c2);
    return c1 - c2;
}

int xu4_strncasecmp(const char *s1, const char *s2, std::size_t n)
{
    unsigned char c1,c2;
    if (s1 == s2 || n == 0) {
        return 0;
    }
    do {
        c1 = xu4_tolower(*s1++);
        c2 = xu4_tolower(*s2++);
        if (--n == 0 || c1 == '\0') {
            break;
        }
    } while (c1 == c2);
    return c1 - c2;
}

char *xu4_strdup(const char *s)
{
    std::size_t length = std::strlen(s) + 1;
    void *copy = std::malloc(length);
    if (copy == nullptr) {
        return nullptr;
    }
    return (char *)std::memcpy(copy, s, length);
}

/**
 * Converts the string to lowercase
 */
std::string lowercase(std::string val)
{
    std::string::iterator i;
    for (i = val.begin(); i != val.end(); i++) {
        *i = xu4_tolower(*i);
    }
    return val;
}


/**
 * Converts the string to uppercase
 */
std::string uppercase(std::string val)
{
    std::string ret = "";
    std::string::iterator i;
    for (i = val.begin(); i != val.end(); i++) {
        if (*i == '~') {
            ret += "SS";
        } else {
            ret += std::string(1, (char)xu4_toupper(*i));
        }
    }
    return ret;
}

std::string deumlaut(std::string val)
{
    std::string ret = "";
    std::string::iterator i;
    for (i = val.begin(); i != val.end(); i++) {
        switch (*i) {
        case '[':
            ret += "AE";
            break;
        case '\\':
            ret += "OE";
            break;
        case ']':
            ret += "UE";
            break;
        case '{':
            ret += "ae";
            break;
        case '|':
            ret += "oe";
            break;
        case '}':
            ret += "ue";
            break;
        case '~':
            ret += "ss";
            break;
        default:
            ret += std::string(1, (char)xu4_toupper(*i));
        }
    }
    return ret;
} // deumlaut


/**
 * Converts an integer value to a string
 */
std::string to_string(int val)
{
    char buffer[16];
    std::sprintf(buffer, "%d", val);
    return buffer;
}


/**
 * Splits a string into substrings, divided by the characters in
 * separators.  Multiple adjacent seperators are treated as one.
 */
std::vector<std::string> split(
    const std::string &s, const std::string &separators
)
{
    std::vector<std::string> result;
    std::string current;
    for (unsigned int i = 0; i < s.length(); i++) {
        if (separators.find(s[i]) != std::string::npos) {
            if (current.length() > 0) {
                result.push_back(current);
            }
            current.erase();
        } else {
            current += s[i];
        }
    }
    if (current.length() > 0) {
        result.push_back(current);
    }
    return result;
}
