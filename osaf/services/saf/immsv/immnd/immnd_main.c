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

/*****************************************************************************
  FILE NAME: immnd_main.c

  DESCRIPTION: APIs used to initialize & Destroy the IMMND Service Part.

******************************************************************************/

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

#include "immnd.h"

#define FD_AMF 0
#define FD_MBX 1

static IMMND_CB _immnd_cb;
IMMND_CB *immnd_cb = &_immnd_cb;

/* Static Function Declerations */

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
	ncs_sel_obj_ind(immnd_cb->usr1_sel_obj);
	/* Do not use trace in the signal handler
	   It can apparently cause the main thread
	   to spin or deadlock. See ticket #1173.
	 */
}

/****************************************************************************
 * Name          : immnd_cb_db_init
 *
 * Description   : This is the function which initializes all the data 
 *                 structures and locks used belongs to IMMND.
 *
 * Arguments     : cb  - IMMND control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t immnd_cb_db_init(IMMND_CB *cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	rc = immnd_client_node_tree_init(cb);

	if (rc == NCSCC_RC_FAILURE)
		LOG_ER("client node tree init failed");

	return (rc);
}

/****************************************************************************
 * Name          : initialize_immnd
 *
 * Description   : This is the function which initalize the IMMND libarary.
 *                 This function creates an IPC mail Box and spawns IMMND
 *                 thread.
 *                 This function initializes the CB, MDS, IMMD connection
 *                 and Registers with AMF with respect to the component Type 
 *                 (IMMND).
 *
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t immnd_initialize(char *progname)
{
	uint32_t rc = NCSCC_RC_FAILURE;
	char *envVar = NULL;

	TRACE_ENTER();

	/* Determine how this process was started, by NID or AMF */
	if (getenv("SA_AMF_COMPONENT_NAME") == NULL)
		immnd_cb->nid_started = 1;

	if (ncs_agents_startup() != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_agents_startup FAILED");
		goto done;
	}

	/* Initialize immnd control block */
	immnd_cb->ha_state = SA_AMF_HA_ACTIVE;
	immnd_cb->cli_id_gen = 1;
	m_NCS_EDU_HDL_INIT(&immnd_cb->immnd_edu_hdl);
	immnd_cb->mState = IMM_SERVER_ANONYMOUS;
	immnd_cb->loaderPid = (-1);
	immnd_cb->syncPid = (-1);
	immnd_cb->mMyPid = getpid();
	immnd_cb->mProgName = progname;
	immnd_cb->mDir = getenv("IMMSV_ROOT_DIRECTORY");
	immnd_cb->mFile = getenv("IMMSV_LOAD_FILE");
	if ((envVar = getenv("IMMSV_NUM_NODES"))) {
		int numNodes = atoi(envVar);
		if(numNodes > 255) {
			LOG_WA("IMMSV_NUM_NODES set to %u, must be "
				"less than 256. Setting to 255", numNodes);
			numNodes = 255;
		}
		immnd_cb->mExpectedNodes = (uint8_t) numNodes;
	}
	if ((envVar = getenv("IMMSV_MAX_WAIT"))) {
		int waitSecs = atoi(envVar);
		if(waitSecs > 255) {
			LOG_WA("IMMSV_MAX_WAIT set to %u, must be "
				"less than 256. Setting to 255", waitSecs);
			waitSecs = 255;
		}
		immnd_cb->mWaitSecs = waitSecs;
	}

	if ((immnd_cb->mPbeFile = getenv("IMMSV_PBE_FILE")) != NULL) {
		LOG_NO("Persistent Back-End capability configured, Pbe file:%s",
			immnd_cb->mPbeFile);
	}

	immnd_cb->mRim = SA_IMM_INIT_FROM_FILE;
	immnd_cb->mPbeVeteran = SA_FALSE;

	if (immnd_cb->mDir == NULL) {
		LOG_ER("Env var IMMSV_ROOT_DIRECTORY missing");
		goto done;
	}

	if (immnd_cb->mFile == NULL) {
		LOG_ER("Env var IMMSV_LOAD_FILE missing");
		goto done;
	}

	TRACE_2("Dir:%s File:%s ExpectedNodes:%u WaitSecs:%u",
		immnd_cb->mDir, immnd_cb->mFile, immnd_cb->mExpectedNodes, immnd_cb->mWaitSecs);

	if ((rc = immnd_cb_db_init(immnd_cb)) == NCSCC_RC_FAILURE) {
		TRACE("immnd_cb_db_init Failed");
		goto done;
	}

	/* Create a mail box */
	if ((rc = m_NCS_IPC_CREATE(&immnd_cb->immnd_mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("m_NCS_IPC_CREATE FAILED");
		goto done;
	}

	/* Attach the IPC to mail box */
	if ((rc = m_NCS_IPC_ATTACH(&immnd_cb->immnd_mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("m_NCS_IPC_ATTACH FAILED");
		goto done;
	}

	if ((rc = immnd_mds_register(immnd_cb)) != NCSCC_RC_SUCCESS) {
		TRACE("immnd_mds_register FAILED %u", rc);
		goto done;
	}

	/* Create a selection object */
	if ((rc = ncs_sel_obj_create(&immnd_cb->usr1_sel_obj)) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_sel_obj_create failed");
		goto done;
	}

	if (immnd_cb->nid_started && signal(SIGUSR1, sigusr1_handler) == SIG_ERR) {
		LOG_ER("signal USR1 failed: %s", strerror(errno));
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* If AMF started register immidiately */
	if (!immnd_cb->nid_started) {
		if ((rc = immnd_amf_init(immnd_cb)) != NCSCC_RC_SUCCESS)
			goto done;
	}

	rc = NCSCC_RC_SUCCESS;
	syslog(LOG_INFO, "Initialization Success");

 done:
	if (immnd_cb->nid_started && (rc == NCSCC_RC_FAILURE)) {
		if (nid_notify("IMMND", NCSCC_RC_FAILURE, NULL) != NCSCC_RC_SUCCESS)
			LOG_ER("nid_notify failed");
	}

	TRACE_LEAVE();
	return (rc);
}

/**
 * The main routine for the immnd daemon.
 * @param argc
 * @param argv
 * 
 * @return int
 */
int main(int argc, char *argv[])
{
	NCS_SEL_OBJ mbx_fd;
	SaAisErrorT error;
	uint32_t timeout = 100;	/* ABT For server maintenance.
				   Using a period of 0.1s during startup. */
	int eventCount = 0;	/* Used to regulate progress of background 
				   server task when we are very bussy. */
	int maxEvt = 100;
	int64_t start_time = 0LL;
	struct pollfd fds[2];

	daemonize(argc, argv);

	if (immnd_initialize(argv[0]) != NCSCC_RC_SUCCESS) {
		LOG_ER("initialize_immd failed");
		goto done;
	}

	/* Get file descriptor for mailbox */
	mbx_fd = m_NCS_IPC_GET_SEL_OBJ(&immnd_cb->immnd_mbx);

	/* Set up all file descriptors to listen to */
	if (immnd_cb->nid_started)
		fds[FD_AMF].fd = immnd_cb->usr1_sel_obj.rmv_obj;
	else
		fds[FD_AMF].fd = (int) immnd_cb->amf_sel_obj;

	fds[FD_AMF].events = POLLIN;
	fds[FD_MBX].fd = mbx_fd.rmv_obj;
	fds[FD_MBX].events = POLLIN;

	while (1) {
		/* Watch out for performance bug. Possibly change from event-count
		   to recalculated timer. */
		/* ABT 13/07 2009 actually using both event-count and recalculated timer now. */
		uint32_t passed_time = (uint32_t) start_time ? (m_NCS_GET_TIME_MS - start_time) : 0;

		maxEvt = (timeout == 100) ? 50 : 100;

		/*
		   TRACE("timeout:%u adjustedTimeout:%u", timeout, 
		   (passed_time<timeout) ? (timeout - passed_time) : 0);
		 */

		/* Wait for events */
		if (!start_time) {
			start_time = m_NCS_GET_TIME_MS;
		}
		int ret = poll(fds, 2, (passed_time < timeout) ? (timeout - passed_time) : 0);

		if (ret == -1) {
			if (errno == EINTR)
				continue;

			LOG_ER("poll failed - %s", strerror(errno));
			break;
		}

		if (ret > 0) {
			++eventCount;

			if (fds[FD_AMF].revents & POLLIN) {
				if (immnd_cb->amf_hdl != 0) {
					error = saAmfDispatch(immnd_cb->amf_hdl, SA_DISPATCH_ALL);
					if (error != SA_AIS_OK) {
						LOG_ER("saAmfDispatch failed: %u", error);
						break;
					}
				} else {
					TRACE("SIGUSR1 event rec");
					ncs_sel_obj_rmv_ind(immnd_cb->usr1_sel_obj, true, true);
					ncs_sel_obj_destroy(immnd_cb->usr1_sel_obj);

					if (immnd_amf_init(immnd_cb) != NCSCC_RC_SUCCESS)
						break;

					TRACE("AMF Initialization SUCCESS......");
					fds[FD_AMF].fd = (int) immnd_cb->amf_sel_obj;
				}
			}

			if (fds[FD_MBX].revents & POLLIN) {
				uint8_t wasCoord = immnd_cb->mIsCoord;
				immnd_process_evt();
				if (!wasCoord && immnd_cb->mIsCoord) {
					TRACE("We just became Coord => Force a server job!");
					/* This is particularly urgent in a failover situation. */
					eventCount = maxEvt;
				}
			}

			if (eventCount >= maxEvt) {
				/* Make some progress on background task, 
				   even when we are very busy. */
				TRACE_2("FORCE a proc_server job after %u events", eventCount);
				uint32_t rc = immnd_proc_server(&timeout);
				if (rc != NCSCC_RC_SUCCESS) {
					LOG_ER("IMMND - Periodic server job failed");
					break;
				}
				eventCount = 0;
				start_time = 0LL;
			}
		} else {
			/* Timeout */
			/*TRACE_2("PERIODIC proc_server eventcout: %u", eventCount); */
			uint32_t rc = immnd_proc_server(&timeout);
			if (rc != NCSCC_RC_SUCCESS) {
				LOG_ER("IMMND - Periodic server job failed");
				break;
			}
			eventCount = 0;
			start_time = 0LL;
		}
	}

 done:
	LOG_ER("Failed, exiting...");
	TRACE_LEAVE();
	return -1;
}
