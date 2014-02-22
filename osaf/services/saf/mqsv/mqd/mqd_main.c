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

#include "mqd.h"

extern void mqd_main_process(uint32_t);
extern MQDLIB_INFO gl_mqdinfo;
static int __init_mqd(void)
{
	NCS_LIB_REQ_INFO lib_create;

	/* Init LIB_CREATE request for Server */
	memset(&lib_create, 0, sizeof(lib_create));
	lib_create.i_op = NCS_LIB_REQ_CREATE;
	lib_create.info.create.argc = 0;
	lib_create.info.create.argv = NULL;

	if (ncs_agents_startup() != NCSCC_RC_SUCCESS) {
		TRACE_4("ncs_agents_startup failed");
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	/* Init MQD */
	printf("\nMQSV:MQD:ON");
	if (mqd_lib_req(&lib_create) != NCSCC_RC_SUCCESS) {
		fprintf(stderr, "mqd_lib_req FAILED\n");
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	return (NCSCC_RC_SUCCESS);
}

int main(int argc, char *argv[])
{
	daemonize(argc, argv);

	if (__init_mqd() != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "__init_mqd() failed");
		exit(EXIT_FAILURE);
	}

	mqd_main_process(gl_mqdinfo.inst_hdl);

	exit(1);
}
