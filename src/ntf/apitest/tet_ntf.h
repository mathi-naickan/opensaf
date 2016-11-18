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
#ifndef NTF_APITEST_TET_NTF_H_
#define NTF_APITEST_TET_NTF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <saNtf.h>
#include <assert.h>

#define tet_printf printf
extern void result(SaAisErrorT rc, SaAisErrorT expected);
extern void tet_result(int result);

#define TET_PASS	0
#define TET_FAIL	1
#define TET_UNRESOLVED	2
#define TET_NOTINUSE	3
#define TET_UNSUPPORTED	4
#define TET_UNTESTED	5
#define TET_UNINITIATED	6
#define TET_NORESULT	7


extern const SaVersionT refVersion;
extern SaVersionT ntfVersion;
extern SaAisErrorT rc;
extern SaNtfHandleT ntfHandle;
extern SaNtfCallbacksT ntfCallbacks;
extern SaSelectionObjectT selectionObject;
int verbose;
int gl_tag_mode;
int gl_prompt_mode;
#define DEFAULT_ADDITIONAL_TEXT "this is additional text info"

typedef struct  {
    SaUint16T numCorrelatedNotifications;
    SaUint16T lengthAdditionalText;
    SaUint16T numAdditionalInfo;
    SaUint16T numSpecificProblems;
    SaUint16T numMonitoredAttributes;
    SaUint16T numProposedRepairActions;
    SaInt16T variableDataSize;
} AlarmNotificationParams;


#endif  // NTF_APITEST_TET_NTF_H_
