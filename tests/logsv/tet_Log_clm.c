/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2016 The OpenSAF Foundation
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
 * Author(s): Oracle 
 *
 */

#include "logtest.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <unistd.h>
#include <stdarg.h>

static char host_name[_POSIX_HOST_NAME_MAX] = {0};
static const char *hostname(void)
{
	gethostname(host_name, _POSIX_HOST_NAME_MAX);
	return host_name;
}

static bool is_test_done_on_pl(void)
{
	return (strncmp(hostname(), "PL-", strlen("PL-")) == 0);
}

static void cond_check(void)
{
	if (is_test_done_on_pl() == false) {
		fprintf(stderr, "Test must be done on payload node \n");
		exit(EXIT_FAILURE);
	}
}


/* Test cases for CLM Member Node.
 * NOTE1: For the moment these test cases need interaction with the tester and
 * cannot be run fully automatic.
 * NOTE2: Must run on PL node!
 */

/**
 * Pause testing and wait for tester to press Enter.
 * Can be used e.g. at a point where examining the log files is a good idea
 */
static int lockNode(void)
{
	int rc;
	char command[256];
	sprintf(command, "amf-adm lock safNode=%s,safCluster=myClmCluster", hostname());
	rc = system(command);
	return WEXITSTATUS(rc);
}

/**
 * Pause testing and wait for tester to press Enter.
 * Can be used e.g. at a point where examining the log files is a good idea
 */
static int unlockNode(void)
{
	int rc;
	char command[256];
	sprintf(command, "amf-adm unlock safNode=%s,safCluster=myClmCluster", hostname());
	rc = system(command);
	return WEXITSTATUS(rc);
}

void saLogInitializ_14_01(void)
{
	cond_check();
	lockNode();
	printf_s(" to saLogInitialize(). ");
	rc = saLogInitialize(&logHandle, &logCallbacks, &logVersion);
	saLogFinalize(logHandle);
	unlockNode();
	test_validate(rc, SA_AIS_ERR_UNAVAILABLE);
}

void saLogInitializ_14_02(void)
{
	cond_check();
	SaVersionT logPreviousVersion = { 'A', 0x02, 0x01 };
	lockNode();
	printf_s(" to saLogInitialize(). ");
	rc = saLogInitialize(&logHandle, &logCallbacks, &logPreviousVersion);
	saLogFinalize(logHandle);
	unlockNode();
	test_validate(rc, SA_AIS_OK);
}

void saLogInitializ_14_03(void)
{
	cond_check();
	lockNode();
	unlockNode();
	printf_s(" to saLogInitialize(). ");
	rc = saLogInitialize(&logHandle, &logCallbacks, &logVersion);
	saLogFinalize(logHandle);
	test_validate(rc, SA_AIS_OK);
}

void saLogInitializ_14_04(void)
{
	cond_check();
	SaVersionT logPreviousVersion = { 'A', 0x02, 0x01 };
	lockNode();
	unlockNode();
	printf_s(" to saLogInitialize(). ");
	rc = saLogInitialize(&logHandle, &logCallbacks, &logPreviousVersion);
	saLogFinalize(logHandle);
	test_validate(rc, SA_AIS_OK);
}

void saLogStreamOpen_14_05(void)
{
	cond_check();
	safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
	lockNode();
	printf_s(" to saLogStreamOpen_2(). ");
	rc = saLogStreamOpen_2(logHandle, &systemStreamName, NULL, 0,
			SA_TIME_ONE_SECOND, &logStreamHandle);
	saLogStreamClose(logStreamHandle);
	safassert(saLogFinalize(logHandle), SA_AIS_OK);
	unlockNode();
	test_validate(rc, SA_AIS_ERR_UNAVAILABLE);
}

void saLogStreamOpen_14_06(void)
{
	cond_check();
	safassert(saLogInitialize(&logHandle, &logCallbacks, &logVersion), SA_AIS_OK);
	lockNode();
	unlockNode();
	printf_s(" to saLogStreamOpen_2(). ");
	rc = saLogStreamOpen_2(logHandle, &systemStreamName, NULL, 0,
			SA_TIME_ONE_SECOND, &logStreamHandle);
	saLogStreamClose(logStreamHandle);
	safassert(saLogFinalize(logHandle), SA_AIS_OK);
	test_validate(rc, SA_AIS_ERR_UNAVAILABLE);

}

void saLogStreamOpen_14_07(void)
{
        cond_check();
	SaVersionT logPreviousVersion = { 'A', 0x02, 0x01 };
        safassert(saLogInitialize(&logHandle, &logCallbacks, &logPreviousVersion), SA_AIS_OK);
        lockNode();
        printf_s(" to saLogStreamOpen_2(). ");
        rc = saLogStreamOpen_2(logHandle, &systemStreamName, NULL, 0,
                        SA_TIME_ONE_SECOND, &logStreamHandle);
        saLogStreamClose(logStreamHandle);
        safassert(saLogFinalize(logHandle), SA_AIS_OK);
        unlockNode();
        test_validate(rc, SA_AIS_OK);
}
        
void saLogStreamOpen_14_08(void)
{       
        cond_check();
	SaVersionT logPreviousVersion = { 'A', 0x02, 0x01 };
        safassert(saLogInitialize(&logHandle, &logCallbacks, &logPreviousVersion), SA_AIS_OK);
        lockNode();
        unlockNode();
        printf_s(" to saLogStreamOpen_2(). ");
        rc = saLogStreamOpen_2(logHandle, &systemStreamName, NULL, 0,
                        SA_TIME_ONE_SECOND, &logStreamHandle);
        saLogStreamClose(logStreamHandle);
        safassert(saLogFinalize(logHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
 
}

void add_suite_14(void) 
{
	test_suite_add(14, "Log Service CLM Operations");
	test_case_add(14, saLogInitializ_14_01, "saLogInitializ() on Node Lock");
	test_case_add(14, saLogInitializ_14_02, "saLogInitializ(previous versions) on Node Lock");
	test_case_add(14, saLogInitializ_14_03, "saLogInitializ() on Node Lock & Un-Lock");
	test_case_add(14, saLogInitializ_14_04, "saLogInitializ(previous versions) on Lock & Un-Lock");
	test_case_add(14, saLogStreamOpen_14_05, "saLogStreamOpen_2() on Node Lock");
	test_case_add(14, saLogStreamOpen_14_06, "saLogStreamOpen_2() On Node Lock & Un-Lock");
	test_case_add(14, saLogStreamOpen_14_07, "saLogStreamOpen_2(previous versions) on Node Lock");
	test_case_add(14, saLogStreamOpen_14_08, "saLogStreamOpen_2(previous versions) On Node Lock & Un-Lock");
}

