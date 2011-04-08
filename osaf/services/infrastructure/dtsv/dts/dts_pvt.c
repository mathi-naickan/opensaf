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

    This has the private functions of the Flex Log Server (DTS).
  ..............................................................................

  FUNCTIONS INCLUDED in this module:
  dts_do_evts               - Infinite loop services the passed SYSF_MBX
  dts_do_evt                - The Master DTS dispatch loop functions.
  dts_register_service      - Register service with DTS.
  dts_unregister_service    - De-register service.
  dts_log_data              - Log data.
  dtsv_log_msg              - Message logging function.
  dts_find_reg              - Find service match.
  dts_default_policy_set    - Initialize the default policy.
  dts_global_policy_set     - Initialize the global policy.
  dts_default_node_policy_set - Initialize the per node policy.
  dts_default_svc_policy_set  - Initialize the node per service policy.
  dts_new_log_file_create     - Create new log file.
  dts_send_filter_config_msg  - Send filter configuration message.
  dts_get_next_node_entry     - Get next node entry.
  dts_get_next_svc_entry      - Get next service entry.

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/

#include <configmake.h>

#include "dts.h"
#include "ncssysf_mem.h"
#include "dts_imm.h"
#include <poll.h>
#include <ctype.h>

#define FD_USR1 0
#define FD_AMF 0
#define FD_MBCSV 1
#define FD_MBX 2
#define FD_IMM 3

static nfds_t nfds =  FD_IMM + 1;

#define m_DTS_COMP_NAME_FILE PKGLOCALSTATEDIR "/dts_comp_name"

static struct pollfd fds[4];
static uns32 dts_stby_initialize(DTS_CB *cb);

/*****************************************************************************

  The master DTS dispatch loop functions

  OVERVIEW:   These functions are used for processing the events in the DTS 
              mailbox. Events includes the one received from the DTA and 
              the DTS for dumping sequencing messages.

  PROCEDURE NAME: dts_do_evts & dts_do_evt

  DESCRIPTION:       

     dts_do_evts      Infinite loop services the passed SYSF_MBX
     dts_do_evt       Master Dispatch function and services off DTS work queue

*****************************************************************************/

/*****************************************************************************
  dts_do_evts
*****************************************************************************/
void dts_do_evts(SYSF_MBX *mbx)
{
	uns32 status;
	NCS_SEL_OBJ mbx_fd;
	SaAisErrorT saf_status = SA_AIS_OK;

	mbx_fd = ncs_ipc_get_sel_obj(mbx);

	fds[FD_MBX].fd = mbx_fd.rmv_obj;
	fds[FD_MBX].events = POLLIN;
	fds[FD_USR1].fd = dts_cb.sighdlr_sel_obj.rmv_obj;
	fds[FD_USR1].events = POLLIN;
	fds[FD_MBCSV].fd = dts_cb.mbcsv_sel_obj;
	fds[FD_MBCSV].events = POLLIN;

	while (1) {

		if (dts_cb.immOiHandle != 0) {
			fds[FD_IMM].fd = dts_cb.imm_sel_obj;
			fds[FD_IMM].events = POLLIN;
			nfds = FD_IMM + 1;
		} else {
			nfds = FD_IMM;
		}

		/* wait for the requests indefinitely */
		int ret = poll(fds, nfds, -1);

		if (ret == -1) {
			if (errno == EINTR)
				continue;
			m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "poll failed");
		}

		/* MBCSv FD is selected */
		if (fds[FD_MBCSV].revents & POLLIN) {
			status = dtsv_mbcsv_dispatch(&dts_cb, SA_DISPATCH_ALL);
			if (status != NCSCC_RC_SUCCESS)
				m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_do_evts: Error received");
		}

		/* AMF FD is selected */
		if (fds[FD_AMF].revents & POLLIN) {
			/* Initialize, get selection object and register with AMF */
			if (dts_cb.amf_hdl != 0) {
				saf_status = saAmfDispatch(dts_cb.amf_hdl, SA_DISPATCH_ALL);
				if (saf_status != SA_AIS_OK) {
					dts_log(NCSFL_SEV_ERROR, "saAmfDispatch failed \n");
					/* log the error  */
					m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_do_evts: AMF Dispatch failing");
				}
			} else {
				status = dts_amf_register(&dts_cb);
				if (status == NCSCC_RC_SUCCESS) {
					TRACE("AMF registration success\n");
					fds[FD_AMF].fd = dts_cb.dts_amf_sel_obj;
				} else {
					LOG_ER("AMF registration failed :%d, exiting", status);
					exit(1);
				}
			}	/* Else amf_hdl!=0 */
		}
		/* Process the messages on the Mail box */
		if (fds[FD_MBX].revents & POLLIN) {
			/* process native requests of MASv */
			status = dts_do_evt((DTSV_MSG *)m_NCS_IPC_NON_BLK_RECEIVE(mbx, NULL));
			if (status != NCSCC_RC_SUCCESS) {
				/*m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
				  "dts_do_evts: Error received....debug!!"); */
			}
		}

		if (fds[FD_IMM].revents & POLLIN) {
			/* dispatch all the IMMSv pending callbacks */
			saf_status = saImmOiDispatch(dts_cb.immOiHandle, SA_DISPATCH_ONE);
			if (saf_status == SA_AIS_ERR_BAD_HANDLE) {
				m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "IMMSv Dispatch failed");
				dts_cb.imm_init_done = FALSE;
				saImmOiFinalize(dts_cb.immOiHandle);
				dts_cb.immOiHandle = 0;
				dts_imm_reinit_bg(&dts_cb);
			} else if (saf_status != SA_AIS_OK) {
				dts_log(NCSFL_SEV_ERROR, "saImmOiDispatch FAILED: %u", saf_status);
				break;
			}
		}

	}			/*end of while */
}

/*****************************************************************************
   dts_do_evt
*****************************************************************************/

uns32 dts_do_evt(DTSV_MSG *msg)
{
	DTS_CB *inst = &dts_cb;
	uns32 rc = NCSCC_RC_SUCCESS;

	if (SA_AMF_HA_ACTIVE == inst->ha_state)
		/*if ((SA_AMF_HA_ACTIVE == inst->ha_state) ||
		   (SA_AMF_HA_QUIESCED == inst->ha_state)) */
	{
		switch (msg->msg_type) {
		case DTS_DTA_EVT_RCV:
			rc = dts_handle_dta_event(msg);
			break;

		case DTA_REGISTER_SVC:
			rc = dts_register_service(msg);
			break;

		case DTA_UNREGISTER_SVC:
			rc = dts_unregister_service(msg);
			break;

		case DTA_LOG_DATA:
			if ((rc = dts_log_my_data(msg)) == NCSCC_RC_CONTINUE)
				return NCSCC_RC_SUCCESS;
			break;

		case DTSV_DUMP_SEQ_MSGS:
			rc = dts_dump_seq_msg(inst, FALSE);
			break;

			/* Added a case for handling msg_type DTS_QUIESCED_CMPLT.
			 * Here set the mbcsv ckpt role to quiesced and change cb->ha_state
			 * Also send SaAmfResponse now */
		case DTS_QUIESCED_CMPLT:
			m_DTS_LK(&inst->lock);
			m_LOG_DTS_LOCK(DTS_LK_LOCKED, &inst->lock);

			/* Change MBCSv ckpt role */
			if (dtsv_set_ckpt_role(inst, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS) {
				m_LOG_DTS_API(DTS_CKPT_CHG_FAIL);
				m_DTS_UNLK(&inst->lock);
				m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
				return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_do_evt: MBCSv ckpt role change failed");
			}

			/* Change ha_state of dts_cb to quiesced now */
			inst->ha_state = SA_AMF_HA_QUIESCED;

			/* Clear sequencing buffer just before going to quiesced */
			/* Clear the sequencing buffer before going to quiesced */
			if (TRUE == inst->g_policy.g_enable_seq)
				dts_disable_sequencing(inst);

			/* Close all open files now */
			dts_close_files_quiesced();

			/* Send a SaAmfResponse now, using amf hdl & invocation in dts_cb */
			saAmfResponse(inst->amf_hdl, inst->csi_cb_invocation, SA_AIS_OK);
			/*m_LOG_DTS_API(DTS_CSI_SET_CB_RESP); */

			m_DTS_UNLK(&inst->lock);
			m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
			break;

			/* Handle spec reload msg */
		case DTS_SPEC_RELOAD:
			/*rc = dts_ascii_spec_reload(inst);
			   m_LOG_DTS_API(DTS_SPEC_RELOAD_CMD); */
			break;

		case DTS_PRINT_CONFIG:
			rc = dts_print_current_config(inst);
			break;

#if (DTA_FLOW == 1)
		case DTA_FLOW_CONTROL:
			if (dts_mds_send_msg(msg, msg->dest_addr, inst->mds_hdl) != NCSCC_RC_SUCCESS) {
				m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
					       "dts_do_evt: Failed to send ACK for Flow control message");
			}
			break;

		case DTS_CONGESTION_HIT:
			ncs_logmsg(NCS_SERVICE_ID_DTSV, DTS_LID_FLOW_UP, 0, NCSFL_LC_HEADLINE, NCSFL_SEV_WARNING, "TLL",
				   m_NCS_NODE_ID_FROM_MDS_DEST(msg->dest_addr), (uns32)msg->dest_addr);
			break;

		case DTS_CONGESTION_CLEAR:
			ncs_logmsg(NCS_SERVICE_ID_DTSV, DTS_LID_FLOW_DOWN, 0, NCSFL_LC_HEADLINE, NCSFL_SEV_WARNING,
				   "TLL", m_NCS_NODE_ID_FROM_MDS_DEST(msg->dest_addr), (uns32)msg->dest_addr);
			break;
#endif

		default:
			rc = m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_do_evt: Wrong message type received");
			break;

		}
	}
	if ((SA_AMF_HA_ACTIVE == inst->ha_state) || (SA_AMF_HA_STANDBY == inst->ha_state)) {
		if (msg->msg_type == DTS_IMMND_EVT_RCV)
			rc = dts_handle_immnd_event(msg);
	}

	if (0 != msg) {
		if ((rc != NCSCC_RC_SUCCESS) && (msg->msg_type == DTA_LOG_DATA))
			if (msg->data.data.msg.log_msg.uba.ub != NULL)
				m_MMGR_FREE_BUFR_LIST(msg->data.data.msg.log_msg.uba.ub);

		m_MMGR_FREE_DTSV_MSG(msg);
	}

	return rc;
}

/**************************************************************************\
 Function: dts_handle_dta_event

 Purpose:  Function used for handling the DTA up down events received
           from MDS.

 Input:    msg : Event message received from the MDS.

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  
\**************************************************************************/
uns32 dts_handle_dta_event(DTSV_MSG *msg)
{

	DTS_CB *inst = &dts_cb;

	m_DTS_LK(&inst->lock);
	m_LOG_DTS_LOCK(DTS_LK_LOCKED, &inst->lock);

	m_LOG_DTS_SVC_PRVDR(DTS_SP_MDS_RCV_EVT);

	switch (msg->data.data.evt.change) {	/* Review change type */
	case NCSMDS_DOWN:
		m_LOG_DTS_DBGSTRLL(DTS_SERVICE, "Received DTA down event for :", msg->node, (uns32)msg->dest_addr);
		dts_set_dta_up_down(msg->node, msg->dest_addr, FALSE);
		break;

	case NCSMDS_UP:
		m_LOG_DTS_DBGSTRLL(DTS_SERVICE, "Received DTA up event for :", msg->node, (uns32)msg->dest_addr);
		dts_set_dta_up_down(msg->node, msg->dest_addr, TRUE);
		break;

	default:
		break;
	}
	m_DTS_UNLK(&inst->lock);
	m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);

	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
 Function: dts_reg_with_imm

 Purpose: Owns all DTS log policie's objects and reads themi from IMMSv. 

 Returns:  void

 Notes:  
\**************************************************************************/
void dts_reg_with_imm(DTS_CB *inst)
{
	dts_imm_declare_implementer(inst);
	/* get default global configuration from global scalar object */
	if (dts_configure_global_policy() == NCSCC_RC_FAILURE) {
		LOG_ER("Failed to load global log policy object from IMMSv, exiting");
		exit(1);
	}

	/* loads all the NodeLogPolicy objects from IMMSv */
	dts_read_log_policies("OpenSAFDtsvNodeLogPolicy");
	/* loads all the ServiceLogPolicy objects from IMMSv */
	dts_read_log_policies("OpenSAFDtsvServiceLogPolicy");
}

/**************************************************************************\
 Function: dts_register_service

 Purpose:  Function used for registering the service with the DTS. Function 
           gets called in the DTS thread and processes the registration 
           request received from the DTA.

 Input:    msg : Registration message received from the DTA

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  
\**************************************************************************/
uns32 dts_register_service(DTSV_MSG *msg)
{
	DTS_CB *inst = &dts_cb;
	SVC_KEY key, nt_key;
	DTS_SVC_REG_TBL *svc, *node_reg;
	DTA_DEST_LIST *to_reg;
	MDS_DEST dta_key;
	NCS_BOOL dta_present = FALSE;
	SYSF_ASCII_SPECS *spec_entry;
	ASCII_SPEC_INDEX spec_key;
	SPEC_ENTRY *per_dta_svc_spec = NULL;
	ASCII_SPEC_LIB *lib_hdl = NULL;

	key.node = msg->node;
	key.ss_svc_id = 0;
	/*  Network order key added */
	nt_key.node = m_NCS_OS_HTONL(msg->node);
	nt_key.ss_svc_id = 0;

	m_DTS_LK(&inst->lock);
	m_LOG_DTS_LOCK(DTS_LK_LOCKED, &inst->lock);

	m_LOG_DTS_EVT(DTS_EV_SVC_REG_REQ_RCV, msg->data.data.reg.svc_id, msg->node, (uns32)msg->dest_addr);

	/*
	 * Check whether the Node exist in the node registration table.
	 * If Yes then do nothing.
	 * If NO then create new entry in the table. Initialize it with the default 
	 */
	/*  Network order key added */
	if ((node_reg = (DTS_SVC_REG_TBL *)ncs_patricia_tree_get(&inst->svc_tbl, (const uns8 *)&nt_key)) == NULL) {
		node_reg = m_MMGR_ALLOC_SVC_REG_TBL;
		if (node_reg == NULL) {
			m_DTS_UNLK(&inst->lock);
			m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_register_service: Memory allocation failed");
		}
		memset(node_reg, '\0', sizeof(DTS_SVC_REG_TBL));
		node_reg->my_key.node = key.node;
		node_reg->my_key.ss_svc_id = 0;
		/*  Network order key added */
		node_reg->ntwk_key.node = nt_key.node;
		node_reg->ntwk_key.ss_svc_id = 0;
		node_reg->node.key_info = (uns8 *)&node_reg->ntwk_key;

		node_reg->per_node_logging = NODE_LOGGING;
		node_reg->my_node = node_reg;

		dts_default_node_policy_set(&node_reg->svc_policy, &node_reg->device, key.node);
		if (ncs_patricia_tree_add(&inst->svc_tbl, (NCS_PATRICIA_NODE *)node_reg) != NCSCC_RC_SUCCESS) {
			m_LOG_DTS_EVT(DTS_EV_SVC_REG_ENT_ADD_FAIL, key.ss_svc_id, key.node, (uns32)msg->dest_addr);
			m_LOG_DTS_EVT(DTS_EV_SVC_REG_FAILED, key.ss_svc_id, key.node, (uns32)msg->dest_addr);

			if (NULL != node_reg)
				m_MMGR_FREE_SVC_REG_TBL(node_reg);

			m_DTS_UNLK(&inst->lock);
			m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
					      "dts_register_service: Failed to add node registration in Patricia tree");
		}
	}

	key.ss_svc_id = msg->data.data.reg.svc_id;
	/*  Network order key added */
	nt_key.ss_svc_id = m_NCS_OS_HTONL(msg->data.data.reg.svc_id);

	dta_key = msg->dest_addr;

	if ((to_reg = (DTA_DEST_LIST *)ncs_patricia_tree_get(&inst->dta_list, (const uns8 *)&dta_key)) != NULL) {
		dta_present = TRUE;
		/* Adjust the pointer to to_reg with the offset */
		to_reg = (DTA_DEST_LIST *)((long)to_reg - DTA_DEST_LIST_OFFSET);
	}
	/*
	 * Check whether the Service exist in the service registration table.
	 * If Yes then add the DTA v-card in the v-card queue.
	 * If NO then create new entry in the table. Initialize it with the default.
	 * Enqueue the v-card in the v-card table.
	 */
	/*  Network order key added */
	if ((svc = (DTS_SVC_REG_TBL *)ncs_patricia_tree_get(&inst->svc_tbl, (const uns8 *)&nt_key)) != NULL) {
		/*Search for dta entry already present in svc->v_cd_list */
		if (dts_find_dta(svc, &dta_key) != NULL) {
			m_DTS_UNLK(&inst->lock);
			m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
			m_LOG_DTS_EVT(DTS_EV_SVC_ALREADY_REG, key.ss_svc_id, key.node, (uns32)msg->dest_addr);
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
					      "dts_register_service: Service is already registered with DTS");
		}

		/* Check whether the DTA_DEST_TBL already exists in pat tree */
		/* If the entry doesn't already exist create a new one and enqueue to 
		 * the v_cd_list for the DTS_SVC_REG_TBL */
		if (dta_present == FALSE) {
			to_reg = m_MMGR_ALLOC_VCARD_TBL;
			if (to_reg == NULL) {
				m_DTS_UNLK(&inst->lock);
				m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
				return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
						      "dts_register_service: Memory allocation failed");
			}
			memset(to_reg, '\0', sizeof(DTA_DEST_LIST));
			to_reg->dta_addr = dta_key;
			to_reg->node.key_info = (uns8 *)&to_reg->dta_addr;

			/* Smik - Add the new DTA_DEST_LIST to the patricia tree */
			if (ncs_patricia_tree_add(&inst->dta_list, (NCS_PATRICIA_NODE *)&to_reg->node) !=
			    NCSCC_RC_SUCCESS) {
				m_LOG_DTS_EVT(DTS_EV_DTA_DEST_ADD_FAIL, 0, key.node, (uns32)to_reg->dta_addr);
				m_LOG_DTS_EVT(DTS_EV_SVC_REG_FAILED, key.ss_svc_id, key.node, (uns32)msg->dest_addr);
				if (NULL != to_reg)
					m_MMGR_FREE_VCARD_TBL(to_reg);

				m_DTS_UNLK(&inst->lock);
				m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
				return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
						      "dts_register_service: Failed to add DTA list in Patricia tree");
			}
			m_LOG_DTS_EVT(DTS_EV_DTA_DEST_ADD_SUCC, 0, key.node, (uns32)to_reg->dta_addr);
		}

		/*end of dta_present == false */
		/* if entry already exits for DTA in the patricia tree just enqueue 
		 * the entry to the v_cd_list */
		to_reg->dta_up = TRUE;
		to_reg->updt_req = FALSE;
		/*Add this DTS_SVC_REG_TBL to the list of services for specific DTA */
		dts_add_svc_to_dta(to_reg, svc);
		m_LOG_DTS_EVT(DTS_EV_DTA_SVC_ADD, key.ss_svc_id, key.node, (uns32)to_reg->dta_addr);

		/* Add the node pointer to svc's my_node if not already filled */
		if (svc->my_node == NULL)
			svc->my_node = node_reg;
	} else {
		/* Add to patricia tree; then create link list and add the new element to it */
		svc = m_MMGR_ALLOC_SVC_REG_TBL;
		if (svc == NULL) {
			m_DTS_UNLK(&inst->lock);
			m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_register_service: Memory allocation failed");
		}
		memset(svc, '\0', sizeof(DTS_SVC_REG_TBL));
		svc->my_key.ss_svc_id = key.ss_svc_id;
		svc->my_key.node = key.node;
		svc->my_node = node_reg;
		/*  Network order key added */
		svc->ntwk_key.ss_svc_id = nt_key.ss_svc_id;
		svc->ntwk_key.node = nt_key.node;
		svc->node.key_info = (uns8 *)&svc->ntwk_key;

		/* Check whether the DTA_DEST_TBL already exists in pat tree */
		/* If the entry doesn't already exist create a new one and enqueue to 
		 * the v_cd_list for the DTS_SVC_REG_TBL */
		if (dta_present == FALSE) {
			to_reg = m_MMGR_ALLOC_VCARD_TBL;
			if (to_reg == NULL) {
				m_DTS_UNLK(&inst->lock);
				m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
				if (svc != NULL)
					m_MMGR_FREE_SVC_REG_TBL(svc);
				return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
						      "dts_register_service: Memory allocation failed");
			}
			memset(to_reg, '\0', sizeof(DTA_DEST_LIST));
			to_reg->dta_addr = dta_key;
			to_reg->node.key_info = (uns8 *)&to_reg->dta_addr;

			if (ncs_patricia_tree_add(&inst->dta_list, (NCS_PATRICIA_NODE *)&to_reg->node) !=
			    NCSCC_RC_SUCCESS) {
				m_LOG_DTS_EVT(DTS_EV_DTA_DEST_ADD_FAIL, key.ss_svc_id, key.node,
					      (uns32)to_reg->dta_addr);
				m_LOG_DTS_EVT(DTS_EV_SVC_REG_FAILED, key.ss_svc_id, key.node, (uns32)msg->dest_addr);
				if (NULL != to_reg)
					m_MMGR_FREE_VCARD_TBL(to_reg);

				if (svc != NULL)
					m_MMGR_FREE_SVC_REG_TBL(svc);

				m_DTS_UNLK(&inst->lock);
				m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
				return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
						      "dts_register_service: Failed to add DTA list in Patricia tree");
			}
			m_LOG_DTS_EVT(DTS_EV_DTA_DEST_ADD_SUCC, key.ss_svc_id, key.node, (uns32)to_reg->dta_addr);
		}
		/* end of if dta_present==FALSE */
		to_reg->dta_up = TRUE;
		to_reg->updt_req = FALSE;
		/* Add the DTA to svc->v_cd_list */
		dts_add_svc_to_dta(to_reg, svc);
		m_LOG_DTS_EVT(DTS_EV_DTA_SVC_ADD, key.ss_svc_id, key.node, (uns32)to_reg->dta_addr);

		/* newly created service, set all the policies to default 
		 * then add new entry to the patricia tree */
		svc->per_node_logging = FALSE;
		/*svc->num_svcs         = 0; */
		dts_default_svc_policy_set(svc);
		if (ncs_patricia_tree_add(&inst->svc_tbl, (NCS_PATRICIA_NODE *)svc) != NCSCC_RC_SUCCESS) {
			m_LOG_DTS_EVT(DTS_EV_SVC_REG_ENT_ADD_FAIL, key.ss_svc_id, key.node, (uns32)msg->dest_addr);
			m_LOG_DTS_EVT(DTS_EV_SVC_REG_FAILED, key.ss_svc_id, key.node, (uns32)msg->dest_addr);

			if (dts_del_svc_frm_dta(to_reg, svc) == NULL)
				m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
					       "dts_register_service: No service entry in dta's service list");

			if (to_reg->svc_list == NULL)
				if (to_reg != NULL)
					m_MMGR_FREE_VCARD_TBL(to_reg);

			if (svc != NULL)
				m_MMGR_FREE_SVC_REG_TBL(svc);

			m_DTS_UNLK(&inst->lock);
			m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
					      "dts_register_service: Failed to add service entry in patricia tree");
		}

		/* Smik - Send Sync update for DTS_SVC_REG_TBL */
		if (dtsv_send_ckpt_data
		    (inst, NCS_MBCSV_ACT_ADD, (MBCSV_REO_HDL)(long)svc, DTSV_CKPT_DTS_SVC_REG_TBL_CONFIG,
		     NCS_MBCSV_SND_SYNC) != NCSCC_RC_SUCCESS)
			m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
				       "dts_service_register: MDS Sync send to Stby failed for Service registration");

	}			/*end of else */

	/*Enqueue dta entry in svc's v_cd_list */
	dts_enqueue_dta(svc, to_reg);
	m_LOG_DTS_EVT(DTS_EV_SVC_DTA_ADD, key.ss_svc_id, key.node, (uns32)to_reg->dta_addr);

	/* Versioning Support: Update DTS CB with ASCII_SPEC to be loaded 
	 * The following data is necessary for STBY DTS to load ASCII_SPEC and
	 * form the SPEC list. If loading of ASCII_SPEC fails here it'll also fail
	 * at STBY DTS too.
	 */
	strcpy(inst->last_spec_loaded.svc_name, msg->data.data.reg.svc_name);
	inst->last_spec_loaded.version = msg->data.data.reg.version;

	/* Now send the sync update for DTA_DEST_LIST to Standby */
	if (dtsv_send_ckpt_data
	    (inst, NCS_MBCSV_ACT_ADD, (MBCSV_REO_HDL)(long)to_reg, DTSV_CKPT_DTA_DEST_LIST_CONFIG,
	     NCS_MBCSV_SND_SYNC) != NCSCC_RC_SUCCESS)
		m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
			       "dts_register_service: MDS Sync send to Stby failed for Service registration");

	/* Versioning change - Reset the DTS CB last_spec_loaded to 0 */
	memset(&inst->last_spec_loaded, '\0', sizeof(SPEC_CKPT));

	/* Versioning support -  form the key to index to existing ASCII_SPEC 
	 * patricia tree 
	 */
	memset(&spec_key, '\0', sizeof(ASCII_SPEC_INDEX));
	spec_key.svc_id = svc->ntwk_key.ss_svc_id;
	/* Get version from msg */
	spec_key.ss_ver = msg->data.data.reg.version;

	/* New code having versioning support */
	/* Call function to load the ASCII_SPEC library and register the spec */
	lib_hdl = (ASCII_SPEC_LIB *)dts_ascii_spec_load(msg->data.data.reg.svc_name, msg->data.data.reg.version, 1);

	/* Now the call to the above functions would've loaded the ASCII_SPEC
	 * table. So a simple look-up in the ASCII_SPEC patricia tree would 
	 * yield the necessary pointer
	 */
	if ((spec_entry =
	     (SYSF_ASCII_SPECS *)ncs_patricia_tree_get(&dts_cb.svcid_asciispec_tree,
						       (const uns8 *)&spec_key)) != NULL) {
		/* Add an ASCII_SPEC entry for each service registration.
		 * So any log message will directly index to this entry and get the
		 * relevant spec pointer to use
		 */
		per_dta_svc_spec = m_MMGR_ALLOC_DTS_SVC_SPEC;
		if (per_dta_svc_spec == NULL) {
			m_DTS_UNLK(&inst->lock);
			/* Do rest of cleanup, cleaning service regsitration table etc. */
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_register_service: Memory allocation failed");
		}
		memset(per_dta_svc_spec, '\0', sizeof(SPEC_ENTRY));
		per_dta_svc_spec->dta_addr = dta_key;
		per_dta_svc_spec->spec_struct = spec_entry;
		per_dta_svc_spec->lib_struct = lib_hdl;
		strcpy(per_dta_svc_spec->svc_name, msg->data.data.reg.svc_name);

		/* Add to the svc reg tbl's spec list */
		/* point next to the rest of list */
		per_dta_svc_spec->next_spec_entry = svc->spec_list;
		svc->spec_list = per_dta_svc_spec;	/*Add new struct to start of list */

		/* Increment use count */
		spec_entry->use_count++;
	} else {
		m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
			       "dts_register_service: ASCII_SPEC with given svc name & version couldn't be associated properly. Cross-check the svc_name and version used in registration, ASCII_SPEC table and library name");
	}

	/* Now configure the service which is just registered with DTS */
	msg->vrid = inst->vrid;
	msg->msg_type = DTS_SVC_REG_CONF;

	msg->data.data.reg_conf.msg_fltr.svc_id = key.ss_svc_id;
	msg->data.data.reg_conf.msg_fltr.enable_log = svc->svc_policy.enable;
	msg->data.data.reg_conf.msg_fltr.category_bit_map = svc->svc_policy.category_bit_map;
	msg->data.data.reg_conf.msg_fltr.severity_bit_map = svc->svc_policy.severity_bit_map;
	/* No need for policy handles */
	/*msg->data.data.reg_conf.msg_fltr.policy_hdl = svc->svc_hdl; */

	m_DTS_UNLK(&inst->lock);
	m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
	if (dts_mds_send_msg(msg, dta_key, inst->mds_hdl) != NCSCC_RC_SUCCESS) {
		m_LOG_DTS_SVC_PRVDR(DTS_SP_MDS_SND_MSG_FAILED);
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_register_service: DTS: MDS send failed");
	}
	m_LOG_DTS_EVT(DTS_EV_SVC_REG_SUCCESSFUL, key.ss_svc_id, key.node, (uns32)dta_key);
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
 Function: dts_unregister_service

 Purpose:  Function used for de-registering the service from the DTS. Function 
           gets called in the DTS thread and processes the de-registration 
           request received from the DTA.

 Input:    msg : De-Registration message received from the DTA

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  
\**************************************************************************/
uns32 dts_unregister_service(DTSV_MSG *msg)
{
	DTS_CB *inst = &dts_cb;
	SVC_KEY key, nt_key;
	DTS_SVC_REG_TBL *node, *svc;
	DTA_DEST_LIST *to_reg = NULL;
	MDS_DEST vkey = msg->dest_addr;
	OP_DEVICE *dev = NULL;

	key.ss_svc_id = msg->data.data.unreg.svc_id;
	key.node = msg->node;
	/*  Network order key added */
	nt_key.ss_svc_id = m_NCS_OS_HTONL(msg->data.data.unreg.svc_id);
	nt_key.node = m_NCS_OS_HTONL(msg->node);

	m_DTS_LK(&inst->lock);
	m_LOG_DTS_LOCK(DTS_LK_LOCKED, &inst->lock);

	m_LOG_DTS_EVT(DTS_EV_SVC_DE_REG_REQ_RCV, msg->data.data.reg.svc_id, msg->node, (uns32)msg->dest_addr);

	if (((node = (DTS_SVC_REG_TBL *)ncs_patricia_tree_get(&inst->svc_tbl, (const uns8 *)&nt_key)) == NULL)
	    || (node->dta_count == 0)) {
		m_LOG_DTS_HEADLINE(DTS_HDLN_NULL_INST);
		m_LOG_DTS_EVT(DTS_EV_SVC_DEREG_FAILED, key.ss_svc_id, key.node, (uns32)msg->dest_addr);
		m_DTS_UNLK(&inst->lock);
		m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
				      "dts_unregister_service: Service does not exist in the service registration table");
	}

	dev = &node->device;

	/* Smik - walk the v_cd_list and for the corresponding dta adest entry to be 
	 *        removed, remove the svc frm list of svcs in dta entry in patricia 
	 *        tree */
	if ((to_reg = (DTA_DEST_LIST *)dts_find_dta(node, &vkey)) == NULL) {
		m_LOG_DTS_HEADLINE(DTS_HDLN_NULL_INST);
		m_LOG_DTS_EVT(DTS_EV_SVC_DEREG_FAILED, key.ss_svc_id, key.node, (uns32)msg->dest_addr);
		m_DTS_UNLK(&inst->lock);
		m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
				      "dts_unregister_service: DTA adest entry doesn't exist in the svc->v_cd_list");
	}

	/* Remove the svc_reg entry from DTA's queue */
	if ((svc = (DTS_SVC_REG_TBL *)dts_del_svc_frm_dta(to_reg, node)) == NULL) {
		m_LOG_DTS_EVT(DTS_EV_DTA_SVC_RMV_FAIL, key.ss_svc_id, key.node, (uns32)to_reg->dta_addr);
		m_LOG_DTS_EVT(DTS_EV_SVC_DEREG_FAILED, key.ss_svc_id, key.node, (uns32)vkey);
		m_DTS_UNLK(&inst->lock);
		m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
				      "dts_unregister_service: Unable to remove svc reg entry from dta's list");
	}
	m_LOG_DTS_EVT(DTS_EV_DTA_SVC_RMV, key.ss_svc_id, key.node, (uns32)to_reg->dta_addr);

	/* Before deleting DTA from Service's v_cd_list update the DTS_CB's 
	 * svc_rmv_mds_dest for svc unregister's async update to stby
	 */
	inst->svc_rmv_mds_dest = to_reg->dta_addr;

	/* Now remove the DTA adest from the list */
	if ((to_reg = (DTA_DEST_LIST *)dts_dequeue_dta(node, to_reg)) == NULL) {
		m_LOG_DTS_EVT(DTS_EV_SVC_DTA_RMV_FAIL, key.ss_svc_id, key.node, (uns32)to_reg->dta_addr);
		m_LOG_DTS_EVT(DTS_EV_SVC_DEREG_FAILED, key.ss_svc_id, key.node, (uns32)vkey);
		memset(&inst->svc_rmv_mds_dest, '\0', sizeof(MDS_DEST));
		m_DTS_UNLK(&inst->lock);
		m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_unregister_service: Unable to remove adest entry");
	}
	m_LOG_DTS_EVT(DTS_EV_SVC_DTA_RMV, key.ss_svc_id, key.node, (uns32)to_reg->dta_addr);

	/* Remove spec entry corresponding to the DTA from svc's spec_list */
	if (dts_del_spec_frm_svc(node, vkey, NULL) != NCSCC_RC_SUCCESS) {
		m_LOG_DTS_EVT(DTS_EV_SVC_DEREG_FAILED, key.ss_svc_id, key.node, (uns32)vkey);
		memset(&inst->svc_rmv_mds_dest, '\0', sizeof(MDS_DEST));
		m_DTS_UNLK(&inst->lock);
		m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_unregister_service: Unable to remove spec entry");
	}

	/* Versioning support : call function to unload the ASCII_SPEC library if
	 * use_count of the library has become 0.
	 */
	dts_ascii_spec_load(msg->data.data.unreg.svc_name, msg->data.data.unreg.version, 0);
	/* Note that the entry from patricia tree indexed by spec svc_id will be 
	 * removed during the call to ascii_spec_unreg routine called from the user
	 * library.
	 */

	/* Smik - Send Async update (REMOVE) for DTS_SVC_REG_TBL */
	m_DTSV_SEND_CKPT_UPDT_ASYNC(inst, NCS_MBCSV_ACT_RMV, (MBCSV_REO_HDL)(long)node,
				    DTSV_CKPT_DTS_SVC_REG_TBL_CONFIG);

	/* After svc unregister's async updates memset DTS_CB's svc_rmv_mds_dest
	 * back to 0 for any further svc_reg remove async updates.
	 */
	memset(&inst->svc_rmv_mds_dest, '\0', sizeof(MDS_DEST));

	/*Smik-Now check if the num of svs in DTA list is 0, if yes then delete the 
	 *     DTA entry from the patricia tree */
	if (to_reg->svc_list == NULL) {
		if (ncs_patricia_tree_del(&inst->dta_list, (NCS_PATRICIA_NODE *)&to_reg->node) != NCSCC_RC_SUCCESS) {
			m_LOG_DTS_EVT(DTS_EV_DTA_DEST_RMV_FAIL, 0, key.node, (uns32)to_reg->dta_addr);
			m_LOG_DTS_EVT(DTS_EV_SVC_DEREG_FAILED, key.ss_svc_id, key.node, (uns32)vkey);
			m_DTS_UNLK(&inst->lock);
			m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
					      "dts_unregister_service: Unable to remove DTA entry frm patricia tree");
		}
		m_LOG_DTS_EVT(DTS_EV_DTA_DEST_RMV_SUCC, 0, key.node, (uns32)to_reg->dta_addr);

		if (NULL != to_reg)
			m_MMGR_FREE_VCARD_TBL(to_reg);
	}

	node->num_svcs--;

	/* Close the open file and let the service log to a new file */
	node->device.new_file = TRUE;
	node->device.cur_file_size = 0;

	/* Check whether we still need this patricia entry. 
	 * If no then delete it and free mem */
	if (node->dta_count == 0) {
		node->v_cd_list = NULL;
		dts_circular_buffer_free(&dev->cir_buffer);
		/* Cleanup the DTS_FILE_LIST datastructure for svc */
		m_DTS_FREE_FILE_LIST(dev);
		/* Cleanup the console devices associated with the node */
		m_DTS_RMV_ALL_CONS(dev);
		if ((TRUE == node->device.file_open) && (node->device.svc_fh != NULL))
			fclose(node->device.svc_fh);
		ncs_patricia_tree_del(&inst->svc_tbl, (NCS_PATRICIA_NODE *)node);
		m_LOG_DTS_EVT(DTS_EV_SVC_REG_ENT_RMVD, node->my_key.ss_svc_id, node->my_key.node, (uns32)vkey);
		if (NULL != node)
			m_MMGR_FREE_SVC_REG_TBL(node);
	}

	m_DTS_UNLK(&inst->lock);
	m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);

	m_LOG_DTS_EVT(DTS_EV_SVC_DEREG_SUCCESSFUL, key.ss_svc_id, key.node, (uns32)vkey);

	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
 Function: dts_handle_fail_over

 Purpose:  Function which handles DTS fail-over and initiates sending new 
           service handle and filtering policies to DTAs.

 Input:    None 

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  
\**************************************************************************/
uns32 dts_handle_fail_over(void)
{
	DTS_CB *inst = &dts_cb;
	uns32 status = NCSCC_RC_SUCCESS;

	m_DTS_LK(&inst->lock);
	m_LOG_DTS_LOCK(DTS_LK_LOCKED, &inst->lock);

	/* Smik - need to initialize stby(now active) DTS with proper policies */
	status = dts_stby_initialize(inst);
	m_DTS_UNLK(&inst->lock);
	m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);

	return status;
}

/**************************************************************************\
 Function: dts_fail_over_enc_msg

 Purpose:  Function used for encoding the DTS_FAIL_OVER message.

 Input:    msg : Log message to be filled with encoded data.

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  
\**************************************************************************/
uns32 dts_fail_over_enc_msg(DTSV_MSG *mm)
{
	DTA_DEST_LIST *dta_ptr;
	NCS_UBAID *uba;
	uns8 *data;
	uns32 count;
	SVC_ENTRY *svc_entry;

	uba = &mm->data.data.msg.log_msg.uba;

	memset(uba, '\0', sizeof(NCS_UBAID));
	if (ncs_enc_init_space(uba) != NCSCC_RC_SUCCESS)
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_fail_over_enc_msg: userbuf init failed");

	dta_ptr = (DTA_DEST_LIST *)(long)mm->data.data.dta_ptr;
	count = dta_ptr->dta_num_svcs;

	/* Smik - Allocate space for all encoding all services corr to DTA */
	data = ncs_enc_reserve_space(uba, (count * DTSV_REG_CONF_MSG_SIZE) + sizeof(uns32));

	if (data == NULL)
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_fail_over_enc_msg: ncs_enc_reserve_space returns NULL");
	/* Smik - First encode the count of services */
	ncs_encode_32bit(&data, count);
	/* Smik - traverse the DTA svc_list queue and encode the data */

	svc_entry = dta_ptr->svc_list;
	while (svc_entry != NULL) {
		ncs_encode_32bit(&data, svc_entry->svc->my_key.ss_svc_id);
		ncs_encode_32bit(&data, svc_entry->svc->svc_policy.enable);
		ncs_encode_32bit(&data, svc_entry->svc->svc_policy.category_bit_map);
		ncs_encode_8bit(&data, svc_entry->svc->svc_policy.severity_bit_map);
		/* No need of policy handles */
		/*ncs_encode_32bit(&data, svc_entry->svc->svc_hdl); */

		svc_entry = svc_entry->next_in_dta_entry;
	}

	ncs_enc_claim_space(uba, (count * DTSV_REG_CONF_MSG_SIZE) + sizeof(uns32));

	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
 Function: dts_log_my_data

 Purpose:  Function used for logging the messages received from the DTA's.

 Input:    msg : Log message received from the DTA

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  
\**************************************************************************/
uns32 dts_log_my_data(DTSV_MSG *msg)
{
	DTS_CB *inst = &dts_cb;
	uns32 rc = NCSCC_RC_SUCCESS;

	m_DTS_LK(&inst->lock);

	rc = dts_log_data(msg);

	m_DTS_UNLK(&inst->lock);

	return rc;
}

/**************************************************************************\
 Function: dts_log_data

 Purpose:  Function used for logging the messages received from the DTA's and
           even messages which are sequenced and required to be dumped. 
           This fuction checks the logging level to be used for logging 
           the message received from DTA and calls function dtsv_log_msg
           with apropriate logging parameters.
           Versioning support changes: SPEC table pointer is retrieved from the
           service's spec_list indexed by MDS_DEST.

 Input:    msg : Log message received from the DTA

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  
\**************************************************************************/
uns32 dts_log_data(DTSV_MSG *msg)
{
	DTS_CB *inst = &dts_cb;
	DTS_SVC_REG_TBL *node = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;
	SVC_KEY nt_key;
	MDS_DEST dta_key = msg->dest_addr;
	NCSFL_ASCII_SPEC *spec = NULL;

	memset(&nt_key, '\0', sizeof(SVC_KEY));
	/* 
	 * If sequencing is enabled then queue the messgae into the sequencing 
	 * buffer otherwise do normal logging.
	 */
	if ((inst->g_policy.g_enable_seq == TRUE) && (msg->seq_msg == FALSE)) {
		if (NCSCC_RC_SUCCESS != dts_queue_seq_msg(inst, msg)) {
			dts_free_msg_content(&msg->data.data.msg.log_msg);
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
					      "dts_log_data: Unable to Queue message into sequencing buffer");
		}

		return NCSCC_RC_CONTINUE;
	}

	/* No need of policy handles */
	nt_key.node = m_NCS_OS_HTONL(msg->node);
	nt_key.ss_svc_id = m_NCS_OS_HTONL(msg->data.data.msg.log_msg.hdr.ss_id);

	if ((node = (DTS_SVC_REG_TBL *)ncs_patricia_tree_get(&inst->svc_tbl, (const uns8 *)&nt_key)) == NULL) {
		/* Print an error message */
		ncs_logmsg(NCS_SERVICE_ID_DTSV, DTS_LID_EVT, DTS_FC_EVT,
			   NCSFL_LC_EVENT, NCSFL_SEV_NOTICE, "TILLL", DTS_EV_LOG_SVC_KEY_WRONG,
			   msg->data.data.msg.log_msg.hdr.ss_id, msg->node, (uns32)msg->dest_addr);
		m_MMGR_FREE_OCT(msg->data.data.msg.log_msg.hdr.fmat_type);
		return NCSCC_RC_FAILURE;
	}

	/* Versioning support: Get the appropriate spec from the node's spec list */
	spec = (NCSFL_ASCII_SPEC *)dts_find_spec(node, &dta_key);
	if (spec == NULL) {
		ncs_logmsg(NCS_SERVICE_ID_DTSV, DTS_LID_LOG_ERR2, DTS_FC_EVT, NCSFL_LC_EVENT, NCSFL_SEV_NOTICE, "TILL",
			   DTS_SPEC_NOT_REG, 0, msg->data.data.msg.log_msg.hdr.ss_id);
		m_MMGR_FREE_OCT(msg->data.data.msg.log_msg.hdr.fmat_type);
		return NCSCC_RC_FAILURE;
	}

	/* 
	 * Check whether global logging is set. If global logging is set
	 * then log using global logging policies.
	 */
	if (inst->g_policy.global_logging == TRUE) {
		rc = dtsv_log_msg(msg, &inst->g_policy.g_policy, &inst->g_policy.device, GLOBAL_FILE, spec);
	} else {
		/* Check whether the service is registered properly, its node entry 
		 * is present 
		 */
		if (node->my_node == NULL) {
			m_MMGR_FREE_OCT(msg->data.data.msg.log_msg.hdr.fmat_type);
			return NCSCC_RC_FAILURE;
		} else if (node->my_node->per_node_logging == TRUE) {
			/* 
			 * If per node logging is set then log the per node log file, 
			 * else use per service log file for logging.
			 */
			rc = dtsv_log_msg(msg, &node->my_node->svc_policy, &node->my_node->device, PER_NODE_FILE, spec);

		} else {
			rc = dtsv_log_msg(msg, &node->svc_policy, &node->device, PER_SVC_FILE, spec);
		}
	}			/*end of else */
	return rc;
}

/**************************************************************************\
 Function: dtsv_log_msg

 Purpose:  Function used for logging the data into the I/O device as 
           per the logging policy provided and the output device.

 Input:    msg      : Log message received from the DTA
           policy   : Logging policy to be used for logging received message.
           device   : Log device to be used for logging received message.
           file_type: Type of the file to be used for message logging.
           spec     : ASCII_SPEC table of the service which logs.

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  
\**************************************************************************/
uns32 dtsv_log_msg(DTSV_MSG *msg, POLICY *policy, OP_DEVICE *device, uns8 file_type, NCSFL_ASCII_SPEC *spec)
{
	char *str = dts_cb.log_str;
	SVC_KEY key;
	uns32 len = 0;
	DTS_LOG_CKPT_DATA data;
	DTS_CONS_LIST *cons_dev;

	cons_dev = device->cons_list_ptr;

	if (dts_log_msg_to_str(&msg->data.data.msg, str,
			       (uns32)msg->node, (uns32)msg->dest_addr, &len, spec) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

	if ((policy->log_dev & LOG_FILE) == LOG_FILE) {
		key.node = msg->node;
		key.ss_svc_id = msg->data.data.msg.log_msg.hdr.ss_id;

		/*
		 * Create new log file if current file size == 0 OR
		 * new log file is set to TRUE
		 * or current file size exceeds the configured value.
		 */
		if ((device->cur_file_size == 0) ||
		    (device->new_file == TRUE) || (device->cur_file_size > (policy->log_file_size * 1024))) {
			m_DTS_ADD_NEW_FILE(device);
			if (0 ==
			    (device->cur_file_size =
			     dts_new_log_file_create(m_DTS_LOG_FILE_NAME(device), &key, file_type)))
				return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_log_msg: Failed to create new log file.");
			device->new_file = FALSE;

			memset(&data, '\0', sizeof(DTS_LOG_CKPT_DATA));
			/* Fill the DTS_LOG_CKPT_DATA for async update */
			strcpy(data.file_name, m_DTS_LOG_FILE_NAME(device));
			/* Fill data key with the key corresponding to node/svc whose policy
			   is being used */
			/* Fill key according to the logging levels */
			if (file_type == GLOBAL_FILE) {
				data.key.node = 0;
				data.key.ss_svc_id = 0;
			} else if (file_type == PER_NODE_FILE) {
				data.key.node = key.node;
				data.key.ss_svc_id = 0;
			} else {
				data.key = key;
			}
			data.new_file = device->new_file;

			/* Send async update to stby */
			m_DTSV_SEND_CKPT_UPDT_ASYNC(&dts_cb, NCS_MBCSV_ACT_ADD, (MBCSV_REO_HDL)(long)&data,
						    DTSV_CKPT_DTS_LOG_FILE_CONFIG);

			if (m_DTS_NUM_LOG_FILES(device) > dts_cb.g_policy.g_num_log_files) {
				/*delete oldest file */
				m_DTS_RMV_FILE(device);
				m_DTSV_SEND_CKPT_UPDT_ASYNC(&dts_cb, NCS_MBCSV_ACT_RMV, (MBCSV_REO_HDL)(long)&data,
							    DTSV_CKPT_DTS_LOG_FILE_CONFIG);
			}
			/* 
			 * Just a precautionary close here which will ensure that the 
			 * old file is getting closed when we are creating a new file.
			 */
			if ((TRUE == device->file_open) && (device->svc_fh != NULL))
				fclose(device->svc_fh);

			if (NULL == (device->svc_fh = sysf_fopen(m_DTS_LOG_FILE_NAME(device), "a+")))
				return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dtsv_log_msg: Failed to open log file");

			device->file_open = TRUE;

		}

		/* Check whether we can dump data into this file...else there is some problem. */
		if ((NULL == device->svc_fh) || (FALSE == device->file_open)) {
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
					      "dtsv_log_msg: Unbale to dump message. Either file handle is NULL or file is not opened properly");
		}
		device->cur_file_size += (CARRIAGE_RETURN + fwrite(str, 1, len, device->svc_fh));

		fflush(device->svc_fh);
	}

	if ((policy->log_dev & CIRCULAR_BUFFER) == CIRCULAR_BUFFER) {
		/* Check whether buffer has been allocated,
		 * If not, allocate according to policy */
		if (device->cir_buffer.buff_allocated == FALSE) {
			if (dts_circular_buffer_alloc(&device->cir_buffer, policy->cir_buff_size) == NCSCC_RC_SUCCESS)
				m_LOG_DTS_CBOP(DTS_CBOP_ALLOCATED, 0, 0);
			else
				return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
						      "dtsv_log_msg: Circular buffer unallocated. Unable to allocate buffer according to policy settings");
		}
		/* Now dump logs to buffer */
		if (dts_dump_to_cir_buffer(&device->cir_buffer, str) != NCSCC_RC_SUCCESS)
			return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
					      "dtsv_log_msg: Failed to dump log into circular buffer.");
	}

	/*
	 * If debug flag is set then log all the logs of severity other than 
	 * debug and Info to console.
	 */
#if (DTS_DEBUG == 1)
	if (((policy->log_dev & OUTPUT_CONSOLE) == OUTPUT_CONSOLE) ||
	    (msg->data.data.msg.log_msg.hdr.severity == NCSFL_SEV_EMERGENCY) ||
	    (msg->data.data.msg.log_msg.hdr.severity == NCSFL_SEV_ALERT) ||
	    (msg->data.data.msg.log_msg.hdr.severity == NCSFL_SEV_CRITICAL)) {
		TRACE("%s", str);
		m_DTS_CONS_PRINT(cons_dev, msg, str, len);
	}
#else
	if ((policy->log_dev & OUTPUT_CONSOLE) == OUTPUT_CONSOLE) {
		m_DTS_CONS_PRINT(cons_dev, msg, str, len);
	}
#endif

	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
 Function: dts_find_reg

 Purpose:  This function searches the queue using the key passedand returns
           true or flase depending on whether it finds the vcard registration
           in the queue.

 Input:    key     : Key to be used for searching the v-card registration.
           qelem   : V-card registration queue.

 Returns:  TRUE/FALSE

 Notes:  
\**************************************************************************/
NCS_BOOL dts_find_reg(void *key, void *qelem)
{
	MDS_DEST *vkey = (MDS_DEST *)key;
	DTA_DEST_LIST *svc = (DTA_DEST_LIST *)qelem;

	if (memcmp(vkey, &svc->dta_addr, sizeof(MDS_DEST)) == 0)
		return TRUE;

	return FALSE;
}

/**************************************************************************\
 Function: dts_default_policy_set

 Purpose:  Function used for setting the policy to default.

 Input:    dflt_plcy  : Default policy table.

 Returns:  None.

 Notes:  
\**************************************************************************/
uns32 gl_severity_filter = GLOBAL_SEVERITY_FILTER;
void dts_default_policy_set(DEFAULT_POLICY *dflt_plcy)
{
	/* Set Default Node Policies */

	dflt_plcy->node_dflt.per_node_logging = NODE_LOGGING;
	dflt_plcy->node_dflt.policy.enable = NODE_ENABLE;
	dflt_plcy->node_dflt.policy.category_bit_map = GLOBAL_CATEGORY_FILTER;
	dflt_plcy->node_dflt.policy.severity_bit_map = gl_severity_filter;

	dflt_plcy->node_dflt.policy.log_dev = NODE_LOG_DEV;
	dflt_plcy->node_dflt.policy.log_file_size = NODE_LOGFILE_SIZE;
	dflt_plcy->node_dflt.policy.file_log_fmt = NODE_FILE_LOG_FMT;
	dflt_plcy->node_dflt.policy.cir_buff_size = NODE_CIR_BUFF_SIZE;
	dflt_plcy->node_dflt.policy.buff_log_fmt = NODE_BUFF_LOG_FMT;

	/* Set Default Svc Policies */

	dflt_plcy->svc_dflt.per_node_logging = NODE_LOGGING;
	dflt_plcy->svc_dflt.policy.enable = SVC_ENABLE;
	dflt_plcy->svc_dflt.policy.category_bit_map = GLOBAL_CATEGORY_FILTER;
	dflt_plcy->svc_dflt.policy.severity_bit_map = gl_severity_filter;

	dflt_plcy->svc_dflt.policy.log_dev = SVC_LOG_DEV;
	dflt_plcy->svc_dflt.policy.log_file_size = SVC_LOGFILE_SIZE;
	dflt_plcy->svc_dflt.policy.file_log_fmt = SVC_FILE_LOG_FMT;
	dflt_plcy->svc_dflt.policy.cir_buff_size = SVC_CIR_BUFF_SIZE;
	dflt_plcy->svc_dflt.policy.buff_log_fmt = SVC_BUFF_LOG_FMT;

	return;
}

/**************************************************************************\
 Function: dts_global_policy_set

 Purpose:  Function used for setting the global policy to default values.

 Input:    gpolicy  : Global policy table to be set to default value.

 Returns:  None.

 Notes:  
\**************************************************************************/
void dts_global_policy_set(GLOBAL_POLICY *gpolicy)
{
	gpolicy->global_logging = GLOBAL_LOGGING;

	gpolicy->g_policy.enable = GLOBAL_ENABLE;
	gpolicy->g_policy.category_bit_map = GLOBAL_CATEGORY_FILTER;
	gpolicy->g_policy.severity_bit_map = gl_severity_filter;

	gpolicy->g_policy.log_dev = GLOBAL_LOG_DEV;
	gpolicy->g_policy.log_file_size = GLOBAL_LOGFILE_SIZE;
	gpolicy->device.new_file = TRUE;
	gpolicy->g_policy.file_log_fmt = GLOBAL_FILE_LOG_FMT;
	gpolicy->g_policy.cir_buff_size = GLOBAL_CIR_BUFF_SIZE;
	gpolicy->g_policy.buff_log_fmt = GLOBAL_BUFF_LOG_FMT;

	gpolicy->g_num_log_files = GLOBAL_NUM_LOG_FILES;
	gpolicy->g_enable_seq = GLOBAL_ENABLE_SEQ;
	gpolicy->g_close_files = FALSE;

	gpolicy->device.cur_file_size = 0;
	gpolicy->device.svc_fh = 0;
	gpolicy->device.file_open = FALSE;
	gpolicy->device.last_rec_id = 0;
	gpolicy->device.cons_list_ptr = NULL;
	gpolicy->device.num_of_cons_conf = 0;

	if (gpolicy->g_policy.log_dev == CIRCULAR_BUFFER) {
		if (dts_circular_buffer_alloc(&gpolicy->device.cir_buffer, GLOBAL_CIR_BUFF_SIZE) == NCSCC_RC_SUCCESS)
			m_LOG_DTS_CBOP(DTS_CBOP_ALLOCATED, 0, 0);
	} else
		dts_cir_buff_set_default(&gpolicy->device.cir_buffer);

	return;
}

/**************************************************************************\
 Function: dts_default_node_policy_set

 Purpose:  This Function is used for setting the default policies of the 
           node which are just registered with the DTS and also if the 
           user creates the new imm object.

 Input:    npolicy  : Node policy table.
           device   : Node logging device.
           node_id  : Node id of the node.

 Returns:  None.

 Notes:  
\**************************************************************************/
void dts_default_node_policy_set(POLICY *npolicy, OP_DEVICE *device, uns32 node_id)
{
	uns8 global_sev_filter = dts_cb.g_policy.g_policy.severity_bit_map;
	uns32 global_cat_filter = dts_cb.g_policy.g_policy.category_bit_map;

	/* Enable flag should inherit from global policy */
	npolicy->enable = dts_cb.g_policy.g_policy.enable;
	/* Node filter policy would always take frm the global filter */
	npolicy->category_bit_map = global_cat_filter;
	npolicy->severity_bit_map = global_sev_filter;

	npolicy->log_dev = NODE_LOG_DEV;
	npolicy->log_file_size = NODE_LOGFILE_SIZE;
	device->new_file = TRUE;
	npolicy->file_log_fmt = NODE_FILE_LOG_FMT;
	npolicy->cir_buff_size = NODE_CIR_BUFF_SIZE;
	npolicy->buff_log_fmt = NODE_BUFF_LOG_FMT;

	device->cur_file_size = 0;
	device->svc_fh = 0;
	device->file_open = FALSE;
	device->last_rec_id = 0;
	memset(&device->log_file_list, '\0', sizeof(DTS_FILE_LIST));
	device->cons_list_ptr = NULL;
	device->num_of_cons_conf = 0;

	if (npolicy->log_dev == CIRCULAR_BUFFER) {
		if (dts_circular_buffer_alloc(&device->cir_buffer, NODE_CIR_BUFF_SIZE) == NCSCC_RC_SUCCESS)
			m_LOG_DTS_CBOP(DTS_CBOP_ALLOCATED, 0, node_id);
	} else
		dts_cir_buff_set_default(&device->cir_buffer);

	return;
}

/**************************************************************************\
 Function: dts_default_svc_policy_set

 Purpose:  This Function is used for setting the default policies of the 
           services which are just registered with the DTS and also if the 
           user creates the new imm object.

 Input:    spolicy  : Service policy table.
           device   : Service logging device.
           node_id  : Node id of the node.
           svc_id   : Service id of the service.

 Returns:  None.

 Notes:  
\**************************************************************************/
void dts_default_svc_policy_set(DTS_SVC_REG_TBL *service)
{
	POLICY *spolicy = &service->svc_policy;
	OP_DEVICE *device = &service->device;
	uns8 global_sev_filter = dts_cb.g_policy.g_policy.severity_bit_map;
	uns32 global_cat_filter = dts_cb.g_policy.g_policy.category_bit_map;

	/* Service filter policy would take from node filter if node */
	if (service->my_node != NULL) {
		spolicy->enable = service->my_node->svc_policy.enable;
		spolicy->category_bit_map = service->my_node->svc_policy.category_bit_map;
		spolicy->severity_bit_map = service->my_node->svc_policy.severity_bit_map;

	} else {
		spolicy->enable = dts_cb.g_policy.g_policy.enable;
		spolicy->category_bit_map = global_cat_filter;
		spolicy->severity_bit_map = global_sev_filter;
	}
	spolicy->log_dev = SVC_LOG_DEV;
	spolicy->log_file_size = SVC_LOGFILE_SIZE;
	device->new_file = TRUE;
	spolicy->file_log_fmt = SVC_FILE_LOG_FMT;
	spolicy->cir_buff_size = SVC_CIR_BUFF_SIZE;
	spolicy->buff_log_fmt = SVC_BUFF_LOG_FMT;

	device->cur_file_size = 0;
	device->svc_fh = 0;
	device->file_open = 0;
	device->last_rec_id = 0;
	memset(&device->log_file_list, '\0', sizeof(DTS_FILE_LIST));
	device->cons_list_ptr = NULL;
	device->num_of_cons_conf = 0;

	if (spolicy->log_dev == CIRCULAR_BUFFER) {
		if (dts_circular_buffer_alloc(&device->cir_buffer, SVC_CIR_BUFF_SIZE) == NCSCC_RC_SUCCESS)
			m_LOG_DTS_CBOP(DTS_CBOP_ALLOCATED, service->my_key.ss_svc_id, service->my_key.node);
	} else
		dts_cir_buff_set_default(&device->cir_buffer);

	return;
}

/**************************************************************************\
 Function: dts_new_log_file_create

 Purpose:  This Function is used for creating the new logging file. It also 
           creates the file header and put in the file.

 Input:    file     : File name string.
           svc      : Service key.
           file_type:  Type of the file to be created.

 Returns:   NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  
\**************************************************************************/
uns32 dts_new_log_file_create(char *file, SVC_KEY *svc, uns8 file_type)
{
	time_t tod;
	char asc_dtime[70];
	char name[20] = "", tfile[100];
	FILE *fh;
	uns32 count = 0;

	char *logging_type[] = { "Global File", "Node File", "Service per Node File" };

	m_GET_ASCII_DATE_TIME_STAMP(tod, asc_dtime);

	strcpy(file, dts_cb.log_path);

	if (file_type == PER_SVC_FILE) {
		if ((m_DTS_GET_SVC_NAME(svc, name)) == NCSCC_RC_FAILURE)
			dts_print_current_config(&dts_cb);
		/* Node-id display change */
		sprintf(tfile, "%s_0x%08x_%s%s", name, svc->node, asc_dtime, ".log");
	} else if (file_type == GLOBAL_FILE) {
		sprintf(tfile, "%s_%s%s", "OPENSAF", asc_dtime, ".log");
	} else {
		/* Node-id display change */
		sprintf(tfile, "%s_0x%08x_%s%s", "NODE", svc->node, asc_dtime, ".log");
	}

	strcat(file, tfile);

	m_GET_ASCII_HDR_DATE_TIME_STAMP(tod, asc_dtime);

	if ((fh = sysf_fopen(file, "a+")) != NULL) {
		count += (CARRIAGE_RETURN +
			  fprintf(fh, "*********************** F I L E   H E A D E R **************************\n"));

		count += (CARRIAGE_RETURN + fprintf(fh, "**   Logging Type  = %s\n", logging_type[file_type]));

		count += (CARRIAGE_RETURN + fprintf(fh, "**   Creation Time = %s\n", asc_dtime));

		if (file_type == GLOBAL_FILE) {
			count += (CARRIAGE_RETURN + fprintf(fh, "**   Node ID       = %s\n", "ALL"));

			count += (CARRIAGE_RETURN + fprintf(fh, "**   Service Name  = %s\n", "ALL"));
		} else if (file_type == PER_SVC_FILE) {
			count += (CARRIAGE_RETURN +
				  /* Node-id display change */
				  fprintf(fh, "**   Node ID       = 0x%08x\n", svc->node));

			count += (CARRIAGE_RETURN + fprintf(fh, "**   Service Name  = %s\n", name));
		} else {
			count += (CARRIAGE_RETURN +
				  /* Node-id display change */
				  fprintf(fh, "**   Node ID       = 0x%08x\n", svc->node));
			count += (CARRIAGE_RETURN + fprintf(fh, "**   Service Name  = %s\n", "ALL"));
		}

		count += (CARRIAGE_RETURN +
			  fprintf(fh, "************************************************************************\n"));
		fclose(fh);
	} else {
		syslog(LOG_ERR, "dts_new_log_file_create: Failed to open log file");
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_new_log_file_create: Failed to open log file");
	}

	if (file_type == GLOBAL_FILE) {
		m_LOG_DTS_LFILE(file, 0, 0);
	} else {
		m_LOG_DTS_LFILE(file, svc->node, svc->ss_svc_id);
	}
	return count;
}

/**************************************************************************\
 Function: dts_send_filter_config_msg

 Purpose:  Change in the Global Filtering policy will affect the entire system
           This function will set the filtering policies of all the nodes and 
           all the services which are currently present in the node and the 
           service registration table respectively. Also, policy change will
           be sent to all the services.

 Input:    cb       : DTS_CB pointer
           svc      : Service to be configured
           dta      : DTA on which this service resides.

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  
\**************************************************************************/
uns32 dts_send_filter_config_msg(DTS_CB *inst, DTS_SVC_REG_TBL *svc, DTA_DEST_LIST *dta)
{
	DTSV_MSG msg;
	if (dta == NULL)
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_send_filter_config_msg: NULL pointer passed");

	if (FALSE == dta->dta_up) {
		dta->updt_req = TRUE;
		return NCSCC_RC_SUCCESS;
	}

	memset(&msg, 0, sizeof(DTSV_MSG));
	msg.vrid = inst->vrid;
	msg.node = svc->my_key.node;
	msg.msg_type = DTS_SVC_MSG_FLTR;

	msg.data.data.msg_fltr.svc_id = svc->my_key.ss_svc_id;
	msg.data.data.msg_fltr.enable_log = svc->svc_policy.enable;
	msg.data.data.msg_fltr.category_bit_map = svc->svc_policy.category_bit_map;
	msg.data.data.msg_fltr.severity_bit_map = svc->svc_policy.severity_bit_map;
	/* No need of policy handles */
	/*msg.data.data.msg_fltr.policy_hdl = svc->svc_hdl; */

	if (dts_mds_send_msg(&msg, dta->dta_addr, inst->mds_hdl) != NCSCC_RC_SUCCESS)
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_send_filter_config_msg: MDS send message failed.");

	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
 Function: dts_open_conf_cons

 Purpose:  Function used for opening console devices consfigured through CLI
           for Global/Node/Service devices.

 Input:    None.

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes: Changes for enabling pty logging.  
\**************************************************************************/
int32 dts_open_conf_cons(DTS_CB *cb, uns32 mode, char *str)
{
	int32 f, fd = -1, fd_init;
	uns32 m;

	/*Init console device */
	if ((fd_init = open(str, O_RDONLY | O_NONBLOCK)) < 0)
		return fd_init;
	else
		close(fd_init);

	/*Open device in non blocking mode */
	m = mode | O_NONBLOCK;

	for (f = 0; f < 5; f++)
		if ((fd = open(str, m)) >= 0)
			break;

	if (fd < 0)
		return fd;

	if (m != mode)
		fcntl(fd, F_SETFL, mode);

	return fd;
}

/**************************************************************************\
 Function: dts_ascii_spec_reload

 Purpose:  Function used for reloading ASCII_SPEC table specified in   
           PKGSYSCONFDIR/dts_ascii_spec_config.

 Input:    None.

 Returns:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

 Notes:   Changes for enabling dynamic reloading of ASCII_SPEC tables. 
          spec_entry->svc_id is already in network order.
          
          With versioning support, reloading of ASCII_SPECs will be 
          discontinued. So this function would not be called.  
\**************************************************************************/
/**************************************************************************\
 Function: dts_close_opened_files

 Purpose:  Function used to close all opened files. Called from CLI. 

 Input:    None.

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  
\**************************************************************************/
uns32 dts_close_opened_files(void)
{
	SVC_KEY nt_key;
	DTS_CB *inst = &dts_cb;
	DTS_SVC_REG_TBL *service;

	m_LOG_DTS_DBGSTR(DTS_SERVICE, "Closing all the open files", 0, 0);

	/* First close files in global policy */
	if ((inst->g_policy.device.file_open == TRUE) && (inst->g_policy.device.svc_fh != NULL)) {
		fclose(inst->g_policy.device.svc_fh);
		inst->g_policy.device.file_open = FALSE;
		inst->g_policy.device.new_file = TRUE;
		inst->g_policy.device.svc_fh = NULL;
		inst->g_policy.device.cur_file_size = 0;
	}

	/* Send the update for the device changes in global policy to STBY */
	m_DTSV_SEND_CKPT_UPDT_ASYNC(inst, NCS_MBCSV_ACT_UPDATE, (MBCSV_REO_HDL)(long)&inst->g_policy,
				    DTSV_CKPT_GLOBAL_POLICY_CONFIG);

	/* Search through registration table, Set all the policies,
	 * configure all the DTA's using this policy. 
	 */
	service = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(&inst->svc_tbl, NULL);
	while (service != NULL) {
		/* Setup key for new search */
		nt_key = service->ntwk_key;
		if ((service->device.file_open == TRUE) && (service->device.svc_fh != NULL)) {
			fclose(service->device.svc_fh);
			service->device.file_open = FALSE;
			service->device.new_file = TRUE;
			service->device.svc_fh = NULL;
			service->device.cur_file_size = 0;
		}
		/* Send update to STBY for the service device variable changes */
		m_DTSV_SEND_CKPT_UPDT_ASYNC(inst, NCS_MBCSV_ACT_UPDATE, (MBCSV_REO_HDL)(long)service,
					    DTSV_CKPT_DTS_SVC_REG_TBL_CONFIG);

		service = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(&inst->svc_tbl, (const uns8 *)&nt_key);
	}

	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
 Function: dts_close_files_quiesced

 Purpose:  Function used to close all opened files while going to quiesced.

 Input:    None.

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:
\**************************************************************************/
uns32 dts_close_files_quiesced(void)
{
	SVC_KEY nt_key;
	DTS_CB *inst = &dts_cb;
	DTS_SVC_REG_TBL *service;

	m_LOG_DTS_DBGSTR(DTS_SERVICE, "Closing all the open files: Going to QUIESCED STATE", 0, 0);

	/* First close files in global policy */
	if ((inst->g_policy.device.file_open == TRUE) && (inst->g_policy.device.svc_fh != NULL)) {
		fclose(inst->g_policy.device.svc_fh);
		inst->g_policy.device.svc_fh = NULL;
		inst->g_policy.device.file_open = FALSE;
		inst->g_policy.device.new_file = TRUE;
		inst->g_policy.device.cur_file_size = 0;
	}

	service = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(&inst->svc_tbl, NULL);
	while (service != NULL) {
		/* Setup key for new search */
		nt_key = service->ntwk_key;
		if ((service->device.file_open == TRUE) && (service->device.svc_fh != NULL)) {
			fclose(service->device.svc_fh);
			service->device.svc_fh = NULL;
			service->device.file_open = FALSE;
			service->device.new_file = TRUE;
			service->device.cur_file_size = 0;
		}
		service = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(&inst->svc_tbl, (const uns8 *)&nt_key);
	}

	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
 Function: dts_create_new_pat_entry

 Purpose:  This function is used creating the new service or node entry in
           the patritia tree.

 Input:    inst     : DTS CB.
           node_id  : Node id.
           svc_id   : Service ID to be added in tree.
           log_level: Set logging level.

 Returns:  Pointer to the next Node entry in pat tree

 Notes:  
\**************************************************************************/
uns32 dts_create_new_pat_entry(DTS_CB *inst, DTS_SVC_REG_TBL **node, uns32 node_id, SS_SVC_ID svc_id, uns8 log_level)
{
	DTS_SVC_REG_TBL *parent_node;
	SVC_KEY nt_key;
	/* 
	 * Alloc memory and initialize for new patritia tree entry.
	 */
	*node = m_MMGR_ALLOC_SVC_REG_TBL;
	if (*node == NULL)
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_create_new_pat_entry: Failed to allocate memory");

	memset(*node, '\0', sizeof(DTS_SVC_REG_TBL));
	(*node)->my_key.node = node_id;
	(*node)->my_key.ss_svc_id = svc_id;
	/* Network order key added */
	(*node)->ntwk_key.node = m_NCS_OS_HTONL(node_id);
	(*node)->ntwk_key.ss_svc_id = m_NCS_OS_HTONL(svc_id);
	(*node)->node.key_info = (uns8 *)&(*node)->ntwk_key;

	(*node)->per_node_logging = log_level;
	/* 
	 * If service id is 0 it meand it is a node entry.
	 */
	if (0 == svc_id) {
		(*node)->my_node = *node;
		dts_default_node_policy_set(&(*node)->svc_policy, &(*node)->device, node_id);
	} else {
		/*  Network order key added */
		nt_key.node = (*node)->ntwk_key.node;
		nt_key.ss_svc_id = 0;
		if ((parent_node =
		     (DTS_SVC_REG_TBL *)ncs_patricia_tree_get(&inst->svc_tbl, (const uns8 *)&nt_key)) == NULL) {
			m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
				       "dts_create_new_pat_entry: No node entry exists for service entry");
			(*node)->my_node = NULL;
		} else
			(*node)->my_node = parent_node;

		/* For service entry create a vcard queue. */
		/*ncs_create_queue(&(*node)->v_cd_list); */
		(*node)->v_cd_list = NULL;
		dts_default_svc_policy_set((*node));
	}

	if (ncs_patricia_tree_add(&inst->svc_tbl, (NCS_PATRICIA_NODE *)(*node)) != NCSCC_RC_SUCCESS) {
		if (NULL != (*node))
			m_MMGR_FREE_SVC_REG_TBL((*node));

		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
				      "dts_create_new_pat_entry: Failed to add entry into patricia tree");
	}

	/*  No need of policy handles */
	/*(*node)->svc_hdl = ncshm_create_hdl(inst->hmpool_id, NCS_SERVICE_ID_DTSV,
	   (NCSCONTEXT)(*node)); */

	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
 Function: dts_stby_initialize

 Purpose:  This function is used to initialize previously stby(now active) 
           DTS with the proper logging/filtering policies in place. 

 Input:    CB : DTS_CB instance

 Returns:  NCSCC_RC_SUCCESS if initialization is successful, otherwise
           NCSCC_RC_FAILURE 

 Notes:  
\**************************************************************************/
static uns32 dts_stby_initialize(DTS_CB *cb)
{
	/* Check for message sequencing enabled or not */
	if (cb->g_policy.g_enable_seq == TRUE) {
		if (dts_enable_sequencing(cb) != NCSCC_RC_SUCCESS)
			m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
				       "dts_stby_initialize: Failed to enable sequencing of messages");
	}

	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
 Function: dts_stby_update_dta_config

 Purpose:  This function is used to update the DTA LIST patricia tree of
           previously stby(now active) and send filtering policies configuration 
           to the existing DTAs

 Input:    CB : DTS_CB instance

 Returns:  NCSCC_RC_SUCCESS if initialization is successful, otherwise
           NCSCC_RC_FAILURE

 Notes:
\**************************************************************************/
uns32 dts_stby_update_dta_config()
{
	DTS_CB *inst = &dts_cb;
	DTSV_MSG *msg = NULL;
	DTA_DEST_LIST *dta_reg = NULL;
	MDS_DEST dta_key;
	uns32 status = NCSCC_RC_SUCCESS;

	m_DTS_LK(&inst->lock);
	m_LOG_DTS_LOCK(DTS_LK_LOCKED, &inst->lock);

	dta_reg = (DTA_DEST_LIST *)ncs_patricia_tree_getnext(&inst->dta_list, NULL);

	/* Using the DTA_LIST patricia tree to traverse all DTAs */
	while (dta_reg != NULL) {
		/* Adjust the pointer to to_reg with the offset */
		dta_reg = (DTA_DEST_LIST *)((long)dta_reg - DTA_DEST_LIST_OFFSET);
		dta_key = dta_reg->dta_addr;
		if (dta_reg->svc_list != NULL) {
			msg = m_MMGR_ALLOC_DTSV_MSG;
			if (msg == NULL) {
				m_DTS_UNLK(&inst->lock);
				m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
				return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
						      "dts_stby_update_dta_config:  Failed to allocate DTSV message");
			}
			memset(msg, '\0', sizeof(DTSV_MSG));

			msg->vrid = inst->vrid;
			msg->msg_type = DTS_FAIL_OVER;
			/* Fill the data, select destination and send to MDS */
			msg->data.data.dta_ptr = (long)dta_reg;

			/*Encode the uba in msg with data */
			if (dts_fail_over_enc_msg(msg) == NCSCC_RC_FAILURE) {
				m_DTS_UNLK(&inst->lock);
				m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
				if (NULL != msg)
					m_MMGR_FREE_DTSV_MSG(msg);
				return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
						      "dts_stby_update_dta_config: Failed to encode msg for fail-over");
			}

			/* Send msg to DTA */
			if (dts_mds_send_msg(msg, dta_key, inst->mds_hdl) != NCSCC_RC_SUCCESS) {
				if (NULL != msg)
					m_MMGR_FREE_DTSV_MSG(msg);
				msg = NULL;
				m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_stby_update_dta_config: DTS: MDS send failed");

				dts_set_dta_up_down(m_NCS_NODE_ID_FROM_MDS_DEST(dta_key), dta_key, FALSE);
			}
		}
		/* end of dta_reg->svc_list != NULL */
		/* Point to next dta */
		dta_reg = (DTA_DEST_LIST *)ncs_patricia_tree_getnext(&inst->dta_list, (const uns8 *)&dta_key);

		/*Free the msg */
		if (msg != NULL)
			m_MMGR_FREE_DTSV_MSG(msg);
	}			/*end of while */

	m_DTS_UNLK(&inst->lock);
	m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);

	return status;
}

/**************************************************************************\
 Function: dts_chk_file_size 

 Purpose:  This function is used for getting file size. 

 Input:    file : file name. 

 Returns:  size in bytes of the file. 

 Notes:  
\**************************************************************************/
uns32 dts_chk_file_size(char *file)
{
	struct stat file_stat;

	if (!stat(file, &file_stat)) {
		return file_stat.st_size;
	}
	return 0;
}

/**************************************************************************\
 Function: dts_enqueue_dta

 Purpose:  This funstion is used to add a DTA_DEST_LIST to
           a svc_reg entry in the patricia tree dta_list.
 
 Input:    dta : DTA_DEST_LIST to be added.
           svc : Service entry to which dta is to be added.

 Returns:  void.

 Notes:    None.
\**************************************************************************/
void dts_enqueue_dta(DTS_SVC_REG_TBL *svc, DTA_DEST_LIST *dta)
{
	DTA_ENTRY *dta_entry, *tmp;

	if (dta != NULL) {
		dta_entry = m_MMGR_ALLOC_DTS_DTA_ENTRY;
		if (dta_entry == NULL) {
			m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_enqueue_dta: Failed to allocate memory");
			return;
		}
		memset(dta_entry, 0, sizeof(DTA_ENTRY));

		tmp = svc->v_cd_list;
		svc->v_cd_list = dta_entry;
		dta_entry->dta = dta;
		dta_entry->next_in_svc_entry = tmp;
		(svc->dta_count)++;
	} else {
		/*Error scenario */
	}
	return;
}

/**************************************************************************\
 Function: dts_add_svc_to_dta

 Purpose:  This funstion is used to add a DTS_SVC_REG_TBL to
           a dta entry in the patricia tree dta_list.
 
 Input:    dta : DTA entry to which service is to be added.
           svc : Service entry to be added.

 Returns:  void.

 Notes:    None.
\**************************************************************************/
void dts_add_svc_to_dta(DTA_DEST_LIST *dta, DTS_SVC_REG_TBL *svc)
{
	SVC_ENTRY *dta_svc_entry, *tmp;

	if (svc != NULL) {
		dta_svc_entry = m_MMGR_ALLOC_DTS_SVC_ENTRY;
		if (dta_svc_entry == NULL) {
			m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_add_svc_to_dta: Failed to allocate memory");
			return;
		}
		memset(dta_svc_entry, 0, sizeof(SVC_ENTRY));
		tmp = dta->svc_list;
		dta->svc_list = dta_svc_entry;
		dta_svc_entry->svc = svc;
		dta_svc_entry->next_in_dta_entry = tmp;
		(dta->dta_num_svcs)++;
	}
	return;
}

/**************************************************************************\
 Function: dts_dequeue_dta

 Purpose:  This funstion is used to delete a DTA_ frm
           a svc_reg entry in the patricia tree svc_tbl.
 
 Input:    dta : dta_entry to be deleted.
           svc : Service entry from which dta_entry is to be deleted.

 Returns:  Pointer to the service entry deleted.
           NULL if the list is empty.

 Notes:    None.
\**************************************************************************/
NCSCONTEXT dts_dequeue_dta(DTS_SVC_REG_TBL *svc, DTA_DEST_LIST *dta)
{
	DTA_DEST_LIST *dta_ptr;
	DTA_ENTRY *dta_entry, *tmp;
	MDS_DEST *key;
	/* Check if svc_list is empty */
	if (svc->v_cd_list == NULL || dta == NULL)
		return (NCSCONTEXT)NULL;

	key = &dta->dta_addr;
	dta_entry = svc->v_cd_list;

	if (dts_find_reg(key, dta_entry->dta) == TRUE) {
		svc->v_cd_list = dta_entry->next_in_svc_entry;
		dta_ptr = dta_entry->dta;
		m_MMGR_FREE_DTS_DTA_ENTRY(dta_entry);
		goto success;
	}

	while ((dta_entry != NULL) && (dta_entry->next_in_svc_entry != NULL)) {
		tmp = dta_entry->next_in_svc_entry;
		if (dts_find_reg(key, dta_entry->next_in_svc_entry->dta) == TRUE) {
			/* Store the svc_reg ptr for return */
			dta_ptr = tmp->dta;
			/*Adjust the linked list */
			dta_entry->next_in_svc_entry = tmp->next_in_svc_entry;
			m_MMGR_FREE_DTS_DTA_ENTRY(tmp);
			goto success;
		}
		dta_entry = dta_entry->next_in_svc_entry;
	}

	/* Failure condition */
	return (NCSCONTEXT)NULL;

	/*Success */
 success:
	(svc->dta_count)--;
	return (NCSCONTEXT)dta_ptr;
}

/**************************************************************************\
 Function: dts_del_svc_frm_dta

 Purpose:  This funstion is used to delete a service entry(DTS_SVC_REG_TBL) frm
           a dta entry in the patricia tree dta_list.
 
 Input:    dta : DTA entry frm which service is to be deleted.
           svc : Service entry to be deleted.

 Returns:  Pointer to the service entry deleted.
           NULL if the list is empty.

 Notes:    None.
\**************************************************************************/
NCSCONTEXT dts_del_svc_frm_dta(DTA_DEST_LIST *dta, DTS_SVC_REG_TBL *svc)
{
	DTS_SVC_REG_TBL *svc_ptr;
	SVC_ENTRY *svc_entry, *tmp;

	/* Check if svc_list is empty */
	if (dta->svc_list == NULL || svc == NULL)
		return (NCSCONTEXT)NULL;

	svc_entry = dta->svc_list;

	if ((svc_entry->svc->my_key.node == svc->my_key.node)
	    && (svc_entry->svc->my_key.ss_svc_id == svc->my_key.ss_svc_id)) {
		dta->svc_list = svc_entry->next_in_dta_entry;
		svc_ptr = svc_entry->svc;
		m_MMGR_FREE_DTS_SVC_ENTRY(svc_entry);
		goto success;
	}

	while ((svc_entry != NULL) && (svc_entry->next_in_dta_entry != NULL)) {
		tmp = svc_entry->next_in_dta_entry;
		if ((tmp->svc->my_key.node == svc->my_key.node)
		    && (tmp->svc->my_key.ss_svc_id == svc->my_key.ss_svc_id)) {
			/* Store the svc_reg ptr for return */
			svc_ptr = tmp->svc;
			/*Adjust the linked list */
			svc_entry->next_in_dta_entry = tmp->next_in_dta_entry;
			m_MMGR_FREE_DTS_SVC_ENTRY(tmp);
			goto success;
		}
		svc_entry = svc_entry->next_in_dta_entry;
	}

	/* Failure condition */
	return (NCSCONTEXT)NULL;

	/*Success */
 success:
	(dta->dta_num_svcs)--;
	return (NCSCONTEXT)svc_ptr;
}

/**************************************************************************\
 Function: dts_del_spec_frm_svc

 Purpose:  This function is used to delete a spec entry(SPEC_ENTRY) frm the
           spec_list of service registration table.
 
 Input:    svc : Service entry from which the spec entry needs to be deleted.
           dta_addr : MDS_DEST of dta whose spec entry has to be deleted.
           ver : Pointer to SPEC_CKPT. Used for return purposes to check the
                 service name and version associated with the entry to be 
                 deleted.

 Returns:  NCSCC_RC_SUCCESS is deletion successful.
           NCSCC_RC_FAILURE on failure.

 Notes:    None.
\**************************************************************************/
uns32 dts_del_spec_frm_svc(DTS_SVC_REG_TBL *svc, MDS_DEST dta_addr, SPEC_CKPT *ver)
{
	SPEC_ENTRY *spec_entry = NULL, *prev = NULL;

	if ((svc == NULL) || (svc->spec_list == NULL))
		return NCSCC_RC_FAILURE;

	spec_entry = svc->spec_list;
	while (spec_entry != NULL) {
		if (spec_entry->dta_addr == dta_addr)
			break;

		prev = spec_entry;
		spec_entry = spec_entry->next_spec_entry;
	}

	if (spec_entry == NULL)
		return NCSCC_RC_FAILURE;

	if (prev != NULL)
		prev->next_spec_entry = spec_entry->next_spec_entry;
	else
		svc->spec_list = spec_entry->next_spec_entry;

	/* Fill the SPEC_CKPT struct for return purposes */
	if (ver != NULL) {
		strcpy(ver->svc_name, spec_entry->svc_name);
		ver->version = spec_entry->spec_struct->ss_spec->ss_ver;
	}
	/*printf("\n#####dts_del_spec_frm_svc(): Spec Details - spec_use_count: %d, lib_use_count: %d, svcname: %s#####\n", spec_entry->spec_struct->use_count, spec_entry->lib_struct->use_count, spec_entry->svc_name);
	   fflush(stdout); */

	/* Decrement the use count of ASCII_SPEC ptr before deletion */
	if (spec_entry->spec_struct != NULL)
		spec_entry->spec_struct->use_count--;
	/* Decrement the use count for library having the spec */
	if (spec_entry->lib_struct != NULL)
		spec_entry->lib_struct->use_count--;

	m_MMGR_FREE_DTS_SVC_SPEC(spec_entry);
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
 Function: dts_find_spec

 Purpose:  This funstion is used to find the right ASCII_SPEC entry in 
           DTS_SVC_REG_TBL's spec searching on MDS_DEST passed
 
 Input:    dta_key : MDS_DEST of DTA whose ASCII_SPEC ptr is required.
           svc : Service whose spec_list is being searched for ASCII_SPEC ptr.

 Returns:  Pointer to ASCII_SPEC table on success. 
           NULL in case search fails.

 Notes:    None.
\**************************************************************************/
NCSCONTEXT dts_find_spec(DTS_SVC_REG_TBL *svc, MDS_DEST *dta_key)
{
	SPEC_ENTRY *spec_entry;

	if (svc->spec_list == NULL)
		return (NCSCONTEXT)NULL;

	spec_entry = svc->spec_list;
	while (spec_entry != NULL) {
		if (memcmp(&spec_entry->dta_addr, dta_key, sizeof(MDS_DEST)) == 0) {
			/*Element found */
			if (spec_entry->spec_struct != NULL)
				return (NCSCONTEXT)spec_entry->spec_struct->ss_spec;
		}
		spec_entry = spec_entry->next_spec_entry;
	}
	return (NCSCONTEXT)NULL;
}

/**************************************************************************\
 Function: dts_find_spec_entry

 Purpose:  This funstion is used to find the right SPEC_ENTRY in 
           DTS_SVC_REG_TBL's spec searching on MDS_DEST passed
 
 Input:    dta_key: MDS_DEST of DTA whose ASCII_SPEC ptr is required.
           svc: Service whose spec_entry is being searched for ASCII_SPEC ptr.

 Returns:  Pointer to SPEC_ENTRY structure on success. 
           NULL in case search fails.

 Notes:    None.
\**************************************************************************/
NCSCONTEXT dts_find_spec_entry(DTS_SVC_REG_TBL *svc, MDS_DEST *dta_key)
{
	SPEC_ENTRY *spec_entry;

	if (svc->spec_list == NULL)
		return (NCSCONTEXT)NULL;

	spec_entry = svc->spec_list;
	while (spec_entry != NULL) {
		if (memcmp(&spec_entry->dta_addr, dta_key, sizeof(MDS_DEST)) == 0) {
			/*Element found */
			return (NCSCONTEXT)spec_entry;
		}
		spec_entry = spec_entry->next_spec_entry;
	}
	return (NCSCONTEXT)NULL;
}

/**************************************************************************\
 Function: dts_find_dta

 Purpose:  This funstion is used to find a DTA_DEST_LIST entry by MDS_DEST in
           a svc_reg's v_cd_list.
 
 Input:    dta_key : Key to DTA_DEST_LIST being searched for.
           svc : Service whose v_cd_list is being searched for dta.

 Returns:  Pointer to dta being searched for on success. 
           NULL in case search fails.

 Notes:    None.
\**************************************************************************/
NCSCONTEXT dts_find_dta(DTS_SVC_REG_TBL *svc, MDS_DEST *dta_key)
{
	DTA_ENTRY *dta_entry;

	if (svc->v_cd_list == NULL)
		return (NCSCONTEXT)NULL;

	dta_entry = svc->v_cd_list;
	while (dta_entry != NULL) {
		if (memcmp(&dta_entry->dta->dta_addr, dta_key, sizeof(MDS_DEST)) == 0) {
			/*Element found */
			return (NCSCONTEXT)dta_entry->dta;
		}
		dta_entry = dta_entry->next_in_svc_entry;
	}
	return (NCSCONTEXT)NULL;
}

/**************************************************************************\
 Function: dts_get_next_node_entry

 Purpose:  This function is used for getting next node entry from the Patricia
           tree. 

 Input:    node : Patricia Tree.
           key  : Key(in network order) to be used for searching the message.

 Returns:  Pointer to the next Node entry in pat tree

 Notes: key is to be supplied in network order 
\**************************************************************************/
NCSCONTEXT dts_get_next_node_entry(NCS_PATRICIA_TREE *node, SVC_KEY *key)
{
	DTS_SVC_REG_TBL *svc;
	SVC_KEY skey;

	/*
	 * If key is NULL it means, we have to search entire tree starting 
	 * from the first node element of the tree. 
	 */
	if (key == NULL) {
		/* 
		 * To search first node entry of the tree start with node id 0 and service
		 * is 0xffffffff (max service id).
		 */
		skey.node = 0;
		skey.ss_svc_id = 0xffffffff;

		/* 
		 * Loop till you find the next node entry in the tree.
		 */
		while (((svc = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(node, (const uns8 *)&skey)) != NULL) &&
		       (svc->my_key.ss_svc_id != 0)) {
			/*  Network order key added */
			skey.node = svc->ntwk_key.node;
			skey.ss_svc_id = 0xffffffff;
		}
	} else {
		key->ss_svc_id = 0xffffffff;

		/* 
		 * Loop till you find the next node entry in the tree.
		 */
		while (((svc = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(node, (const uns8 *)key)) != NULL) &&
		       (svc->my_key.ss_svc_id != 0)) {
			key->node = svc->ntwk_key.node;
			key->ss_svc_id = 0xffffffff;
		}
	}

	return ((NCSCONTEXT)svc);
}

/**************************************************************************\
 Function: dts_get_next_svc_entry

 Purpose:  This function is used for getting next Service entry from the Patricia
           tree. 

 Input:    node : Patricia Tree.
           key  : Key(in network order) to be used for searching the message.

 Returns:  Pointer to the next Service entry in pat tree

 Notes: key is to be supplied in network order  
\**************************************************************************/
NCSCONTEXT dts_get_next_svc_entry(NCS_PATRICIA_TREE *node, SVC_KEY *key)
{
	DTS_SVC_REG_TBL *svc;
	SVC_KEY skey;

	/*
	 * If key is NULL it means, we have to search entire tree starting 
	 * from the first service element of the tree. 
	 */
	if (key == NULL) {
		/* 
		 * To search first node entry of the tree start with node id 1 and service
		 * is 0.
		 */
		skey.node = m_NCS_OS_HTONL(1);
		skey.ss_svc_id = 0;

		/* 
		 * Loop till you find the next service entry in the tree.
		 */
		while (((svc = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(node, (const uns8 *)&skey)) != NULL) &&
		       (svc->my_key.ss_svc_id == 0)) {
			skey.node = svc->ntwk_key.node;
			skey.ss_svc_id = svc->ntwk_key.ss_svc_id;
		}
	} else {
		/* 
		 * Loop till you find the next service entry in the tree.
		 */
		while (((svc = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(node, (const uns8 *)key)) != NULL) &&
		       (svc->my_key.ss_svc_id == 0)) {
			key->node = svc->ntwk_key.node;
			key->ss_svc_id = svc->ntwk_key.ss_svc_id;
		}
	}

	return ((NCSCONTEXT)svc);
}

/*****************************************************************************

  PROCEDURE NAME:    dts_log_msg_to_str

  DESCRIPTION:       Converts the Flex Log message into string.

*****************************************************************************/

uns32 dts_free_msg_content(NCSFL_NORMAL *msg)
{
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME: dts_print_current_config 

  DESCRIPTION:    Prints the DTS Service registration info to file.

  RETURNS:        NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  NOTES:          Configuration data is printed to $DTSV_ROOT_DIRECTORY
*****************************************************************************/
uns32 dts_print_current_config(DTS_CB *cb)
{
	time_t tod;
	char asc_dtime[70] = "", file[250] = "";
	char tfile[40] = "";
	FILE *fh;

	m_GET_ASCII_DATE_TIME_STAMP(tod, asc_dtime);
	strcpy(file, cb->log_path);
	sprintf(tfile, "%s_%s%s", "DTS", asc_dtime, ".config");
	strcat(file, tfile);
	if ((fh = sysf_fopen(file, "a+")) != NULL) {
		m_DTS_LK(&cb->lock);
		dts_print_cb(cb, fh);
		dts_print_svc_reg_pat(cb, fh);
		m_DTS_UNLK(&cb->lock);
		fflush(fh);
		fclose(fh);
	} else
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
				      "dts_print_current_config: Failed to open file for printing configuration data");

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:    dts_print_dta_dest_pat

  DESCRIPTION:       Prints the DTA Patricia tree contents.

*****************************************************************************/
void dts_print_dta_dest_pat(void)
{
	DTS_CB *cb = &dts_cb;
	DTA_DEST_LIST *dta_dest;
	MDS_DEST key;
	SVC_ENTRY *svc_entry;

	memset(&key, 0, sizeof(MDS_DEST));
	printf("\n***Printing DTS dta_list Patricia tree***");
	printf("\n--------------***-----------------\n");

	dta_dest = (DTA_DEST_LIST *)ncs_patricia_tree_getnext(&cb->dta_list, NULL);

	while (dta_dest != NULL) {
		dta_dest = (DTA_DEST_LIST *)((long)dta_dest - DTA_DEST_LIST_OFFSET);
		printf
		    ("\nDta entry : dta_addr={with node_id=%d, dta_key=%d}, dta_up=%d, updt_req=%d, dta_num_svcs=%d}\n",
		     m_NCS_NODE_ID_FROM_MDS_DEST(dta_dest->dta_addr), (uns32)dta_dest->dta_addr, dta_dest->dta_up,
		     dta_dest->updt_req, dta_dest->dta_num_svcs);

		printf("Services registered with this dta are:");
		svc_entry = dta_dest->svc_list;
		while (svc_entry != NULL) {
			printf("\nService ID:%d", svc_entry->svc->my_key.ss_svc_id);
			svc_entry = svc_entry->next_in_dta_entry;
		}
		key = dta_dest->dta_addr;
		dta_dest = (DTA_DEST_LIST *)ncs_patricia_tree_getnext(&cb->dta_list, (const uns8 *)&key);
		printf("\n--------------***-----------------\n");
	}
	return;
}

/*****************************************************************************

  PROCEDURE NAME:    dts_print_svc_reg_pat

  DESCRIPTION:       Prints the svc_reg Patricia tree contents.

*****************************************************************************/
void dts_print_svc_reg_pat(DTS_CB *cb, FILE *fp)
{
	DTS_SVC_REG_TBL *svc;
	SVC_KEY nt_key;
	DTA_ENTRY *dta_entry;
	DTS_LL_FILE *file_node;
	/*OP_DEVICE          *device; */
	uns32 count = 0;
	DTS_CONS_LIST *cons_ptr;
	SPEC_ENTRY *spec_entry = NULL;

	if (fp == NULL)
		fp = stdout;

	fprintf(fp, "\n***Printing DTS svc_tbl Patricia tree***\n");
	fprintf(fp, "\n--------------***-----------------\n");

	svc = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(&cb->svc_tbl, (const uns8 *)NULL);
	while (svc != NULL) {
		fprintf(fp, "\nSvc Reg Entry INFO:");
		fprintf(fp, "\n------------------");

		fprintf(fp, "\nKey: {Node=%d, Svc_ID=%d}", svc->my_key.node, svc->my_key.ss_svc_id);
		fprintf(fp, "\nNetwork-order Key: {Node=%u, Svc_ID=%u}", svc->ntwk_key.node, svc->ntwk_key.ss_svc_id);
		fprintf(fp, "\nPer_node_logging: %d", svc->per_node_logging);

		fprintf(fp, "\nPolicy info:\n------------------");
		fprintf(fp,
			"\n\tLog Dev: %d\n\tLog File Size: %u\n\tLog File format: %d\n\tCircular Buffer Size: %u\n\tBuff_Log_Fmt: %d\n\tLogging Enable(1-TRUE, 2-FALSE): %d\n\tCategory_Bit_Map:%u\n\tSeverity_Bit_Map:%u",
			svc->svc_policy.log_dev, svc->svc_policy.log_file_size, svc->svc_policy.file_log_fmt,
			svc->svc_policy.cir_buff_size, svc->svc_policy.buff_log_fmt, svc->svc_policy.enable,
			svc->svc_policy.category_bit_map, svc->svc_policy.severity_bit_map);

		fprintf(fp, "\nDevice info:\n--------------");
		fprintf(fp,
			"\n\tFile_Open(0-FALSE, 1-TRUE): %d\n\tCurrent File Size: %u\n\tNew File: %d\n\tlast_rec_id: %d\n\tfile_log_fmt_change: %d\n\tbuff_log_fmt_change: %d\n",
			svc->device.file_open, svc->device.cur_file_size, svc->device.new_file, svc->device.last_rec_id,
			svc->device.file_log_fmt_change, svc->device.buff_log_fmt_change);

		fprintf(fp, "\nAssociated DTAs:\n------------------");
		fprintf(fp, "\n\tCount: %d", svc->dta_count);
		dta_entry = svc->v_cd_list;

		while (dta_entry != NULL) {
			fprintf(fp, "\n\tDTA Info:\n\t------------");
			fprintf(fp, "\n\tdta_addr:{node_id= %d, dta_key= %d}",
				m_NCS_NODE_ID_FROM_MDS_DEST(dta_entry->dta->dta_addr), (uns32)dta_entry->dta->dta_addr);
			dta_entry = dta_entry->next_in_svc_entry;
		}

		fprintf(fp, "\nAssociated Log files:\n---------------------");
		fprintf(fp, "\n\tCount: %d", svc->device.log_file_list.num_of_files);
		file_node = svc->device.log_file_list.head;
		/*device = &svc->device; */
		while (file_node != NULL) {
			count++;
			fprintf(fp, "\n\tFile%d : %s", count, file_node->file_name);
			file_node = file_node->next;
		}

		fprintf(fp, "\nAssociated console devices:\n---------------------------");
		fprintf(fp, "\n\tCount: %d", svc->device.num_of_cons_conf);
		cons_ptr = svc->device.cons_list_ptr;
		while (cons_ptr != NULL) {
			fprintf(fp, "\n\tConsole: %s, Severity: %u", cons_ptr->cons_dev, cons_ptr->cons_sev_filter);
			cons_ptr = cons_ptr->next;
		}

		fprintf(fp, "\nAssociated spec structures:\n---------------------------");
		spec_entry = svc->spec_list;
		while (spec_entry != NULL) {
			fprintf(fp, "\n\tMDS DEST: %ld, lib_hdl: 0x%08lx, spec_hdl: 0x%08lx name: %s\n",
				(long)spec_entry->dta_addr, (long)spec_entry->lib_struct, (long)spec_entry->spec_struct,
				spec_entry->svc_name);
			spec_entry = spec_entry->next_spec_entry;
		}

		nt_key = svc->ntwk_key;
		svc = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(&cb->svc_tbl, (const uns8 *)&nt_key);
		fprintf(fp, "\n********************************************\n");
	}

	return;
}

/*****************************************************************************

  PROCEDURE NAME:    dts_print_cb

  DESCRIPTION:       Prints the control block data.

*****************************************************************************/
void dts_print_cb(DTS_CB *cb, FILE *fp)
{
	DTS_LL_FILE *file_node;
	uns32 count = 0;
	DTS_CONS_LIST *cons_ptr;
	ASCII_SPEC_LIB *lib_entry;
	SYSF_ASCII_SPECS *spec_struct = NULL;

	if (fp == NULL)
		fp = stdout;

	fprintf(fp, "***DTS control block info***");
	fprintf(fp, "\n--------------***-----------------\n");

	fprintf(fp,
		"\ndts created = %d\ndts_enbl = %d\nVdest Id = %d\nLog Path = %s\nSAF Component Name = %s\nHealthCheck key = %s\nIn-sync = %d\nCold-Sync in Progress = %d\nCold-Sync Status = %d\nAsync update count :\n\tsvc reg updates = %u\n\tdta dest updates = %u\n\tglobal policy updates = %u\n\tlog updates = %u\nConsole Device(Serial) = %s\nConsole Device(Serial) fd = %d\nCLI Bit Map = %d",
		cb->created, cb->dts_enbl, cb->vaddr, cb->log_path, cb->comp_name.value, cb->health_chk_key.key,
		cb->in_sync, cb->cold_sync_in_progress, cb->cold_sync_done, cb->async_updt_cnt.dts_svc_reg_tbl_updt,
		cb->async_updt_cnt.dta_dest_list_updt, cb->async_updt_cnt.global_policy_updt,
		cb->async_updt_cnt.dts_log_updt, cb->cons_dev, cb->cons_fd, cb->cli_bit_map);

	fprintf(fp, "\nGlobal Policy info:\n-------------------");
	fprintf(fp,
		"\nGlobal Logging = %d\nDefault Logging = %d\nMax number of Log Files = %u\nMax number of console devices = %d\nEnable Sequencing = %d",
		cb->g_policy.global_logging, cb->g_policy.dflt_logging, cb->g_policy.g_num_log_files,
		cb->g_policy.g_num_cons_dev, cb->g_policy.g_enable_seq);

	fprintf(fp, "\nPolicy info:\n------------------");
	fprintf(fp,
		"\n\tLog Dev: %d\n\tLog File Size: %u\n\tLog File format: %d\n\tCircular Buffer Size: %u\n\tBuff_Log_Fmt: %d\n\tLogging Enable(1-TRUE, 2-FALSE): %d\n\tCategory_Bit_Map:%u\n\tSeverity_Bit_Map:%u",
		cb->g_policy.g_policy.log_dev, cb->g_policy.g_policy.log_file_size, cb->g_policy.g_policy.file_log_fmt,
		cb->g_policy.g_policy.cir_buff_size, cb->g_policy.g_policy.buff_log_fmt, cb->g_policy.g_policy.enable,
		cb->g_policy.g_policy.category_bit_map, cb->g_policy.g_policy.severity_bit_map);

	fprintf(fp, "\nDevice info:\n--------------");
	fprintf(fp,
		"\n\tFile_Open(0-FALSE, 1-TRUE): %d\n\tCurrent File Size: %u\n\tNew File: %d\n\tlast_rec_id: %d\n\tfile_log_fmt_change: %d\n\tbuff_log_fmt_change: %d\n",
		cb->g_policy.device.file_open, cb->g_policy.device.cur_file_size, cb->g_policy.device.new_file,
		cb->g_policy.device.last_rec_id, cb->g_policy.device.file_log_fmt_change,
		cb->g_policy.device.buff_log_fmt_change);

	fprintf(fp, "\nAssociated Log files:\n---------------------");
	fprintf(fp, "\n\tCount: %d", cb->g_policy.device.log_file_list.num_of_files);
	file_node = cb->g_policy.device.log_file_list.head;
	while (file_node != NULL) {
		count++;
		fprintf(fp, "\n\tFile%d : %s", count, file_node->file_name);
		file_node = file_node->next;
	}

	fprintf(fp, "\nAssociated console devices:\n---------------------------");
	fprintf(fp, "\n\tCount: %d", cb->g_policy.device.num_of_cons_conf);
	cons_ptr = cb->g_policy.device.cons_list_ptr;
	while (cons_ptr != NULL) {
		fprintf(fp, "\n\tConsole: %s, Severity: %u", cons_ptr->cons_dev, cons_ptr->cons_sev_filter);
		cons_ptr = cons_ptr->next;
	}

	fprintf(fp, "\n\nDynamic libraries loaded\n-------------------------");

	lib_entry = (ASCII_SPEC_LIB *)ncs_patricia_tree_getnext(&cb->libname_asciispec_tree, NULL);
	while (lib_entry != NULL) {
		fprintf(fp, "\n%s  use_count:%d", lib_entry->lib_name, lib_entry->use_count);
		lib_entry =
		    (ASCII_SPEC_LIB *)ncs_patricia_tree_getnext(&cb->libname_asciispec_tree, (const uns8 *)lib_entry->lib_name);
	}

	fprintf(fp, "\n\nASCII_SPECs loaded\n-------------------------");
	spec_struct = (SYSF_ASCII_SPECS *)ncs_patricia_tree_getnext(&cb->svcid_asciispec_tree, NULL);
	while (spec_struct != NULL) {
		fprintf(fp, "\nSvc-id: %d, version: %d, use_count: %d spec_name: %s", spec_struct->key.svc_id,
			spec_struct->key.ss_ver, spec_struct->use_count, spec_struct->ss_spec->svc_name);
		spec_struct =
		    (SYSF_ASCII_SPECS *)ncs_patricia_tree_getnext(&cb->svcid_asciispec_tree,
								  (const uns8 *)&spec_struct->key);
	}

	fprintf(fp, "\n--------------***-----------------\n\n");

	return;
}

/*****************************************************************************

  DESCRIPTION: Following routine prints DTS related datastructures.
               Provided for debugging purposes only.

*****************************************************************************/
void dts_print_reg_tbl_dbg(void)
{
	DTS_CB *cb = &dts_cb;
	DTS_SVC_REG_TBL *svc;
	SVC_KEY nt_key;
	DTA_ENTRY *dta_entry;
	DTS_LL_FILE *file_node;
	uns32 count = 0;
	DTS_CONS_LIST *cons_ptr;
	SPEC_ENTRY *spec_entry = NULL;

	TRACE("\n***Printing DTS svc_tbl Patricia tree***\n");
	TRACE("\n--------------***-----------------\n");

	m_DTS_LK(&cb->lock);
	svc = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(&cb->svc_tbl, (const uns8 *)NULL);
	while (svc != NULL) {
		printf("\nSvc Reg Entry INFO:");
		printf("\n------------------");

		printf("\nKey: {Node=%d, Svc_ID=%d}", svc->my_key.node, svc->my_key.ss_svc_id);
		printf("\nNetwork-order Key: {Node=%d, Svc_ID=%d}", svc->ntwk_key.node, svc->ntwk_key.ss_svc_id);
		printf("\nPer_node_logging: %d", svc->per_node_logging);

		printf("\nPolicy info:\n------------------");
		printf
		    ("\n\tLog Dev: %d\n\tLog File Size: %u\n\tLog File format: %d\n\tCircular Buffer Size: %u\n\tBuff_Log_Fmt: %d\n\tLogging Enable(1-TRUE, 2-FALSE): %d\n\tCategory_Bit_Map:%u\n\tSeverity_Bit_Map:%u",
		     svc->svc_policy.log_dev, svc->svc_policy.log_file_size, svc->svc_policy.file_log_fmt,
		     svc->svc_policy.cir_buff_size, svc->svc_policy.buff_log_fmt, svc->svc_policy.enable,
		     svc->svc_policy.category_bit_map, svc->svc_policy.severity_bit_map);

		printf("\nDevice info:\n--------------");
		printf
		    ("\n\tFile_Open(0-FALSE, 1-TRUE): %d\n\tCurrent File Size: %u\n\tNew File: %d\n\tlast_rec_id: %d\n\tfile_log_fmt_change: %d\n\tbuff_log_fmt_change: %d\n",
		     svc->device.file_open, svc->device.cur_file_size, svc->device.new_file, svc->device.last_rec_id,
		     svc->device.file_log_fmt_change, svc->device.buff_log_fmt_change);

		printf("\nAssociated DTAs:\n------------------");
		printf("\n\tCount: %d", svc->dta_count);
		dta_entry = svc->v_cd_list;

		while (dta_entry != NULL) {
			printf("\n\tDTA Info:\n\t------------");
			printf("\n\tdta_addr:{node_id= %d, dta_key= %d}",
			       m_NCS_NODE_ID_FROM_MDS_DEST(dta_entry->dta->dta_addr), (uns32)dta_entry->dta->dta_addr);
			dta_entry = dta_entry->next_in_svc_entry;
		}

		printf("\nAssociated Log files:\n---------------------");
		printf("\n\tCount: %d", svc->device.log_file_list.num_of_files);
		file_node = svc->device.log_file_list.head;

		while (file_node != NULL) {
			count++;
			printf("\n\tFile%d : %s", count, file_node->file_name);
			file_node = file_node->next;
		}

		printf("\nAssociated console devices:\n---------------------------");
		printf("\n\tCount: %d", svc->device.num_of_cons_conf);
		cons_ptr = svc->device.cons_list_ptr;
		while (cons_ptr != NULL) {
			printf("\n\tConsole: %s, Severity: %u", cons_ptr->cons_dev, cons_ptr->cons_sev_filter);
			cons_ptr = cons_ptr->next;
		}

		printf("\nAssociated spec structures:\n---------------------------");
		spec_entry = svc->spec_list;
		while (spec_entry != NULL) {
			printf("\n\tMDS DEST: %ld, lib_hdl: 0x%08lx, spec_hdl: 0x%08lx name: %s\n",
			       (long)spec_entry->dta_addr, (long)spec_entry->lib_struct, (long)spec_entry->spec_struct,
			       spec_entry->svc_name);
			spec_entry = spec_entry->next_spec_entry;
		}

		nt_key = svc->ntwk_key;
		svc = (DTS_SVC_REG_TBL *)ncs_patricia_tree_getnext(&cb->svc_tbl, (const uns8 *)&nt_key);
		printf("\n********************************************\n");
	}

	fflush(stdout);
	m_DTS_UNLK(&cb->lock);
	return;
}

void dts_print_cb_dbg(void)
{
	DTS_CB *cb = &dts_cb;
	DTS_LL_FILE *file_node;
	uns32 count = 0;
	DTS_CONS_LIST *cons_ptr;
	ASCII_SPEC_LIB *lib_entry;
	SYSF_ASCII_SPECS *spec_struct = NULL;

	printf("\n***DTS control block info***");
	printf("\n--------------***-----------------\n");

	m_DTS_LK(&cb->lock);

	printf
	    ("\ndts created = %d\ndts_enbl = %d\nVdest Id = %d\nLog Path = %s\nSAF Component Name = %s\nHealthCheck key = %s\nIn-sync = %d\nCold-Sync in Progress = %d\nCold-Sync Status = %d\nAsync update count :\n\tsvc reg updates = %u\n\tdta dest updates = %u\n\tglobal policy updates = %u\n\tlog updates = %u\nConsole Device(Serial) = %s\nConsole Device(Serial) fd = %d\nCLI Bit Map = %d",
	     cb->created, cb->dts_enbl, cb->vaddr, cb->log_path, cb->comp_name.value, cb->health_chk_key.key,
	     cb->in_sync, cb->cold_sync_in_progress, cb->cold_sync_done, cb->async_updt_cnt.dts_svc_reg_tbl_updt,
	     cb->async_updt_cnt.dta_dest_list_updt, cb->async_updt_cnt.global_policy_updt,
	     cb->async_updt_cnt.dts_log_updt, cb->cons_dev, cb->cons_fd, cb->cli_bit_map);

	printf("\nGlobal Policy info:\n-------------------");
	printf
	    ("\nGlobal Logging = %d\nDefault Logging = %d\nMax number of Log Files = %u\nMax number of console devices = %d\nEnable Sequencing = %d",
	     cb->g_policy.global_logging, cb->g_policy.dflt_logging, cb->g_policy.g_num_log_files,
	     cb->g_policy.g_num_cons_dev, cb->g_policy.g_enable_seq);

	printf("\nPolicy info:\n------------------");
	printf
	    ("\n\tLog Dev: %d\n\tLog File Size: %u\n\tLog File format: %d\n\tCircular Buffer Size: %u\n\tBuff_Log_Fmt: %d\n\tLogging Enable(1-TRUE, 2-FALSE): %d\n\tCategory_Bit_Map:%u\n\tSeverity_Bit_Map:%u",
	     cb->g_policy.g_policy.log_dev, cb->g_policy.g_policy.log_file_size, cb->g_policy.g_policy.file_log_fmt,
	     cb->g_policy.g_policy.cir_buff_size, cb->g_policy.g_policy.buff_log_fmt, cb->g_policy.g_policy.enable,
	     cb->g_policy.g_policy.category_bit_map, cb->g_policy.g_policy.severity_bit_map);

	printf("\nDevice info:\n--------------");
	printf
	    ("\n\tFile_Open(0-FALSE, 1-TRUE): %d\n\tCurrent File Size: %u\n\tNew File: %d\n\tlast_rec_id: %d\n\tfile_log_fmt_change: %d\n\tbuff_log_fmt_change: %d\n",
	     cb->g_policy.device.file_open, cb->g_policy.device.cur_file_size, cb->g_policy.device.new_file,
	     cb->g_policy.device.last_rec_id, cb->g_policy.device.file_log_fmt_change,
	     cb->g_policy.device.buff_log_fmt_change);

	printf("\nAssociated Log files:\n---------------------");
	printf("\n\tCount: %d", cb->g_policy.device.log_file_list.num_of_files);
	file_node = cb->g_policy.device.log_file_list.head;
	while (file_node != NULL) {
		count++;
		printf("\n\tFile%d : %s", count, file_node->file_name);
		file_node = file_node->next;
	}

	printf("\nAssociated console devices:\n---------------------------");
	printf("\n\tCount: %d", cb->g_policy.device.num_of_cons_conf);
	cons_ptr = cb->g_policy.device.cons_list_ptr;
	while (cons_ptr != NULL) {
		printf("\n\tConsole: %s, Severity: %u", cons_ptr->cons_dev, cons_ptr->cons_sev_filter);
		cons_ptr = cons_ptr->next;
	}

	printf("\n\nDynamic libraries loaded\n-------------------------");
	lib_entry = (ASCII_SPEC_LIB *)ncs_patricia_tree_getnext(&cb->libname_asciispec_tree, NULL);
	while (lib_entry != NULL) {
		printf("\n%s. use count: %d", lib_entry->lib_name, lib_entry->use_count);
		lib_entry =
		    (ASCII_SPEC_LIB *)ncs_patricia_tree_getnext(&cb->libname_asciispec_tree, (const uns8 *)lib_entry->lib_name);
	}

	printf("\n\nASCII_SPECs loaded\n-------------------------");
	spec_struct = (SYSF_ASCII_SPECS *)ncs_patricia_tree_getnext(&cb->svcid_asciispec_tree, NULL);
	while (spec_struct != NULL) {
		printf("\nSvc-id: %d, version: %d", spec_struct->key.svc_id, spec_struct->key.ss_ver);
		spec_struct =
		    (SYSF_ASCII_SPECS *)ncs_patricia_tree_getnext(&cb->svcid_asciispec_tree,
								  (const uns8 *)&spec_struct->key);
	}

	printf("\n--------------***-----------------\n\n");
	fflush(stdout);

	m_DTS_UNLK(&cb->lock);
	return;
}

void dts_printall_svc_per_node(uns32 node_id)
{
	DTS_CB *cb = &dts_cb;
	DTS_SVC_REG_TBL *svc;
	SVC_KEY nt_key;
	DTA_ENTRY *dta_entry;
	DTS_LL_FILE *file_node;
	OP_DEVICE *device;
	uns32 count = 0;
	DTS_CONS_LIST *cons_ptr;

	nt_key.node = m_NCS_OS_HTONL(node_id);
	nt_key.ss_svc_id = 0;

	printf("\n***Printing DTS svc_tbl entries for node:%d***\n", node_id);
	printf("\n--------------***-----------------\n");

	m_DTS_LK(&cb->lock);
	while ((svc = (DTS_SVC_REG_TBL *)dts_get_next_svc_entry(&cb->svc_tbl, (SVC_KEY *)&nt_key)) != NULL) {
		printf("\nSvc Reg Entry INFO:");
		printf("\n------------------");

		printf("\nKey: {Node=%d, Svc_ID=%d}", svc->my_key.node, svc->my_key.ss_svc_id);
		printf("\nPer_node_logging: %d", svc->per_node_logging);

		printf("\nPolicy info:\n------------------");
		printf
		    ("\n\tLog Dev: %d\n\tLog File Size: %d\n\tLog File format: %d\n\tCircular Buffer Size: %d\n\tBuff_Log_Fmt: %d\n\tLogging Enable(1-TRUE, 2-FALSE): %d\n\tCategory_Bit_Map:%u\n\tSeverity_Bit_Map:%u",
		     svc->svc_policy.log_dev, svc->svc_policy.log_file_size, svc->svc_policy.file_log_fmt,
		     svc->svc_policy.cir_buff_size, svc->svc_policy.buff_log_fmt, svc->svc_policy.enable,
		     svc->svc_policy.category_bit_map, svc->svc_policy.severity_bit_map);

		printf("\nDevice info:\n--------------");
		printf
		    ("\n\tFile_Open(0-FALSE, 1-TRUE): %d\n\tCurrent File Size: %d\n\tNew File: %d\n\tlast_rec_id: %d\n\tfile_log_fmt_change: %d\n\tbuff_log_fmt_change: %d\n",
		     svc->device.file_open, svc->device.cur_file_size, svc->device.new_file, svc->device.last_rec_id,
		     svc->device.file_log_fmt_change, svc->device.buff_log_fmt_change);

		printf("\nAssociated DTAs:\n------------------");
		printf("\n\tCount: %d", svc->dta_count);
		dta_entry = svc->v_cd_list;

		while (dta_entry != NULL) {
			printf("\n\tDTA Info:\n\t------------");
			printf("\n\tdta_addr:{node_id= %d, dta_key= %d}",
			       m_NCS_NODE_ID_FROM_MDS_DEST(dta_entry->dta->dta_addr), (uns32)dta_entry->dta->dta_addr);
			dta_entry = dta_entry->next_in_svc_entry;
		}

		printf("\nAssociated Log files:\n---------------------");
		printf("\n\tCount: %d", svc->device.log_file_list.num_of_files);
		file_node = svc->device.log_file_list.head;
		device = &svc->device;
		while (file_node != NULL) {
			count++;
			printf("\n\tFile%d : %s", count, file_node->file_name);
			file_node = file_node->next;
		}

		printf("\nAssociated console devices:\n---------------------------");
		printf("\n\tCount: %d", svc->device.num_of_cons_conf);
		cons_ptr = svc->device.cons_list_ptr;
		while (cons_ptr != NULL) {
			printf("\n\tConsole: %s, Severity: %u", cons_ptr->cons_dev, cons_ptr->cons_sev_filter);
			cons_ptr = cons_ptr->next;
		}

		printf("\n********************************************\n");

		/* Increment the key to get the next service */
		nt_key = svc->ntwk_key;
	}

	m_DTS_UNLK(&cb->lock);
	return;
}

/**************************************************************************\
 Function: dts_handle_immnd_event

 Purpose:  Function used for handling the DTA up down events received
           from MDS.

 Input:    msg : Event message received from the MDS.

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  
\**************************************************************************/
uns32 dts_handle_immnd_event(DTSV_MSG *msg)
{
	DTS_CB *inst = &dts_cb;

	m_DTS_LK(&inst->lock);
	m_LOG_DTS_LOCK(DTS_LK_LOCKED, &inst->lock);

	m_LOG_DTS_SVC_PRVDR(DTS_SP_MDS_RCV_EVT);

	switch (msg->data.data.evt.change) {	/* Review change type */
	case NCSMDS_DOWN:
		break;
	case NCSMDS_UP:
		m_LOG_DTS_DBGSTRLL(DTS_SERVICE, "Received IMMND up event for :", msg->node, (uns32)msg->dest_addr);
		if (inst->imm_init_done == FALSE) {
			if (dts_imm_initialize(inst) != SA_AIS_OK) {
				LOG_ER("imm initialize failed, exiting");
				exit(1);
			}
			if (inst->ha_state == SA_AMF_HA_ACTIVE) {
				dts_reg_with_imm(inst);
			}

			fds[FD_IMM].fd = inst->imm_sel_obj;
			fds[FD_IMM].events = POLLIN;
			nfds = FD_IMM + 1;
			inst->imm_init_done = TRUE;
		}
		break;

	default:
		break;
	}			/* End Switch */
	m_DTS_UNLK(&inst->lock);
	m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);

	return NCSCC_RC_SUCCESS;
}
