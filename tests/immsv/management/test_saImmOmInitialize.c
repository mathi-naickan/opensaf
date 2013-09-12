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

#include "immtest.h"

void saImmOmInitialize_01(void)
{
    immOmHandle = 0LL;
    rc = saImmOmFinalize(immOmHandle);
    safassert(rc, SA_AIS_ERR_BAD_HANDLE);


    rc = saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion);
    test_validate(rc, SA_AIS_OK);
    if (rc == SA_AIS_OK)
        safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmInitialize_02(void)
{
    rc = saImmOmInitialize(&immOmHandle, NULL, &immVersion);
    test_validate(rc, SA_AIS_OK);
    if (rc == SA_AIS_OK)
        safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmInitialize_03(void)
{
    rc = saImmOmInitialize(0, &immOmCallbacks, &immVersion);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saImmOmInitialize_04(void)
{
    SaVersionT version = {0, 0, 0};

    rc = saImmOmInitialize(&immOmHandle, &immOmCallbacks, &version);

    //printf("000 Version out: %c %u %u\n", version.releaseCode, version.majorVersion, version.minorVersion);

    if(version.releaseCode != immVersion.releaseCode) {
        printf("\tRelease code not set correctly as out param expected %c got %c\n",
            immVersion.releaseCode, version.releaseCode);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }
    if(version.majorVersion != immVersion.majorVersion) {
        printf("\tMajor version not set correctly as out param expected %u got %u\n",
            immVersion.majorVersion, version.majorVersion);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }
    if(version.minorVersion != immVersion.minorVersion) {
        printf("\tMinor version not set correctly as out param expected %u got %u\n",
            immVersion.minorVersion, version.minorVersion);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }

    test_validate(rc, SA_AIS_ERR_VERSION);
}

void saImmOmInitialize_05(void)
{
    SaVersionT version = {'B', 1, 1};

    rc = saImmOmInitialize(&immOmHandle, &immOmCallbacks, &version);

    //printf("B11 Version out: %c %u %u\n", version.releaseCode, version.majorVersion, version.minorVersion);

    if(version.releaseCode != immVersion.releaseCode) {
        printf("\tRelease code not set correctly as out param expected %c got %c\n",
            immVersion.releaseCode, version.releaseCode);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }
    if(version.majorVersion != immVersion.majorVersion) {
        printf("\tMajor version not set correctly as out param expected %u got %u\n",
            immVersion.majorVersion, version.majorVersion);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }
    if(version.minorVersion != immVersion.minorVersion) {
        printf("\tMinor version not set correctly as out param expected %u got %u\n",
            immVersion.minorVersion, version.minorVersion);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }

    test_validate(rc, SA_AIS_ERR_VERSION);

}

void saImmOmInitialize_06(void)
{
    SaVersionT version = {'A', 2, 2};

    rc = saImmOmInitialize(&immOmHandle, &immOmCallbacks, &version);
 
    //printf("A22 Version out: %c %u %u\n", version.releaseCode, version.majorVersion, version.minorVersion);

    if(version.releaseCode != immVersion.releaseCode) {
        printf("\tRelease code not set correctly as out param expected %c got %c\n",
            immVersion.releaseCode, version.releaseCode);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }
    if(version.majorVersion != immVersion.majorVersion) {
        printf("\tMajor version not set correctly as out param expected %u got %u\n",
            immVersion.majorVersion, version.majorVersion);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }
    if(version.minorVersion != immVersion.minorVersion) {
        printf("\tMinor version not set correctly as out param expected %u got %u\n",
            immVersion.minorVersion, version.minorVersion);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }

    test_validate(rc, SA_AIS_OK);

    if (rc == SA_AIS_OK)
        safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

}

void saImmOmInitialize_07(void)
{
    SaVersionT version = {'A', 3, 0};

    rc = saImmOmInitialize(&immOmHandle, &immOmCallbacks, &version);

    //printf("A30 Version out: %c %u %u\n", version.releaseCode, version.majorVersion, version.minorVersion);

    if(version.releaseCode != immVersion.releaseCode) {
        printf("\tRelease code not set correctly as out param expected %c got %c\n",
            immVersion.releaseCode, version.releaseCode);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }
    if(version.majorVersion != immVersion.majorVersion) {
        printf("\tMajor version not set correctly as out param expected %u got %u\n",
            immVersion.majorVersion, version.majorVersion);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }
    if(version.minorVersion != immVersion.minorVersion) {
        printf("\tMinor version not set correctly as out param expected %u got %u\n",
            immVersion.minorVersion, version.minorVersion);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }

    test_validate(rc, SA_AIS_ERR_VERSION);
}

void saImmOmInitialize_08(void)
{
    SaVersionT version = {'B', 3, 99};

    rc = saImmOmInitialize(&immOmHandle, &immOmCallbacks, &version);

    //printf("B399 Version out: %c %u %u\n", version.releaseCode, version.majorVersion, version.minorVersion);

    if(version.releaseCode != immVersion.releaseCode) {
        printf("\tRelease code not set correctly as out param expected %c got %c\n",
            immVersion.releaseCode, version.releaseCode);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }
    if(version.majorVersion != immVersion.majorVersion) {
        printf("\tMajor version not set correctly as out param expected %u got %u\n",
            immVersion.majorVersion, version.majorVersion);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }
    if(version.minorVersion != immVersion.minorVersion) {
        printf("\tMinor version not set correctly as out param expected %u got %u\n",
            immVersion.minorVersion, version.minorVersion);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }

    test_validate(rc, SA_AIS_ERR_VERSION);

}

void saImmOmInitialize_09(void)
{
    SaVersionT version = {'A', 1, 1};

    rc = saImmOmInitialize(&immOmHandle, &immOmCallbacks, &version);

    //printf("A11 Version out: %c %u %u\n", version.releaseCode, version.majorVersion, version.minorVersion);

    if(version.releaseCode != immVersion.releaseCode) {
        printf("\tRelease code not set correctly as out param expected %c got %c\n",
            immVersion.releaseCode, version.releaseCode);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }
    if(version.majorVersion != immVersion.majorVersion) {
        printf("\tMajor version not set correctly as out param expected %u got %u\n",
            immVersion.majorVersion, version.majorVersion);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }
    if(version.minorVersion != immVersion.minorVersion) {
        printf("\tMinor version not set correctly as out param expected %u got %u\n",
            immVersion.minorVersion, version.minorVersion);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }

    test_validate(rc, SA_AIS_ERR_VERSION); /* Version A.1.1 is explicitly NOT supported. */

}


void saImmOmInitialize_10(void)
{
    SaVersionT version = {'A', 2, 99};

    rc = saImmOmInitialize(&immOmHandle, &immOmCallbacks, &version);

    //printf("A.2.99 Version out: %c %u %u\n", version.releaseCode, version.majorVersion, version.minorVersion);

    if(version.releaseCode != immVersion.releaseCode) {
        printf("\tRelease code not set correctly as out param expected %c got %c\n",
            immVersion.releaseCode, version.releaseCode);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }
    if(version.majorVersion != immVersion.majorVersion) {
        printf("\tMajor version not set correctly as out param expected %u got %u\n",
            immVersion.majorVersion, version.majorVersion);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }
    if(version.minorVersion != immVersion.minorVersion) {
        printf("\tMinor version not set correctly as out param expected %u got %u\n",
            immVersion.minorVersion, version.minorVersion);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }

    test_validate(rc, SA_AIS_OK);

    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmInitialize_11(void)
{
    SaVersionT version = {'A', 2, 1};

    rc = saImmOmInitialize_o2(&immOmHandle, NULL, &version);

    //printf("A.2.99 Version out: %c %u %u\n", version.releaseCode, version.majorVersion, version.minorVersion);

    if(version.releaseCode != immVersion.releaseCode) {
        printf("\tRelease code not set correctly as out param expected %c got %c\n",
            immVersion.releaseCode, version.releaseCode);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }
    if(version.majorVersion != immVersion.majorVersion) {
        printf("\tMajor version not set correctly as out param expected %u got %u\n",
            immVersion.majorVersion, version.majorVersion);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }
    if(version.minorVersion != immVersion.minorVersion) {
        printf("\tMinor version not set correctly as out param expected %u got %u\n",
            immVersion.minorVersion, version.minorVersion);
	rc = SA_AIS_ERR_FAILED_OPERATION;
    }

    test_validate(rc, SA_AIS_ERR_VERSION);
}


extern void saImmOmSelectionObjectGet_01(void);
extern void saImmOmSelectionObjectGet_02(void);
extern void saImmOmDispatch_01(void);
extern void saImmOmDispatch_02(void);
extern void saImmOmDispatch_03(void);
extern void saImmOmDispatch_04(void);
extern void saImmOmFinalize_01(void);
extern void saImmOmFinalize_02(void);

__attribute__ ((constructor)) static void saImmOmInitialize_constructor(void)
{
    test_suite_add(1, "Library Life Cycle");
    test_case_add(1, saImmOmInitialize_01, "saImmOmInitialize - SA_AIS_OK");
    test_case_add(1, saImmOmInitialize_02, "saImmOmInitialize - SA_AIS_OK - NULL pointer to callbacks");
    test_case_add(1, saImmOmInitialize_03, "saImmOmInitialize - SA_AIS_ERR_INVALID_PARAM - uninitialized handle");
    test_case_add(1, saImmOmInitialize_04, "saImmOmInitialize - SA_AIS_ERR_VERSION - uninitialized version");
    test_case_add(1, saImmOmInitialize_05, "saImmOmInitialize - SA_AIS_ERR_VERSION - too high release level");
    test_case_add(1, saImmOmInitialize_06, "saImmOmInitialize - SA_AIS_OK - minor version set to 2");
    test_case_add(1, saImmOmInitialize_07, "saImmOmInitialize - SA_AIS_ERR_VERSION - major version set to 3");
    test_case_add(1, saImmOmInitialize_08, "saImmOmInitialize - SA_AIS_ERR_VERSION - B.3.99 requested");
    test_case_add(1, saImmOmInitialize_09, "saImmOmInitialize - SA_AIS_ERR_VERSION - A.1.1 requested");
    test_case_add(1, saImmOmInitialize_10, "saImmOmInitialize - SA_AIS_OK - A.2.99 requested");
    test_case_add(1, saImmOmInitialize_11, "saImmOmInitialize - SA_AIS_ERR_VERSION - A.2.1 requested on _o2");

    test_case_add(1, saImmOmSelectionObjectGet_01, "saImmOmSelectionObjectGet - SA_AIS_ERR_INVALID_PARAM - no callback defined");
    test_case_add(1, saImmOmSelectionObjectGet_02, "saImmOmSelectionObjectGet - SA_AIS_ERR_BAD_HANDLE - invalid handle");

    test_case_add(1, saImmOmDispatch_01, "saImmOmDispatch - SA_AIS_OK SA_DISPATCH_ALL");
    test_case_add(1, saImmOmDispatch_02, "saImmOmDispatch - SA_AIS_OK SA_DISPATCH_ONE");
    test_case_add(1, saImmOmDispatch_03, "saImmOmDispatch - SA_AIS_ERR_BAD_HANDLE - invalid handle");
    test_case_add(1, saImmOmDispatch_04, "saImmOmDispatch - SA_AIS_ERR_INVALID_PARAM - invalid dispatchFlags");

    test_case_add(1, saImmOmFinalize_01, "saImmOmFinalize - SA_AIS_OK");
    test_case_add(1, saImmOmFinalize_02, "saImmOmFinalize - SA_AIS_ERR_BAD_HANDLE - invalid handle");
}

