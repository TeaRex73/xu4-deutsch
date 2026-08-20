#ifndef U6STACK_H
#define U6STACK_H

#include <vector>

namespace U6Decode
{

class Stack {
public:
    Stack() = default;

    bool is_empty() const
    {
        return stack.empty();
    }

    bool is_full() const
    {
        return stack.size() == stack_size;
    }

    void push(const unsigned char element)
    {
        if (!is_full()) {
            stack.push_back(element);
        }
    }

    unsigned char pop()
    {
        unsigned char element;
        if (!is_empty()) {
            element = stack.back();
            stack.pop_back();
        } else {
            element = 0;
        }
        return element;
    }

    unsigned char get_top() const
    {
        if (!is_empty()) {
            return stack.back();
        }
        return 0;
    }

private:
    static constexpr unsigned int stack_size = 10000;
    std::vector<unsigned char> stack;
};

}
#endif // U6STACK_H
