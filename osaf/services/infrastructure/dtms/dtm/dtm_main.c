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
 * licensing terms.z
 *
 * Author(s): GoAhead Software
 *
 */

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#define _GNU_SOURCE
#include <daemon.h>
#include <nid_api.h>
#include <configmake.h>
#include "ncs_main_papi.h"
#include "dtm.h"
#include "dtm_node.h"
#include "osaf_poll.h"

#include <stdlib.h>
#include <sched.h>

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

#define DTM_CONFIG_FILE PKGSYSCONFDIR "/dtmd.conf"
/* pack_size + cluster_id + node_id + mcast_flag +  stream_port +  i_addr_family + ip_addr */
#define DTM_BCAST_HDR_SIZE 58

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

NCSCONTEXT gl_node_dis_task_hdl = 0;
NCSCONTEXT gl_serv_dis_task_hdl = 0;

static DTM_INTERNODE_CB _dtms_cb;
DTM_INTERNODE_CB *dtms_gl_cb = &_dtms_cb;

uint8_t initial_discovery_phase = true;

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */

/**
 * Function to init the dtm process
 *
 * @param dtms_cb
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
static uint32_t dtm_init(DTM_INTERNODE_CB * dtms_cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	memset(dtms_cb, 0, sizeof(DTM_INTERNODE_CB));

	if (ncs_leap_startup() != NCSCC_RC_SUCCESS) {
		LOG_ER("DTM: LEAP svcs startup failed \n");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* Initialize  control block */
	if ((rc = dtm_cb_init(dtms_cb)) != NCSCC_RC_SUCCESS) {
		rc = NCSCC_RC_FAILURE;
		LOG_ER("DTM: dtm_cb_init FAILED");
		goto done;
	}

 done:

	TRACE_LEAVE2("rc : %d", rc);
	return rc;
}

static uint32_t dtm_construct_bcast_hdr(DTM_INTERNODE_CB * dtms_cb, uint8_t *buf_ptr, int *pack_size)
{
	TRACE_ENTER();

	uint8_t *data = buf_ptr;

	*pack_size = DTM_BCAST_HDR_SIZE;

	ncs_encode_16bit(&data, *pack_size);
	ncs_encode_16bit(&data, dtms_cb->cluster_id);
	ncs_encode_32bit(&data, dtms_cb->node_id);
	ncs_encode_8bit(&data, dtms_cb->mcast_flag);
	ncs_encode_16bit(&data, dtms_cb->stream_port);
	ncs_encode_8bit(&data, (uint8_t)dtms_cb->i_addr_family);
	memcpy(data, dtms_cb->ip_addr, INET6_ADDRSTRLEN);

	TRACE_LEAVE();

	return NCSCC_RC_SUCCESS;

}

/**
 * Function to destroy node discovery thread
 *
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
static uint32_t dtm_destroy_node_discovery_task(void)
{
	TRACE_ENTER();

	if (m_NCS_TASK_STOP(gl_node_dis_task_hdl) != NCSCC_RC_SUCCESS) {
		LOG_ER("DTM: Stop of the Created Task-failed:gl_node_dis_task_hdl  \n");
	}

	if (m_NCS_TASK_RELEASE(gl_node_dis_task_hdl) != NCSCC_RC_SUCCESS) {
		LOG_ER("DTM: Stop of the Created Task-failed:gl_node_dis_task_hdl \n");
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

}

/**
 * Function to destroy service discovery thread
 *
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
static uint32_t dtm_destroy_service_discovery_task(void)
{
	TRACE_ENTER();

	if (m_NCS_TASK_STOP(gl_serv_dis_task_hdl) != NCSCC_RC_SUCCESS) {
		LOG_ER("DTM: Stop of the Created Task-failed:gl_serv_dis_task_hdl  \n");
	}

	if (m_NCS_TASK_RELEASE(gl_serv_dis_task_hdl) != NCSCC_RC_SUCCESS) {
		LOG_ER("DTM: Stop of the Created Task-failed:gl_serv_dis_task_hdl \n");
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

}

/**
 * Function to create & start node_discovery task
 *
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_node_discovery_task_create(void)
{
	uint32_t rc;
	TRACE_ENTER();

	int policy = SCHED_RR; /*root defaults */
	int max_prio = sched_get_priority_max(policy);
	int min_prio = sched_get_priority_min(policy);
	int prio_val = ((max_prio - min_prio) * 0.87); 

	rc = m_NCS_TASK_CREATE((NCS_OS_CB)node_discovery_process, NULL,
			       (char *)"OSAF_NODE_DISCOVERY", prio_val, policy, m_NODE_DISCOVERY_STACKSIZE,
			       &gl_node_dis_task_hdl);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("DTM: node_discovery thread CREATE failed");
		goto err;
	}

	TRACE("DTM: Created node_discovery thread ");

	/* now start the task */
	rc = m_NCS_TASK_START(gl_node_dis_task_hdl);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("DTM: node_discovery thread START failed : gl_node_dis_task_hdl ");
		goto err;
	}

	TRACE("DTM: Started node_discovery thread");
	TRACE_LEAVE2("rc : %d", rc);
	return rc;

 err:
	/* destroy the task */
	if (gl_node_dis_task_hdl) {
		/* release the task */
		m_NCS_TASK_RELEASE(gl_node_dis_task_hdl);

		gl_node_dis_task_hdl = 0;
	} else {

		/* Detach this thread and allow it to have its own life. This thread is going to exit on its own 
		   This macro is going to release the refecences to this thread in the LEAP */
		m_NCS_TASK_DETACH(gl_node_dis_task_hdl);
	}

	TRACE_LEAVE2("rc : %d", rc);
	return rc;
}

/**
 *  DTM process main function
 *
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
int main(int argc, char *argv[])
{
	int rc = -1;
	uint8_t send_bcast_buffer[255];
	int bcast_buf_len = 0;
	long int dis_time_out_usec = 0;
	long int dis_elapsed_time_usec = 0;
	DTM_INTERNODE_CB *dtms_cb = dtms_gl_cb;

	TRACE_ENTER();

	memset(send_bcast_buffer, 0, 255);

	daemonize(argc, argv);

	/*************************************************************/
	/* Set up CB stuff  */
	/*************************************************************/

	if (dtm_init(dtms_cb) != NCSCC_RC_SUCCESS) {
		LOG_ER("DTM: dtm_init failed");
		goto done3;
	}

	rc = dtm_read_config(dtms_cb, DTM_CONFIG_FILE);

	if (rc != 0) {

		LOG_ER("DTM:Error reading %s.  errno : %d", DTM_CONFIG_FILE, rc);
		goto done3;
	}

	/*************************************************************/
	/* Set up the initial bcast or mcast sender socket */
	/*************************************************************/

	if (dtms_cb->mcast_flag != true) {
		rc = dtm_dgram_bcast_sender(dtms_cb);
		if (NCSCC_RC_SUCCESS != rc) {
			LOG_ER("DTM:Set up the initial bcast  sender socket   failed rc : %d ", rc);
			goto done3;
		}
	} else {
		/*
		   0 
		   restricted to the same host 
		   1 
		   restricted to the same subnet 
		   32 
		   restricted to the same site 
		   64 
		   restricted to the same region 
		   128 
		   restricted to the same continent 
		   255 
		   unrestricted 
		 */
		rc = dtm_dgram_mcast_sender(dtms_cb, 64);	/*TODO */
		if (NCSCC_RC_SUCCESS != rc) {
			LOG_ER("DTM:Set up the initial mcast sender socket  failed rc : %d ", rc);
			goto done3;
		}
	}

	/*************************************************************/
	/* construct  bcast or mcast hdr */
	/*************************************************************/
	rc = dtm_construct_bcast_hdr(dtms_cb, send_bcast_buffer, &bcast_buf_len);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("DTM:construct  bcast or mcast hdr  failed rc : %d", rc);
		goto done3;
	}

	/*************************************************************/
	/* Set up the initial node_discovery_task  */
	/*************************************************************/
	rc = dtm_node_discovery_task_create();
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("DTM: node_discovery thread CREATE failed rc : %d ", rc);
		goto done2;
	}

	/*************************************************************/
	/* Set up the initialservice_discovery_task */
	/*************************************************************/
	rc = dtm_service_discovery_init();
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("DTM:service_discovery thread CREATE failed rc : %d ", rc);
		goto done1;
	}

	rc = nid_notify("DTM", NCSCC_RC_SUCCESS, NULL);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("DTM: nid_notify failed rc : %d ", rc);
		goto done1;
	}
	dis_time_out_usec = (dtms_cb->initial_dis_timeout * 1000000);

	do {
		/* Wait up to bcast_msg_freq seconds. */
		/* Check if stdin has input. */
		if (osaf_poll_one_fd(dtms_cb->dgram_sock_sndr, dtms_cb->bcast_msg_freq) < 0) {
			LOG_ER("DTM: poll failed");
			goto done1;

		}

		/* Broadcast msg string in datagram to clients every 250  m seconds */
		if (dtms_cb->mcast_flag == true) {

			rc = dtm_dgram_sendto_mcast(dtms_cb, send_bcast_buffer, bcast_buf_len);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("DTM: dtm_dgram_sendto_mcast Failed rc : %d \n", rc);
			}

		} else {

			rc = dtm_dgram_sendto_bcast(dtms_cb, send_bcast_buffer, bcast_buf_len);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("DTM: dtm_dgram_sendto_bcast Failed rc : %d \n", rc);
			}

		}

		dis_elapsed_time_usec = dis_elapsed_time_usec + (dtms_cb->bcast_msg_freq * 1000);

	} while (dis_elapsed_time_usec <= dis_time_out_usec);
	/*************************************************************/
	/* Keep waiting forever */
	/*************************************************************/
	initial_discovery_phase = false;
	while (1) {
		m_NCS_TASK_SLEEP(0xfffffff0);
		/* m_NCS_TASK_SLEEP(30000); */
	}
 done1:
	LOG_ER("DTM : dtm_destroy_service_discovery_task exiting...");
	/* Destroy node_discovery_task  task */
	rc = dtm_destroy_service_discovery_task();
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("DTM: service_discover Task Destruction Failed rc : %d \n", rc);
	}
 done2:
	/* Destroy receiving task */
	rc = dtm_destroy_node_discovery_task();
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("DTM: node_discovery Task Destruction Failed rc : %d \n", rc);
	}

 done3:
	TRACE_LEAVE();
	(void)nid_notify("DTM", NCSCC_RC_FAILURE, NULL);
	exit(1);

}

/**
 *  DTMS CB dump
 *
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
void dtms_cb_dump(void)
{

	TRACE("DTM : ***************************************************************************************");

	TRACE("DTM: My DB Snapshot ");

	TRACE("DTM : ***********************************************************************************");
}
