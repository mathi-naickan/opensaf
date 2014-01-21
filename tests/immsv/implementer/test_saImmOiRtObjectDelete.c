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

static SaNameT dn = 
{
    .value = "Test,rdn=root",
    .length = sizeof("Test,rdn=root")
};

static SaNameT dn2 = 
{
    .value = "XXX,rdn=root",
    .length = sizeof("XXX,rdn=root")
};

const char *str123="Test";
static SaImmAttrValueT nameValues[] = {&str123};

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

void saImmOiRtObjectDelete_01(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiRtObjectCreate_2(immOiHandle, className, &rootObj, attrValues), SA_AIS_OK);

    rc = saImmOiRtObjectDelete(immOiHandle, &dn);
    test_validate(rc, SA_AIS_OK);

    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiRtObjectDelete_03(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiRtObjectCreate_2(immOiHandle, className, &rootObj, attrValues), SA_AIS_OK);

    rc = saImmOiRtObjectDelete(-1, &dn);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);

    safassert(saImmOiRtObjectDelete(immOiHandle, &dn), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiRtObjectDelete_04(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiRtObjectCreate_2(immOiHandle, className, &rootObj, attrValues), SA_AIS_OK);
    safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);

    rc = saImmOiRtObjectDelete(immOiHandle, &dn);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);

    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiRtObjectDelete(immOiHandle, &dn), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiRtObjectDelete_05(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

    rc = saImmOiRtObjectDelete(immOiHandle, &dn2);
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);

    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiRtObjectDelete_06(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

    rc = saImmOiRtObjectDelete(immOiHandle, &rootObj);
    test_validate(rc, SA_AIS_ERR_BAD_OPERATION);

    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

