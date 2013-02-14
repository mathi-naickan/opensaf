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

static SaImmAccessorHandleT accessorHandle;
static SaImmAttrValuesT_2** attributes;
static SaNameT nonExistingObjectName = {
    .value = "nonExistingObjectName",
    .length = sizeof("nonExistingObjectName"),
};
static const SaNameT objectName = {
    .value = "opensafImm=opensafImm,safApp=safImmService",
    .length = sizeof("opensafImm=opensafImm,safApp=safImmService"),
};
static const SaNameT badObjectName = {
    .value = "opensafImm=badOpensafImm,safApp=safImmService",
    .length = sizeof("opensafImm=badOpensafImm,safApp=safImmService"),
};


static unsigned int print_SaImmAttrValuesT_2(SaImmAttrValuesT_2 **attributes)
{
    int i = 0, j;

    TRACE("\n");

    while (attributes[i])
    {
        TRACE("  %s\n", attributes[i]->attrName);
        for (j = 0; j < attributes[i]->attrValuesNumber; j++)
        {
            switch (attributes[i]->attrValueType)
            {
                case SA_IMM_ATTR_SAINT32T:
                    TRACE("    %d\n", *((SaInt32T*)attributes[i]->attrValues[j]));
                    break;
                case SA_IMM_ATTR_SAUINT32T:
                    TRACE("    %u\n", *((SaInt32T*)attributes[i]->attrValues[j]));
                    break;
                case SA_IMM_ATTR_SAINT64T:
                    TRACE("    %lld\n", *((SaInt64T*)attributes[i]->attrValues[j]));
                    break;
                case SA_IMM_ATTR_SAUINT64T:
                    TRACE("    %llu\n", *((SaUint64T*)attributes[i]->attrValues[j]));
                    break;
                case SA_IMM_ATTR_SATIMET:
                    TRACE("    %lld\n", *((SaTimeT*)attributes[i]->attrValues[j]));
                    break;
                case SA_IMM_ATTR_SANAMET:
                    TRACE("    %s (%u)\n",
                        ((SaNameT*)attributes[i]->attrValues[j])->value,
                        ((SaNameT*)attributes[i]->attrValues[j])->length);
                    break;
                case SA_IMM_ATTR_SAFLOATT:
                    TRACE("    %f\n", *((SaFloatT*)attributes[i]->attrValues[j]));
                    break;
                case SA_IMM_ATTR_SADOUBLET:
                    TRACE("    %ff\n", *((SaDoubleT*)attributes[i]->attrValues[j]));
                    break;
                case SA_IMM_ATTR_SASTRINGT:
                    TRACE("    %s\n", *((SaStringT*)attributes[i]->attrValues[j]));
                    break;
                case SA_IMM_ATTR_SAANYT:
                    TRACE("    %llu, %p\n",
                        ((SaAnyT*)attributes[i]->attrValues[j])->bufferSize,
                        ((SaAnyT*)attributes[i]->attrValues[j])->bufferAddr);
                    break;
                default:
                    break;
            }
        }
        i++;
    }

    TRACE("\n");

    return i;
}

void saImmOmAccessorGet_2_01(void)
{
    int cnt;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    rc = saImmOmAccessorGet_2(accessorHandle, &objectName, NULL, &attributes);
    safassert(rc, SA_AIS_OK);
    cnt = print_SaImmAttrValuesT_2(attributes);
    test_validate(rc, SA_AIS_OK);
    assert(cnt > 7);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAccessorGet_2_02(void)
{
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    rc = saImmOmAccessorGet_2(-1, &objectName, NULL, &attributes);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAccessorGet_2_03(void)
{
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    rc = saImmOmAccessorGet_2(accessorHandle, &nonExistingObjectName, NULL, &attributes);
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAccessorGet_2_04(void)
{
    int cnt=0;
    SaImmAttrNameT accessorGetConfigAttrsToken[2] = {"SA_IMM_SEARCH_GET_CONFIG_ATTR", NULL };
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    rc = saImmOmAccessorGet_2(accessorHandle, &objectName, accessorGetConfigAttrsToken, &attributes);
    /* Verify the number of config attributes */
    cnt = print_SaImmAttrValuesT_2(attributes);
    assert(cnt == 5);
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAccessorGet_2_05(void)
{
    SaImmAttrNameT attributeNames[] = { NULL };

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    rc = saImmOmAccessorGet_2(accessorHandle, &objectName, attributeNames, NULL);
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAccessorGet_2_06(void)
{
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    rc = saImmOmAccessorGet_2(accessorHandle, &objectName, NULL, NULL);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAccessorGet_2_07(void)
{
    SaImmAttrNameT attributeNames[] = { "SaImmAttrClassName", NULL };

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    rc = saImmOmAccessorGet_2(accessorHandle, &objectName, attributeNames, NULL);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAccessorGet_2_08(void)
{
    SaImmAttrNameT attributeNames[] = { NULL };

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    rc = saImmOmAccessorGet_2(accessorHandle, &badObjectName, attributeNames, NULL);
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAccessorGet_2_09(void)
{
    SaNameT objectName1 = {
        .value = "",
        .length = 1,
    };


    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    rc = saImmOmAccessorGet_2(accessorHandle, &objectName1, NULL, &attributes);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAccessorGet_2_10(void)
{
    int count;
    rc = SA_AIS_OK;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    for(count=0; count< 100 && rc == SA_AIS_OK; ++count) {
	rc = saImmOmAccessorGet_2(accessorHandle, &objectName, NULL, &attributes);
    }
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

