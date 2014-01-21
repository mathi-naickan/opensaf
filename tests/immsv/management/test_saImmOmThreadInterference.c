/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2012 The OpenSAF Foundation
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
#include <poll.h>
#include <pthread.h>
#include "immtest.h"


static int isAdminOperDone;
static int isOmDone;
static int isOiDone;
static int isReady;
static SaNameT objectName;
static const SaNameT *objectNames[2] = { &objectName, NULL };
static const SaImmOiImplementerNameT implementerName = __FILE__;
static const SaImmAdminOwnerNameT adminOwnerName = __FILE__;


static void saImmOiAdminOperationCallback(SaImmOiHandleT immOiHandle, SaInvocationT invocation,
							const SaNameT *objectName, SaImmAdminOperationIdT operationId,
							const SaImmAdminOperationParamsT_2 **params) {
	isReady = 1;
	while(!isAdminOperDone)
		usleep(500);
	saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);
}

SaImmOiCallbacksT_2 immOiCallbacks = { saImmOiAdminOperationCallback , NULL , NULL , NULL , NULL , NULL , NULL , NULL };


static void *implementer_thread(void *arg) {
	struct pollfd fd[1];
	SaSelectionObjectT selObject;
	int ret;
	SaAisErrorT err;
	SaImmOiHandleT immOiHandle = *(SaImmOiHandleT *)arg;

	safassert(saImmOiSelectionObjectGet(immOiHandle, &selObject), SA_AIS_OK);

	fd[0].fd = (int)selObject;
	fd[0].events = POLLIN;

	isReady = 1;

	while(1) {
		ret = poll(fd, 1, 1000);
		if(ret == 1)
			if((err = saImmOiDispatch(immOiHandle, SA_DISPATCH_ONE)) != SA_AIS_OK)
				break;
	}

	isOiDone = 1;

	return NULL;
}

static void *lockomhandle_thread(void *arg) {
	SaImmAdminOwnerHandleT ownerHandle = *(SaImmAdminOwnerHandleT *)arg;
	const SaImmAdminOperationParamsT_2 *params[1] = { NULL };
	SaAisErrorT rc;

	/* "Lock" IMM OM handle */
	safassert(saImmOmAdminOperationInvoke_2(ownerHandle, &objectName, 0, 0, params, &rc, SA_TIME_ONE_HOUR), SA_AIS_OK);

	isOmDone = 1;

	return NULL;
}

void saImmOmThreadInterference_01(void) {
	SaImmHandleT immHandle;
	SaImmOiHandleT immOiHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmSearchHandleT searchHandle;
	SaImmCcbHandleT ccbHandle;
	pthread_t threadid1, threadid2;
	SaAisErrorT rc;
	SaImmAttrDefinitionT_2 attrDef = { "rdn", SA_IMM_ATTR_SASTRINGT, SA_IMM_ATTR_RDN | SA_IMM_ATTR_CONFIG, NULL };
	const SaImmAttrDefinitionT_2 *attrDefs[2] = { &attrDef, NULL };
	SaImmClassCategoryT classCategory;
	SaImmAttrDefinitionT_2 **attrDefinitions;
	SaImmAccessorHandleT accessorHandle;
	SaImmAttrNameT attributeNames[2] = { "SaImmAttrClassName", NULL};
	SaImmAttrValuesT_2 **attributes;
	SaImmAttrValueT objNames[2] = { &objectName, NULL };
	SaImmAttrValuesT_2 attrValue = { (SaImmAttrNameT)"rdn", SA_IMM_ATTR_SANAMET, 1, (SaImmAttrValueT *)objNames };
	const SaImmAttrValuesT_2 *attrValues[2] = { &attrValue, NULL };
	SaImmAttrValueT modAttrValue[2] = { "Test", NULL };
	const SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE, { "attr4", SA_IMM_ATTR_SASTRINGT, 1, modAttrValue } };
	const SaImmAttrModificationT_2 *attrMods[] = { &attrMod, NULL };

	objectName.length = strlen("Obj1");
	strcpy((char *)objectName.value, "Obj1");

	/* management */
	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	/* create test object */
	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
	safassert(saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues), SA_AIS_OK);
	safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);

	/* implementer */
	isReady = 0;
	isOiDone = 0;
	safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
	safassert(saImmOiObjectImplementerSet(immOiHandle, &objectName, SA_IMM_ONE), SA_AIS_OK);
	assert(!pthread_create(&threadid1, NULL, implementer_thread, &immOiHandle));

	while(!isReady)
		usleep(100);

	/* "lock" IMM OM handle */
	isReady = 0;
	isOmDone = 0;
	isAdminOperDone = 0;
	assert(!pthread_create(&threadid2, NULL, lockomhandle_thread, &ownerHandle));

	while(!isReady)
		usleep(100);

	/* OM function tests */
	if((rc = saImmOmFinalize(immHandle)) !=  SA_AIS_ERR_LIBRARY) goto done;

	if((rc = saImmOmClassCreate_2(immHandle, (const SaImmClassNameT)"TestClass", SA_IMM_CLASS_CONFIG, (const SaImmAttrDefinitionT_2 **)attrDefs)) !=  SA_AIS_ERR_LIBRARY) goto done;
	if((rc = saImmOmClassDescriptionGet_2(immHandle, (const SaImmClassNameT)"TestClass", &classCategory, &attrDefinitions)) !=  SA_AIS_ERR_LIBRARY) goto done;
	if((rc = saImmOmClassDelete(immHandle, (const SaImmClassNameT)"TestClass")) !=  SA_AIS_ERR_LIBRARY) goto done;

	if((rc = saImmOmSearchInitialize_2(immHandle, NULL, SA_IMM_SUBTREE,
			SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_NO_ATTR, NULL, NULL, &searchHandle)) !=  SA_AIS_ERR_LIBRARY) goto done;

	safassert(saImmOmAccessorInitialize(immHandle, &accessorHandle), SA_AIS_OK);
	if((rc = saImmOmAccessorGet_2(accessorHandle, &objectName, attributeNames, &attributes)) !=  SA_AIS_ERR_LIBRARY) goto done;
	safassert(saImmOmAccessorFinalize(accessorHandle), SA_AIS_OK);

	if((rc = saImmOmCcbObjectModify_2(ccbHandle, &objectName, attrMods)) !=  SA_AIS_ERR_LIBRARY) goto done;
	if((rc = saImmOmCcbObjectDelete(ccbHandle, &objectName)) !=  SA_AIS_ERR_LIBRARY) goto done;

done:
	isAdminOperDone = 1;
	while(!isOmDone)
		usleep(200);

	test_validate(rc, SA_AIS_ERR_LIBRARY);

	/* Finalize OI handle */
	safassert(saImmOiObjectImplementerRelease(immOiHandle, &objectName, SA_IMM_ONE), SA_AIS_OK);
	safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

	/* Use pthread_detach() to remove pthread_create@@GLIBC leak from valgrind */
	pthread_detach(threadid1);
	pthread_detach(threadid2);

	while(!isOiDone)
		usleep(200);

	/* Remove test object and test class */
	safassert(saImmOmCcbObjectDelete(ccbHandle, &objectName), SA_AIS_OK);
	safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

	/* Finalize OM handles */
	saImmOmCcbFinalize(ccbHandle);
	saImmOmAdminOwnerRelease(ownerHandle, objectNames, SA_IMM_ONE);
	saImmOmAdminOwnerFinalize(ownerHandle);
	saImmOmFinalize(immHandle);
}
