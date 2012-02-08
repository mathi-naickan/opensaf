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

  This file contains routines used by AvA library for MDS interaction.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

******************************************************************************
*/

#include "ava.h"

/* static function declarations */

static uint32_t ava_mds_rcv(AVA_CB *, MDS_CALLBACK_RECEIVE_INFO *);

static uint32_t ava_mds_svc_evt(AVA_CB *, MDS_CALLBACK_SVC_EVENT_INFO *);

static uint32_t ava_mds_flat_enc(AVA_CB *, MDS_CALLBACK_ENC_FLAT_INFO *);

static uint32_t ava_mds_flat_dec(AVA_CB *, MDS_CALLBACK_DEC_FLAT_INFO *);

static uint32_t ava_mds_param_get(AVA_CB *);

static uint32_t ava_mds_msg_async_send(AVA_CB *, NCSMDS_INFO *, AVSV_NDA_AVA_MSG *);

static uint32_t ava_mds_msg_syn_send(AVA_CB *, NCSMDS_INFO *, AVSV_NDA_AVA_MSG *);

static uint32_t ava_mds_enc(AVA_CB *cb, MDS_CALLBACK_ENC_INFO *enc_info);

static uint32_t ava_mds_dec(AVA_CB *cb, MDS_CALLBACK_DEC_INFO *dec_info);

static const MDS_CLIENT_MSG_FORMAT_VER ava_avnd_msg_fmt_map_table[AVA_AVND_SUBPART_VER_MAX] =
    { AVSV_AVND_AVA_MSG_FMT_VER_1 };

/**
 * function called when MDS down for avnd (AMF) is received
 * Can be used by a client to reboot the system.
 */
static void (*amf_down_cb) (void);

/****************************************************************************
  Name          : ava_mds_reg
 
  Description   : This routine registers the AVA service with MDS. It does 
                  the following:
                  a) Gets the MDS handle & AvA MDS address
                  b) installs AvA service with MDS
                  c) Subscribes to MDS events
 
  Arguments     : cb - ptr to the AvA control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t ava_mds_reg(AVA_CB *cb)
{
	NCSMDS_INFO mds_info;
	MDS_SVC_ID svc_id;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();	

	/* get the mds-hdl & ava mds address */
	rc = ava_mds_param_get(cb);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("Unable to get MDS handle");
		return NCSCC_RC_FAILURE;
	}

	/* fill common fields */
	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_AVA;

   /*** install ava service with mds ***/
	mds_info.i_op = MDS_INSTALL;
	mds_info.info.svc_install.i_mds_q_ownership = false;
	mds_info.info.svc_install.i_svc_cb = ava_mds_cbk;
	mds_info.info.svc_install.i_yr_svc_hdl = (MDS_CLIENT_HDL)cb->cb_hdl;
	mds_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_INTRANODE;
	mds_info.info.svc_install.i_mds_svc_pvt_ver = AVA_MDS_SUB_PART_VERSION;

	rc = ncsmds_api(&mds_info);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("MDS Install failed");
		goto done;
	}

   /*** subscribe to avnd mds event ***/
	mds_info.i_op = MDS_SUBSCRIBE;
	mds_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_INTRANODE;
	mds_info.info.svc_subscribe.i_svc_ids = &svc_id;
	mds_info.info.svc_subscribe.i_num_svcs = 1;
	svc_id = NCSMDS_SVC_ID_AVND;
	rc = ncsmds_api(&mds_info);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("MDS Subscription failed");
		goto done;
	}

 done:
	if (NCSCC_RC_SUCCESS != rc)
		rc = ava_mds_unreg(cb);

	TRACE_LEAVE2("retval = %u", rc);
	return rc;
}

/****************************************************************************
  Name          : ava_mds_unreg
 
  Description   : This routine unregisters the AVA service from MDS.
 
  Arguments     : cb - ptr to the AvA control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t ava_mds_unreg(AVA_CB *cb)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();	

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_AVA;
	mds_info.i_op = MDS_UNINSTALL;

	rc = ncsmds_api(&mds_info);

	TRACE_LEAVE2("retval = %u",rc);
	return rc;
}

/****************************************************************************
  Name          : ava_mds_cbk
 
  Description   : This routine is a callback routine that is provided to MDS.
                  MDS calls this routine for encode, decode, copy, receive &
                  service event notification operations.
 
  Arguments     : info - ptr to the MDS callback info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t ava_mds_cbk(NCSMDS_CALLBACK_INFO *info)
{
	AVA_CB *cb = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();	

	if (!info)
		goto done;

	/* retrieve ava cb */
	if (0 == (cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, (uint32_t)info->i_yr_svc_hdl))) {
		LOG_ER("Unable to retrieve control block handle");
		rc = NCSCC_RC_SUCCESS;
		goto done;
	}

	switch (info->i_op) {
	case MDS_CALLBACK_RECEIVE:
		{
			/* acquire cb write lock */
			m_NCS_LOCK(&cb->lock, NCS_LOCK_WRITE);

			/* process the received msg */
			rc = ava_mds_rcv(cb, &info->info.receive);

			/* release cb write lock */
			m_NCS_UNLOCK(&cb->lock, NCS_LOCK_WRITE);

			if (NCSCC_RC_SUCCESS != rc)
				LOG_ER("MDS receive callback failed");
			else
				TRACE("MDS receive callback success");
		}
		break;

	case MDS_CALLBACK_COPY:
		osafassert(0);	/* AvA never resides with AvND */

	case MDS_CALLBACK_SVC_EVENT:
		{
			rc = ava_mds_svc_evt(cb, &info->info.svc_evt);

			if (NCSCC_RC_SUCCESS != rc)
				LOG_ER("MDS service event callback failed");
			else
				TRACE("MDS service event callback success");
		}
		break;

	case MDS_CALLBACK_ENC_FLAT:
		{
			info->info.enc.o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->info.enc_flat.i_rem_svc_pvt_ver,
									     AVA_AVND_SUBPART_VER_MIN,
									     AVA_AVND_SUBPART_VER_MAX,
									     ava_avnd_msg_fmt_map_table);

			if (info->info.enc_flat.o_msg_fmt_ver < AVSV_AVND_AVA_MSG_FMT_VER_1) {
				/* log the problem */
				LOG_ER("MDS flat encode callback failed: Incorrect message format version");
				return NCSCC_RC_FAILURE;
			}

			rc = ava_mds_flat_enc(cb, &info->info.enc_flat);

			if (NCSCC_RC_SUCCESS != rc)
				LOG_ER("MDS flat encode callback failed");
			else
				TRACE("MDS flat encode callback success");
		}
		break;

	case MDS_CALLBACK_DEC_FLAT:
		{
			if (!m_NCS_MSG_FORMAT_IS_VALID(info->info.dec_flat.i_msg_fmt_ver,
						       AVA_AVND_SUBPART_VER_MIN,
						       AVA_AVND_SUBPART_VER_MAX, ava_avnd_msg_fmt_map_table)) {
				/* log the problem */
				LOG_ER("MDS flat decode callback failed: Incorrect message format version");
				return NCSCC_RC_FAILURE;
			}

			rc = ava_mds_flat_dec(cb, &info->info.dec_flat);

			if (NCSCC_RC_SUCCESS != rc)
				LOG_ER("MDS flat decode callback failed");
			else
				TRACE("MDS flat decode callback success");
		}
		break;

	case MDS_CALLBACK_DEC:
		{
			if (!m_NCS_MSG_FORMAT_IS_VALID(info->info.dec.i_msg_fmt_ver,
						       AVA_AVND_SUBPART_VER_MIN,
						       AVA_AVND_SUBPART_VER_MAX, ava_avnd_msg_fmt_map_table)) {
				/* log the problem */
				LOG_ER("MDS decode callback failed: Incorrect message format version");

				return NCSCC_RC_FAILURE;
			}

			rc = ava_mds_dec(cb, &info->info.dec);

			if (NCSCC_RC_SUCCESS != rc)
				LOG_ER("MDS decode callback failed");
			else
				TRACE("MDS decode callback success");
		}
		break;
	case MDS_CALLBACK_ENC:
		{
			info->info.enc.o_msg_fmt_ver = m_NCS_ENC_MSG_FMT_GET(info->info.enc.i_rem_svc_pvt_ver,
									     AVA_AVND_SUBPART_VER_MIN,
									     AVA_AVND_SUBPART_VER_MAX,
									     ava_avnd_msg_fmt_map_table);

			if (info->info.enc.o_msg_fmt_ver < AVSV_AVND_AVA_MSG_FMT_VER_1) {
				/* log the problem */
				LOG_ER("MDS flat encode callback failed: Incorrect message format version");
				return NCSCC_RC_FAILURE;
			}

			rc = ava_mds_enc(cb, &info->info.enc);

			if (NCSCC_RC_SUCCESS != rc)
				LOG_ER("MDS encode callback failed");
			else
				TRACE("MDS encode callback success");
		}

		break;

	default:
		osafassert(0);
	}

 done:
	/* return ava cb */
	if (cb)
		ncshm_give_hdl((uint32_t)info->i_yr_svc_hdl);

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ava_mds_send
 
  Description   : This routine sends the AvA message to AvND. The send 
                  operation may be a 'normal' send or a synchronous call that 
                  blocks until the response is received from AvND.
 
  Arguments     : cb  - ptr to the AvA CB
                  i_msg - ptr to the AvA message
                  o_msg - double ptr to AvA message response
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE/timeout
 
  Notes         : None.
******************************************************************************/
uint32_t ava_mds_send(AVA_CB *cb, AVSV_NDA_AVA_MSG *i_msg, AVSV_NDA_AVA_MSG **o_msg)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();	

	if (!i_msg || !m_AVA_FLAG_IS_AVND_UP(cb)) {
		LOG_ER("Incoming message is not NULL or AvND is not yet up");
		return NCSCC_RC_FAILURE;
	}

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_AVA;
	mds_info.i_op = MDS_SEND;

	if (o_msg) {
		/* synchronous blocking send */
		rc = ava_mds_msg_syn_send(cb, &mds_info, i_msg);
		if (NCSCC_RC_SUCCESS == rc) {
			/* retrieve the response */
			*o_msg = (AVSV_NDA_AVA_MSG *)mds_info.info.svc_send.info.sndrsp.o_rsp;
			mds_info.info.svc_send.info.sndrsp.o_rsp = 0;
		}
	} else
		/* just a 'normal' send */
		rc = ava_mds_msg_async_send(cb, &mds_info, i_msg);

	if (NCSCC_RC_SUCCESS != rc)
		LOG_ER("AVA MDS send failed");
	else
		TRACE("AVA MDS send success");

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ava_mds_rcv
 
  Description   : This routine is invoked when AvA message is received from 
                  AvND.
 
  Arguments     : cb       - ptr to the AvA control block
                  rcv_info - ptr to the MDS receive info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t ava_mds_rcv(AVA_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info)
{
	AVSV_NDA_AVA_MSG *msg = (AVSV_NDA_AVA_MSG *)rcv_info->i_msg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();	

	if (!msg) {
		LOG_ER("Message is NULL");
		return NCSCC_RC_FAILURE;
	}

	/* process the message */
	rc = ava_avnd_msg_prc(cb, msg);

	/* free the message content */
	if (msg)
		avsv_nda_ava_msg_free(msg);

	TRACE_LEAVE2("ret val = %d",rc);
	return rc;
}

/****************************************************************************
  Name          : ava_mds_svc_evt
 
  Description   : This routine is invoked to inform AvA of MDS events. AvA 
                  had subscribed to these events during MDS registration.
 
  Arguments     : cb       - ptr to the AvA control block
                  evt_info - ptr to the MDS event info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t ava_mds_svc_evt(AVA_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *evt_info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("EventType = %d, service id = %d",evt_info->i_change, evt_info->i_svc_id);	

	/* assign mds-dest values for AVD, AVND & AVA as per the MDS event */
	switch (evt_info->i_change) {
	case NCSMDS_UP:
		switch (evt_info->i_svc_id) {
		case NCSMDS_SVC_ID_AVND:
			cb->avnd_dest = evt_info->i_dest;
			m_AVA_FLAG_SET(cb, AVA_FLAG_AVND_UP);

			/* Protect FD_VALID_FLAG with lock */
			m_NCS_LOCK(&cb->lock, NCS_LOCK_WRITE);
			if (m_AVA_FLAG_IS_FD_VALID(cb))
				/* write into the fd */
				m_NCS_SEL_OBJ_IND(cb->sel_obj);
			m_NCS_UNLOCK(&cb->lock, NCS_LOCK_WRITE);
			break;

		default:
			osafassert(0);
		}
		break;

	case NCSMDS_DOWN:
		switch (evt_info->i_svc_id) {
		case NCSMDS_SVC_ID_AVND:
			memset(&cb->avnd_dest, 0, sizeof(MDS_DEST));
			m_AVA_FLAG_RESET(cb, AVA_FLAG_AVND_UP);
			if (amf_down_cb == NULL) {
				/* AMF is down, terminate this process */
				LOG_AL("AMF Node Director is down, terminate this process");
				raise(SIGTERM);
			}
			else {
				/* A client has installed a callback pointer, call it */
				LOG_IN("Invoking callback registered by the client");
				amf_down_cb();
			}
			break;

		default:
			osafassert(0);
		}
		break;

	default:
		break;
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ava_mds_flat_enc
 
  Description   : This routine is invoked to encode AvND messages.
 
  Arguments     : cb       - ptr to the AvA control block
                  enc_info - ptr to the MDS encode info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t ava_mds_flat_enc(AVA_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *enc_info)
{
	AVSV_NDA_AVA_MSG *msg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();	

	/* get the message ptr */
	msg = (AVSV_NDA_AVA_MSG *)enc_info->i_msg;

	switch (msg->type) {
	case AVSV_AVA_API_MSG:
		/* encode into userbuf */
		rc = ncs_encode_n_octets_in_uba(enc_info->io_uba, (uint8_t *)msg, sizeof(AVSV_NDA_AVA_MSG));
		break;

	case AVSV_AVND_AMF_CBK_MSG:
	case AVSV_AVND_AMF_API_RESP_MSG:
	default:
		osafassert(0);
	}			/* switch */

	TRACE_LEAVE2("retval = %u",rc);
	return rc;
}

/****************************************************************************
  Name          : ava_mds_flat_dec
 
  Description   : This routine is invoked to decode AvND messages.
 
  Arguments     : cb       - ptr to the AvA control block
                  rcv_info - ptr to the MDS decode info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t ava_mds_flat_dec(AVA_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *dec_info)
{
	AVSV_NDA_AVA_MSG *msg = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();	

	/* allocate the msg */
	msg = calloc(1, sizeof(AVSV_NDA_AVA_MSG));
	if (!msg) {
		rc = NCSCC_RC_FAILURE;
		LOG_CR("Calloc failed");
		goto err;
	}

	/* decode the top level ava msg contents */
	rc = ncs_decode_n_octets_from_uba(dec_info->io_uba, (uint8_t *)msg, sizeof(AVSV_NDA_AVA_MSG));
	if (NCSCC_RC_SUCCESS != rc) {
		TRACE_2("ncs_decode_n_octets_from_uba failed with rc = %d", rc);
		goto err;
	}

	/* decode individual ava msg (if not decoded above) */
	switch (msg->type) {
	case AVSV_AVND_AMF_CBK_MSG:
		{
			/* alloc cbk-info */
			msg->info.cbk_info = 0;
			if (0 == (msg->info.cbk_info = calloc(1, sizeof(AVSV_AMF_CBK_INFO)))) {
				LOG_CR("Calloc failed");
				rc = NCSCC_RC_FAILURE;
				goto err;
			}

			/* decode cbk-info */
			rc = ncs_decode_n_octets_from_uba(dec_info->io_uba,
							  (uint8_t *)msg->info.cbk_info, sizeof(AVSV_AMF_CBK_INFO));
			if (NCSCC_RC_SUCCESS != rc) {
				TRACE_2("ncs_decode_n_octets_from_uba failed with rc = %d", rc);
				goto err;
			}

			switch (msg->info.cbk_info->type) {
			case AVSV_AMF_CSI_SET:
				{
					AVSV_AMF_CSI_SET_PARAM *csi_set = &msg->info.cbk_info->param.csi_set;
					uint16_t len, i;
					uint8_t *p8;
					if (csi_set->attrs.number) {
						csi_set->attrs.list = 0;
						csi_set->attrs.list =
						    calloc(1, csi_set->attrs.number * sizeof(AVSV_ATTR_NAME_VAL));
						if (!csi_set->attrs.list) {
							rc = NCSCC_RC_FAILURE;
							LOG_CR("Calloc failed");
							goto err;
						}
						for(i=0; i<csi_set->attrs.number; i++) {
							rc = ncs_decode_n_octets_from_uba(dec_info->io_uba,
								(uint8_t *)&csi_set->attrs.list[i].name,
								sizeof(SaNameT));
							if (NCSCC_RC_SUCCESS != rc) {
								LOG_CR("ncs_decode_n_octets_from_uba failed with rc= %d", rc);
								goto err;
							}
							p8 = ncs_dec_flatten_space(dec_info->io_uba, (uint8_t *)&len, 2);
							len = ncs_decode_16bit(&p8);
							ncs_dec_skip_space(dec_info->io_uba, 2);
							csi_set->attrs.list[i].string_ptr = calloc(1, len+1);
							osafassert(csi_set->attrs.list[i].string_ptr);
							rc = ncs_decode_n_octets_from_uba(dec_info->io_uba,
								(uint8_t *)csi_set->attrs.list[i].string_ptr,
								(len+1));
							if (NCSCC_RC_SUCCESS != rc) {
								LOG_CR("ncs_decode_n_octets_from_uba failed with rc = %d",                                                                             rc);
								goto err;
							}
						}
					}
				}
				break;

			case AVSV_AMF_PG_TRACK:
				{
					AVSV_AMF_PG_TRACK_PARAM *pg_track = &msg->info.cbk_info->param.pg_track;

					if (pg_track->buf.numberOfItems) {
						pg_track->buf.notification = 0;
						pg_track->buf.notification =
						    calloc(1, pg_track->buf.numberOfItems *
							   sizeof(SaAmfProtectionGroupNotificationT));
						if (!pg_track->buf.notification) {
							rc = NCSCC_RC_FAILURE;
							LOG_CR("Calloc failed");
							goto err;
						}

						rc = ncs_decode_n_octets_from_uba(dec_info->io_uba,
										  (uint8_t *)pg_track->buf.notification,
										  pg_track->buf.numberOfItems *
										  sizeof
										  (SaAmfProtectionGroupNotificationT));
						if (NCSCC_RC_SUCCESS != rc) {
							TRACE_2("ncs_decode_n_octets_from_uba failed with rc = %d", rc);
							goto err;
						}
					}
				}
				break;

			case AVSV_AMF_HC:
			case AVSV_AMF_COMP_TERM:
			case AVSV_AMF_CSI_REM:
			case AVSV_AMF_PXIED_COMP_INST:
			case AVSV_AMF_PXIED_COMP_CLEAN:
				/* already decoded above */
				break;

			default:
				osafassert(0);
			}	/* switch */
		}
		break;

	case AVSV_AVND_AMF_API_RESP_MSG:
		/* already decoded above */
		break;

	case AVSV_AVA_API_MSG:
	default:
		osafassert(0);
	}			/* switch */

	/* decode successful */
	dec_info->o_msg = (NCSCONTEXT)msg;

	TRACE_LEAVE2("retval = %u",rc);
	return rc;

 err:
	if (msg)
		avsv_nda_ava_msg_free(msg);
	dec_info->o_msg = 0;
	TRACE_LEAVE2("retval = %u",rc);
	return rc;
}

/****************************************************************************
  Name          : ava_mds_msg_async_send
 
  Description   : This routine sends the AvA message to AvND.
 
  Arguments     : cb  - ptr to the AvA CB
                  mds_info - ptr to MDS info
                  i_msg - ptr to the AvA message
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t ava_mds_msg_async_send(AVA_CB *cb, NCSMDS_INFO *mds_info, AVSV_NDA_AVA_MSG *i_msg)
{
	MDS_SEND_INFO *send_info = &mds_info->info.svc_send;
	MDS_SENDTYPE_SND_INFO *send = &send_info->info.snd;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();	

	/* populate the send info */
	send_info->i_msg = (NCSCONTEXT)i_msg;
	send_info->i_to_svc = NCSMDS_SVC_ID_AVND;
	send_info->i_priority = MDS_SEND_PRIORITY_MEDIUM;	/* Discuss the priority assignments TBD */
	send_info->i_sendtype = MDS_SENDTYPE_SND;
	send->i_to_dest = cb->avnd_dest;

	/* release the cb lock */
	if ((AVSV_AVA_API_MSG == i_msg->type) && (AVSV_AMF_INITIALIZE == i_msg->info.api_info.type))
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_WRITE);
	else
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);

	/* send the message */
	rc = ncsmds_api(mds_info);

	/* reacquire the cb lock */
	if ((AVSV_AVA_API_MSG == i_msg->type) &&
	    ((AVSV_AMF_INITIALIZE == i_msg->info.api_info.type) || (AVSV_AMF_FINALIZE == i_msg->info.api_info.type)))
		m_NCS_LOCK(&cb->lock, NCS_LOCK_WRITE);
	else
		m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);

	TRACE_LEAVE2("retval = %u",rc);
	return rc;
}

/****************************************************************************
  Name          : ava_mds_msg_syn_send
 
  Description   : This routine sends the AvA message to AvND & blocks for 
                  the response from AvND.
 
  Arguments     : cb  - ptr to the AvA CB
                  mds_info - ptr to MDS info
                  i_msg - ptr to the AvA message
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t ava_mds_msg_syn_send(AVA_CB *cb, NCSMDS_INFO *mds_info, AVSV_NDA_AVA_MSG *i_msg)
{
	MDS_SEND_INFO *send_info = &mds_info->info.svc_send;
	MDS_SENDTYPE_SNDRSP_INFO *send = &send_info->info.sndrsp;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();	

	/* populate the send info */
	send_info->i_msg = (NCSCONTEXT)i_msg;
	send_info->i_to_svc = NCSMDS_SVC_ID_AVND;
	send_info->i_priority = MDS_SEND_PRIORITY_MEDIUM;	/* Discuss the priority assignments TBD */
	send_info->i_sendtype = MDS_SENDTYPE_SNDRSP;
	send->i_to_dest = cb->avnd_dest;
	send->i_time_to_wait = SYSF_AVA_API_RESP_TIME;

	/* release the cb lock */
	if ((AVSV_AVA_API_MSG == i_msg->type) && (AVSV_AMF_FINALIZE == i_msg->info.api_info.type))
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_WRITE);
	else
		m_NCS_UNLOCK(&cb->lock, NCS_LOCK_READ);

	/* send the message & block until AvND responds or operation times out */
	rc = ncsmds_api(mds_info);

	/* reacquire the cb lock */
	if ((AVSV_AVA_API_MSG == i_msg->type) &&
	    ((AVSV_AMF_INITIALIZE == i_msg->info.api_info.type) || (AVSV_AMF_FINALIZE == i_msg->info.api_info.type)))
		m_NCS_LOCK(&cb->lock, NCS_LOCK_WRITE);
	else
		m_NCS_LOCK(&cb->lock, NCS_LOCK_READ);

	TRACE_LEAVE2("retval = %u",rc);
	return rc;
}

/****************************************************************************
 * Name          : ava_mds_param_get
 *
 * Description   : This routine gets the mds handle & AvA MDS address.
 *
 * Arguments     : cb - ptr to the AvA control block
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t ava_mds_param_get(AVA_CB *cb)
{
	NCSADA_INFO ada_info;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();	

	memset(&ada_info, 0, sizeof(ada_info));

	ada_info.req = NCSADA_GET_HDLS;

	/* invoke ada request */
	rc = ncsada_api(&ada_info);
	if (NCSCC_RC_SUCCESS != rc) {
		TRACE_2("MDS Get handle request failed");
		goto done;
	}	

	/* store values returned by ada */
	cb->mds_hdl = ada_info.info.adest_get_hdls.o_mds_pwe1_hdl;
	cb->ava_dest = ada_info.info.adest_get_hdls.o_adest;

 done:
	TRACE_LEAVE2("retal = %d",rc);
	return rc;
}

/****************************************************************************
  Name          : ava_mds_dec 

  Description   : This routine is invoked to decode AVND message.

  Arguments     : cb       - ptr to the AvA control block
                  dec_info - ptr to the MDS decode info

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
uint32_t ava_mds_dec(AVA_CB *cb, MDS_CALLBACK_DEC_INFO *dec_info)
{
	EDU_ERR ederror = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();	

	switch (dec_info->i_fr_svc_id) {
	case NCSMDS_SVC_ID_AVND:

		dec_info->o_msg = NULL;

		rc = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_nda_msg, dec_info->io_uba,
					EDP_OP_TYPE_DEC, (AVSV_NDA_AVA_MSG **)&dec_info->o_msg,
					&ederror, dec_info->i_msg_fmt_ver);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_2("EDU rules execution failed");
			if (dec_info->o_msg != NULL) {
				avsv_nda_ava_msg_free(dec_info->o_msg);
				dec_info->o_msg = NULL;
			}
			return rc;
		}
		break;

	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE2("retval = %u",rc);
	return rc;
}

/****************************************************************************
  Name          : ava_mds_enc

  Description   : This routine is invoked to encode AVND message.

  Arguments     : cb       - ptr to the AvA control block
                  enc_info - ptr to the MDS encode info

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
uint32_t ava_mds_enc(AVA_CB *cb, MDS_CALLBACK_ENC_INFO *enc_info)
{
	EDU_ERR ederror = 0;
	AVSV_NDA_AVA_MSG *msg = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();	

	msg = (AVSV_NDA_AVA_MSG *)enc_info->i_msg;

	switch (msg->type) {
	case AVSV_AVA_API_MSG:
		rc = m_NCS_EDU_VER_EXEC(&cb->edu_hdl, avsv_edp_nda_msg, enc_info->io_uba,
					EDP_OP_TYPE_ENC, msg, &ederror, enc_info->o_msg_fmt_ver);
		return rc;
		break;

	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE2("retval = %u", rc);
	return rc;
}

/**
 * Install a callback pointer that will be called when it is detected that AMF has gone down.
 * No public definition in a header file on purpose!
 * 
 * @param cb
 * 
 */
void ava_install_amf_down_cb(void (*cb) (void))
{
	TRACE_ENTER();

	osafassert(amf_down_cb == NULL);
	amf_down_cb = cb;

	TRACE_LEAVE();
}
