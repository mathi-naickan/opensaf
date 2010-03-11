/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
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

#include <logtrace.h>
#include <daemon.h>

#include "avnd.h"

static int __init_avnd(void)
{
	char *trace_mask_env;
	unsigned int trace_mask;

	if ((trace_mask_env = getenv("AMFND_TRACE_CATEGORIES")) != NULL) {
		trace_mask = strtoul(trace_mask_env, NULL, 0);
		trace_category_set(trace_mask);
	}

	if (ncs_agents_startup() != NCSCC_RC_SUCCESS)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	return (NCSCC_RC_SUCCESS);
}

int main(int argc, char *argv[])
{
	uns32 error;

	daemonize(argc, argv);

	if (__init_avnd() != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "__init_avd() failed");
		goto done;
	}

	/* should never return */
	avnd_main_process();

	ncs_reboot("avnd_main_proc exited");
	exit(1);

done:
	(void) nid_notify("AMFND", NCSCC_RC_FAILURE, &error);
	fprintf(stderr, "failed, exiting\n");
	exit(1);
}
