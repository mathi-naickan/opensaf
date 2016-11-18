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

#include "imm/apitest/immtest.h"

static SaNameT dn = 
{
    .value = "Test,rdn=root",
    .length = sizeof("Test,rdn=root")
};

static char *str123="Test";
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

static SaUint32T int1Value = 0xdeadbeef;

static SaImmAttrValueT attrUpdateValues[] ={&int1Value};
static SaImmAttrModificationT_2 attrMod1 =
{
    .modType = SA_IMM_ATTR_VALUES_REPLACE,
    .modAttr.attrName = "saLogStreamFixedLogRecordSize",
    .modAttr.attrValuesNumber = 1,
    .modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T,
    .modAttr.attrValues = attrUpdateValues
};

static SaImmAttrModificationT_2* attrMods[] = {&attrMod1, NULL};

void saImmOiRtObjectUpdate_2_01(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiRtObjectCreate_2(immOiHandle, className, &rootObj, attrValues), SA_AIS_OK);
    safassert(saImmOiRtObjectUpdate_2(immOiHandle, &dn, NULL), SA_AIS_ERR_INVALID_PARAM);

    int1Value = 0xbadbabe;
    rc = saImmOiRtObjectUpdate_2(immOiHandle, &dn, (const SaImmAttrModificationT_2**) attrMods);
    test_validate(rc, SA_AIS_OK);

    safassert(saImmOiRtObjectDelete(immOiHandle, &dn), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiRtObjectUpdate_2_02(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiRtObjectCreate_2(immOiHandle, className, &rootObj, attrValues), SA_AIS_OK);

    rc = saImmOiRtObjectUpdate_2(-1, &dn, (const SaImmAttrModificationT_2**) attrMods);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);

    safassert(saImmOiRtObjectDelete(immOiHandle, &dn), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiRtObjectUpdate_2_03(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiRtObjectCreate_2(immOiHandle, className, &rootObj, attrValues), SA_AIS_OK);
    safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);

    rc = saImmOiRtObjectUpdate_2(immOiHandle, &dn, (const SaImmAttrModificationT_2**) attrMods);
    test_validate(rc, SA_AIS_ERR_BAD_OPERATION);

    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiRtObjectDelete(immOiHandle, &dn), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiRtObjectUpdate_2_04(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

    rc = saImmOiRtObjectUpdate_2(immOiHandle, &rootObj, (const SaImmAttrModificationT_2**) attrMods);
    test_validate(rc, SA_AIS_ERR_BAD_OPERATION);

    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiRtObjectUpdate_2_05(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    char * newstr="new_name";

    SaImmAttrValueT attrUpdateValues_5[] ={&newstr};
    SaImmAttrModificationT_2 attrMod_5 =
    {
        .modType = SA_IMM_ATTR_VALUES_REPLACE,
        .modAttr.attrName = "safLgStr",
        .modAttr.attrValuesNumber = 1,
        .modAttr.attrValueType = SA_IMM_ATTR_SASTRINGT,
        .modAttr.attrValues = attrUpdateValues_5
    };

    SaImmAttrModificationT_2* attrMods_5[] = {&attrMod_5, NULL};


    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiRtObjectCreate_2(immOiHandle, className, &rootObj, attrValues), SA_AIS_OK);

    rc = saImmOiRtObjectUpdate_2(immOiHandle, &dn, (const SaImmAttrModificationT_2**) attrMods_5);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

    safassert(saImmOiRtObjectDelete(immOiHandle, &dn), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

static const SaNameT rdn = {
    .value = "Obj2",
    .length = sizeof("Obj2"),
};

void saImmOiRtObjectUpdate_2_06(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    SaNameT nameValue;

    SaImmAttrValueT attrUpdateValues_6[] ={&nameValue};
    SaImmAttrModificationT_2 attrMod_6 =
    {
        .modType = SA_IMM_ATTR_VALUES_REPLACE,
        .modAttr.attrName = "rdn",
        .modAttr.attrValuesNumber = 1,
        .modAttr.attrValueType = SA_IMM_ATTR_SANAMET,
        .modAttr.attrValues = attrUpdateValues_6
    };

    nameValue.length = strlen("Bogus");
    strcpy((char *) &(nameValue.value), "Bogus");


    SaImmAttrModificationT_2* attrMods_6[] = {&attrMod_6, NULL};

    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};
    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    /* Create test object under root */
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiObjectImplementerSet(immOiHandle, &rdn, SA_IMM_ONE), SA_AIS_OK);

    rc = saImmOiRtObjectUpdate_2(immOiHandle, &rdn, (const SaImmAttrModificationT_2**) attrMods_6);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

    safassert(saImmOiObjectImplementerRelease(immOiHandle, &rdn, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOiRtObjectUpdate_2_07(void) {
    /*
     * STRONG_DEFAULT, Set value of runtime attribute to NULL
     */

    /* Create class */
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 rdn =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED | SA_IMM_ATTR_RDN, NULL};
    SaUint32T defaultVal = 100;
    SaImmAttrDefinitionT_2 attr =
        {"attr", SA_IMM_ATTR_SAUINT32T,
            SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED | SA_IMM_ATTR_STRONG_DEFAULT, &defaultVal};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&rdn, &attr, NULL};
    safassert(saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_RUNTIME, attrDefinitions), SA_AIS_OK);

    /* Create object */
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaNameT obj = { strlen("id=1"), "id=1" };
    SaImmAttrValueT nameValue = &obj;
    SaImmAttrValuesT_2 rdnValue = {"rdn",  SA_IMM_ATTR_SANAMET, 1, &nameValue};
    SaUint32T val = 200;
    SaImmAttrValueT valArray = &val;
    SaImmAttrValuesT_2 createValue= { "attr", SA_IMM_ATTR_SAUINT32T, 1, &valArray };
    const SaImmAttrValuesT_2 * attrValues[] = {&rdnValue, &createValue, NULL};
    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiRtObjectCreate_2(immOiHandle, className, NULL, attrValues), SA_AIS_OK);

    /* Set the strong default attribute to NULL */
    SaImmAttrValuesT_2 value = { "attr", SA_IMM_ATTR_SAUINT32T, 0, NULL };
    SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE, value };
    const SaImmAttrModificationT_2 *attrMods[] = { &attrMod, NULL };
    safassert(saImmOiRtObjectUpdate_2(immOiHandle, &obj, attrMods), SA_AIS_OK);

    /* Check value of the attribute */
    SaImmAccessorHandleT accessorHandle;
    const SaImmAttrNameT attName = "attr";
    SaImmAttrNameT attNames[] = {attName, NULL};
    SaImmAttrValuesT_2 ** resultAttrs;
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    safassert(saImmOmAccessorGet_2(accessorHandle, &obj, attNames, &resultAttrs), SA_AIS_OK);
    assert(resultAttrs[0] && (resultAttrs[0]->attrValueType == SA_IMM_ATTR_SAUINT32T));
    assert(resultAttrs[0]->attrValuesNumber == 1);
    test_validate(*((SaUint32T *) resultAttrs[0]->attrValues[0]), defaultVal);

    /* Delete Object */
    safassert(saImmOiRtObjectDelete(immOiHandle, &obj), SA_AIS_OK);
    safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

    /* Delete class */
    safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOiRtObjectUpdate_2_08(void) {
    /*
     * STRONG_DEFAULT, Set value of multi-valued runtime attribute to NULL
     */

    /* Create class */
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 rdn =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED | SA_IMM_ATTR_RDN, NULL};
    SaUint32T defaultVal = 100;
    SaImmAttrDefinitionT_2 attr =
        {"attr", SA_IMM_ATTR_SAUINT32T,
            SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED | SA_IMM_ATTR_STRONG_DEFAULT | SA_IMM_ATTR_MULTI_VALUE, &defaultVal};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&rdn, &attr, NULL};
    safassert(saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_RUNTIME, attrDefinitions), SA_AIS_OK);

    /* Create object */
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaNameT obj = { strlen("id=1"), "id=1" };
    SaImmAttrValueT nameValue = &obj;
    SaImmAttrValuesT_2 rdnValue = {"rdn",  SA_IMM_ATTR_SANAMET, 1, &nameValue};
    SaUint32T val1 = 200;
    SaUint32T val2 = 300;
    SaImmAttrValueT valArray[] = {&val1, &val2};
    SaImmAttrValuesT_2 createValue= { "attr", SA_IMM_ATTR_SAUINT32T, 2, valArray };
    const SaImmAttrValuesT_2 * attrValues[] = {&rdnValue, &createValue, NULL};
    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiRtObjectCreate_2(immOiHandle, className, NULL, attrValues), SA_AIS_OK);

    /* Set the strong default attribute to NULL */
    SaImmAttrValuesT_2 value = { "attr", SA_IMM_ATTR_SAUINT32T, 0, NULL };
    SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE, value };
    const SaImmAttrModificationT_2 *attrMods[] = { &attrMod, NULL };
    safassert(saImmOiRtObjectUpdate_2(immOiHandle, &obj, attrMods), SA_AIS_OK);

    /* Check value of the attribute */
    SaImmAccessorHandleT accessorHandle;
    const SaImmAttrNameT attName = "attr";
    SaImmAttrNameT attNames[] = {attName, NULL};
    SaImmAttrValuesT_2 ** resultAttrs;
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    safassert(saImmOmAccessorGet_2(accessorHandle, &obj, attNames, &resultAttrs), SA_AIS_OK);
    assert(resultAttrs[0] && (resultAttrs[0]->attrValueType == SA_IMM_ATTR_SAUINT32T));
    assert(resultAttrs[0]->attrValuesNumber == 1);
    test_validate(*((SaUint32T *) resultAttrs[0]->attrValues[0]), defaultVal);

    /* Delete Object */
    safassert(saImmOiRtObjectDelete(immOiHandle, &obj), SA_AIS_OK);
    safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

    /* Delete class */
    safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOiRtObjectUpdate_2_09(void) {
    /*
     * STRONG_DEFAULT, Delete all values of multi-valued runtime attribute
     */

    /* Create class */
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 rdn =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED | SA_IMM_ATTR_RDN, NULL};
    SaUint32T defaultVal = 100;
    SaImmAttrDefinitionT_2 attr =
        {"attr", SA_IMM_ATTR_SAUINT32T,
            SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED | SA_IMM_ATTR_STRONG_DEFAULT | SA_IMM_ATTR_MULTI_VALUE, &defaultVal};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&rdn, &attr, NULL};
    safassert(saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_RUNTIME, attrDefinitions), SA_AIS_OK);

    /* Create object */
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaNameT obj = { strlen("id=1"), "id=1" };
    SaImmAttrValueT nameValue = &obj;
    SaImmAttrValuesT_2 rdnValue = {"rdn",  SA_IMM_ATTR_SANAMET, 1, &nameValue};
    SaUint32T val1 = 200;
    SaUint32T val2 = 300;
    SaImmAttrValueT valArray[] = {&val1, &val2};
    SaImmAttrValuesT_2 createValue= { "attr", SA_IMM_ATTR_SAUINT32T, 2, valArray };
    const SaImmAttrValuesT_2 * attrValues[] = {&rdnValue, &createValue, NULL};
    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiRtObjectCreate_2(immOiHandle, className, NULL, attrValues), SA_AIS_OK);

    /* Set the strong default attribute to NULL */
    SaImmAttrValuesT_2 value = { "attr", SA_IMM_ATTR_SAUINT32T, 2, valArray };
    SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_DELETE, value };
    const SaImmAttrModificationT_2 *attrMods[] = { &attrMod, NULL };
    safassert(saImmOiRtObjectUpdate_2(immOiHandle, &obj, attrMods), SA_AIS_OK);

    /* Check value of the attribute */
    SaImmAccessorHandleT accessorHandle;
    const SaImmAttrNameT attName = "attr";
    SaImmAttrNameT attNames[] = {attName, NULL};
    SaImmAttrValuesT_2 ** resultAttrs;
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    safassert(saImmOmAccessorGet_2(accessorHandle, &obj, attNames, &resultAttrs), SA_AIS_OK);
    assert(resultAttrs[0] && (resultAttrs[0]->attrValueType == SA_IMM_ATTR_SAUINT32T));
    assert(resultAttrs[0]->attrValuesNumber == 1);
    test_validate(*((SaUint32T *) resultAttrs[0]->attrValues[0]), defaultVal);

    /* Delete Object */
    safassert(saImmOiRtObjectDelete(immOiHandle, &obj), SA_AIS_OK);
    safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

    /* Delete class */
    safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}
