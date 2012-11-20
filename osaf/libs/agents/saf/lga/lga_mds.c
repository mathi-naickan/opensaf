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
#include "lga.h"

static MDS_CLIENT_MSG_FORMAT_VER
 LGA_WRT_LGS_MSG_FMT_ARRAY[LGA_WRT_LGS_SUBPART_VER_RANGE] = {
	1			/*msg format version for LGA subpart version 1 */
};

/****************************************************************************
  Name          : lga_enc_initialize_msg
 
  Description   : This routine encodes an initialize API msg
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t lga_enc_initialize_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_initialize_req_t *param = &msg->info.api_info.param.init;

	TRACE_ENTER();
	osafassert(uba != NULL);

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 3);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_8bit(&p8, param->version.releaseCode);
	ncs_encode_8bit(&p8, param->version.majorVersion);
	ncs_encode_8bit(&p8, param->version.minorVersion);
	ncs_enc_claim_space(uba, 3);
	total_bytes += 3;

	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
  Name          : lga_enc_finalize_msg
 
  Description   : This routine encodes a finalize API msg
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t lga_enc_finalize_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_finalize_req_t *param = &msg->info.api_info.param.finalize;

	TRACE_ENTER();

	osafassert(uba != NULL);

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
  Name          : lga_enc_lstr_open_sync_msg
 
  Description   : This routine encodes a log stream open sync API msg
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t lga_enc_lstr_open_sync_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	int len;
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_stream_open_req_t *param = &msg->info.api_info.param.lstr_open_sync;

	TRACE_ENTER();
	osafassert(uba != NULL);

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 6);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_16bit(&p8, param->lstr_name.length);
	ncs_enc_claim_space(uba, 6);
	total_bytes += 6;

	/* Encode log stream name */
	ncs_encode_n_octets_in_uba(uba, param->lstr_name.value, (uint32_t)param->lstr_name.length);
	total_bytes += (uint32_t)param->lstr_name.length;

	/* Encode logFileName if initiated */
	p8 = ncs_enc_reserve_space(uba, 2);
	if (!p8) {
		TRACE("p8 NULL!!!");
		goto done;
	}
	if (param->logFileName != NULL)
		len = strlen(param->logFileName) + 1;
	else
		len = 0;
	ncs_encode_16bit(&p8, len);
	ncs_enc_claim_space(uba, 2);
	total_bytes += 2;

	if (param->logFileName != NULL) {
		ncs_encode_n_octets_in_uba(uba, (uint8_t *)param->logFileName, len);
		total_bytes += len;
	}

	/* Encode logFilePathName if initiated */
	p8 = ncs_enc_reserve_space(uba, 2);
	if (!p8) {
		TRACE("p8 NULL!!!");
		goto done;
	}
	if (param->logFilePathName)
		len = strlen(param->logFilePathName) + 1;
	else
		len = 0;
	ncs_encode_16bit(&p8, len);
	ncs_enc_claim_space(uba, 2);
	total_bytes += 2;

	if (param->logFilePathName != NULL) {
		ncs_encode_n_octets_in_uba(uba, (uint8_t *)param->logFilePathName, len);
		total_bytes += len;
	}

	/* Encode format string if initiated */
	p8 = ncs_enc_reserve_space(uba, 24);
	if (!p8) {
		TRACE("p8 NULL!!!");
		goto done;
	}
	if (param->logFileFmt != NULL)
		len = strlen(param->logFileFmt) + 1;
	else
		len = 0;
	ncs_encode_64bit(&p8, param->maxLogFileSize);
	ncs_encode_32bit(&p8, param->maxLogRecordSize);
	ncs_encode_32bit(&p8, (uint32_t)param->haProperty);
	ncs_encode_32bit(&p8, (uint32_t)param->logFileFullAction);
	ncs_encode_16bit(&p8, param->maxFilesRotated);
	ncs_encode_16bit(&p8, len);
	ncs_enc_claim_space(uba, 24);
	total_bytes += 24;

	if (len > 0) {
		ncs_encode_n_octets_in_uba(uba, (uint8_t *)param->logFileFmt, len);
		total_bytes += len;
	}

	/* Encode last item in struct => open flags!!!! */
	p8 = ncs_enc_reserve_space(uba, 1);
	if (!p8) {
		TRACE("p8 NULL!!!");
		goto done;
	}

	ncs_encode_8bit(&p8, param->lstr_open_flags);
	ncs_enc_claim_space(uba, 1);
	total_bytes += 1;

 done:
	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
  Name          : lga_enc_lstr_close_msg
 
  Description   : This routine encodes a log stream close API msg
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t lga_enc_lstr_close_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_stream_close_req_t *param = &msg->info.api_info.param.lstr_close;

	osafassert(uba != NULL);

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_32bit(&p8, param->lstr_id);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	return total_bytes;
}

/****************************************************************************
  Name          : lga_enc_write_log_async_msg
 
  Description   : This routine encodes a write log async API msg
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t lga_enc_write_log_async_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_write_log_async_req_t *param = &msg->info.api_info.param.write_log_async;
	const SaLogNtfLogHeaderT *ntfLogH;
	const SaLogGenericLogHeaderT *genLogH;

	osafassert(uba != NULL);

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 20);
	if (!p8) {
		TRACE("Could not reserve space");
		return 0;
	}
	ncs_encode_64bit(&p8, param->invocation);
	ncs_encode_32bit(&p8, param->ack_flags);
	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_32bit(&p8, param->lstr_id);
	ncs_enc_claim_space(uba, 20);
	total_bytes += 20;

	p8 = ncs_enc_reserve_space(uba, 12);
	if (!p8) {
		TRACE("Could not reserve space");
		return 0;
	}
	ncs_encode_64bit(&p8, *param->logTimeStamp);
	ncs_encode_32bit(&p8, (uint32_t)param->logRecord->logHdrType);
	ncs_enc_claim_space(uba, 12);
	total_bytes += 12;

	/* Alarm and application streams so far. */

	switch (param->logRecord->logHdrType) {
	case SA_LOG_NTF_HEADER:
		ntfLogH = &param->logRecord->logHeader.ntfHdr;
		p8 = ncs_enc_reserve_space(uba, 14);
		if (!p8) {
			TRACE("Could not reserve space");
			return 0;
		}
		ncs_encode_64bit(&p8, ntfLogH->notificationId);
		ncs_encode_32bit(&p8, (uint32_t)ntfLogH->eventType);
		ncs_encode_16bit(&p8, ntfLogH->notificationObject->length);
		ncs_enc_claim_space(uba, 14);
		total_bytes += 14;

		ncs_encode_n_octets_in_uba(uba,
					   ntfLogH->notificationObject->value,
					   (uint32_t)ntfLogH->notificationObject->length);
		total_bytes += ntfLogH->notificationObject->length;

		p8 = ncs_enc_reserve_space(uba, 2);
		if (!p8) {
			TRACE("Could not reserve space");
			return 0;
		}
		ncs_encode_16bit(&p8, ntfLogH->notifyingObject->length);
		ncs_enc_claim_space(uba, 2);
		total_bytes += 2;

		ncs_encode_n_octets_in_uba(uba, ntfLogH->notifyingObject->value, ntfLogH->notifyingObject->length);
		total_bytes += ntfLogH->notifyingObject->length;

		p8 = ncs_enc_reserve_space(uba, 16);
		if (!p8) {
			TRACE("Could not reserve space");
			return 0;
		}
		ncs_encode_32bit(&p8, ntfLogH->notificationClassId->vendorId);
		ncs_encode_16bit(&p8, ntfLogH->notificationClassId->majorId);
		ncs_encode_16bit(&p8, ntfLogH->notificationClassId->minorId);
		ncs_encode_64bit(&p8, ntfLogH->eventTime);
		ncs_enc_claim_space(uba, 16);
		total_bytes += 16;
		break;

	case SA_LOG_GENERIC_HEADER:
		genLogH = &param->logRecord->logHeader.genericHdr;
		p8 = ncs_enc_reserve_space(uba, 10);
		if (!p8) {
			TRACE("Could not reserve space");
			return 0;
		}
		ncs_encode_32bit(&p8, 0);
		ncs_encode_16bit(&p8, 0);
		ncs_encode_16bit(&p8, 0);
		ncs_encode_16bit(&p8, param->logSvcUsrName->length);
		ncs_enc_claim_space(uba, 10);
		total_bytes += 10;

		ncs_encode_n_octets_in_uba(uba,
					   (uint8_t *)param->logSvcUsrName->value, (uint32_t)param->logSvcUsrName->length);
		total_bytes += param->logSvcUsrName->length;

		p8 = ncs_enc_reserve_space(uba, 2);
		if (!p8) {
			TRACE("Could not reserve space");
			return 0;
		}
		ncs_encode_16bit(&p8, genLogH->logSeverity);
		total_bytes += 2;

		break;

	default:
		TRACE("ERROR IN logHdrType in logRecord!!!");
		break;
	}

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("Could not reserve space");
		return 0;
	}

	if (param->logRecord->logBuffer == NULL) {
		ncs_encode_32bit(&p8, (uint32_t)0);
	} else {
		ncs_encode_32bit(&p8, (uint32_t)param->logRecord->logBuffer->logBufSize);
	}
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	if ((param->logRecord->logBuffer != NULL) && (param->logRecord->logBuffer->logBuf != NULL)) {
		ncs_encode_n_octets_in_uba(uba,
					   param->logRecord->logBuffer->logBuf,
					   (uint32_t)param->logRecord->logBuffer->logBufSize);
		total_bytes += (uint32_t)param->logRecord->logBuffer->logBufSize;
	}
	return total_bytes;
}

/****************************************************************************
  Name          : lga_lgs_msg_proc
 
  Description   : This routine is used to process the ASYNC incoming
                  LGS messages. 
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t lga_lgs_msg_proc(lga_cb_t *cb, lgsv_msg_t *lgsv_msg, MDS_SEND_PRIORITY_TYPE prio)
{
	TRACE_ENTER();

	switch (lgsv_msg->type) {
	case LGSV_LGS_CBK_MSG:
		switch (lgsv_msg->info.cbk_info.type) {
		case LGSV_WRITE_LOG_CALLBACK_IND:
			{
				lga_client_hdl_rec_t *lga_hdl_rec;

				TRACE_2("LGSV_LGS_WRITE_LOG_CBK: inv = %d, error = %d",
					(int)lgsv_msg->info.cbk_info.inv, (int)lgsv_msg->info.cbk_info.write_cbk.error);

			/** Create the chan hdl record here before 
                         ** queing this message onto the priority queue
                         ** so that the dispatch by the application to fetch
                         ** the callback is instantaneous.
                         **/

			/** Lookup the hdl rec by client_id  **/
				if (NULL == (lga_hdl_rec =
					     lga_find_hdl_rec_by_regid(cb, lgsv_msg->info.cbk_info.lgs_client_id))) {
					TRACE("regid not found");
					lga_msg_destroy(lgsv_msg);
					TRACE_LEAVE();
					return NCSCC_RC_FAILURE;
				}

			/** enqueue this message  **/
				if (NCSCC_RC_SUCCESS != m_NCS_IPC_SEND(&lga_hdl_rec->mbx, lgsv_msg, prio)) {
					TRACE("IPC SEND FAILED");
					lga_msg_destroy(lgsv_msg);
					TRACE_LEAVE();
					return NCSCC_RC_FAILURE;
				}
			}
			break;

		default:
			TRACE("unknown type %d", lgsv_msg->info.cbk_info.type);
			lga_msg_destroy(lgsv_msg);
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
			break;
		}
		break;
	default:
	    /** Unexpected message **/
		TRACE_2("Unexpected message type: %d", lgsv_msg->type);
		lga_msg_destroy(lgsv_msg);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
		break;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : lga_mds_svc_evt
 
  Description   : This is a callback routine that is invoked to inform LGA 
                  of MDS events. LGA had subscribed to these events during
                  through MDS subscription.
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t lga_mds_svc_evt(struct ncsmds_callback_info *mds_cb_info)
{
	TRACE_2("LGA Rcvd MDS subscribe evt from svc %d \n", mds_cb_info->info.svc_evt.i_svc_id);

	switch (mds_cb_info->info.svc_evt.i_change) {
	case NCSMDS_NO_ACTIVE:
	case NCSMDS_DOWN:
		if (mds_cb_info->info.svc_evt.i_svc_id == NCSMDS_SVC_ID_LGS) {
		/** TBD what to do if LGS goes down
                 ** Hold on to the subscription if possible
                 ** to send them out if LGS comes back up
                 **/
			TRACE("LGS down");
			pthread_mutex_lock(&lga_cb.cb_lock);
			memset(&lga_cb.lgs_mds_dest, 0, sizeof(MDS_DEST));
			lga_cb.lgs_up = 0;
			pthread_mutex_unlock(&lga_cb.cb_lock);
		}
		break;
	case NCSMDS_NEW_ACTIVE:
	case NCSMDS_UP:
		switch (mds_cb_info->info.svc_evt.i_svc_id) {
		case NCSMDS_SVC_ID_LGS:
		    /** Store the MDS DEST of the LGS 
                     **/
			TRACE_2("MSG from LGS NCSMDS_NEW_ACTIVE/UP");
			pthread_mutex_lock(&lga_cb.cb_lock);
			lga_cb.lgs_mds_dest = mds_cb_info->info.svc_evt.i_dest;
			lga_cb.lgs_up = 1;
			if (lga_cb.lgs_sync_awaited) {
				/* signal waiting thread */
				m_NCS_SEL_OBJ_IND(lga_cb.lgs_sync_sel);
			}
			pthread_mutex_unlock(&lga_cb.cb_lock);
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
  Name          : lga_mds_rcv
 
  Description   : This is a callback routine that is invoked when LGA message
                  is received from LGS.
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

static uint32_t lga_mds_rcv(struct ncsmds_callback_info *mds_cb_info)
{
	lgsv_msg_t *lgsv_msg = (lgsv_msg_t *)mds_cb_info->info.receive.i_msg;
	uint32_t rc;

	pthread_mutex_lock(&lga_cb.cb_lock);

	/* process the message */
	rc = lga_lgs_msg_proc(&lga_cb, lgsv_msg, mds_cb_info->info.receive.i_priority);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_2("lga_lgs_msg_proc returned: %d", rc);
	}

	pthread_mutex_unlock(&lga_cb.cb_lock);

	return rc;
}

/****************************************************************************
  Name          : lga_mds_enc
 
  Description   : This is a callback routine that is invoked to encode LGS
                  messages.
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t lga_mds_enc(struct ncsmds_callback_info *info)
{
	lgsv_msg_t *msg;
	NCS_UBAID *uba;
	uint8_t *p8;
	uint32_t total_bytes = 0;

	MDS_CLIENT_MSG_FORMAT_VER msg_fmt_version;

	TRACE_ENTER();
	msg_fmt_version = m_NCS_ENC_MSG_FMT_GET(info->info.enc.i_rem_svc_pvt_ver,
						LGA_WRT_LGS_SUBPART_VER_AT_MIN_MSG_FMT,
						LGA_WRT_LGS_SUBPART_VER_AT_MAX_MSG_FMT, LGA_WRT_LGS_MSG_FMT_ARRAY);
	if (0 == msg_fmt_version) {
		TRACE("Wrong msg_fmt_version!!\n");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	info->info.enc.o_msg_fmt_ver = msg_fmt_version;

	msg = (lgsv_msg_t *)info->info.enc.i_msg;
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
		return NCSCC_RC_FAILURE;
	}
	ncs_encode_32bit(&p8, msg->type);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	TRACE_2("msgtype: %d", msg->type);
	if (LGSV_LGA_API_MSG == msg->type) {
	/** encode the API msg subtype **/
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE("encode API msg subtype FAILED");
			TRACE_LEAVE();	/* fixme: ok to do return fail?? */
			return NCSCC_RC_FAILURE;
		}
		ncs_encode_32bit(&p8, msg->info.api_info.type);
		ncs_enc_claim_space(uba, 4);
		total_bytes += 4;

		TRACE_2("api_info.type: %d\n", msg->info.api_info.type);
		switch (msg->info.api_info.type) {
		case LGSV_INITIALIZE_REQ:
			total_bytes += lga_enc_initialize_msg(uba, msg);
			break;

		case LGSV_FINALIZE_REQ:
			total_bytes += lga_enc_finalize_msg(uba, msg);
			break;

		case LGSV_STREAM_OPEN_REQ:
			total_bytes += lga_enc_lstr_open_sync_msg(uba, msg);
			break;

		case LGSV_STREAM_CLOSE_REQ:
			total_bytes += lga_enc_lstr_close_msg(uba, msg);
			break;

		case LGSV_WRITE_LOG_ASYNC_REQ:
			total_bytes += lga_enc_write_log_async_msg(uba, msg);
			break;

		default:
			TRACE("Unknown API type = %d", msg->info.api_info.type);
			break;
		}
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : lga_dec_initialize_rsp_msg
 
  Description   : This routine decodes an initialize sync response message
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t lga_dec_initialize_rsp_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_initialize_rsp_t *param = &msg->info.api_resp_info.param.init_rsp;
	uint8_t local_data[100];

	osafassert(uba != NULL);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->client_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	return total_bytes;
}

/****************************************************************************
  Name          : lga_dec_finalize_rsp_msg
 
  Description   : This routine decodes an finalize sync response message
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t lga_dec_finalize_rsp_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_finalize_rsp_t *param = &msg->info.api_resp_info.param.finalize_rsp;
	uint8_t local_data[100];

	osafassert(uba != NULL);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->client_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	return total_bytes;
}

/****************************************************************************
  Name          : lga_dec_lstr_close_rsp_msg
 
  Description   : This routine decodes a close response message
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t lga_dec_lstr_close_rsp_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_stream_close_rsp_t *param = &msg->info.api_resp_info.param.close_rsp;
	uint8_t local_data[100];

	osafassert(uba != NULL);

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->client_id = ncs_decode_32bit(&p8);
	param->lstr_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	return total_bytes;
}

/****************************************************************************
  Name          : lga_dec_write_ckb_msg
 
  Description   : This routine decodes an initialize sync response message
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t lga_dec_write_cbk_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_write_log_callback_ind_t *param = &msg->info.cbk_info.write_cbk;
	uint8_t local_data[100];

	osafassert(uba != NULL);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->error = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	return total_bytes;
}

/****************************************************************************
  Name          : lga_dec_lstr_open_sync_rsp_msg
 
  Description   : This routine decodes a log stream open sync response message
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t lga_dec_lstr_open_sync_rsp_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_stream_open_rsp_t *param = &msg->info.api_resp_info.param.lstr_open_rsp;
	uint8_t local_data[100];

	osafassert(uba != NULL);

	/* chan_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->lstr_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	return total_bytes;
}

/****************************************************************************
  Name          : lga_mds_dec
 
  Description   : This is a callback routine that is invoked to decode LGS
                  messages.
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t lga_mds_dec(struct ncsmds_callback_info *info)
{
	uint8_t *p8;
	lgsv_msg_t *msg;
	NCS_UBAID *uba = info->info.dec.io_uba;
	uint8_t local_data[20];
	uint32_t total_bytes = 0;
	TRACE_ENTER();

	if (0 == m_NCS_MSG_FORMAT_IS_VALID(info->info.dec.i_msg_fmt_ver,
					   LGA_WRT_LGS_SUBPART_VER_AT_MIN_MSG_FMT,
					   LGA_WRT_LGS_SUBPART_VER_AT_MAX_MSG_FMT, LGA_WRT_LGS_MSG_FMT_ARRAY)) {
		TRACE("Invalid message format!!!\n");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

    /** Allocate a new msg in both sync/async cases 
     **/
	if (NULL == (msg = calloc(1, sizeof(lgsv_msg_t)))) {
		TRACE("calloc failed\n");
		return NCSCC_RC_FAILURE;
	}

	info->info.dec.o_msg = (uint8_t *)msg;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	msg->type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	switch (msg->type) {
	case LGSV_LGA_API_RESP_MSG:
		{
			p8 = ncs_dec_flatten_space(uba, local_data, 8);
			msg->info.api_resp_info.type = ncs_decode_32bit(&p8);
			msg->info.api_resp_info.rc = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 8);
			total_bytes += 8;
			TRACE_2("LGSV_LGA_API_RESP_MSG");

			switch (msg->info.api_resp_info.type) {
			case LGSV_INITIALIZE_RSP:
				total_bytes += lga_dec_initialize_rsp_msg(uba, msg);
				break;
			case LGSV_FINALIZE_RSP:
				total_bytes += lga_dec_finalize_rsp_msg(uba, msg);
				break;
			case LGSV_STREAM_OPEN_RSP:
				total_bytes += lga_dec_lstr_open_sync_rsp_msg(uba, msg);
				break;
			case LGSV_STREAM_CLOSE_RSP:
				total_bytes += lga_dec_lstr_close_rsp_msg(uba, msg);
				break;
			default:
				TRACE_2("Unknown API RSP type %d", msg->info.api_resp_info.type);
				break;
			}
		}
		break;
	case LGSV_LGS_CBK_MSG:
		{
			p8 = ncs_dec_flatten_space(uba, local_data, 16);
			msg->info.cbk_info.type = ncs_decode_32bit(&p8);
			msg->info.cbk_info.lgs_client_id = ncs_decode_32bit(&p8);
			msg->info.cbk_info.inv = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(uba, 16);
			total_bytes += 16;
			TRACE_2("LGSV_LGS_CBK_MSG");
			switch (msg->info.cbk_info.type) {
			case LGSV_WRITE_LOG_CALLBACK_IND:
				TRACE_2("decode writelog message");
				total_bytes += lga_dec_write_cbk_msg(uba, msg);
				break;
			default:
				TRACE_2("Unknown callback type = %d!", msg->info.cbk_info.type);
				break;
			}
		}
		break;
	default:
		TRACE("Unknown MSG type %d", msg->type);
		break;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : lga_mds_enc_flat
 *
 * Description   : MDS encode and flatten
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t lga_mds_enc_flat(struct ncsmds_callback_info *info)
{
	/* Modify the MDS_INFO to populate enc */
	info->info.enc = info->info.enc_flat;
	/* Invoke the regular mds_enc routine  */
	return lga_mds_enc(info);
}

/****************************************************************************
 * Name          : lga_mds_dec_flat
 *
 * Description   : MDS decode and flatten
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t lga_mds_dec_flat(struct ncsmds_callback_info *info)
{
	uint32_t rc;

	/* Modify the MDS_INFO to populate dec */
	info->info.dec = info->info.dec_flat;
	/* Invoke the regular mds_dec routine  */
	rc = lga_mds_dec(info);
	if (rc != NCSCC_RC_SUCCESS)
		TRACE("lga_mds_dec rc = %d", rc);

	return rc;
}

/****************************************************************************
  Name          : lga_mds_cpy
 
  Description   : This function copies an events sent from LGS.
 
  Arguments     :pointer to struct ncsmds_callback_info
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t lga_mds_cpy(struct ncsmds_callback_info *info)
{
	TRACE_ENTER();
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : lga_mds_callback
 *
 * Description   : Callback Dispatcher for various MDS operations.
 *
 * Arguments     : cb   : LGS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t lga_mds_callback(struct ncsmds_callback_info *info)
{
	uint32_t rc;

	static NCSMDS_CALLBACK_API cb_set[MDS_CALLBACK_SVC_MAX] = {
		lga_mds_cpy,	/* MDS_CALLBACK_COPY      0 */
		lga_mds_enc,	/* MDS_CALLBACK_ENC       1 */
		lga_mds_dec,	/* MDS_CALLBACK_DEC       2 */
		lga_mds_enc_flat,	/* MDS_CALLBACK_ENC_FLAT  3 */
		lga_mds_dec_flat,	/* MDS_CALLBACK_DEC_FLAT  4 */
		lga_mds_rcv,	/* MDS_CALLBACK_RECEIVE   5 */
		lga_mds_svc_evt	/* MDS_CALLBACK_SVC_EVENT 6 */
	};

	if (info->i_op <= MDS_CALLBACK_SVC_EVENT) {
		rc = (*cb_set[info->i_op]) (info);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE("MDS_CALLBACK_SVC_EVENT not in range");

		return rc;
	} else
		return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : lga_mds_init
 
  Description   : This routine registers the LGA Service with MDS.
 
  Arguments     : cb - ptr to the LGA control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t lga_mds_init(lga_cb_t *cb)
{
	NCSADA_INFO ada_info;
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;
	MDS_SVC_ID svc = NCSMDS_SVC_ID_LGS;

	TRACE_ENTER();
    /** Create the ADEST for LGA and get the pwe hdl**/
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
	mds_info.i_svc_id = NCSMDS_SVC_ID_LGA;
	mds_info.i_op = MDS_INSTALL;

	mds_info.info.svc_install.i_yr_svc_hdl = 0;
	mds_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;	/* PWE scope */
	mds_info.info.svc_install.i_svc_cb = lga_mds_callback;	/* callback */
	mds_info.info.svc_install.i_mds_q_ownership = false;	/* LGA doesn't own the mds queue */
	mds_info.info.svc_install.i_mds_svc_pvt_ver = LGA_SVC_PVT_SUBPART_VERSION;

	if ((rc = ncsmds_api(&mds_info)) != NCSCC_RC_SUCCESS) {
		TRACE("mds api call failed");
		return NCSCC_RC_FAILURE;
	}

	/* Now subscribe for events that will be generated by MDS */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_LGA;
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
  Name          : lga_mds_finalize
 
  Description   : This routine unregisters the LGA Service from MDS.
 
  Arguments     : cb - ptr to the LGA control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
void lga_mds_finalize(lga_cb_t *cb)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	/* Un-install your service into MDS. 
	   No need to cancel the services that are subscribed */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_LGA;
	mds_info.i_op = MDS_UNINSTALL;

	if ((rc = ncsmds_api(&mds_info)) != NCSCC_RC_SUCCESS) {
		TRACE("mds api call failed");
	}

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : lga_mds_msg_sync_send
 
  Description   : This routine sends the LGA message to LGS. The send 
                  operation is a synchronous call that 
                  blocks until the response is received from LGS.
 
  Arguments     : cb  - ptr to the LGA CB
                  i_msg - ptr to the LGSv message
                  o_msg - double ptr to LGSv message response
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE/timeout
 
  Notes         : None.
******************************************************************************/
uint32_t lga_mds_msg_sync_send(lga_cb_t *cb, lgsv_msg_t *i_msg, lgsv_msg_t **o_msg, uint32_t timeout, uint32_t prio)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	osafassert(cb != NULL && i_msg != NULL && o_msg != NULL);

	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_LGA;
	mds_info.i_op = MDS_SEND;

	/* Fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_msg;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_LGS;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SNDRSP;
	mds_info.info.svc_send.i_priority = prio;	/* fixme? */
	/* fill the sub send rsp strcuture */
	mds_info.info.svc_send.info.sndrsp.i_time_to_wait = timeout;	/* timeto wait in 10ms FIX!!! */
	mds_info.info.svc_send.info.sndrsp.i_to_dest = cb->lgs_mds_dest;

	/* send the message */
	if (NCSCC_RC_SUCCESS == (rc = ncsmds_api(&mds_info))) {
		/* Retrieve the response and take ownership of the memory  */
		*o_msg = (lgsv_msg_t *)mds_info.info.svc_send.info.sndrsp.o_rsp;
		mds_info.info.svc_send.info.sndrsp.o_rsp = NULL;
	} else
		TRACE("lga_mds_msg_sync_send FAILED: %u", rc);

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : lga_mds_msg_async_send
 
  Description   : This routine sends the LGA message to LGS.
 
  Arguments     : cb  - ptr to the LGA CB
                  i_msg - ptr to the LGSv message
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t lga_mds_msg_async_send(lga_cb_t *cb, struct lgsv_msg *i_msg, uint32_t prio)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	osafassert(cb != NULL && i_msg != NULL);

	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_LGA;
	mds_info.i_op = MDS_SEND;

	/* fill the main send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_msg;
	mds_info.info.svc_send.i_priority = prio;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_LGS;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;

	/* fill the sub send strcuture */
	mds_info.info.svc_send.info.snd.i_to_dest = cb->lgs_mds_dest;

	/* send the message */
	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS)
		TRACE("failed");

	TRACE_LEAVE();
	return rc;
}
