/*
 * MIT License
 *
 * Copyright (c) 2025 Didarul Islam <didarulislam85@hotmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * t_framework.h - v.1.0.0 - A minimalist single-header testing framework.
 */

#ifndef T_FRAMEWORK_H
#define T_FRAMEWORK_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

/* ANSI color codes */
#define ANSI_COLOR_BLUE      "\x1B[1;34m"
#define ANSI_COLOR_YELLOW    "\x1B[33m"
#define ANSI_COLOR_RED       "\x1B[1;31m"
#define ANSI_COLOR_GREEN     "\x1B[1;32m"
#define ANSI_COLOR_RESET     "\x1B[0m"

typedef struct {
    size_t case_total;
    size_t case_passed;
    size_t case_failed;
    size_t assert_total;
    size_t assert_passed;
    size_t assert_failed;
} test_t;

typedef struct {
    uint64_t start;
    uint64_t duration;
} test_timer_t;

static size_t FAILED = 0;
static test_t test = {0};
static test_timer_t test_timer = {0};

/*
 * Get monotonic clock time in microseconds
 */
static inline uint64_t TEST_CLOCK_MONO(void)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now)) {
        exit(EXIT_FAILURE);
    }
    return ((uint64_t)now.tv_sec) * 1000000 + (uint64_t)(now.tv_nsec / 1000);
}

#define TEST_TIMER_START() (test_timer.start = TEST_CLOCK_MONO())
#define TEST_TIMER_END() (test_timer.duration = TEST_CLOCK_MONO() - test_timer.start)

#define TEST_ADD(func, ...) void func(__VA_ARGS__)

#define TEST_RUN(func, ...)                                                              \
    do {                                                                                 \
        ++test.case_total;                                                               \
        FAILED = 0; /* Reset before each test case */                                    \
        printf("\n" ANSI_COLOR_BLUE "🧪 [%s]\n" ANSI_COLOR_RESET, #func);                \
        func(__VA_ARGS__);                                                               \
        printf(ANSI_COLOR_YELLOW "⏳  Time: %" PRIu64 " μs\n" ANSI_COLOR_RESET,          \
               test_timer.duration);                                                     \
        test_timer.duration = 0; /* Reset after each test case */                        \
        if (FAILED) {                                                                    \
            ++test.case_failed;                                                          \
        } else {                                                                         \
            ++test.case_passed;                                                          \
        }                                                                                \
    } while (0)

#define ASSERT(expr)                                                                     \
    do {                                                                                 \
        ++test.assert_total;                                                             \
        if (!(expr)) {                                                                   \
            FAILED = 1;                                                                  \
            ++test.assert_failed;                                                        \
            printf("❌  Failed [line: %d] - %s\n", __LINE__, #expr);                     \
        } else {                                                                         \
            ++test.assert_passed;                                                        \
            printf("✅  Passed - %s\n", #expr);                                          \
        }                                                                                \
    } while (0)

#define TEST_LOOP_START(id)                                                              \
    size_t total_##id = 0;                                                               \
    size_t passed_##id = 0;                                                              \
    size_t failed_##id = 0;

#define ASSERT_LOOP(expr, id)                                                            \
    do {                                                                                 \
        ++test.assert_total;                                                             \
        ++total_##id;                                                                    \
        if (!(expr)) {                                                                   \
            FAILED = 1;                                                                  \
            ++test.assert_failed;                                                        \
            ++failed_##id;                                                               \
            printf("❌  Failed [line: %d] - %s\n", __LINE__, #expr);                     \
        } else {                                                                         \
            ++test.assert_passed;                                                        \
            ++passed_##id;                                                               \
        }                                                                                \
    } while (0)

#define TEST_LOOP_END(id)                                                                \
    do {                                                                                 \
        if (!failed_##id) {                                                              \
            printf("✅  Passed - %zu/%zu iteration assertions\n", passed_##id,           \
                   total_##id);                                                          \
        } else {                                                                         \
            printf("❌  Failed - %zu/%zu iteration assertions\n", failed_##id,           \
                   total_##id);                                                          \
        }                                                                                \
    } while (0)

#define TEST_PRINT_SUMMARY()                                                             \
    do {                                                                                 \
        printf("\n" ANSI_COLOR_BLUE "TEST CASES: %10zu" ANSI_COLOR_RESET "  |  ",        \
               test.case_total);                                                         \
        printf(ANSI_COLOR_GREEN "%10zu PASSED" ANSI_COLOR_RESET "  |  ",                 \
               test.case_passed);                                                        \
        printf(ANSI_COLOR_RED "%10zu FAILED" ANSI_COLOR_RESET "\n", test.case_failed);   \
        printf(ANSI_COLOR_BLUE "ASSERTIONS: %10zu" ANSI_COLOR_RESET "  |  ",             \
               test.assert_total);                                                       \
        printf(ANSI_COLOR_GREEN "%10zu PASSED" ANSI_COLOR_RESET "  |  ",                 \
               test.assert_passed);                                                      \
        printf(ANSI_COLOR_RED "%10zu FAILED" ANSI_COLOR_RESET "\n\n",                    \
               test.assert_failed);                                                      \
    } while (0)

#define TEST_MAIN(...)                                                                   \
    int main(void)                                                                       \
    {                                                                                    \
        __VA_ARGS__;                                                                     \
        TEST_PRINT_SUMMARY();                                                            \
        return test.case_failed;                                                         \
    }

#endif /* T_FRAMEWORK_H */
