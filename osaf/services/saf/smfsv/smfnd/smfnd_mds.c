/*      OpenSAF
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
 * Author(s): Ericsson AB
 *
 */

#include <logtrace.h>

#include "smfnd.h"
#include "smfsv_evt.h"
#include "smfnd_evt.h"

uns32 mds_register(smfnd_cb_t * cb);
void mds_unregister(smfnd_cb_t * cb);

/****************************************************************************
 * Name          : mds_cpy
 *
 * Description   : MDS copy.
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 mds_cpy(struct ncsmds_callback_info *info)
{
	/* TODO; */
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : mds_enc
 *
 * Description   : MDS encode.
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 mds_enc(struct ncsmds_callback_info *info)
{
	SMFSV_EVT *evt;
	NCS_UBAID *uba;

	evt = (SMFSV_EVT *) info->info.enc.i_msg;
	uba = info->info.enc.io_uba;

	if (uba == NULL) {
		LOG_ER("uba == NULL");
		goto err;
	}

	if (smfsv_evt_enc(evt, uba) != NCSCC_RC_SUCCESS) {
		LOG_ER("encoding event %d failed", evt->type);
		goto err;
	}

	return NCSCC_RC_SUCCESS;

 err:
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : mds_dec
 *
 * Description   : MDS decode
 *
 * Arguments     : pointer to ncsmds_callback_info 
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 mds_dec(struct ncsmds_callback_info *info)
{
	SMFSV_EVT *evt;
	NCS_UBAID *uba = info->info.dec.io_uba;

    /** allocate an SMFSV_EVENT now **/
	if (NULL == (evt = calloc(1, sizeof(SMFSV_EVT)))) {
		LOG_ER("calloc FAILED");
		goto err;
	}

	/* Assign the allocated event */
	info->info.dec.o_msg = (uns8 *) evt;

	if (smfsv_evt_dec(uba, evt) != NCSCC_RC_SUCCESS) {
		LOG_ER("decoding event %d failed", evt->type);
		goto err;
	}

	return NCSCC_RC_SUCCESS;

 err:
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : mds_enc_flat
 *
 * Description   : MDS encode and flatten
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 mds_enc_flat(struct ncsmds_callback_info *info)
{
	uns32 rc;

	/* Retrieve info from the enc_flat */
	MDS_CALLBACK_ENC_INFO enc = info->info.enc_flat;
	/* Modify the MDS_INFO to populate enc */
	info->info.enc = enc;
	/* Invoke the regular mds_enc routine */
	rc = mds_enc(info);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("mds_enc FAILED");
	}
	return rc;
}

/****************************************************************************
 * Name          : mds_dec_flat
 *
 * Description   : MDS decode and flatten
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 mds_dec_flat(struct ncsmds_callback_info *info)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	/* Retrieve info from the dec_flat */
	MDS_CALLBACK_DEC_INFO dec = info->info.dec_flat;
	/* Modify the MDS_INFO to populate dec */
	info->info.dec = dec;
	/* Invoke the regular mds_dec routine */
	rc = mds_dec(info);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("mds_dec FAILED ");
	}
	return rc;
}

/****************************************************************************
 * Name          : mds_rcv
 *
 * Description   : MDS rcv evt.
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 mds_rcv(struct ncsmds_callback_info *mds_info)
{
	SMFSV_EVT *smfsv_evt = (SMFSV_EVT *) mds_info->info.receive.i_msg;
	uns32 rc = NCSCC_RC_SUCCESS;

	smfsv_evt->cb_hdl = (uns32) mds_info->i_yr_svc_hdl;
	smfsv_evt->fr_node_id = mds_info->info.receive.i_node_id;
	smfsv_evt->fr_dest = mds_info->info.receive.i_fr_dest;
	smfsv_evt->fr_svc = mds_info->info.receive.i_fr_svc_id;
	smfsv_evt->rcvd_prio = mds_info->info.receive.i_priority;
	smfsv_evt->mds_ctxt = mds_info->info.receive.i_msg_ctxt;

	/* Send the event to our mailbox */
	rc = m_NCS_IPC_SEND(&smfnd_cb->mbx, smfsv_evt, mds_info->info.receive.i_priority);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("IPC send failed %d", rc);
	}

	return rc;
}

/****************************************************************************
 * Name          : mds_quiesced_ack
 *
 * Description   : MDS quised ack.
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 mds_quiesced_ack(struct ncsmds_callback_info *mds_info)
{
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : mds_svc_event
 *
 * Description   : MDS subscription evt.
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 mds_svc_event(struct ncsmds_callback_info *info)
{
	smfnd_cb_t *cb = smfnd_cb;
	MDS_CALLBACK_SVC_EVENT_INFO *svc_evt = &info->info.svc_evt; 
	
	switch(svc_evt->i_change){
		case NCSMDS_UP:
			/* TODO: No lock is taken. This might be dangerous.*/
			if (NCSMDS_SVC_ID_SMFA == svc_evt->i_svc_id){
				cb->agent_cnt++;
				LOG_NO("Count of agents incremeted to : %d",cb->agent_cnt);
			}else if (NCSMDS_SVC_ID_SMFD == svc_evt->i_svc_id){
				/* Catch the vdest of SMFD*/
				if (m_MDS_DEST_IS_AN_ADEST(svc_evt->i_dest))
					return NCSCC_RC_SUCCESS;
				cb->smfd_dest = svc_evt->i_dest;
			}
			break;

		case NCSMDS_DOWN:
			/* TODO: No lock is taken. This might be dangerous.*/
			/* TODO: Need to clean up the cb->cbk_list, otherwise there will be
			 memory leak. For the time being we are storing only conut of agents,
			 not the adest of agents and hence it is not possible to clean up cbk_list.*/
			if (NCSMDS_SVC_ID_SMFA == svc_evt->i_svc_id){
				cb->agent_cnt--;
				LOG_NO("Count of agents decremeted to : %d",cb->agent_cnt);
			}else if (NCSMDS_SVC_ID_SMFD == svc_evt->i_svc_id){
				if (m_MDS_DEST_IS_AN_ADEST(svc_evt->i_dest))
					return NCSCC_RC_SUCCESS;
				cb->smfd_dest = 0;
			}
			break;
		default:
			TRACE("Got the svc evt: %d for SMFND",svc_evt->i_change);
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : mds_sys_evt
 *
 * Description   : MDS sys evt .
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 mds_sys_event(struct ncsmds_callback_info *mds_info)
{
	/* Not supported now */
	TRACE("FAILED");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : mds_callback
 *
 * Description   : Callback Dispatcher for various MDS operations.
 *
 * Arguments     : info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 mds_callback(struct ncsmds_callback_info *info)
{
	static NCSMDS_CALLBACK_API cb_set[MDS_CALLBACK_SVC_MAX] = {
		mds_cpy,	 /* MDS_CALLBACK_COPY         */
		mds_enc,	 /* MDS_CALLBACK_ENC          */
		mds_dec,	 /* MDS_CALLBACK_DEC          */
		mds_enc_flat, 	 /* MDS_CALLBACK_ENC_FLAT     */
		mds_dec_flat,	 /* MDS_CALLBACK_DEC_FLAT     */
		mds_rcv,	 /* MDS_CALLBACK_RECEIVE      */
		mds_svc_event,	 /* MDS_CALLBACK_SVC_EVENT    */
		mds_sys_event,	 /* MDS_CALLBACK_SYS_EVENT    */
		mds_quiesced_ack /* MDS_CALLBACK_QUIESCED_ACK */
	};

	if (info->i_op > MDS_CALLBACK_QUIESCED_ACK) {
		LOG_ER("mds callback out of range %u", info->i_op);
		return NCSCC_RC_FAILURE;
	}

	return (*cb_set[info->i_op]) (info);
}

/****************************************************************************
 * Name          : mds_get_handle
 *
 * Description   : This function Gets the Handles of local MDS destination
 *
 * Arguments     : cb   : SMFND control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

uns32 mds_get_handle(smfnd_cb_t * cb)
{
	NCSADA_INFO arg;
	uns32 rc;

	memset(&arg, 0, sizeof(NCSADA_INFO));
	arg.req = NCSADA_GET_HDLS;
	rc = ncsada_api(&arg);

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("mds_get_handle: get handle FAILED\n");
		return rc;
	}
	cb->mds_handle = arg.info.adest_get_hdls.o_mds_pwe1_hdl;
	return rc;
}

/****************************************************************************
  Name          : mds_register
 
  Description   : This routine registers the SMFND Service with MDS.
 
  Arguments     : smfnd_cb - ptr to the SMFND control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

uns32 mds_register(smfnd_cb_t * cb)
{
	NCSMDS_INFO svc_info;
	MDS_SVC_ID svc_id[2] = { NCSMDS_SVC_ID_SMFD,NCSMDS_SVC_ID_SMFA };

	/* clear the svc_info */
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));

	/* Install on ADEST with MDS with service ID NCSMDS_SVC_ID_SMFND. */
	svc_info.i_mds_hdl = cb->mds_handle;
	svc_info.i_svc_id = NCSMDS_SVC_ID_SMFND;
	svc_info.i_op = MDS_INSTALL;

	svc_info.info.svc_install.i_yr_svc_hdl = 0;
	svc_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;	/* node specific */
	svc_info.info.svc_install.i_svc_cb = mds_callback;	/* callback */
	svc_info.info.svc_install.i_mds_q_ownership = FALSE;	/* SMFND owns the mds queue */
	svc_info.info.svc_install.i_mds_svc_pvt_ver = SMFND_MDS_PVT_SUBPART_VERSION;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_ER("mds_register: install adest FAILED\n");
		return NCSCC_RC_FAILURE;
	}
	cb->mds_dest = svc_info.info.svc_install.o_dest;
	
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));
	svc_info.i_mds_hdl = cb->mds_handle;
	svc_info.i_svc_id = NCSMDS_SVC_ID_SMFND;
	svc_info.i_op = MDS_SUBSCRIBE;

	svc_info.info.svc_subscribe.i_num_svcs = 2;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_info.info.svc_subscribe.i_svc_ids = svc_id;
	if (NCSCC_RC_SUCCESS != ncsmds_api(&svc_info)){
		LOG_ER("SMFND: MDS Subscription FAILED.");
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : mds_unregister
 *
 * Description   : This function un-registers the SMFND Service with MDS.
 *
 * Arguments     : cb   : SMFND control Block pointer.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/
void mds_unregister(smfnd_cb_t * cb)
{
	NCSMDS_INFO arg;

	/* Uninstall the service from MDS. */
	memset(&arg, 0, sizeof(NCSMDS_INFO));

	arg.i_mds_hdl = cb->mds_handle;
	arg.i_svc_id = NCSMDS_SVC_ID_SMFND;
	arg.i_op = MDS_UNINSTALL;

	if (ncsmds_api(&arg) != NCSCC_RC_SUCCESS) {
		LOG_ER("mds_unregister: uninstall adest FAILED\n");
	}
	return;
}

/****************************************************************************
 * Name          : smfnd_mds_init
 *
 * Description   : This function .
 *
 * Arguments     : cb   : SMFND control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 smfnd_mds_init(smfnd_cb_t * cb)
{
	uns32 rc;

	TRACE_ENTER();

	rc = mds_get_handle(cb);

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("smfnd_mds_init: get mds handle FAILED\n");
		goto done;
	}

	rc = mds_register(cb);

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("smfnd_mds_init: mds register FAILED\n");
		goto done;
	}

 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : smfnd_mds_finalize
 *
 * Description   : This function un-registers the SMFND Service with MDS.
 *
 * Arguments     : Uninstall SMFND from MDS.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 smfnd_mds_finalize(smfnd_cb_t * cb)
{
	/* Destroy the Destination of SMFND */
	mds_unregister(cb);
	return NCSCC_RC_SUCCESS;
}
