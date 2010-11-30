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

  DESCRIPTION: This file contains avnd to avnd communication handling functions.

  FUNCTIONS INCLUDED in this module:
  
****************************************************************************/
#include "avnd.h"

extern const AVND_EVT_HDLR g_avnd_func_list[AVND_EVT_MAX];

static uns32 avnd_evt_avnd_avnd_api_msg_hdl(AVND_CB *cb, AVND_EVT *evt);
static uns32 avnd_evt_avnd_avnd_cbk_msg_hdl(AVND_CB *cb, AVND_EVT *evt);
static uns32 avnd_evt_avnd_avnd_api_resp_msg_hdl(AVND_CB *cb, AVND_EVT *evt);
/******************************************************************************
  Name          : avnd_evt_avnd_avnd_msg
 
  Description   : This routine handles events from another AvNDs.
 
  Arguments     : cb  - ptr to the AvND control block.
                  evt - ptr to the AvND event.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_evt_avnd_avnd_evh(AVND_CB *cb, AVND_EVT *evt)
{
	uns32 res = NCSCC_RC_SUCCESS;
	AVSV_ND2ND_AVND_MSG *avnd_avnd_msg = evt->info.avnd;
	AVSV_ND2ND_AVA_MSG *msg = NULL;

	TRACE_ENTER2("%u", avnd_avnd_msg->type);

	if (AVND_AVND_CBK_DEL == avnd_avnd_msg->type) {
		/* This is a Callback Del message */
		AVSV_ND2ND_CBK_DEL *del_cbk = NULL;
		AVND_COMP *o_comp = NULL;
		AVND_COMP_CBK *cbk_rec = NULL;

		del_cbk = &avnd_avnd_msg->info.cbk_del;
		o_comp = m_AVND_INT_EXT_COMPDB_REC_GET(cb->internode_avail_comp_db, del_cbk->comp_name);
		if (NULL == o_comp) {
			m_AVND_AVND_ERR_LOG("Comp not in Inter/Ext Comp DB:Comp and opq_hdl are",
					    &del_cbk->comp_name, del_cbk->opq_hdl, 0, 0, 0);
			return NCSCC_RC_FAILURE;
		}

		m_AVND_COMP_CBQ_ORIG_INV_GET(o_comp, del_cbk->opq_hdl, cbk_rec);

		if (!cbk_rec) {
			m_AVND_AVND_DEBUG_LOG("No callback record found:Comp and opq_hdl are",
					      &del_cbk->comp_name, del_cbk->opq_hdl, 0, 0, 0);
			goto done;
		}

		avnd_comp_cbq_rec_pop_and_del(cb, o_comp, cbk_rec, FALSE);
		goto done;
	}

	msg = avnd_avnd_msg->info.msg;

	if (AVSV_AVA_API_MSG == msg->type) {
		res = avnd_evt_avnd_avnd_api_msg_hdl(cb, evt);
	} else if (AVSV_AVND_AMF_CBK_MSG == msg->type) {
		res = avnd_evt_avnd_avnd_cbk_msg_hdl(cb, evt);
	} else if (AVSV_AVND_AMF_API_RESP_MSG == msg->type) {
		res = avnd_evt_avnd_avnd_api_resp_msg_hdl(cb, evt);
	} else {
		res = NCSCC_RC_FAILURE;
	}

done:
	if (NCSCC_RC_SUCCESS != res) {
		m_AVND_AVND_ERR_LOG("avnd_evt_avnd_avnd_msg:failure:Msg Type and res are",
				    NULL, avnd_avnd_msg->type, res, 0, 0);
	}

	TRACE_LEAVE();
	return res;
}

/******************************************************************************
  Name          : avnd_evt_avnd_avnd_api_msg_hdl
 
  Description   : This routine handles AVA messages (AVSV_AVA_API_MSG) 
                  from proxy AvNDs.
 
  Arguments     : cb  - ptr to the AvND control block.
                  evt - ptr to the AvND event.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_evt_avnd_avnd_api_msg_hdl(AVND_CB *cb, AVND_EVT *evt)
{
	uns32 res = NCSCC_RC_SUCCESS;
	AVND_EVT_TYPE evt_type;

	/* The event is coming from AvND and not from AvA, so we need to 
	   copy mds context information to evt so that we spoof it like 
	   coming from AvA and then call existing AvA handling functions.
	   mds context information can be returned to sender AvND so that
	   it can send response back to waiting AvA. mds context information
	   is needed only in case of SYNC call from AvA ->proxy AvND. 
	   Presently only two calls from AvA are ASYNC and 
	   those are quiescing_compl and ava_resp, these wouln't require
	   mds context information and it will be NULL for these two calls */

	memcpy(&(evt->mds_ctxt), &(evt->info.avnd->mds_ctxt), sizeof(MDS_SYNC_SND_CTXT));

	/* Create AvA message, so that it look like it is coming from AvA. */
	evt_type = evt->info.avnd->info.msg->info.api_info.type - AVSV_AMF_FINALIZE + AVND_EVT_AVA_FINALIZE;
	evt->info.ava.msg = evt->info.avnd->info.msg;

	m_AVND_AVND_ENTRY_LOG("avnd_evt_avnd_avnd_api_msg_hdl(): evt_type is", NULL, evt_type, 0, 0, 0);

	if (AVND_EVT_AVA_COMP_REG == evt_type) {
		if (AVND_LED_STATE_GREEN != cb->led_state) {
			/* AvND Not Ready, so send TRY AGAIN to sender AvND */
			AVSV_AMF_API_INFO *api_info = &evt->info.ava.msg->info.api_info;
			AVSV_AMF_COMP_REG_PARAM *reg = &api_info->param.reg;
			AVND_COMP *comp = 0;
			SaAisErrorT amf_rc = SA_AIS_OK;
			NCS_BOOL msg_from_avnd = TRUE;

			comp = m_AVND_COMPDB_REC_GET(cb->compdb, reg->comp_name);
			if (NULL == comp)
				return NCSCC_RC_FAILURE;

			/* send the response back to AvA */
			amf_rc = SA_AIS_ERR_TRY_AGAIN;
			res = avnd_amf_resp_send(cb, AVSV_AMF_COMP_REG, amf_rc, 0,
						 &api_info->dest, &evt->mds_ctxt, comp, msg_from_avnd);
			return res;
		}		/* if(AVND_LED_STATE_GREEN != cb->led_state) */
	}

	if ((evt_type >= AVND_EVT_AVA_FINALIZE) && (evt_type < AVND_EVT_AVA_MAX)) {
		res = g_avnd_func_list[evt_type] (cb, evt);
	} else {
		res = NCSCC_RC_FAILURE;
		goto done;
	}

 done:

	return res;

}

/******************************************************************************
  Name          : avnd_evt_avnd_avnd_api_resp_msg_hdl
 
  Description   : This routine handles API response message from proxied AvND.
                  It just forwards the message to AvA.
 
  Arguments     : cb  - ptr to the AvND control block.
                  evt - ptr to the AvND event.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_evt_avnd_avnd_api_resp_msg_hdl(AVND_CB *cb, AVND_EVT *evt)
{
	uns32 res = NCSCC_RC_SUCCESS;
	AVSV_ND2ND_AVND_MSG *avnd_msg = evt->info.avnd;
	AVND_COMP *o_comp = NULL;
	AVSV_AMF_API_RESP_INFO *resp_info = &avnd_msg->info.msg->info.api_resp_info;
	SaAmfHAStateT *ha_state = NULL;
	MDS_DEST reg_dest = 0;

	m_AVND_AVND_ENTRY_LOG("avnd_evt_avnd_avnd_api_resp_msg_hdl():Comp,Type and rc are",
			      &avnd_msg->comp_name, resp_info->type, resp_info->rc, 0, 0);

	o_comp = m_AVND_INT_EXT_COMPDB_REC_GET(cb->internode_avail_comp_db, avnd_msg->comp_name);
	if (NULL == o_comp) {
		m_AVND_AVND_ERR_LOG("Couldn't find comp in Inter/Ext Comp DB:Comp,Type and rc are",
				    &avnd_msg->comp_name, resp_info->type, resp_info->rc, 0, 0);
		res = NCSCC_RC_FAILURE;
		goto done;
	}
	reg_dest = o_comp->reg_dest;

/*****************************************************************************
*   Some processing for registration and unregistration starts
*****************************************************************************/
	if (AVSV_AMF_COMP_REG == resp_info->type) {
		if (SA_AIS_OK != resp_info->rc) {
			/* We got comp reg failure. We need to delete the component.  */
			o_comp = m_AVND_INT_EXT_COMPDB_REC_GET(cb->internode_avail_comp_db, avnd_msg->comp_name);
			if (NULL == o_comp) {
				m_AVND_AVND_ERR_LOG("Couldn't find comp in Inter/Ext Comp DB:Comp,Type and rc are",
						    &avnd_msg->comp_name, resp_info->type, resp_info->rc, 0, 0);
				res = NCSCC_RC_FAILURE;
				goto done;
			}
			reg_dest = o_comp->reg_dest;
			m_AVND_SEND_CKPT_UPDT_ASYNC_RMV(cb, o_comp, AVND_CKPT_COMP_CONFIG);
			res = avnd_internode_comp_del(cb, &(cb->internode_avail_comp_db), &(avnd_msg->comp_name));
		} else {
			o_comp = m_AVND_INT_EXT_COMPDB_REC_GET(cb->internode_avail_comp_db, avnd_msg->comp_name);
			if (NULL == o_comp) {
				m_AVND_AVND_ERR_LOG("Couldn't find comp in Inter/Ext Comp DB:Comp,Type and rc are",
						    &avnd_msg->comp_name, resp_info->type, resp_info->rc, 0, 0);
				res = NCSCC_RC_FAILURE;
				goto done;
			}
			reg_dest = o_comp->reg_dest;
			/* Reg reg SUCCESS. Add proxied information to proxy */
			o_comp->reg_resp_pending = FALSE;
			m_AVND_COMP_REG_SET(o_comp);
			res = avnd_comp_proxied_add(cb, o_comp, o_comp->pxy_comp, FALSE);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, o_comp, AVND_CKPT_COMP_PROXY_PROXIED_ADD);
		}
	}

	if (AVSV_AMF_COMP_UNREG == resp_info->type) {
		if (SA_AIS_OK != resp_info->rc) {
			/* We got comp unreg failure. We need to forward the resp to the user. */
		} else {
			/* Unreg SUCCESS. We need to delete the component as well as proxy-proxied 
			   relation */
			o_comp = m_AVND_INT_EXT_COMPDB_REC_GET(cb->internode_avail_comp_db, avnd_msg->comp_name);
			if (NULL == o_comp) {
				m_AVND_AVND_ERR_LOG("Couldn't find comp in Inter/Ext Comp DB:Comp,Type and rc are",
						    &avnd_msg->comp_name, resp_info->type, resp_info->rc, 0, 0);
				res = NCSCC_RC_FAILURE;
				goto done;
			}
			reg_dest = o_comp->reg_dest;
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, o_comp, AVND_CKPT_COMP_PROXY_PROXIED_DEL);
			res = avnd_comp_proxied_del(cb, o_comp, o_comp->pxy_comp, FALSE, NULL);
			m_AVND_SEND_CKPT_UPDT_ASYNC_RMV(cb, o_comp, AVND_CKPT_COMP_CONFIG);
			res = avnd_internode_comp_del(cb, &(cb->internode_avail_comp_db), &(o_comp->name));
		}
	}

	if (AVSV_AMF_HA_STATE_GET == resp_info->type) {
		ha_state = &resp_info->param.ha_get.ha;
	}

/*****************************************************************************
*   Some processing for registration and unregistration ends
*****************************************************************************/

	/* We have to fprwrd this message to AvA.  */
	res = avnd_amf_resp_send(cb, resp_info->type, resp_info->rc, (uns8 *)ha_state,
				 &reg_dest, &avnd_msg->mds_ctxt, NULL, FALSE);

	if (NCSCC_RC_SUCCESS != res) {
		m_AVND_AVND_ERR_LOG
		    ("avnd_evt_avnd_avnd_api_resp_msg_hdl:Msg Send to AvA Failed:Comp,Type,rc and Dest are",
		     &avnd_msg->comp_name, resp_info->type, resp_info->rc, reg_dest, 0);
	}

 done:
	if (NCSCC_RC_SUCCESS != res) {
		m_AVND_AVND_ERR_LOG("avnd_evt_avnd_avnd_api_resp_msg_hdl:Failure:Comp,Type and rc are",
				    &avnd_msg->comp_name, resp_info->type, resp_info->rc, 0, 0);
	}
	return res;

}

/******************************************************************************
  Name          : avnd_evt_avnd_avnd_cbk_msg_hdl
 
  Description   : This routine handles callbacks from other AvNDs. It forwards
                  these requests to AvA.
 
  Arguments     : cb  - ptr to the AvND control block.
                  evt - ptr to the AvND event.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_evt_avnd_avnd_cbk_msg_hdl(AVND_CB *cb, AVND_EVT *evt)
{
	uns32 rc = 0;
	AVSV_ND2ND_AVND_MSG *avnd_msg = evt->info.avnd;
	AVND_COMP *comp = NULL;
	AVSV_AMF_CBK_INFO *cbk_info = avnd_msg->info.msg->info.cbk_info;
	AVSV_AMF_CBK_INFO *cbk_rec = NULL;
	AVND_COMP_CBK *rec = NULL;

	m_AVND_AVND_ENTRY_LOG("avnd_evt_avnd_avnd_cbk_msg_hdl():Type,Hdl and Inv are",
			      NULL, cbk_info->type, cbk_info->hdl, cbk_info->inv, 0);

	/* Create a callback record for storing purpose. */
	rc = avsv_amf_cbk_copy(&cbk_rec, cbk_info);

	if (NCSCC_RC_SUCCESS != rc)
		goto done;

	/* Get the component pointer. */

	if (0 == (comp = m_AVND_INT_EXT_COMPDB_REC_GET(cb->internode_avail_comp_db, cbk_info->param.hc.comp_name))) {
		/* 
		   NOTE : The component name has been taken from health check callback structure,
		   this will not make any difference to other types of callback structures,
		   such as comp_term, csi_set, csi_rem, etc. We can take name from any one
		   of them as the name is the same in all the structures. Like we can use
		   cbk_info->param.csi_set.comp_name or 
		   cbk_info->param.pxied_comp_inst.comp_name.
		 */
		rc = NCSCC_RC_FAILURE;
		m_AVND_AVND_ERR_LOG("Couldn't find comp in Inter/Ext Comp DB:Comp,Type,Hdl and Inv are",
				    &cbk_info->param.hc.comp_name, cbk_info->type, cbk_info->hdl, cbk_info->inv, 0);
		/* free the callback info */
		if (cbk_rec)
			avsv_amf_cbk_free(cbk_rec);

		goto done;
	}

	/* add the record */
	if ((0 != (rec = avnd_comp_cbq_rec_add(cb, comp, cbk_rec, &comp->reg_dest, 0)))) {
		/* assign inv & hdl values */
		/* Here we will preserve the original invocation handle cbk_rec->inv and 
		   use our created handle rec->opq_hdl. We will use orig_opq_hdl handle 
		   when this AvND will forward the callback response back to caller AvND */
		rec->orig_opq_hdl = cbk_rec->inv;
		rec->cbk_info->inv = rec->opq_hdl;
		rec->cbk_info->hdl = comp->reg_hdl;

		m_AVND_SEND_CKPT_UPDT_ASYNC_ADD(cb, rec, AVND_CKPT_COMP_CBK_REC);

		/* send the request if comp is not in orphaned state.
		   in case of orphaned component we will send it later when
		   comp moves back to instantiated. we will free it, if the comp
		   doesnt move to instantiated */
		if (!m_AVND_COMP_PRES_STATE_IS_ORPHANED(comp)) {
			rc = avnd_comp_cbq_rec_send(cb, comp, rec, FALSE);
		}

		if (NCSCC_RC_SUCCESS != rc) {
			m_AVND_AVND_ERR_LOG("Comp Cbk Rec Send Failed:Comp,Type,Hdl,Inv and Dest are",
					    &comp->name, cbk_info->type,
					    cbk_info->hdl, cbk_info->inv, comp->reg_dest);
		}
	} else {
		rc = NCSCC_RC_FAILURE;
		m_AVND_AVND_ERR_LOG("Comp Cbk Rec Add Failed:Comp,Type,Hdl,Inv and Dest are",
				    &comp->name, cbk_info->type, cbk_info->hdl, cbk_info->inv, comp->reg_dest);
	}

	if (NCSCC_RC_SUCCESS != rc && rec) {
		/* pop & delete */
		uns32 found;

		m_AVND_COMP_CBQ_REC_POP(comp, rec, found);
		rec->cbk_info = 0;
		if (found)
			avnd_comp_cbq_rec_del(cb, comp, rec);
	}

 done:

	if (NCSCC_RC_SUCCESS != rc) {
		m_AVND_AVND_ERR_LOG("avnd_evt_avnd_avnd_cbk_msg_hdl():Failure:Type,Hdl and Inv are",
				    NULL, cbk_info->type, cbk_info->hdl, cbk_info->inv, 0);
	}
	return rc;
}

/******************************************************************************
  Name          : avnd_evt_avd_reboot_evh
 
  Description   : This routine handles callbacks from other AvNDs. It forwards
                  these requests to AvA.
 
  Arguments     : cb  - ptr to the AvND control block.
                  evt - ptr to the AvND event.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_evt_avd_reboot_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_D2N_REBOOT_MSG_INFO *info;

        info = &evt->info.avd->msg_info.d2n_reboot_info;
	TRACE_ENTER2("%u, %u", info->node_id, info->msg_id);

        assert(AVSV_D2N_REBOOT_MSG == evt->info.avd->msg_type);

        if (info->msg_id != (cb->rcv_msg_id + 1)) {
		/* We need to go ahead even if there is some mismatch in seq id as this is reboot command and might 
		   have come because of some problem only. */
		LOG_ER("Mismatch in sequence number, info->msg_id %u, cb->rcv_msg_id %u", info->msg_id, cb->rcv_msg_id);
        }

	cb->rcv_msg_id = info->msg_id;
	opensaf_reboot(cb->node_info.nodeId, (char *)cb->node_info.executionEnvironment.value,
			"This node received reboot command from amfd.");

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
