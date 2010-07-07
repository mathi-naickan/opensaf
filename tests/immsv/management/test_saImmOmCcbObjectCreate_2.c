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

static const SaImmClassNameT testConfigClassName = "TestClassConfig";
static const SaImmClassNameT testRuntimeClassName = "TestClassRuntime";
static const SaImmClassNameT nonExistingClassName = (SaImmClassNameT) __FILE__;

static SaAisErrorT config_class_create(SaImmHandleT immHandle)
{
    SaImmAttrDefinitionT_2 rdn = {
        "rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN,
        NULL
    };

    SaImmAttrDefinitionT_2 attr1 = {
        "attr1", SA_IMM_ATTR_SAUINT32T, SA_IMM_ATTR_CONFIG, NULL};

        SaImmAttrDefinitionT_2 attr2 = {
        "attr2", SA_IMM_ATTR_SAUINT32T, SA_IMM_ATTR_RUNTIME, NULL};

    const SaImmAttrDefinitionT_2* attributes[] = {&rdn, &attr1, &attr2, NULL};

    return  saImmOmClassCreate_2(immHandle, testConfigClassName, SA_IMM_CLASS_CONFIG,
        attributes);
}

static SaAisErrorT runtime_class_create(SaImmHandleT immHandle)
{
    SaImmAttrDefinitionT_2 rdn = {
        "rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED,
        NULL
    };

    SaImmAttrDefinitionT_2 attr1 = {
        "attr1", SA_IMM_ATTR_SAUINT32T, SA_IMM_ATTR_RUNTIME, NULL};

    const SaImmAttrDefinitionT_2* attributes[] = {&rdn, &attr1, NULL};

    return  saImmOmClassCreate_2(immHandle, testRuntimeClassName, SA_IMM_CLASS_RUNTIME,
        attributes);
}

void saImmOmCcbObjectCreate_01(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaNameT rdn = {strlen("Obj1"), "Obj1"};
    SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};
    const SaNameT parentName = {strlen("opensafImm=opensafImm,safApp=safImmService"), "opensafImm=opensafImm,safApp=safImmService"};
    const SaNameT *objectNames[] = {&parentName, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);

    /* Create object under root */
    if ((rc = saImmOmCcbObjectCreate_2(ccbHandle, testConfigClassName, NULL, attrValues)) != SA_AIS_OK)
        goto done;

    /* Create object under parent, must be owner */
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, testConfigClassName, &parentName, NULL), SA_AIS_ERR_INVALID_PARAM);
    rc = saImmOmCcbObjectCreate_2(ccbHandle, testConfigClassName, &parentName, attrValues);

done:
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmClassDelete(immOmHandle, testConfigClassName), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectCreate_01_bad(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaNameT rdn = {strlen("Obj1"), "Obj1"};
    SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);

    test_validate(saImmOmCcbObjectCreate_2(ccbHandle, testConfigClassName, NULL, attrValues), SA_AIS_OK);

    /* Deleting a non existing class */
    safassert(saImmOmClassDelete(immOmHandle, nonExistingClassName), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectCreate_02(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaNameT rdn = {strlen("Obj1"), "Obj1"};
    SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    /* already finalized ccbHandle */
    rc = saImmOmCcbObjectCreate_2(ccbHandle, testConfigClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_BAD_HANDLE)
        goto done;

    /* invalid ccbHandle */
    rc = saImmOmCcbObjectCreate_2(-1, testConfigClassName, NULL, attrValues);

done:
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOmClassDelete(immOmHandle, testConfigClassName), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectCreate_03(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaNameT name = {strlen("Obj1"), "Obj1"};
    SaNameT* nameValues[] = {&name};
    SaImmAttrValuesT_2 rdn = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaUint32T* int2Values[] = {&int1Value1, &int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    SaImmAttrValuesT_2 v2 = {"attr2", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    SaImmAttrValuesT_2 v3 = {"attr3", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    SaImmAttrValuesT_2 v4 = {"attr1", SA_IMM_ATTR_SAINT32T, 1, (void**)int1Values};
    SaImmAttrValuesT_2 v5 = {"attr1", SA_IMM_ATTR_SAINT32T, 2, (void**)int2Values};
    const SaImmAttrValuesT_2 * attrValues[4] = {&rdn, &v1, NULL, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    safassert(runtime_class_create(immOmHandle), SA_AIS_OK);

    /* the className parameter specifies a runtime object class */
    rc = saImmOmCcbObjectCreate_2(ccbHandle, testRuntimeClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_INVALID_PARAM)
    {
        TRACE_ENTER();
        goto done;
    }

    /* there is no valid RDN attribute specified for the new object */
    attrValues[0] = NULL;
    rc = saImmOmCcbObjectCreate_2(ccbHandle, testConfigClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_INVALID_PARAM)
    {
        TRACE_ENTER();
        goto done;
    }

    /* all of the configuration attributes required at object creation are not provided by
    the caller */
    attrValues[1] = NULL;
    rc = saImmOmCcbObjectCreate_2(ccbHandle, testConfigClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_INVALID_PARAM)
    {
        TRACE_ENTER();
        goto done;
    }
    attrValues[1] = &v1;

    /* attrValues parameter includes: non-persistent runtime attributes */
    attrValues[2] = &v2;
    rc = saImmOmCcbObjectCreate_2(ccbHandle, testConfigClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_INVALID_PARAM)
    {
        TRACE_ENTER();
        goto done;
    }

    /* attrValues parameter includes: attributes that are not defined for the specified class */
    attrValues[2] = &v3;
    rc = saImmOmCcbObjectCreate_2(ccbHandle, testConfigClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_INVALID_PARAM)
    {
        TRACE_ENTER();
        goto done;
    }
    attrValues[2] = NULL;

    /* attrValues parameter includes: attributes with values that do not match
    ** the defined value type for the attribute
    */
    attrValues[1] = &v4;
    rc = saImmOmCcbObjectCreate_2(ccbHandle, testConfigClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_INVALID_PARAM)
    {
        TRACE_ENTER();
        goto done;
    }

    /* attrValues parameter includes: multiple values for a single-valued attribute */
    attrValues[1] = &v5;
    rc = saImmOmCcbObjectCreate_2(ccbHandle, testConfigClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_INVALID_PARAM)
    {
        TRACE_ENTER();
        goto done;
    }

done:
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmClassDelete(immOmHandle, testRuntimeClassName), SA_AIS_OK);
    safassert(saImmOmClassDelete(immOmHandle, testConfigClassName), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectCreate_04(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaNameT rdn = {strlen(__FUNCTION__), "__FUNCTION__"};
    SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};
    const SaNameT parentName = {strlen("opensafImm=opensafImm,safApp=safImmService"), "opensafImm=opensafImm,safApp=safImmService"};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);

    /* Create object under a non owned parent */
    test_validate(saImmOmCcbObjectCreate_2(ccbHandle, testConfigClassName,
        &parentName, attrValues), SA_AIS_ERR_BAD_OPERATION);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmClassDelete(immOmHandle, testConfigClassName), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectCreate_05(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaNameT rdn = {strlen(__FUNCTION__), "__FUNCTION__"};
    SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    SaImmAttrValuesT_2 v3 = {"attr3", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[4] = {&v1, &v2, NULL, NULL};
    const SaNameT parentName = {strlen("nonExisting"), "nonExisting"};
    const SaImmClassNameT nonExistingClassName = "nonExistingClass";

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);

    /*
    ** The name to which the parentName parameter points is not the name of an
    ** existing object.
    */
    rc = saImmOmCcbObjectCreate_2(ccbHandle, testConfigClassName, &parentName, attrValues);
    if (rc != SA_AIS_ERR_NOT_EXIST)
        goto done;

    /*                                                              
    ** The className parameter is not the name of an existing object class.
    */
    rc = saImmOmCcbObjectCreate_2(ccbHandle, nonExistingClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_NOT_EXIST)
        goto done;

    /*                                                              
    ** One or more of the attributes specified by attrValues are not valid attribute
    ** names for className
    */
    attrValues[2] = &v3;
    rc = saImmOmCcbObjectCreate_2(ccbHandle, nonExistingClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_NOT_EXIST)
        goto done;

done:
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmClassDelete(immOmHandle, testConfigClassName), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectCreate_06(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaNameT rdn = {strlen(__FUNCTION__), "__FUNCTION__"};
    SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);

    safassert(saImmOmCcbObjectCreate_2(ccbHandle, testConfigClassName, NULL, attrValues), SA_AIS_OK);
    /* Create same object again */
    test_validate(saImmOmCcbObjectCreate_2(ccbHandle, testConfigClassName, NULL, attrValues), SA_AIS_ERR_EXIST);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmClassDelete(immOmHandle, testConfigClassName), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectCreate_07(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, NULL};

    const char *long_name = "safLgStrCfg=saLogAlarm\\,safApp=safLogService\\,safApp=safImmService"; 
    const char *long_name2 = "safLgStrCfg=saLogAlarm\\#safApp=safLogService\\,safApp=safImmService"; 
    static const SaNameT parentName= 
        {sizeof("safApp=safImmService"), "safApp=safImmService"};
    static const SaNameT *objs[] = {&parentName, NULL};

    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;

    SaNameT rdn;
    rdn.length = strlen(long_name) + 1;
    strncpy((char *) rdn.value, long_name, rdn.length);

    SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v1 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_CONFIG, attrDefinitions), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objs, SA_IMM_ONE), SA_AIS_OK);
    /* Create association object */
    test_validate(saImmOmCcbObjectCreate_2(ccbHandle, className, &parentName, attrValues), SA_AIS_OK);

    rdn.length = strlen(long_name2) + 1;
    strncpy((char *) rdn.value, long_name2, rdn.length);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "OpensafImm", &parentName, attrValues), 
        SA_AIS_ERR_INVALID_PARAM);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK); /* aborts ccb */
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}
