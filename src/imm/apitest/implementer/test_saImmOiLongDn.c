/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2014 The OpenSAF Foundation
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


#include <unistd.h>
#include <pthread.h>
#include <poll.h>

#include "imm/apitest/immtest.h"
#include "base/osaf_extended_name.h"


static char *longDn = "longdn=0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
static char *longDn2 = "longdn=0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
		"012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
		"012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";


static void setupOiLongDn(void) {
	SaImmHandleT immHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaNameT dn;
	const SaImmAdminOwnerNameT ownerName = (const SaImmAdminOwnerNameT)__FUNCTION__;

	SaImmAttrValueT attrValues[1] = { &dn };
	SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE,
			{ "attr8", SA_IMM_ATTR_SANAMET, 1, attrValues }};
	const SaImmAttrModificationT_2 *attrMods[2] = { &attrMod, NULL };

	osaf_extended_name_lend(longDn, &dn);

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);
	config_class_create(immHandle);
	safassert(saImmOmAdminOwnerInitialize(immHandle, ownerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(object_create(immHandle, ownerHandle, configClassName, &dn, NULL, NULL), SA_AIS_OK);

	// Change SaNameT attribute for testing
	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
	safassert(saImmOmCcbObjectModify_2(ccbHandle, &dn, attrMods), SA_AIS_OK);
	safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);
}

static void cleanOiLongDn(void) {
	SaImmHandleT immHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaNameT dn;
	const SaNameT *objects[2] = { &dn, NULL };
	const SaImmAdminOwnerNameT ownerName = (const SaImmAdminOwnerNameT)__FUNCTION__;

	osaf_extended_name_lend(longDn, &dn);

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immHandle, ownerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	saImmOmAdminOwnerSet(ownerHandle, objects, SA_IMM_ONE);

	object_delete(ownerHandle, &dn, 0);
	config_class_delete(immHandle);

	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);
}


static void saImmOiAdminOperationCallback(SaImmOiHandleT immOiHandle,
				      SaInvocationT invocation,
				      const SaNameT *objectName,
				      SaImmAdminOperationIdT operationId,
				      const SaImmAdminOperationParamsT_2 **params) {

	if(params && params[0] && !strcmp(params[0]->paramName, "attr8")) {
		SaNameT *val = (SaNameT *)params[0]->paramBuffer;
		if(!strcmp(osaf_extended_name_borrow(val), longDn2)) {
			// Return input parameters
			saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_OK, params);
			return;
		}
	}

	saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL);
}

static void saImmOiCcbAbortCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId) {
}

static void saImmOiCcbApplyCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId) {
}

static SaAisErrorT saImmOiCcbCompletedCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId) {
	return SA_AIS_OK;
}

static SaAisErrorT saImmOiCcbObjectCreateCallback(SaImmOiHandleT immOiHandle,
				       SaImmOiCcbIdT ccbId,
				       const SaImmClassNameT className,
				       const SaNameT *parentName, const SaImmAttrValuesT_2 **attr) {
	return SA_AIS_OK;
}

static SaAisErrorT saImmOiCcbObjectDeleteCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId, const SaNameT *objectName) {
	return SA_AIS_OK;
}

static SaAisErrorT saImmOiCcbObjectModifyCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId, const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods) {
	return SA_AIS_OK;
}

static SaAisErrorT saImmOiRtAttrUpdateCallback(SaImmOiHandleT immOiHandle, const SaNameT *objectName, const SaImmAttrNameT *attributeNames) {
	SaNameT val;
	SaImmAttrValueT attrValues[1] = { &val };
	SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE,
				{ "attr2", SA_IMM_ATTR_SANAMET, 1, attrValues }};
	const SaImmAttrModificationT_2 *attrMods[2] = { &attrMod, NULL };

	if(osaf_extended_name_length(objectName) == strlen(longDn)
			&& !strncmp(osaf_extended_name_borrow(objectName), longDn, strlen(longDn2))) {
		osaf_extended_name_lend(longDn2, &val);
		saImmOiRtObjectUpdate_2(immOiHandle, objectName, attrMods);
		return SA_AIS_OK;
	}

	return SA_AIS_ERR_FAILED_OPERATION;
}

static SaImmOiCallbacksT_2 localImmOiCallbacks = {
		saImmOiAdminOperationCallback,
		saImmOiCcbAbortCallback,
		saImmOiCcbApplyCallback,
		saImmOiCcbCompletedCallback,
		saImmOiCcbObjectCreateCallback,
		saImmOiCcbObjectDeleteCallback,
		saImmOiCcbObjectModifyCallback,
		saImmOiRtAttrUpdateCallback,
};

enum {
	S_DISP_START = 0,
	S_DISP_READY,
	S_DISP_FINISH
};

static int s_dispatch_status = 0;

static void *dispatchThread(void *param) {
	SaImmOiHandleT oiHandle = *((SaImmOiHandleT*) param);
	SaSelectionObjectT selObj;
	int rc;
	struct pollfd fds[2];

	safassert(saImmOiSelectionObjectGet(oiHandle, &selObj), SA_AIS_OK);
	s_dispatch_status = S_DISP_READY;

	fds[0].fd = (int) selObj;
	fds[0].events = POLLIN;
	fds[1].fd = stopFd[0];
	fds[1].events = POLLIN;

	while(1) {
		rc = poll(fds, 2, -1);
		if (rc == -1)
			fprintf(stderr, "poll error: %s\n", strerror(errno));

		if (fds[0].revents & POLLIN)
			saImmOiDispatch(oiHandle, SA_DISPATCH_ONE);

		if (fds[1].revents & POLLIN)
			break;
	}

	return NULL;
}

static void saImmOiLongDn_01(void) {
	SaAisErrorT rc;
	SaImmOiHandleT immOiHandle;
	SaImmHandleT immOmHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaImmAdminOwnerNameT ownerName = (SaImmAdminOwnerNameT)__FUNCTION__;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FILE__;
	pthread_t threadid;

	SaNameT dn;
	SaImmAttrValueT rdnVal[1] = { (SaImmAttrValueT)&dn };
	SaImmAttrValuesT_2 attrValue = { "rdn", SA_IMM_ATTR_SANAMET, 1, rdnVal };
	const SaImmAttrValuesT_2 *attrValues[2] = { &attrValue, NULL };

	osaf_extended_name_lend(longDn, &dn);

	safassert(saImmOiInitialize_2(&immOiHandle, &localImmOiCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

	s_dispatch_status = S_DISP_START;
	pipe_stop_fd();
	assert(pthread_create(&threadid, NULL, dispatchThread, &immOiHandle) == 0);

	while(s_dispatch_status != S_DISP_READY) {
		usleep(100);
	}

	safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
	config_class_create(immOmHandle);

	safassert(saImmOiClassImplementerSet(immOiHandle, configClassName), SA_AIS_OK);

	safassert(saImmOmAdminOwnerInitialize(immOmHandle, ownerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	rc = saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues);

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);

	safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);

	// Wait for thread to exit
	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

	config_class_delete(immOmHandle);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

	test_validate(rc, SA_AIS_OK);
}

static void saImmOiLongDn_02(void) {
	SaAisErrorT rc;
	SaImmOiHandleT immOiHandle;
	SaImmHandleT immOmHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaImmAdminOwnerNameT ownerName = (SaImmAdminOwnerNameT)__FUNCTION__;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FILE__;
	pthread_t threadid;
	SaNameT dn;
	SaNameT val;
	const SaNameT *objects[2] = { &dn, NULL };

	SaImmAttrValueT attrValues[1] = { &val };
	SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE,
			{ "attr8", SA_IMM_ATTR_SANAMET, 1, attrValues }};
	const SaImmAttrModificationT_2 *attrMods[2] = { &attrMod, NULL };

	osaf_extended_name_lend(longDn, &dn);
	osaf_extended_name_lend(longDn2, &val);

	setupOiLongDn();

	safassert(saImmOiInitialize_2(&immOiHandle, &localImmOiCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
	safassert(saImmOiClassImplementerSet(immOiHandle, configClassName), SA_AIS_OK);

	s_dispatch_status = S_DISP_START;
	pipe_stop_fd();
	pthread_create(&threadid, NULL, dispatchThread, &immOiHandle);

	while(s_dispatch_status != S_DISP_READY) {
		usleep(100);
	}

	safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immOmHandle, ownerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet(ownerHandle, objects, SA_IMM_ONE), SA_AIS_OK);
	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	rc = saImmOmCcbObjectModify_2(ccbHandle, &dn, attrMods);

	safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

	safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);

	// Wait for thread to exit
	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

	test_validate(rc, SA_AIS_OK);

	cleanOiLongDn();
}

static void saImmOiLongDn_03(void) {
	SaAisErrorT rc;
	SaImmOiHandleT immOiHandle;
	SaImmHandleT immOmHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaImmAdminOwnerNameT ownerName = (SaImmAdminOwnerNameT)__FUNCTION__;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FILE__;
	pthread_t threadid;
	SaNameT dn;
	const SaNameT *objects[2] = { &dn, NULL };

	osaf_extended_name_lend(longDn, &dn);

	setupOiLongDn();

	safassert(saImmOiInitialize_2(&immOiHandle, &localImmOiCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
	safassert(saImmOiClassImplementerSet(immOiHandle, configClassName), SA_AIS_OK);

	s_dispatch_status = S_DISP_START;
	pipe_stop_fd();
	pthread_create(&threadid, NULL, dispatchThread, &immOiHandle);

	while(s_dispatch_status != S_DISP_READY) {
		usleep(100);
	}

	safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immOmHandle, ownerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet(ownerHandle, objects, SA_IMM_ONE), SA_AIS_OK);
	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	rc = saImmOmCcbObjectDelete(ccbHandle, &dn);

	safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

	safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);

	// Wait for thread to exit
	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

	test_validate(rc, SA_AIS_OK);

	cleanOiLongDn();
}

static void saImmOiLongDn_04(void) {
	SaAisErrorT rc, rc1;
	SaImmOiHandleT immOiHandle;
	SaImmHandleT immOmHandle;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FILE__;
	pthread_t threadid;

	SaNameT dn;
	SaImmAttrValueT rdnVal[1] = { (SaImmAttrValueT)&dn };
	SaImmAttrValuesT_2 attrValue = { "rdn", SA_IMM_ATTR_SANAMET, 1, rdnVal };
	const SaImmAttrValuesT_2 *attrValues[2] = { &attrValue, NULL };

	osaf_extended_name_lend(longDn, &dn);

	safassert(saImmOiInitialize_2(&immOiHandle, &localImmOiCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

	s_dispatch_status = S_DISP_START;
	pipe_stop_fd();
	pthread_create(&threadid, NULL, dispatchThread, &immOiHandle);

	while(s_dispatch_status != S_DISP_READY) {
		usleep(100);
	}

	safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);

	rc = saImmOiRtObjectCreate_2(immOiHandle, runtimeClassName, NULL, attrValues);
	rc1 = saImmOiRtObjectDelete(immOiHandle, &dn);

	if(rc == SA_AIS_OK) {
		rc = rc1;
	}

	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

	// Wait for thread to exit
	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

	test_validate(rc, SA_AIS_OK);
}

static void saImmOiLongDn_05(void) {
	SaAisErrorT rc;
	SaImmOiHandleT immOiHandle;
	SaImmHandleT immOmHandle;
	SaImmAccessorHandleT accessorHandle;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FILE__;
	pthread_t threadid;
	SaImmAttrValuesT_2 **attributes = NULL;

	SaNameT dn;
	SaImmAttrValueT rdnVal[1] = { (SaImmAttrValueT)&dn };
	SaImmAttrValuesT_2 attrValue = { "rdn", SA_IMM_ATTR_SANAMET, 1, rdnVal };
	const SaImmAttrValuesT_2 *attrValues[2] = { &attrValue, NULL };

	osaf_extended_name_lend(longDn, &dn);

	safassert(saImmOiInitialize_2(&immOiHandle, &localImmOiCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

	s_dispatch_status = S_DISP_START;
	pipe_stop_fd();
	pthread_create(&threadid, NULL, dispatchThread, &immOiHandle);

	while(s_dispatch_status != S_DISP_READY) {
		usleep(100);
	}

	safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOiRtObjectCreate_2(immOiHandle, runtimeClassName, NULL, attrValues), SA_AIS_OK);

	safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
	rc = saImmOmAccessorGet_2(accessorHandle, &dn, NULL, &attributes);
	if(rc == SA_AIS_OK) {
		// Accesor get must return a value. attributes cannot be NULL
		assert(attributes);

		SaNameT *val;
		int i;
		for(i=0; attributes[i]; i++) {
			if(!strcmp(attributes[i]->attrName, "attr2")) {
				// Must return 1 value
				assert(attributes[i]->attrValuesNumber == 1);
				assert(attributes[i]->attrValues);

				val = (SaNameT *)attributes[i]->attrValues[0];
				// Check attribute value
				assert(osaf_extended_name_length(val) == strlen(longDn2));
				assert(strncmp(osaf_extended_name_borrow(val), longDn2, strlen(longDn2)) == 0);
				break;
			}
		}

		// Updated value was not found in the result list
		assert(attributes[i]);
	}

	safassert(saImmOiRtObjectDelete(immOiHandle, &dn), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

	// Wait for thread to exit
	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

	test_validate(rc, SA_AIS_OK);
}

static void saImmOiLongDn_06(void) {
	SaAisErrorT rc;
	SaAisErrorT result;
	SaImmOiHandleT immOiHandle;
	SaImmHandleT immOmHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	const SaImmAdminOwnerNameT ownerName = (SaImmAdminOwnerNameT)__FUNCTION__;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FILE__;
	pthread_t threadid;
	SaNameT dn;
	SaNameT val;
	const SaNameT *objects[2] = { &dn, NULL };
	SaImmAdminOperationParamsT_2 param = {
			"attr8",
			SA_IMM_ATTR_SANAMET,
			&val
	};
	const SaImmAdminOperationParamsT_2 *params[2] = { &param, NULL };
	SaImmAdminOperationParamsT_2 **returnParams;

	osaf_extended_name_lend(longDn, &dn);
	osaf_extended_name_lend(longDn2, &val);

	setupOiLongDn();

	safassert(saImmOiInitialize_2(&immOiHandle, &localImmOiCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
	safassert(saImmOiClassImplementerSet(immOiHandle, configClassName), SA_AIS_OK);

	s_dispatch_status = S_DISP_START;
	pipe_stop_fd();
	pthread_create(&threadid, NULL, dispatchThread, &immOiHandle);

	while(s_dispatch_status != S_DISP_READY) {
		usleep(100);
	}

	safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immOmHandle, ownerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet(ownerHandle, objects, SA_IMM_ONE), SA_AIS_OK);

	rc = saImmOmAdminOperationInvoke_o2(ownerHandle, &dn, 1, 1, params, &result, 10, &returnParams);

	if(rc == SA_AIS_OK) {
		assert(returnParams);
		assert(returnParams[0]);
		assert(!strcmp(returnParams[0]->paramName, "attr8"));
		SaNameT *val = (SaNameT *)returnParams[0]->paramBuffer;
		assert(!strcmp(osaf_extended_name_borrow(val), longDn2));
		saImmOmAdminOperationMemoryFree(ownerHandle, returnParams);
	}

	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

	safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);

	// Wait for thread to exit
	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

	test_validate(rc, SA_AIS_OK);

	cleanOiLongDn();
}

static void saImmOiLongDn_07(void) {
	SaAisErrorT rc;
	SaImmOiHandleT immOiHandle;
	const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT)__FILE__;
	SaNameT dn;

	osaf_extended_name_lend(longDn, &dn);

	setupOiLongDn();

	safassert(saImmOiInitialize_2(&immOiHandle, &localImmOiCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

	rc = saImmOiObjectImplementerSet(immOiHandle, &dn, SA_IMM_ONE);
	if(rc == SA_AIS_OK) {
		rc = saImmOiObjectImplementerRelease(immOiHandle, &dn, SA_IMM_ONE);
	}

	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

	test_validate(rc, SA_AIS_OK);

	cleanOiLongDn();
}

void addOiLongDnTestCases() {
	test_case_add(7, saImmOiLongDn_01, "SA_AIS_OK - Object create callback");
	test_case_add(7, saImmOiLongDn_02, "SA_AIS_OK - Object modify callback");
	test_case_add(7, saImmOiLongDn_03, "SA_AIS_OK - Object delete callback");
	test_case_add(7, saImmOiLongDn_04, "SA_AIS_OK - Rt Object create and delete");
	test_case_add(7, saImmOiLongDn_05, "SA_AIS_OK - Rt Object update");
	test_case_add(7, saImmOiLongDn_06, "SA_AIS_OK - Admin operation callback");
	test_case_add(7, saImmOiLongDn_07, "SA_AIS_OK - saImmOiObjectImplementerSet");
}

__attribute__ ((constructor)) static void saImmOiLongDn_constructor(void)
{
	test_suite_add(7, "Long DN");
}
