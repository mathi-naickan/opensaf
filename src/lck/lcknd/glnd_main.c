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

#include "base/ncssysf_tsk.h"

#include "base/logtrace.h"
#include "base/daemon.h"

#include "lck/lcknd/glnd.h"
extern void glnd_main_process(SYSF_MBX *);
static int __init_glnd(void)
{
	NCS_LIB_REQ_INFO lib_create;

	/* Init LIB_CREATE request for Server */
	memset(&lib_create, 0, sizeof(lib_create));
	lib_create.i_op = NCS_LIB_REQ_CREATE;
	lib_create.info.create.argc = 0;
	lib_create.info.create.argv = NULL;

	if (ncs_agents_startup() != NCSCC_RC_SUCCESS)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	/* Init GLND */
	TRACE("GLSV:GLND:ON");
	if (glnd_lib_req(&lib_create) != NCSCC_RC_SUCCESS) {
		fprintf(stderr, "glnd_lib_req FAILED\n");
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	
	return (NCSCC_RC_SUCCESS);
}

int main(int argc, char *argv[])
{
	GLND_CB *glnd_cb = NULL;
	uint32_t cb_hdl = 0;

	daemonize(argc, argv);

	if (__init_glnd() != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "__init_glnd() failed");
		exit(EXIT_FAILURE);
	}
        
	cb_hdl = m_GLND_RETRIEVE_GLND_CB_HDL; 

	/* Get the CB from the handle */
	glnd_cb = ncshm_take_hdl(NCS_SERVICE_ID_GLND, cb_hdl);

	if (!glnd_cb) {
		LOG_ER("cb take hdl failed");
		exit(EXIT_FAILURE);
	}

	/* giveup the handle */
	ncshm_give_hdl(cb_hdl);

	glnd_main_process(&glnd_cb->glnd_mbx);

	LOG_ER("exiting the select loop of GLND...");

	exit(1);
}
