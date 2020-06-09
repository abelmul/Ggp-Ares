#ifndef EXCEPTIONS_HH
#define EXCEPTIONS_HH

#include <stdexcept>

namespace Ares
{
    class UnbalancedParentheses : public std::runtime_error
    {
    private:
        /* data */
    public:
        UnbalancedParentheses(std::string s):std::runtime_error(s) {}
        UnbalancedParentheses(const char* c):std::runtime_error(c) {}
        ~UnbalancedParentheses() {}
    };
    class SyntaxError : public std::runtime_error
    {
    private:
        /* data */
    public:
        SyntaxError(std::string s):std::runtime_error(s) {}
        SyntaxError(const char* c):std::runtime_error(c) {}
        ~SyntaxError() {}
    };
    
} // namespace Ares

#endif