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

void saImmOmClassDescriptionGet_2_01(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitionsIn[] = {&attr1, NULL};
    SaImmClassCategoryT classCategory;
    SaImmAttrDefinitionT_2** attrDefinitionsOut;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_RUNTIME, attrDefinitionsIn), SA_AIS_OK);
    rc = saImmOmClassDescriptionGet_2(immOmHandle, className, &classCategory, &attrDefinitionsOut);
    assert(classCategory == SA_IMM_CLASS_RUNTIME);
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmClassDescriptionMemoryFree_2(immOmHandle, attrDefinitionsOut), SA_AIS_OK);
    safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassDescriptionGet_2_02(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitionsIn[] = {&attr1, NULL};
    SaImmClassCategoryT classCategory;
    SaImmAttrDefinitionT_2** attrDefinitionsOut;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_RUNTIME, attrDefinitionsIn), SA_AIS_OK);
    rc = saImmOmClassDescriptionGet_2(-1, className, &classCategory, &attrDefinitionsOut);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassDescriptionGet_2_03(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmClassCategoryT classCategory;
    SaImmAttrDefinitionT_2** attrDefinitionsOut;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmClassDescriptionGet_2(immOmHandle, className, &classCategory, &attrDefinitionsOut);
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassDescriptionGet_2_04(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED, NULL};
    SaImmAttrDefinitionT_2 attr2 =
        {"notified_rta",  SA_IMM_ATTR_SAUINT32T, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_NOTIFY | 
	 SA_IMM_ATTR_CACHED, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitionsIn[] = {&attr1, &attr2, NULL};
    SaImmClassCategoryT classCategory;
    SaImmAttrDefinitionT_2** attrDefinitionsOut;
    SaImmAttrDefinitionT_2** attrDOut;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_RUNTIME, attrDefinitionsIn), SA_AIS_OK);
    safassert(saImmOmClassDescriptionGet_2(immOmHandle, className, &classCategory, &attrDefinitionsOut),
	    SA_AIS_OK);
    assert(classCategory == SA_IMM_CLASS_RUNTIME);
    int ix=0;
    rc = SA_AIS_ERR_LIBRARY;
    attrDOut = attrDefinitionsOut;
    while(attrDOut) {
	    SaImmAttrDefinitionT_2* attr = attrDOut[0];
	    if(strcmp(attr->attrName, attr2.attrName)==0) {
		    if(attr->attrFlags & SA_IMM_ATTR_NOTIFY) {
			    rc = SA_AIS_OK;
		    }
		    if(attr->attrFlags & SA_IMM_ATTR_NO_DUPLICATES) {
			    rc = SA_AIS_ERR_LIBRARY;
		    }
		    break;
	    }
	    ++ix;
	    ++attrDOut;
    }
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmClassDescriptionMemoryFree_2(immOmHandle, attrDefinitionsOut), SA_AIS_OK);
    safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassDescriptionGet_2_05(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN, NULL};
    SaImmAttrDefinitionT_2 attr2 =
        {"attr1",  SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE | SA_IMM_ATTR_NO_DANGLING, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitionsIn[] = {&attr1, &attr2, NULL};
    SaImmClassCategoryT classCategory;
    SaImmAttrDefinitionT_2** attrDefinitionsOut;
    SaImmAttrDefinitionT_2** attrDOut;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_CONFIG, attrDefinitionsIn), SA_AIS_OK);
    safassert(saImmOmClassDescriptionGet_2(immOmHandle, className, &classCategory, &attrDefinitionsOut),
	    SA_AIS_OK);
    assert(classCategory == SA_IMM_CLASS_CONFIG);
    rc = SA_AIS_ERR_LIBRARY;
    attrDOut = attrDefinitionsOut;
    while(attrDOut) {
	    SaImmAttrDefinitionT_2* attr = attrDOut[0];
	    if(strcmp(attr->attrName, attr2.attrName)==0) {
		    if(attr->attrFlags & SA_IMM_ATTR_NO_DANGLING) {
			    rc = SA_AIS_OK;
		    }
		    break;
	    }
	    ++attrDOut;
    }
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmClassDescriptionMemoryFree_2(immOmHandle, attrDefinitionsOut), SA_AIS_OK);
    safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}
