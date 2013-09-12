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

#include "immtest.h"


static char *objects[] = {
		"Obj1",
		"Obj3",
		"Obj1,opensafImm=opensafImm,safApp=safImmService",
		"saImmOmCcbObjectCreate_06",
		"safLgStrCfg=saLogAlarm\\,safApp=safLogService\\,safApp=safImmService",
		NULL
};

static char *classes[] = {
		"TestClassConfig",
		"TestClassRuntime",
		"saImmOmCcbObjectCreate_07",
		"saImmOmClassCreate_2_01",
		"saImmOmClassCreate_2_02",
		"saImmOmClassCreate_2_12",
		"saImmOmClassCreate_2_13",
		"saImmOmClassDelete_2_01",
		"saImmOmClassDelete_2_02",
		"saImmOmClassDescriptionGet_2_01",
		"saImmOmClassDescriptionGet_2_02",
		"saImmOmClassDescriptionMemoryFree_2_01",
		"saImmOmClassDescriptionMemoryFree_2_02",
		"saImmOmClassCreate_2_14",
		"saImmOmClassDescriptionGet_2_04",
		NULL
};


static void cleanup() {
	SaVersionT version = { 'A', 2, 11 };
	SaImmHandleT immHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaAisErrorT rc;
	char **obj;
	char **cls;

	TRACE_ENTER();

	/* Initialize handles */
	rc = saImmOmInitialize(&immHandle, NULL, &version);
	assert(rc == SA_AIS_OK);

	rc = saImmOmAdminOwnerInitialize(immHandle, "immoitest", SA_TRUE, &ownerHandle);
	assert(rc == SA_AIS_OK);

	rc = saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle);
	assert(rc == SA_AIS_OK);

	/* Delete objects */
	int len;
	SaNameT objectName = {0};
	SaNameT *objectNames[2] = { &objectName, NULL };
	obj = objects;
	while(*obj) {
		len = strlen(*obj);
		objectName.length = (len > SA_MAX_NAME_LENGTH) ? SA_MAX_NAME_LENGTH : len;
		strncpy((char *)objectName.value, *obj, objectName.length);

		rc = saImmOmAdminOwnerSet(ownerHandle, (const SaNameT **)objectNames, SA_IMM_ONE);
		if(rc == SA_AIS_ERR_NOT_EXIST) {
			obj++;
			continue;
		}
		assert(rc == SA_AIS_OK);

		rc = saImmOmCcbObjectDelete(ccbHandle, &objectName);
		if(rc != SA_AIS_OK && rc != SA_AIS_ERR_NOT_EXIST)
			fprintf(stderr, "Failed to delete object '%s' with error code: %d\n", *obj, rc);
		assert(rc == SA_AIS_OK || rc == SA_AIS_ERR_NOT_EXIST);

		obj++;
	}

	if(*objects) {
		rc = saImmOmCcbApply(ccbHandle);
		assert(rc == SA_AIS_OK);
	}

	/* Close Ccb handle */
	rc = saImmOmCcbFinalize(ccbHandle);
	assert(rc == SA_AIS_OK);

	/* Delete classes */
	cls = classes;
	while(*cls) {
		rc = saImmOmClassDelete(immHandle, *cls);
		if(rc == SA_AIS_ERR_BUSY)
			fprintf(stderr, "Class '%s' contains object instances\n", *cls);
		assert(rc == SA_AIS_OK || rc == SA_AIS_ERR_NOT_EXIST);

		cls++;
	}

	/* Close handles */
	rc = saImmOmAdminOwnerFinalize(ownerHandle);
	assert(rc == SA_AIS_OK);

	rc = saImmOmFinalize(immHandle);
	assert(rc == SA_AIS_OK);

	TRACE_LEAVE();
}


__attribute__ ((constructor)) static void cleanup_constructor(void)
{
	/* If an earlier test is aborted, then remained test objects and
	   test classes in IMM need to be cleaned up */
	test_setup = cleanup;

	/* On successful finished test, clean up IMM */
	test_cleanup = cleanup;
}


