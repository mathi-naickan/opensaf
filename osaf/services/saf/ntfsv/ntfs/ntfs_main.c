/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008-2010 The OpenSAF Foundation
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
 *            Wind River Systems
 *
 */

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#define _GNU_SOURCE
#include <libgen.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <poll.h>

#include <configmake.h>
#include <rda_papi.h>
#include <daemon.h>
#include <nid_api.h>
#include <ncs_main_papi.h>

#include "ntfs.h"
#include "ntfs_imcnutil.h"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

enum {
	FD_USR1 = 0,
	FD_AMF = 0,
	FD_MBCSV,
	FD_MBX,
	FD_LOG,
	FD_TERM,
	SIZE_FDS
} NTFS_FDS;

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
static NCS_SEL_OBJ usr1_sel_obj;
static NCS_SEL_OBJ term_sel_obj; /* Selection object for TERM signal events */

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */
extern void initAdmin(void);
extern void printAdminInfo();
extern void logEvent();

/**
 * Callback from RDA. Post a message/event to the ntfs mailbox.
 * @param cb_hdl
 * @param cb_info
 * @param error_code
 */
static void rda_cb(uint32_t cb_hdl, PCS_RDA_CB_INFO *cb_info, PCSRDA_RETURN_CODE error_code)
{
	uint32_t rc;
	ntfsv_ntfs_evt_t *evt=NULL;

	TRACE_ENTER();

	evt = calloc(1, sizeof(ntfsv_ntfs_evt_t));
	if (NULL == evt) {
		LOG_ER("calloc failed");
		goto done;
	}

	evt->evt_type = NTFSV_EVT_RDA;
	evt->info.rda_info.io_role = cb_info->info.io_role;

	rc = ncs_ipc_send(&ntfs_cb->mbx, (NCS_IPC_MSG *)evt, MDS_SEND_PRIORITY_VERY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("IPC send failed %d", rc);
		if (evt != NULL) {
			free(evt);
		}
	}

done:
	TRACE_LEAVE();
}

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
 * TERM signal handler
 *
 * @param sig
 */
static void sigterm_handler(int sig)
{
	ncs_sel_obj_ind(term_sel_obj);
	signal(SIGTERM, SIG_IGN);
}

/**
 * TERM event handler
 */
static void handle_sigterm_event(void)
{
	LOG_NO("exiting on signal %d", SIGTERM);
	(void) stop_ntfimcn();
	_Exit(EXIT_SUCCESS);
}

#if 0
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
#endif

/**
 * Initialize ntfs
 * 
 * @return uns32
 */
static uint32_t initialize()
{
	uint32_t rc = NCSCC_RC_SUCCESS;;

	TRACE_ENTER();

	if (ncs_agents_startup() != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_core_agents_startup FAILED");
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

	if ((rc = rda_register_callback(0, rda_cb)) != NCSCC_RC_SUCCESS) {
		LOG_ER("rda_register_callback FAILED %u", rc);
		goto done;
	}

	m_NCS_EDU_HDL_INIT(&ntfs_cb->edu_hdl);

	/* Create the mailbox used for communication with NTFS */
	if ((rc = m_NCS_IPC_CREATE(&ntfs_cb->mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("m_NCS_IPC_CREATE FAILED %d", rc);
		goto done;
	}

	/* Attach mailbox to this thread */
	if ((rc = m_NCS_IPC_ATTACH(&ntfs_cb->mbx) != NCSCC_RC_SUCCESS)) {
		LOG_ER("m_NCS_IPC_ATTACH FAILED %d", rc);
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
	uint32_t rc;
	struct pollfd fds[SIZE_FDS];

	TRACE_ENTER();

	daemonize(argc, argv);

	if (initialize() != NCSCC_RC_SUCCESS) {
		LOG_ER("initialize in ntfs  FAILED, exiting...");
		goto done;
	}

	/* Start the secondary configuration notifier process */
	init_ntfimcn(ntfs_cb->ha_state);

	if (ncs_sel_obj_create(&term_sel_obj) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_sel_obj_create failed");
		exit(EXIT_FAILURE);
	}

	if (signal(SIGTERM, sigterm_handler) == SIG_ERR) {
		LOG_ER("signal TERM failed: %s", strerror(errno));
		exit(EXIT_FAILURE);
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
	fds[FD_TERM].fd = term_sel_obj.rmv_obj;
	fds[FD_TERM].events = POLLIN;

	/* NTFS main processing loop. */
	while (1) {
		int ret = poll(fds, SIZE_FDS, -1);

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
				ncs_sel_obj_rmv_ind(usr1_sel_obj, true, true);
				ncs_sel_obj_destroy(usr1_sel_obj);

				if (ntfs_amf_init() != SA_AIS_OK)
					break;

				TRACE("AMF Initialization SUCCESS......");
				fds[FD_AMF].fd = ntfs_cb->amfSelectionObject;
			}

		}

		if (fds[FD_TERM].revents & POLLIN) {
			handle_sigterm_event();
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
	exit(EXIT_FAILURE);
}
