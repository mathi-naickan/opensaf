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
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:

  This module is the main module for Availability Director. This module
  deals with the initialisation of AvD.

..............................................................................

  FUNCTIONS INCLUDED in this module:

  avd_init_proc - entry function to AvD thread or task.
  avd_initialize - creation and starting of AvD task/thread.
  avd_tmr_cl_init_func - the event handler for AMF cluster timer expiry.
  avd_destroy - the destruction of the AVD module.
  avd_lib_req - DL SE API for init and destroy of the AVD module

  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#include <poll.h>
#include <saImmOm.h>
#include <immutil.h>
#include <logtrace.h>
#include <rda_papi.h>

#include <avd.h>
#include <avd_hlt.h>
#include <avd_imm.h>
#include <avd_clm.h>
#include <avd_cluster.h>
#include <avd_sutype.h>

/* 
** The singleton AVD Cluster Control Block. Statically allocated which is
** good for debugging core dumps
*/
static AVD_CL_CB _avd_cb;

/* A handy global reference to the control block */
AVD_CL_CB *avd_cb = &_avd_cb;

/**
 * Callback from RDA. Post a message to the AVD mailbox.
 * @param notused
 * @param cb_info
 * @param error_code
 */
static void rda_cb(uns32 notused, PCS_RDA_CB_INFO *cb_info, PCSRDA_RETURN_CODE error_code)
{
	(void) notused;

	TRACE_ENTER();

	if (((avd_cb->avail_state_avd == SA_AMF_HA_STANDBY) ||
	     (avd_cb->avail_state_avd == SA_AMF_HA_QUIESCED)) &&
	    (cb_info->info.io_role == PCS_RDA_ACTIVE)) {

		uns32 rc;
		AVD_EVT *evt;

		evt = malloc(sizeof(AVD_EVT));
		if (evt == NULL) {
			LOG_ER("malloc failed");
			assert(0);
		}
		evt->rcv_evt = AVD_EVT_ROLE_CHANGE;
		evt->info.avd_msg = malloc(sizeof(AVD_D2D_MSG));
		if (evt->info.avd_msg == NULL) {
			LOG_ER("malloc failed");
			assert(0);
		}
		evt->info.avd_msg->msg_type = AVD_D2D_CHANGE_ROLE_REQ;
		evt->info.avd_msg->msg_info.d2d_chg_role_req.cause = AVD_FAIL_OVER;
		evt->info.avd_msg->msg_info.d2d_chg_role_req.role = cb_info->info.io_role;

		rc = ncs_ipc_send(&avd_cb->avd_mbx, (NCS_IPC_MSG *)evt, MDS_SEND_PRIORITY_HIGH);
		assert(rc == NCSCC_RC_SUCCESS);
	} else
		TRACE("Ignoring change from %u to %u", avd_cb->avail_state_avd, cb_info->info.io_role);

	TRACE_LEAVE();
}

/**
 * Initialize everything...
 * 
 * @return uns32 NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 */
uns32 avd_initialize(void)
{
	AVD_CL_CB *cb = avd_cb;
	NCS_PATRICIA_PARAMS patricia_params = {0};
	int rc = NCSCC_RC_FAILURE;
	SaVersionT ntfVersion = { 'A', 0x01, 0x01 };
	SaAmfHAStateT role;
	char *val;

	TRACE_ENTER();

	/* run the class constructors */
	avd_apptype_constructor();
	avd_app_constructor();
	avd_compglobalattrs_constructor();
	avd_compcstype_constructor();
	avd_comp_constructor();
	avd_ctcstype_constructor();
	avd_comptype_constructor();
	avd_cluster_constructor();
	avd_cstype_constructor();
	avd_csi_constructor();
	avd_csiattr_constructor();
	avd_hctype_constructor();
	avd_hc_constructor();
	avd_node_constructor();
	avd_ng_constructor();
	avd_nodeswbundle_constructor();
	avd_sgtype_constructor();
	avd_sg_constructor();
	avd_svctypecstypes_constructor();
	avd_svctype_constructor();
	avd_si_constructor();
	avd_sirankedsu_constructor();
	avd_sidep_constructor();
	avd_sutcomptype_constructor();
	avd_su_constructor();
	avd_sutype_constructor();

	if (ncs_ipc_create(&cb->avd_mbx) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_ipc_create FAILED");
		goto done;
	}

	if (ncs_ipc_attach(&cb->avd_mbx) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_ipc_attach FAILED");
		goto done;
	}

	cb->init_state = AVD_INIT_BGN;
	cb->swap_switch = SA_FALSE;
	cb->stby_sync_state = AVD_STBY_IN_SYNC;
	cb->sync_required = TRUE;
	
	cb->heartbeat_tmr.is_active = FALSE;
	cb->heartbeat_tmr.type = AVD_TMR_SND_HB;
	cb->heartbeat_tmr_period = AVSV_DEF_HB_PERIOD;

	if ((val = getenv("AVSV_HB_PERIOD")) != NULL) {
		SaTimeT heartbeat_tmr_period = strtoll(val, NULL, 0);
		if ((heartbeat_tmr_period < 100 * SA_TIME_ONE_MILLISECOND) ||
		    (heartbeat_tmr_period > 100 * SA_TIME_ONE_SECOND)) {
			LOG_WA("Value of AVSV_HB_PERIOD unreasonable (%llu), using default (%llu)",
				heartbeat_tmr_period, cb->heartbeat_tmr_period);
		} else
			cb->heartbeat_tmr_period = heartbeat_tmr_period;
	}

	patricia_params.key_size = sizeof(SaClmNodeIdT);
	if (ncs_patricia_tree_init(&cb->node_list, &patricia_params) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_patricia_tree_init FAILED");
		goto done;
	}

	if ((rc = rda_get_role(&role)) != NCSCC_RC_SUCCESS) {
		LOG_ER("rda_get_role FAILED");
		goto done;
	}

	cb->avail_state_avd = role;

	/* get the node id of the node on which the AVD is running. */
	cb->node_id_avd = m_NCS_GET_NODE_ID;

	if (avd_mds_init(cb) != NCSCC_RC_SUCCESS) {
		LOG_ER("avd_mds_init FAILED");
		goto done;
	}

	if (NCSCC_RC_FAILURE == avsv_mbcsv_register(cb)) {
		LOG_ER("avsv_mbcsv_register FAILED");
		goto done;
	}

	if (avd_clm_init() != SA_AIS_OK) {
		LOG_EM("avd_clm_init FAILED");
		goto done;
	}

	if (avd_imm_init(cb) != SA_AIS_OK) {
		LOG_ER("avd_imm_init FAILED");
		goto done;
	}

	if ((rc = saNtfInitialize(&cb->ntfHandle, NULL, &ntfVersion)) != SA_AIS_OK) {
		LOG_ER("saNtfInitialize Failed (%u)", rc);
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	if (NCSCC_RC_SUCCESS != avd_mds_set_vdest_role(cb, role)) {
		LOG_ER("avd_mds_set_vdest_role FAILED");
		goto done;
	}

	if (NCSCC_RC_SUCCESS != avsv_set_ckpt_role(cb, role)) {
		LOG_ER("avsv_set_ckpt_role FAILED");
		goto done;
	}

	if ((rc = rda_register_callback(0, rda_cb)) != NCSCC_RC_SUCCESS) {
		LOG_ER("rda_register_callback FAILED %u", rc);
		goto done;
	}

	if (role == SA_AMF_HA_ACTIVE) {
		rc = avd_active_role_initialization(cb, role);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("avd_active_role_initialization FAILED");
			goto done;
		}
	}

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE();
	return rc;
}

