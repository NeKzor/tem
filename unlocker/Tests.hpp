#pragma once
#include <exception>
#include <format>
#include <vector>

using _UnitTestFunction = void (*)(struct UnitTest* test);

struct UnitTest {
    const char* case_name;
    const char* description;
    _UnitTestFunction callback;

    static std::vector<UnitTest*>& test_cases();

    UnitTest(const char* case_name, const char* description, _UnitTestFunction callback);
};

#define TEST(case_name, description)                                                                                   \
    void case_name##description##_callback(UnitTest* test);                                                            \
    UnitTest case_name##description(#case_name, #description, case_name##description##_callback);                      \
    void case_name##description##_callback(UnitTest* test)

#define EXPECT_TRUE(condition)                                                                                         \
    if (!condition) {                                                                                                  \
        throw std::exception(std::format("[failed] {} - {}\n - condition \"{}\" evaluated to to false",                \
            test->case_name, test->description, #condition)                                                            \
                                 .c_str());                                                                            \
    }
#define EXPECT_EQ(actual, expected)                                                                                    \
    if (!(actual == expected)) {                                                                                       \
        throw std::exception(std::format(                                                                              \
            "[failed] {} - {}\n - expected: {}\n - actual: {}", test->case_name, test->description, expected, actual)  \
                                 .c_str());                                                                            \
    }

extern auto run_all_tests() -> int;
