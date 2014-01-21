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

static const SaImmClassNameT nonExistingClassName = (SaImmClassNameT) __FILE__;

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
    const SaNameT *objectNames[] = {&rootObj, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    /* Create object under root */
    if ((rc = saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues)) != SA_AIS_OK)
        goto done;

    /* Create object under parent, must be owner */
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, configClassName, &rootObj, NULL), SA_AIS_ERR_INVALID_PARAM);
    rc = saImmOmCcbObjectCreate_2(ccbHandle, configClassName, &rootObj, attrValues);

done:
    test_validate(rc, SA_AIS_OK);
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
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    /* already finalized ccbHandle */
    rc = saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_BAD_HANDLE)
        goto done;

    /* invalid ccbHandle */
    rc = saImmOmCcbObjectCreate_2(-1, configClassName, NULL, attrValues);

done:
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
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
    safassert(runtime_class_create(immOmHandle), SA_AIS_OK);

    /* the className parameter specifies a runtime object class */
    rc = saImmOmCcbObjectCreate_2(ccbHandle, runtimeClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_INVALID_PARAM)
    {
        TRACE_ENTER();
        goto done;
    }

    /* there is no valid RDN attribute specified for the new object */
    attrValues[0] = NULL;
    rc = saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_INVALID_PARAM)
    {
        TRACE_ENTER();
        goto done;
    }

    /* all of the configuration attributes required at object creation are not provided by
    the caller */
    attrValues[1] = NULL;
    rc = saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_INVALID_PARAM)
    {
        TRACE_ENTER();
        goto done;
    }
    attrValues[1] = &v1;

    /* attrValues parameter includes: non-persistent runtime attributes */
    attrValues[2] = &v2;
    rc = saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_INVALID_PARAM)
    {
        TRACE_ENTER();
        goto done;
    }

    /* attrValues parameter includes: attributes that are not defined for the specified class */
    attrValues[2] = &v3;
    rc = saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues);
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
    rc = saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_INVALID_PARAM)
    {
        TRACE_ENTER();
        goto done;
    }

    /* attrValues parameter includes: multiple values for a single-valued attribute */
    attrValues[1] = &v5;
    rc = saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues);
    if (rc != SA_AIS_ERR_INVALID_PARAM)
    {
        TRACE_ENTER();
        goto done;
    }

done:
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(runtime_class_delete(immOmHandle), SA_AIS_OK);
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

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    /* Create object under a non owned parent */
    test_validate(saImmOmCcbObjectCreate_2(ccbHandle, configClassName,
        &rootObj, attrValues), SA_AIS_ERR_BAD_OPERATION);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
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

    /*
    ** The name to which the parentName parameter points is not the name of an
    ** existing object.
    */
    rc = saImmOmCcbObjectCreate_2(ccbHandle, configClassName, &parentName, attrValues);
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

    safassert(saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues), SA_AIS_OK);
    /* Create same object again */
    test_validate(saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues), SA_AIS_ERR_EXIST);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
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

void saImmOmCcbObjectCreate_08(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaNameT rdn = {strlen("Obj1"), "Obj1"};
    SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaStringT str4Value = "";
    SaStringT *str4Values[] = {&str4Value};
    SaImmAttrValuesT_2 v4 = {"attr4", SA_IMM_ATTR_SASTRINGT, 1, (void**)str4Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v2, &v4, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    test_validate(saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues), SA_AIS_OK);

    saImmOmCcbObjectDelete(ccbHandle, &rdn);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectCreate_09(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaNameT rdn = {strlen("Obj1=Sch\366ne"), "Obj1=Sch\366ne"};
    SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    const SaImmAttrValuesT_2 * attrValues[] = {&v2, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    test_validate(saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues), SA_AIS_ERR_INVALID_PARAM);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectCreate_10(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaNameT rdn = {strlen("Obj1=Sch\303\266ne"), "Obj1=Sch\303\266ne"};
    SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    const SaImmAttrValuesT_2 * attrValues[] = {&v2, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    test_validate(saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues), SA_AIS_ERR_INVALID_PARAM);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectCreate_11(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaNameT rdn = {strlen("Obj1=\001\002"), "\001\002"};
    SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    const SaImmAttrValuesT_2 * attrValues[] = {&v2, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    test_validate(saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues), SA_AIS_ERR_INVALID_PARAM);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectCreate_12(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaNameT rdn = {strlen(""), ""};
    SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    const SaImmAttrValuesT_2 * attrValues[] = {&v2, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    test_validate(saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues), SA_AIS_ERR_INVALID_PARAM);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectCreate_13(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaImmAccessorHandleT accessorHandle;
    SaNameT rdn = {strlen("Obj1"), "Obj1"};
    SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaAnyT anyValue = { 0, (SaUint8T *)"" };
    SaAnyT* anyValues[] = { &anyValue, &anyValue, &anyValue };
    SaImmAttrValuesT_2 any5 = {"attr5", SA_IMM_ATTR_SAANYT, 1, (void **)anyValues};
    SaImmAttrValuesT_2 any6 = {"attr6", SA_IMM_ATTR_SAANYT, 3, (void **)anyValues};
    const SaImmAttrValuesT_2 * attrValues[] = {&v2, &any5, &any6, NULL};
    SaImmAttrValuesT_2 **attributes;
    int i, k, counter;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    safassert(saImmOmAccessorGet_2(accessorHandle, &rdn, NULL, &attributes), SA_AIS_OK);

    counter = 0;
    for(i=0; attributes[i]; i++) {
    	SaAnyT *any;
    	if(!strcmp(attributes[i]->attrName, "attr5")) {
    		counter++;
    		/* Test that there is one SaAnyT value */
    		assert(attributes[i]->attrValuesNumber == 1);
    		/* ... and it's not NULL */
    		assert(attributes[i]->attrValues);
    		any = (SaAnyT *)(attributes[i]->attrValues[0]);
    		/* ... and return value is empty string */
    		assert(any);
    		assert(any->bufferSize == 0);
    	} else if(!strcmp(attributes[i]->attrName, "attr6")) {
    		counter++;
    		/* Test that there are three SaAnyT values */
    		assert(attributes[i]->attrValuesNumber == 3);
    		assert(attributes[i]->attrValues);
    		/* All three values are empty strings */
    		for(k=0; k<3; k++) {
				any = (SaAnyT *)(attributes[i]->attrValues[k]);
				assert(any);
				assert(any->bufferSize == 0);
    		}
    	}
    }

    /* We have tested both parameters */
    assert(counter == 2);

    /* If we come here, then the test is successful */
    test_validate(SA_AIS_OK, SA_AIS_OK);

    safassert(saImmOmAccessorFinalize(accessorHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectCreate_14(void)
{
	/*
	 * Create an object with a NO_DANGLING attribute
	 */
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT obj = { strlen("id=1"), "id=1" };
    const SaNameT* rdnValues[] = {&obj};
    SaImmAttrValuesT_2 rdnAttr = {"rdn", SA_IMM_ATTR_SANAMET, 1, (void**)rdnValues};
    const SaImmAttrValuesT_2 *attrValues[] = {&rdnAttr, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(nodangling_class_create(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    rc = saImmOmCcbObjectCreate_2(ccbHandle, nodanglingClassName, NULL, attrValues);
    if(rc == SA_AIS_OK) {
        rc = saImmOmCcbApply(ccbHandle);
    }
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    object_delete(ownerHandle, &obj, 0);
    safassert(nodangling_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_OK);
}

void saImmOmCcbObjectCreate_15(void)
{
	/*
	 * Create an object with a NO_DANGLING reference to another object
	 */
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT obj1 = { strlen("id=1"), "id=1" };
    const SaNameT obj2 = { strlen("id=2"), "id=2" };
    const SaNameT* refValues[] = {&obj1};
    SaImmAttrValuesT_2 refAttr = {"attr1", SA_IMM_ATTR_SANAMET, 1, (void**)refValues};
    const SaNameT* rdnValues[] = {&obj2};
    SaImmAttrValuesT_2 rdnAttr = {"rdn", SA_IMM_ATTR_SANAMET, 1, (void**)rdnValues};
    const SaImmAttrValuesT_2 *attrValues[] = {&rdnAttr, &refAttr, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(nodangling_class_create(immOmHandle), SA_AIS_OK);
    safassert(object_create(immOmHandle, ownerHandle, nodanglingClassName, &obj1, NULL, NULL), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    rc = saImmOmCcbObjectCreate_2(ccbHandle, nodanglingClassName, NULL, attrValues);
    if(rc == SA_AIS_OK) {
        rc = saImmOmCcbApply(ccbHandle);
    }
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(object_delete(ownerHandle, &obj2, 1), SA_AIS_OK);
    safassert(object_delete(ownerHandle, &obj1, 1), SA_AIS_OK);
    safassert(nodangling_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_OK);
}

void saImmOmCcbObjectCreate_16(void)
{
	/*
	 * Create an object with a NO_DANGLING reference to non-existing object
	 */
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT obj1 = { strlen("id=1"), "id=1" };
    const SaNameT obj2 = { strlen("id=2"), "id=2" };
    const SaNameT* refValues[] = {&obj2};
    SaImmAttrValuesT_2 refAttr = {"attr1", SA_IMM_ATTR_SANAMET, 1, (void**)refValues};
    const SaNameT* rdnValues[] = {&obj1};
    SaImmAttrValuesT_2 rdnAttr = {"rdn", SA_IMM_ATTR_SANAMET, 1, (void**)rdnValues};
    const SaImmAttrValuesT_2 *attrValues[] = {&rdnAttr, &refAttr, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(nodangling_class_create(immOmHandle), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, nodanglingClassName, NULL, attrValues), SA_AIS_OK);
    rc = saImmOmCcbApply(ccbHandle);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(nodangling_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_FAILED_OPERATION);
}

void saImmOmCcbObjectCreate_17(void)
{
	/*
	 * Create bidirectional references in the same CCB
	 */
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT obj1 = { strlen("id=1"), "id=1" };
    const SaNameT obj2 = { strlen("id=2"), "id=2" };

    const SaNameT* refValues1[] = {&obj2};
    SaImmAttrValuesT_2 refAttr1 = {"attr1", SA_IMM_ATTR_SANAMET, 1, (void**)refValues1};
    const SaNameT* rdnValues1[] = {&obj1};
    SaImmAttrValuesT_2 rdnAttr1 = {"rdn", SA_IMM_ATTR_SANAMET, 1, (void**)rdnValues1};
    const SaImmAttrValuesT_2 *attrValues1[] = {&rdnAttr1, &refAttr1, NULL};

    const SaNameT* refValues2[] = {&obj1};
    SaImmAttrValuesT_2 refAttr2 = {"attr1", SA_IMM_ATTR_SANAMET, 1, (void**)refValues2};
    const SaNameT* rdnValues2[] = {&obj2};
    SaImmAttrValuesT_2 rdnAttr2 = {"rdn", SA_IMM_ATTR_SANAMET, 1, (void**)rdnValues2};
    const SaImmAttrValuesT_2 *attrValues2[] = {&rdnAttr2, &refAttr2, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(nodangling_class_create(immOmHandle), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    rc = saImmOmCcbObjectCreate_2(ccbHandle, nodanglingClassName, NULL, attrValues1);
    if(rc == SA_AIS_OK) {
        rc = saImmOmCcbObjectCreate_2(ccbHandle, nodanglingClassName, NULL, attrValues2);
        if(rc == SA_AIS_OK) {
            rc = saImmOmCcbApply(ccbHandle);
        }
    }
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    object_delete_2(ccbHandle, &obj2, 0);
    object_delete_2(ccbHandle, &obj1, 0);
    saImmOmCcbApply(ccbHandle);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(nodangling_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_OK);
}

void saImmOmCcbObjectCreate_18(void)
{
	/*
	 * Create object with a NO_DANGLING reference to an object that is deleted by the same CCB
	 */
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT obj1 = { strlen("id=1"), "id=1" };
    const SaNameT obj2 = { strlen("id=2"), "id=2" };

    const SaNameT* refValues[] = {&obj1};
    SaImmAttrValuesT_2 refAttr = {"attr1", SA_IMM_ATTR_SANAMET, 1, (void**)refValues};
    const SaNameT* rdnValues[] = {&obj2};
    SaImmAttrValuesT_2 rdnAttr = {"rdn", SA_IMM_ATTR_SANAMET, 1, (void**)rdnValues};
    const SaImmAttrValuesT_2 *attrValues[] = {&rdnAttr, &refAttr, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(nodangling_class_create(immOmHandle), SA_AIS_OK);
    safassert(object_create(immOmHandle, ownerHandle, nodanglingClassName, &obj1, NULL, NULL), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectDelete(ccbHandle, &obj1), SA_AIS_OK);
    rc = saImmOmCcbObjectCreate_2(ccbHandle, nodanglingClassName, NULL, attrValues);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    object_delete_2(ccbHandle, &obj2, 0);
    safassert(object_delete_2(ccbHandle, &obj1, 1), SA_AIS_OK);
    saImmOmCcbApply(ccbHandle);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(nodangling_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_BAD_OPERATION);
}

void saImmOmCcbObjectCreate_19(void)
{
    /*
     * Create object with a NO_DANGLING reference to an object that is deleted by another CCB
     */
    SaImmHandleT immOmHandle1, immOmHandle2;
    const SaImmAdminOwnerNameT adminOwnerName1 = (SaImmAdminOwnerNameT) __FUNCTION__;
    const SaImmAdminOwnerNameT adminOwnerName2 = (SaImmAdminOwnerNameT) __FILE__;
    SaImmAdminOwnerHandleT ownerHandle1, ownerHandle2;
    SaImmCcbHandleT ccbHandle1, ccbHandle2;
    const SaNameT obj1 = { strlen("id=1"), "id=1" };
    const SaNameT obj2 = { strlen("id=2"), "id=2" };

    const SaNameT* refValues[] = {&obj1};
    SaImmAttrValuesT_2 refAttr = {"attr1", SA_IMM_ATTR_SANAMET, 1, (void**)refValues};
    const SaNameT* rdnValues[] = {&obj2};
    SaImmAttrValuesT_2 rdnAttr = {"rdn", SA_IMM_ATTR_SANAMET, 1, (void**)rdnValues};
    const SaImmAttrValuesT_2 *attrValues[] = {&rdnAttr, &refAttr, NULL};

    safassert(saImmOmInitialize(&immOmHandle1, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle1, adminOwnerName1, SA_TRUE, &ownerHandle1), SA_AIS_OK);
    safassert(nodangling_class_create(immOmHandle1), SA_AIS_OK);
    safassert(object_create(immOmHandle1, ownerHandle1, nodanglingClassName, &obj1, NULL, NULL), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle1, 0, &ccbHandle1), SA_AIS_OK);
    safassert(saImmOmCcbObjectDelete(ccbHandle1, &obj1), SA_AIS_OK);

    safassert(saImmOmInitialize(&immOmHandle2, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle2, adminOwnerName2, SA_TRUE, &ownerHandle2), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle2, 0, &ccbHandle2), SA_AIS_OK);
    rc = saImmOmCcbObjectCreate_2(ccbHandle2, nodanglingClassName, NULL, attrValues);
    safassert(saImmOmCcbFinalize(ccbHandle2), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle1), SA_AIS_OK);

    safassert(saImmOmAdminOwnerFinalize(ownerHandle2), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle2), SA_AIS_OK);

    object_delete(ownerHandle1, &obj2, 0);
    safassert(object_delete(ownerHandle1, &obj1, 1), SA_AIS_OK);
    safassert(nodangling_class_delete(immOmHandle1), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle1), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle1), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_BUSY);
}

void saImmOmCcbObjectCreate_20(void)
{
    /*
     * Create object with a NO_DANGLING reference to an object that is created by another CCB
     */
    SaImmHandleT immOmHandle1, immOmHandle2;
    const SaImmAdminOwnerNameT adminOwnerName1 = (SaImmAdminOwnerNameT) __FUNCTION__;
    const SaImmAdminOwnerNameT adminOwnerName2 = (SaImmAdminOwnerNameT) __FILE__;
    SaImmAdminOwnerHandleT ownerHandle1, ownerHandle2;
    SaImmCcbHandleT ccbHandle1, ccbHandle2;
    const SaNameT obj1 = { strlen("id=1"), "id=1" };
    const SaNameT obj2 = { strlen("id=2"), "id=2" };

    const SaNameT* refValues[] = {&obj1};
    SaImmAttrValuesT_2 refAttr = {"attr1", SA_IMM_ATTR_SANAMET, 1, (void**)refValues};
    const SaNameT* rdnValues[] = {&obj2};
    SaImmAttrValuesT_2 rdnAttr = {"rdn", SA_IMM_ATTR_SANAMET, 1, (void**)rdnValues};
    const SaImmAttrValuesT_2 *attrValues[] = {&rdnAttr, &refAttr, NULL};

    safassert(saImmOmInitialize(&immOmHandle1, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle1, adminOwnerName1, SA_TRUE, &ownerHandle1), SA_AIS_OK);
    safassert(nodangling_class_create(immOmHandle1), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle1, 0, &ccbHandle1), SA_AIS_OK);
    safassert(object_create_2(immOmHandle1, ccbHandle1, nodanglingClassName, &obj1, NULL, NULL), SA_AIS_OK);

    safassert(saImmOmInitialize(&immOmHandle2, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle2, adminOwnerName2, SA_TRUE, &ownerHandle2), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle2, 0, &ccbHandle2), SA_AIS_OK);
    rc = saImmOmCcbObjectCreate_2(ccbHandle2, nodanglingClassName, NULL, attrValues);
    safassert(saImmOmCcbFinalize(ccbHandle2), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle1), SA_AIS_OK);

    safassert(saImmOmAdminOwnerFinalize(ownerHandle2), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle2), SA_AIS_OK);

    safassert(nodangling_class_delete(immOmHandle1), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle1), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle1), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_BUSY);
}

void saImmOmCcbObjectCreate_21(void)
{
	/*
	 * Create object with a NO_DANGLING reference to an object that is deleted by the same CCB
	 */
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT obj1 = { strlen("id=1"), "id=1" };
    const SaNameT obj2 = { strlen("id=2"), "id=2" };

    const SaNameT* rdnValues1[] = {&obj1};
    SaImmAttrValuesT_2 rdnAttr1 = {"rdn", SA_IMM_ATTR_SANAMET, 1, (void**)rdnValues1};
    const SaImmAttrValuesT_2 *attrValues1[] = {&rdnAttr1, NULL};

    const SaNameT* refValues2[] = {&obj1};
    SaImmAttrValuesT_2 refAttr2 = {"attr1", SA_IMM_ATTR_SANAMET, 1, (void**)refValues2};
    const SaNameT* rdnValues2[] = {&obj2};
    SaImmAttrValuesT_2 rdnAttr2 = {"rdn", SA_IMM_ATTR_SANAMET, 1, (void**)rdnValues2};
    const SaImmAttrValuesT_2 *attrValues2[] = {&rdnAttr2, &refAttr2, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(nodangling_class_create(immOmHandle), SA_AIS_OK);
    safassert(runtime_class_create(immOmHandle), SA_AIS_OK);

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiRtObjectCreate_2(immOiHandle, runtimeClassName, NULL, attrValues1), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    rc = saImmOmCcbObjectCreate_2(ccbHandle, nodanglingClassName, NULL, attrValues2);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(saImmOiRtObjectDelete(immOiHandle, &obj1), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

    safassert(runtime_class_delete(immOmHandle), SA_AIS_OK);
    safassert(nodangling_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}



