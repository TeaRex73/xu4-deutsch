#include <vector>

using std::vector;

class U6Decode::Dict {
public:

    Dict()
        :dict(), contains(0x102)
    {
    }

    void init()
    {
        contains = 0x102;
    }

    void add(unsigned char root, int codeword)
    {
        dict.resize(contains+1);
        dict[contains].root = root;
        dict[contains].codeword = codeword;
        contains++;
    }

    unsigned char get_root(int codeword) const
    {
        return dict[codeword].root;
    }

    int get_codeword(int codeword) const
    {
        return dict[codeword].codeword;
    }

private:
    struct dict_entry {
        unsigned char root;
        int codeword;
    };

    vector<dict_entry> dict;
    int contains;
};
