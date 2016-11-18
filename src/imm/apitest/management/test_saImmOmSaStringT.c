/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2015 The OpenSAF Foundation
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

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>

#include "base/osaf_extended_name.h"
#include "imm/apitest/immtest.h"

static int mainThreadReady;

// Parameters from saImmOmAdminOperationInvokeCallback
static SaInvocationT callbackInvocation = 0;
static SaAisErrorT callbackOperationReturnValue;
static SaAisErrorT callbackError;

static SaStringT expectedObjectName;	// Used for both DN and RDN checks


static void saImmOiAdminOperationCallback_o3(SaImmOiHandleT immOiHandle,
		SaInvocationT invocation,
		SaConstStringT objectName,
		SaImmAdminOperationIdT operationId,
		const SaImmAdminOperationParamsT_2 **params) {
	if(!strcmp(objectName, expectedObjectName)) {
		safassert(saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_OK, params), SA_AIS_OK);
	} else {
		safassert(saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM, params), SA_AIS_OK);
	}
}

static void saImmOmAdminOperationInvokeCallback(SaInvocationT invocation,
		SaAisErrorT operationReturnValue, SaAisErrorT error) {
	callbackInvocation = invocation;
	callbackOperationReturnValue = operationReturnValue;
	callbackError = error;
}

static const SaImmOiCallbacksT_o3 immOiCallbacks_o3 = {
		saImmOiAdminOperationCallback_o3,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
};


static const SaImmCallbacksT immOmCallback = {
	saImmOmAdminOperationInvokeCallback
};

static void *oi_thread(void *arg) {
	SaImmOiHandleT immOiHandle = *(SaImmOiHandleT *)arg;
	SaSelectionObjectT selObj;
	struct pollfd fds[2];
	int rc;

	safassert(saImmOiSelectionObjectGet(immOiHandle, &selObj), SA_AIS_OK);
	mainThreadReady = 1;

	fds[0].fd = (int)selObj;
	fds[0].events = POLLIN;
	fds[1].fd = stopFd[0];
	fds[1].events = POLLIN;

	while (1) {
		rc = poll(fds, 2, -1);
		if (rc == -1)
			fprintf(stderr, "poll error: %s\n", strerror(errno));

		if (fds[0].revents & POLLIN)
			saImmOiDispatch(immOiHandle, SA_DISPATCH_ONE);

		if (fds[1].revents & POLLIN)
			break;
	}

	return NULL;
}

static SaAisErrorT config_object_create(SaImmHandleT immHandle,
		SaImmAdminOwnerHandleT ownerHandle,
		SaConstStringT objectName)
{
	SaImmCcbHandleT ccbHandle;
	SaUint32T  int1Value1 = __LINE__;
	SaUint32T* int1Values[] = {&int1Value1};
	SaStringT strValue1 = "String1-duplicate";
	SaStringT strValue2 = "String2";
	SaStringT* strValues[] = {&strValue2};
	SaStringT* str2Values[] = {&strValue1, &strValue2, &strValue1};
	SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
	SaImmAttrValuesT_2 v2 = {"attr3", SA_IMM_ATTR_SASTRINGT, 3, (void**)str2Values};
	SaImmAttrValuesT_2 v3 = {"attr4", SA_IMM_ATTR_SASTRINGT, 1, (void**)strValues};
	const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, &v3, NULL};

	safassert(saImmOmCcbInitialize(ownerHandle, SA_IMM_CCB_REGISTERED_OI | SA_IMM_CCB_ALLOW_NULL_OI, &ccbHandle), SA_AIS_OK);
	safassert(saImmOmCcbObjectCreate_o3(ccbHandle, configClassName, objectName, attrValues), SA_AIS_OK);
	safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
	return saImmOmCcbFinalize(ccbHandle);
}

static SaAisErrorT config_object_delete(SaImmHandleT immHandle,
		SaImmAdminOwnerHandleT ownerHandle, SaConstStringT objectName)
{
	SaImmCcbHandleT ccbHandle;

	safassert(saImmOmCcbInitialize(ownerHandle, SA_IMM_CCB_REGISTERED_OI | SA_IMM_CCB_ALLOW_NULL_OI, &ccbHandle), SA_AIS_OK);
	safassert(saImmOmCcbObjectDelete_o3(ccbHandle, objectName), SA_AIS_OK);
	safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
	return saImmOmCcbFinalize(ccbHandle);
}

static void saImmOmSaStringT_01(void) {
	SaImmHandleT immOmHandle;
	const SaImmClassNameT className = "TestClass";

	SaImmAttrDefinitionT_2 rdnDef = {
			"rdn",
			SA_IMM_ATTR_SANAMET,
			SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN,
			NULL
	};
	SaImmAttrDefinitionT_2 strDnDef = {
			"attr",
			SA_IMM_ATTR_SASTRINGT,
			SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE | SA_IMM_ATTR_DN,
			NULL
	};
	const SaImmAttrDefinitionT_2 *attrDefs[] = {
			&rdnDef,
			&strDnDef,
			NULL
	};

	safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
	rc = saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_CONFIG, attrDefs);
	test_validate(rc, SA_AIS_OK);
	if(rc == SA_AIS_OK) {
		safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
	}
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

static void saImmOmSaStringT_02(void) {
	SaImmHandleT immOmHandle;
	const SaImmClassNameT className = "TestClass";

	SaImmAttrDefinitionT_2 rdnDef = {
			"rdn",
			SA_IMM_ATTR_SANAMET,
			SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN,
			NULL
	};
	SaImmAttrDefinitionT_2 strDnDef = {
			"attr",
			SA_IMM_ATTR_SASTRINGT,
			SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE | SA_IMM_ATTR_DN,
			NULL
	};
	const SaImmAttrDefinitionT_2 *attrDefs[] = {
			&rdnDef,
			&strDnDef,
			NULL
	};

	safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
	rc = saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_CONFIG, attrDefs);
	test_validate(rc, SA_AIS_OK);
	if(rc == SA_AIS_OK) {
		safassert(saImmOmClassDelete(immOmHandle, className), SA_AIS_OK);
	}
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

static void saImmOmSaStringT_03(void) {
	SaImmHandleT immOmHandle;
	const SaImmClassNameT className = "TestClass";
	SaImmClassCategoryT classCategory;
	SaImmAttrDefinitionT_2 **attrDefinitions;

	SaImmAttrDefinitionT_2 rdnDef = {
			"rdn",
			SA_IMM_ATTR_SANAMET,
			SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN,
			NULL
	};
	SaImmAttrDefinitionT_2 strDnDef = {
			"attr",
			SA_IMM_ATTR_SASTRINGT,
			SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE | SA_IMM_ATTR_DN,
			NULL
	};
	const SaImmAttrDefinitionT_2 *attrDefs[] = {
			&rdnDef,
			&strDnDef,
			NULL
	};

	safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOmClassCreate_2(immOmHandle, className, SA_IMM_CLASS_CONFIG, attrDefs), SA_AIS_OK);
	rc = saImmOmClassDescriptionGet_2(immOmHandle, className, &classCategory, &attrDefinitions);
	if(rc == SA_AIS_OK) {
		int i;
		for(i=0; attrDefinitions[i]; i++) {
			if((attrDefinitions[i]->attrFlags & SA_IMM_ATTR_DN) &&
					!strcmp(attrDefinitions[i]->attrName, "attr")) {
				break;
			}
		}
		if(!attrDefinitions[i]) {
			/* Attribute with SA_DN flag was not found in the result of attribute definition
			 * SA_AIS_ERR_INVALID_PARAM will be set as error code */
			rc = SA_AIS_ERR_INVALID_PARAM;
		}
		saImmOmClassDescriptionMemoryFree_2(immOmHandle, attrDefinitions);
	}

	test_validate(rc, SA_AIS_OK);

	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

static void saImmOmSaStringT_04(void) {
	SaImmHandleT immOmHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmAdminOwnerNameT ownerName = (SaImmAdminOwnerNameT)__FUNCTION__;
	SaConstStringT objectNames[] = { "rdn=root", NULL };
	SaAisErrorT rc;

	safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immOmHandle, ownerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	rc = saImmOmAdminOwnerSet_o3(ownerHandle, objectNames, SA_IMM_ONE);
	if(rc == SA_AIS_OK) {
		rc = saImmOmAdminOwnerRelease_o3(ownerHandle, objectNames, SA_IMM_ONE);
	}
	test_validate(rc, SA_AIS_OK);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

static void saImmOmSaStringT_05(void) {
	SaImmHandleT immOmHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmAdminOwnerNameT ownerName = (SaImmAdminOwnerNameT)__FUNCTION__;
	SaConstStringT objectNames[] = { "rdn=root", NULL };
	SaAisErrorT rc;

	safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immOmHandle, ownerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	rc = saImmOmAdminOwnerSet_o3(ownerHandle, objectNames, SA_IMM_ONE);
	if(rc == SA_AIS_OK) {
		rc = saImmOmAdminOwnerClear_o3(immOmHandle, objectNames, SA_IMM_ONE);
	}
	test_validate(rc, SA_AIS_OK);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

static void saImmOmSaStringT_06(void) {
	SaImmHandleT immOmHandle;
	SaImmSearchHandleT searchHandle;
	SaStringT root = "opensafImm=opensafImm,safApp=safImmService";
	SaStringT objectName = NULL;
	SaImmAttrValuesT_2 **attributes;

	safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);

	rc = saImmOmSearchInitialize_o3(immOmHandle,
				root,
				SA_IMM_SUBTREE,
				SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR,
				NULL,
				NULL,
				&searchHandle);

	if(rc == SA_AIS_OK) {
		rc = saImmOmSearchNext_o3(searchHandle, &objectName, &attributes);

		// Extra check for object name
		if(rc == SA_AIS_OK) {
			if(!objectName || strcmp(root, objectName)) {
				rc = SA_AIS_ERR_INVALID_PARAM;
			}
		}
	}

	test_validate(rc, SA_AIS_OK);

	safassert(saImmOmSearchFinalize(searchHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

static void saImmOmSaStringT_07(void) {
	SaImmHandleT immOmHandle;
	SaImmAccessorHandleT accessorHandle;
	SaStringT root = "opensafImm=opensafImm,safApp=safImmService";
	SaImmAttrValuesT_2 **attributes;

	safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);

	rc = saImmOmAccessorGet_o3(accessorHandle, root, NULL, &attributes);
	test_validate(rc, SA_AIS_OK);

	safassert(saImmOmAccessorFinalize(accessorHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

static void saImmOmSaStringT_08(void) {
	SaImmHandleT immOmHandle;
	SaImmOiHandleT immOiHandle;
	const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
	SaImmAdminOwnerHandleT ownerHandle;
	SaStringT dn = "obj=1,rdn=root";
	SaStringT root = "rdn=root";
	SaConstStringT dnArr[2] = { root, NULL };
	pthread_t threadid;
	SaInvocationT invocation;
	SaSelectionObjectT selObj;
	SaUint32T paramVal = 1234;
	SaImmAdminOperationParamsT_2 param = { "attr1", SA_IMM_ATTR_SAUINT32T, &paramVal };
	const SaImmAdminOperationParamsT_2 *params[2] = { &param, NULL };

	mainThreadReady = 0;
	expectedObjectName = dn;

	safassert(saImmOmInitialize(&immOmHandle, &immOmCallback, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet_o3(ownerHandle, dnArr, SA_IMM_ONE), SA_AIS_OK);
	config_object_create(immOmHandle, ownerHandle, dn);

	safassert(saImmOiInitialize_o3(&immOiHandle, &immOiCallbacks_o3, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
	safassert(saImmOiClassImplementerSet(immOiHandle, configClassName), SA_AIS_OK);

	pipe_stop_fd();
	assert(!pthread_create(&threadid, NULL, oi_thread, (void *)&immOiHandle));

	// Wait OI thread to be ready
	while(!mainThreadReady) {
		usleep(100);
	}

	safassert(saImmOmSelectionObjectGet(immOmHandle, &selObj), SA_AIS_OK);

	invocation = 111;
	rc = saImmOmAdminOperationInvokeAsync_o3(ownerHandle, invocation, dn, 1, 1, params);

	if(rc == SA_AIS_OK) {
		struct pollfd fds[1];
		fds[0].fd = (int)selObj;
		fds[0].events = POLLIN;
		// Wait for max 6 seconds
		if((poll(fds, 1, 6000) == 1) && (fds[0].fd == (int)selObj)) {
			safassert(saImmOmDispatch(immOmHandle, SA_DISPATCH_ONE), SA_AIS_OK);

			// Extra check
			if(callbackInvocation != invocation
					|| callbackOperationReturnValue != SA_AIS_OK
					|| callbackError != SA_AIS_OK) {
				rc = SA_AIS_ERR_BAD_OPERATION;
			}
		} else {
			rc = SA_AIS_ERR_BAD_OPERATION;
		}
	}

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	test_validate(rc, SA_AIS_OK);

	safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);
	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

	config_object_delete(immOmHandle, ownerHandle, dn);

	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

static void saImmOmSaStringT_09(void) {
	SaImmHandleT immOmHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaImmAdminOwnerNameT ownerName = (SaImmAdminOwnerNameT)__FUNCTION__;
	SaConstStringT objectNames[] = { "rdn=root", NULL };
	const SaImmAttrValuesT_2 *attrValues[1] = { NULL };
	SaAisErrorT rc;

	safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immOmHandle, ownerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet_o3(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	rc = saImmOmCcbObjectCreate_o3(ccbHandle, configClassName, "obj=1,rdn=root", attrValues);
	test_validate(rc, SA_AIS_OK);

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

static void saImmOmSaStringT_10(void) {
	SaImmHandleT immOmHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaImmAdminOwnerNameT ownerName = (SaImmAdminOwnerNameT)__FUNCTION__;
	SaConstStringT objectNames[] = { "rdn=root", NULL };
	SaAisErrorT rc;

	safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immOmHandle, ownerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet_o3(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	rc = saImmOmCcbObjectCreate_o3(ccbHandle, configClassName, "obj=1,rdn=root", NULL);
	test_validate(rc, SA_AIS_OK);

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

static void saImmOmSaStringT_11(void) {
	SaImmSearchHandleT searchHandle;
	SaNameT objectName;
	SaImmAttrValuesT_2 **attributes;
	SaImmSearchParametersT_2 searchParam;
	SaStringT className = "OpensafImm";
	SaAisErrorT rc = SA_AIS_OK;
	int i;

	/* Search for opensafImm=opensafImm,safApp=safImmService */
	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOmSearchInitialize_o3(immOmHandle, NULL, SA_IMM_SUBTREE,
			SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam, NULL, &searchHandle), SA_AIS_OK);

	safassert(saImmOmSearchNext_2(searchHandle, &objectName, &attributes), SA_AIS_OK);

	/* Check that RDN attribute is not in the result */
	for(i=0; attributes[i]; i++) {
		if(!strcmp(attributes[i]->attrName, "opensafImm")) {
			rc = SA_AIS_ERR_INVALID_PARAM;
			break;
		}
	}

	test_validate(rc, SA_AIS_OK);
	safassert(saImmOmSearchFinalize(searchHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

static void saImmOmSaStringT_12(void) {
	SaImmSearchHandleT searchHandle;
	SaNameT objectName;
	SaImmAttrValuesT_2 **attributes;
	SaImmSearchParametersT_2 searchParam;
	SaStringT className = "OpensafImm";
	SaImmAttrNameT attributeNames[2] = { "opensafImm", NULL };
	SaAisErrorT rc;
	int i;

	/* Search for opensafImm=opensafImm,safApp=safImmService */
	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOmSearchInitialize_o3(immOmHandle, NULL, SA_IMM_SUBTREE,
			SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR, &searchParam, attributeNames, &searchHandle), SA_AIS_OK);

	safassert(saImmOmSearchNext_2(searchHandle, &objectName, &attributes), SA_AIS_OK);

	/* Find RDN attribute in the result */
	for(i=0; attributes[i]; i++) {
		if(!strcmp(attributes[i]->attrName, "opensafImm")) {
			break;
		}
	}

	/* Check that RDN exists */
	if(attributes[i]) {
		rc = SA_AIS_OK;
	} else {
		rc = SA_AIS_ERR_INVALID_PARAM;
	}

	test_validate(rc, SA_AIS_OK);
	safassert(saImmOmSearchFinalize(searchHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmSaStringT_13(void)
{
	SaImmAccessorHandleT accessorHandle;
	SaConstStringT objectName = "opensafImm=opensafImm,safApp=safImmService";
	SaImmAttrValuesT_2 **attributes;
	SaAisErrorT rc = SA_AIS_OK;
	int i;

	safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
	safassert(saImmOmAccessorGet_o3(accessorHandle, objectName, NULL, &attributes), SA_AIS_OK);

	/* Find RDN attribute in the result */
	for(i=0; attributes[i]; i++) {
		if(!strcmp(attributes[i]->attrName, "opensafImm")) {
			rc = SA_AIS_ERR_INVALID_PARAM;
			break;
		}
	}

	test_validate(rc, SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmSaStringT_14(void)
{
	SaImmAccessorHandleT accessorHandle;
	SaConstStringT objectName = "opensafImm=opensafImm,safApp=safImmService";
	SaImmAttrNameT attributeNames[2] = { "opensafImm", NULL };
	SaImmAttrValuesT_2 **attributes;
	SaAisErrorT rc = SA_AIS_OK;
	int i;

	safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
	safassert(saImmOmAccessorGet_o3(accessorHandle, objectName, attributeNames, &attributes), SA_AIS_OK);

	/* Find RDN attribute in the result */
	for(i=0; attributes[i]; i++) {
		if(!strcmp(attributes[i]->attrName, "opensafImm")) {
			break;
		}
	}

	/* Check that RDN exists */
	if(attributes[i]) {
		rc = SA_AIS_OK;
	} else {
		rc = SA_AIS_ERR_INVALID_PARAM;
	}

	test_validate(rc, SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}


__attribute__ ((constructor)) static void saImmOmString_constructor(void)
{
	test_suite_add(9, "SaNameT to SaStringT");
	test_case_add(9, saImmOmSaStringT_01, "saImmOmClassCreate_2 and SA_DN - SA_AIS_OK");
	test_case_add(9, saImmOmSaStringT_02, "saImmOmClassCreate_2 and non-string attribute with SA_DN - SA_AIS_ERR_INVALID_PARAM");
	test_case_add(9, saImmOmSaStringT_03, "saImmOmClassDescriptionGet_2 and SA_DN - SA_AIS_OK");
	test_case_add(9, saImmOmSaStringT_04, "saImmOmAdminOwnerSet_o3 and saImmOmAdminOwnerRelease_o3 - SA_AIS_OK");
	test_case_add(9, saImmOmSaStringT_05, "saImmOmAdminOwnerSet_o3 and saImmOmAdminOwnerClear_o3 - SA_AIS_OK");
	test_case_add(9, saImmOmSaStringT_06, "saImmOmSearchInitialize_o3 and saImmOmSearchNext_o3 - SA_AIS_OK");
	test_case_add(9, saImmOmSaStringT_07, "saImmOmAccessorGet_o3 - SA_AIS_OK");
	test_case_add(9, saImmOmSaStringT_08, "saImmOmAdminOperationInvokeAsync_o3 - SA_AIS_OK");
	test_case_add(9, saImmOmSaStringT_09, "saImmOmCcbObjectCreate_o3 without attributes - SA_AIS_OK");
	test_case_add(9, saImmOmSaStringT_10, "saImmOmCcbObjectCreate_o3 with NULL in attrValues - SA_AIS_OK");
	test_case_add(9, saImmOmSaStringT_11, "saImmOmSearchInitialize_o3 with SA_IMM_SEARCH_GET_ALL_ATTR - SA_AIS_OK");
	test_case_add(9, saImmOmSaStringT_12, "saImmOmSearchInitialize_o3 with SA_IMM_SEARCH_GET_SOME_ATTR - SA_AIS_OK");
	test_case_add(9, saImmOmSaStringT_13, "saImmOmAccessorGet_o3 without RDN in result - SA_AIS_OK");
	test_case_add(9, saImmOmSaStringT_14, "saImmOmAccessorGet_o3 with RDN in result - SA_AIS_OK");
}
