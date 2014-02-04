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

#include "fm.h"

const MDS_CLIENT_MSG_FORMAT_VER fm_fm_msg_fmt_map_table[FM_SUBPART_VER_MAX] = { FM_FM_MSG_FMT_VER_1 };

static uint32_t fm_mds_callback(NCSMDS_CALLBACK_INFO *info);
static uint32_t fm_mds_get_adest_hdls(FM_CB *cb);
static uint32_t fm_mds_svc_evt(FM_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt);
static uint32_t fm_mds_rcv_evt(FM_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info);

static uint32_t fm_encode(MDS_CALLBACK_ENC_INFO *enc_info);
static uint32_t fm_decode(MDS_CALLBACK_DEC_INFO *dec_info);
static uint32_t fm_fm_mds_enc(MDS_CALLBACK_ENC_INFO *enc_info);
static uint32_t fm_fm_mds_dec(MDS_CALLBACK_DEC_INFO *dec_info);
static uint32_t fm_mds_node_evt(FM_CB *cb, MDS_CALLBACK_NODE_EVENT_INFO * node_evt);
static uint32_t fm_fill_mds_evt_post_fm_mbx(FM_CB *cb, FM_EVT *fm_evt, NODE_ID node_id, FM_FSM_EVT_CODE evt_code);

uint32_t
fm_mds_sync_send(FM_CB *fm_cb, NCSCONTEXT msg,
		 NCSMDS_SVC_ID svc_id,
		 MDS_SEND_PRIORITY_TYPE priority,
		 MDS_SENDTYPES send_type, MDS_DEST *i_to_dest, MDS_SYNC_SND_CTXT *mds_ctxt);

uint32_t
fm_mds_async_send(FM_CB *fm_cb, NCSCONTEXT msg,
		  NCSMDS_SVC_ID svc_id,
		  MDS_SEND_PRIORITY_TYPE priority,
		  MDS_SENDTYPES send_type, MDS_DEST i_to_dest, NCSMDS_SCOPE_TYPE bcast_scope);

/****************************************************************************
* Name          : fm_mds_init
*
* Description   : Installs FMS with MDS and subscribes to FM events. 
*
* Arguments     : Pointer to FMS control block 
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
*
* Notes         : None.
*****************************************************************************/
uint32_t fm_mds_init(FM_CB *cb)
{
	NCSMDS_INFO arg;
	MDS_SVC_ID svc_id[] = { NCSMDS_SVC_ID_GFM, NCSMDS_SVC_ID_AVND, NCSMDS_SVC_ID_IMMND };
	MDS_SVC_ID immd_id[2] = { NCSMDS_SVC_ID_IMMD, NCSMDS_SVC_ID_AVD };

/* Get the MDS handles to be used. */
	if (fm_mds_get_adest_hdls(cb) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

/* Install FM on ADEST. */
	memset(&arg, 0, sizeof(NCSMDS_INFO));
	arg.i_mds_hdl = cb->adest_pwe1_hdl;
	arg.i_svc_id = NCSMDS_SVC_ID_GFM;
	arg.i_op = MDS_INSTALL;

	arg.info.svc_install.i_yr_svc_hdl = (MDS_CLIENT_HDL)gl_fm_hdl;
	arg.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;
	arg.info.svc_install.i_svc_cb = fm_mds_callback;
	arg.info.svc_install.i_mds_q_ownership = false;
	arg.info.svc_install.i_mds_svc_pvt_ver = FM_MDS_SUB_PART_VERSION;

	if (ncsmds_api(&arg) != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "MDS_INSTALL failed");
		return NCSCC_RC_FAILURE;
	}

	memset(&arg, 0, sizeof(NCSMDS_INFO));
	arg.i_mds_hdl = cb->adest_pwe1_hdl;
	arg.i_svc_id = NCSMDS_SVC_ID_GFM;

/* Subcribe to AVND and FMSV MDS UP/DOWN events. */
	arg.i_op = MDS_SUBSCRIBE;
	arg.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	arg.info.svc_subscribe.i_num_svcs = 3;
	arg.info.svc_subscribe.i_svc_ids = svc_id;

	if (ncsmds_api(&arg) != NCSCC_RC_SUCCESS) {
/* Subcription failed amd hence uninstall. */
		syslog(LOG_ERR, "MDS_SUBSCRIBE failed");
		arg.i_op = MDS_UNINSTALL;
		ncsmds_api(&arg);
		return NCSCC_RC_FAILURE;
	}

/* Subscribe to IMMD redundant down event */
        memset(&arg, 0, sizeof(NCSMDS_INFO));
        arg.i_mds_hdl = cb->adest_pwe1_hdl;
        arg.i_svc_id = NCSMDS_SVC_ID_GFM;
        arg.i_op = MDS_RED_SUBSCRIBE;
        arg.info.svc_subscribe.i_num_svcs = 2;
        arg.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
        arg.info.svc_subscribe.i_svc_ids = immd_id;
        if (ncsmds_api(&arg) == NCSCC_RC_FAILURE) {
		syslog(LOG_ERR, "MDS_RED_SUBSCRIBE failed");
		arg.i_op = MDS_UNINSTALL;
		ncsmds_api(&arg);
                return NCSCC_RC_FAILURE;
        }

	memset(&arg, 0, sizeof(NCSMDS_INFO));
	arg.i_op = MDS_NODE_SUBSCRIBE;
	arg.i_svc_id = NCSMDS_SVC_ID_GFM;
	arg.i_mds_hdl = cb->adest_pwe1_hdl;
/* Finally, start using MDS API */
	if (ncsmds_api(&arg) != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "MDS_SUBSCRIBE failed");
		memset(&arg, 0, sizeof(NCSMDS_INFO));
		arg.i_mds_hdl = (MDS_HDL)cb->adest_pwe1_hdl;
		arg.i_svc_id = NCSMDS_SVC_ID_GFM;
		arg.i_op = MDS_UNINSTALL;
		ncsmds_api(&arg);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
* Name          : fm_mds_finalize
*
* Description   : Finalizes with MDS.
*
* Arguments     : pointer to FMS control block. 
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE 
*
* Notes         : None.
*****************************************************************************/
uint32_t fm_mds_finalize(FM_CB *cb)
{
	NCSMDS_INFO arg;
	uint32_t return_val;

	memset(&arg, 0, sizeof(NCSMDS_INFO));
	arg.i_mds_hdl = (MDS_HDL)cb->adest_pwe1_hdl;
	arg.i_svc_id = NCSMDS_SVC_ID_GFM;
	arg.i_op = MDS_UNINSTALL;

	return_val = ncsmds_api(&arg);

	return return_val;
}

/****************************************************************************
* Name          : fm_mds_get_adest_hdls
*
* Description   : Gets a handle to the MDS ADEST 
*
* Arguments     : pointer to the FMS control block. 
*
* Return Values : 
*
* Notes         : None.
*****************************************************************************/
static uint32_t fm_mds_get_adest_hdls(FM_CB *cb)
{
	NCSADA_INFO arg;

	memset(&arg, 0, sizeof(NCSADA_INFO));

	arg.req = NCSADA_GET_HDLS;

	if (ncsada_api(&arg) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

	cb->adest = arg.info.adest_get_hdls.o_adest;
	cb->adest_hdl = arg.info.adest_get_hdls.o_mds_adest_hdl;
	cb->adest_pwe1_hdl = arg.info.adest_get_hdls.o_mds_pwe1_hdl;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
* Name          : fm_mds_callback
*
* Description   : Callback registered with MDS 
*
* Arguments     : pointer to the NCSMDS_CALLBACK_INFO structure
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*
* Notes         : None.
*****************************************************************************/
static uint32_t fm_mds_callback(NCSMDS_CALLBACK_INFO *info)
{
	uint32_t cb_hdl;
	FM_CB *cb;
	uint32_t return_val = NCSCC_RC_SUCCESS;

	if (info == NULL) {
		syslog(LOG_INFO, "fm_mds_callback: Invalid param info");
		return NCSCC_RC_SUCCESS;
	}

	cb_hdl = (uint32_t)info->i_yr_svc_hdl;
	cb = (FM_CB *)ncshm_take_hdl(NCS_SERVICE_ID_GFM, cb_hdl);
	if (cb == NULL) {
		syslog(LOG_INFO, "fm_mds_callback: CB retrieve failed");
		return NCSCC_RC_SUCCESS;
	}

	switch (info->i_op) {
/* For intra-process MDS. Not required for us. */
	case MDS_CALLBACK_COPY:
		break;

	case MDS_CALLBACK_ENC:
		return_val = fm_encode(&info->info.enc);
		break;

/* Calling enc for enc_flat too. */
	case MDS_CALLBACK_ENC_FLAT:
		return_val = fm_encode(&info->info.enc_flat);
		break;

	case MDS_CALLBACK_DEC:
		return_val = fm_decode(&info->info.dec);
		break;

/* Calling dec for dec_flat too. */
	case MDS_CALLBACK_DEC_FLAT:
		return_val = fm_decode(&info->info.dec_flat);
		break;

	case MDS_CALLBACK_RECEIVE:
		return_val = fm_mds_rcv_evt(cb, &(info->info.receive));
		break;

/* Received AVM/AVND/GFM UP/DOWN event. */
	case MDS_CALLBACK_SVC_EVENT:
		return_val = fm_mds_svc_evt(cb, &(info->info.svc_evt));
		break;
	case MDS_CALLBACK_NODE_EVENT:
		return_val = fm_mds_node_evt(cb, &(info->info.node_evt));
		break;

	default:
		return_val = NCSCC_RC_FAILURE;
		break;
	}

	ncshm_give_hdl(cb_hdl);

	return return_val;
}

uint32_t fm_send_node_down_to_mbx(FM_CB *cb, uint32_t node_id)
{
	FM_EVT *fm_evt = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	fm_evt = m_MMGR_ALLOC_FM_EVT;
	if (NULL == fm_evt) {
		syslog(LOG_INFO, "fm_mds_rcv_evt: fm_evt allocation FAILED.");
		return NCSCC_RC_FAILURE;
	}
	rc = fm_fill_mds_evt_post_fm_mbx(cb, fm_evt, node_id, FM_EVT_NODE_DOWN);
	if (rc == NCSCC_RC_FAILURE) {
		m_MMGR_FREE_FM_EVT(fm_evt);
		fm_evt = NULL;
		LOG_IN("node down event post to mailbox failed");
	}
	return rc;
}

static void fm_send_svc_down_to_mbx(FM_CB *cb, uint32_t node_id, NCSMDS_SVC_ID svc_id)
{
	FM_EVT *fm_evt = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	fm_evt = m_MMGR_ALLOC_FM_EVT;
	if (NULL == fm_evt) {
		syslog(LOG_INFO, "fm_mds_rcv_evt: fm_evt allocation FAILED.");
		return;
	}
	fm_evt->svc_id = svc_id;
	rc = fm_fill_mds_evt_post_fm_mbx(cb, fm_evt, node_id, FM_EVT_SVC_DOWN);
	if (rc == NCSCC_RC_FAILURE) {
		m_MMGR_FREE_FM_EVT(fm_evt);
		LOG_IN("service down event post to mailbox failed");
		fm_evt = NULL;
	}
	return;
}

/****************************************************************************
* Name          : fm_mds_node_evt
*
* Description   : Function to process MDS NODE/control events.
*
* Arguments     : pointer to FM CB & MDS_CALLBACK_NODE_EVENT_INFO
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*
* Notes         : None.
*****************************************************************************/
static uint32_t fm_mds_node_evt(FM_CB *cb, MDS_CALLBACK_NODE_EVENT_INFO * node_evt)
{
	uint32_t return_val = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	switch (node_evt->node_chg) {
	case NCSMDS_NODE_DOWN:
		if (node_evt->node_id == cb->peer_node_id && cb->control_tipc) {
			/* Process NODE_DOWN only if OpenSAF is controling TIPC */
			LOG_NO("Node Down event for node id %x:", node_evt->node_id);
			return_val = fm_send_node_down_to_mbx(cb, node_evt->node_id);
		}
		break;

	case NCSMDS_NODE_UP:
		break;

	default:

		LOG_IN("Wrong MDS event from GFM.");
		break;
	}
	TRACE_LEAVE();
	return return_val;
}

/****************************************************************************
* Name          : fm_mds_svc_evt
*
* Description   : Function to process MDS service/control events.
*
* Arguments     : pointer to FM CB & MDS_CALLBACK_SVC_EVENT_INFO
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*
* Notes         : None.
*****************************************************************************/
static uint32_t fm_mds_svc_evt(FM_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt)
{
	uint32_t return_val = NCSCC_RC_SUCCESS;
	FM_EVT *fm_evt;
	TRACE_ENTER();

	if (NULL == svc_evt) {
		syslog(LOG_INFO, "fm_mds_svc_evt: svc_evt NULL.");
		return NCSCC_RC_FAILURE;
	}

	switch (svc_evt->i_change) {
	case NCSMDS_DOWN:
		switch(svc_evt->i_svc_id) {
			/* Depend on service downs if OpenSAF is not controling TIPC */
			case NCSMDS_SVC_ID_GFM:
				if (svc_evt->i_node_id == cb->peer_node_id) {
					cb->peer_adest = 0;
					if (!cb->control_tipc)
						TRACE("FM down on: %x", cb->peer_node_id);
				}
				break;
			case NCSMDS_SVC_ID_IMMND:
				if (svc_evt->i_node_id == cb->peer_node_id
							&& !cb->control_tipc) {
					fm_send_svc_down_to_mbx(cb, svc_evt->i_node_id, svc_evt->i_svc_id);
				}
				break;
			case NCSMDS_SVC_ID_AVND:
				if (svc_evt->i_node_id == cb->peer_node_id
							&& !cb->control_tipc) {
					fm_send_svc_down_to_mbx(cb, svc_evt->i_node_id, svc_evt->i_svc_id);
				}
				break;
			default:
				TRACE("Not interested in service down of other services");
				break;
		}

		break;

	case NCSMDS_RED_DOWN:
		switch (svc_evt->i_svc_id) {
			/* Depend on service downs if OpenSAF is not controling TIPC */
			case NCSMDS_SVC_ID_IMMD:
				if (svc_evt->i_node_id == cb->peer_node_id
							&& !cb->control_tipc) {
					fm_send_svc_down_to_mbx(cb, svc_evt->i_node_id, svc_evt->i_svc_id);
				}
				break;
			case NCSMDS_SVC_ID_AVD:
				if (svc_evt->i_node_id == cb->peer_node_id
							&& !cb->control_tipc) {
					fm_send_svc_down_to_mbx(cb, svc_evt->i_node_id, svc_evt->i_svc_id);
				}
				break;
			default:
				TRACE("Not interested in service down of other services");
				break;
		}
		break;

	case NCSMDS_UP:
		switch (svc_evt->i_svc_id) {
		case NCSMDS_SVC_ID_GFM:
			if ((svc_evt->i_node_id != cb->node_id) && (m_MDS_DEST_IS_AN_ADEST(svc_evt->i_dest) == true)) {

				fm_evt = m_MMGR_ALLOC_FM_EVT;
				if (NULL == fm_evt) {
					syslog(LOG_INFO, "fm_mds_svc_evt: fm_evt allocation FAILED.");
					return NCSCC_RC_FAILURE;
				}
				cb->peer_adest = svc_evt->i_dest;
				cb->peer_node_id = svc_evt->i_node_id;
				return_val = fm_fill_mds_evt_post_fm_mbx(cb, fm_evt, cb->peer_node_id, FM_EVT_PEER_UP);

				if (NCSCC_RC_FAILURE == return_val) {
					m_MMGR_FREE_FM_EVT(fm_evt);
					fm_evt = NULL;
				}
			}
			break;
		case NCSMDS_SVC_ID_IMMND:
				if (svc_evt->i_node_id == cb->peer_node_id
							&& !cb->control_tipc)
					cb->immnd_down = false; /* Only IMMND is restartable */
			break;
		default:
			break;
		}
		break;

	default:
		syslog(LOG_INFO, "Wrong MDS event");
		break;
	}

	TRACE_LEAVE();
	return return_val;
}

/***************************************************************************
* Name          : fm_mds_rcv_evt 
*
* Description   : Top level function that receives/processes MDS events.
*                                                                        
* Arguments     : Pointer to FM control block & MDS_CALLBACK_RECEIVE_INFO    
*                                                                           
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE          
*                                                                           
* Notes         : None.   
***************************************************************************/
static uint32_t fm_mds_rcv_evt(FM_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info)
{
	uint32_t return_val = NCSCC_RC_SUCCESS;
	GFM_GFM_MSG *gfm_rcv_msg = NULL;

	if (NULL == rcv_info) {
		syslog(LOG_INFO, "fm_mds_rcv_evt: rcv_info NULL.");
		return NCSCC_RC_FAILURE;
	}

	if (NCSMDS_SVC_ID_GFM == rcv_info->i_fr_svc_id) {
		gfm_rcv_msg = (GFM_GFM_MSG *)rcv_info->i_msg;
		switch (gfm_rcv_msg->msg_type) {
		case GFM_GFM_EVT_NODE_INFO_EXCHANGE:

			cb->peer_node_id = gfm_rcv_msg->info.node_info.node_id;
			cb->peer_node_name.length = gfm_rcv_msg->info.node_info.node_name.length;
			memcpy(cb->peer_node_name.value, gfm_rcv_msg->info.node_info.node_name.value,
			       cb->peer_node_name.length);
			LOG_IN("Peer Node_id  %u : EE_ID %s", cb->peer_node_id, cb->peer_node_name.value);
			break;

		default:
			syslog(LOG_INFO, "Wrong MDS event from GFM.");
			return_val = NCSCC_RC_FAILURE;
			break;
		}
/* Free gfm_rcv_msg here. Mem allocated in decode. */
		m_MMGR_FREE_FM_FM_MSG(gfm_rcv_msg);
	}

	return return_val;
}

/***************************************************************************
* Name          : fm_fill_mds_evt_post_fm_mbx 
*                                                                           
* Description   : Posts an event to mail box.     
*                                                                        
* Arguments     : Control Block, Pointer to event, slot, subslot and Event Code. 
*                                                                           
* Return Values :  NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS                       
*                                                                           
* Notes         :   None.
***************************************************************************/
static uint32_t fm_fill_mds_evt_post_fm_mbx(FM_CB *cb, FM_EVT *fm_evt, NODE_ID node_id, FM_FSM_EVT_CODE evt_code)
{
	fm_evt->evt_code = evt_code;
	fm_evt->node_id = node_id;
	if (m_NCS_IPC_SEND(&cb->mbx, fm_evt, NCS_IPC_PRIORITY_HIGH) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/***************************************************************************
* Name          : fm_mds_sync_send
*
* Description   : Sends a message to destination in SYNC.     
*                                                                        
* Arguments     :  Control Block, Pointer to message, Priority, Send Type, 
*                  Pointer to MDS DEST, Context of the message.   
*                                                                           
* Return Values :  NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS                       
*                                                                           
* Notes         :  None.  
***************************************************************************/
uint32_t fm_mds_sync_send(FM_CB *fm_cb, NCSCONTEXT msg, NCSMDS_SVC_ID svc_id,
		       MDS_SEND_PRIORITY_TYPE priority, MDS_SENDTYPES send_type,
		       MDS_DEST *i_to_dest, MDS_SYNC_SND_CTXT *mds_ctxt)
{
	NCSMDS_INFO info;
	uint32_t return_val;

	memset(&info, '\0', sizeof(NCSMDS_INFO));

	info.i_mds_hdl = (MDS_HDL)fm_cb->adest_pwe1_hdl;
	info.i_svc_id = NCSMDS_SVC_ID_GFM;
	info.i_op = MDS_SEND;

	info.info.svc_send.i_msg = msg;
	info.info.svc_send.i_priority = priority;
	info.info.svc_send.i_to_svc = svc_id;

	if (mds_ctxt) {
		info.info.svc_send.i_sendtype = send_type;
		info.info.svc_send.info.rsp.i_sender_dest = *i_to_dest;
		info.info.svc_send.info.rsp.i_msg_ctxt = *mds_ctxt;
	} else {
		syslog(LOG_INFO, "fm_mds_sync_send: mds_ctxt NULL");
		return NCSCC_RC_FAILURE;
	}

	return_val = ncsmds_api(&info);

	return return_val;
}

/***************************************************************************
* Name          : fm_mds_async_send 
*
*                                                                           
* Description   : Sends a message to destination in async. 
*                                                                        
* Arguments     :  Control Block, Pointer to message, Priority, Send Type, 
*                  Pointer to MDS DEST, Context of the message. 
*                                                                           
* Return Values :  NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS                       
*                                                                           
* Notes         : None 
***************************************************************************/
uint32_t fm_mds_async_send(FM_CB *fm_cb, NCSCONTEXT msg, NCSMDS_SVC_ID svc_id,
			MDS_SEND_PRIORITY_TYPE priority, MDS_SENDTYPES send_type,
			MDS_DEST i_to_dest, NCSMDS_SCOPE_TYPE bcast_scope)
{
	NCSMDS_INFO info;
	uint32_t return_val;

	if (NCSMDS_SVC_ID_GFM == svc_id) {
		memset(&info, 0, sizeof(info));

		info.i_mds_hdl = (MDS_HDL)fm_cb->adest_pwe1_hdl;
		info.i_op = MDS_SEND;
		info.i_svc_id = NCSMDS_SVC_ID_GFM;

		info.info.svc_send.i_msg = msg;
		info.info.svc_send.i_priority = priority;
		info.info.svc_send.i_sendtype = send_type;
		info.info.svc_send.i_to_svc = svc_id;

		memset(&(info.info.svc_send.info.snd.i_to_dest), 0, sizeof(MDS_DEST));

		if (bcast_scope) {
			info.info.svc_send.info.bcast.i_bcast_scope = bcast_scope;
		} else {
			info.info.svc_send.info.snd.i_to_dest = i_to_dest;
		}

		return_val = ncsmds_api(&info);
		if (NCSCC_RC_FAILURE == return_val) {
			syslog(LOG_INFO, "fms async send failed ");
		}
	} else {
		return_val = NCSCC_RC_FAILURE;
		syslog(LOG_INFO, "fm_mds_async_send: MDS Send fail: alt gfm NOT UP/Invalid svc id.");
	}

	return return_val;
}

/*******************************************************************************
* Name          : fm_encode 
* 
* Description   : Encode callback registered with MDS.
*
* Arguments     : Pointer to the MDS callback info struct MDS_CALLBACK_ENC_INFO
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
* 
* Notes         : None.  
********************************************************************************/
static uint32_t fm_encode(MDS_CALLBACK_ENC_INFO *enc_info)
{
	if (NCSMDS_SVC_ID_GFM == enc_info->i_to_svc_id) {
		enc_info->o_msg_fmt_ver = m_MSG_FMT_VER_GET(enc_info->i_rem_svc_pvt_ver,
							    FM_SUBPART_VER_MIN,
							    FM_SUBPART_VER_MAX, fm_fm_msg_fmt_map_table);

		if (enc_info->o_msg_fmt_ver < FM_FM_MSG_FMT_VER_1) {
			syslog(LOG_INFO, "fm_encode: MSG FMT VER from GFM mis-match");
			return NCSCC_RC_FAILURE;
		}

		return fm_fm_mds_enc(enc_info);
	}

	syslog(LOG_INFO, "fm_encode: MDS MSG is not for GFM.");

	return NCSCC_RC_SUCCESS;
}

/********************************************************************************
* Name          : fm_decode
*
* Description   : Decode callback  
*
* Arguments     : Pointer to the MDS callback info struct MDS_CALLBACK_DEC_INFO
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE                        
*                                                                           
* Notes         : None.
*********************************************************************************/
static uint32_t fm_decode(MDS_CALLBACK_DEC_INFO *dec_info)
{
	if (NCSMDS_SVC_ID_GFM == dec_info->i_fr_svc_id) {
		if (!m_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
					   FM_SUBPART_VER_MIN, FM_SUBPART_VER_MAX, fm_fm_msg_fmt_map_table)) {
			syslog(LOG_INFO, "fm_decode: MSG FMT VER from GFM mis-match");
			return NCSCC_RC_FAILURE;
		}

		return fm_fm_mds_dec(dec_info);
	}

	syslog(LOG_INFO, "fm_decode: MDS MSG is not for GFM.");

	return NCSCC_RC_SUCCESS;
}

/***************************************************************************
* Name          : fm_fm_mds_enc
*
* Description   :  To encode GFM related messages
*                                                                        
* Arguments     :Pointer to the MDS callback info struct MDS_CALLBACK_DEC_INFO
*                                                                           
* Return Values :  NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS                       
*                                                                           
* Notes         :    None.
***************************************************************************/
static uint32_t fm_fm_mds_enc(MDS_CALLBACK_ENC_INFO *enc_info)
{
	GFM_GFM_MSG *msg;
	NCS_UBAID *uba;
	uint8_t *data;

	if (NULL == enc_info)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	if ((NULL == enc_info->i_msg) || (NULL == enc_info->io_uba))
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	uba = enc_info->io_uba;
	msg = (GFM_GFM_MSG *)enc_info->i_msg;

	data = ncs_enc_reserve_space(uba, sizeof(uint8_t));
	if (NULL == data)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	ncs_encode_8bit(&data, msg->msg_type);
	ncs_enc_claim_space(uba, sizeof(uint8_t));

	switch (msg->msg_type) {
	case GFM_GFM_EVT_NODE_INFO_EXCHANGE:
		data = ncs_enc_reserve_space(uba, (2 * sizeof(uint32_t)));
		if (data == NULL) {
			m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}

		ncs_encode_32bit(&data, msg->info.node_info.node_id);
		ncs_encode_32bit(&data, msg->info.node_info.node_name.length);
		ncs_enc_claim_space(uba, 2 * sizeof(uint32_t));
		ncs_encode_n_octets_in_uba(uba, msg->info.node_info.node_name.value,
					   (uint32_t)msg->info.node_info.node_name.length);
		break;

	default:
		syslog(LOG_INFO, "fm_fm_mds_enc: Invalid msg type for encode.");
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		break;
	}

	return NCSCC_RC_SUCCESS;
}

/***************************************************************************
* Name          : fm_fm_mds_dec
*                                                                           
* Description   : To decode GFM related messages                                                      
*                                                                        
* Arguments     : Ptr to the MDS callback info struct MDS_CALLBACK_DEC_INFO
*                                                                           
* Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS                        
*                                                                           
* Notes         : None.
***************************************************************************/
static uint32_t fm_fm_mds_dec(MDS_CALLBACK_DEC_INFO *dec_info)
{
	GFM_GFM_MSG *msg;
	NCS_UBAID *uba;
	uint8_t *data;
	uint8_t data_buff[256];

	if (NULL == dec_info->io_uba) {
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	msg = m_MMGR_ALLOC_FM_FM_MSG;
	if (NULL == msg)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	memset(msg, 0, sizeof(GFM_GFM_MSG));

	dec_info->o_msg = msg;
	uba = dec_info->io_uba;

	data = ncs_dec_flatten_space(uba, data_buff, sizeof(uint8_t));
	if (NULL == data)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	msg->msg_type = (GFM_GFM_MSG_TYPE)ncs_decode_8bit(&data);
	ncs_dec_skip_space(uba, sizeof(uint8_t));

	switch (msg->msg_type) {
	case GFM_GFM_EVT_NODE_INFO_EXCHANGE:
		data = ncs_dec_flatten_space(uba, data_buff, 2 * sizeof(uint32_t));
		if (data == NULL) {
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}

		msg->info.node_info.node_id = (uint32_t)ncs_decode_32bit(&data);
		msg->info.node_info.node_name.length = (uint32_t)ncs_decode_32bit(&data);
		ncs_dec_skip_space(uba, 2 * sizeof(uint32_t));

		ncs_decode_n_octets_from_uba(uba, msg->info.node_info.node_name.value,
					     msg->info.node_info.node_name.length);
		break;
	default:
		syslog(LOG_INFO, "fm_fm_mds_dec: Invalid msg for decoding.");
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		break;
	}

	return NCSCC_RC_SUCCESS;
}
