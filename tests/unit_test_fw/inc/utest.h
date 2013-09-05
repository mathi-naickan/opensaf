/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Ericsson AB
 *
 */

#ifndef test_h
#define test_h

#include <logtrace.h>
#include <saAis.h>

#define ALL_SUITES -1
#define ALL_TESTS -1

/* This variable will be set to 1 if the environment variable TESTDEBUG is defined */
extern unsigned int testdebug;

/**
 * Associate a test suite number with a printable name.
 * @param suite suite number
 * @param name name of suite
 */
extern void test_suite_add(unsigned int suite, const char *name);

/**
 * Install a test case, associate it with a test suite and a
 * printable slogan.
 * @param suite suite number
 * @param testfunc function pointer to test case
 * @param slogan printable slogan
 */
extern void test_case_add(unsigned int suite, void(*testfunc)(void), const char *slogan);

/**
 * Validate and print result of a test case.
 * NOTE: Deprecated. Use aisrc_validate()
 * @param actual actual status code
 * @param expected expected status code
 */
extern void test_validate(SaUint32T actual, SaUint32T expected);

/**
 * Validate and print result of a test case.
 * Used when rc is an SA_AIS_XXX return code
 * @param actual actual status code
 * @param expected expected status code
 */
void aisrc_validate(SaAisErrorT rc, SaAisErrorT expected);

/**
 * Validate and print result of a test case.
 * Used when rc is an exit code from an external tool e.g. immadm
 * @param rc
 * @param expected
 */
void rc_validate(int rc, int expected);

/**
 * Run all test cases, all test cases in a suite or a specific
 * test case. suites will be run in increasing order using the
 * test suite number. Test cases will be run in order installed.
 * 
 * @param suite ALL_SUITES or a specific number
 * @param tcase ALL_TESTS or a specific number
 * 
 * @return int zero if sucess or -1 if an error occurred
 */
extern int test_run(unsigned int suite, unsigned int tcase);

/**
 * List all test cases
 */
extern void test_list(void);

#endif

