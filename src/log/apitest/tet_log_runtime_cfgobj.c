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

#include "logtest.h"
#include <limits.h>
#include <unistd.h>
#include "base/saf_error.h"
#include "osaf/immutil/immutil.h"

static SaVersionT immVersion = { 'A', 2, 11 };

/**
 * Log configuration config obj <=> runtime obj
 *
 * Verfy that the configuration and runtime object for log server configuration
 * data contains the same number of attributes
 */
void log_rt_cf_obj_compare(void)
{
	SaImmHandleT omHandle;
	SaNameT object_name;
	SaImmAccessorHandleT accessorHandle;
	SaImmAttrValuesT_2 **attributes;
	uint32_t r_cnt = 0; /* Counter for attributes in runtime object */
	uint32_t c_cnt = 0;
	SaAisErrorT ais_rc = SA_AIS_OK;
	int tst_res = 0; /* Test result: 0 = PASS */

	/* NOTE: immutil will osaf_assert if error */
	(void) immutil_saImmOmInitialize(&omHandle, NULL, &immVersion);
	(void) immutil_saImmOmAccessorInitialize(omHandle, &accessorHandle);

	/* Count attributes in configuration object
	 */
	saAisNameLend(LOGTST_IMM_LOG_CONFIGURATION, &object_name);
	ais_rc = immutil_saImmOmAccessorGet_2(accessorHandle, &object_name, NULL,
			&attributes);
	if (ais_rc != SA_AIS_OK) {
		tst_res = 1; /* FAIL */
		fprintf(stderr, "Could not read config attributes %s\n",
			saf_error(ais_rc));
		goto done;
	}

	c_cnt = 0;
	while (attributes[c_cnt++] != NULL); /* Count the attributes */

	/* Count attributes in runtime object
	 */
	saAisNameLend(LOGTST_IMM_LOG_RUNTIME, &object_name);

	ais_rc = immutil_saImmOmAccessorGet_2(accessorHandle, &object_name, NULL,
			&attributes);
	if (ais_rc != SA_AIS_OK) {
		tst_res = 1; /* FAIL */
		fprintf(stderr, "Could not read runtime attributes %s\n",
			saf_error(ais_rc));
		goto done;
	}

	r_cnt = 0;
	while (attributes[r_cnt++] != NULL); /* Count the attributes */

	/* Compare number of attributes. Test pass if the same number
	 */
	if (c_cnt != r_cnt) {
		tst_res = 1;
		fprintf(stderr, "Found %d configuration attributes and"
		" %d runtime attributes\n", c_cnt, r_cnt);
	}

	done:

	(void) immutil_saImmOmAccessorFinalize(accessorHandle);
	(void) immutil_saImmOmFinalize(omHandle);

	rc_validate(tst_res, 0);
}


/* Load tests */
__attribute__ ((constructor)) static void saOiOperations_constructor(void)
{
	test_suite_add(8, "Log configuration runtime object tests");
	test_case_add(8, log_rt_cf_obj_compare, "Log configuration config obj <=> runtime obj");
}
