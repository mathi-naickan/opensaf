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
 * Author(s): Ericsson AB
 *
 */

#include <stdlib.h>
#include "ntfa.h"
#include "ntfsv_enc_dec.h"

static MDS_CLIENT_MSG_FORMAT_VER
 NTFA_WRT_NTFS_MSG_FMT_ARRAY[NTFA_WRT_NTFS_SUBPART_VER_RANGE] = {
	1			/*msg format version for NTFA subpart version 1 */
};

/****************************************************************************
  Name          : ntfa_enc_initialize_msg
 
  Description   : This routine encodes an initialize API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_enc_initialize_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_initialize_req_t *param = &msg->info.api_info.param.init;

	TRACE_ENTER();
	osafassert(uba != NULL);

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 3);
	if (!p8) {
		TRACE("NULL pointer");
		return NCSCC_RC_OUT_OF_MEM;
	}
	ncs_encode_8bit(&p8, param->version.releaseCode);
	ncs_encode_8bit(&p8, param->version.majorVersion);
	ncs_encode_8bit(&p8, param->version.minorVersion);
	ncs_enc_claim_space(uba, 3);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : ntfa_enc_finalize_msg
 
  Description   : This routine encodes a finalize API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_enc_finalize_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_finalize_req_t *param = &msg->info.api_info.param.finalize;

	TRACE_ENTER();

	osafassert(uba != NULL);

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return NCSCC_RC_OUT_OF_MEM;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_enc_claim_space(uba, 4);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : ntfa_enc_subscribe_msg
 
  Description   : This routine encodes a subscribe API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_enc_subscribe_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	ntfsv_subscribe_req_t *param = &msg->info.api_info.param.subscribe;
	return ntfsv_enc_subscribe_msg(uba, param);
}

/****************************************************************************
  Name          : ntfa_enc_unsubscribe_msg
 
  Description   : This routine encodes a log stream close API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_enc_unsubscribe_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	ntfsv_unsubscribe_req_t *param = &msg->info.api_info.param.unsubscribe;
	return ntfsv_enc_unsubscribe_msg(uba, param);
}

/****************************************************************************
  Name          : ntfa_enc_send_not_msg
 
  Description   : This routine encodes a send notification API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_enc_send_not_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	ntfsv_send_not_req_t *param = msg->info.api_info.param.send_notification;
	return ntfsv_enc_not_msg(uba, param);
}

/****************************************************************************
  Name          : ntfa_enc_reader_initialize_msg
 
  Description   : This routine encodes an reader_initialize API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_enc_reader_initialize_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	return ntfsv_enc_reader_initialize_msg(uba,msg);
}

/****************************************************************************
  Name          : ntfa_enc_reader_initialize_msg_2
 
  Description   : This routine encodes an reader_initialize API msg 
                  filter is added in reader_initialize_msg_2
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_enc_reader_initialize_msg_2(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	return ntfsv_enc_reader_initialize_msg_2(uba,msg);
}

/****************************************************************************
  Name          : ntfa_enc_reader_finalize_msg
 
  Description   : This routine encodes an reader_finalize API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_enc_reader_finalize_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_reader_finalize_req_t *param = &msg->info.api_info.param.reader_finalize;

	TRACE_ENTER();
	osafassert(uba != NULL);

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("NULL pointer");
		return NCSCC_RC_OUT_OF_MEM;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_32bit(&p8, param->readerId);
	ncs_enc_claim_space(uba, 8);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : ntfa_enc_read_next_msg
 
  Description   : This routine encodes an read_next API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_enc_read_next_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_read_next_req_t *param = &msg->info.api_info.param.read_next;

	TRACE_ENTER();
	osafassert(uba != NULL);

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 10);
	if (!p8) {
		TRACE("NULL pointer");
		return NCSCC_RC_OUT_OF_MEM;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_8bit(&p8, param->searchDirection);
	ncs_encode_32bit(&p8, param->readerId);
	ncs_enc_claim_space(uba, 10);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : ntfa_ntfs_msg_proc
 
  Description   : This routine is used to process the ASYNC incoming
                  NTFS messages. 
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t ntfa_ntfs_msg_proc(ntfa_cb_t *cb, ntfsv_msg_t *ntfsv_msg, MDS_SEND_PRIORITY_TYPE prio)
{
	TRACE_ENTER();

	switch (ntfsv_msg->type) {
	case NTFSV_NTFS_CBK_MSG:
		switch (ntfsv_msg->info.cbk_info.type) {
		case NTFSV_NOTIFICATION_CALLBACK:
			{
				ntfa_client_hdl_rec_t *ntfa_hdl_rec;

				TRACE_2("NTFSV_NOTIFICATION_CALLBACK: "
					"subscriptionId = %d,"
					" client_id = %d",
					(int)ntfsv_msg->info.cbk_info.subscriptionId,
					(int)ntfsv_msg->info.cbk_info.ntfs_client_id);
			/** Lookup the hdl rec by client_id  **/
				if (NULL == (ntfa_hdl_rec =
					     ntfa_find_hdl_rec_by_client_id(cb,
									    ntfsv_msg->info.cbk_info.ntfs_client_id))) {
					TRACE("client_id not found");
					ntfa_msg_destroy(ntfsv_msg);
					TRACE_LEAVE();
					return NCSCC_RC_FAILURE;
				}
			/** enqueue this message  **/
				if (NCSCC_RC_SUCCESS != m_NCS_IPC_SEND(&ntfa_hdl_rec->mbx, ntfsv_msg, prio)) {
					TRACE("IPC SEND FAILED");
					TRACE_LEAVE();
					return NCSCC_RC_FAILURE;
				}
			}
			break;
		case NTFSV_DISCARDED_CALLBACK:
			{
				ntfa_client_hdl_rec_t *ntfa_hdl_rec;

				TRACE_2("NTFSV_DISCARDED_CALLBACK: "
					"subscriptionId = %d,"
					" client_id = %d",
					(int)ntfsv_msg->info.cbk_info.subscriptionId,
					(int)ntfsv_msg->info.cbk_info.ntfs_client_id);
			/** Lookup the hdl rec by client_id  **/
				if (NULL == (ntfa_hdl_rec =
					     ntfa_find_hdl_rec_by_client_id(cb,
									    ntfsv_msg->info.cbk_info.ntfs_client_id))) {
					TRACE("client_id not found");
					ntfa_msg_destroy(ntfsv_msg);
					TRACE_LEAVE();
					return NCSCC_RC_FAILURE;
				}
			/** enqueue this message  **/
				if (NCSCC_RC_SUCCESS != m_NCS_IPC_SEND(&ntfa_hdl_rec->mbx, ntfsv_msg, prio)) {
					TRACE("IPC SEND FAILED");
					TRACE_LEAVE();
					return NCSCC_RC_FAILURE;
				}
			}
			break;

		default:
			TRACE("unknown type %d", ntfsv_msg->info.cbk_info.type);
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
			break;
		}
		break;
	default:
	    /** Unexpected message **/
		TRACE_2("Unexpected message type: %d", ntfsv_msg->type);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
		break;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : ntfa_mds_svc_evt
 
  Description   : This is a callback routine that is invoked to inform NTFA 
                  of MDS events. NTFA had subscribed to these events during
                  through MDS subscription.
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_mds_svc_evt(struct ncsmds_callback_info *mds_cb_info)
{
	TRACE_2("NTFA Rcvd MDS subscribe evt from svc %d \n", mds_cb_info->info.svc_evt.i_svc_id);

	switch (mds_cb_info->info.svc_evt.i_change) {
	case NCSMDS_NO_ACTIVE:
	case NCSMDS_DOWN:
		if (mds_cb_info->info.svc_evt.i_svc_id == NCSMDS_SVC_ID_NTFS) {
		/** TBD what to do if NTFS goes down
                 ** Hold on to the subscription if possible
                 ** to send them out if NTFS comes back up
                 **/
			TRACE("NTFS down");
			pthread_mutex_lock(&ntfa_cb.cb_lock);
			memset(&ntfa_cb.ntfs_mds_dest, 0, sizeof(MDS_DEST));
			ntfa_cb.ntfs_up = 0;
			pthread_mutex_unlock(&ntfa_cb.cb_lock);
		}
		break;
	case NCSMDS_NEW_ACTIVE:
	case NCSMDS_UP:
		switch (mds_cb_info->info.svc_evt.i_svc_id) {
		case NCSMDS_SVC_ID_NTFS:
		    /** Store the MDS DEST of the NTFS 
                     **/
			TRACE_2("MSG from NTFS NCSMDS_NEW_ACTIVE/UP");
			pthread_mutex_lock(&ntfa_cb.cb_lock);
			ntfa_cb.ntfs_mds_dest = mds_cb_info->info.svc_evt.i_dest;
			ntfa_cb.ntfs_up = 1;
			if (ntfa_cb.ntfs_sync_awaited) {
				/* signal waiting thread */
				m_NCS_SEL_OBJ_IND(&ntfa_cb.ntfs_sync_sel);
			}
			pthread_mutex_unlock(&ntfa_cb.cb_lock);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : ntfa_mds_rcv
 
  Description   : This is a callback routine that is invoked when NTFA message
                  is received from NTFS.
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

static uint32_t ntfa_mds_rcv(struct ncsmds_callback_info *mds_cb_info)
{
	ntfsv_msg_t *ntfsv_msg = (ntfsv_msg_t *)mds_cb_info->info.receive.i_msg;
	uint32_t rc;

	pthread_mutex_lock(&ntfa_cb.cb_lock);

	/*this priority is later used in unsubscribe api to post messeges back to mailbox*/
	ntfsv_msg->info.cbk_info.mds_send_priority = mds_cb_info->info.receive.i_priority;

	/* process the message */
	rc = ntfa_ntfs_msg_proc(&ntfa_cb, ntfsv_msg, mds_cb_info->info.receive.i_priority);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_2("ntfa_ntfs_msg_proc returned: %d", rc);
	}

	pthread_mutex_unlock(&ntfa_cb.cb_lock);

	return rc;
}

/****************************************************************************
  Name          : ntfa_mds_enc
 
  Description   : This is a callback routine that is invoked to encode NTFS
                  messages.
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_mds_enc(struct ncsmds_callback_info *info)
{
	ntfsv_msg_t *msg;
	NCS_UBAID *uba;
	uint8_t *p8;
	uint32_t rc = NCSCC_RC_FAILURE;
	MDS_CLIENT_MSG_FORMAT_VER msg_fmt_version;

	TRACE_ENTER();
	msg_fmt_version = m_NCS_ENC_MSG_FMT_GET(info->info.enc.i_rem_svc_pvt_ver,
						NTFA_WRT_NTFS_SUBPART_VER_AT_MIN_MSG_FMT,
						NTFA_WRT_NTFS_SUBPART_VER_AT_MAX_MSG_FMT, NTFA_WRT_NTFS_MSG_FMT_ARRAY);
	if (0 == msg_fmt_version) {
		TRACE("Wrong msg_fmt_version!!\n");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	info->info.enc.o_msg_fmt_ver = msg_fmt_version;

	msg = (ntfsv_msg_t *)info->info.enc.i_msg;
	uba = info->info.enc.io_uba;

	if (uba == NULL) {
		TRACE("uba=NULL");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

    /** encode the type of message **/
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		TRACE_LEAVE();
		return NCSCC_RC_OUT_OF_MEM;  
	}
	ncs_encode_32bit(&p8, msg->type);
	ncs_enc_claim_space(uba, 4);

	TRACE_2("msgtype: %d", msg->type);
	if (NTFSV_NTFA_API_MSG == msg->type) {
	/** encode the API msg subtype **/
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE("encode API msg subtype FAILED");
			TRACE_LEAVE();	/* fixme: ok to do return fail?? */
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_32bit(&p8, msg->info.api_info.type);
		ncs_enc_claim_space(uba, 4);

		TRACE_2("api_info.type: %d\n", msg->info.api_info.type);
		switch (msg->info.api_info.type) {
		case NTFSV_INITIALIZE_REQ:
			rc = ntfa_enc_initialize_msg(uba, msg);
			break;

		case NTFSV_FINALIZE_REQ:
			rc = ntfa_enc_finalize_msg(uba, msg);
			break;

		case NTFSV_SUBSCRIBE_REQ:
			rc = ntfa_enc_subscribe_msg(uba, msg);
			break;

		case NTFSV_UNSUBSCRIBE_REQ:
			rc = ntfa_enc_unsubscribe_msg(uba, msg);
			break;
		case NTFSV_SEND_NOT_REQ:
			rc = ntfa_enc_send_not_msg(uba, msg);
			break;
		case NTFSV_READER_INITIALIZE_REQ:
			rc = ntfa_enc_reader_initialize_msg(uba, msg);
			break;

		case NTFSV_READER_INITIALIZE_REQ_2:
			rc = ntfa_enc_reader_initialize_msg_2(uba, msg);
			break;

		case NTFSV_READER_FINALIZE_REQ:
			rc = ntfa_enc_reader_finalize_msg(uba, msg);
			break;

		case NTFSV_READ_NEXT_REQ:
			rc = ntfa_enc_read_next_msg(uba, msg);
			break;

		default:
			TRACE("Unknown API type = %d", msg->info.api_info.type);
			rc = NCSCC_RC_FAILURE;
			break;
		}
	}
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_2("enc mgs type %d, returned err rc = %u", msg->type, rc);
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ntfa_dec_initialize_rsp_msg
 
  Description   : This routine decodes an initialize sync response message
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_dec_initialize_rsp_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_initialize_rsp_t *param = &msg->info.api_resp_info.param.init_rsp;
	uint8_t local_data[4];

	osafassert(uba != NULL);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->client_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : ntfa_dec_not_send_cbk_msg
 
  Description   : This routine decode an notification callback msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_dec_not_send_cbk_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	osafassert(uba != NULL);
	ntfsv_send_not_req_t *param = msg->info.cbk_info.param.notification_cbk;
	return ntfsv_dec_not_msg(uba, param);
}

/****************************************************************************
  Name          : ntfa_dec_not_discard_cbk_msg
 
  Description   : This routine decode discarded callback msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_dec_not_discard_cbk_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	osafassert(uba != NULL);
	ntfsv_discarded_info_t *param = &msg->info.cbk_info.param.discarded_cbk;
	return ntfsv_dec_discard_msg(uba, param);
}

/****************************************************************************
  Name          : ntfa_dec_subscribe_rsp_msg
 
  Description   : This routine decodes a subscribe response message
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_dec_subscribe_rsp_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_subscribe_rsp_t *param = &msg->info.api_resp_info.param.subscribe_rsp;
	uint8_t local_data[4];

	osafassert(uba != NULL);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->subscriptionId = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : ntfa_dec_send_not_rsp_msg
 
  Description   : This routine decodes a notification send response message
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_dec_send_not_rsp_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_send_not_rsp_t *param = &msg->info.api_resp_info.param.send_not_rsp;
	uint8_t local_data[8];

	osafassert(uba != NULL);

	/* chan_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->notificationId = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : ntfa_dec_reader_initialize_rsp_msg
 
  Description   : This routine decodes an reader_initialize sync response message
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_dec_reader_initialize_rsp_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_reader_init_rsp_t *param = &msg->info.api_resp_info.param.reader_init_rsp;
	uint8_t local_data[4];

	osafassert(uba != NULL);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->readerId = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : ntfa_dec_reader_finalize_rsp_msg
 
  Description   : This routine decodes an reader_finalize sync response message
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_dec_reader_finalize_rsp_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_reader_finalize_rsp_t *param = &msg->info.api_resp_info.param.reader_finalize_rsp;
	uint8_t local_data[4];

	osafassert(uba != NULL);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->reader_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : ntfa_dec_read_next_rsp_msg
 
  Description   : This routine decodes an read_next sync response message
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_dec_read_next_rsp_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	ntfsv_read_next_rsp_t *param = &msg->info.api_resp_info.param.read_next_rsp;
	osafassert(uba != NULL);
	if (msg->info.api_resp_info.rc != SA_AIS_OK) {
		/* only header is received */
		  return NCSCC_RC_SUCCESS;
	}
	param->readNotification = calloc(1, sizeof(ntfsv_send_not_req_t));
	if (NULL == param->readNotification)
		return NCSCC_RC_OUT_OF_MEM;
	return ntfsv_dec_not_msg(uba, param->readNotification);
}

/****************************************************************************
  Name          : ntfa_mds_dec
 
  Description   : This is a callback routine that is invoked to decode NTFS
                  messages.
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_mds_dec(struct ncsmds_callback_info *info)
{
	uint8_t *p8;
	ntfsv_msg_t *msg;
	NCS_UBAID *uba = info->info.dec.io_uba;
	uint8_t local_data[12];
	uint32_t rc;
	TRACE_ENTER();

	if (0 == m_NCS_MSG_FORMAT_IS_VALID(info->info.dec.i_msg_fmt_ver,
					   NTFA_WRT_NTFS_SUBPART_VER_AT_MIN_MSG_FMT,
					   NTFA_WRT_NTFS_SUBPART_VER_AT_MAX_MSG_FMT, NTFA_WRT_NTFS_MSG_FMT_ARRAY)) {
		TRACE("Invalid message format!!!\n");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

    /** Allocate a new msg in both sync/async cases 
     **/
	if (NULL == (msg = calloc(1, sizeof(ntfsv_msg_t)))) {
		TRACE("calloc failed\n");
		return NCSCC_RC_FAILURE;
	}

	info->info.dec.o_msg = (uint8_t *)msg;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	msg->type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

	switch (msg->type) {
	case NTFSV_NTFA_API_RESP_MSG:
		{
			p8 = ncs_dec_flatten_space(uba, local_data, 8);
			msg->info.api_resp_info.type = ncs_decode_32bit(&p8);
			msg->info.api_resp_info.rc = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 8);
			TRACE_2("NTFSV_NTFA_API_RESP_MSG rc = %d", (int)msg->info.api_resp_info.rc);

			switch (msg->info.api_resp_info.type) {
			case NTFSV_INITIALIZE_RSP:
				rc = ntfa_dec_initialize_rsp_msg(uba, msg);
				break;
			case NTFSV_SUBSCRIBE_RSP:
				rc = ntfa_dec_subscribe_rsp_msg(uba, msg);
				break;
			case NTFSV_UNSUBSCRIBE_RSP:	/* Only header sent from server */
				rc = NCSCC_RC_SUCCESS;
				break;
			case NTFSV_SEND_NOT_RSP:
				rc = ntfa_dec_send_not_rsp_msg(uba, msg);
				break;
			case NTFSV_READER_INITIALIZE_RSP:
				rc = ntfa_dec_reader_initialize_rsp_msg(uba, msg);
				break;
			case NTFSV_READER_FINALIZE_RSP:
				rc = ntfa_dec_reader_finalize_rsp_msg(uba, msg);
				break;
			case NTFSV_READ_NEXT_RSP:
				rc = ntfa_dec_read_next_rsp_msg(uba, msg);
				break;
			case NTFSV_FINALIZE_RSP:
				rc = NCSCC_RC_SUCCESS;
				break;
			default:
				TRACE_2("Unknown API RSP type %d", msg->info.api_resp_info.type);
				rc = NCSCC_RC_FAILURE;
				break;
			}
		}
		break;
	case NTFSV_NTFS_CBK_MSG:
		{
			p8 = ncs_dec_flatten_space(uba, local_data, 12);
			msg->info.cbk_info.type = ncs_decode_32bit(&p8);
			msg->info.cbk_info.ntfs_client_id = ncs_decode_32bit(&p8);
			msg->info.cbk_info.subscriptionId = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 12);
			TRACE_2("NTFSV_NTFS_CBK_MSG");
			switch (msg->info.cbk_info.type) {
			case NTFSV_NOTIFICATION_CALLBACK:
				/* TODO: use notificationAlloc here? */
				msg->info.cbk_info.param.notification_cbk = calloc(1, sizeof(ntfsv_send_not_req_t));
				if (NULL == msg->info.cbk_info.param.notification_cbk) {
					TRACE_1("could not allocate memory");
					rc = NCSCC_RC_OUT_OF_MEM;
					break;
				}
				TRACE_2("decode notification cbk message");
				rc = ntfa_dec_not_send_cbk_msg(uba, msg);
				break;
			case NTFSV_DISCARDED_CALLBACK:
				TRACE_2("decode discarded cbk message");
				rc = ntfa_dec_not_discard_cbk_msg(uba, msg);
				break;
			default:
				TRACE_2("Unknown callback type = %d!", msg->info.cbk_info.type);
				rc = NCSCC_RC_FAILURE;
				break;
			}
		}
		break;
	default:
		TRACE("Unknown MSG type %d", msg->type);
		rc = NCSCC_RC_FAILURE;		
		break;
	}
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_2("enc mgs type %d, api/cbk type %d returned err rc = %u",
			     msg->type, msg->info.api_resp_info.type, rc);
	}
	
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : ntfa_mds_enc_flat
 *
 * Description   : MDS encode and flatten
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t ntfa_mds_enc_flat(struct ncsmds_callback_info *info)
{
	/* Modify the MDS_INFO to populate enc */
	info->info.enc = info->info.enc_flat;
	/* Invoke the regular mds_enc routine  */
	return ntfa_mds_enc(info);
}

/****************************************************************************
 * Name          : ntfa_mds_dec_flat
 *
 * Description   : MDS decode and flatten
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t ntfa_mds_dec_flat(struct ncsmds_callback_info *info)
{
	uint32_t rc;

	/* Modify the MDS_INFO to populate dec */
	info->info.dec = info->info.dec_flat;
	/* Invoke the regular mds_dec routine  */
	rc = ntfa_mds_dec(info);
	if (rc != NCSCC_RC_SUCCESS)
		TRACE("ntfa_mds_dec rc = %d", rc);

	return rc;
}

/****************************************************************************
  Name          : ntfa_mds_cpy
 
  Description   : This function copies an events sent from NTFS.
 
  Arguments     :pointer to struct ncsmds_callback_info
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t ntfa_mds_cpy(struct ncsmds_callback_info *info)
{
	TRACE_ENTER();
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ntfa_mds_callback
 *
 * Description   : Callback Dispatcher for various MDS operations.
 *
 * Arguments     : cb   : NTFS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t ntfa_mds_callback(struct ncsmds_callback_info *info)
{
	uint32_t rc;

	static NCSMDS_CALLBACK_API cb_set[MDS_CALLBACK_SVC_MAX] = {
		ntfa_mds_cpy,	/* MDS_CALLBACK_COPY      0 */
		ntfa_mds_enc,	/* MDS_CALLBACK_ENC       1 */
		ntfa_mds_dec,	/* MDS_CALLBACK_DEC       2 */
		ntfa_mds_enc_flat,	/* MDS_CALLBACK_ENC_FLAT  3 */
		ntfa_mds_dec_flat,	/* MDS_CALLBACK_DEC_FLAT  4 */
		ntfa_mds_rcv,	/* MDS_CALLBACK_RECEIVE   5 */
		ntfa_mds_svc_evt	/* MDS_CALLBACK_SVC_EVENT 6 */
	};

	if (info->i_op <= MDS_CALLBACK_SVC_EVENT) {
		rc = (*cb_set[info->i_op]) (info);
		if (rc != NCSCC_RC_SUCCESS){
			if (rc == NCSCC_RC_OUT_OF_MEM) {
				LOG_ER("Exiting: out of memory");
				exit (EXIT_FAILURE);
			}  
		}
	} else {
		TRACE("mds callback out of range op: %d", info->i_op);
		rc = NCSCC_RC_FAILURE;
	}
	return rc;
}

/****************************************************************************
  Name          : ntfa_mds_init
 
  Description   : This routine registers the NTFA Service with MDS.
 
  Arguments     : cb - ptr to the NTFA control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t ntfa_mds_init(ntfa_cb_t *cb)
{
	NCSADA_INFO ada_info;
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;
	MDS_SVC_ID svc = NCSMDS_SVC_ID_NTFS;

	TRACE_ENTER();
    /** Create the ADEST for NTFA and get the pwe hdl**/
	memset(&ada_info, '\0', sizeof(ada_info));
	ada_info.req = NCSADA_GET_HDLS;

	if (NCSCC_RC_SUCCESS != (rc = ncsada_api(&ada_info))) {
		TRACE("NCSADA_GET_HDLS failed, rc = %d", rc);
		return NCSCC_RC_FAILURE;
	}

    /** Store the info obtained from MDS ADEST creation  **/
	cb->mds_hdl = ada_info.info.adest_get_hdls.o_mds_pwe1_hdl;

    /** Now install into mds **/
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_NTFA;
	mds_info.i_op = MDS_INSTALL;

	mds_info.info.svc_install.i_yr_svc_hdl = 0;
	mds_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;	/* PWE scope */
	mds_info.info.svc_install.i_svc_cb = ntfa_mds_callback;	/* callback */
	mds_info.info.svc_install.i_mds_q_ownership = false;	/* NTFA doesn't own the mds queue */
	mds_info.info.svc_install.i_mds_svc_pvt_ver = NTFA_SVC_PVT_SUBPART_VERSION;

	if ((rc = ncsmds_api(&mds_info)) != NCSCC_RC_SUCCESS) {
		TRACE("mds api call failed");
		return NCSCC_RC_FAILURE;
	}

	/* Now subscribe for events that will be generated by MDS */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_NTFA;
	mds_info.i_op = MDS_SUBSCRIBE;

	mds_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	mds_info.info.svc_subscribe.i_num_svcs = 1;
	mds_info.info.svc_subscribe.i_svc_ids = &svc;

	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("mds api call failed");
		return rc;
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ntfa_mds_finalize
 
  Description   : This routine unregisters the NTFA Service from MDS.
 
  Arguments     : cb - ptr to the NTFA control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
void ntfa_mds_finalize(ntfa_cb_t *cb)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	/* Un-install your service into MDS. 
	   No need to cancel the services that are subscribed */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_NTFA;
	mds_info.i_op = MDS_UNINSTALL;

	if ((rc = ncsmds_api(&mds_info)) != NCSCC_RC_SUCCESS) {
		TRACE("mds api call failed");
	}

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : ntfa_mds_msg_sync_send
 
  Description   : This routine sends the NTFA message to NTFS. The send 
                  operation is a synchronous call that 
                  blocks until the response is received from NTFS.
 
  Arguments     : cb  - ptr to the NTFA CB
                  i_msg - ptr to the NTFSv message
                  o_msg - double ptr to NTFSv message response
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE/timeout
 
  Notes         : None.
******************************************************************************/
uint32_t ntfa_mds_msg_sync_send(ntfa_cb_t *cb, ntfsv_msg_t *i_msg, ntfsv_msg_t **o_msg, uint32_t timeout)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	osafassert(cb != NULL && i_msg != NULL && o_msg != NULL);

	*o_msg = NULL;
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_NTFA;
	mds_info.i_op = MDS_SEND;

	/* Fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_msg;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_NTFS;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SNDRSP;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;	/* fixme? */
	/* fill the sub send rsp strcuture */
	mds_info.info.svc_send.info.sndrsp.i_time_to_wait = timeout;	/* timeto wait in 10ms FIX!!! */
	mds_info.info.svc_send.info.sndrsp.i_to_dest = cb->ntfs_mds_dest;

	/* send the message */
	if (NCSCC_RC_SUCCESS == (rc = ncsmds_api(&mds_info))) {
		/* Retrieve the response and take ownership of the memory  */
		*o_msg = (ntfsv_msg_t *)mds_info.info.svc_send.info.sndrsp.o_rsp;
		mds_info.info.svc_send.info.sndrsp.o_rsp = NULL;
	} else
		TRACE("ntfa_mds_msg_sync_send FAILED");

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ntfa_mds_msg_async_send
 
  Description   : This routine sends the NTFA message to NTFS.
 
  Arguments     : cb  - ptr to the NTFA CB
                  i_msg - ptr to the NTFSv message
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t ntfa_mds_msg_async_send(ntfa_cb_t *cb, struct ntfsv_msg *i_msg, uint32_t prio)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	osafassert(cb != NULL && i_msg != NULL);

	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_NTFA;
	mds_info.i_op = MDS_SEND;

	/* fill the main send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_msg;
	mds_info.info.svc_send.i_priority = prio;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_NTFS;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;

	/* fill the sub send strcuture */
	mds_info.info.svc_send.info.snd.i_to_dest = cb->ntfs_mds_dest;

	/* send the message */
	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS)
		TRACE("failed");

	TRACE_LEAVE();
	return rc;
}
