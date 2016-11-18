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

#include "logtest.h"

void saLogWriteLog_01(void)
{
	rc = logInitialize();
	if (rc != SA_AIS_OK) {
		test_validate(rc, SA_AIS_OK);
		return;
	}
	rc = logStreamOpen(&systemStreamName);
	if (rc != SA_AIS_OK) {
		test_validate(rc, SA_AIS_OK);
		goto done;
	}
	rc = saLogWriteLog(logStreamHandle, SA_TIME_ONE_SECOND, &genLogRecord);
	test_validate(rc, SA_AIS_ERR_NOT_SUPPORTED);

done:
	logFinalize();
}

