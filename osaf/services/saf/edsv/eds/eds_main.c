/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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
 * Author(s): Wind River Systems
 *
 */

#include <ncssysf_tsk.h>
#include <logtrace.h>

#include "eds.h"

static int __init_eds(void)
{
	NCS_LIB_REQ_INFO lib_create;

	/* Init LIB_CREATE request for Server */
	memset(&lib_create, 0, sizeof(lib_create));
	lib_create.i_op = NCS_LIB_REQ_CREATE;
	lib_create.info.create.argc = 0;
	lib_create.info.create.argv = NULL;

	if (ncs_agents_startup() != NCSCC_RC_SUCCESS) {
		LOG_ER("Agents Startup Failed");
		return NCSCC_RC_FAILURE;
	}
	/* Init EDS */
	if (ncs_edsv_eds_lib_req(&lib_create) != NCSCC_RC_SUCCESS) {
		LOG_ER("EDS Library Request Failed");
		return NCSCC_RC_FAILURE;

	}

	return (NCSCC_RC_SUCCESS);
}

int main(int argc, char *argv[])
{
	EDS_CB *eds_cb = NULL;
	daemonize(argc, argv);

	if (__init_eds() != NCSCC_RC_SUCCESS) {
		LOG_ER("EDS Initialization failed");
		exit(EXIT_FAILURE);
	}
        
	if (NULL == (eds_cb = (EDS_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl))) {
		LOG_ER("Global take handle failed");
		exit(EXIT_FAILURE);
	}

	/* Give back the handle */
	ncshm_give_hdl(gl_eds_hdl);

	eds_main_process(&eds_cb->mbx);

	LOG_ER("eds_main_process() returned ..Exiting");

	exit(1);
}
