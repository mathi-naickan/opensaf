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


void saImmOmClassCreate_2_01(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_CONFIG, attrDefinitions);
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassCreate_2_02(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_RUNTIME, attrDefinitions);
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassCreate_2_03(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmClassCreate_2(-1, className, SA_IMM_CLASS_RUNTIME, attrDefinitions);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassCreate_2_04(void)
{
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmClassCreate_2(immOmHandle, "", SA_IMM_CLASS_CONFIG, attrDefinitions);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    if (rc == SA_AIS_OK)
        /* an error? */
        safassert(saImmOmClassDelete(immOmHandle, ""), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassCreate_2_05(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_CONFIG, attrDefinitions);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassCreate_2_06(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", -1, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_CONFIG, attrDefinitions);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassCreate_2_07(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    /* mismatch classCategory vs attribute type */
    rc = saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_CONFIG, attrDefinitions);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassCreate_2_08(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    /* mismatch classCategory vs attribute type */
    rc = saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_RUNTIME, attrDefinitions);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassCreate_2_09(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    /* SA_IMM_ATTR_CACHED missing */
    rc = saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_RUNTIME, attrDefinitions);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassCreate_2_10(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED, NULL};
    SaImmAttrDefinitionT_2 attr2 =
        {"config_attr", SA_IMM_ATTR_SAUINT32T, SA_IMM_ATTR_CONFIG, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, &attr2, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    /* config attributes in runtime class */
    rc = saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_RUNTIME, attrDefinitions);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassCreate_2_11(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    /* Illegal classCategory */
    rc = saImmOmClassCreate_2(immOmHandle, className, 99, attrDefinitions);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassCreate_2_12(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_RUNTIME, attrDefinitions), SA_AIS_OK);
    rc = saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_RUNTIME, attrDefinitions);
    test_validate(rc, SA_AIS_ERR_EXIST);
    safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

#define OPENSAF_IMM_OBJECT_DN "opensafImm=opensafImm,safApp=safImmService"
#define OPENSAF_IMM_ATTR_NOSTD_FLAGS "opensafImmNostdFlags"
#define OPENSAF_IMM_FLAG_SCHCH_ALLOW 0x00000001

void saImmOmClassCreate_2_13(void)
{
    SaUint32T noStdFlags = 0;
    SaImmAccessorHandleT accessorHandle;
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    const SaImmAttrNameT attName = (char *) OPENSAF_IMM_ATTR_NOSTD_FLAGS;
    SaNameT myObj;
    strcpy((char *) myObj.value, OPENSAF_IMM_OBJECT_DN);
    myObj.length = strlen((const char *) myObj.value);
    SaImmAttrNameT attNames[] = {attName, NULL};
    SaImmAttrValuesT_2 ** resultAttrs;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN, NULL};
    SaImmAttrDefinitionT_2 attr2 =
        {"attr2", SA_IMM_ATTR_SASTRINGT, SA_IMM_ATTR_CONFIG, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions2[] = {&attr1, &attr2, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    safassert(saImmOmAccessorGet_2(accessorHandle, &myObj, attNames, &resultAttrs), SA_AIS_OK);
    assert(resultAttrs[0] && (resultAttrs[0]->attrValueType == SA_IMM_ATTR_SAUINT32T));
    if(resultAttrs[0]->attrValuesNumber == 1) {
	    noStdFlags = *((SaUint32T *) resultAttrs[0]->attrValues[0]);
    }

    safassert(saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_CONFIG, attrDefinitions), SA_AIS_OK);
    rc = saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_CONFIG, attrDefinitions2);
    if(noStdFlags & OPENSAF_IMM_FLAG_SCHCH_ALLOW) {
	    test_validate(rc, SA_AIS_OK);
    } else {
	    if(rc != SA_AIS_OK) {
		    test_validate(rc, SA_AIS_ERR_EXIST);
	    } else {
		    test_validate(rc, SA_AIS_OK);
	    }
    }
    safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassCreate_2_14(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED, NULL};
    SaImmAttrDefinitionT_2 attr2 =
        {"notified_rta",  SA_IMM_ATTR_SAUINT32T, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_NOTIFY, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, &attr2, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    /* config attributes in runtime class */
    rc = saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_RUNTIME, attrDefinitions);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    //safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassCreate_2_15(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED, NULL};
    SaImmAttrDefinitionT_2 attr2 =
        {"nodupnonmulti_rta",  SA_IMM_ATTR_SAUINT32T, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_NO_DUPLICATES, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, &attr2, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    /* config attributes in runtime class */
    rc = saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_RUNTIME, attrDefinitions);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    //safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassCreate_2_16(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN, NULL};
    SaImmAttrDefinitionT_2 attr2 =
        {"attr1",  SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE | SA_IMM_ATTR_NO_DANGLING, NULL};
    SaImmAttrDefinitionT_2 attr3 =
        {"attr2",  SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE | SA_IMM_ATTR_MULTI_VALUE | SA_IMM_ATTR_NO_DANGLING, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, &attr2, &attr3, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_CONFIG, attrDefinitions);
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassCreate_2_17(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN, NULL};
    SaImmAttrDefinitionT_2 attr2 =
        {"attr3",  SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_PERSISTENT | SA_IMM_ATTR_NO_DANGLING, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, &attr2, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_CONFIG, attrDefinitions);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}


extern void saImmOmClassDescriptionGet_2_01(void);
extern void saImmOmClassDescriptionGet_2_02(void);
extern void saImmOmClassDescriptionGet_2_03(void);
extern void saImmOmClassDescriptionGet_2_04(void);
extern void saImmOmClassDescriptionGet_2_05(void);
extern void saImmOmClassDescriptionMemoryFree_2_01(void);
extern void saImmOmClassDescriptionMemoryFree_2_02(void);

extern void saImmOmClassDelete_2_01(void);
extern void saImmOmClassDelete_2_02(void);
extern void saImmOmClassDelete_2_03(void);
extern void saImmOmClassDelete_2_04(void);

__attribute__ ((constructor)) static void saImmOmInitialize_constructor(void)
{
    test_suite_add(2, "Object Class Management");
    test_case_add(2, saImmOmClassCreate_2_01, "saImmOmClassCreate_2 - SA_AIS_OK - CONFIG");
    test_case_add(2, saImmOmClassCreate_2_02, "saImmOmClassCreate_2 - SA_AIS_OK - RUNTIME");
    test_case_add(2, saImmOmClassCreate_2_03, "saImmOmClassCreate_2 - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(2, saImmOmClassCreate_2_04, "saImmOmClassCreate_2 - SA_AIS_ERR_INVALID_PARAM, className==\"\"");
    test_case_add(2, saImmOmClassCreate_2_05, "saImmOmClassCreate_2 - SA_AIS_ERR_INVALID_PARAM, zero length attr name");
    test_case_add(2, saImmOmClassCreate_2_06, "saImmOmClassCreate_2 - SA_AIS_ERR_INVALID_PARAM, invalid value type");
    test_case_add(2, saImmOmClassCreate_2_07,
        "saImmOmClassCreate_2 - SA_AIS_ERR_INVALID_PARAM, inconsistent attr flags, mismatch classCategory vs attribute type");
    test_case_add(2, saImmOmClassCreate_2_08,
        "saImmOmClassCreate_2 - SA_AIS_ERR_INVALID_PARAM, inconsistent attr flags, mismatch classCategory vs attribute type");
    test_case_add(2, saImmOmClassCreate_2_09,
        "saImmOmClassCreate_2 - SA_AIS_ERR_INVALID_PARAM, inconsistent attr flags, SA_IMM_ATTR_CACHED missing for runtime class");
    test_case_add(2, saImmOmClassCreate_2_10,
        "saImmOmClassCreate_2 - SA_AIS_ERR_INVALID_PARAM, inconsistent attr flags, config attributes in runtime class");
    test_case_add(2, saImmOmClassCreate_2_11, "saImmOmClassCreate_2 - SA_AIS_ERR_INVALID_PARAM, Invalid classCategory");
    test_case_add(2, saImmOmClassCreate_2_12, "saImmOmClassCreate_2 - SA_AIS_ERR_EXIST, className already exist");
    test_case_add(2, saImmOmClassCreate_2_14, "saImmOmClassCreate_2 - SA_AIS_ERR_INVALID_PARAM, flag SA_IMM_ATTR_NOTIFY not allowed on pure RTAs");
    test_case_add(2, saImmOmClassCreate_2_15, "saImmOmClassCreate_2 - SA_AIS_ERR_INVALID_PARAM, flag SA_IMM_ATTR_NO_DUPLICATES only allowed on multivalued");
    test_case_add(2, saImmOmClassCreate_2_16, "saImmOmClassCreate_2 - SA_AIS_OK, flag SA_IMM_ATTR_NO_DANGLING");
    test_case_add(2, saImmOmClassCreate_2_17, "saImmOmClassCreate_2 - SA_AIS_ERR_INVALID_PARAM, flag SA_IMM_ATTR_NO_DANGLING for PRTA");

    test_case_add(2, saImmOmClassDescriptionGet_2_01, "saImmOmClassDescriptionGet_2 - SA_AIS_OK");
    test_case_add(2, saImmOmClassDescriptionGet_2_02, "saImmOmClassDescriptionGet_2 - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(2, saImmOmClassDescriptionGet_2_03, "saImmOmClassDescriptionGet_2 - SA_AIS_ERR_NOT_EXIST, className does not exist");
    test_case_add(2, saImmOmClassDescriptionGet_2_04, "saImmOmClassDescriptionGet_2 - SA_AIS_OK, Fetch includes SA_IMM_ATTR_NOTIFY");
    test_case_add(2, saImmOmClassDescriptionGet_2_05, "saImmOmClassDescriptionGet_2 - SA_AIS_OK, Fetch includes SA_IMM_ATTR_NO_DANGLING");

    test_case_add(2, saImmOmClassDescriptionMemoryFree_2_01, "saImmOmClassDescriptionMemoryFree_2 - SA_AIS_OK");
    test_case_add(2, saImmOmClassDescriptionMemoryFree_2_02, "saImmOmClassDescriptionMemoryFree_2 - SA_AIS_ERR_BAD_HANDLE");

    test_case_add(2, saImmOmClassDelete_2_01, "saImmOmClassDelete_2 - SA_AIS_OK");
    test_case_add(2, saImmOmClassDelete_2_02, "saImmOmClassDelete_2 - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(2, saImmOmClassDelete_2_03, "saImmOmClassDelete_2 - SA_AIS_ERR_NOT_EXIST, className does not exist");
    test_case_add(2, saImmOmClassCreate_2_13, "saImmOmClassCreate_2 UPGRADE - SA_AIS_OK/SA_AIS_ERR_EXIST, Added attribute to class");
    test_case_add(2, saImmOmClassDelete_2_04, "saImmOmClassDelete_2  - SA_AIS_ERR_INVALID_PARAM, Empty classname");
}

