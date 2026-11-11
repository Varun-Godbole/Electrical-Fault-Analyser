/**
 * @file gtest.h
 * @brief Lightweight Google Test compatible framework header.
 */

#ifndef GTEST_GTEST_H
#define GTEST_GTEST_H

#include <iostream>
#include <vector>
#include <string>
#include <cmath>

namespace testing {

class Test {
public:
    virtual ~Test() {}
    virtual void TestBody() = 0;
};

struct TestInfo {
    std::string test_case_name;
    std::string test_name;
    Test* (*factory)();
};

class UnitTest {
public:
    static UnitTest* GetInstance() {
        static UnitTest instance;
        return &instance;
    }

    void RegisterTest(const char* test_case_name, const char* test_name, Test* (*factory)()) {
        TestInfo info;
        info.test_case_name = test_case_name;
        info.test_name = test_name;
        info.factory = factory;
        tests_.push_back(info);
    }

    int RunAllTests() {
        int total = tests_.size();
        int passed = 0;
        int failed = 0;

        std::cout << "[==========] Running " << total << " tests from test suite.\n";
        for (const auto& test_info : tests_) {
            std::cout << "[ RUN      ] " << test_info.test_case_name << "." << test_info.test_name << "\n";
            current_test_failed_ = false;

            Test* test = test_info.factory();
            try {
                test->TestBody();
            } catch (...) {
                current_test_failed_ = true;
            }
            delete test;

            if (current_test_failed_) {
                std::cout << "[  FAILED  ] " << test_info.test_case_name << "." << test_info.test_name << "\n";
                failed++;
            } else {
                std::cout << "[       OK ] " << test_info.test_case_name << "." << test_info.test_name << "\n";
                passed++;
            }
        }
        std::cout << "[==========] " << total << " tests ran.\n";
        std::cout << "[  PASSED  ] " << passed << " tests.\n";
        if (failed > 0) {
            std::cout << "[  FAILED  ] " << failed << " tests.\n";
            return 1;
        }
        return 0;
    }

    void SetFailed() {
        current_test_failed_ = true;
    }

private:
    std::vector<TestInfo> tests_;
    bool current_test_failed_ = false;
};

inline void InitGoogleTest(int* /*argc*/, char** /*argv*/) {}

} // namespace testing

#define TEST(test_case_name, test_name) \
    class test_case_name##_##test_name##_Test : public ::testing::Test { \
    public: \
        void TestBody() override; \
        static ::testing::Test* Create() { return new test_case_name##_##test_name##_Test(); } \
    }; \
    static int dummy_##test_case_name##_##test_name = []() { \
        ::testing::UnitTest::GetInstance()->RegisterTest(#test_case_name, #test_name, &test_case_name##_##test_name##_Test::Create); \
        return 0; \
    }(); \
    void test_case_name##_##test_name##_Test::TestBody()

#define EXPECT_EQ(val1, val2) do { \
    if (!((val1) == (val2))) { \
        std::cout << "  Failure: " << #val1 << " (" << (val1) << ") == " << #val2 << " (" << (val2) << ")\n"; \
        ::testing::UnitTest::GetInstance()->SetFailed(); \
    } \
} while (0)

#define EXPECT_TRUE(cond) do { \
    if (!(cond)) { \
        std::cout << "  Failure: Expected true: " << #cond << "\n"; \
        ::testing::UnitTest::GetInstance()->SetFailed(); \
    } \
} while (0)

#define EXPECT_FALSE(cond) do { \
    if (cond) { \
        std::cout << "  Failure: Expected false: " << #cond << "\n"; \
        ::testing::UnitTest::GetInstance()->SetFailed(); \
    } \
} while (0)

#define EXPECT_NEAR(val1, val2, abs_error) do { \
    if (std::fabs((val1) - (val2)) > (abs_error)) { \
        std::cout << "  Failure: Expected near: " << #val1 << " (" << (val1) << ") and " << #val2 << " (" << (val2) << "), error=" << std::fabs((val1) - (val2)) << " > " << (abs_error) << "\n"; \
        ::testing::UnitTest::GetInstance()->SetFailed(); \
    } \
} while (0)

#define RUN_ALL_TESTS() ::testing::UnitTest::GetInstance()->RunAllTests()

#endif // GTEST_GTEST_H
