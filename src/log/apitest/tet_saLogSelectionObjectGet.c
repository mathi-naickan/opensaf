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

void saLogSelectionObjectGet_01(void)
{
	rc = logInitialize();
	if (rc == SA_AIS_OK)
		rc = saLogSelectionObjectGet(logHandle, &selectionObject);
	test_validate(rc, SA_AIS_OK);
	logFinalize();
}

void saLogSelectionObjectGet_02(void)
{
	rc = saLogSelectionObjectGet(0, &selectionObject);
	test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

