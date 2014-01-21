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

static const SaNameT rdnObj1 = {sizeof("Obj1"), "Obj1"};
static const SaNameT rdnObj2 = {sizeof("Obj2"), "Obj2"};
static SaNameT dnObj1;
static SaNameT dnObj2;

static SaStringT rdnStr1="Obj1";
static const SaImmAttrValueT nameValues[] = {&rdnStr1};
static SaUint32T uint1 = 0xbadbad;
static SaUint64T ulint1 = 0xbadbad;
static SaImmAttrValueT int1Values[] = {&uint1};
static SaImmAttrValueT lint1Values[] = {&ulint1};

static SaStringT str1="DummyFileName";
static SaImmAttrValueT strValues[] = {&str1};
static SaStringT str2="DummyPathName";
static SaImmAttrValueT strValues2[] = {&str2};

static SaImmAttrValuesT_2 v1 = { "saLogStreamFixedLogRecordSize", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values };
static SaImmAttrValuesT_2 v2 = { "safLgStr",  SA_IMM_ATTR_SASTRINGT, 1, (void**)nameValues };
static SaImmAttrValuesT_2 v3 = { "saLogStreamFileName",  SA_IMM_ATTR_SASTRINGT, 1, (void**)strValues };
static SaImmAttrValuesT_2 v4 = { "saLogStreamPathName",  SA_IMM_ATTR_SASTRINGT, 1, (void**)strValues2 };
static SaImmAttrValuesT_2 v5 = { "saLogStreamMaxLogFileSize",  SA_IMM_ATTR_SAUINT64T, 1, (void**)lint1Values};
static SaImmAttrValuesT_2 v6 = { "saLogStreamLogFullAction", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values };
static SaImmAttrValuesT_2 v7 = { "saLogStreamHaProperty", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values };
static SaImmAttrValuesT_2 v8 = { "saLogStreamMaxFilesRotated", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values };
static SaImmAttrValuesT_2 v9 = { "saLogStreamLogFileFormat",  SA_IMM_ATTR_SASTRINGT, 1, (void**)strValues };
static SaImmAttrValuesT_2 v10 = { "saLogStreamSeverityFilter", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values };
static SaImmAttrValuesT_2 v11 = { "saLogStreamCreationTimestamp",  SA_IMM_ATTR_SATIMET, 1, (void**)lint1Values};

static const SaImmAttrValuesT_2* attrValues[] = {&v1, &v2, &v3, &v4, &v5, &v6, &v7, &v8, &v9, &v10, &v11, NULL};
static const SaImmClassNameT className = "SaLogStream";

void saImmOiRtObjectCreate_2_01(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiRtObjectCreate_2(immOiHandle, className, NULL, NULL), SA_AIS_ERR_INVALID_PARAM);
    /* Create under root */
    if ((rc = saImmOiRtObjectCreate_2(immOiHandle, className, NULL, attrValues)) != SA_AIS_OK)
        goto done;
    safassert(saImmOiRtObjectDelete(immOiHandle, &rdnObj1), SA_AIS_OK);

    /* Create under parent */
    if ((rc = saImmOiRtObjectCreate_2(immOiHandle, className, &rootObj, attrValues)) != SA_AIS_OK)
        goto done;

    safassert(saImmOiRtObjectDelete(immOiHandle, &dnObj1), SA_AIS_OK);

done:
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiRtObjectCreate_2_03(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

    rc = saImmOiRtObjectCreate_2(-1, className, &rootObj, attrValues);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);

    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiRtObjectCreate_2_04(void)
{
    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);

    rc = saImmOiRtObjectCreate_2(immOiHandle, className, &rootObj, attrValues);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);

    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiRtObjectCreate_2_05(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

    rc = saImmOiRtObjectCreate_2(immOiHandle, "XXX", &rootObj, attrValues);
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);

    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiRtObjectCreate_2_06(void)
{
    SaImmOiHandleT newhandle;
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    const SaImmOiImplementerNameT implementerName2 = (SaImmOiImplementerNameT) "tet_imm_implementer2";

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiRtObjectCreate_2(immOiHandle, "SaLogStream", &rootObj, attrValues), SA_AIS_OK);

    /* try create the same object again using a new handle */
    safassert(saImmOiInitialize_2(&newhandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(newhandle, implementerName2), SA_AIS_OK);

    rc = saImmOiRtObjectCreate_2(newhandle, "SaLogStream", &rootObj, attrValues);
    test_validate(rc, SA_AIS_ERR_EXIST);

    safassert(saImmOiRtObjectDelete(immOiHandle, &dnObj1), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
    safassert(saImmOiFinalize(newhandle), SA_AIS_OK);
}

void saImmOiRtObjectCreate_2_07(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    const char *long_name = 
        "123456789012345678901234567890123456789012345678901234567890123"; /* 63 chars long */


    SaNameT rdnObj27;
    rdnObj27.length = 64;
    strncpy((char *) rdnObj27.value, long_name, 64);
    SaImmAttrValueT nameValues27[] = {&long_name};

    //const SaNameT *nameValues27[] = {&rdnObj27};
    const SaImmAttrValuesT_2 v27 = { "safLgStr",  SA_IMM_ATTR_SASTRINGT, 1, (void**)nameValues27 };
    const SaImmAttrValuesT_2* attrValues27[] = {&v1, &v27, &v3, &v4, &v5, &v6, &v7, &v8, &v9, &v10, &v11, NULL};

    SaNameT tmpName;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

    /*Create first rt object. */
    safassert(saImmOiRtObjectCreate_2(immOiHandle, className, &rootObj, attrValues27), SA_AIS_OK);/*107  (63+44)*/

    tmpName.length = 107;
    strncpy((char *) tmpName.value, 
        "123456789012345678901234567890123456789012345678901234567890123,rdn=root", 107);
    /*Create second rt object. */
    safassert(saImmOiRtObjectCreate_2(immOiHandle, className, &tmpName, attrValues27), SA_AIS_OK);/* 171 (63 + 108)*/

    tmpName.length = 171;
    strncpy((char *) tmpName.value, 
        "123456789012345678901234567890123456789012345678901234567890123,"
        "123456789012345678901234567890123456789012345678901234567890123,rdn=root", 171);
    /*Create third rt object. */
    safassert(saImmOiRtObjectCreate_2(immOiHandle, className, &tmpName, attrValues27), SA_AIS_OK);/* 235  (63 + 172) */

    tmpName.length = 235;
    strncpy((char *) tmpName.value, 
        "123456789012345678901234567890123456789012345678901234567890123,"
        "123456789012345678901234567890123456789012345678901234567890123,"
        "123456789012345678901234567890123456789012345678901234567890123,rdn=root", 235);
    /*Create of fourth rt object should fail. */
    rc = saImmOiRtObjectCreate_2(immOiHandle, className, &tmpName, attrValues27);/*299!!   (63 + 236)*/

    test_validate(rc, SA_AIS_ERR_NAME_TOO_LONG);

    /*Tear down*/

    tmpName.length = 107;
    strncpy((char *) tmpName.value, 
        "123456789012345678901234567890123456789012345678901234567890123,rdn=root", 107);
    /* Delete first rt object and all its subobjects! */
    safassert(saImmOiRtObjectDelete(immOiHandle, &tmpName), SA_AIS_OK);

    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

extern void saImmOiRtObjectDelete_01(void);
extern void saImmOiRtObjectDelete_03(void);
extern void saImmOiRtObjectDelete_04(void);
extern void saImmOiRtObjectDelete_05(void);
extern void saImmOiRtObjectDelete_06(void);
extern void saImmOiRtObjectUpdate_2_01(void);
extern void saImmOiRtObjectUpdate_2_02(void);
extern void saImmOiRtObjectUpdate_2_03(void);
extern void saImmOiRtObjectUpdate_2_04(void);
extern void saImmOiRtObjectUpdate_2_05(void);
extern void saImmOiRtObjectUpdate_2_06(void);
extern void SaImmOiRtAttrUpdateCallbackT_01(void);

__attribute__ ((constructor)) static void saImmOiRtObjectCreate_2_constructor(void)
{
    dnObj1.length = (SaUint16T) sprintf((char*) dnObj1.value, "%s,%s", rdnObj1.value, rootObj.value);
    dnObj2.length = (SaUint16T) sprintf((char*) dnObj2.value, "%s,%s", rdnObj2.value, rootObj.value);

    test_suite_add(3, "Runtime Objects Management");
    test_case_add(3, saImmOiRtObjectCreate_2_01, "saImmOiRtObjectCreate_2 - SA_AIS_OK");
    test_case_add(3, saImmOiRtObjectCreate_2_03, "saImmOiRtObjectCreate_2 - SA_AIS_ERR_BAD_HANDLE - invalid handle");
    test_case_add(3, saImmOiRtObjectCreate_2_04, "saImmOiRtObjectCreate_2 - SA_AIS_ERR_BAD_HANDLE - immOiHandle not associated with implementer name");
    test_case_add(3, saImmOiRtObjectCreate_2_05, "saImmOiRtObjectCreate_2 - SA_AIS_ERR_NOT_EXIST - className non existing");
    test_case_add(3, saImmOiRtObjectCreate_2_06, "saImmOiRtObjectCreate_2 - SA_AIS_ERR_EXIST - object already created");
    test_case_add(3, saImmOiRtObjectCreate_2_07, "saImmOiRtObjectCreate_2 - SA_AIS_ERR_NAME_TOO_LONG - size of dn for new object too big");

    test_case_add(3, saImmOiRtObjectDelete_01, "saImmOiRtObjectDelete - SA_AIS_OK");
    test_case_add(3, saImmOiRtObjectDelete_03, "saImmOiRtObjectDelete - SA_AIS_ERR_BAD_HANDLE - invalid handle");
    test_case_add(3, saImmOiRtObjectDelete_04, "saImmOiRtObjectDelete - SA_AIS_ERR_BAD_HANDLE - immOiHandle not associated with implementer name");
    test_case_add(3, saImmOiRtObjectDelete_05, "saImmOiRtObjectDelete - SA_AIS_ERR_INVALID_PARAM - non existing object");
    test_case_add(3, saImmOiRtObjectDelete_06, "saImmOiRtObjectDelete - SA_AIS_ERR_BAD_OPERATION - delete configuration object");

    test_case_add(3, saImmOiRtObjectUpdate_2_01, "saImmOiRtObjectUpdate_2 - SA_AIS_OK");
    test_case_add(3, saImmOiRtObjectUpdate_2_02, "saImmOiRtObjectUpdate_2 - SA_AIS_ERR_BAD_HANDLE - invalid handle");
    test_case_add(3, saImmOiRtObjectUpdate_2_03, "saImmOiRtObjectUpdate_2 - SA_AIS_ERR_BAD_HANDLE - immOiHandle not associated with implementer name");
    test_case_add(3, saImmOiRtObjectUpdate_2_04, "saImmOiRtObjectUpdate_2 - SA_AIS_ERR_BAD_OPERATION - update object not owned by implementer");
    test_case_add(3, saImmOiRtObjectUpdate_2_05, "saImmOiRtObjectUpdate_2 - SA_AIS_ERR_INVALID_PARAM - new value for the RDN attr");
    test_case_add(3, saImmOiRtObjectUpdate_2_06, "saImmOiRtObjectUpdate_2 - SA_AIS_ERR_INVALID_PARAM - update configuration attribute");

    test_case_add(3, SaImmOiRtAttrUpdateCallbackT_01, "SaImmOiRtAttrUpdateCallbackT - SA_AIS_OK");
}

