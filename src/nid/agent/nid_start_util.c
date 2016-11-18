/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2009 The OpenSAF Foundation
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
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "osaf/saf/saAis.h"
#include "base/logtrace.h"
#include "base/ncsgl_defs.h"

/**
 * Get AMF component name from file. File name found in
 * environment variable. Set standard environment variable with
 * value.
 * 
 * @param env_name - name of environment variable with file name
 * @param dn [out]
 * 
 * @return unsigned int
 */
unsigned int amf_comp_name_get_set_from_file(const char *env_name, SaNameT *dn)
{
	unsigned int rc = NCSCC_RC_FAILURE;
	char comp_name[257] = { 0 };
	FILE *fp;
	char *comp_name_file;

	if ((comp_name_file = getenv(env_name)) == NULL) {
		LOG_ER("getenv '%s' failed", env_name);
		goto done;
	}

	if ((fp = fopen(comp_name_file, "r")) == NULL) {
		LOG_ER("fopen '%s' failed - %s", comp_name_file, strerror(errno));
		goto done;
	}

	if (fscanf(fp, "%256s", comp_name) != 1) {
		(void)fclose(fp);
		LOG_ER("Unable to retrieve component name from file '%s'", comp_name_file);
		goto done;
	}

	if (setenv("SA_AMF_COMPONENT_NAME", comp_name, 1) == -1) {
		fclose(fp);
		LOG_ER("setenv failed - %s", strerror(errno));
		goto done;
	}

	if (fclose(fp) != 0) {
		LOG_ER("fclose failed - %s", strerror(errno));
		goto done;
	}

	dn->length = strlen(comp_name);
	if (dn->length <= SA_MAX_NAME_LENGTH) {
		strcpy((char *)dn->value, comp_name);
	} else {
		LOG_ER("Comp name too long %u", dn->length);
		/* SA_MAX_NAME_LENGTH is an arbitrary length delimiter in this 
		   case. On the other hand, it should be long enough for all
		   reasonable comp names */
		dn->length = 0;
		goto done;
	}

	rc = NCSCC_RC_SUCCESS;

 done:
	return rc;
}
