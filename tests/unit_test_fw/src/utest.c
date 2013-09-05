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

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <saAis.h>
#include "utest.h"
#include "util.h"

struct test
{
    void (*testfunc)();
    const char *slogan;
};

/* test statistics */
static unsigned int test_total;
static unsigned int test_passed;
static unsigned int test_failed;

#define NO_SUITES 64
#define NR_TESTS 64
static struct test testlist[NO_SUITES][NR_TESTS];
static const char *suite_name[NO_SUITES];
static int last_test_status;
static SaAisErrorT actualStatus, expectedStatus;
static unsigned int current_test;

unsigned int testdebug;

/**
 * Used when rc is an SA_AIS_XXX reurn code
 * 
 * NOTE: Deprecated. Use aisrc_validate()
 * 
 * @param rc
 * @param expected
 */
void test_validate(SaUint32T rc, SaUint32T expected)
{
    /* Save for later use */
    actualStatus = rc;
    expectedStatus = expected;

    if (rc == expected)
    {
        test_passed++;
        printf("  %3d  PASSED", current_test);
        last_test_status = 0;
    }
    else
    {
        test_failed++;
        printf("  %3d  FAILED", current_test);
        last_test_status = -1;
    }

    test_total++;
}

/**
 * Used when rc is an SA_AIS_XXX reurn code
 * 
 * @param rc
 * @param expected
 */
void aisrc_validate(SaAisErrorT rc, SaAisErrorT expected)
{
    /* Save for later use */
    actualStatus = rc;
    expectedStatus = expected;

    if (rc == expected)
    {
        test_passed++;
        printf("  %3d  PASSED", current_test);
        last_test_status = 0;
    }
    else
    {
        test_failed++;
        printf("  %3d  FAILED", current_test);
        last_test_status = -1;
    }

    test_total++;
}

/**
 * Used when rc is an exit code from an external tool e.g. immadm
 * 
 * @param rc
 * @param expected
 */
void rc_validate(int rc, int expected)
{
	char *exit_str[] = {"EXIT_SUCCESS","EXIT_FAILURE"};
	char other_str[] = "UNKNOWN";
	char *rcstr, *expstr;
	
    /* Save for later use */
    actualStatus = rc;
    expectedStatus = expected;
    last_test_status = 0;
	
    if (rc == expected)
    {
        test_passed++;
        printf("  %3d  PASSED", current_test);
    }
    else
    {
        test_failed++;
        printf("  %3d  FAILED", current_test);
		rcstr = (rc < 2) ? exit_str[rc]: other_str;
		expstr = (expected < 2) ? exit_str[expected]: other_str;
		printf("\t(expected %s, got %s (%d))",expstr, rcstr, rc);
    }

    test_total++;
}

void test_suite_add(unsigned int suite, const char *name)
{
    suite_name[suite] = name;
}

void test_case_add(unsigned int suite, void (*testfunc)(void), const char *slogan)
{
    int i;

    assert(suite < NO_SUITES);
    
    for (i = 1; i < NR_TESTS; i++)
    {
        if (testlist[suite][i].testfunc == NULL)
        {
            testlist[suite][i].testfunc = testfunc;
            testlist[suite][i].slogan = slogan;
            break;
        }
    }

    assert(i < NR_TESTS);
}

static int run_test_case(unsigned int suite, unsigned int tcase)
{
    if (testlist[suite][tcase].testfunc != NULL)
    {
        current_test = tcase;
        testlist[suite][tcase].testfunc();
        if (last_test_status == 0)
            printf("\t%s\n", testlist[suite][tcase].slogan);
        else
            printf("\t%s (expected %s, got %s)\n",
                testlist[suite][tcase].slogan, 
                get_saf_error(expectedStatus),
                get_saf_error(actualStatus));
        return 0;
    }
    else
        return -1;
}

int test_run(unsigned int suite, unsigned int tcase)
{
    int i, j;

    if (suite == ALL_SUITES)
    {
        /* run all suites */
        for (i = 0; i < NO_SUITES; i++)
        {
            if (suite_name[i] != NULL)
                printf("\nSuite %d: %s\n", i, suite_name[i]);
            for (j = 0; j < NR_TESTS; j++)
                (void) run_test_case(i, j);
        }       
    }
    else
    {
        if (suite > NO_SUITES)
        {
            fprintf(stderr, "error: invalid suite (%d)\n", suite);
            return -1;
        }

        /* run specfic suite */
        if (tcase == ALL_TESTS)
        {
            printf("\nSuite %d: %s\n", suite, suite_name[suite]);
            /* run all cases */
            for (j = 0; j < NR_TESTS; j++)
                (void) run_test_case(suite, j);
        }
        else
        {
            if (tcase > NR_TESTS)
            {
                fprintf(stderr, "error: invalid test case (%d)\n", tcase);
                return -1;
            }

            printf("\nSuite %d: %s\n", suite, suite_name[suite]);
            /* run specific test case*/
            if (run_test_case(suite, tcase) == -1)
                return -1;
        }
    }

    /* Print result */
    printf("\n=====================================================================================\n\n");
    printf("   Test Result:\n");
    printf("      Total:  %u\n", test_total);
    printf("      Passed: %u\n", test_passed);
    printf("      Failed: %u\n", test_failed);

    if (test_failed == 0)
        return 0;
    else
        return -1;
}

void test_list(void)
{
    int i, j;

    for (i = 0; i < NO_SUITES; i++)
    {
        if (suite_name[i] != NULL)
            printf("\nSuite %d: %s\n", i, suite_name[i]);
        for (j = 0; j < NR_TESTS; j++)
        {
            if (testlist[i][j].testfunc != NULL)
                printf("\t%d\t%s\n", j, testlist[i][j].slogan);
        }
    }       
}

