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
#include <time.h>

#include "base/osaf_extended_name.h"
#include "imm/apitest/immtest.h"

static int mainThreadReady;

static char *expectedRdnAttr;
static SaStringT expectedObjectName;	// Used for both DN and RDN checks
static SaNameT *expectedParentName;
static SaUint32T expectedU32;
static SaStringT expectedString;


static SaAisErrorT saImmOiCcbObjectCreateCallback_o3(SaImmOiHandleT immOiHandle,
		SaImmOiCcbIdT ccbId,
		const SaImmClassNameT className,
		SaConstStringT objectName, const SaImmAttrValuesT_2 **attr) {
	if(!strcmp(objectName, expectedObjectName)) {
		return SA_AIS_OK;
	} else {
		return SA_AIS_ERR_INVALID_PARAM;
	}
}

static SaAisErrorT saImmOiCcbObjectCreateCallback(SaImmOiHandleT immOiHandle,
		SaImmOiCcbIdT ccbId,
		const SaImmClassNameT className,
		const SaNameT *parentName, const SaImmAttrValuesT_2 **attr) {

	size_t len = osaf_extended_name_length(expectedParentName);
	if((osaf_extended_name_length(parentName) != len)
			|| (strncmp(osaf_extended_name_borrow(parentName), osaf_extended_name_borrow(expectedParentName), len))) {
		return SA_AIS_ERR_INVALID_PARAM;
	}

	int i;
	for(i=0; attr[i]; i++) {
		if(strcmp(attr[i]->attrName, expectedRdnAttr)) {
			continue;
		}

		// For RDN, number of values must be one
		if(attr[i]->attrValuesNumber != 1) {
			return SA_AIS_ERR_INVALID_PARAM;
		}

		if(attr[i]->attrValueType == SA_IMM_ATTR_SASTRINGT) {
			SaStringT rdn = attr[i]->attrValues[0];
			if(!strcmp(rdn, expectedObjectName)) {
				return SA_AIS_OK;
			}
		} else {
			SaNameT *rdn = attr[i]->attrValues[0];
			if(!strncmp(osaf_extended_name_borrow(rdn), expectedObjectName, strlen(expectedObjectName))) {
				return SA_AIS_OK;
			}
		}
	}

	return SA_AIS_ERR_INVALID_PARAM;
}

static SaAisErrorT saImmOiCcbObjectModifyCallback_o3(SaImmOiHandleT immOiHandle,
		SaImmOiCcbIdT ccbId,
		SaConstStringT objectName,
		const SaImmAttrModificationT_2 **attrMods) {
	if(!strcmp(objectName, expectedObjectName)) {
		return SA_AIS_OK;
	} else {
		return SA_AIS_ERR_INVALID_PARAM;
	}
}

static SaAisErrorT saImmOiCcbObjectModifyCallback(SaImmOiHandleT immOiHandle,
		SaImmOiCcbIdT ccbId,
		const SaNameT *objectName,
		const SaImmAttrModificationT_2 **attrMods) {
	size_t len = strlen(expectedObjectName);
	if((osaf_extended_name_length(objectName) == len)
			&& (!strncmp(osaf_extended_name_borrow(objectName), expectedObjectName, len))) {
		return SA_AIS_OK;
	} else {
		return SA_AIS_ERR_INVALID_PARAM;
	}
}

static SaAisErrorT saImmOiCcbObjectDeleteCallback_o3(SaImmOiHandleT immOiHandle,
		SaImmOiCcbIdT ccbId,
		SaConstStringT objectName) {
	if(!strcmp(objectName, expectedObjectName)) {
		return SA_AIS_OK;
	} else {
		return SA_AIS_ERR_INVALID_PARAM;
	}
}

static SaAisErrorT saImmOiCcbObjectDeleteCallback(SaImmOiHandleT immOiHandle,
		SaImmOiCcbIdT ccbId,
		const SaNameT *objectName) {
	size_t len = strlen(expectedObjectName);
	if(osaf_extended_name_length(objectName) == len &&
			!strncmp(osaf_extended_name_borrow(objectName), expectedObjectName, len)) {
		return SA_AIS_OK;
	} else {
		return SA_AIS_ERR_INVALID_PARAM;
	}
}

static SaAisErrorT saImmOiRtAttrUpdateCallback_o3(SaImmOiHandleT immOiHandle,
		SaConstStringT objectName,
		const SaImmAttrNameT *attributeNames) {
	SaUint32T attrValue = expectedU32;
	SaImmAttrValueT attrValues[1] = { &attrValue };
	SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE ,
			{ attributeNames[0], SA_IMM_ATTR_SAUINT32T, 1, attrValues} };
	const SaImmAttrModificationT_2 *attrMods[2] = { &attrMod, NULL };

	if(!strcmp(objectName, expectedObjectName) && !strcmp(attributeNames[0], "attr1")) {
		safassert(saImmOiRtObjectUpdate_o3(immOiHandle, objectName, attrMods), SA_AIS_OK);
		return SA_AIS_OK;
	} else {
		return SA_AIS_ERR_INVALID_PARAM;
	}
}

static SaAisErrorT saImmOiRtAttrUpdateCallback(SaImmOiHandleT immOiHandle,
		const SaNameT *objectName,
		const SaImmAttrNameT *attributeNames) {
	SaUint32T attrValue = expectedU32;
	SaImmAttrValueT attrValues[1] = { &attrValue };
	SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE ,
			{ attributeNames[0], SA_IMM_ATTR_SAUINT32T, 1, attrValues} };
	const SaImmAttrModificationT_2 *attrMods[2] = { &attrMod, NULL };

	size_t len = strlen(expectedObjectName);
	if(osaf_extended_name_length(objectName) == len
			&& !strncmp(osaf_extended_name_borrow(objectName), expectedObjectName, len)
			&& !strcmp(attributeNames[0], "attr1")) {
		safassert(saImmOiRtObjectUpdate_2(immOiHandle, objectName, attrMods), SA_AIS_OK);
		return SA_AIS_OK;
	} else {
		return SA_AIS_ERR_INVALID_PARAM;
	}
}

static void saImmOiAdminOperationCallback_o3(SaImmOiHandleT immOiHandle,
		SaInvocationT invocation,
		SaConstStringT objectName,
		SaImmAdminOperationIdT operationId,
		const SaImmAdminOperationParamsT_2 **params) {
	/* We expect only one parameter, "attr1" */
	if(!strcmp(objectName, expectedObjectName)
			&& params && params[0] && !params[1]
			&& !strcmp(params[0]->paramName, "attr1")
			&& *(SaUint32T *)params[0]->paramBuffer == expectedU32) {
		// Increase incoming value by one
		(*(SaUint32T *)params[0]->paramBuffer)++;
		safassert(saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_OK, params), SA_AIS_OK);
	} else {
		safassert(saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM, params), SA_AIS_OK);
	}
}

static void saImmOiAdminOperationCallback(SaImmOiHandleT immOiHandle,
		SaInvocationT invocation,
		const SaNameT *objectName,
		SaImmAdminOperationIdT operationId,
		const SaImmAdminOperationParamsT_2 **params) {

	size_t len = strlen(expectedObjectName);
	if(osaf_extended_name_length(objectName) == len
			&& !strncmp(osaf_extended_name_borrow(objectName), expectedObjectName, len)
			&& params && params[0] && !params[1]
			&& !strcmp(params[0]->paramName, "attr1")
			&& *(SaUint32T *)params[0]->paramBuffer == expectedU32) {
		// Increase incoming value by one
		(*(SaUint32T *)params[0]->paramBuffer)++;
		safassert(saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_OK, params), SA_AIS_OK);
	} else {
		safassert(saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM, params), SA_AIS_OK);
	}
}

static SaAisErrorT saImmOiCcbCompletedCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId) {
	return SA_AIS_OK;
}

static void saImmOiCcbApplyCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId) {
}

static void saImmOiCcbAbortCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId) {
}

static const SaImmOiCallbacksT_o3 immOiCallbacks_o3 = {
		saImmOiAdminOperationCallback_o3,
		saImmOiCcbAbortCallback,
		saImmOiCcbApplyCallback,
		saImmOiCcbCompletedCallback,
		saImmOiCcbObjectCreateCallback_o3,
		saImmOiCcbObjectDeleteCallback_o3,
		saImmOiCcbObjectModifyCallback_o3,
		saImmOiRtAttrUpdateCallback_o3
};

static const SaImmOiCallbacksT_2 immOiCallbacks_2 = {
		saImmOiAdminOperationCallback,
		saImmOiCcbAbortCallback,
		saImmOiCcbApplyCallback,
		saImmOiCcbCompletedCallback,
		saImmOiCcbObjectCreateCallback,
		saImmOiCcbObjectDeleteCallback,
		saImmOiCcbObjectModifyCallback,
		saImmOiRtAttrUpdateCallback
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

	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
	safassert(saImmOmCcbObjectCreate_o3(ccbHandle, configClassName, objectName, attrValues), SA_AIS_OK);
	safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
	return saImmOmCcbFinalize(ccbHandle);
}

static SaAisErrorT config_object_delete(SaImmHandleT immHandle,
	SaImmAdminOwnerHandleT ownerHandle, SaConstStringT objectName)
{
	SaImmCcbHandleT ccbHandle;

	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
	safassert(saImmOmCcbObjectDelete_o3(ccbHandle, objectName), SA_AIS_OK);
	safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
	return saImmOmCcbFinalize(ccbHandle);
}

static SaAisErrorT runtime_object_create(SaImmOiHandleT immOiHandle,
	SaConstStringT objectName)
{
	const SaImmAttrValuesT_2 * attrValues[1] = { NULL };

	return saImmOiRtObjectCreate_o3(immOiHandle, runtimeClassName, objectName, attrValues);
}

static SaAisErrorT runtime_object_delete(SaImmOiHandleT immOiHandle,
	SaConstStringT objectName)
{
	return saImmOiRtObjectDelete_o3(immOiHandle, objectName);
}

static void saImmOiSaStringT_01(void) {
	const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaStringT dn = "obj=1,rdn=root";
	SaStringT root = "rdn=root";
	SaConstStringT dnArr[2] = { root, NULL };
	SaUint32T  int1Value1 = 7;
	SaUint32T* int1Values[] = {&int1Value1};
	SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
	const SaImmAttrValuesT_2 * attrValues[] = {&v1, NULL};
	pthread_t threadid;

	mainThreadReady = 0;
	expectedObjectName = dn;

	safassert(saImmOiInitialize_o3(&immOiHandle, &immOiCallbacks_o3, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
	safassert(saImmOiClassImplementerSet(immOiHandle, configClassName), SA_AIS_OK);

	pipe_stop_fd();
	assert(!pthread_create(&threadid, NULL, oi_thread, (void *)&immOiHandle));

	// Wait OI thread to be ready
	while(!mainThreadReady) {
		usleep(100);
	}

	safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet_o3(ownerHandle, dnArr, SA_IMM_ONE), SA_AIS_OK);
	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	/* Create object under root */
	rc = saImmOmCcbObjectCreate_o3(ccbHandle, configClassName, dn, attrValues);

	test_validate(rc, SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

	safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);
	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

static void saImmOiSaStringT_02(void) {
	const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaStringT dn = "obj=1,rdn=root";
	SaStringT root = "rdn=root";
	SaConstStringT dnArr[2] = { root, NULL };
	SaUint32T  int1Value1 = 7;
	SaUint32T* int1Values[] = {&int1Value1};
	SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
	const SaImmAttrValuesT_2 * attrValues[] = {&v1, NULL};
	pthread_t threadid;
	SaNameT expParentName = { strlen("rdn=root"), "rdn=root" };

	mainThreadReady = 0;
	expectedObjectName = "obj=1";	// RDN
	expectedParentName = &expParentName;
	expectedRdnAttr = "rdn";

	safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks_2, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
	safassert(saImmOiClassImplementerSet(immOiHandle, configClassName), SA_AIS_OK);

	pipe_stop_fd();
	assert(!pthread_create(&threadid, NULL, oi_thread, (void *)&immOiHandle));

	// Wait OI thread to be ready
	while(!mainThreadReady) {
		usleep(100);
	}

	safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet_o3(ownerHandle, dnArr, SA_IMM_ONE), SA_AIS_OK);
	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	/* Create object under root */
	rc = saImmOmCcbObjectCreate_o3(ccbHandle, configClassName, dn, attrValues);

	test_validate(rc, SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

	safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);
	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

static void saImmOiSaStringT_03(void) {
	const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaStringT root = "rdn=root";
	SaConstStringT dnArr[2] = { root, NULL };
	const SaNameT parentName = { strlen("rdn=root"), "rdn=root" };
	SaNameT rdn = { strlen("obj=1"), "obj=1" };
	SaNameT* nameValues[] = {&rdn};
	SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
	SaUint32T  int1Value1 = 7;
	SaUint32T* int1Values[] = {&int1Value1};
	SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
	const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};
	pthread_t threadid;

	mainThreadReady = 0;
	expectedObjectName = "obj=1,rdn=root";

	safassert(saImmOiInitialize_o3(&immOiHandle, &immOiCallbacks_o3, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
	safassert(saImmOiClassImplementerSet(immOiHandle, configClassName), SA_AIS_OK);

	pipe_stop_fd();
	assert(!pthread_create(&threadid, NULL, oi_thread, (void *)&immOiHandle));

	// Wait OI thread to be ready
	while(!mainThreadReady) {
		usleep(100);
	}

	safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet_o3(ownerHandle, dnArr, SA_IMM_ONE), SA_AIS_OK);
	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	/* Create object under root */
	rc = saImmOmCcbObjectCreate_2(ccbHandle, configClassName, &parentName, attrValues);

	test_validate(rc, SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

	safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);
	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

static void saImmOiSaStringT_04(void) {
	const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaStringT root = "rdn=root";
	SaConstStringT dnArr[2] = { root, NULL };
	SaStringT dn = "obj=1,rdn=root";
	SaUint32T  int1Value1 = __LINE__;
	SaUint32T* int1Values[] = {&int1Value1};
	SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
	SaImmAttrModificationT_2 attrMod = {SA_IMM_ATTR_VALUES_REPLACE, v1};
	const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, NULL};
	pthread_t threadid;

	mainThreadReady = 0;
	expectedObjectName = dn;

	safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
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

	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	/* Create object under root */
	expectedObjectName = dn;
	rc = saImmOmCcbObjectModify_o3(ccbHandle, dn, attrMods);

	test_validate(rc, SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

	safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);
	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

	config_object_delete(immOmHandle, ownerHandle, dn);

	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

static void saImmOiSaStringT_05(void) {
	const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaStringT dn = "obj=1,rdn=root";
	SaStringT root = "rdn=root";
	SaConstStringT dnArr[2] = { root, NULL };
	SaUint32T  int1Value1 = __LINE__;
	SaUint32T* int1Values[] = {&int1Value1};
	SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
	SaImmAttrModificationT_2 attrMod = {SA_IMM_ATTR_VALUES_REPLACE, v1};
	const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, NULL};
	pthread_t threadid;

	mainThreadReady = 0;
	expectedObjectName = dn;

	safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet_o3(ownerHandle, dnArr, SA_IMM_ONE), SA_AIS_OK);

	config_object_create(immOmHandle, ownerHandle, dn);

	safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks_2, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
	safassert(saImmOiClassImplementerSet(immOiHandle, configClassName), SA_AIS_OK);

	pipe_stop_fd();
	assert(!pthread_create(&threadid, NULL, oi_thread, (void *)&immOiHandle));

	// Wait OI thread to be ready
	while(!mainThreadReady) {
		usleep(100);
	}

	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	/* Create object under root */
	expectedObjectName = dn;
	rc = saImmOmCcbObjectModify_o3(ccbHandle, dn, attrMods);

	test_validate(rc, SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

	safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);
	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

	config_object_delete(immOmHandle, ownerHandle, dn);

	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

static void saImmOiSaStringT_06(void) {
	const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	const SaNameT dn = { strlen("obj=1,rdn=root"), "obj=1,rdn=root" };
	SaStringT root = "rdn=root";
	SaConstStringT dnArr[2] = { root, NULL };
	SaUint32T  int1Value1 = __LINE__;
	SaUint32T* int1Values[] = {&int1Value1};
	SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
	SaImmAttrModificationT_2 attrMod = {SA_IMM_ATTR_VALUES_REPLACE, v1};
	const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, NULL};
	pthread_t threadid;

	mainThreadReady = 0;
	expectedObjectName = (SaStringT)dn.value;

	safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet_o3(ownerHandle, dnArr, SA_IMM_ONE), SA_AIS_OK);

	config_object_create(immOmHandle, ownerHandle, (SaConstStringT)dn.value);

	safassert(saImmOiInitialize_o3(&immOiHandle, &immOiCallbacks_o3, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
	safassert(saImmOiClassImplementerSet(immOiHandle, configClassName), SA_AIS_OK);

	pipe_stop_fd();
	assert(!pthread_create(&threadid, NULL, oi_thread, (void *)&immOiHandle));

	// Wait OI thread to be ready
	while(!mainThreadReady) {
		usleep(100);
	}

	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	/* Create object under root */
	rc = saImmOmCcbObjectModify_2(ccbHandle, &dn, attrMods);

	test_validate(rc, SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

	safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);
	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

	config_object_delete(immOmHandle, ownerHandle, (SaConstStringT)dn.value);

	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

static void saImmOiSaStringT_07(void) {
	const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaStringT dn = "obj=1,rdn=root";
	SaStringT root = "rdn=root";
	SaConstStringT dnArr[2] = { root, NULL };
	pthread_t threadid;

	mainThreadReady = 0;
	expectedObjectName = dn;

	safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
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

	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	dnArr[0] = dn;
	safassert(saImmOmAdminOwnerSet_o3(ownerHandle, dnArr, SA_IMM_ONE), SA_AIS_OK);

	/* Create object under root */
	expectedObjectName = dn;
	rc = saImmOmCcbObjectDelete_o3(ccbHandle, dn);
	safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

	test_validate(rc, SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

	safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);
	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

static void saImmOiSaStringT_08(void) {
	const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaStringT dn = "obj=1,rdn=root";
	SaStringT root = "rdn=root";
	SaConstStringT dnArr[2] = { root, NULL };
	pthread_t threadid;

	mainThreadReady = 0;
	expectedObjectName = dn;

	safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
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

	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	dnArr[0] = dn;
	safassert(saImmOmAdminOwnerSet_o3(ownerHandle, dnArr, SA_IMM_ONE), SA_AIS_OK);

	/* Create object under root */
	expectedObjectName = dn;
	rc = saImmOmCcbObjectDelete_o3(ccbHandle, dn);
	safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

	test_validate(rc, SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

	safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);
	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

static void saImmOiSaStringT_09(void) {
	const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	const SaNameT dn = { strlen("obj=1,rdn=root"), "obj=1,rdn=root" };
	SaStringT root = "rdn=root";
	SaConstStringT dnArr[2] = { root, NULL };
	pthread_t threadid;

	mainThreadReady = 0;
	expectedObjectName = (SaStringT)dn.value;

	safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet_o3(ownerHandle, dnArr, SA_IMM_ONE), SA_AIS_OK);

	config_object_create(immOmHandle, ownerHandle, (SaStringT)dn.value);

	safassert(saImmOiInitialize_o3(&immOiHandle, &immOiCallbacks_o3, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
	safassert(saImmOiClassImplementerSet(immOiHandle, configClassName), SA_AIS_OK);

	pipe_stop_fd();
	assert(!pthread_create(&threadid, NULL, oi_thread, (void *)&immOiHandle));

	// Wait OI thread to be ready
	while(!mainThreadReady) {
		usleep(100);
	}

	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	dnArr[0] = (SaStringT)dn.value;
	safassert(saImmOmAdminOwnerSet_o3(ownerHandle, dnArr, SA_IMM_ONE), SA_AIS_OK);

	/* Create object under root */
	rc = saImmOmCcbObjectDelete(ccbHandle, &dn);
	safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

	test_validate(rc, SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

	safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);
	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

static void saImmOiSaStringT_10(void) {
	SaImmOiHandleT immOiHandle;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
	SaStringT dn = "rtobj=1,rdn=root";
	const SaImmAttrValuesT_2 * attrValues[1] = { NULL };
	pthread_t threadid;

	mainThreadReady = 0;

	safassert(saImmOiInitialize_o3(&immOiHandle, &immOiCallbacks_o3, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

	pipe_stop_fd();
	assert(!pthread_create(&threadid, NULL, oi_thread, (void *)&immOiHandle));

	// Wait OI thread to be ready
	while(!mainThreadReady) {
		usleep(100);
	}

	rc = saImmOiRtObjectCreate_o3(immOiHandle, runtimeClassName, dn, attrValues);

	runtime_object_delete(immOiHandle, dn);

	test_validate(rc, SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

static void saImmOiSaStringT_11(void) {
	SaImmOiHandleT immOiHandle;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
	SaStringT dn = "rtobj=1,rdn=root";
	pthread_t threadid;

	mainThreadReady = 0;

	safassert(saImmOiInitialize_o3(&immOiHandle, &immOiCallbacks_o3, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

	pipe_stop_fd();
	assert(!pthread_create(&threadid, NULL, oi_thread, (void *)&immOiHandle));

	// Wait OI thread to be ready
	while(!mainThreadReady) {
		usleep(100);
	}

	rc = saImmOiRtObjectCreate_o3(immOiHandle, runtimeClassName, dn, NULL);

	runtime_object_delete(immOiHandle, dn);

	test_validate(rc, SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

static void saImmOiSaStringT_12(void) {
	SaImmOiHandleT immOiHandle;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
	SaStringT dn = "rtobj=1,rdn=root";
	pthread_t threadid;

	mainThreadReady = 0;
	expectedObjectName = dn;

	safassert(saImmOiInitialize_o3(&immOiHandle, &immOiCallbacks_o3, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

	pipe_stop_fd();
	assert(!pthread_create(&threadid, NULL, oi_thread, (void *)&immOiHandle));

	// Wait OI thread to be ready
	while(!mainThreadReady) {
		usleep(100);
	}

	runtime_object_create(immOiHandle, dn);

	rc = saImmOiRtObjectDelete_o3(immOiHandle, dn);

	test_validate(rc, SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

static void saImmOiSaStringT_13(void) {
	SaImmOiHandleT immOiHandle;
	SaImmHandleT immHandle;
	SaImmAccessorHandleT accessorHandle;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
	SaStringT dn = "rtobj=1,rdn=root";
	pthread_t threadid;
	const SaImmAttrNameT attributeNames[3] = { "attr1", NULL };
	SaImmAttrValuesT_2 **attributes;

	mainThreadReady = 0;
	expectedObjectName = dn;
	srand((int)time(NULL));
	expectedU32 = (SaUint32T)rand();
	expectedString = (SaStringT)__FUNCTION__;

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAccessorInitialize(immHandle, &accessorHandle), SA_AIS_OK);

	safassert(saImmOiInitialize_o3(&immOiHandle, &immOiCallbacks_o3, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

	pipe_stop_fd();
	assert(!pthread_create(&threadid, NULL, oi_thread, (void *)&immOiHandle));

	// Wait OI thread to be ready
	while(!mainThreadReady) {
		usleep(100);
	}

	runtime_object_create(immOiHandle, dn);

	rc = saImmOmAccessorGet_o3(accessorHandle, dn, attributeNames, &attributes);
	// Test search result
	int i, j;
	for(i = 0; rc == SA_AIS_OK && attributeNames[i]; i++) {
		for(j=0; attributes[j]; j++) {
			if(!strcmp(attributes[j]->attrName, "attr1")) {
				if(*(SaUint32T *)attributes[j]->attrValues[0] != expectedU32) {
					rc = SA_AIS_ERR_INVALID_PARAM;
				}
			} else {
				rc = SA_AIS_ERR_INVALID_PARAM;
			}
		}
	}

	runtime_object_delete(immOiHandle, dn);

	test_validate(rc, SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOmAccessorFinalize(accessorHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);

	safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);
	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

static void saImmOiSaStringT_14(void) {
	const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
	SaImmAdminOwnerHandleT ownerHandle;
	SaStringT dn = "obj=1,rdn=root";
	SaStringT root = "rdn=root";
	SaConstStringT dnArr[2] = { root, NULL };
	pthread_t threadid;
	SaUint32T paramVal;
	SaImmAdminOperationParamsT_2 param = { "attr1", SA_IMM_ATTR_SAUINT32T, &paramVal };
	const SaImmAdminOperationParamsT_2 *params[2] = { &param, NULL };
	SaImmAdminOperationParamsT_2 **returnParams;
	SaAisErrorT returnError;

	mainThreadReady = 0;
	expectedObjectName = dn;
	srand((int)time(NULL));
	expectedU32 = (SaUint32T)rand();

	paramVal = expectedU32;

	safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet_o3(ownerHandle, dnArr, SA_IMM_ONE), SA_AIS_OK);

	config_object_create(immOmHandle, ownerHandle, dn);

	dnArr[0] = dn;
	safassert(saImmOmAdminOwnerSet_o3(ownerHandle, dnArr, SA_IMM_ONE), SA_AIS_OK);

	safassert(saImmOiInitialize_o3(&immOiHandle, &immOiCallbacks_o3, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
	safassert(saImmOiClassImplementerSet(immOiHandle, configClassName), SA_AIS_OK);

	pipe_stop_fd();
	assert(!pthread_create(&threadid, NULL, oi_thread, (void *)&immOiHandle));

	// Wait OI thread to be ready
	while(!mainThreadReady) {
		usleep(100);
	}

	rc = saImmOmAdminOperationInvoke_o3(ownerHandle, dn, 1, 1, params, &returnError, 10 * SA_TIME_ONE_SECOND, &returnParams);

	/* Extra check for return parameters */
	if(rc == SA_AIS_OK) {
		if(returnError == SA_AIS_OK
				&& returnParams && returnParams[0] && !returnParams[1]
				&& !strcmp(returnParams[0]->paramName, "attr1")) {
			/* For the test, return parameter "attr1" is increased by 1 */
			if(*(SaUint32T *)returnParams[0]->paramBuffer != expectedU32 + 1) {
				rc = SA_AIS_ERR_BAD_OPERATION;
			}
		} else {
			rc = SA_AIS_ERR_BAD_OPERATION;
		}
	}

	test_validate(rc, SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	saImmOmAdminOperationMemoryFree(ownerHandle, returnParams);

	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

	safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);
	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}


__attribute__ ((constructor)) static void saImmOiString_constructor(void)
{
	test_suite_add(8, "SaNameT to SaStringT");
	test_case_add(8, saImmOiSaStringT_01, "saImmOmCcbObjectCreate_o3 and SaImmOiCcbObjectCreateCallbackT_o3 - SA_AIS_OK");
	test_case_add(8, saImmOiSaStringT_02, "saImmOmCcbObjectCreate_o3 and SaImmOiCcbObjectCreateCallbackT - SA_AIS_OK");
	test_case_add(8, saImmOiSaStringT_03, "saImmOmCcbObjectCreate_2 and SaImmOiCcbObjectCreateCallbackT_o3 - SA_AIS_OK");
	test_case_add(8, saImmOiSaStringT_04, "saImmOmCcbObjectModify_o3 and SaImmOiCcbObjectModifyCallbackT_o3 - SA_AIS_OK");
	test_case_add(8, saImmOiSaStringT_05, "saImmOmCcbObjectModify_o3 and SaImmOiCcbObjectModifyCallbackT - SA_AIS_OK");
	test_case_add(8, saImmOiSaStringT_06, "saImmOmCcbObjectModify_2 and SaImmOiCcbObjectModifyCallbackT_o3 - SA_AIS_OK");
	test_case_add(8, saImmOiSaStringT_07, "saImmOmCcbObjectDelete_o3 and SaImmOiCcbObjectDeleteCallbackT_o3 - SA_AIS_OK");
	test_case_add(8, saImmOiSaStringT_08, "saImmOmCcbObjectDelete_o3 and SaImmOiCcbObjectDeleteCallbackT - SA_AIS_OK");
	test_case_add(8, saImmOiSaStringT_09, "saImmOmCcbObjectDelete and SaImmOiCcbObjectDeleteCallbackT_o3 - SA_AIS_OK");
	test_case_add(8, saImmOiSaStringT_10, "saImmOiRtObjectCreate_o3 - SA_AIS_OK");
	test_case_add(8, saImmOiSaStringT_11, "saImmOiRtObjectCreate_o3 with NULL in attrValues - SA_AIS_OK");
	test_case_add(8, saImmOiSaStringT_12, "saImmOiRtObjectDelete_o3 - SA_AIS_OK");
	test_case_add(8, saImmOiSaStringT_13, "saImmOiRtObjectUpdate_o3 and SaImmOiRtAttrUpdateCallbackT_o3 - SA_AIS_OK");
	test_case_add(8, saImmOiSaStringT_14, "saImmOmAdminOperationInvoke_o3 and SaImmOiAdminOperationCallbackT_o3 - SA_AIS_OK");
}
