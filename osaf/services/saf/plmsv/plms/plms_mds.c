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
 * licensing terms.
 *
 * Author(s): Emerson Network Power
 *
 */

/**************************************************************************

Description: This function contains the PLMS routines for MDS interaction

****************************************************************************/
#include "ncsgl_defs.h"
#include "plms.h"
#include "ncs_util.h"

/****************** Function Prototypes ***********************************/
static SaUint32T plms_mds_callback(struct ncsmds_callback_info *info);
static SaUint32T plms_mds_rcv(MDS_CALLBACK_RECEIVE_INFO *rcv_info);
static SaUint32T plms_mds_svc_evt(MDS_CALLBACK_SVC_EVENT_INFO *svc_evt);
void plms_mds_unregister();


/*****************************************************************************
 Name    :  plms_get_slot_and_subslot_id_from_node_id

 Description :  To get the physical slot & sbuslot unique  id from the node id

 Arguments   :
*****************************************************************************/
SaUint32T plms_get_slot_and_subslot_id_from_node_id(NCS_NODE_ID node_id)
{
	NCS_PHY_SLOT_ID phy_slot;
	NCS_SUB_SLOT_ID sub_slot;
	m_NCS_GET_PHYINFO_FROM_NODE_ID(node_id, NULL, &phy_slot, &sub_slot);
	return ((sub_slot * NCS_SUB_SLOT_MAX) + (phy_slot));
}
   


/****************************************************************************\
 PROCEDURE NAME : plms_mds_vdest_create

 DESCRIPTION    : This function Creates the Virtual destination for PLMS

 ARGUMENTS      : NULL 

 RETURNS        : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
\****************************************************************************/
SaUint32T plms_mds_vdest_create()
{
	PLMS_CB *cb = plms_cb;
        NCSVDA_INFO arg;
        uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

        memset(&arg, 0, sizeof(arg));

        cb->mdest_id = PLMS_VDEST_ID;

        arg.req = NCSVDA_VDEST_CREATE;
        arg.info.vdest_create.i_persistent = false;
        arg.info.vdest_create.i_policy = NCS_VDEST_TYPE_DEFAULT;
        arg.info.vdest_create.i_create_type = NCSVDA_VDEST_CREATE_SPECIFIC;
        arg.info.vdest_create.info.specified.i_vdest = cb->mdest_id;

        /* Create VDEST */
        rc = ncsvda_api(&arg);
        if (NCSCC_RC_SUCCESS != rc) {
                LOG_ER("PLMS NCSVDA_VDEST_CREATE failed");
                return rc;
        }

        cb->mds_hdl = arg.info.vdest_create.o_mds_pwe1_hdl;

	TRACE_LEAVE();

        return rc;
}
/****************************************************************************
 * Name          : plms_mds_change_role
 *
 * Description   : This function informs mds of our vdest role change
 *
 * Arguments     : cb   : PLMS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *****************************************************************************/
SaUint32T plms_mds_change_role()
{
        NCSVDA_INFO arg;
	PLMS_CB *cb = plms_cb;
        TRACE_ENTER();

        memset(&arg, 0, sizeof(NCSVDA_INFO));

        arg.req = NCSVDA_VDEST_CHG_ROLE;
        arg.info.vdest_chg_role.i_vdest = cb->mdest_id;
        arg.info.vdest_chg_role.i_new_role = cb->mds_role;

        TRACE_LEAVE();
        return ncsvda_api(&arg);
}


/****************************************************************************
  Name          : plms_mds_register

  Description   : This routine registers the PLMS Service with MDS.

  Arguments     : NULL 

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
****************************************************************************/
SaUint32T plms_mds_register()
{
        uint32_t rc = NCSCC_RC_SUCCESS;
        NCSMDS_INFO svc_info;
        MDS_SVC_ID svc_id[1] = { NCSMDS_SVC_ID_PLMA };
        MDS_SVC_ID plms_id[1] = { NCSMDS_SVC_ID_PLMS };
        MDS_SVC_ID plms_hrb_id[1] = { NCSMDS_SVC_ID_PLMS_HRB };
	PLMS_CB *cb = plms_cb;

	TRACE_ENTER();

        /* Create the virtual Destination for  PLMS */
        rc = plms_mds_vdest_create();
        if (NCSCC_RC_SUCCESS != rc) {
                LOG_ER("PLMS - VDEST CREATE FAILED");
                return rc;
        }

        /* Set the role of MDS */
        if (cb->ha_state == SA_AMF_HA_ACTIVE) {
                TRACE_5("Set MDS role to ACTIVE");
                cb->mds_role = V_DEST_RL_ACTIVE;
        } else if (cb->ha_state == SA_AMF_HA_STANDBY) {
                TRACE_5("Set MDS role to STANDBY");
                cb->mds_role = V_DEST_RL_STANDBY;
        } else {
                TRACE_5("Could not set MDS role");
        }

        if (NCSCC_RC_SUCCESS != (rc = plms_mds_change_role())) {
                LOG_ER("MDS role change to %d FAILED", cb->mds_role);
                return rc;
        }

        memset(&svc_info, 0, sizeof(NCSMDS_INFO));

        /* STEP 2 : Install with MDS with service ID NCSMDS_SVC_ID_PLMS. */
        svc_info.i_mds_hdl = cb->mds_hdl;
        svc_info.i_svc_id = NCSMDS_SVC_ID_PLMS;
        svc_info.i_op = MDS_INSTALL;

        svc_info.info.svc_install.i_yr_svc_hdl = 0;
        svc_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;  /*node specific */
        svc_info.info.svc_install.i_svc_cb = plms_mds_callback; /* callback */
        svc_info.info.svc_install.i_mds_q_ownership = false;
        svc_info.info.svc_install.i_mds_svc_pvt_ver = PLMS_MDS_PVT_SUBPART_VERSION;

        if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
                LOG_ER("PLMS - MDS Install Failed");
                return NCSCC_RC_FAILURE;
        }

        /* STEP 3: Subscribe to PLMS for redundancy events */
        memset(&svc_info, 0, sizeof(NCSMDS_INFO));
        svc_info.i_mds_hdl = cb->mds_hdl;
        svc_info.i_svc_id = NCSMDS_SVC_ID_PLMS;
        svc_info.i_op = MDS_RED_SUBSCRIBE;
        svc_info.info.svc_subscribe.i_num_svcs = 1;
        svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
        svc_info.info.svc_subscribe.i_svc_ids = plms_id;
        if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
                LOG_ER("PLMS - MDS Subscribe for redundancy Failed");
                plms_mds_unregister();
                return NCSCC_RC_FAILURE;
        }

       /* STEP 4: Subscribe to PLMA up/down events */
        memset(&svc_info, 0, sizeof(NCSMDS_INFO));
        svc_info.i_mds_hdl = cb->mds_hdl;
        svc_info.i_svc_id = NCSMDS_SVC_ID_PLMS;
        svc_info.i_op = MDS_SUBSCRIBE;
        svc_info.info.svc_subscribe.i_num_svcs = 1;
        svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
        svc_info.info.svc_subscribe.i_svc_ids = svc_id;
        if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
                LOG_ER("PLMS - MDS Subscribe for PLMA up/down Failed");
                plms_mds_unregister();
                return NCSCC_RC_FAILURE;
        }

       /* STEP 5: Subscribe to HRB up/down events */
        memset(&svc_info, 0, sizeof(NCSMDS_INFO));
        svc_info.i_mds_hdl = cb->mds_hdl;
        svc_info.i_svc_id = NCSMDS_SVC_ID_PLMS;
        svc_info.i_op = MDS_SUBSCRIBE;
        svc_info.info.svc_subscribe.i_num_svcs = 1;
        svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_INTRANODE;
        svc_info.info.svc_subscribe.i_svc_ids = plms_hrb_id;
        if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
                LOG_ER("PLMS - MDS Subscribe for PLMS_HRB up/down Failed");
                plms_mds_unregister();
                return NCSCC_RC_FAILURE;
        }


        /* Get the node id of local PLMS */
        cb->node_id = m_NCS_GET_NODE_ID;

        cb->plms_self_id = plms_get_slot_and_subslot_id_from_node_id(cb->node_id);
        TRACE_5("NodeId:%x SelfId:%x", cb->node_id, cb->plms_self_id);
	TRACE_LEAVE();
        return NCSCC_RC_SUCCESS;
}
/***********************************************************************
* Name          : plms_mds_cpy
*
* Description   : This function copies an events received.
*
* Arguments     : clbk_info(input)
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*
* Notes         : None.
**********************************************************************/
SaUint32T plms_mds_cpy(struct ncsmds_callback_info *clbk_info)
{
	PLMS_HPI_REQ    *dst_msg = NULL;
	PLMS_HPI_REQ    *src_msg = NULL;

	src_msg = (PLMS_HPI_REQ *)clbk_info->info.cpy.i_msg;

	dst_msg = (PLMS_HPI_REQ *)calloc(1,sizeof(PLMS_HPI_REQ));
	if(NULL == dst_msg){
		 LOG_ER("Memory allocation failed ret:%s",strerror(errno));
		 assert(0);
	}


	dst_msg->entity_path = (SaStringT)calloc(1,strlen(src_msg->entity_path) +1);
	if(NULL == dst_msg->entity_path){
		 LOG_ER("Memory allocation failed ret:%s",strerror(errno));
		 assert(0);
	}

	memcpy(dst_msg->entity_path,src_msg->entity_path,src_msg->entity_path_len+1);	

	dst_msg->entity_path_len = src_msg->entity_path_len;
	dst_msg->cmd = src_msg->cmd;
	dst_msg->arg = src_msg->arg;
	
	clbk_info->info.cpy.o_cpy = (uint8_t*)dst_msg;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
Name          : plms_mds_callback

Description   : This callback routine will be called by MDS on event arrival

Arguments     : info - pointer to the mds callback info

Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

Notes         : None.
*****************************************************************************/
SaUint32T plms_mds_callback(struct ncsmds_callback_info *info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	PLMS_CB *cb = plms_cb;

	assert(info != NULL);

	switch (info->i_op) {
		case MDS_CALLBACK_COPY:
			rc = plms_mds_cpy(info);
			break;
		case MDS_CALLBACK_ENC:
			rc = plms_mds_enc(&info->info.enc, &cb->edu_hdl);
			break;
		case MDS_CALLBACK_DEC:
			rc = plms_mds_dec(&info->info.dec,&cb->edu_hdl);
			break;
		case MDS_CALLBACK_ENC_FLAT:
			if (1) {        /*Set to zero for righorous testing of byteorderr
				   enc/dec. */
				rc = plms_mds_enc_flat( info,&cb->edu_hdl);
			} else {
				rc = plms_mds_enc( &info->info.enc_flat,&cb->edu_hdl);
			}
			break;
		case MDS_CALLBACK_DEC_FLAT:
			if (1) {        /*Set to zero for righorous testing of byteorderr
				    enc/dec. */
				rc = plms_mds_dec_flat( info,&cb->edu_hdl);
			} else {
				rc = plms_mds_dec( &info->info.dec_flat,&cb->edu_hdl);
			}
			break;
		case MDS_CALLBACK_RECEIVE:
			rc = plms_mds_rcv(&(info->info.receive));
			break;
		case MDS_CALLBACK_SVC_EVENT:
			rc = plms_mds_svc_evt(&(info->info.svc_evt));
			break;
		case MDS_CALLBACK_QUIESCED_ACK:
			rc = plms_proc_quiesced_ack_evt();
			break;
		case MDS_CALLBACK_DIRECT_RECEIVE:
			rc = NCSCC_RC_FAILURE;
			TRACE_1("MDS_CALLBACK_DIRECT_RECEIVE - do nothing");
			break;
		default:
			LOG_ER("Illegal type of MDS message");
			rc = NCSCC_RC_FAILURE;
			break;
	}
	return rc;
}


/****************************************************************************
 * Name          : plms_mds_rcv
 *
 * Description   : MDS will call this function on receiving PLMS messages.
 *
 * Arguments     : rcv_info - MDS Receive information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
****************************************************************************/
static SaUint32T plms_mds_rcv(MDS_CALLBACK_RECEIVE_INFO *rcv_info)
{
	PLMS_CB * cb = plms_cb;
	uint32_t rc = NCSCC_RC_SUCCESS;
	PLMS_EVT *pEvt = (PLMS_EVT *)rcv_info->i_msg;

	pEvt->sinfo.ctxt = rcv_info->i_msg_ctxt;
	pEvt->sinfo.dest = rcv_info->i_fr_dest;
	pEvt->sinfo.to_svc = rcv_info->i_fr_svc_id;
	
	if (rcv_info->i_rsp_reqd) {
		pEvt->sinfo.stype = MDS_SENDTYPE_RSP;
	}

	/* Put it in PLMS's Event Queue */
	rc = m_NCS_IPC_SEND(&cb->mbx, (NCSCONTEXT)pEvt, NCS_IPC_PRIORITY_NORMAL);

	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("PLMS - IPC SEND FAILED");
	}

	return rc;
}
	   
/****************************************************************************
 * Name          : plms_mds_svc_evt
 *
 * Description   : PLMS is informed when MDS events occur that he has
 *                 subscribed to
 *
 * Arguments     :
 *   cb          : PLMS control Block.
 *   enc_info    : Svc evt info.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/
static SaUint32T plms_mds_svc_evt(MDS_CALLBACK_SVC_EVENT_INFO *svc_evt)
{
	PLMS_EVT *evt = NULL;
	PLMS_CB *cb = plms_cb;
	uint32_t rc;

	evt = (PLMS_EVT *)calloc(1, sizeof(PLMS_EVT));
	if (!evt) {
		LOG_ER("PLMS - Evt Alloc Failed");
		return NCSCC_RC_OUT_OF_MEM;
	}

	/* Service  PLMA events at PLMS */
	memset(evt, 0, sizeof(PLMS_EVT));
	evt->req_res= PLMS_REQ;
	evt->req_evt.req_type = PLMS_MDS_INFO_EVT_T;
	evt->req_evt.mds_info.change = svc_evt->i_change;
	evt->req_evt.mds_info.dest = svc_evt->i_dest;
	evt->req_evt.mds_info.svc_id = svc_evt->i_svc_id;
	evt->req_evt.mds_info.node_id = svc_evt->i_node_id;

	/* Put it in PLMS's Event Queue */
	rc = m_NCS_IPC_SEND(&cb->mbx, (NCSCONTEXT)evt, NCS_IPC_PRIORITY_HIGH);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("PLMS - IPC SEND FAILED");
		free(evt);
	}
	return rc;
}
	     

/****************************************************************************
 * Name          : plms_mds_unregister
 *
 * Description   : This function un-registers the PLMS Service with MDS.
 *
 * Arguments     : cb   : PLMS control Block pointer.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/
	  
void plms_mds_unregister()
{
	NCSMDS_INFO arg;
	uint32_t rc;
	PLMS_CB * cb = plms_cb;
	
	/* NCSVDA_INFO    vda_info; */

	TRACE_ENTER();

	/* Un-install your service into MDS.
	   No need to cancel the services that are subscribed */
	memset(&arg, 0, sizeof(NCSMDS_INFO));

	arg.i_mds_hdl = cb->mds_hdl;
	arg.i_svc_id = NCSMDS_SVC_ID_PLMS;
	arg.i_op = MDS_UNINSTALL;

	if ((rc = ncsmds_api(&arg)) != NCSCC_RC_SUCCESS) {
		LOG_ER("PLMS - MDS Unregister Failed rc:%u", rc);
		goto done;
       }
done:
	TRACE_LEAVE();
	return;
}

