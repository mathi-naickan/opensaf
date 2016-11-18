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

void saImmOmClassDescriptionMemoryFree_2_01(void)
{
    SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitionsIn[] = {&attr1, NULL};
    SaImmClassCategoryT classCategory;
    SaImmAttrDefinitionT_2** attrDefinitionsOut;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_RUNTIME, attrDefinitionsIn), SA_AIS_OK);
    safassert(saImmOmClassDescriptionGet_2(immOmHandle, className, &classCategory, &attrDefinitionsOut), SA_AIS_OK);
    rc = saImmOmClassDescriptionMemoryFree_2(immOmHandle, attrDefinitionsOut);
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassDescriptionMemoryFree_2_02(void)
{
    SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitionsIn[] = {&attr1, NULL};
    SaImmClassCategoryT classCategory;
    SaImmAttrDefinitionT_2** attrDefinitionsOut;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_RUNTIME, attrDefinitionsIn), SA_AIS_OK);
    safassert(saImmOmClassDescriptionGet_2(immOmHandle, className, &classCategory, &attrDefinitionsOut), SA_AIS_OK);
    rc = saImmOmClassDescriptionMemoryFree_2(-1, attrDefinitionsOut);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOmClassDescriptionMemoryFree_2(immOmHandle, attrDefinitionsOut), SA_AIS_OK);/* Avoid leaking. */
    safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

