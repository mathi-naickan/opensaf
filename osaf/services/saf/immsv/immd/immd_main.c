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

#include "immd.h"
#include <immd_proc.h>

static IMMD_CB _immd_cb;
IMMD_CB *immd_cb = &_immd_cb;

#define FD_TERM 0
#define FD_AMF 1
#define FD_MBCSV 2
#define FD_MBX 3

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */

/**
 * Callback from RDA. Post a message/event to the IMMD mailbox.
 * @param cb_hdl
 * @param cb_info
 * @param error_code
 */
static void rda_cb(uint32_t cb_hdl, PCS_RDA_CB_INFO *cb_info, PCSRDA_RETURN_CODE error_code)
{
	uint32_t rc;
	IMMSV_EVT *evt;

	TRACE_ENTER();

	evt = calloc(1, sizeof(IMMSV_EVT));
	if (NULL == evt) {
		LOG_ER("calloc failed");
		goto done;
	}

	evt->type = IMMSV_EVT_TYPE_IMMD;
	evt->info.immd.type = IMMD_EVT_LGA_CB;
	evt->info.immd.info.rda_info.io_role = cb_info->info.io_role;

	rc = ncs_ipc_send(&immd_cb->mbx, (NCS_IPC_MSG *)evt, MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS)
		LOG_ER("IPC send failed %d", rc);

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
	(void)sig;
	signal(SIGUSR1, SIG_IGN);
	ncs_sel_obj_ind(&immd_cb->usr1_sel_obj);
	/* Do not use trace in the signal handler
	   It can apparently cause the main thread
	   to spin or deadlock. See ticket #1173.
	 */
}

/**
 * Initialize immd
 * 
 * @return uns32
 */
static uint32_t immd_initialize(void)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* Determine how this process was started, by NID or AMF */
	if (getenv("SA_AMF_COMPONENT_NAME") == NULL)
		immd_cb->nid_started = true;

	if (ncs_agents_startup() != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_agents_startup FAILED");
		goto done;
	}

	if ((rc = rda_get_role(&immd_cb->ha_state)) != NCSCC_RC_SUCCESS) {
		LOG_ER("rda_get_role FAILED");
		goto done;
	}

	if ((rc = rda_register_callback(0, rda_cb)) != NCSCC_RC_SUCCESS) {
		LOG_ER("rda_register_callback FAILED %u", rc);
		goto done;
	}

	/* Init the EDU Handle */
	m_NCS_EDU_HDL_INIT(&immd_cb->edu_hdl);

	if ((rc = immd_cb_db_init(immd_cb)) != NCSCC_RC_SUCCESS) {
		LOG_ER("immd_cb_db_init FAILED");
		goto done;
	}

	/* Create a mail box */
	if ((rc = m_NCS_IPC_CREATE(&immd_cb->mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("m_NCS_IPC_CREATE FAILED");
		goto done;
	}

	/* Attach the IPC to mail box */
	if ((rc = m_NCS_IPC_ATTACH(&immd_cb->mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("m_NCS_IPC_ATTACH FAILED");
		goto done;
	}

	if ((rc = immd_mds_register(immd_cb)) != NCSCC_RC_SUCCESS) {
		LOG_ER("immd_mds_register FAILED %d", rc);
		goto done;
	}

	/* Initialise with the MBCSV service  */
	if ((rc = immd_mbcsv_register(immd_cb)) != NCSCC_RC_SUCCESS) {
		LOG_ER("immd_mbcsv_register FAILED %d", rc);
		goto done;
	}

	if ((rc = immd_mbcsv_chgrole(immd_cb)) != NCSCC_RC_SUCCESS) {
		LOG_ER("immd_mbcsv_chgrole FAILED %d", rc);
		goto done;
	}

	/* Create a selection object */
	if (immd_cb->nid_started &&
		(rc = ncs_sel_obj_create(&immd_cb->usr1_sel_obj)) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_sel_obj_create failed");
		goto done;
	}

	/*
	 * Initialize a signal handler that will use the selection object.
	 * The signal is sent from our script when AMF does instantiate.
	 */
	if (immd_cb->nid_started &&
		signal(SIGUSR1, sigusr1_handler) == SIG_ERR) {
		LOG_ER("signal USR1 failed: %s", strerror(errno));
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* If AMF started register immediately */
	if (!immd_cb->nid_started &&
		(rc = immd_amf_init(immd_cb)) != NCSCC_RC_SUCCESS) {
		goto done;
	}

	syslog(LOG_INFO, "Initialization Success, role %s",
	       (immd_cb->ha_state == SA_AMF_HA_ACTIVE) ? "ACTIVE" : "STANDBY");

done:
	if (immd_cb->nid_started &&
		nid_notify("IMMD", rc, NULL) != NCSCC_RC_SUCCESS) {
		LOG_ER("nid_notify failed");
		rc = NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return rc;
}

/**
 * The main routine for the IMM director daemon.
 * @param argc
 * @param argv
 * 
 * @return int
 */
int main(int argc, char *argv[])
{
	SaAisErrorT error;
	NCS_SEL_OBJ mbx_fd;
	struct pollfd fds[4];
	const int peerMaxWaitMin = 5; /*5 sec*/
	const char * peerWaitStr = getenv("IMMSV_2PBE_PEER_SC_MAX_WAIT");
	int32_t timeout = (-1);
	int32_t total_wait = (-1);
	int64_t start_time = 0LL;
	uint32_t print_at_secs = 1LL;
	int term_fd;

	daemonize(argc, argv);

	if (immd_initialize() != NCSCC_RC_SUCCESS) {
		TRACE("initialize_immd failed");
		goto done;
	}

	daemon_sigterm_install(&term_fd);

	if(peerWaitStr) {
		int32_t peerMaxWait = atoi(peerWaitStr);
		if(peerMaxWait < peerMaxWaitMin) {
			LOG_WA("IMMSV_PEER_SC_MAX_WAIT has value (%d) less than %u, "
				"Correcting to %u", peerMaxWait, peerMaxWaitMin,
				peerMaxWaitMin);
			peerMaxWait = peerMaxWaitMin;
		}
		LOG_NO("2PBE configured with IMMSV_PEER_SC_MAX_WAIT: %d seconds", peerMaxWait);

		total_wait = peerMaxWait * 1000;
		timeout = total_wait;
		start_time = m_NCS_GET_TIME_MS;

		immd_cb->mIs2Pbe = true; /* Redundant PBE */
		immd_cb->m2PbeCanLoad = false; /* Not ready to load yet */
	}


	/* Get file descriptor for mailbox */
	mbx_fd = ncs_ipc_get_sel_obj(&immd_cb->mbx);

	/* Set up all file descriptors to listen to */
	fds[FD_TERM].fd = term_fd;
	fds[FD_TERM].events = POLLIN;
	fds[FD_AMF].fd = immd_cb->nid_started ?
		immd_cb->usr1_sel_obj.rmv_obj : immd_cb->amf_sel_obj;
	fds[FD_AMF].events = POLLIN;
	fds[FD_MBCSV].fd = immd_cb->mbcsv_sel_obj;
	fds[FD_MBCSV].events = POLLIN;
	fds[FD_MBX].fd = mbx_fd.rmv_obj;
	fds[FD_MBX].events = POLLIN;

	while (1) {
		int ret = poll(fds, 4, timeout);

		if (ret == -1) {
			if (errno == EINTR)
				continue;

			LOG_ER("poll failed - %s", strerror(errno));
			break;
		}

		if (fds[FD_TERM].revents & POLLIN) {
			daemon_exit();
		}

		if (fds[FD_MBCSV].revents & POLLIN) {
			if (immd_mbcsv_dispatch(immd_cb) != NCSCC_RC_SUCCESS) {
				LOG_ER("MBCSv Dispatch Failed");
				break;
			}
		}

		if (fds[FD_MBX].revents & POLLIN)
			immd_process_evt();

		if (fds[FD_AMF].revents & POLLIN) {
			if (immd_cb->amf_hdl != 0) {
				error = saAmfDispatch(immd_cb->amf_hdl, SA_DISPATCH_ALL);
				if (error != SA_AIS_OK) {
					LOG_ER("saAmfDispatch failed: %u", error);
					break;
				}
			} else {
				TRACE("SIGUSR1 event rec");
				ncs_sel_obj_rmv_ind(&immd_cb->usr1_sel_obj, true, true);
				ncs_sel_obj_destroy(&immd_cb->usr1_sel_obj);

				if (immd_amf_init(immd_cb) != NCSCC_RC_SUCCESS)
					break;

				TRACE("AMF Initialization SUCCESS......");
				fds[FD_AMF].fd = immd_cb->amf_sel_obj;
			}
		}

		if(timeout > 0) {
			if(immd_cb->m2PbeCanLoad) {
				/* Peer immnd has been resolved. */
				timeout = (-1);
			} else {
				/* Otherwise decrement timeout by time consumed. */
				uint32_t passed_time = (uint32_t) (m_NCS_GET_TIME_MS - start_time);
				if(passed_time < total_wait) {
					timeout = total_wait - passed_time;
					if(print_at_secs <= (passed_time/1000)) {
						LOG_NO("2PBE wait. Passed time:%u new timeout: %d msecs", passed_time, timeout);
						print_at_secs++;
					}
				} else if(!(immd_cb->m2PbeExtraWait) && immd_cb->is_loc_immnd_up && immd_cb->is_rem_immnd_up) {
					/* We prolong the wait at most once and only if both SCs have been introduced,
					   but loading arbitration has obviously not been completed yet.
					   This is to avoid the second SC joining late, starting preloading and regular
					   loading then starting before the second preloading has replied.
					*/

					immd_cb->m2PbeExtraWait = true; 
					total_wait = 2 * 1000;
					timeout = total_wait;
					start_time = m_NCS_GET_TIME_MS;
				} else {
					LOG_WA("2PBE TIMEOUT WAITING FOR PEER, POSSIBLE REWIND OF IMM STATE.");
					TRACE("Passed time:%u", passed_time);
					timeout = (-1);
					start_time = 0LL;
					immd_cb->m2PbeCanLoad=true;
					if (!(immd_cb->immnd_coord) && (immd_cb->ha_state == SA_AMF_HA_ACTIVE)) {
						LOG_NO("Electing coord, my HA-STATE:%u", immd_cb->ha_state);
						/* Will tell standby also */
						immd_proc_arbitrate_2pbe_preload(immd_cb);
					}
				}
			}
		}
	}

done:
	LOG_ER("Failed, exiting...");
	TRACE_LEAVE();
	exit(1);
}
