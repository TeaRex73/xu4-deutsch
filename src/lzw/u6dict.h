#ifndef U6DICT_H
#define U6DICT_H

#include <vector>

namespace U6Decode
{

class Dict {
public:

    Dict()
        : contains(0x102)
    {
    }

    void init()
    {
        contains = 0x102;
    }

    void add(const unsigned char root, const int codeword)
    {
        dict.resize(contains+1);
        dict[contains].root = root;
        dict[contains].codeword = codeword;
        contains++;
    }

    unsigned char get_root(const int codeword) const
    {
        return dict[codeword].root;
    }

    int get_codeword(const int codeword) const
    {
        return dict[codeword].codeword;
    }

private:
    struct dict_entry {
        unsigned char root;
        int codeword;
    };

    std::vector<dict_entry> dict;
    int contains;
};

}
#endif // U6DICT_H
