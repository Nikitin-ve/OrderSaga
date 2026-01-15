#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <stdexcept>

/**
 * @brief Простой тестовый фреймворк
 */

struct TestCase {
    std::string name;
    std::function<void()> func;
};

inline std::vector<TestCase>& GetTestCases() {
    static std::vector<TestCase> tests;
    return tests;
}

struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<void()> func) {
        GetTestCases().push_back({name, func});
    }
};

#define TEST(name) \
    void test_##name(); \
    TestRegistrar registrar_##name(#name, test_##name); \
    void test_##name()

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error("ASSERT_TRUE failed: " #condition); \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            throw std::runtime_error("ASSERT_FALSE failed: " #condition); \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    do { \
        auto _expected = (expected); \
        auto _actual = (actual); \
        if (_expected != _actual) { \
            std::ostringstream oss; \
            oss << "ASSERT_EQ failed: expected " << _expected << ", got " << _actual; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define ASSERT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            throw std::runtime_error("ASSERT_NE failed: values are equal"); \
        } \
    } while(0)

#define ASSERT_GT(val1, val2) \
    do { \
        if (!((val1) > (val2))) { \
            throw std::runtime_error("ASSERT_GT failed: " #val1 " <= " #val2); \
        } \
    } while(0)

#define ASSERT_GE(val1, val2) \
    do { \
        if (!((val1) >= (val2))) { \
            throw std::runtime_error("ASSERT_GE failed: " #val1 " < " #val2); \
        } \
    } while(0)

#define ASSERT_LT(val1, val2) \
    do { \
        if (!((val1) < (val2))) { \
            throw std::runtime_error("ASSERT_LT failed: " #val1 " >= " #val2); \
        } \
    } while(0)

#define ASSERT_LE(val1, val2) \
    do { \
        if (!((val1) <= (val2))) { \
            throw std::runtime_error("ASSERT_LE failed: " #val1 " > " #val2); \
        } \
    } while(0)

#define ASSERT_NEAR(expected, actual, epsilon) \
    do { \
        double _diff = std::abs((expected) - (actual)); \
        if (_diff > (epsilon)) { \
            std::ostringstream oss; \
            oss << "ASSERT_NEAR failed: |" << (expected) << " - " << (actual) << "| > " << (epsilon); \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define ASSERT_THROW(statement, exception_type) \
    do { \
        bool caught = false; \
        try { \
            statement; \
        } catch (const exception_type&) { \
            caught = true; \
        } catch (...) { \
            throw std::runtime_error("ASSERT_THROW: wrong exception type"); \
        } \
        if (!caught) { \
            throw std::runtime_error("ASSERT_THROW: no exception thrown"); \
        } \
    } while(0)

inline std::int32_t RunAllTests(const std::string& filter = "") {
    std::cout << "\n========================================\n";
    std::cout << "       Запуск тестов OrderSaga\n";
    std::cout << "========================================\n\n";
    
    std::int32_t passed = 0;
    std::int32_t failed = 0;
    std::int32_t skipped = 0;
    
    for (const auto& test : GetTestCases()) {
        if (!filter.empty() && test.name.find(filter) == std::string::npos) {
            skipped++;
            continue;
        }
        
        std::cout << "[ RUN      ] " << test.name << "\n";
        try {
            test.func();
            std::cout << "[       OK ] " << test.name << "\n";
            passed++;
        } catch (const std::exception& e) {
            std::cout << "[  FAILED  ] " << test.name << "\n";
            std::cout << "             " << e.what() << "\n";
            failed++;
        }
    }
    
    std::cout << "\n========================================\n";
    std::cout << "Результаты: " << passed << " из " << (passed + failed) << " тестов прошло";
    if (skipped > 0) {
        std::cout << " (" << skipped << " пропущено)";
    }
    std::cout << "\n";
    
    if (failed == 0) {
        std::cout << "Все тесты успешно пройдены!\n";
    } else {
        std::cout << "ВНИМАНИЕ: " << failed << " тестов не прошло!\n";
    }
    std::cout << "========================================\n\n";
    
    return failed;
}
