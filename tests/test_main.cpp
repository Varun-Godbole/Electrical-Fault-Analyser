/**
 * @file test_main.cpp
 * @brief Main entry point runner for Google Test test suite.
 */

#include "gtest/gtest.h"

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    std::cout << "\n=================================================================\n";
    std::cout << "  GOOGLE TEST RUNNER: ELECTRICAL FAULT DIAGNOSIS & PROTECTION\n";
    std::cout << "=================================================================\n\n";
    return RUN_ALL_TESTS();
}
