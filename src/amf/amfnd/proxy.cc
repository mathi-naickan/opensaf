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

  DESCRIPTION: This file contains proxy internode & external proxy component
               handling functions.

  FUNCTIONS INCLUDED in this module:
  
****************************************************************************/
#include "amf/amfnd/avnd.h"
#include "base/osaf_extended_name.h"
static uint32_t avnd_int_ext_comp_val(AVND_CB *, const std::string&, AVND_COMP **, SaAisErrorT *);
/******************************************************************************
  Name          : avnd_evt_mds_avnd_up
 
  Description   : This routine handles MDS UP event of AvNDs.
 
  Arguments     : cb  - ptr to the AvND control block.
                  evt - ptr to the AvND event.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_evt_mds_avnd_up_evh(AVND_CB *cb, AVND_EVT *evt)
{
	uint32_t res = 0;

	TRACE_ENTER();

	if (evt->type != AVND_EVT_MDS_AVND_UP) {
		return NCSCC_RC_FAILURE;
	}

	/* Add node id to mds dest mapping in the data base. */
	res = avnd_nodeid_mdsdest_rec_add(cb, evt->info.mds.mds_dest);

	TRACE_LEAVE();
	return res;
}

/******************************************************************************
  Name          : avnd_evt_mds_avnd_dn
 
  Description   : This routine handles MDS DOWN event of AvNDs.
 
  Arguments     : cb  - ptr to the AvND control block.
                  evt - ptr to the AvND event.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_evt_mds_avnd_dn_evh(AVND_CB *cb, AVND_EVT *evt)
{
	uint32_t res = 0;

	TRACE_ENTER();

	if (evt->type != AVND_EVT_MDS_AVND_DN) {
		return NCSCC_RC_FAILURE;
	}

	if (cb->term_state != AVND_TERM_STATE_OPENSAF_SHUTDOWN_STARTED) {
		/* Delete node id to mds dest mapping in the data base. */
		res = avnd_nodeid_mdsdest_rec_del(cb, evt->info.mds.mds_dest);
	}

	TRACE_LEAVE();
	return res;
}

/******************************************************************************
  Name          : avnd_evt_ava_comp_val_req
 
  Description   : This routine creates a validation req msg and sends to AvD.
 
  Arguments     : cb  - ptr to the AvND control block.
                  evt - ptr to the AvND event.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_evt_ava_comp_val_req(AVND_CB *cb, AVND_EVT *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVND_DND_MSG_LIST *rec = 0;
	AVND_MSG msg;
	AVSV_AMF_API_INFO *api_info = &evt->info.ava.msg->info.api_info;
	AVSV_AMF_COMP_REG_PARAM *reg = &api_info->param.reg;
	const std::string comp_name = Amf::to_string(&reg->comp_name);

	TRACE_ENTER2("%s,Type=%u,Hdl=%llx",
			      comp_name.c_str(), api_info->type, reg->hdl);

	memset(&msg, 0, sizeof(AVND_MSG));

	/* populate the msg */
	msg.info.avd = static_cast<AVSV_DND_MSG*>(calloc(1, sizeof(AVSV_DND_MSG)));
	msg.type = AVND_MSG_AVD;
	msg.info.avd->msg_type = AVSV_N2D_COMP_VALIDATION_MSG;
	msg.info.avd->msg_info.n2d_comp_valid_info.msg_id = ++(cb->snd_msg_id);
	msg.info.avd->msg_info.n2d_comp_valid_info.node_id = cb->node_info.nodeId;
	msg.info.avd->msg_info.n2d_comp_valid_info.comp_name =
	    evt->info.ava.msg->info.api_info.param.reg.comp_name;

	/* add the record to the AvD msg list */
	if ((0 != (rec = avnd_diq_rec_add(cb, &msg)))) {
		/* These parameters would not be encoded or decoded so, wouldn't be sent to AvD. */
		rec->msg.info.avd->msg_info.n2d_comp_valid_info.hdl = reg->hdl;
		rec->msg.info.avd->msg_info.n2d_comp_valid_info.proxy_comp_name = reg->proxy_comp_name;
		rec->msg.info.avd->msg_info.n2d_comp_valid_info.mds_dest = api_info->dest;
		rec->msg.info.avd->msg_info.n2d_comp_valid_info.mds_ctxt = evt->mds_ctxt;
		/* send the message */
		rc = avnd_diq_rec_send(cb, rec);

		if ((NCSCC_RC_SUCCESS != rc) && rec) {
			LOG_ER("avnd_diq_rec_send:failed:%s,Type:%u and Hdl%llx",
					    comp_name.c_str(), api_info->type, reg->hdl);
			/* pop & delete */
			m_AVND_DIQ_REC_FIND_POP(cb, rec);
			avnd_diq_rec_del(cb, rec);
		}
	} else {
		rc = NCSCC_RC_FAILURE;
		LOG_ER("avnd_diq_rec_add failed::%s,Type:%u and Hdl%llx",
					comp_name.c_str(), api_info->type, reg->hdl);
	}

	if (NCSCC_RC_FAILURE == rc) {
		LOG_ER("avnd_evt_ava_comp_val_req:%s,Type:%u and Hdl%llx",
						comp_name.c_str(), api_info->type, reg->hdl); 
	}
	/* free the contents of avnd message */
	avnd_msg_content_free(cb, &msg);

	return rc;

}

/******************************************************************************
  Name          : avnd_evt_avd_comp_validation_resp_msg
 
  Description   : This routine creates a registration message and sends it to 
                  corresponding AvND, if the AvND is UP. If AvND is Down then
                  it sends resp to AvA as TRY_AGAIN.
 
  Arguments     : cb  - ptr to the AvND control block.
                  evt - ptr to the AvND event.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_evt_avd_comp_validation_resp_evh(AVND_CB *cb, AVND_EVT *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVND_DND_MSG_LIST *rec = 0;
	AVSV_D2N_COMP_VALIDATION_RESP_INFO *info = nullptr;
	SaAisErrorT amf_rc = SA_AIS_OK;
	AVND_COMP *comp = nullptr, *pxy_comp = nullptr;
	AVSV_N2D_COMP_VALIDATION_INFO comp_valid_info;
	AVSV_AMF_API_INFO api_info;
	std::string info_comp_name;

	TRACE_ENTER();

	/* dont process unless AvD is up */
	if (!m_AVND_CB_IS_AVD_UP(cb))
		goto done;

	info = &evt->info.avd->msg_info.d2n_comp_valid_resp_info;
	info_comp_name = Amf::to_string(&info->comp_name);

	TRACE("%s:MsgId=%u,NodeId=%u,result:%u",
			      info_comp_name.c_str(), info->msg_id, info->node_id, info->result);

	m_AVND_DIQ_REC_FIND(cb, info->msg_id, rec);

	if ((nullptr == rec) || (memcmp(osaf_extended_name_borrow(&info->comp_name),
									osaf_extended_name_borrow(&rec->msg.info.avd->msg_info.n2d_comp_valid_info.comp_name),
									osaf_extended_name_length(&info->comp_name)) != 0)) { 
		/* Seems the rec was deleted, some problem. */
		LOG_ER("Valid Rep:Rec is NULL or Name Mismatch:%s:MsgId:%u,NodeId:%u,result:%u",
				    info_comp_name.c_str(), info->msg_id, info->node_id, info->result);
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	comp_valid_info = rec->msg.info.avd->msg_info.n2d_comp_valid_info;

	if (AVSV_VALID_FAILURE == info->result) {
		/* Component is not configured. Send registration failure to the proxy. */
		amf_rc = SA_AIS_ERR_INVALID_PARAM;
		goto send_resp;

	} else if (AVSV_VALID_SUCC_COMP_NODE_UP == info->result) {
		/* So, let us add this component in the data base. And send reg req
		   to the AvND, where proxied comp is running. */
		comp = avnd_internode_comp_add(cb, info_comp_name,
				info->node_id, &rc, false, false);
		if ((comp) && (SA_AIS_OK == rc)) {
			/* Fill other informations here */
			comp->reg_hdl = comp_valid_info.hdl;
			comp->reg_dest = comp_valid_info.mds_dest;
			comp->mds_ctxt = rec->msg.info.avd->msg_info.n2d_comp_valid_info.mds_ctxt;
			/* We need to update node id attribute in rec as it was node id of proxy
			   actually it should be node id of the proxied component.  */
			comp_valid_info.node_id = info->node_id;

			/* Create proxy-proxied support here */
			if ((pxy_comp = avnd_compdb_rec_get(cb->compdb, Amf::to_string(&comp_valid_info.proxy_comp_name))) == nullptr) {
				avnd_internode_comp_del(cb, info_comp_name);
				rc = NCSCC_RC_FAILURE;
				amf_rc = SA_AIS_ERR_INVALID_PARAM;
				goto send_resp;
			}

			if ((nullptr != pxy_comp) && (!m_AVND_COMP_IS_REG(pxy_comp))) {
				avnd_internode_comp_del(cb, info_comp_name);
				rc = NCSCC_RC_FAILURE;
				amf_rc = SA_AIS_ERR_NOT_EXIST;
				goto send_resp;
			}

			/* When REG RESP will come from other AvND, then we will need
			   proxy of this component, so maintain a link here,
			   though the same link will be generated after REG RESP
			   comes back in avnd_comp_proxied_add (called from 
			   avnd_comp_reg_prc function) */

			comp->pxy_comp = pxy_comp;

			/* Send a registration message to the corresponding AvND */
			memset(&api_info, 0, sizeof(AVSV_AMF_API_INFO));
			avnd_comp_reg_msg_fill(&api_info, comp->reg_dest,
								   comp->reg_hdl, comp->name, comp->pxy_comp->name);
			rc = avnd_avnd_msg_send(cb, (uint8_t *)&(api_info), AVSV_AMF_COMP_REG,
						&comp->mds_ctxt, comp->node_id);

			if (rc != NCSCC_RC_SUCCESS) {
				LOG_ER("avnd_avnd_msg_send failed:%s:MsgId:%u,NodeId:%u,result:%u",
						    info_comp_name.c_str(), info->msg_id, info->node_id, rc);

				amf_rc = SA_AIS_ERR_TRY_AGAIN;
				avnd_internode_comp_del(cb, info_comp_name);
				goto send_resp;
			}
			comp->reg_resp_pending = true;
			goto done;
		} else {
			amf_rc = static_cast<SaAisErrorT>(rc);
			rc = NCSCC_RC_FAILURE;
			goto send_resp;
		}

	} else if (AVSV_VALID_SUCC_COMP_NODE_DOWN == info->result) {
		/* Component node is down. Send TRY AGAIN to AvA. 
		   We cann't send reg succ to AvA as we cann't validate
		   the registration message, it is validated by the proxied
		   component AvND and then only we send Reg Succ to AvA. */
		amf_rc = SA_AIS_ERR_TRY_AGAIN;
		goto send_resp;

	} /* else if(AVSV_VALID_SUCC_COMP_NODE_DOWN == info->result) */
	else {
		/* Wrong result */
		amf_rc = SA_AIS_ERR_TRY_AGAIN;
		rc = NCSCC_RC_FAILURE;
		goto send_resp;
	}

 send_resp:

	/* send the response back to AvA */
	rc = avnd_amf_resp_send(cb, AVSV_AMF_COMP_REG, amf_rc, 0,
				&comp_valid_info.mds_dest, &comp_valid_info.mds_ctxt, nullptr, false);

 done:
	if (rec) {
		m_AVND_DIQ_REC_FIND_POP(cb, rec);
		avnd_diq_rec_del(cb, rec);
	}

	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("avnd_evt_avd_comp_validation_resp_msg failed:%s:MsgId:%u,NodeId:%u,result:%u",
				    info_comp_name.c_str(), info->msg_id, info->node_id, info->result);
	}
	TRACE_LEAVE();
	return rc;
}

/******************************************************************************
  Name          : avnd_avnd_msg_send
 
  Description   : This routine sends messages of type AVSV_AMF_API_TYPE to the 
                  corresponding AvND in ASYNC.
 
  Arguments     : cb  - ptr to the AvND control block.
                  info - ptr to the msg information to be sent.
                  type - Type of the AvA message.
                  ctxt - Ptr to the MDS Context.
                  node_id - Node Id of the AvND to be sent.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_avnd_msg_send(AVND_CB *cb, uint8_t *msg_info, AVSV_AMF_API_TYPE type, MDS_SYNC_SND_CTXT *ctxt, NODE_ID node_id)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVND_MSG msg;
	AVSV_ND2ND_AVA_MSG *nd_nd_ava_msg = nullptr;
	MDS_DEST i_to_dest = 0;
	AVSV_AMF_API_INFO *info = (AVSV_AMF_API_INFO *)msg_info;

	TRACE_ENTER2("Type%u,NodeID=%u",type,node_id);

	/* Create a Registration message and send to AvND */
	memset(&msg, 0, sizeof(AVND_MSG));

	/* populate the msg */
	msg.info.avnd = static_cast<AVSV_ND2ND_AVND_MSG*>(calloc(1, sizeof(AVSV_ND2ND_AVND_MSG)));
	msg.type = AVND_MSG_AVND;

	nd_nd_ava_msg = static_cast<AVSV_NDA_AVA_MSG*>(calloc(1, sizeof(AVSV_NDA_AVA_MSG)));

	msg.info.avnd->type = AVND_AVND_AVA_MSG;
	msg.info.avnd->info.msg = nd_nd_ava_msg;
	memcpy(&msg.info.avnd->mds_ctxt, ctxt, sizeof(MDS_SYNC_SND_CTXT));

	switch (type) {
	case AVSV_AMF_COMP_REG:
	case AVSV_AMF_FINALIZE:
	case AVSV_AMF_COMP_UNREG:
	case AVSV_AMF_PM_START:
	case AVSV_AMF_PM_STOP:
	case AVSV_AMF_HC_START:
	case AVSV_AMF_HC_STOP:
	case AVSV_AMF_HC_CONFIRM:
	case AVSV_AMF_CSI_QUIESCING_COMPLETE:
	case AVSV_AMF_HA_STATE_GET:
	case AVSV_AMF_PG_START:
	case AVSV_AMF_PG_STOP:
	case AVSV_AMF_ERR_REP:
	case AVSV_AMF_ERR_CLEAR:
	case AVSV_AMF_RESP:
		{
			nd_nd_ava_msg->type = AVSV_AVA_API_MSG;
			memcpy(&nd_nd_ava_msg->info.api_info, info, sizeof(AVSV_AMF_API_INFO));
			break;
		}		/* case AVSV_AMF_HC_START */

	default:
		goto done;
		break;

	}			/* switch(type) */

	/* Check node id value. If it is zero then it is an external component.
	   So, we need to use Vdest of Controller AvND. */
	if (node_id != 0)
		i_to_dest = avnd_get_mds_dest_from_nodeid(cb, node_id);
	else {
		i_to_dest = cb->cntlr_avnd_vdest;
	}

	if (0 != i_to_dest)
		rc = avnd_avnd_mds_send(cb, i_to_dest, &msg);
	else
		rc = NCSCC_RC_FAILURE;

 done:
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("%s,AvND Send Failure:Type:%u,NodeID:%u, Mds:%" PRId64 ", rc:%u",
				    __FUNCTION__, type, node_id, i_to_dest, rc);
	}

	/* free the contents of the message */
	avnd_msg_content_free(cb, &msg);

	return rc;
}

/******************************************************************************
  Name          : avnd_int_ext_comp_hdlr

  Description   : This routine checks for int/ext comp and forwards the req 
                  to AvND.

  Arguments     : cb  - ptr to the AvND control block.
                  api_info - ptr to the api info structure.
                  ctxt - ptr to mds context information.
                  o_amf_rc - ptr to the amf-rc (o/p).

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None
******************************************************************************/
uint32_t avnd_int_ext_comp_hdlr(AVND_CB *cb, AVSV_AMF_API_INFO *api_info,
			     MDS_SYNC_SND_CTXT *ctxt, SaAisErrorT *o_amf_rc, bool *int_ext_comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVND_COMP *o_comp = nullptr;
	std::string comp_name;
	bool send_resp = true;
	AVND_COMP_CBK *cbk_rec = 0;

	*o_amf_rc = SA_AIS_OK;
	*int_ext_comp = false;

	switch (api_info->type) {
	case AVSV_AMF_COMP_UNREG:
		{
			comp_name = Amf::to_string(&api_info->param.unreg.comp_name);
			break;
		}

	case AVSV_AMF_HC_START:
		{
			comp_name = Amf::to_string(&api_info->param.hc_start.comp_name);
			break;
		}

	case AVSV_AMF_HC_STOP:
		{
			comp_name = Amf::to_string(&api_info->param.hc_stop.comp_name);
			break;
		}

	case AVSV_AMF_HC_CONFIRM:
		{
			comp_name = Amf::to_string(&api_info->param.hc_confirm.comp_name);
			break;
		}

	case AVSV_AMF_PM_START:
		{
			comp_name = Amf::to_string(&api_info->param.pm_start.comp_name);
			break;
		}

	case AVSV_AMF_PM_STOP:
		{
			comp_name = Amf::to_string(&api_info->param.pm_stop.comp_name);
			break;
		}

	case AVSV_AMF_CSI_QUIESCING_COMPLETE:
		{
			comp_name = Amf::to_string(&api_info->param.csiq_compl.comp_name);
			send_resp = false;
			break;
		}

	case AVSV_AMF_HA_STATE_GET:
		{
			comp_name = Amf::to_string(&api_info->param.ha_get.comp_name);
			break;
		}

	case AVSV_AMF_ERR_REP:
		{
			comp_name = Amf::to_string(&api_info->param.err_rep.err_comp);
			break;
		}

	case AVSV_AMF_ERR_CLEAR:
		{
			comp_name = Amf::to_string(&api_info->param.err_clear.comp_name);
			break;
		}

	case AVSV_AMF_RESP:
		{
			comp_name = Amf::to_string(&api_info->param.resp.comp_name);
			send_resp = false;
			break;
		}

	default:
		rc = NCSCC_RC_FAILURE;
		*o_amf_rc = SA_AIS_ERR_INVALID_PARAM;
		LOG_ER("avnd_int_ext_comp_hdlr:Wrong Type: Type:%u,Mds Dest:%" PRId64,
				    api_info->type, api_info->dest);
		goto done;
		break;
	}

	TRACE("%s: Type=%u", comp_name.c_str(), api_info->type);

	rc = avnd_int_ext_comp_val(cb, comp_name, &o_comp, o_amf_rc);
	if ((NCSCC_RC_SUCCESS == rc) && (SA_AIS_OK == *o_amf_rc)) {
		*int_ext_comp = true;
/*****************************  Section 1 Starts  **********************/
		if ((AVSV_AMF_RESP == api_info->type) || (AVSV_AMF_CSI_QUIESCING_COMPLETE == api_info->type)) {
			AVSV_AMF_RESP_PARAM *resp = &api_info->param.resp;
/*******************************************************************************
We need to consider AVSV_AMF_RESP/AVSV_AMF_CSI_QUIESCING_COMPLETE, here. 
Since this may a response for an internode/ext component's for callbacks, 
so we might have stored the callback in the component cbk_list. 
We need to find them and remove from the cbk_list list and the forward the 
resp to originator AvND.
*******************************************************************************/
			/* get the matching entry from the cbk list. Note that if the resp
			   has come from AvA and the component name has been found in 
			   internode_avail_comp_db, then definetely the comp is a proxied 
			   component and not a proxy component, so need not to search in the
			   comp->pxied_list list for resp->inv. */
			m_AVND_COMP_CBQ_INV_GET(o_comp, resp->inv, cbk_rec);

			if (!cbk_rec) {
				LOG_ER("avnd_int_ext_comp_hdlr:Couldn't get cbk_rec:%s,Type:%u,Mds:%" PRId64, comp_name.c_str(),
				     api_info->type, api_info->dest);
				rc = NCSCC_RC_FAILURE;
				goto done;
			}
			/* We got the callback record, so before deleting it, replace the 
			   invocation handle in the response with the original one. Check
			   function avnd_evt_avnd_avnd_cbk_msg_hdl()'s comments */
			resp->inv = cbk_rec->orig_opq_hdl;
			avnd_comp_cbq_rec_pop_and_del(cb, o_comp, cbk_rec, false);

		}

		/* if(AVSV_AMF_RESP == api_info->type)  */
 /************************* Section 1 Ends *********************************/
 /************************* Section 2 Starts  ******************************/
		/* If the registration request has been sent to proxied component AvND,
		   then we should obviate the other operations on the proxied component
		   till we get the SUCC response and we finally consider this component as 
		   a valid registered component.
		 */
		if (true == o_comp->reg_resp_pending) {
			/* Let at least this operation complete. */
			*o_amf_rc = SA_AIS_ERR_TRY_AGAIN;
			goto resp_send;
		}

/************************* Section 2 Ends *********************************/

		/* We need to forward this req to other AvND */
		rc = avnd_avnd_msg_send(cb, (uint8_t *)api_info, api_info->type, ctxt, o_comp->node_id);
		if (NCSCC_RC_SUCCESS != rc) {
			/* We couldn't send this to other AvND, tell user to try again.  */
			*o_amf_rc = SA_AIS_ERR_TRY_AGAIN;
			LOG_ER("avnd_int_ext_comp_hdlr:Msg Send Failed:%s:Type:%u,Mds:%" PRId64,
					    comp_name.c_str(), api_info->type, api_info->dest);
			goto resp_send;
		} else {
			/* Send SUCCESSFULLY. Return. */
			return rc;
		}
	} else {
		/* This is not an internode/ext component, so return SUCCESS. */
		*int_ext_comp = false;
		return NCSCC_RC_SUCCESS;
	}

 resp_send:

	if (true == send_resp) {
		rc = avnd_amf_resp_send(cb, api_info->type, *o_amf_rc, 0, &api_info->dest, ctxt, nullptr, false);
	}

 done:
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("avnd_int_ext_comp_hdlr():Failure:%s,Type:%u,Mds Dest:%" PRId64,
				    comp_name.c_str(), api_info->type, api_info->dest);
	}
	return rc;
}

/******************************************************************************
  Name          : avnd_int_ext_comp_val

  Description   : This routine checks for int/ext comp.

  Arguments     : cb  - ptr to the AvND control block.
                  api_info - ptr to the api info structure.
                  ctxt - ptr to mds context information.
                  o_amf_rc - ptr to the amf-rc (o/p).

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None
******************************************************************************/
uint32_t avnd_int_ext_comp_val(AVND_CB *cb, const std::string& comp_name, AVND_COMP **o_comp, SaAisErrorT *o_amf_rc)
{
	uint32_t res = NCSCC_RC_SUCCESS;
	*o_amf_rc = SA_AIS_OK;

	TRACE_ENTER2("%s",comp_name.c_str());

	if ((*o_comp = m_AVND_INT_EXT_COMPDB_REC_GET(cb->internode_avail_comp_db, comp_name)) == nullptr) {
		return NCSCC_RC_FAILURE;
	} else {
		/* This means that this is an internode component. But need to check wether
		   it is a proxy for external component. If it is, then we shouldn't treat
		   it as an external component though it is in internode DB. This is bz of
		   a proxy on Ctrl is proxying external component. */
		if (m_AVND_PROXY_IS_FOR_EXT_COMP(*o_comp)){
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
	}
	TRACE_LEAVE();
	return res;
}

/******************************************************************************
  Name          : avnd_avnd_cbk_del_send

  Description   : This routine sends Delete Callback to a particular AvND.

  Arguments     : cb  - ptr to the AvND control block.
                  comp_name - ptr to the comp name.
                  opq_hdl - ptr to handle.
                  node_id - ptr to the node_id.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None
******************************************************************************/
uint32_t avnd_avnd_cbk_del_send(AVND_CB *cb, const std::string &comp_name, uint32_t *opq_hdl, NODE_ID *node_id)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	MDS_DEST i_to_dest = 0;
	AVND_MSG msg;
	SaNameT name;

	TRACE_ENTER2("%s,NodeID=%u,opq_hdl=%u",
				 comp_name.c_str(), *node_id, *opq_hdl);
	osaf_extended_name_alloc(comp_name.c_str(), &name);

	/* Create a Registration message and send to AvND */
	memset(&msg, 0, sizeof(AVND_MSG));

	/* populate the msg */
	msg.info.avnd = static_cast<AVSV_ND2ND_AVND_MSG*>(calloc(1, sizeof(AVSV_ND2ND_AVND_MSG)));
	msg.type = AVND_MSG_AVND;
	msg.info.avnd->type = AVND_AVND_CBK_DEL;
	msg.info.avnd->info.cbk_del.comp_name = name;
	msg.info.avnd->info.cbk_del.opq_hdl = *opq_hdl;

	i_to_dest = avnd_get_mds_dest_from_nodeid(cb, *node_id);

	rc = avnd_avnd_mds_send(cb, i_to_dest, &msg);

 	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("AvND Send Failure:%s:NodeID:%u,opq_hdl:%u,MdsDest:%" PRId64,
			   		comp_name.c_str(), *node_id, *opq_hdl, i_to_dest);
	}

	/* free the contents of the message */
	avnd_msg_content_free(cb, &msg);

	return rc;
}
