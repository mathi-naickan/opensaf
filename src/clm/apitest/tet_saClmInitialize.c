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

#include "clmtest.h"


void saClmInitialize_01(void)
{
	rc = saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1);
	safassert(saClmFinalize(clmHandle), SA_AIS_OK);
	test_validate(rc, SA_AIS_OK);
}

void saClmInitialize_02(void)
{
        rc = saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmInitialize_03(void)
{
        rc = saClmInitialize(NULL, &clmCallbacks_1, &clmVersion_1);
	test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saClmInitialize_04(void)
{
        rc = saClmInitialize_4(NULL, &clmCallbacks_4, &clmVersion_4);
        test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saClmInitialize_05(void)
{
        rc = saClmInitialize(&clmHandle, NULL, &clmVersion_1);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmInitialize_06(void)
{
        rc = saClmInitialize_4(&clmHandle, NULL, &clmVersion_4);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmInitialize_07(void)
{
        rc = saClmInitialize(&clmHandle, NULL, NULL);
        test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
        rc = saClmInitialize_4(&clmHandle, NULL, NULL);
        test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saClmInitialize_08(void)
{	SaVersionT version1, version4;
        rc = saClmInitialize(&clmHandle, &clmCallbacks_1, &version1);
        test_validate(rc, SA_AIS_ERR_VERSION);
	rc = saClmInitialize_4(&clmHandle, &clmCallbacks_4, &version4);
        test_validate(rc, SA_AIS_ERR_VERSION);
}

void saClmInitialize_09(void)
{      
	SaVersionT version1 = {'C', 1, 1};
	SaVersionT version4 = {'C', 1, 1};
        rc = saClmInitialize(&clmHandle, &clmCallbacks_1, &version1);
        test_validate(rc, SA_AIS_ERR_VERSION);
        rc = saClmInitialize_4(&clmHandle, &clmCallbacks_4, &version4);
        test_validate(rc, SA_AIS_ERR_VERSION);
}

void saClmInitialize_10(void)
{       
	SaVersionT version1 = {'B', 5, 1};
	SaVersionT version4 = {'B', 5, 1};
        rc = saClmInitialize(&clmHandle, &clmCallbacks_1, &version1);
        test_validate(rc, SA_AIS_ERR_VERSION);
        rc = saClmInitialize_4(&clmHandle, &clmCallbacks_4, &version4);
        test_validate(rc, SA_AIS_ERR_VERSION);
}

void saClmInitialize_11(void)
{       clmVersion_1.minorVersion--;
        rc = saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1);
	safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
        clmVersion_4.minorVersion--;
        rc = saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4);
	safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmInitialize_12(void)
{
	SaVersionT version1 = {'B', 0, 0};
	SaVersionT version4 = {'B', 0, 0};
        rc = saClmInitialize(&clmHandle, &clmCallbacks_1, &version1);
        test_validate(rc, SA_AIS_ERR_VERSION);
        rc = saClmInitialize_4(&clmHandle, &clmCallbacks_4, &version4);
        test_validate(rc, SA_AIS_ERR_VERSION);
}

/*for code coverage*/
void saClmInitialize_13(void)
{
	SaClmHandleT clmHandle1,clmHandle2,clmHandle3;
        rc = saClmInitialize(&clmHandle1, &clmCallbacks_1, &clmVersion_1);
        rc = saClmInitialize(&clmHandle2, &clmCallbacks_1, &clmVersion_1);
        rc = saClmInitialize(&clmHandle3, &clmCallbacks_1, &clmVersion_1);
        safassert(saClmFinalize(clmHandle1), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle2), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle3), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}
#if 0
void saClmInitialize_13(void)
        rc = saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1);
        test_validate(rc, SA_AIS_ERR_LIBRARY);
        rc = saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4);
        test_validate(rc, SA_AIS_ERR_LIBRARY);
}

void saClmInitialize_14(void)
        rc = saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1);
        test_validate(rc, SA_AIS_ERR_TRY_AGAIN);
        rc = saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4);
        test_validate(rc, SA_AIS_ERR_TRY_AGAIN);
}
#endif 

__attribute__ ((constructor)) static void saNtfInitialize_constructor(void)
{
	test_suite_add(1, "Life Cykel API");
	test_case_add(1, saClmInitialize_01, "saClmInitialize with A.01.01 SA_AIS_OK");
	test_case_add(1, saClmInitialize_02, "saClmInitialize_4 with A.04.01 SA_AIS_OK");
	test_case_add(1, saClmInitialize_03, "saClmInitialize with NULL pointer to handle");
	test_case_add(1, saClmInitialize_04, "saClmInitialize_4 with NULL pointer to handle");
	test_case_add(1, saClmInitialize_05, "saClmInitialize with NULL pointer to callback");
	test_case_add(1, saClmInitialize_06, "saClmInitialize_4 with NULL pointer to callback");
	test_case_add(1, saClmInitialize_07, "saClmInitialize & saClmInitialize_4 with NULL pointer to callback & Version");
	test_case_add(1, saClmInitialize_08, "saClmInitialize & saClmInitialize_4 with uninitialized version"); 
        test_case_add(1, saClmInitialize_09, "saClmInitialize & saClmInitialize_4 with too high release"); 
        test_case_add(1, saClmInitialize_10, "saClmInitialize & saClmInitialize_4 with too high major version"); 
        test_case_add(1, saClmInitialize_11, "saClmInitialize & saClmInitialize_4 with too low minor version"); 
	test_case_add(1, saClmInitialize_12, "saClmInitialize & saClmInitialize_4 with B.0.0 version"); 
	test_case_add(1, saClmInitialize_13, "saClmInitialize with multiple instance"); 
#if 0
	/* this test case is for SA_AIS_ERR_LIBRARY, which will be passed when tipc 
	*  is running and clma_create() will fail*/
        test_case_add(1, saClmInitialize_13, "saClmInitialize & saClmInitialize_4 with"  
					      "valid param");
	/*this test case is for SA_AIS_ERR_TRY_AGAIN, which will be passed when tipc 
	* is running but server is not */
        test_case_add(1, saClmInitialize_14, "saClmInitialize & saClmInitialize_4 with"
					      "valid param");
#endif 
}
