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
 * Author(s): Emerson Network Power
 *
 */

#ifndef CLM_APITEST_CLMTEST_H_
#define CLM_APITEST_CLMTEST_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <saClm.h>
#include <assert.h>
#include "osaf/apitest/utest.h"
#include "osaf/apitest/util.h"
#include <poll.h>
#include <unistd.h>

extern SaVersionT clmVersion_4;
extern SaVersionT clmVersion_1;
extern SaVersionT clmVersion;
extern SaAisErrorT rc;
extern SaClmHandleT clmHandle;
extern SaClmCallbacksT_4 clmCallbacks_4;
extern SaClmCallbacksT clmCallbacks_1;
extern SaSelectionObjectT selectionObject;
extern SaNameT node_name;


#endif  // CLM_APITEST_CLMTEST_H_
