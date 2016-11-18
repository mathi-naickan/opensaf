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

#include "imm/apitest/immtest.h"
#include "base/osaf_extended_name.h"
#include "osaf/immutil/immutil.h"


static char *objects[] = {
		"Obj1",
		"Obj3",
		"Obj1,rdn=root",
		"saImmOmCcbObjectCreate_06",
		"safLgStrCfg=saLogAlarm\\,safApp=safLogService\\,safApp=safImmService",
		"id=1",
		"id=2",
		"id=3",
		"obj=1,rdn=root",
		"rdn=root",
		"longdn=012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789",
		NULL
};

static char *classes[] = {
		"TestClassConfig",
		"TestClassRuntime",
		"TestClassNoDangling",
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
		"TestClass",
		NULL
};

extern struct ImmutilWrapperProfile immutilWrapperProfile;
extern void addOmLongDnTestCases();

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
	rc = immutil_saImmOmInitialize(&immHandle, NULL, &version);
	assert(rc == SA_AIS_OK);

	rc = immutil_saImmOmAdminOwnerInitialize(immHandle, "immoitest", SA_TRUE, &ownerHandle);
	assert(rc == SA_AIS_OK);

	rc = immutil_saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle);
	assert(rc == SA_AIS_OK);

	/* Delete objects */
	SaNameT objectName = {0};
	SaNameT *objectNames[2] = { &objectName, NULL };
	obj = objects;
	while(*obj) {
		if(osaf_is_extended_names_enabled() || strlen(*obj) < SA_MAX_NAME_LENGTH) {
			osaf_extended_name_lend(*obj, &objectName);

			rc = immutil_saImmOmAdminOwnerSet(ownerHandle, (const SaNameT **)objectNames, SA_IMM_SUBTREE);
			if(rc == SA_AIS_ERR_NOT_EXIST) {
				obj++;
				continue;
			}
			assert(rc == SA_AIS_OK);

			rc = immutil_saImmOmCcbObjectDelete(ccbHandle, &objectName);
			if(rc != SA_AIS_OK && rc != SA_AIS_ERR_NOT_EXIST)
				fprintf(stderr, "Failed to delete object '%s' with error code: %d\n", *obj, rc);
			assert(rc == SA_AIS_OK || rc == SA_AIS_ERR_NOT_EXIST);
		}

		obj++;
	}

	if(*objects) {
		rc = immutil_saImmOmCcbApply(ccbHandle);
		assert(rc == SA_AIS_OK);
	}

	/* Close Ccb handle */
	rc = immutil_saImmOmCcbFinalize(ccbHandle);
	assert(rc == SA_AIS_OK);

	/* Delete classes */
	cls = classes;
	while(*cls) {
		rc = immutil_saImmOmClassDelete(immHandle, *cls);
		if(rc == SA_AIS_ERR_BUSY)
			fprintf(stderr, "Class '%s' contains object instances\n", *cls);
		assert(rc == SA_AIS_OK || rc == SA_AIS_ERR_NOT_EXIST);

		cls++;
	}

	/* Close handles */
	rc = immutil_saImmOmAdminOwnerFinalize(ownerHandle);
	assert(rc == SA_AIS_OK);

	rc = immutil_saImmOmFinalize(immHandle);
	assert(rc == SA_AIS_OK);

	TRACE_LEAVE();
}

static void startup() {
	TRACE_ENTER();

	/* First, remove all objects */
	cleanup();

	/* Create root object */
	const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
	SaImmAdminOwnerHandleT ownerHandle;

	safassert(immutil_saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(immutil_saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(config_class_create(immOmHandle), SA_AIS_OK);
	safassert(runtime_class_create(immOmHandle), SA_AIS_OK);
	safassert(object_create(immOmHandle, ownerHandle, configClassName, &rootObj, NULL, NULL), SA_AIS_OK);
	safassert(immutil_saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(immutil_saImmOmFinalize(immOmHandle), SA_AIS_OK);

	/* Add long DN test cases */
	char *env;
	if((env = getenv("SA_ENABLE_EXTENDED_NAMES"))) {
		if(!strcmp(env, "1")) {
			addOmLongDnTestCases();
		}
	}

	TRACE_LEAVE();
}

__attribute__ ((constructor)) static void cleanup_constructor(void)
{
	/* Disable exit on error*/
	immutilWrapperProfile.errorsAreFatal = 0;

	/* If an earlier test is aborted, then remained test objects and
	   test classes in IMM need to be cleaned up */
	test_setup = startup;

	/* On successful finished test, clean up IMM */
	test_cleanup = cleanup;
}


