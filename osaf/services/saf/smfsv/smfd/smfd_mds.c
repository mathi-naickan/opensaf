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

#include "smfd.h"
#include "smfd_smfnd.h"

uint32_t mds_register(smfd_cb_t * cb);
void mds_unregister(smfd_cb_t * cb);

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

static uint32_t mds_cpy(struct ncsmds_callback_info *info)
{
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

static uint32_t mds_enc(struct ncsmds_callback_info *info)
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
		TRACE("encoding event %d failed", evt->type);
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

static uint32_t mds_dec(struct ncsmds_callback_info *info)
{
	SMFSV_EVT *evt;
	NCS_UBAID *uba = info->info.dec.io_uba;

    /** allocate an SMFSV_EVENT now **/
	if (NULL == (evt = calloc(1, sizeof(SMFSV_EVT)))) {
		LOG_WA("calloc FAILED");
		goto err;
	}

	/* Assign the allocated event */
	info->info.dec.o_msg = (uint8_t *) evt;

	if (smfsv_evt_dec(uba, evt) != NCSCC_RC_SUCCESS) {
		TRACE("decoding event %d failed", evt->type);
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

static uint32_t mds_enc_flat(struct ncsmds_callback_info *info)
{
	uint32_t rc;

	/* Retrieve info from the enc_flat */
	MDS_CALLBACK_ENC_INFO enc = info->info.enc_flat;
	/* Modify the MDS_INFO to populate enc */
	info->info.enc = enc;
	/* Invoke the regular mds_enc routine */
	rc = mds_enc(info);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("mds_enc FAILED");
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

static uint32_t mds_dec_flat(struct ncsmds_callback_info *info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	/* Retrieve info from the dec_flat */
	MDS_CALLBACK_DEC_INFO dec = info->info.dec_flat;
	/* Modify the MDS_INFO to populate dec */
	info->info.dec = dec;
	/* Invoke the regular mds_dec routine */
	rc = mds_dec(info);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("mds_dec FAILED ");
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

static uint32_t mds_rcv(struct ncsmds_callback_info *mds_info)
{
	SMFSV_EVT *smfsv_evt = (SMFSV_EVT *) mds_info->info.receive.i_msg;
	uint32_t rc = NCSCC_RC_SUCCESS;

	smfsv_evt->cb_hdl = (uint32_t) mds_info->i_yr_svc_hdl;
	smfsv_evt->fr_node_id = mds_info->info.receive.i_node_id;
	smfsv_evt->fr_dest = mds_info->info.receive.i_fr_dest;
	smfsv_evt->fr_svc = mds_info->info.receive.i_fr_svc_id;
	smfsv_evt->rcvd_prio = mds_info->info.receive.i_priority;
	smfsv_evt->mds_ctxt = mds_info->info.receive.i_msg_ctxt;

	/* Send the event to our mailbox */
	rc = m_NCS_IPC_SEND(&smfd_cb->mbx, smfsv_evt,
			    mds_info->info.receive.i_priority);
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

static uint32_t mds_quiesced_ack(struct ncsmds_callback_info *mds_info)
{
        SMFSV_EVT *smfsv_evt;
	if (smfd_cb->is_quiesced_set == true) {
                /** allocate an SMFSV_EVT **/
                if (NULL == (smfsv_evt = calloc(1, sizeof(SMFSV_EVT)))) {
                        LOG_NO("calloc FAILED");
                        goto err;
                }
                /** Initialize the Event **/
		smfsv_evt->type = SMFSV_EVT_TYPE_SMFD;
		smfsv_evt->info.smfd.type = SMFD_EVT_QUIESCED_ACK;
		smfsv_evt->cb_hdl = (uint32_t)mds_info->i_yr_svc_hdl;

                /* Push the event and we are done */
		if (NCSCC_RC_FAILURE == m_NCS_IPC_SEND(&smfd_cb->mbx, smfsv_evt, NCS_IPC_PRIORITY_VERY_HIGH)) {
			TRACE("ipc send failed");
			smfsv_evt_destroy(smfsv_evt);
			goto err;
		}
	}

	return NCSCC_RC_SUCCESS;

err:
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

static uint32_t mds_svc_event(struct ncsmds_callback_info *info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	SMFSV_EVT *evt = NULL;
	MDS_CALLBACK_SVC_EVENT_INFO *svc_evt = &info->info.svc_evt;

	/* First make sure that this event is indeed for us */
	if (info->info.svc_evt.i_your_id != NCSMDS_SVC_ID_SMFD) {
		TRACE("event not NCSMDS_SVC_ID_SMFD");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* If this evt was sent from SMFND act on this */
	if (info->info.svc_evt.i_svc_id == NCSMDS_SVC_ID_SMFND) {
	/** allocate an SMFSV_EVENT **/
		if (NULL == (evt = calloc(1, sizeof(SMFSV_EVT)))) {
			LOG_ER("calloc FAILED");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

		/* Send the SMFD_EVT_MDS_INFO to the mailbox */
		evt->type = SMFSV_EVT_TYPE_SMFD;
		evt->info.smfd.type = SMFD_EVT_MDS_INFO;
		evt->info.smfd.event.mds_info.change = svc_evt->i_change;
		evt->info.smfd.event.mds_info.dest = svc_evt->i_dest;
		evt->info.smfd.event.mds_info.svc_id = svc_evt->i_svc_id;
		evt->info.smfd.event.mds_info.node_id = svc_evt->i_node_id;
		evt->info.smfd.event.mds_info.rem_svc_pvt_ver = svc_evt->i_rem_svc_pvt_ver;

		TRACE("SMFND SVC event %d for nodeid %x, svc version %u", svc_evt->i_change,
		      svc_evt->i_node_id, svc_evt->i_rem_svc_pvt_ver);

		/* Put it in SMFD's Event Queue */
		rc = m_NCS_IPC_SEND(&smfd_cb->mbx, (NCSCONTEXT) evt,
				    NCS_IPC_PRIORITY_HIGH);
		if (NCSCC_RC_SUCCESS != rc) {
			free(evt);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
	}

 done:
	return rc;
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

static uint32_t mds_sys_event(struct ncsmds_callback_info *mds_info)
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

static uint32_t mds_callback(struct ncsmds_callback_info *info)
{
	static NCSMDS_CALLBACK_API cb_set[MDS_CALLBACK_SVC_MAX] = {
		mds_cpy,	/* MDS_CALLBACK_COPY      */
		mds_enc,	/* MDS_CALLBACK_ENC       */
		mds_dec,	/* MDS_CALLBACK_DEC       */
		mds_enc_flat,	/* MDS_CALLBACK_ENC_FLAT  */
		mds_dec_flat,	/* MDS_CALLBACK_DEC_FLAT  */
		mds_rcv,	/* MDS_CALLBACK_RECEIVE   */
		mds_svc_event,	/* MDS_CALLBACK_SVC_EVENT */
		mds_sys_event,	/* MDS_CALLBACK_SYS_EVENT */
		mds_quiesced_ack	/* MDS_CALLBACK_QUIESCED_ACK */
	};

	if (info->i_op <= MDS_CALLBACK_QUIESCED_ACK) {
		return (*cb_set[info->i_op]) (info);
	}

	LOG_ER("mds callback out of range %u", info->i_op);

	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : mds_vdest_create
 *
 * Description   : This function created the VDEST for SMFD
 *
 * Arguments     : 
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mds_vdest_create(smfd_cb_t * cb)
{
	NCSVDA_INFO arg;
	uint32_t rc = NCSCC_RC_SUCCESS;

	memset(&arg, 0, sizeof(arg));

	cb->mds_dest = SMFD_VDEST_ID;

	arg.req = NCSVDA_VDEST_CREATE;
	arg.info.vdest_create.i_persistent = false;
	arg.info.vdest_create.i_policy = NCS_VDEST_TYPE_DEFAULT;
	arg.info.vdest_create.i_create_type = NCSVDA_VDEST_CREATE_SPECIFIC;
	arg.info.vdest_create.info.specified.i_vdest = cb->mds_dest;

	/* Create VDEST */
	rc = ncsvda_api(&arg);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("mds_vdest_create: create vdest for SMFD FAILED\n");
		return rc;
	}

	cb->mds_handle = arg.info.vdest_create.o_mds_pwe1_hdl;
	return rc;
}

/****************************************************************************
  Name          : mds_register
 
  Description   : This routine registers the SMFD Service with MDS.
 
  Arguments     : mqa_cb - ptr to the SMFD control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

uint32_t mds_register(smfd_cb_t * cb)
{
	NCSMDS_INFO svc_info;
	MDS_SVC_ID smfnd_id[1] = { NCSMDS_SVC_ID_SMFND };
	MDS_SVC_ID smfd_id[1] = { NCSMDS_SVC_ID_SMFD };

	/* memset the svc_info */
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));

	/* Install with MDS our service ID NCSMDS_SVC_ID_SMFD. */
	svc_info.i_mds_hdl = cb->mds_handle;
	svc_info.i_svc_id = NCSMDS_SVC_ID_SMFD;
	svc_info.i_op = MDS_INSTALL;

	svc_info.info.svc_install.i_yr_svc_hdl = 0;
	svc_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;	/* node specific */
	svc_info.info.svc_install.i_svc_cb = mds_callback;	/* callback */
	svc_info.info.svc_install.i_mds_q_ownership = false;
	svc_info.info.svc_install.i_mds_svc_pvt_ver =
	    SMFD_MDS_PVT_SUBPART_VERSION;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_ER("smfd_mds_register: mds install SMFD FAILED\n");
		return NCSCC_RC_FAILURE;
	}

	/* Subscribe to SMFD for redundancy events, needed ?? */
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));
	svc_info.i_mds_hdl = cb->mds_handle;
	svc_info.i_svc_id = NCSMDS_SVC_ID_SMFD;
	svc_info.i_op = MDS_RED_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_info.info.svc_subscribe.i_svc_ids = smfd_id;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_ER
		    ("smfd_mds_register: MDS Subscribe for redundancy Failed");
		mds_unregister(cb);
		return NCSCC_RC_FAILURE;
	}

	/* Subscribe to SMFND up/down events */
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));
	svc_info.i_mds_hdl = cb->mds_handle;
	svc_info.i_svc_id = NCSMDS_SVC_ID_SMFD;
	svc_info.i_op = MDS_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_info.info.svc_subscribe.i_svc_ids = smfnd_id;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_ER("smfd_mds_register: mds subscribe SMFD FAILED\n");
		mds_unregister(cb);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : mds_unregister
 *
 * Description   : This function un-registers the SMFD Service with MDS.
 *
 * Arguments     : cb   : SMFD control Block pointer.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/
void mds_unregister(smfd_cb_t * cb)
{
	NCSMDS_INFO arg;

	/* Uninstall our service from MDS. 
	   No need to cancel the services that are subscribed */

	memset(&arg, 0, sizeof(NCSMDS_INFO));

	arg.i_mds_hdl = cb->mds_handle;
	arg.i_svc_id = NCSMDS_SVC_ID_SMFD;
	arg.i_op = MDS_UNINSTALL;

	if (ncsmds_api(&arg) != NCSCC_RC_SUCCESS) {
		LOG_ER("smfd_mds_unregister: mds uninstall FAILED\n");
	}
	return;
}

/****************************************************************************
 * Name          : smfd_mds_init
 *
 * Description   : This function .
 *
 * Arguments     : cb   : SMFD control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t smfd_mds_init(smfd_cb_t * cb)
{
	uint32_t rc;

	TRACE_ENTER();

	/* Create the VDEST for SMFD */
	rc = mds_vdest_create(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER(" smfd_mds_init: named vdest create FAILED\n");
		goto done;
	}

	/* Register MDS communication */
	rc = mds_register(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER(" smfd_mds_init: mds register FAILED\n");
		goto done;
	}

	/* Set the role of MDS */
	if (cb->ha_state == SA_AMF_HA_ACTIVE)
		cb->mds_role = V_DEST_RL_ACTIVE;
	else
		cb->mds_role = V_DEST_RL_STANDBY;

	rc = smfd_mds_change_role(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("MDS role change to %d FAILED\n", cb->mds_role);
		goto done;
	}

 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : smfd_mds_change_role
 *
 * Description   : This function informs mds of our vdest role change 
 *
 * Arguments     : cb   : SMFD control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t smfd_mds_change_role(smfd_cb_t * cb)
{
	NCSVDA_INFO arg;

	memset(&arg, 0, sizeof(NCSVDA_INFO));

	arg.req = NCSVDA_VDEST_CHG_ROLE;
	arg.info.vdest_chg_role.i_vdest = cb->mds_dest;
	arg.info.vdest_chg_role.i_new_role = cb->mds_role;

	return ncsvda_api(&arg);
}

/****************************************************************************
 * Name          : mds_vdest_destroy
 *
 * Description   : This function Destroys the Virtual destination of LGS
 *
 * Arguments     : cb   : SMFD control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mds_vdest_destroy(smfd_cb_t * cb)
{
	NCSVDA_INFO vda_info;
	uint32_t rc;

	memset(&vda_info, 0, sizeof(NCSVDA_INFO));
	vda_info.req = NCSVDA_VDEST_DESTROY;
	vda_info.info.vdest_destroy.i_vdest = cb->mds_dest;

	if (NCSCC_RC_SUCCESS != (rc = ncsvda_api(&vda_info))) {
		LOG_ER("NCSVDA_VDEST_DESTROY failed");
		return rc;
	}

	return rc;
}

/****************************************************************************
 * Name          : smfd_mds_finalize
 *
 * Description   : This function un-registers the SMFD Service with MDS.
 *
 * Arguments     : Uninstall SMFD from MDS.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t smfd_mds_finalize(smfd_cb_t * cb)
{
	uint32_t rc;

	/* Destroy the virtual Destination of SMFD */
	rc = mds_vdest_destroy(cb);
	return rc;
}
