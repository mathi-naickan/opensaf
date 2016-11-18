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

#include <stdlib.h>
#include "base/ncssysf_tsk.h"

#include "base/logtrace.h"
#include "base/daemon.h"

#include "ckpt/ckptd/cpd.h"
extern void cpd_main_process(CPD_CB *cb);
static int __init_cpd(void)
{
	NCS_LIB_REQ_INFO lib_create;

	/* Enable extended SaNameT */
	if (setenv("SA_ENABLE_EXTENDED_NAMES", "1", 1) != 0) {
		LOG_ER("Failed to set environment variable: SA_ENABLE_EXTENDED_NAMES");
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	/* Init LIB_CREATE request for Server */
	memset(&lib_create, 0, sizeof(lib_create));
	lib_create.i_op = NCS_LIB_REQ_CREATE;
	lib_create.info.create.argc = 0;
	lib_create.info.create.argv = NULL;

	if (ncs_agents_startup() != NCSCC_RC_SUCCESS) {
	
		LOG_ER("ncs_agents_startup FAILED");
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	/* Init CPD */
	printf("\nCPSV:CPD:ON");
	if (cpd_lib_req(&lib_create) != NCSCC_RC_SUCCESS) {
		LOG_ER("cpd_lib_req FAILED");
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	return (NCSCC_RC_SUCCESS);
}

int main(int argc, char *argv[])
{
	CPD_CB *cb;
	daemonize(argc, argv);

	if (__init_cpd() != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "__init_cpd() failed");
		exit(EXIT_FAILURE);
	}

	cb = ncshm_take_hdl(NCS_SERVICE_ID_CPD, gl_cpd_cb_hdl);
	if (!cb) {
		LOG_ER("Handle get failed");
		return 0;
	}
	ncshm_give_hdl(cb->cpd_hdl);
	cpd_main_process(cb);

	exit(1);
}
