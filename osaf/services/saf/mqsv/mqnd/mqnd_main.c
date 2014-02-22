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

#include "mqnd.h"
extern void mqnd_main_process(uint32_t hdl);
static int __init_mqnd(void)
{
	NCS_LIB_REQ_INFO lib_create;
	TRACE_ENTER();

	/* Init LIB_CREATE request for Server */
	memset(&lib_create, 0, sizeof(lib_create));
	lib_create.i_op = NCS_LIB_REQ_CREATE;
	lib_create.info.create.argc = 0;
	lib_create.info.create.argv = NULL;

	if (ncs_agents_startup() != NCSCC_RC_SUCCESS) {
		TRACE_4("ncs_agents_startup failed");
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	/* Init MQND */
	printf("\nMQSV:MQND:ON");
	if (mqnd_lib_req(&lib_create) != NCSCC_RC_SUCCESS) {
		fprintf(stderr, "mqnd_lib_req FAILED\n");
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	TRACE_LEAVE();
	return (NCSCC_RC_SUCCESS);
}

int main(int argc, char *argv[])
{
	uint32_t mqnd_hdl;
	daemonize(argc, argv);

	if (__init_mqnd() != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "__init_mqnd() failed");
		exit(EXIT_FAILURE);
	}

	mqnd_hdl = m_MQND_GET_HDL();

	mqnd_main_process(mqnd_hdl);

	exit(1);
}
