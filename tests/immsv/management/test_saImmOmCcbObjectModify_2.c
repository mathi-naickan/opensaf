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
#include <unistd.h>

static const SaNameT rdnObj1 = {sizeof("Obj1"), "Obj1"};
static const SaNameT rdnObj2 = {sizeof("Obj2"), "Obj2"};
static SaNameT dnObj1;
static SaNameT dnObj2;
static const SaNameT *dnObjs[] = {&dnObj1, NULL};

static SaAisErrorT config_object_create(SaImmHandleT immHandle,
    SaImmAdminOwnerHandleT ownerHandle,
    const SaNameT *parentName)
{
    SaImmCcbHandleT ccbHandle;
    const SaNameT* nameValues[] = {&rdnObj1, NULL};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = __LINE__;
    SaUint32T* int1Values[] = {&int1Value1};
    SaStringT strValue1 = "String1-duplicate";
    SaStringT strValue2 = "String2";
    SaStringT* strValues[] = {&strValue2};
    SaStringT* str2Values[] = {&strValue1, &strValue2, &strValue1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    SaImmAttrValuesT_2 v3 = {"attr3", SA_IMM_ATTR_SASTRINGT, 3, (void**)str2Values};
    SaImmAttrValuesT_2 v4 = {"attr4", SA_IMM_ATTR_SASTRINGT, 1, (void**)strValues};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, &v3, &v4, NULL};

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, configClassName, parentName, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    return saImmOmCcbFinalize(ccbHandle);
}

static SaAisErrorT config_object_delete(SaImmHandleT immHandle,
    SaImmAdminOwnerHandleT ownerHandle)
{
    SaImmCcbHandleT ccbHandle;

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectDelete(ccbHandle, &dnObj1), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    return saImmOmCcbFinalize(ccbHandle);
}

void saImmOmCcbObjectModify_2_01(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};
    SaUint32T  int1Value1 = __LINE__;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    SaImmAttrModificationT_2 attrMod = {SA_IMM_ATTR_VALUES_REPLACE, v1};
    const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &rootObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, dnObjs, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    safassert(saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, NULL), 
        SA_AIS_ERR_INVALID_PARAM);
    test_validate(saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods), SA_AIS_OK);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectModify_2_02(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};
    SaUint32T  int1Value1 = __LINE__;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    SaImmAttrModificationT_2 attrMod = {SA_IMM_ATTR_VALUES_REPLACE, v1};
    const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &rootObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, dnObjs, SA_IMM_ONE), SA_AIS_OK);

    /* invalid handle */
    if ((rc = saImmOmCcbObjectModify_2(-1, &dnObj1, attrMods)) != SA_AIS_ERR_BAD_HANDLE)
        goto done;

    /* already finalized handle */
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    rc = saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods);

done:
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

/* SA_AIS_ERR_INVALID_PARAM */
void saImmOmCcbObjectModify_2_03(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};
    SaUint32T  int1Value1 = __LINE__;
    SaUint32T* int1Values[] = {&int1Value1};
    const SaNameT rdn = {sizeof("Obj2"), "Obj2"};
    const SaNameT* nameValues[] = {&rdn, NULL};
    SaImmAttrValuesT_2 v1 = {"attr2", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    SaImmAttrValuesT_2 v3 = {"attr1", SA_IMM_ATTR_SAINT32T, 1, (void**)int1Values};
    SaImmAttrValuesT_2 v4 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaImmAttrModificationT_2 attrMod1 = {SA_IMM_ATTR_VALUES_REPLACE, v1};
    const SaImmAttrModificationT_2 *attrMods1[] = {&attrMod1, NULL};
    SaImmAttrModificationT_2 attrMod3 = {SA_IMM_ATTR_VALUES_REPLACE, v3};
    const SaImmAttrModificationT_2 *attrMods3[] = {&attrMod3, NULL};
    SaImmAttrModificationT_2 attrMod4 = {SA_IMM_ATTR_VALUES_REPLACE, v4};
    const SaImmAttrModificationT_2 *attrMods4[] = {&attrMod4, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &rootObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, dnObjs, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    /* runtime attributes */
    if ((rc = saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods1)) != SA_AIS_ERR_INVALID_PARAM)
        goto done;

#if 0

    A.02.01 spec bug. Fixed in A.03.01

    /* attributes that are not defined for the specified class */
    if ((rc = saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods2)) != SA_AIS_ERR_INVALID_PARAM)
        goto done;
#endif

    /* attributes with values that do not match the defined value type for the attribute */
    if ((rc = saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods3)) != SA_AIS_ERR_INVALID_PARAM)
        goto done;

    /* a new value for the RDN attribute */
    if ((rc = saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods4)) != SA_AIS_ERR_INVALID_PARAM)
        goto done;

    /* attributes that cannot be modified */

    /* multiple values or additional values for a single-valued attribute */

done:
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

/* SA_AIS_ERR_BAD_OPERATION */
void saImmOmCcbObjectModify_2_04(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};
    SaUint32T  int1Value1 = __LINE__;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    SaImmAttrModificationT_2 attrMod = {SA_IMM_ATTR_VALUES_REPLACE, v1};
    const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_SUBTREE), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &rootObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerRelease(ownerHandle, objectNames, SA_IMM_SUBTREE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    rc = saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_SUBTREE), SA_AIS_OK);
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_BAD_OPERATION);
}

/* SA_AIS_ERR_NOT_EXIST */
void saImmOmCcbObjectModify_2_05(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};
    SaUint32T  int1Value1 = __LINE__;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    SaImmAttrValuesT_2 v2 = {"attr-not-exists", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    SaImmAttrModificationT_2 attrMod1 = {SA_IMM_ATTR_VALUES_REPLACE, v1};
    const SaImmAttrModificationT_2 *attrMods1[] = {&attrMod1, NULL};
    SaImmAttrModificationT_2 attrMod2 = {SA_IMM_ATTR_VALUES_REPLACE, v2};
    const SaImmAttrModificationT_2 *attrMods2[] = {&attrMod2, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &rootObj), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, SA_IMM_CCB_REGISTERED_OI, &ccbHandle), SA_AIS_OK);

    /*                                                              
    ** The name to which the objectName parameter points is not the name of an
    ** existing object.                                                                
    */
    if ((rc = saImmOmCcbObjectModify_2(ccbHandle, &dnObj2, attrMods1)) != SA_AIS_ERR_NOT_EXIST)
        goto done;

    /*                                                              
    ** One or more attribute names specified by the attrMods parameter are not valid
    ** for the object class.                                                                
    */
    if ((rc = saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods2)) != SA_AIS_ERR_NOT_EXIST)
        goto done;

    /*                                                              
    ** There is no registered Object Implementer for the object designated by the name
    ** to which the objectName parameter points, and the CCB has been initialized
    ** with the SA_IMM_CCB_REGISTERED_OI flag set.
    */
    if ((rc = saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods1)) != SA_AIS_ERR_NOT_EXIST)
        goto done;

done:
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
}

/* SA_AIS_ERR_BUSY */
void saImmOmCcbObjectModify_2_06(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle1;
    SaImmCcbHandleT ccbHandle2;
    const SaNameT *objectNames[] = {&rootObj, NULL};
    SaUint32T  int1Value1 = __LINE__;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    SaImmAttrModificationT_2 attrMod = {SA_IMM_ATTR_VALUES_REPLACE, v1};
    const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &rootObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, dnObjs, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle1), SA_AIS_OK);
    safassert(saImmOmCcbObjectModify_2(ccbHandle1, &dnObj1, attrMods), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle2), SA_AIS_OK);
    test_validate(saImmOmCcbObjectModify_2(ccbHandle2, &dnObj1, attrMods), SA_AIS_ERR_BUSY);

    safassert(saImmOmCcbFinalize(ccbHandle1), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle2), SA_AIS_OK);
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectModify_2_07(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};
    SaStringT strValue1 = "String1-duplicate";
    SaStringT strValue2 = "String2";
    SaStringT* strValues[] = {&strValue1, &strValue2};
    SaImmAttrValuesT_2 v1 = {"attr3", SA_IMM_ATTR_SASTRINGT, 2, (void**)strValues};
    SaImmAttrModificationT_2 attrMod = {SA_IMM_ATTR_VALUES_DELETE, v1};
    const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &rootObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, dnObjs, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    safassert(saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods), SA_AIS_OK);
    test_validate(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectModify_2_08(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};
    SaStringT strValue1 = "String1-duplicate";
    SaStringT strValue2 = "String2";
    SaStringT* strValues[] = {&strValue1, &strValue2};
    SaImmAttrValuesT_2 v1 = {"attr4", SA_IMM_ATTR_SASTRINGT, 2, (void**)strValues};
    SaImmAttrModificationT_2 attrMod = {SA_IMM_ATTR_VALUES_DELETE, v1};
    const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &rootObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, dnObjs, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    safassert(saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods), SA_AIS_OK);
    test_validate(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectModify_2_09(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};
    SaStringT strValue = "Sch\366ne";
    SaStringT* strValues[] = {&strValue};
    SaImmAttrValuesT_2 v1 = {"attr4", SA_IMM_ATTR_SASTRINGT, 1, (void**)strValues};
    SaImmAttrModificationT_2 attrMod = {SA_IMM_ATTR_VALUES_REPLACE, v1};
    const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &rootObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, dnObjs, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    test_validate(saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods), SA_AIS_ERR_INVALID_PARAM);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectModify_2_10(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};
    SaStringT strValue = "Sch\303\266ne";
    SaStringT* strValues[] = {&strValue};
    SaImmAttrValuesT_2 v1 = {"attr4", SA_IMM_ATTR_SASTRINGT, 1, (void**)strValues};
    SaImmAttrModificationT_2 attrMod = {SA_IMM_ATTR_VALUES_REPLACE, v1};
    const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &rootObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, dnObjs, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    safassert(saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods), SA_AIS_OK);
    test_validate(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectModify_2_11(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};
    SaStringT strValue = "Sch\001\002ne";
    SaStringT* strValues[] = {&strValue};
    SaImmAttrValuesT_2 v1 = {"attr4", SA_IMM_ATTR_SASTRINGT, 1, (void**)strValues};
    SaImmAttrModificationT_2 attrMod = {SA_IMM_ATTR_VALUES_REPLACE, v1};
    const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &rootObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, dnObjs, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    safassert(saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods), SA_AIS_OK);
    test_validate(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectModify_2_12(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaImmAccessorHandleT accessorHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};
    SaStringT strValue = "";
    SaStringT* strValues[] = {&strValue};
    SaImmAttrValuesT_2 v1 = {"attr4", SA_IMM_ATTR_SASTRINGT, 1, (void**)strValues};
    SaImmAttrModificationT_2 attrMod = {SA_IMM_ATTR_VALUES_REPLACE, v1};
    const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, NULL};
    const SaImmAttrNameT attributeNames[2] = { "attr4", NULL };
    SaImmAttrValuesT_2 **attributes;
    SaStringT str;

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &rootObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, dnObjs, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    safassert(saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    safassert(saImmOmAccessorGet_2(accessorHandle, &dnObj1, attributeNames, &attributes), SA_AIS_OK);

    assert(attributes);
    assert(attributes[0]);

    assert(attributes[0]->attrValuesNumber == 1);
    str = *(SaStringT *)attributes[0]->attrValues[0];

    assert(str);
    assert(strlen(str) == 0);

    safassert(saImmOmAccessorFinalize(accessorHandle), SA_AIS_OK);

    test_validate(SA_AIS_OK, SA_AIS_OK);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectModify_2_13(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaImmAccessorHandleT accessorHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};
    SaAnyT anyValue = { 0, (SaUint8T *)"" };
    SaAnyT* anyValues[] = { &anyValue, &anyValue, &anyValue };
    SaImmAttrValuesT_2 any5 = {"attr5", SA_IMM_ATTR_SAANYT, 1, (void **)anyValues};
    SaImmAttrValuesT_2 any6 = {"attr6", SA_IMM_ATTR_SAANYT, 3, (void **)anyValues};
    SaImmAttrModificationT_2 attrMod5 = {SA_IMM_ATTR_VALUES_REPLACE, any5};
    SaImmAttrModificationT_2 attrMod6 = {SA_IMM_ATTR_VALUES_REPLACE, any6};
    const SaImmAttrModificationT_2 *attrMods[] = {&attrMod5, &attrMod6, NULL};
    SaImmAttrValuesT_2 **attributes;
    int i, k, counter;

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &rootObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, dnObjs, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    safassert(saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    safassert(saImmOmAccessorGet_2(accessorHandle, &dnObj1, NULL, &attributes), SA_AIS_OK);

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
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectModify_2_14(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};
    SaAnyT anyValue = { 0, (SaUint8T *)"" };
    SaAnyT* anyValues[] = { &anyValue, &anyValue, &anyValue };
    SaImmAttrValuesT_2 any5 = {"attr5", SA_IMM_ATTR_SAANYT, 1, (void **)anyValues};
    SaImmAttrValuesT_2 any6 = {"attr6", SA_IMM_ATTR_SAANYT, 3, (void **)anyValues};
    SaImmAttrValuesT_2 any7 = {"attr7", SA_IMM_ATTR_SAANYT, 3, (void **)anyValues};
    SaImmAttrModificationT_2 attrMod5 = {SA_IMM_ATTR_VALUES_REPLACE, any5};
    SaImmAttrModificationT_2 attrMod6 = {SA_IMM_ATTR_VALUES_REPLACE, any6};
    SaImmAttrModificationT_2 attrMod7 = {SA_IMM_ATTR_VALUES_REPLACE, any7};
    const SaImmAttrModificationT_2 *attrMods[] = {&attrMod5, &attrMod6, &attrMod7, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &rootObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, dnObjs, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    safassert(saImmOmCcbObjectModify_2(ccbHandle, &dnObj1, attrMods), SA_AIS_ERR_INVALID_PARAM);

    /* If we come here, then the test is successful */
    test_validate(SA_AIS_OK, SA_AIS_OK);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectModify_2_15(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT obj1 = { strlen("id=1"), "id=1" };
    const SaNameT obj2 = { strlen("id=2"), "id=2" };
    const SaNameT* attrValues[] = { &obj1 };
    SaImmAttrValuesT_2 v1 = { "attr1", SA_IMM_ATTR_SANAMET, 1, (void**)attrValues };
    SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE, v1 };
    const SaImmAttrModificationT_2 *attrMods[] = { &attrMod, NULL };

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(nodangling_class_create(immOmHandle), SA_AIS_OK);
    safassert(object_create(immOmHandle, ownerHandle, nodanglingClassName, &obj1, NULL, NULL), SA_AIS_OK);
    safassert(object_create(immOmHandle, ownerHandle, nodanglingClassName, &obj2, NULL, NULL), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    rc = saImmOmCcbObjectModify_2(ccbHandle, &obj2, attrMods);
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

void saImmOmCcbObjectModify_2_16(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT obj1 = { strlen("id=1"), "id=1" };
    const SaNameT obj2 = { strlen("id=2"), "id=2" };
    const SaNameT obj3 = { strlen("id=3"), "id=3" };
    const SaNameT* attrValues1[] = { &obj1 };
    SaImmAttrValuesT_2 v1 = { "attr1", SA_IMM_ATTR_SANAMET, 1, (void**)attrValues1 };
    SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE, v1 };
    const SaImmAttrModificationT_2 *attrMods[] = { &attrMod, NULL };

    const SaNameT* attrValues2[] = { &obj2 };
    SaImmAttrValuesT_2 v2 = {"attr1", SA_IMM_ATTR_SANAMET, 1, (void**)attrValues2};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(nodangling_class_create(immOmHandle), SA_AIS_OK);
    safassert(object_create(immOmHandle, ownerHandle, nodanglingClassName, &obj1, NULL, NULL), SA_AIS_OK);
    safassert(object_create(immOmHandle, ownerHandle, nodanglingClassName, &obj2, NULL, NULL), SA_AIS_OK);
    safassert(object_create(immOmHandle, ownerHandle, nodanglingClassName, &obj3, NULL, &v2), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    rc = saImmOmCcbObjectModify_2(ccbHandle, &obj3, attrMods);
    if(rc == SA_AIS_OK) {
    	rc = saImmOmCcbApply(ccbHandle);
    }

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(object_delete(ownerHandle, &obj3, 1), SA_AIS_OK);
    safassert(object_delete(ownerHandle, &obj2, 1), SA_AIS_OK);
    safassert(object_delete(ownerHandle, &obj1, 1), SA_AIS_OK);
    safassert(nodangling_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_OK);
}

void saImmOmCcbObjectModify_2_17(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT obj1 = { strlen("id=1"), "id=1" };
    const SaNameT obj2 = { strlen("id=2"), "id=2" };
    const SaNameT* attrValues[] = { &obj1 };
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SANAMET, 1, (void**)attrValues};
    SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE, v1 };
    const SaImmAttrModificationT_2 *attrMods[] = { &attrMod, NULL };

    const SaNameT* rdnValues1[] = { &obj1 };
    SaImmAttrValuesT_2 rdnAttr1 = { "rdn", SA_IMM_ATTR_SANAMET, 1, (void**)rdnValues1 };
    const SaImmAttrValuesT_2 *attrValues1[] = { &rdnAttr1, NULL };

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(nodangling_class_create(immOmHandle), SA_AIS_OK);
    safassert(runtime_class_create(immOmHandle), SA_AIS_OK);
    safassert(object_create(immOmHandle, ownerHandle, nodanglingClassName, &obj2, NULL, NULL), SA_AIS_OK);

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiRtObjectCreate_2(immOiHandle, runtimeClassName, NULL, attrValues1), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    rc = saImmOmCcbObjectModify_2(ccbHandle, &obj2, attrMods);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(saImmOiRtObjectDelete(immOiHandle, &obj1), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

    safassert(object_delete(ownerHandle, &obj2, 1), SA_AIS_OK);
    safassert(runtime_class_delete(immOmHandle), SA_AIS_OK);
    safassert(nodangling_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saImmOmCcbObjectModify_2_18(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT obj1 = { strlen("id=1"), "id=1" };
    const SaNameT obj2 = { strlen("id=2"), "id=2" };
    const SaNameT* attrValues[] = { &obj1 };
    SaImmAttrValuesT_2 v1 = { "attr1", SA_IMM_ATTR_SANAMET, 1, (void**)attrValues };
    SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE, v1 };
    const SaImmAttrModificationT_2 *attrMods[] = { &attrMod, NULL };

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(nodangling_class_create(immOmHandle), SA_AIS_OK);
    safassert(object_create(immOmHandle, ownerHandle, nodanglingClassName, &obj1, NULL, NULL), SA_AIS_OK);
    safassert(object_create(immOmHandle, ownerHandle, nodanglingClassName, &obj2, NULL, NULL), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    safassert(object_delete_2(ccbHandle, &obj1, 1), SA_AIS_OK);
    rc = saImmOmCcbObjectModify_2(ccbHandle, &obj2, attrMods);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(object_delete(ownerHandle, &obj2, 1), SA_AIS_OK);
    safassert(object_delete(ownerHandle, &obj1, 1), SA_AIS_OK);
    safassert(nodangling_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_BAD_OPERATION);
}

void saImmOmCcbObjectModify_2_19(void)
{
	SaImmHandleT immOmHandle1, immOmHandle2;
    const SaImmAdminOwnerNameT adminOwnerName1 = (SaImmAdminOwnerNameT) __FUNCTION__;
    const SaImmAdminOwnerNameT adminOwnerName2 = (SaImmAdminOwnerNameT) __FILE__;
    SaImmAdminOwnerHandleT ownerHandle1;
    SaImmAdminOwnerHandleT ownerHandle2;
    SaImmCcbHandleT ccbHandle1;
    SaImmCcbHandleT ccbHandle2;
    const SaNameT obj1 = { strlen("id=1"), "id=1" };
    const SaNameT obj2 = { strlen("id=2"), "id=2" };
    const SaNameT* attrValues[] = { &obj1 };
    SaImmAttrValuesT_2 v1 = { "attr1", SA_IMM_ATTR_SANAMET, 1, (void**)attrValues };
    SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE, v1 };
    const SaImmAttrModificationT_2 *attrMods[] = { &attrMod, NULL };

    safassert(saImmOmInitialize(&immOmHandle1, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle1, adminOwnerName1, SA_TRUE, &ownerHandle1), SA_AIS_OK);
    safassert(saImmOmInitialize(&immOmHandle2, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle2, adminOwnerName2, SA_TRUE, &ownerHandle2), SA_AIS_OK);
    safassert(nodangling_class_create(immOmHandle1), SA_AIS_OK);
    safassert(object_create(immOmHandle1, ownerHandle1, nodanglingClassName, &obj1, NULL, NULL), SA_AIS_OK);
    safassert(object_create(immOmHandle2, ownerHandle2, nodanglingClassName, &obj2, NULL, NULL), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle1, 0, &ccbHandle1), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle2, 0, &ccbHandle2), SA_AIS_OK);

    safassert(object_delete_2(ccbHandle1, &obj1, 1), SA_AIS_OK);
    rc = saImmOmCcbObjectModify_2(ccbHandle2, &obj2, attrMods);

    safassert(saImmOmCcbFinalize(ccbHandle2), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle1), SA_AIS_OK);
    safassert(object_delete(ownerHandle2, &obj2, 1), SA_AIS_OK);
    safassert(object_delete(ownerHandle1, &obj1, 1), SA_AIS_OK);
    safassert(nodangling_class_delete(immOmHandle1), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle2), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle1), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle2), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle1), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_BUSY);
}

void saImmOmCcbObjectModify_2_20(void)
{
	SaImmHandleT immOmHandle1, immOmHandle2;
    const SaImmAdminOwnerNameT adminOwnerName1 = (SaImmAdminOwnerNameT) __FUNCTION__;
    const SaImmAdminOwnerNameT adminOwnerName2 = (SaImmAdminOwnerNameT) __FILE__;
    SaImmAdminOwnerHandleT ownerHandle1;
    SaImmAdminOwnerHandleT ownerHandle2;
    SaImmCcbHandleT ccbHandle1;
    SaImmCcbHandleT ccbHandle2;
    const SaNameT obj1 = { strlen("id=1"), "id=1" };
    const SaNameT obj2 = { strlen("id=2"), "id=2" };
    const SaNameT* attrValues[] = { &obj1 };
    SaImmAttrValuesT_2 v1 = { "attr1", SA_IMM_ATTR_SANAMET, 1, (void**)attrValues };
    SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE, v1 };
    const SaImmAttrModificationT_2 *attrMods[] = { &attrMod, NULL };

    safassert(saImmOmInitialize(&immOmHandle1, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle1, adminOwnerName1, SA_TRUE, &ownerHandle1), SA_AIS_OK);
    safassert(saImmOmInitialize(&immOmHandle2, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle2, adminOwnerName2, SA_TRUE, &ownerHandle2), SA_AIS_OK);
    safassert(nodangling_class_create(immOmHandle1), SA_AIS_OK);
    safassert(object_create(immOmHandle2, ownerHandle2, nodanglingClassName, &obj2, NULL, NULL), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle1, 0, &ccbHandle1), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle2, 0, &ccbHandle2), SA_AIS_OK);

    safassert(object_create_2(immOmHandle1, ccbHandle1, nodanglingClassName, &obj1, NULL, NULL), SA_AIS_OK);
    rc = saImmOmCcbObjectModify_2(ccbHandle2, &obj2, attrMods);

    safassert(saImmOmCcbFinalize(ccbHandle2), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle1), SA_AIS_OK);
    safassert(object_delete(ownerHandle2, &obj2, 1), SA_AIS_OK);
    safassert(nodangling_class_delete(immOmHandle1), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle2), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle1), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle2), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle1), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_BUSY);
}





__attribute__ ((constructor)) static void saImmOmCcbObjectModify_2_constructor(void)
{
    dnObj1.length = (SaUint16T) sprintf((char*) dnObj1.value, "%s,%s", rdnObj1.value, rootObj.value);
    dnObj2.length = (SaUint16T) sprintf((char*) dnObj2.value, "%s,%s", rdnObj2.value, rootObj.value);
}

