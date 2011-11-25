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

  This module does the initialisation of MDS and provides callback functions 
  at Availability Directors. It contains the receive callback that
  is called by MDS for the messages addressed to AvD.
..............................................................................

  FUNCTIONS INCLUDED in this module:

  avd_mds_reg_def - registers the AVD role Service with MDS.
  avd_mds_reg - registers the AVD Service with MDS.

  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#include <avd_msg.h>
#include "avd.h"

const MDS_CLIENT_MSG_FORMAT_VER avd_avnd_msg_fmt_map_table[AVD_AVND_SUBPART_VER_MAX] =
    { AVSV_AVD_AVND_MSG_FMT_VER_1, AVSV_AVD_AVND_MSG_FMT_VER_2, AVSV_AVD_AVND_MSG_FMT_VER_3};
const MDS_CLIENT_MSG_FORMAT_VER avd_avd_msg_fmt_map_table[AVD_AVD_SUBPART_VER_MAX] = { AVD_AVD_MSG_FMT_VER_1, AVD_AVD_MSG_FMT_VER_2, AVD_AVD_MSG_FMT_VER_3};

/* fwd decl */

static uns32 avd_mds_svc_evt(MDS_CALLBACK_SVC_EVENT_INFO *evt_info);
static uns32 avd_mds_qsd_ack_evt(MDS_CALLBACK_QUIESCED_ACK_INFO *evt_info);

/****************************************************************************
  Name          : avd_mds_set_vdest_role
 
  Description   : This routine is used for setting the VDEST role.
 
  Arguments     : cb - ptr to the AVD control block
                  role - Set the role.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avd_mds_set_vdest_role(AVD_CL_CB *cb, SaAmfHAStateT role)
{
	NCSVDA_INFO vda_info;
	uns32 rc;

	memset(&vda_info, '\0', sizeof(NCSVDA_INFO));

	/* set the role of the vdest */
	vda_info.req = NCSVDA_VDEST_CHG_ROLE;
	vda_info.info.vdest_chg_role.i_new_role = role;
	vda_info.info.vdest_chg_role.i_vdest = cb->vaddr;

	if ((rc = ncsvda_api(&vda_info)) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncsvda_api NCSVDA_VDEST_CHG_ROLE failed %u", rc);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/**
 * Initialize MDS. Compile the EDP, create vdest, install svc id on vdest, subscribe, ...
 * @param cb
 * 
 * @return uns32
 */
uns32 avd_mds_init(AVD_CL_CB *cb)
{
	NCSVDA_INFO vda_info;
	NCSMDS_INFO svc_to_mds_info;
	MDS_SVC_ID svc_id[1];
	uns32 rc;
	NCSADA_INFO ada_info;
	EDU_ERR err = EDU_NORMAL;

	TRACE_ENTER();

	/* Initialise the EDU to be used with MDS */
	m_NCS_EDU_HDL_INIT(&cb->edu_hdl);

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl, avsv_edp_dnd_msg, &err);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("m_NCS_EDU_COMPILE_EDP failed");
		m_NCS_EDU_HDL_FLUSH(&cb->edu_hdl);
		return rc;
	}

	/* prepare the cb with the vaddress */
	m_NCS_SET_VDEST_ID_IN_MDS_DEST(cb->vaddr, AVSV_AVD_VCARD_ID);

	memset(&vda_info, '\0', sizeof(NCSVDA_INFO));

	vda_info.req = NCSVDA_VDEST_CREATE;
	vda_info.info.vdest_create.i_persistent = FALSE;
	vda_info.info.vdest_create.i_policy = NCS_VDEST_TYPE_DEFAULT;
	vda_info.info.vdest_create.i_create_type = NCSVDA_VDEST_CREATE_SPECIFIC;
	vda_info.info.vdest_create.info.specified.i_vdest = cb->vaddr;

	/* create Vdest address */
	if (ncsvda_api(&vda_info) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncsvda_api NCSVDA_VDEST_CREATE failed");
		return NCSCC_RC_FAILURE;
	}

	TRACE("vdest created");

	/* store the info returned by MDS */
	cb->vaddr_pwe_hdl = vda_info.info.vdest_create.o_mds_pwe1_hdl;
	cb->vaddr_hdl = vda_info.info.vdest_create.o_mds_vdest_hdl;

	/* Install on mds VDEST */
	svc_to_mds_info.i_mds_hdl = cb->vaddr_pwe_hdl;
	svc_to_mds_info.i_svc_id = NCSMDS_SVC_ID_AVD;
	svc_to_mds_info.i_op = MDS_INSTALL;
	svc_to_mds_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;
	svc_to_mds_info.info.svc_install.i_svc_cb = avd_mds_cbk;
	svc_to_mds_info.info.svc_install.i_mds_q_ownership = FALSE;
	svc_to_mds_info.info.svc_install.i_mds_svc_pvt_ver = AVD_MDS_SUB_PART_VERSION;

	if (ncsmds_api(&svc_to_mds_info) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncsmds_api MDS_INSTALL vdest failed");
		memset(&vda_info, '\0', sizeof(NCSVDA_INFO));
		vda_info.req = NCSVDA_VDEST_DESTROY;
		vda_info.info.vdest_destroy.i_vdest = cb->vaddr;
		ncsvda_api(&vda_info);
		return NCSCC_RC_FAILURE;
	}

	TRACE("mds install vdest");

	/*** subscribe to AVD mds event ***/
	svc_to_mds_info.i_op = MDS_RED_SUBSCRIBE;
	svc_to_mds_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_to_mds_info.info.svc_subscribe.i_svc_ids = svc_id;
	svc_to_mds_info.info.svc_subscribe.i_num_svcs = 1;
	svc_id[0] = NCSMDS_SVC_ID_AVD;

	rc = ncsmds_api(&svc_to_mds_info);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("ncsmds_api MDS_RED_SUBSCRIBE failed");
		memset(&vda_info, '\0', sizeof(NCSVDA_INFO));
		vda_info.req = NCSVDA_VDEST_DESTROY;
		vda_info.info.vdest_destroy.i_vdest = cb->vaddr;
		ncsvda_api(&vda_info);
		return NCSCC_RC_FAILURE;
	}

	/* AVD ADEST create install and subscription */
	memset(&ada_info, 0, sizeof(ada_info));

	ada_info.req = NCSADA_GET_HDLS;

	if (ncsada_api(&ada_info) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncsada_api NCSADA_GET_HDLS failed");
		return NCSCC_RC_FAILURE;
	}

	/* Store values returned by ADA */
	cb->adest_hdl = ada_info.info.adest_get_hdls.o_mds_pwe1_hdl;

	/* Install on mds ADEST */
	svc_to_mds_info.i_mds_hdl = cb->adest_hdl;
	svc_to_mds_info.i_svc_id = NCSMDS_SVC_ID_AVD;
	svc_to_mds_info.i_op = MDS_INSTALL;
	svc_to_mds_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;
	svc_to_mds_info.info.svc_install.i_svc_cb = avd_mds_cbk;
	svc_to_mds_info.info.svc_install.i_mds_q_ownership = FALSE;
	svc_to_mds_info.info.svc_install.i_mds_svc_pvt_ver = AVD_MDS_SUB_PART_VERSION;

	if (ncsmds_api(&svc_to_mds_info) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncsmds_api MDS_INSTALL adest failed");
		memset(&vda_info, '\0', sizeof(NCSVDA_INFO));
		vda_info.req = NCSVDA_VDEST_DESTROY;
		vda_info.info.vdest_destroy.i_vdest = cb->vaddr;
		ncsvda_api(&vda_info);
		return NCSCC_RC_FAILURE;
	}

	TRACE("mds install adest");

	/*** subscribe to avnd, just for not delaying bcast ***/
	svc_to_mds_info.i_mds_hdl = cb->adest_hdl;
	svc_to_mds_info.i_op = MDS_SUBSCRIBE;
	svc_to_mds_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_to_mds_info.info.svc_subscribe.i_svc_ids = svc_id;
	svc_to_mds_info.info.svc_subscribe.i_num_svcs = 1;
	svc_id[0] = NCSMDS_SVC_ID_AVND;

	rc = ncsmds_api(&svc_to_mds_info);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("ncsmds_api MDS_SUBSCRIBE avnd failed");
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : avd_mds_cbk

  Description   : This is a callback routine invoked by MDS for delivering
  messages and events. It also calls back for encoding or decoding messages
  on the Vdest address (from node director).
 
  Arguments     : info:  MDS call back information
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

uns32 avd_mds_cbk(struct ncsmds_callback_info *info)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	switch (info->i_op) {
	case MDS_CALLBACK_RECEIVE:
		/* check that the node director sent this
		 * message to the director.
		 */
		if ((info->info.receive.i_fr_svc_id == NCSMDS_SVC_ID_AVND) &&
		    (info->info.receive.i_to_svc_id == NCSMDS_SVC_ID_AVD)) {
			NODE_ID node_id = 0;
			uns16 msg_fmt_ver = 0;
			node_id = m_NCS_NODE_ID_FROM_MDS_DEST(info->info.receive.i_fr_dest);
			msg_fmt_ver = info->info.receive.i_msg_fmt_ver;
			rc = avd_n2d_msg_rcv((AVD_DND_MSG *)info->info.receive.i_msg, node_id, msg_fmt_ver);
			info->info.receive.i_msg = (NCSCONTEXT)0;
			if (rc != NCSCC_RC_SUCCESS)
				return rc;
		}
		/* check if AvD sent this
		 ** message to the director if not return error.
		 */
		else if ((info->info.receive.i_fr_svc_id == NCSMDS_SVC_ID_AVD) &&
			 (info->info.receive.i_to_svc_id == NCSMDS_SVC_ID_AVD)) {
			rc = avd_d2d_msg_rcv((AVD_D2D_MSG *)info->info.receive.i_msg);
			info->info.receive.i_msg = (NCSCONTEXT)0;
			if (rc != NCSCC_RC_SUCCESS)
				return rc;
		} else {
			LOG_WA("%s: unknown svc id %u", __FUNCTION__, info->info.receive.i_fr_svc_id);
			return NCSCC_RC_FAILURE;
		}
		break;
	case MDS_CALLBACK_SVC_EVENT:
		{
			rc = avd_mds_svc_evt(&info->info.svc_evt);
			if (rc != NCSCC_RC_SUCCESS) {
				return rc;
			}
		}
		break;
	case MDS_CALLBACK_COPY:
		{
			info->info.cpy.o_msg_fmt_ver =
			    avd_avnd_msg_fmt_map_table[info->info.cpy.i_rem_svc_pvt_ver - 1];

			rc = avd_mds_cpy(&info->info.cpy);
			if (rc != NCSCC_RC_SUCCESS)
				return rc;
		}
		break;
	case MDS_CALLBACK_ENC:
	case MDS_CALLBACK_ENC_FLAT:
		{

			if ((info->i_yr_svc_id == NCSMDS_SVC_ID_AVD) &&
			    (info->info.enc.i_to_svc_id == NCSMDS_SVC_ID_AVD)) {
				info->info.enc.o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->info.enc.i_rem_svc_pvt_ver,
										     AVD_AVD_SUBPART_VER_MIN,
										     AVD_AVD_SUBPART_VER_MAX,
										     avd_avd_msg_fmt_map_table);

				if (info->info.enc.o_msg_fmt_ver < AVD_AVD_MSG_FMT_VER_1) {
					LOG_ER("%s: wrong msg fmt ver %u, %u.", __FUNCTION__, info->info.enc.o_msg_fmt_ver,info->info.enc.i_rem_svc_pvt_ver);
					return NCSCC_RC_FAILURE;
				}

				avd_mds_d_enc(&info->info.enc);
			} else if ((info->i_yr_svc_id == NCSMDS_SVC_ID_AVD) &&
				   (info->info.enc.i_to_svc_id == NCSMDS_SVC_ID_AVND)) {
				info->info.enc.o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->info.enc.i_rem_svc_pvt_ver,
										     AVD_AVND_SUBPART_VER_MIN,
										     AVD_AVND_SUBPART_VER_MAX,
										     avd_avnd_msg_fmt_map_table);

				if (info->info.enc.o_msg_fmt_ver < AVSV_AVD_AVND_MSG_FMT_VER_1) {
					LOG_ER("%s: wrong msg fmt ver %u, %u", __FUNCTION__, info->info.enc.o_msg_fmt_ver,info->info.enc.i_rem_svc_pvt_ver);
					return NCSCC_RC_FAILURE;
				}

				rc = avd_mds_enc(&info->info.enc);
				if (rc != NCSCC_RC_SUCCESS)
					return rc;
			} else {
				LOG_ER("%s: svc id %u %u", __FUNCTION__, info->i_yr_svc_id, info->info.enc.i_to_svc_id);
				return NCSCC_RC_FAILURE;
			}
		}

		break;
	case MDS_CALLBACK_DEC:
	case MDS_CALLBACK_DEC_FLAT:
		{
			if ((info->i_yr_svc_id == NCSMDS_SVC_ID_AVD)
			    && (info->info.dec.i_fr_svc_id == NCSMDS_SVC_ID_AVD)) {
				if (!m_NCS_MSG_FORMAT_IS_VALID(info->info.dec.i_msg_fmt_ver,
							       AVD_AVD_SUBPART_VER_MIN,
							       AVD_AVD_SUBPART_VER_MAX, avd_avd_msg_fmt_map_table)) {
					LOG_ER("%s: wrong msg fmt not valid %u", __FUNCTION__, info->info.dec.i_msg_fmt_ver);
					return NCSCC_RC_FAILURE;
				}

				avd_mds_d_dec(&info->info.dec);
			} else if ((info->i_yr_svc_id == NCSMDS_SVC_ID_AVD)
				   && (info->info.dec.i_fr_svc_id == NCSMDS_SVC_ID_AVND)) {
				if (!m_NCS_MSG_FORMAT_IS_VALID(info->info.dec.i_msg_fmt_ver,
							       AVD_AVND_SUBPART_VER_MIN,
							       AVD_AVND_SUBPART_VER_MAX, avd_avnd_msg_fmt_map_table)) {
					LOG_ER("%s: wrong msg fmt not valid %u", __FUNCTION__, info->info.dec.i_msg_fmt_ver);
					return NCSCC_RC_FAILURE;
				}

				rc = avd_mds_dec(&info->info.dec);
			} else {
				LOG_ER("%s: svc id %u %u", __FUNCTION__, info->i_yr_svc_id, info->info.dec.i_fr_svc_id);
				return NCSCC_RC_FAILURE;
			}

			if (rc != NCSCC_RC_SUCCESS)
				return rc;
		}

		break;
	case MDS_CALLBACK_QUIESCED_ACK:
		{
			rc = avd_mds_qsd_ack_evt(&info->info.quiesced_ack);
			if (rc != NCSCC_RC_SUCCESS)
				return rc;
		}
		break;
	default:
		LOG_WA("%s: unknown op %u", __FUNCTION__, info->i_op);
		return NCSCC_RC_FAILURE;
		break;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : avd_mds_svc_evt
 
  Description   : This routine is invoked to inform AvD of MDS events. AvD 
                  had subscribed to these events during MDS registration.
 
  Arguments     : evt_info - ptr to the MDS event info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uns32 avd_mds_svc_evt(MDS_CALLBACK_SVC_EVENT_INFO *evt_info)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_CL_CB *cb = avd_cb;

	switch (evt_info->i_change) {
	case NCSMDS_UP:
		switch (evt_info->i_svc_id) {
		case NCSMDS_SVC_ID_AVD:
			/* if((Is this up from other node) && (Is this Up from an Adest)) */
			if ((evt_info->i_node_id != cb->node_id_avd) && (m_MDS_DEST_IS_AN_ADEST(evt_info->i_dest))) {
				cb->node_id_avd_other = evt_info->i_node_id;
				cb->other_avd_adest = evt_info->i_dest;
			}
			break;

		case NCSMDS_SVC_ID_AVND:
			if (evt_info->i_node_id == cb->node_id_avd) {
				AVD_EVT *evt = calloc(1, sizeof(AVD_EVT));
				assert(evt);
				evt->rcv_evt = AVD_EVT_MDS_AVND_UP;
				cb->local_avnd_adest = evt_info->i_dest;
				if (m_NCS_IPC_SEND(&cb->avd_mbx, evt, NCS_IPC_PRIORITY_HIGH) != NCSCC_RC_SUCCESS) {
					LOG_ER("%s: ncs_ipc_send failed", __FUNCTION__);
					free(evt);
				}
			}
			break;

		default:
			assert(0);
		}
		break;

	case NCSMDS_DOWN:
		switch (evt_info->i_svc_id) {
		case NCSMDS_SVC_ID_AVD:
			/* if(Is this down from an Adest) && (Is this adest same as Adest in CB) */
			if (m_MDS_DEST_IS_AN_ADEST(evt_info->i_dest)
			    && m_NCS_MDS_DEST_EQUAL(&evt_info->i_dest, &cb->other_avd_adest)) {
				memset(&cb->other_avd_adest, '\0', sizeof(MDS_DEST));
			}
			break;

		case NCSMDS_SVC_ID_AVND:
			{
				AVD_EVT *evt = calloc(1, sizeof(AVD_EVT));
				assert(evt);
				evt->rcv_evt = AVD_EVT_MDS_AVND_DOWN;
				evt->info.node_id = m_NCS_NODE_ID_FROM_MDS_DEST(evt_info->i_dest);
				TRACE("avnd %llx down", evt_info->i_dest);
				if (m_NCS_IPC_SEND(&cb->avd_mbx, evt, NCS_IPC_PRIORITY_HIGH) 
						!= NCSCC_RC_SUCCESS) {
					LOG_ER("%s: ncs_ipc_send failed", __FUNCTION__);
					free(evt);
				}
			}
			break;

		default:
			assert(0);
		}
		break;

	default:
		break;
	}

	return rc;
}

/****************************************************************************
  Name          : avd_mds_qsd_ack_evt
 
  Description   : This routine is invoked to inform AvD of MDS quiesce ack 
                  events.  
 
  Arguments     : evt_info - ptr to the MDS event info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : We will post a message on to ourself when we get the quiesced 
                  ack from mds. This new event will not have any info
                  part, only the msg type is enough 
******************************************************************************/
static uns32 avd_mds_qsd_ack_evt(MDS_CALLBACK_QUIESCED_ACK_INFO *evt_info)
{
	AVD_EVT *evt = AVD_EVT_NULL;
	AVD_CL_CB *cb = avd_cb;

	TRACE_ENTER();

	evt = calloc(1, sizeof(AVD_EVT));
	if (evt == AVD_EVT_NULL) {
		LOG_ER("calloc failed");
		assert(0);
	}

	evt->rcv_evt = AVD_EVT_MDS_QSD_ACK;

	if (m_NCS_IPC_SEND(&cb->avd_mbx, evt, NCS_IPC_PRIORITY_HIGH) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: m_NCS_IPC_SEND failed", __FUNCTION__);
		free(evt);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : avd_avnd_mds_send

  Description   : This routine sends MDS message from AvD to AvND. 

  Arguments     : cb - ptr to the AVD control block

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
uns32 avd_avnd_mds_send(AVD_CL_CB *cb, AVD_AVND *nd_node, AVD_DND_MSG *snd_msg)
{
	NCSMDS_INFO snd_mds;
	uns32 rc;

	TRACE_ENTER();

	memset(&snd_mds, '\0', sizeof(NCSMDS_INFO));

	snd_mds.i_mds_hdl = cb->adest_hdl;
	snd_mds.i_svc_id = NCSMDS_SVC_ID_AVD;
	snd_mds.i_op = MDS_SEND;
	snd_mds.info.svc_send.i_msg = (NCSCONTEXT)snd_msg;
	snd_mds.info.svc_send.i_to_svc = NCSMDS_SVC_ID_AVND;
	snd_mds.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;
	snd_mds.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
	snd_mds.info.svc_send.info.snd.i_to_dest = nd_node->adest;

	/*
	 * Now do MDS send.
	 */
	if ((rc = ncsmds_api(&snd_mds)) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: ncsmds_api MDS_SEND failed %u", __FUNCTION__, rc);
		return rc;
	}

	return rc;
}

/*****************************************************************************
 * Function: avd_mds_avd_up_evh
 *
 * Purpose:  This function is the handler for the other AvD up event from
 * mds. The function right now is just a place holder.
 *
 * Input:
 *
 * Returns:
 *
 * NOTES:
 *
 *
 **************************************************************************/

void avd_mds_avd_up_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
        TRACE_ENTER();
}

/*****************************************************************************
 * Function: avd_mds_avd_down_evh
 *
 * Purpose:  This function is the handler for the standby AvD down event from
 * mds. This function will issue restart request to HPI to restart the
 * system controller card on which the standby is running. It then marks the
 * node director on that card as down and migrates all the Service instances
 * to other inservice SUs. It works similar to avd_tmr_rcv_hb_d_evh.
 *
 * Input:
 *
 * Returns:
 *
 * NOTES:
 *
 *
 **************************************************************************/

void avd_mds_avd_down_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
        TRACE_ENTER();
}

/*****************************************************************************
 * Function: avd_standby_avd_down_evh
 *
 * Purpose:  This function is the handler for the active AvD down event from
 * mds. This function will issue restart request to HPI to restart the
 * system controller card on which the active is running. It then marks the
 * node director on that card as down and migrates all the Service instances
 * to other inservice SUs. It works similar to avd_standby_tmr_rcv_hb_d_evh.
 *
 * Input:
 *
 * Returns:
 *
 * NOTES:
 *
 *
 **************************************************************************/

void avd_standby_avd_down_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
        TRACE_ENTER();
}

uns32 avd_mds_send(MDS_SVC_ID i_to_svc, MDS_DEST i_to_dest, NCSCONTEXT i_msg)
{
	uns32 rc;
	NCSMDS_INFO snd_mds = {0};

	snd_mds.i_mds_hdl = avd_cb->adest_hdl;
	snd_mds.i_svc_id = NCSMDS_SVC_ID_AVD;
	snd_mds.i_op = MDS_SEND;
	snd_mds.info.svc_send.i_msg = i_msg;
	snd_mds.info.svc_send.i_to_svc = i_to_svc;
	snd_mds.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;
	snd_mds.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
	snd_mds.info.svc_send.info.snd.i_to_dest = i_to_dest;

	if ((rc = ncsmds_api(&snd_mds)) != NCSCC_RC_SUCCESS)
		LOG_WA("%s: failed %u, to %u@%llx", __FUNCTION__, rc, i_to_svc, i_to_dest);

	return rc;
}

