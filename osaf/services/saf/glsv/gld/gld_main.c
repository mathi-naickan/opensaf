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

#include "gld.h"
extern void gld_main_process(SYSF_MBX *);
static int __init_gld(void)
{
	NCS_LIB_REQ_INFO lib_create;

	/* Init LIB_CREATE request for Server */
	memset(&lib_create, 0, sizeof(lib_create));
	lib_create.i_op = NCS_LIB_REQ_CREATE;
	lib_create.info.create.argc = 0;
	lib_create.info.create.argv = NULL;

	if (ncs_agents_startup() != NCSCC_RC_SUCCESS)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	/* Init GLD */
	TRACE("GLSV:GLD:ON");
	if (gld_lib_req(&lib_create) != NCSCC_RC_SUCCESS) {
		fprintf(stderr, "gld_lib_req FAILED\n");
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	
	return (NCSCC_RC_SUCCESS);
}

int main(int argc, char *argv[])
{
	GLSV_GLD_CB *gld_cb;
	daemonize(argc, argv);

	if (__init_gld() != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "__init_gld() failed");
		exit(EXIT_FAILURE);
	}

	if ((gld_cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl))== NULL) {
		LOG_ER("cb take hdl failed");
		exit(EXIT_FAILURE);
	} 
	ncshm_give_hdl(gl_gld_hdl);

	gld_main_process(&gld_cb->mbx);

	LOG_ER("GLD is exiting");

	exit(1);
}
