/**
 * @file
 * @brief utility functions for tests
 *
 * (c) CossackLabs
 */

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "common/error.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef int test_utils_status_t;

test_utils_status_t string_to_bytes(const char *str, uint8_t *bytes, size_t size);

/* These are wrappers for sput test framework. Since sput uses a static state variable, when called from different files, it resets test number count (since for each file different test context is used). */
void testsuite_start_testing(void);
void testsuite_enter_suite(const char *suite_name);
void testsuite_run_test_in_file(void (*test_func)(void), const char *test_func_name, const char *test_file_name);
void testsuite_finish_testing(void);
void testsuite_fail_if_on_line(bool condition, const char *name, unsigned long line);
void testsuite_fail_unless_on_line(bool condition, const char *name, unsigned long line);

#define testsuite_fail_if(_COND_, _NAME_) testsuite_fail_if_on_line(_COND_, _NAME_, __LINE__)
#define testsuite_fail_unless(_COND_, _NAME_) testsuite_fail_unless_on_line(_COND_, _NAME_, __LINE__)
#define testsuite_run_test(_FUNC_) testsuite_run_test_in_file(_FUNC_, #_FUNC_, __FILE__)

#endif /* TEST_UTILS_H */
