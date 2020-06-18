#ifndef EXCEPTIONS_HH
#define EXCEPTIONS_HH

#include <stdexcept>

namespace ares
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

    class DistinctNotGround : public std::runtime_error
    {
    private:
        /* data */
    public:
        DistinctNotGround(std::string s):std::runtime_error(s) {}
        DistinctNotGround(const char* c):std::runtime_error(c) {}
        ~DistinctNotGround() {}
    };
    class NegationNotGround : public std::runtime_error
    {
    private:
        /* data */
    public:
        NegationNotGround(std::string s):std::runtime_error(s) {}
        NegationNotGround(const char* c):std::runtime_error(c) {}
        ~NegationNotGround() {}
    };
    class IndexOutOfRange : public std::runtime_error
    {
    private:
        /* data */
    public:
        IndexOutOfRange(std::string s):std::runtime_error(s) {}
        IndexOutOfRange(const char* c):std::runtime_error(c) {}
        ~IndexOutOfRange() {}
    };
    class EmptyBodyPop : public std::runtime_error
    {
    private:
        /* data */
    public:
        EmptyBodyPop(std::string s):std::runtime_error(s) {}
        EmptyBodyPop(const char* c):std::runtime_error(c) {}
        ~EmptyBodyPop() {}
    };

    class NonExistentTerm : public std::runtime_error
    {
    private:
        /* data */
    public:
        NonExistentTerm(std::string s):std::runtime_error(s) {}
        NonExistentTerm(const char* c):std::runtime_error(c) {}
        ~NonExistentTerm() {}
    };

    class BadAllocation : public std::runtime_error
    {
    private:
        /* data */
    public:
        BadAllocation(std::string s):std::runtime_error(s) {}
        BadAllocation(const char* c):std::runtime_error(c) {}
        ~BadAllocation() {}
    };
} // namespace ares

#endif