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

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <poll.h>
#include <configmake.h>
#include <ncs_main_pvt.h>
#include <rda_papi.h>

#include "ntfs.h"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

#define FD_USR1 0
#define FD_AMF 0
#define FD_MBCSV 1
#define FD_MBX 2
#define FD_LOG 3

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */
static ntfs_cb_t _ntfs_cb;
ntfs_cb_t *ntfs_cb = &_ntfs_cb;

static int category_mask;
static NCS_SEL_OBJ usr1_sel_obj;
/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */
extern void initAdmin(void);
extern void printAdminInfo();
extern void logEvent();

/**
 * USR1 signal is used when AMF wants instantiate us as a
 * component. Wake up the main thread so it can register with
 * AMF.
 * 
 * @param i_sig_num
 */
static void sigusr1_handler(int sig)
{
	(void) sig;
	signal(SIGUSR1, SIG_IGN);
	ncs_sel_obj_ind(usr1_sel_obj);
	TRACE("Got USR1 signal");
}

/**
 * USR1 signal handler to enable/disable trace (toggle)
 * @param sig
 */
static void sigusr2_handler(int sig)
{
	if (category_mask == 0) {
		category_mask = CATEGORY_ALL;
		printf("Enabling traces");
	} else {
		category_mask = 0;
		printf("Disabling traces");
	}

	if (trace_category_set(category_mask) == -1)
		printf("trace_category_set failed");
}

/**
 *  dump information from all data structures
 * @param sig
 */
static void dump_sig_handler(int sig)
{
	int old_category_mask = category_mask;

	if (trace_category_set(CATEGORY_ALL) == -1)
		printf("trace_category_set failed");

	TRACE("---- USR2 signal received, dump info ----");
	TRACE("Control block information");
	TRACE("  comp_name:      %s", ntfs_cb->comp_name.value);
	TRACE("  ntf_version:    %c.%02d.%02d", ntfs_cb->ntf_version.releaseCode,
	      ntfs_cb->ntf_version.majorVersion, ntfs_cb->ntf_version.minorVersion);
	TRACE("  mds_role:       %u", ntfs_cb->mds_role);
	TRACE("  ha_state:       %u", ntfs_cb->ha_state);
	TRACE("  ckpt_state:     %u", ntfs_cb->ckpt_state);
	printAdminInfo();
	TRACE("---- USR2 dump info end ----");

	if (trace_category_set(old_category_mask) == -1)
		printf("trace_category_set failed");
}

/**
 * Initialize ntfs
 * 
 * @return uns32
 */
static uns32 initialize(const char *progname)
{
	uns32 rc = NCSCC_RC_SUCCESS;;
	FILE *fp = NULL;
	char path[NAME_MAX + 32];
	char *value;
	char *ntfd_argv[] = { "", "MDS_SUBSCRIPTION_TMR_VAL=1" };

	TRACE_ENTER();

	/* Create pidfile */
	sprintf(path, PIDPATH "%s.pid", basename(progname));
	if ((fp = fopen(path, "w")) == NULL) {
		LOG_ER("Could not open %s", path);
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* Write PID to it */
	if (fprintf(fp, "%d", getpid()) < 1) {
		fclose(fp);
		LOG_ER("Could not write to: %s", path);
		rc = NCSCC_RC_FAILURE;
		goto done;
	}
	fclose(fp);

	if ((value = getenv("NTFSV_TRACE_PATHNAME")) != NULL) {
		if (logtrace_init(basename(progname), value) != 0) {
			syslog(LOG_ERR, "logtrace_init FAILED, exiting...");
			goto done;
		}

		if ((value = getenv("NTFSV_TRACE_CATEGORIES")) != NULL) {
			/* Do not care about categories now, get all */
			category_mask = CATEGORY_ALL;
			trace_category_set(category_mask);
		}
	}

	if (ncspvt_svcs_startup(2, ntfd_argv, NULL) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncspvt_svcs_startup failed");
		goto done;
	}

	/* Initialize ntfs control block */
	if (ntfs_cb_init(ntfs_cb) != NCSCC_RC_SUCCESS) {
		TRACE("ntfs_cb_init FAILED");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	if ((rc = rda_get_role(&ntfs_cb->ha_state)) != NCSCC_RC_SUCCESS) {
		LOG_ER("rda_get_role FAILED");
		goto done;
	}

	m_NCS_EDU_HDL_INIT(&ntfs_cb->edu_hdl);

	/* Create the mailbox used for communication with NTFS */
	if ((rc = m_NCS_IPC_CREATE(&ntfs_cb->mbx)) != NCSCC_RC_SUCCESS) {
		TRACE("m_NCS_IPC_CREATE FAILED %d", rc);
		goto done;
	}

	/* Attach mailbox to this thread */
	if ((rc = m_NCS_IPC_ATTACH(&ntfs_cb->mbx) != NCSCC_RC_SUCCESS)) {
		TRACE("m_NCS_IPC_ATTACH FAILED %d", rc);
		goto done;
	}

	if ((rc = ntfs_mds_init(ntfs_cb)) != NCSCC_RC_SUCCESS) {
		TRACE("ntfs_mds_init FAILED %d", rc);
		return rc;
	}

	if ((rc = ntfs_mbcsv_init(ntfs_cb)) != NCSCC_RC_SUCCESS) {
		TRACE("ntfs_mbcsv_init FAILED");
		return rc;
	}

	if ((rc = ncs_sel_obj_create(&usr1_sel_obj)) != NCSCC_RC_SUCCESS)
	{
		LOG_ER("ncs_sel_obj_create failed");
		goto done;
	}

	if (signal(SIGUSR1, sigusr1_handler) == SIG_ERR) {
		LOG_ER("signal USR1 failed: %s", strerror(errno));
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	if (signal(SIGUSR2, sigusr2_handler) == SIG_ERR) {
		LOG_ER("signal USR2 failed: %s", strerror(errno));
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	initAdmin();
 done:
	if (nid_notify("NTFD", rc, NULL) != NCSCC_RC_SUCCESS) {
		LOG_ER("nid_notify failed");
		rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return (rc);
}

/**
 * Forever wait on events on AMF, MBCSV & NTFS Mailbox file descriptors
 * and process them.
 */
int main(int argc, char *argv[])
{
	NCS_SEL_OBJ mbx_fd;
	SaAisErrorT error;
	uns32 rc;
	struct pollfd fds[4];

	TRACE_ENTER();

	if (initialize(argv[0]) != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "initialize in ntfs  FAILED, exiting...");
		goto done;
	}

	mbx_fd = ncs_ipc_get_sel_obj(&ntfs_cb->mbx);

	/* Set up all file descriptors to listen to */
	fds[FD_USR1].fd = usr1_sel_obj.rmv_obj;
	fds[FD_USR1].events = POLLIN;
	fds[FD_MBCSV].fd = ntfs_cb->mbcsv_sel_obj;
	fds[FD_MBCSV].events = POLLIN;
	fds[FD_MBX].fd = mbx_fd.rmv_obj;
	fds[FD_MBX].events = POLLIN;
	fds[FD_LOG].fd = ntfs_cb->logSelectionObject;
	fds[FD_LOG].events = POLLIN;

    /** NTFS main processing loop. */
	while (1) {
		int ret = poll(fds, 4, -1);

		if (ret == -1) {
			if (errno == EINTR)
				continue;

			LOG_ER("poll failed - %s", strerror(errno));
			break;
		}

		if (fds[FD_AMF].revents & POLLIN) {
			/* dispatch all the AMF pending function */

			if (ntfs_cb->amf_hdl != 0) {
				if ((error = saAmfDispatch(ntfs_cb->amf_hdl, SA_DISPATCH_ALL)) != SA_AIS_OK) {
					LOG_ER("saAmfDispatch failed: %u", error);
					break;
				}
			} else {

				TRACE("SIGUSR1 event rec");
				ncs_sel_obj_rmv_ind(usr1_sel_obj, TRUE, TRUE);
				ncs_sel_obj_destroy(usr1_sel_obj);

				if (ntfs_amf_init(ntfs_cb) != NCSCC_RC_SUCCESS)
					break;

				TRACE("AMF Initialization SUCCESS......");
				fds[FD_AMF].fd = ntfs_cb->amfSelectionObject;
			}

		}

		if (fds[FD_MBCSV].revents & POLLIN) {
			rc = ntfs_mbcsv_dispatch(ntfs_cb->mbcsv_hdl);
			if (rc != NCSCC_RC_SUCCESS)
				LOG_ER("MBCSV DISPATCH FAILED: %u", rc);
		}

		/* Process the NTFS Mail box, if ntfs is ACTIVE. */
		if (fds[FD_MBX].revents & POLLIN)
			ntfs_process_mbx(&ntfs_cb->mbx);

		/* process all the log callbacks */
		if (fds[FD_LOG].revents & POLLIN)
			logEvent();
	}

 done:
	LOG_ER("Failed, exiting...");
	TRACE_LEAVE();
	exit(1);

}
