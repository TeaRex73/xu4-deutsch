#include <cstdint>
#include <map>
#include <string>
#include <vector>

typedef std::vector<std::uint8_t> ByteVector;

typedef struct {
    std::string name;
    ByteVector contents;
} InternalFile;

typedef std::map<std::string, InternalFile> XorDataMap;

extern const XorDataMap xorDataMap;
