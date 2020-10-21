#ifndef TEST_RUNNER_HH
#define TEST_RUNNER_HH

#include "utils/utils/color.hh"

#include <chrono>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

void print(bool)
{
    std::cout << rang::style::reset << "\n";
    fflush(NULL);
}

template <class T, class... Types>
void print(bool first, T s1, Types... args)
{
    using namespace rang;
    using namespace std;
    if (first)
        cout << style::bold << fg::green;
    cout << s1 << " ";
    print(false, args...);
}

void print_fail(bool)
{
    using namespace rang;
    using namespace std;
    cout << style::reset << endl;
    fflush(NULL);
}

template <class T, class... Types>
void print_fail(bool first, T s1, Types... args)
{
    using namespace rang;
    using namespace std;
    if (first)
        cout << style::bold << fg::red;
    cout << s1 << " ";
    print_fail(false, args...);
}

struct Assert {
    Assert() { count = 0; }
    template <class... Types>
    void operator()(bool cond, Types... msg) const
    {
        if (!cond) {
            print_fail(true, "Assertion Failed : ", msg...);
            abort();
        } else
            count++;
    }
    mutable uint count;
};

typedef std::function<void(void)> Test;

struct TestCase {
    Test test;
    TestCase() {}
    TestCase(Test _test) { test = _test; }

    void operator()() { test(); }

    TestCase& operator=(Test _test)
    {
        test = _test;
        return *this;
    }
};
enum Iter { N_ITER = 1 };

struct Runner {
    void operator()()
    {
        for (auto&& tc : testcases) {
            print(true, "--- Running Test", tc.first, "---");
            print(true, "Iterations:", iter);
            auto c = std::chrono::high_resolution_clock();
            auto begin = c.now();
            for (size_t i = 0; i < iter; i++) { tc.second(); }
            auto end = c.now();
            auto dur = std::chrono::duration_cast<std::chrono::microseconds>(
                end - begin);
            print(true, "Test", tc.first, "Successful. ", dur.count() / 1000,
                  " microsecs\n");
        }

        for (auto&& tc : tests) {
            print(true, "--- Running Test", tc.first, "---");
            print(true, "Iterations:", iter);
            auto c = std::chrono::high_resolution_clock();
            auto begin = c.now();
            for (size_t i = 0; i < iter; i++) { tc.second(); }
            auto end = c.now();
            auto dur = std::chrono::duration_cast<std::chrono::microseconds>(
                end - begin);
            print(true, "Test", tc.first, "Successful. ", dur.count() / 1000,
                  " microsecs\n");
        }
        print(true, "\n---------------");
        print(true, "Run", testcases.size() + tests.size(), "Tests.");
        print(true, _assert_true.count, "Asserts.");
    }

    void add(TestCase testcase, const char* name)
    {
        std::string _name(name);
        _name[0] = toupper(_name[0]);
        testcases.push_back(std::make_pair(_name, testcase));
    }
    void add(Test test, const char* name)
    {
        std::string _name(name);
        _name[0] = toupper(_name[0]);
        tests.push_back(std::make_pair(_name, test));
    }

    static Assert _assert_true;
    uint iter = 100;

 private:
    std::vector<std::pair<std::string, TestCase>> testcases;
    std::vector<std::pair<std::string, Test>> tests;
};

Assert Runner::_assert_true;

#define add_test(runner, test) runner.add(test, #test)
#define assert_true(cond) Runner::_assert_true(cond, #cond);
#define assert_false(cond) Runner::_assert_true(!(cond), #cond);
#endif