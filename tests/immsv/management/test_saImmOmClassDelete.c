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

void saImmOmClassDelete_2_01(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_CONFIG, attrDefinitions), SA_AIS_OK);
    rc = saImmOmClassDelete(immOmHandle, className);
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassDelete_2_02(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;
    SaImmAttrDefinitionT_2 attr1 =
        {"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN, NULL};
    const SaImmAttrDefinitionT_2 *attrDefinitions[] = {&attr1, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_CONFIG, attrDefinitions), SA_AIS_OK);
    rc = saImmOmClassDelete(-1, className);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassDelete_2_03(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) __FUNCTION__;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmClassDelete(immOmHandle, className);
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmClassDelete_2_04(void)
{
    const SaImmClassNameT className = (SaImmClassNameT) ""; /* The empty classname */

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmClassDelete(immOmHandle, className);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

