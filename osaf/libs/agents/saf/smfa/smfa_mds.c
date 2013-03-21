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
* Author(s): GoAhead Software
*
*/
/*************************************************************************** 
 * @file	smfa_mds.c
 * @brief	This file contains all MDS related functions.	
 * @author	GoAhead Software
*****************************************************************************/

#include "smfa.h"

extern SMFA_CB _smfa_cb;

uint32_t smfa_mds_callback(struct ncsmds_callback_info *);
uint32_t smfa_mds_rcv_cbk(MDS_CALLBACK_RECEIVE_INFO *);
uint32_t smfa_mds_svc_evt_cbk(MDS_CALLBACK_SVC_EVENT_INFO *);

/*************************************************************************** 
@brief		: Register with MDS and Subscribe to SMFND svc evts.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*****************************************************************************/
uint32_t smfa_mds_register()
{
	NCSADA_INFO arg;
	NCSMDS_INFO svc_info;
	MDS_SVC_ID svc_id[1] = { NCSMDS_SVC_ID_SMFND };
	SMFA_CB *cb = &_smfa_cb;
	
	/* Get MDS handle.*/		
	memset(&arg, 0, sizeof(NCSADA_INFO));
	arg.req = NCSADA_GET_HDLS;
	if (NCSCC_RC_SUCCESS != ncsada_api(&arg)){
		LOG_ER("SMFA: MDS get handle FAILED.");
		return NCSCC_RC_FAILURE;
	}
	cb->smfa_mds_hdl = arg.info.adest_get_hdls.o_mds_pwe1_hdl;

	/* Install with MDS. */ 
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));
	svc_info.i_mds_hdl = cb->smfa_mds_hdl;
	svc_info.i_svc_id = NCSMDS_SVC_ID_SMFA;
	svc_info.i_op = MDS_INSTALL;

	svc_info.info.svc_install.i_yr_svc_hdl = 0;
	svc_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_INTRANODE;
	svc_info.info.svc_install.i_svc_cb = smfa_mds_callback;
	svc_info.info.svc_install.i_mds_q_ownership = false;
	if (NCSCC_RC_SUCCESS != ncsmds_api(&svc_info)){
		LOG_ER("SMFA: MDS Install FAILED.");
		return NCSCC_RC_FAILURE;
	}

	/* Subscribe for SMFND UP/DOWN evetns.*/
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));
	svc_info.i_mds_hdl = cb->smfa_mds_hdl;
	svc_info.i_svc_id = NCSMDS_SVC_ID_SMFA;
	svc_info.i_op = MDS_SUBSCRIBE;

	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_INTRANODE;
	svc_info.info.svc_subscribe.i_svc_ids = svc_id;
	if (NCSCC_RC_SUCCESS != ncsmds_api(&svc_info)){
		LOG_ER("SMFA: MDS Subscription FAILED.");
		smfa_mds_unregister();
		return NCSCC_RC_FAILURE;
	}
	
	return NCSCC_RC_SUCCESS;	 
}

/*************************************************************************** 
@brief		: Unregister with MDS.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*****************************************************************************/
uint32_t smfa_mds_unregister()
{
	NCSMDS_INFO svc_info;
	SMFA_CB *cb = &_smfa_cb;

	memset(&svc_info, 0, sizeof(NCSMDS_INFO));
	svc_info.i_mds_hdl = cb->smfa_mds_hdl;
	svc_info.i_svc_id = NCSMDS_SVC_ID_SMFA;
	svc_info.i_op = MDS_UNINSTALL;
	if (NCSCC_RC_SUCCESS != ncsmds_api(&svc_info)){
		LOG_ER("SMFA: MDS Unregistration FAILED.");
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/*************************************************************************** 
@brief		: Async send from SMFA to SMFND.
@param[in]	: evt - evt to be sent.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*****************************************************************************/
uint32_t smfa_to_smfnd_mds_async_send(NCSCONTEXT evt)
{
	NCSMDS_INFO   info;
	SMFA_CB *cb = &_smfa_cb;

	memset(&info, 0, sizeof(info));
	info.i_mds_hdl = cb->smfa_mds_hdl;
	info.i_op = MDS_SEND;
	info.i_svc_id = NCSMDS_SVC_ID_SMFA;

	info.info.svc_send.i_msg = evt;
	info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
	info.info.svc_send.i_to_svc   = NCSMDS_SVC_ID_SMFND;
	
	info.info.svc_send.info.snd.i_to_dest = cb->smfnd_adest;

	return ncsmds_api(&info);
}

/*************************************************************************** 
@brief		: MDS callback. 
@param[in]	: info - MDS callback info.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*****************************************************************************/
uint32_t smfa_mds_callback(struct ncsmds_callback_info *info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	SMFSV_EVT *evt;
	NCS_UBAID *uba;
	
	switch(info->i_op){
		case MDS_CALLBACK_COPY:
			break;
		case MDS_CALLBACK_ENC_FLAT:
		case MDS_CALLBACK_ENC:
			evt = (SMFSV_EVT *) info->info.enc.i_msg;
			uba = info->info.enc.io_uba;
			rc = smfsv_evt_enc(evt, uba);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("SMFA: Encoding failed");
			}
			break;
		case MDS_CALLBACK_DEC_FLAT:
		case MDS_CALLBACK_DEC:
			uba = info->info.dec.io_uba;
			if (NULL == (evt = calloc(1, sizeof(SMFSV_EVT)))) {
				LOG_ER("SMFA: Decode Calloc failed");
				osafassert(0);
			}
			info->info.dec.o_msg = (uint8_t *) evt;
			rc = smfsv_evt_dec(uba,evt);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("SMFA: Decoding failed");
			}
			break;
		case MDS_CALLBACK_RECEIVE:
			rc = smfa_mds_rcv_cbk(&info->info.receive);
			break;
		case MDS_CALLBACK_SVC_EVENT:
			rc = smfa_mds_svc_evt_cbk(&info->info.svc_evt);
			break;
		default:
			rc = NCSCC_RC_FAILURE;
			LOG_ER("SMFA: Received mds cbk for: %d",info->i_op);
	}
	return rc;
}

/*************************************************************************** 
@brief		: MDS svc evt callback. In this context called for SMFND 
		  UP/DOWN events. 
@param[in]	: svc_evt - MDS svc event callback info.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*****************************************************************************/
uint32_t smfa_mds_svc_evt_cbk(MDS_CALLBACK_SVC_EVENT_INFO *svc_evt)
{
	SMFA_CB *cb = &_smfa_cb;

	/* Agent subscribes for only local ND UP/DOWN.*/
	if (NCSMDS_SVC_ID_SMFND != svc_evt->i_svc_id){
		LOG_NO("SMFA: Got svc evt for: %d",svc_evt->i_svc_id);
		return NCSCC_RC_SUCCESS;
	}

	switch(svc_evt->i_change){
		case NCSMDS_UP:
			/* Catch the adest of SMFND*/
			if (!m_MDS_DEST_IS_AN_ADEST(svc_evt->i_dest))
				return NCSCC_RC_SUCCESS;
			/* TODO: No lock is taken. This might be dangerous.*/
			cb->is_smfnd_up = true;
			cb->smfnd_adest = svc_evt->i_dest;
			break;

		case NCSMDS_DOWN:
			if (!m_MDS_DEST_IS_AN_ADEST(svc_evt->i_dest))
				return NCSCC_RC_SUCCESS;

			/* TODO: No lock is taken. This might be dangerous.*/
			cb->is_smfnd_up = false;
			cb->smfnd_adest = 0;
			break;
		default:
			LOG_NO("SMFA: Got the svc evt: %d for SMFND",svc_evt->i_change);
	}

	return NCSCC_RC_SUCCESS;
}

/*************************************************************************** 
@brief		: MDS rcv callback. In this context called if any MDS msg is 
		  received by SMFA.
@param[in]	: rcv_evt - MDS rcv callback info.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*****************************************************************************/
uint32_t smfa_mds_rcv_cbk(MDS_CALLBACK_RECEIVE_INFO *rcv_evt)
{
	SMFSV_EVT *smfsv_evt = rcv_evt->i_msg;
	SMF_EVT *evt = &smfsv_evt->info.smfa.event.cbk_req_rsp;
	SMFA_CLIENT_INFO *client_info = NULL;
	SMFA_CB *cb = &_smfa_cb;
	uint32_t filter_match = false;
	SMFSV_EVT resp_evt;

	if (NULL == evt){
		LOG_NO("SMFA: evt is NULL in rcv_cbk");
		return NCSCC_RC_SUCCESS;
	}

	if (SMF_CLBK_EVT != evt->evt_type){
		LOG_NO("SMFA: evt_type is : %d",evt->evt_type);
		return NCSCC_RC_SUCCESS;
	}

	/* TODO: I need to take READ LOCK here. But in MDS thread ???*/
	client_info = cb->smfa_client_info_list;
	while (NULL != client_info){
		/* If filter matches, post the evt to the corresponding MBX.*/
		if (NCSCC_RC_SUCCESS == smfa_cbk_filter_match(client_info,&evt->evt.cbk_evt))
			filter_match = true;
		client_info = client_info->next_client;
	}

	/* If filters dont match, respond to ND as SA_AIS_OK*/
	if (false == filter_match){
		memset(&resp_evt,0,sizeof(SMFSV_EVT));
		resp_evt.type = SMFSV_EVT_TYPE_SMFND;
		resp_evt.info.smfnd.type = SMFND_EVT_CBK_RSP;
		resp_evt.info.smfnd.event.cbk_req_rsp.evt_type = SMF_RSP_EVT;
		resp_evt.info.smfnd.event.cbk_req_rsp.evt.resp_evt.inv_id = evt->evt.cbk_evt.inv_id;
		resp_evt.info.smfnd.event.cbk_req_rsp.evt.resp_evt.err = SA_AIS_OK;

		if (NCSCC_RC_SUCCESS != smfa_to_smfnd_mds_async_send((NCSCONTEXT)&resp_evt)){
			LOG_ER("SMFA: MDS send to SMFND FAILED.");
			/* TODO: What should I do?? ND is waiting for my response.*/
		}
	}

	/* This is not the same evt we are putting in client MBX, so free it.*/
	free(evt->evt.cbk_evt.params);
	free(evt->evt.cbk_evt.cbk_label.label);
	free(smfsv_evt);

	return NCSCC_RC_SUCCESS;
}
