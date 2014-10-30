/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2014 The OpenSAF Foundation
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

#include <sys/types.h>
#include <sys/wait.h>
#include <saImm.h>
#include <saImmOm.h>
#include <limits.h>
#include <unistd.h>
#include "logtest.h"

/*
 * Miscellaneous tests that does not fit in any of the other test suites
 */

/* 
 * Test cases
 */

void saLogMisc_01(void)
{
	int rc;
	char command[512];

	snprintf(command, 512, "logtestfr");
	rc = system(command);
	
	rc_validate(WEXITSTATUS(rc), 0);
}

/* 
 * Test suite 7
 */
__attribute__ ((constructor)) static void saOiOperations_constructor(void)
{
    test_suite_add(7, "LOG Miscellaneous tests");
    test_case_add(7, saLogMisc_01, "Log file rotation if file names truncated");
}
