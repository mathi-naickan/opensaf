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
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <stdlib.h>

void saImmOiImplementerSet_01(void)
{
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    test_validate(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiImplementerSet_02(void)
{
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    test_validate(saImmOiImplementerSet(-1, implementerName), SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiImplementerSet_03(void)
{
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaImmOiHandleT newHandle;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

    safassert(saImmOiInitialize_2(&newHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    test_validate(saImmOiImplementerSet(newHandle, implementerName), SA_AIS_ERR_EXIST);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
    safassert(saImmOiFinalize(newHandle), SA_AIS_OK);
}

void saImmOiImplementerSet_04(void)
{
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaImmOiImplementerNameT implementerName2 = (SaImmOiImplementerNameT) "MuddyWaters";

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

    test_validate(saImmOiImplementerSet(immOiHandle, implementerName2), SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiImplementerSet_05(void)
{
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) "\001";/* Bad implementer name */

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    test_validate(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

int saImmOiImplementerSet_callback_wait = 15;
SaImmClassNameT saImmOiImplementerSet_className = NULL;

static SaAisErrorT saImmOiImplementerSet_ModifyCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
		const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods) {

	sleep(saImmOiImplementerSet_callback_wait);
	return SA_AIS_OK;
}

static SaAisErrorT saImmOiImplementerSet_RtAttrUpdateCallbackT(SaImmOiHandleT immOiHandle, const SaNameT *objectName, const SaImmAttrNameT *attributeNames) {
	SaUint32T attrVal = (SaUint32T)random();
	SaImmAttrValueT attrValues[1] = { &attrVal };
	SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE, { "attr1", SA_IMM_ATTR_SAUINT32T, 1, attrValues } };
	const SaImmAttrModificationT_2 *attrMods[2] = { &attrMod, NULL };

	sleep(saImmOiImplementerSet_callback_wait);

	saImmOiRtObjectUpdate_2(immOiHandle, objectName, attrMods);

	return SA_AIS_OK;
}

void *saImmOiImplementerSet_modify_thread(void *arg) {
	int *ready = (int *)arg;
	SaImmOiHandleT immOiHandle;
	SaSelectionObjectT selObj;
	SaImmOiCallbacksT_2 configImmOiCallbacks = { NULL , NULL , NULL , NULL , NULL , NULL , saImmOiImplementerSet_ModifyCallback , NULL };
	SaImmOiCallbacksT_2 rtImmOiCallbacks = { NULL , NULL , NULL , NULL , NULL , NULL , NULL , saImmOiImplementerSet_RtAttrUpdateCallbackT };
	SaNameT rdn = { 5, "obj=1" };
	struct pollfd fds[2];
	int rc = 1;
	int config = 1;

	assert(saImmOiImplementerSet_className != NULL);

	if(!strcmp(saImmOiImplementerSet_className, runtimeClassName)) {
		SaImmAttrValueT attrVal[1] = { &rdn };
		SaImmAttrValuesT_2 rdnVal = { "rdn", SA_IMM_ATTR_SANAMET, 1, attrVal };
		const SaImmAttrValuesT_2 *attrValues[2] = { &rdnVal, NULL };

		safassert(saImmOiInitialize_2(&immOiHandle, &rtImmOiCallbacks, &immVersion), SA_AIS_OK);
		safassert(saImmOiSelectionObjectGet(immOiHandle, &selObj), SA_AIS_OK);
		safassert(saImmOiImplementerSet(immOiHandle, (SaImmOiImplementerNameT)__FUNCTION__), SA_AIS_OK);
		safassert(saImmOiRtObjectCreate_2(immOiHandle, runtimeClassName, NULL, attrValues), SA_AIS_OK);
		config = 0;
	} else {
		safassert(saImmOiInitialize_2(&immOiHandle, &configImmOiCallbacks, &immVersion), SA_AIS_OK);
		safassert(saImmOiSelectionObjectGet(immOiHandle, &selObj), SA_AIS_OK);
		safassert(saImmOiImplementerSet(immOiHandle, (SaImmOiImplementerNameT)__FUNCTION__), SA_AIS_OK);
		safassert(saImmOiClassImplementerSet(immOiHandle, saImmOiImplementerSet_className), SA_AIS_OK);
	}

	fds[0].fd = (int) selObj;
	fds[0].events = POLLIN;
	fds[1].fd = stopFd[0];
	fds[1].events = POLLIN;
	*ready = 1;

	while (1) {
		rc = poll(fds, 2, -1);
		if (rc == -1)
			fprintf(stderr, "poll error: %s\n", strerror(errno));

		if (fds[0].revents & POLLIN)
			safassert(saImmOiDispatch(immOiHandle, SA_DISPATCH_ONE), SA_AIS_OK);

		if (fds[1].revents & POLLIN)
			break;
	}

	if(!config) {
		safassert(saImmOiRtObjectDelete(immOiHandle, &rdn), SA_AIS_OK);
	} else {
		safassert(saImmOiClassImplementerRelease(immOiHandle, saImmOiImplementerSet_className), SA_AIS_OK);
	}

	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

	return NULL;
}

void saImmOiImplementerSet_06(void)
{
	SaImmHandleT immHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaNameT obj1 = { 5, "obj=1" };
	pthread_t threadid;
	int ready = 0;
	SaUint32T val = 123;
	SaImmAttrValueT attrVal[1] = { &val };
	SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE, { "attr1" , SA_IMM_ATTR_SAUINT32T, 1, attrVal } };
	SaImmAttrModificationT_2 *attrMods[2] = { &attrMod, NULL };

	saImmOiImplementerSet_callback_wait = 13;
	saImmOiImplementerSet_className = configClassName;

	assert(setenv("IMMA_SYNCR_TIMEOUT", "2000", 1) == 0);
	assert(setenv("IMMA_OI_CALLBACK_TIMEOUT", "15", 1) == 0);

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immHandle, (SaImmAdminOwnerNameT)__FUNCTION__, SA_TRUE, &ownerHandle), SA_AIS_OK);
	config_class_create(immHandle);
	safassert(object_create(immHandle, ownerHandle, configClassName, &obj1, NULL, NULL), SA_AIS_OK);

	pipe_stop_fd();
	assert(!pthread_create(&threadid, NULL, saImmOiImplementerSet_modify_thread, &ready));

	while(!ready) {
		usleep(100000);
	}

	safassert(saImmOmCcbInitialize(ownerHandle, SA_IMM_CCB_REGISTERED_OI, &ccbHandle), SA_AIS_OK);

	SaAisErrorT rc = saImmOmCcbObjectModify_2(ccbHandle, &obj1, (const SaImmAttrModificationT_2 **)attrMods);
	test_validate(rc, SA_AIS_OK);

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(object_delete(ownerHandle, &obj1, 0), SA_AIS_OK);
	config_class_delete(immHandle);

	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);

	unsetenv("IMMA_SYNCR_TIMEOUT");
	unsetenv("IMMA_OI_CALLBACK_TIMEOUT");
}

void saImmOiImplementerSet_07(void)
{
	SaImmHandleT immHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaNameT obj1 = { 5, "obj=1" };
	pthread_t threadid;
	int ready = 0;
	SaUint32T val = 123;
	SaImmAttrValueT attrVal[1] = { &val };
	SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE, { "attr1" , SA_IMM_ATTR_SAUINT32T, 1, attrVal } };
	SaImmAttrModificationT_2 *attrMods[2] = { &attrMod, NULL };

	saImmOiImplementerSet_callback_wait = 18;
	saImmOiImplementerSet_className = configClassName;

	assert(setenv("IMMA_SYNCR_TIMEOUT", "2000", 1) == 0);
	assert(setenv("IMMA_OI_CALLBACK_TIMEOUT", "15", 1) == 0);

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immHandle, (SaImmAdminOwnerNameT)__FUNCTION__, SA_TRUE, &ownerHandle), SA_AIS_OK);
	config_class_create(immHandle);
	safassert(object_create(immHandle, ownerHandle, configClassName, &obj1, NULL, NULL), SA_AIS_OK);

	pipe_stop_fd();
	assert(!pthread_create(&threadid, NULL, saImmOiImplementerSet_modify_thread, &ready));

	while(!ready) {
		usleep(100000);
	}

	safassert(saImmOmCcbInitialize(ownerHandle, SA_IMM_CCB_REGISTERED_OI, &ccbHandle), SA_AIS_OK);

	SaAisErrorT rc = saImmOmCcbObjectModify_2(ccbHandle, &obj1, (const SaImmAttrModificationT_2 **)attrMods);
	test_validate(rc, SA_AIS_ERR_FAILED_OPERATION);

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	const SaNameT *objectNames[2] = { &obj1, NULL };
	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immHandle, (SaImmAdminOwnerNameT)__FUNCTION__, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
	safassert(object_delete(ownerHandle, &obj1, 0), SA_AIS_OK);
	config_class_delete(immHandle);

	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);

	unsetenv("IMMA_SYNCR_TIMEOUT");
	unsetenv("IMMA_OI_CALLBACK_TIMEOUT");
}

void saImmOiImplementerSet_08(void)
{
	SaImmHandleT immHandle;
	SaNameT obj1 = { 5, "obj=1" };
	pthread_t threadid;
	int ready = 0;
	SaImmAccessorHandleT accessorHandle;
	SaImmAttrValuesT_2 **attributes;

	saImmOiImplementerSet_callback_wait = 13;
	saImmOiImplementerSet_className = runtimeClassName;

	assert(setenv("IMMA_SYNCR_TIMEOUT", "2000", 1) == 0);
	assert(setenv("IMMA_OI_CALLBACK_TIMEOUT", "15", 1) == 0);

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);

	pipe_stop_fd();
	assert(!pthread_create(&threadid, NULL, saImmOiImplementerSet_modify_thread, &ready));

	while(!ready) {
		usleep(100000);
	}

	safassert(saImmOmAccessorInitialize(immHandle, &accessorHandle), SA_AIS_OK);
	rc = saImmOmAccessorGet_2(accessorHandle, &obj1, NULL, &attributes);
	test_validate(rc, SA_AIS_OK);

	safassert(saImmOmAccessorFinalize(accessorHandle), SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);

	unsetenv("IMMA_SYNCR_TIMEOUT");
	unsetenv("IMMA_OI_CALLBACK_TIMEOUT");
}

void saImmOiImplementerSet_09(void)
{
	SaImmHandleT immHandle;
	SaNameT obj1 = { 5, "obj=1" };
	pthread_t threadid;
	int ready = 0;
	SaImmAccessorHandleT accessorHandle;
	SaImmAttrValuesT_2 **attributes;

	saImmOiImplementerSet_callback_wait = 18;
	saImmOiImplementerSet_className = runtimeClassName;

	assert(setenv("IMMA_SYNCR_TIMEOUT", "2000", 1) == 0);
	assert(setenv("IMMA_OI_CALLBACK_TIMEOUT", "15", 1) == 0);

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);

	pipe_stop_fd();
	assert(!pthread_create(&threadid, NULL, saImmOiImplementerSet_modify_thread, &ready));

	while(!ready) {
		usleep(100000);
	}

	safassert(saImmOmAccessorInitialize(immHandle, &accessorHandle), SA_AIS_OK);
	rc = saImmOmAccessorGet_2(accessorHandle, &obj1, NULL, &attributes);
	test_validate(rc, SA_AIS_ERR_TIMEOUT);

	safassert(saImmOmAccessorFinalize(accessorHandle), SA_AIS_OK);

	indicate_stop_fd();
	pthread_join(threadid, NULL);
	close_stop_fd();

	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);

	unsetenv("IMMA_SYNCR_TIMEOUT");
	unsetenv("IMMA_OI_CALLBACK_TIMEOUT");
}

void saImmOiImplementerSet_10(void)
{
	SaImmHandleT immHandle;
	SaImmOiHandleT immOiHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaAisErrorT err;
	SaImmOiCallbacksT_2 configImmOiCallbacks = { NULL , NULL , NULL , NULL , NULL , NULL , saImmOiImplementerSet_ModifyCallback , NULL };

	const SaNameT immObj = { strlen("opensafImm=opensafImm,safApp=safImmService"),
			"opensafImm=opensafImm,safApp=safImmService" };
	const SaNameT *immObjs[2] = { &immObj, NULL };
	SaUint32T paramVal = 16;
	SaImmAdminOperationParamsT_2 param = { "opensafImmNostdFlags", SA_IMM_ATTR_SAUINT32T, &paramVal };
	const SaImmAdminOperationParamsT_2 *params[2] = { &param, NULL };
	SaNameT obj1 = { 5, "obj=1" };


	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);

	// Disable OpenSAF 4.5 features
	safassert(saImmOmAdminOwnerInitialize(immHandle, "safImmService", SA_FALSE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet(ownerHandle, immObjs, SA_IMM_ONE), SA_AIS_OK);
	safassert(saImmOmAdminOperationInvoke_2(ownerHandle, &immObj, 1, 2, params, &err, 100), SA_AIS_OK);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);

	safassert(saImmOmAdminOwnerInitialize(immHandle, (const SaImmAdminOwnerNameT)__FUNCTION__, SA_TRUE, &ownerHandle), SA_AIS_OK);
	config_class_create(immHandle);
	safassert(object_create(immHandle, ownerHandle, configClassName, &obj1, NULL, NULL), SA_AIS_OK);

	assert(setenv("IMMA_OI_CALLBACK_TIMEOUT", "15", 1) == 0);

	safassert(saImmOiInitialize_2(&immOiHandle, &configImmOiCallbacks, &immVersion), SA_AIS_OK);
	test_validate(saImmOiImplementerSet(immOiHandle, (const SaImmOiImplementerNameT)__FUNCTION__), SA_AIS_ERR_NO_RESOURCES);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

	safassert(object_delete(ownerHandle, &obj1, 0), SA_AIS_OK);
	config_class_delete(immHandle);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);

	// Enable OpenSAF 4.5 features
	safassert(saImmOmAdminOwnerInitialize(immHandle, "safImmService", SA_FALSE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet(ownerHandle, immObjs, SA_IMM_ONE), SA_AIS_OK);
	safassert(saImmOmAdminOperationInvoke_2(ownerHandle, &immObj, 1, 1, params, &err, 100), SA_AIS_OK);
	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);

	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);
}


extern void saImmOiImplementerClear_01(void);
extern void saImmOiImplementerClear_02(void);
extern void saImmOiClassImplementerSet_01(void);
extern void saImmOiClassImplementerSet_02(void);
extern void saImmOiClassImplementerSet_03(void);
extern void saImmOiClassImplementerSet_04(void);
extern void saImmOiClassImplementerSet_05(void);
extern void saImmOiClassImplementerSet_06(void);
extern void saImmOiClassImplementerRelease_01(void);
extern void saImmOiClassImplementerRelease_02(void);
extern void saImmOiClassImplementerRelease_03(void);
extern void saImmOiObjectImplementerSet_01(void);
extern void saImmOiObjectImplementerSet_02(void);
extern void saImmOiObjectImplementerSet_03(void);
extern void saImmOiObjectImplementerSet_04(void);
extern void saImmOiObjectImplementerSet_05(void);
extern void saImmOiObjectImplementerSet_06(void);
extern void saImmOiObjectImplementerSet_07(void);
extern void saImmOiObjectImplementerRelease_01(void);
extern void saImmOiObjectImplementerRelease_02(void);
extern void saImmOiObjectImplementerRelease_03(void);
extern void saImmOiObjectImplementerRelease_04(void);

__attribute__ ((constructor)) static void saImmOiImplementerSet_constructor(void)
{
    test_suite_add(2, "Object Implementer");
    test_case_add(2, saImmOiImplementerSet_01, "saImmOiImplementerSet - SA_AIS_OK");
    test_case_add(2, saImmOiImplementerSet_02, "saImmOiImplementerSet - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(2, saImmOiImplementerSet_03, "saImmOiImplementerSet - SA_AIS_ERR_EXIST - som other handle already uses name");
    test_case_add(2, saImmOiImplementerSet_04, "saImmOiImplementerSet - SA_AIS_ERR_INVALID_PARAM - handle already associated with name");

    test_case_add(2, saImmOiImplementerClear_01, "saImmOiImplementerClear - SA_AIS_OK");
    test_case_add(2, saImmOiImplementerClear_02, "saImmOiImplementerClear - SA_AIS_ERR_BAD_HANDLE");

    test_case_add(2, saImmOiClassImplementerSet_01, "saImmOiClassImplementerSet - SA_AIS_OK");
    test_case_add(2, saImmOiClassImplementerSet_02, "saImmOiClassImplementerSet - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(2, saImmOiClassImplementerSet_03, "saImmOiClassImplementerSet - SA_AIS_ERR_BAD_OPERATION");
    test_case_add(2, saImmOiClassImplementerSet_04, "saImmOiClassImplementerSet - SA_AIS_ERR_NOT_EXIST");
    test_case_add(2, saImmOiClassImplementerSet_05, "saImmOiClassImplementerSet - SA_AIS_ERR_EXIST");
    test_case_add(2, saImmOiClassImplementerSet_06, "saImmOiClassImplementerSet - SA_AIS_ERR_EXIST - object implementer already set");

    test_case_add(2, saImmOiClassImplementerRelease_01, "saImmOiClassImplementerRelease - SA_AIS_OK");
    test_case_add(2, saImmOiClassImplementerRelease_02, "saImmOiClassImplementerRelease - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(2, saImmOiClassImplementerRelease_03, "saImmOiClassImplementerRelease - SA_AIS_ERR_NOT_EXIST");

    test_case_add(2, saImmOiObjectImplementerSet_01, "saImmOiObjectImplementerSet - SA_AIS_OK");
    test_case_add(2, saImmOiObjectImplementerSet_02, "saImmOiObjectImplementerSet - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(2, saImmOiObjectImplementerSet_03, "saImmOiObjectImplementerSet - SA_AIS_ERR_BAD_HANDLE - handle not associated with implementer name");
    test_case_add(2, saImmOiObjectImplementerSet_04, "saImmOiObjectImplementerSet - SA_AIS_ERR_NOT_EXIST - objectName non existing");
    test_case_add(2, saImmOiObjectImplementerSet_05, "saImmOiObjectImplementerSet - SA_AIS_ERR_EXIST - objectName already has implementer");
    test_case_add(2, saImmOiObjectImplementerSet_06, "saImmOiObjectImplementerSet - SA_AIS_ERR_EXIST - class already has implementer");

    test_case_add(2, saImmOiObjectImplementerRelease_01, "saImmOiObjectImplementerRelease - SA_AIS_OK");
    test_case_add(2, saImmOiObjectImplementerRelease_02, "saImmOiObjectImplementerRelease - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(2, saImmOiObjectImplementerRelease_03, "saImmOiObjectImplementerRelease - SA_AIS_ERR_NOT_EXIST - objectName non existing");
    test_case_add(2, saImmOiObjectImplementerRelease_04, "saImmOiObjectImplementerRelease - SA_AIS_ERR_INVALID_PARAM - invalid scope");

    test_case_add(2, saImmOiImplementerSet_05, "saImmOiImplementerSet - SA_AIS_ERR_INVALID_PARAM - Bad implementer name");
    test_case_add(2, saImmOiObjectImplementerSet_07, "saImmOiObjectImplementerSet - SA_AIS_ERR_INVALID_PARAM - empty object name with wrong size");

    test_case_add(2, saImmOiImplementerSet_06, "saImmOiImplementerSet - SA_AIS_OK - OI callback timeout");
    test_case_add(2, saImmOiImplementerSet_07, "saImmOiImplementerSet - SA_AIS_ERR_FAILED_OPERATION - OI callback timeout");
    test_case_add(2, saImmOiImplementerSet_08, "saImmOiImplementerSet - SA_AIS_OK - RTA update callback timeout");
    test_case_add(2, saImmOiImplementerSet_09, "saImmOiImplementerSet - SA_AIS_ERR_TIMEOUT - RTA update callback timeout");
    test_case_add(2, saImmOiImplementerSet_10, "saImmOiImplementerSet - SA_AIS_ERR_NO_RESOURCES - OpenSAF 4.5 features disabled");
}


