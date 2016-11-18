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
#include <poll.h>
#include "imm/saf/saImm.h"
#include "imm/saf/saImmOm.h"
#include <limits.h>
#include <unistd.h>
#include <stdarg.h>
#include "base/osaf_utility.h"
#include "base/saf_error.h"
#include "base/osaf_poll.h"
#include "logtest.h"

/*
 * Miscellaneous tests that does not fit in any of the other test suites
 */

/* ********************************
 * Test cases automatically loaded
 */

/**
 * Log rotation when names of log files has been changed outside of log service
 * The log service shall not crash
 *
 * "Log file rotation if file names truncated"
 */
void saLogMisc_71(void)
{
	int rc;
	char command[512];

	snprintf(command, 512, "logtestfr");
	rc = system(command);
	
	rc_validate(WEXITSTATUS(rc), 0);
}

/* 
 * Test suite 7
 * Miscellaneous tests that does not fit in any of the other test suites
 */
__attribute__ ((constructor)) static void saMiscLogTest_constructor(void)
{
	test_suite_add(7, "LOG Miscellaneous tests");
	test_case_add(7, saLogMisc_71, "Log file rotation if file names truncated");
}
