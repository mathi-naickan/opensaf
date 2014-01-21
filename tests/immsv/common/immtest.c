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

#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include "immtest.h"

const SaVersionT constImmVersion = {'A', 0x02, 0x0d};
SaVersionT immVersion = {'A', 0x02, 0x0d};
SaAisErrorT rc;
SaImmHandleT immOmHandle;
SaImmHandleT immOiHandle;
SaImmOiCallbacksT_2 immOiCallbacks;
SaImmCallbacksT immOmCallbacks = {NULL};
SaImmCallbacksT_o2 immOmA2bCallbacks = {NULL};
SaSelectionObjectT selectionObject;

const SaImmClassNameT configClassName = (SaImmClassNameT) "TestClassConfig";
const SaImmClassNameT runtimeClassName = "TestClassRuntime";
const SaImmClassNameT nodanglingClassName = (SaImmClassNameT) "TestClassNoDangling";

const SaNameT rootObj = {
		.length = sizeof("rdn=root"),
		.value = "rdn=root"
};

void (*test_setup)(void) = NULL;
void (*test_cleanup)(void) = NULL;

int main(int argc, char **argv) 
{
    int suite = ALL_SUITES, tcase = ALL_TESTS;
    int rc = 0;

    srandom(getpid());

    if (argc > 1)
    {
        suite = atoi(argv[1]);
    }

    if (argc > 2)
    {
        tcase = atoi(argv[2]);
    }

    if(test_setup)
        test_setup();

    if (suite == 0)
    {
        test_list();
    }
    else
    {
        rc = test_run(suite, tcase);
    }

    if(test_cleanup)
        test_cleanup();

    /* Added pthread_exit() to remove dlopen@@GLIBC leak from valgrind */
    pthread_exit(NULL);

    return rc;
}  

SaAisErrorT config_class_create(SaImmHandleT immHandle)
{
    SaAisErrorT err=SA_AIS_OK;
    SaImmAttrDefinitionT_2 rdn = {
        "rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN,
        NULL
    };

    SaImmAttrDefinitionT_2 attr1 = {
        "attr1", SA_IMM_ATTR_SAUINT32T, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE,
	NULL};

    SaImmAttrDefinitionT_2 attr2 = {
        "attr2", SA_IMM_ATTR_SAUINT32T, SA_IMM_ATTR_RUNTIME, NULL};

    SaImmAttrDefinitionT_2 attr3 = {
        "attr3", SA_IMM_ATTR_SASTRINGT, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE | SA_IMM_ATTR_MULTI_VALUE, NULL};

    SaImmAttrDefinitionT_2 attr4 = {
        "attr4", SA_IMM_ATTR_SASTRINGT, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE, NULL};

    SaImmAttrDefinitionT_2 attr5 = {
        "attr5", SA_IMM_ATTR_SAANYT, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE, NULL};

    SaImmAttrDefinitionT_2 attr6 = {
        "attr6", SA_IMM_ATTR_SAANYT, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE | SA_IMM_ATTR_MULTI_VALUE, NULL};

    SaImmAttrDefinitionT_2 attr7 = {
        "attr7", SA_IMM_ATTR_SAANYT, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE | SA_IMM_ATTR_MULTI_VALUE | SA_IMM_ATTR_NO_DUPLICATES, NULL};

    const SaImmAttrDefinitionT_2* attributes[] = {&rdn, &attr1, &attr2, &attr3, &attr4, &attr5, &attr6, &attr7, NULL};

    err = saImmOmClassCreate_2(immHandle, configClassName, SA_IMM_CLASS_CONFIG,
        attributes);
    if((err == SA_AIS_OK) || (err == SA_AIS_ERR_EXIST)) {
        return SA_AIS_OK;
    }

    return err;
}

SaAisErrorT config_class_delete(SaImmHandleT immHandle)
{
    SaAisErrorT err = saImmOmClassDelete(immHandle, configClassName);
    if((err == SA_AIS_OK) || (err == SA_AIS_ERR_NOT_EXIST)) {
        return SA_AIS_OK;
    }

    return err;
}


SaAisErrorT runtime_class_create(SaImmHandleT immHandle)
{
    SaImmAttrDefinitionT_2 rdn = {
        "rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED,
        NULL
    };

    SaImmAttrDefinitionT_2 attr1 = {
        "attr1", SA_IMM_ATTR_SAUINT32T, SA_IMM_ATTR_RUNTIME, NULL};

    const SaImmAttrDefinitionT_2* attributes[] = {&rdn, &attr1, NULL};

    return  saImmOmClassCreate_2(immHandle, runtimeClassName, SA_IMM_CLASS_RUNTIME,
        attributes);
}


SaAisErrorT runtime_class_delete(SaImmHandleT immHandle)
{
    SaAisErrorT err = saImmOmClassDelete(immHandle, runtimeClassName);
    if((err == SA_AIS_OK) || (err == SA_AIS_ERR_NOT_EXIST)) {
        return SA_AIS_OK;
    }

    return err;
}


SaAisErrorT nodangling_class_create(SaImmHandleT immHandle)
{
    SaAisErrorT err=SA_AIS_OK;
    SaImmAttrDefinitionT_2 rdn = {
        "rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN,
        NULL
    };

    SaImmAttrDefinitionT_2 attr1 = {
        "attr1", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE | SA_IMM_ATTR_NO_DANGLING,
        NULL};

    SaImmAttrDefinitionT_2 attr2 = {
        "attr2", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE | SA_IMM_ATTR_MULTI_VALUE | SA_IMM_ATTR_NO_DANGLING,
        NULL};

    const SaImmAttrDefinitionT_2* attributes[] = {&rdn, &attr1, &attr2, NULL};

    err = saImmOmClassCreate_2(immHandle, nodanglingClassName, SA_IMM_CLASS_CONFIG,
        attributes);
    if((err == SA_AIS_OK) || (err == SA_AIS_ERR_EXIST)) {
        return SA_AIS_OK;
    }

    return err;
}


SaAisErrorT nodangling_class_delete(SaImmHandleT immHandle)
{
    SaAisErrorT err = saImmOmClassDelete(immHandle, nodanglingClassName);
    if((err == SA_AIS_OK) || (err == SA_AIS_ERR_NOT_EXIST)) {
        return SA_AIS_OK;
    }

    return err;
}

SaAisErrorT object_create_2(SaImmHandleT immHandle,
        SaImmCcbHandleT ccbHandle,
        const SaImmClassNameT className,
        const SaNameT *rdnObj,
        const SaNameT *parentName,
        const SaImmAttrValuesT_2 *value)
{
    SaAisErrorT err;
    const SaNameT* nameValues[] = {rdnObj, NULL};
    SaImmAttrValuesT_2 v = {NULL,  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    const SaImmAttrValuesT_2 * attrValues[] = {&v, value, NULL};
    SaImmClassCategoryT category;
    SaImmAttrDefinitionT_2 **attrDefinition;

    err = saImmOmClassDescriptionGet_2(immHandle, className, &category, &attrDefinition);
    if(err != SA_AIS_OK) {
        return err;
    }

    int i = 0;
    while(attrDefinition[i]) {
        if(attrDefinition[i]->attrFlags & SA_IMM_ATTR_RDN) {
            v.attrName = attrDefinition[i]->attrName;
            break;
        }
        i++;
    }

    assert(attrDefinition[i]);

    err = saImmOmCcbObjectCreate_2(ccbHandle, className, parentName, attrValues);

    safassert(saImmOmClassDescriptionMemoryFree_2(immHandle, attrDefinition), SA_AIS_OK);

    return err;
}

SaAisErrorT object_delete_2(SaImmCcbHandleT ccbHandle,
        const SaNameT *dnObj,
        int strict)
{
	SaAisErrorT err;

    err = saImmOmCcbObjectDelete(ccbHandle, dnObj);
    if(!strict && err == SA_AIS_ERR_NOT_EXIST) {
    	err = SA_AIS_OK;
    }

    return err;
}

SaAisErrorT object_create(SaImmHandleT immHandle,
		SaImmAdminOwnerHandleT ownerHandle,
        const SaImmClassNameT className,
        const SaNameT *rdnObj,
        const SaNameT *parentName,
        const SaImmAttrValuesT_2 *value)
{
    SaAisErrorT err;
    SaImmCcbHandleT ccbHandle;

    err = saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle);
    if(err != SA_AIS_OK) {
        return err;
    }

    err = object_create_2(immHandle, ccbHandle, className, rdnObj, parentName, value);
    if(err == SA_AIS_OK) {
        err = saImmOmCcbApply(ccbHandle);
    }

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    return err;
}

SaAisErrorT object_delete(SaImmAdminOwnerHandleT ownerHandle,
        const SaNameT *dnObj,
        int strict)
{
	SaAisErrorT err;
    SaImmCcbHandleT ccbHandle;

    err = saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle);
    if(err != SA_AIS_OK) {
        return err;
    }

    err = object_delete_2(ccbHandle, dnObj, 1);
    if(err == SA_AIS_OK) {
        err = saImmOmCcbApply(ccbHandle);
    } else if(!strict && err == SA_AIS_ERR_NOT_EXIST) {
        err = SA_AIS_OK;
    }

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    return err;
}
