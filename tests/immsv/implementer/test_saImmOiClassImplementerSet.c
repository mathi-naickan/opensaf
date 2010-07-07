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

void saImmOiClassImplementerSet_01(void)
{
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    test_validate(saImmOiClassImplementerSet(immOiHandle, configClassName), SA_AIS_OK);
    safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
    safassert(config_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOiClassImplementerSet_02(void)
{
    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);

    /* invalid */
    if ((rc = saImmOiClassImplementerSet(-1, configClassName)) != SA_AIS_ERR_BAD_HANDLE)
        goto done;

    /* not associated with an implementer name.*/
    rc = saImmOiClassImplementerSet(immOiHandle, configClassName);

done:
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
    safassert(config_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOiClassImplementerSet_03(void)
{
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(runtime_class_create(immOmHandle), SA_AIS_OK);
    test_validate(saImmOiClassImplementerSet(immOiHandle, runtimeClassName), SA_AIS_ERR_BAD_OPERATION);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
    safassert(runtime_class_delete(immOmHandle), SA_AIS_OK);    
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOiClassImplementerSet_04(void)
{
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaImmClassNameT nonExistingclassName = (SaImmClassNameT) "XXX";

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    test_validate(saImmOiClassImplementerSet(immOiHandle, nonExistingclassName), SA_AIS_ERR_NOT_EXIST);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiClassImplementerSet_05(void)
{
    SaImmOiHandleT newhandle;
    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaImmOiImplementerNameT implementerName2 = (SaImmOiImplementerNameT) __FILE__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiClassImplementerSet(immOiHandle, configClassName), SA_AIS_OK);

    safassert(saImmOiInitialize_2(&newhandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(newhandle, implementerName2), SA_AIS_OK);
    test_validate(saImmOiClassImplementerSet(newhandle, configClassName), SA_AIS_ERR_EXIST);
    safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);
    safassert(saImmOiImplementerClear(newhandle), SA_AIS_OK);
    safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
    safassert(saImmOiFinalize(newhandle), SA_AIS_OK);
    safassert(config_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOiClassImplementerSet_06(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT rdn = {strlen("Obj1"), "Obj1"};
    const SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};

    /* Test interference between saImmOiObjectImplementerSet and saImmOiClassImplementerSet */
    SaImmOiHandleT newhandle;
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaImmOiImplementerNameT implementerName2 = (SaImmOiImplementerNameT) __FILE__;

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);

    /* Create test object under root */
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

    safassert(saImmOiInitialize_2(&newhandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(newhandle, implementerName2), SA_AIS_OK);
    safassert(saImmOiObjectImplementerSet(newhandle, &rdn, SA_IMM_ONE), SA_AIS_OK);

    rc = saImmOiClassImplementerSet(immOiHandle, configClassName);

    /* Cleanup */
    safassert(saImmOiObjectImplementerRelease(newhandle, &rdn, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOiImplementerClear(newhandle), SA_AIS_OK);
    safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
    safassert(saImmOiFinalize(newhandle), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(config_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_EXIST);
}
