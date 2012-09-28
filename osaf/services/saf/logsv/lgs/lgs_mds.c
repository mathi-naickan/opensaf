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

#include <ncsencdec_pub.h>
#include "lgs.h"

#define LGS_SVC_PVT_SUBPART_VERSION 1
#define LGS_WRT_LGA_SUBPART_VER_AT_MIN_MSG_FMT 1
#define LGS_WRT_LGA_SUBPART_VER_AT_MAX_MSG_FMT 1
#define LGS_WRT_LGA_SUBPART_VER_RANGE             \
        (LGS_WRT_LGA_SUBPART_VER_AT_MAX_MSG_FMT - \
         LGS_WRT_LGA_SUBPART_VER_AT_MIN_MSG_FMT + 1)

static MDS_CLIENT_MSG_FORMAT_VER
 LGS_WRT_LGA_MSG_FMT_ARRAY[LGS_WRT_LGA_SUBPART_VER_RANGE] = {
	1			/*msg format version for LGA subpart version 1 */
};

/****************************************************************************
 * Name          : lgs_evt_destroy
 * 
 * Description   : This is the function which is called to destroy an event.
 * 
 * Arguments     : LGSV_LGS_EVT *
 * 
 * Return Values : NONE
 * 
 * Notes         : None.
 * 
 * @param evt
 */
void lgs_evt_destroy(lgsv_lgs_evt_t *evt)
{
	osafassert(evt != NULL);
	free(evt);
}

/****************************************************************************
  Name          : dec_initialize_msg
 
  Description   : This routine decodes an initialize API msg
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t dec_initialize_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_initialize_req_t *param = &msg->info.api_info.param.init;
	uint8_t local_data[20];

	/* releaseCode, majorVersion, minorVersion */
	p8 = ncs_dec_flatten_space(uba, local_data, 3);
	param->version.releaseCode = ncs_decode_8bit(&p8);
	param->version.majorVersion = ncs_decode_8bit(&p8);
	param->version.minorVersion = ncs_decode_8bit(&p8);
	ncs_dec_skip_space(uba, 3);
	total_bytes += 3;

	TRACE_8("LGSV_INITIALIZE_REQ");
	return total_bytes;
}

/****************************************************************************
  Name          : dec_finalize_msg
 
  Description   : This routine decodes a finalize API msg
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t dec_finalize_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_finalize_req_t *param = &msg->info.api_info.param.finalize;
	uint8_t local_data[20];

	/* client_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->client_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	TRACE_8("LGSV_FINALIZE_REQ");
	return total_bytes;
}

/****************************************************************************
  Name          : dec_lstr_open_sync_msg
 
  Description   : This routine decodes a log stream open sync API msg
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t dec_lstr_open_sync_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	int len;
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_stream_open_req_t *param = &msg->info.api_info.param.lstr_open_sync;
	uint8_t local_data[256];

	/* client_id  */
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->client_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	/* log stream name length */
	p8 = ncs_dec_flatten_space(uba, local_data, 2);
	param->lstr_name.length = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, 2);
	total_bytes += 2;

	/* log stream name */
	ncs_decode_n_octets_from_uba(uba, param->lstr_name.value, (uint32_t)param->lstr_name.length);
	total_bytes += (uint32_t)param->lstr_name.length;

	/* log file name */
	p8 = ncs_dec_flatten_space(uba, local_data, 2);
	len = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, 2);
	total_bytes += 2;

	if (len > 0) {
		ncs_decode_n_octets_from_uba(uba, (uint8_t *)param->logFileName, len);
		total_bytes += len;
	}

	/* log file path name */
	p8 = ncs_dec_flatten_space(uba, local_data, 2);
	len = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, 2);
	total_bytes += 2;

	if (len > 0) {
		ncs_decode_n_octets_from_uba(uba, (uint8_t *)param->logFilePathName, len);
		total_bytes += len;
	}

	/* log record format length */
	p8 = ncs_dec_flatten_space(uba, local_data, 24);
	param->maxLogFileSize = ncs_decode_64bit(&p8);
	param->maxLogRecordSize = ncs_decode_32bit(&p8);
	param->haProperty = ncs_decode_32bit(&p8);
	param->logFileFullAction = ncs_decode_32bit(&p8);
	param->maxFilesRotated = ncs_decode_16bit(&p8);
	len = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, 24);
	total_bytes += 24;

	/* Decode format string if initiated */
	if (len > 0) {
		if ((param->logFileFmt = malloc(len)) == NULL) {
			LOG_WA("malloc FAILED");
			total_bytes = 0;
			goto done;
		}
		ncs_decode_n_octets_from_uba(uba, (uint8_t *)param->logFileFmt, len);
		total_bytes += len;
	}

	/* log stream open flags */
	p8 = ncs_dec_flatten_space(uba, local_data, 1);
	param->lstr_open_flags = ncs_decode_8bit(&p8);
	ncs_dec_skip_space(uba, 1);
	total_bytes += 1;

 done:
	TRACE_8("LGSV_STREAM_OPEN_REQ");
	return total_bytes;
}

/****************************************************************************
  Name          : dec_lstr_close_msg
 
  Description   : This routine decodes a log stream close API msg
 
  Arguments     : NCS_UBAID *uba,
                  LGSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t dec_lstr_close_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_stream_close_req_t *param = &msg->info.api_info.param.lstr_close;
	uint8_t local_data[20];

	/* client_id, lstr_id, lstr_open_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->client_id = ncs_decode_32bit(&p8);
	param->lstr_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	TRACE_8("LGSV_STREAM_CLOSE_REQ");
	return total_bytes;
}

/****************************************************************************
  Name          : dec_write_log_async_msg
 
  Description   : This routine decodes a write async log API msg
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t dec_write_log_async_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	SaNameT *logSvcUsrName;
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_write_log_async_req_t *param = &msg->info.api_info.param.write_log_async;
	uint8_t local_data[1024];

	p8 = ncs_dec_flatten_space(uba, local_data, 20);
	param->invocation = ncs_decode_64bit(&p8);
	param->ack_flags = ncs_decode_32bit(&p8);
	param->client_id = ncs_decode_32bit(&p8);
	param->lstr_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 20);
	total_bytes += 20;

	param->logRecord = malloc(sizeof(SaLogRecordT));
	if (!param->logRecord) {
		LOG_WA("malloc FAILED");
		return (0);
	}
	/* ************* SaLogRecord decode ************** */
	p8 = ncs_dec_flatten_space(uba, local_data, 12);
	param->logRecord->logTimeStamp = ncs_decode_64bit(&p8);
	param->logRecord->logHdrType = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 12);
	total_bytes += 12;

	/*Only alarm, and application log streams so far.. */
	SaLogNtfLogHeaderT *ntfLogH;
	SaLogGenericLogHeaderT *genLogH;
	ntfLogH = &param->logRecord->logHeader.ntfHdr;
	genLogH = &param->logRecord->logHeader.genericHdr;

	/*
	 ** When allocating an SaNameT, add an extra byte for null termination.
	 ** The log record formatter requires that.
	 */

	switch (param->logRecord->logHdrType) {
	case SA_LOG_NTF_HEADER:
		p8 = ncs_dec_flatten_space(uba, local_data, 14);
		ntfLogH->notificationId = ncs_decode_64bit(&p8);
		ntfLogH->eventType = ncs_decode_32bit(&p8);

		ntfLogH->notificationObject = malloc(sizeof(SaNameT) + 1);
		if (!ntfLogH->notificationObject) {
			LOG_WA("malloc FAILED");
			return (0);
		}

		ntfLogH->notificationObject->length = ncs_decode_16bit(&p8);
		if (SA_MAX_NAME_LENGTH <= ntfLogH->notificationObject->length) {
			TRACE("notificationObject to big");
			return (0);
		}
		ncs_dec_skip_space(uba, 14);
		total_bytes += 14;

		ncs_decode_n_octets_from_uba(uba,
					     ntfLogH->notificationObject->value, ntfLogH->notificationObject->length);
		total_bytes += ntfLogH->notificationObject->length;
		ntfLogH->notificationObject->value[ntfLogH->notificationObject->length] = '\0';

		ntfLogH->notifyingObject = malloc(sizeof(SaNameT) + 1);
		if (!ntfLogH->notifyingObject) {
			LOG_WA("malloc FAILED");
			return (0);
		}
		p8 = ncs_dec_flatten_space(uba, local_data, 2);
		ntfLogH->notifyingObject->length = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 2);
		total_bytes += 2;

		if (SA_MAX_NAME_LENGTH <= ntfLogH->notifyingObject->length) {
			TRACE("notifyingObject to big");
			return (0);
		}

		ncs_decode_n_octets_from_uba(uba, ntfLogH->notifyingObject->value, ntfLogH->notifyingObject->length);
		total_bytes += ntfLogH->notifyingObject->length;
		ntfLogH->notifyingObject->value[ntfLogH->notifyingObject->length] = '\0';

		ntfLogH->notificationClassId = malloc(sizeof(SaNtfClassIdT));
		if (!ntfLogH->notificationClassId) {
			LOG_WA("malloc FAILED");
			return (0);
		}
		p8 = ncs_dec_flatten_space(uba, local_data, 16);
		ntfLogH->notificationClassId->vendorId = ncs_decode_32bit(&p8);
		ntfLogH->notificationClassId->majorId = ncs_decode_16bit(&p8);
		ntfLogH->notificationClassId->minorId = ncs_decode_16bit(&p8);
		ntfLogH->eventTime = ncs_decode_64bit(&p8);
		ncs_dec_skip_space(uba, 16);
		total_bytes += 16;

		break;

	case SA_LOG_GENERIC_HEADER:
		genLogH->notificationClassId = malloc(sizeof(SaNtfClassIdT));
		if (!genLogH->notificationClassId) {
			LOG_WA("malloc FAILED");
			return (0);
		}
		p8 = ncs_dec_flatten_space(uba, local_data, 10);
		genLogH->notificationClassId->vendorId = ncs_decode_32bit(&p8);
		genLogH->notificationClassId->majorId = ncs_decode_16bit(&p8);
		genLogH->notificationClassId->minorId = ncs_decode_16bit(&p8);

		logSvcUsrName = malloc(sizeof(SaNameT) + 1);
		if (!logSvcUsrName) {
			LOG_WA("malloc FAILED");
			return (0);
		}

		/*
		 ** A const value in genLogHeader is fucking up...
		 ** Extra instance used.
		 */
		logSvcUsrName->length = ncs_decode_16bit(&p8);

		if (SA_MAX_NAME_LENGTH <= logSvcUsrName->length) {
			LOG_ER("logSvcUsrName too big");
			free(logSvcUsrName);
			return (0);
		}
		ncs_dec_skip_space(uba, 10);
		total_bytes += 10;

		ncs_decode_n_octets_from_uba(uba, logSvcUsrName->value, logSvcUsrName->length);
		total_bytes += logSvcUsrName->length;
		logSvcUsrName->value[logSvcUsrName->length] = '\0';
		genLogH->logSvcUsrName = logSvcUsrName;
		p8 = ncs_dec_flatten_space(uba, local_data, 2);
		genLogH->logSeverity = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 2);
		total_bytes += 2;
		break;

	default:
		TRACE("ERROR IN logHdrType in logRecord");
		break;
	}
	param->logRecord->logBuffer = malloc(sizeof(SaLogBufferT));
	if (!param->logRecord->logBuffer) {
		LOG_WA("malloc FAILED");
		return (0);	/* FIX no error handling! */
	}
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->logRecord->logBuffer->logBufSize = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	/* Make sure at least one byte is allocated for later */
	param->logRecord->logBuffer->logBuf = calloc(1, param->logRecord->logBuffer->logBufSize + 1);
	if (param->logRecord->logBuffer->logBuf == NULL) {
		LOG_WA("malloc FAILED");
		return (0);	/* FIX no error handling! */
	}
	if (param->logRecord->logBuffer->logBufSize > 0) {
		ncs_decode_n_octets_from_uba(uba,
					     param->logRecord->logBuffer->logBuf,
					     (uint32_t)param->logRecord->logBuffer->logBufSize);
		total_bytes += (uint32_t)param->logRecord->logBuffer->logBufSize;
	}

    /************ end saLogRecord decode ****************/
	TRACE_8("LGSV_WRITE_LOG_ASYNC_REQ");
	return total_bytes;
}

/****************************************************************************
  Name          : enc_initialize_rsp_msg
 
  Description   : This routine encodes an initialize resp msg
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t enc_initialize_rsp_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_initialize_rsp_t *param = &msg->info.api_resp_info.param.init_rsp;

	/* client_id */
	p8 = ncs_enc_reserve_space(uba, 4);
	if (p8 == NULL) {
		TRACE("ncs_enc_reserve_space failed");
		goto done;
	}

	ncs_encode_32bit(&p8, param->client_id);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

 done:
	TRACE_8("LGSV_INITIALIZE_RSP");
	return total_bytes;
}

/****************************************************************************
  Name          : enc_finalize_rsp_msg
 
  Description   : This routine encodes a finalize resp msg
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t enc_finalize_rsp_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_finalize_rsp_t *param = &msg->info.api_resp_info.param.finalize_rsp;

	/* client_id */
	p8 = ncs_enc_reserve_space(uba, 4);
	if (p8 == NULL) {
		TRACE("ncs_enc_reserve_space failed");
		goto done;
	}

	ncs_encode_32bit(&p8, param->client_id);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

 done:
	TRACE_8("LGSV_FINALIZE_RSP");
	return total_bytes;
}

/****************************************************************************
  Name          : enc_lstr_close_rsp_msg
 
  Description   : This routine encodes a close resp msg
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t enc_lstr_close_rsp_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_stream_close_rsp_t *param = &msg->info.api_resp_info.param.close_rsp;

	/* client_id */
	p8 = ncs_enc_reserve_space(uba, 8);
	if (p8 == NULL) {
		TRACE("ncs_enc_reserve_space failed");
		goto done;
	}

	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_32bit(&p8, param->lstr_id);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

 done:
	TRACE_8("LGSV_CLOSE_RSP");
	return total_bytes;
}

/****************************************************************************
  Name          : enc_lstr_open_rsp_msg
 
  Description   : This routine decodes a log stream open sync resp msg
 
  Arguments     : NCS_UBAID *msg,
                  LGSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t enc_lstr_open_rsp_msg(NCS_UBAID *uba, lgsv_msg_t *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	lgsv_stream_open_rsp_t *param = &msg->info.api_resp_info.param.lstr_open_rsp;

	/* lstr_id */
	p8 = ncs_enc_reserve_space(uba, 8);
	if (p8 == NULL) {
		TRACE("ncs_enc_reserve_space failed");
		goto done;
	}
	ncs_encode_32bit(&p8, param->lstr_id);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

 done:
	TRACE_8("LGSV_STREAM_OPEN_RSP");
	return total_bytes;
}

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

static uint32_t mds_enc(struct ncsmds_callback_info *info)
{
	lgsv_msg_t *msg;
	NCS_UBAID *uba;
	uint8_t *p8;
	uint32_t total_bytes = 0;
	MDS_CLIENT_MSG_FORMAT_VER msg_fmt_version;

	msg_fmt_version = m_NCS_ENC_MSG_FMT_GET(info->info.enc.i_rem_svc_pvt_ver,
						LGS_WRT_LGA_SUBPART_VER_AT_MIN_MSG_FMT,
						LGS_WRT_LGA_SUBPART_VER_AT_MAX_MSG_FMT, LGS_WRT_LGA_MSG_FMT_ARRAY);
	if (0 == msg_fmt_version) {
		LOG_ER("msg_fmt_version FAILED!");
		return NCSCC_RC_FAILURE;
	}
	info->info.enc.o_msg_fmt_ver = msg_fmt_version;

	msg = (lgsv_msg_t *)info->info.enc.i_msg;
	uba = info->info.enc.io_uba;

	if (uba == NULL) {
		LOG_ER("uba == NULL");
		goto err;
	}

    /** encode the type of message **/
	p8 = ncs_enc_reserve_space(uba, 4);
	if (p8 == NULL) {
		TRACE("ncs_enc_reserve_space failed");
		goto err;
	}
	ncs_encode_32bit(&p8, msg->type);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	if (LGSV_LGA_API_RESP_MSG == msg->type) {
	/** encode the API RSP msg subtype **/
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			goto err;
		}
		ncs_encode_32bit(&p8, msg->info.api_resp_info.type);
		ncs_enc_claim_space(uba, 4);
		total_bytes += 4;

		/* rc */
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			goto err;
		}
		ncs_encode_32bit(&p8, msg->info.api_resp_info.rc);
		ncs_enc_claim_space(uba, 4);
		total_bytes += 4;

		switch (msg->info.api_resp_info.type) {
		case LGSV_INITIALIZE_RSP:
			total_bytes += enc_initialize_rsp_msg(uba, msg);
			break;
		case LGSV_FINALIZE_RSP:
			total_bytes += enc_finalize_rsp_msg(uba, msg);
			break;
		case LGSV_STREAM_OPEN_RSP:
			total_bytes += enc_lstr_open_rsp_msg(uba, msg);
			break;
		case LGSV_STREAM_CLOSE_RSP:
			total_bytes += enc_lstr_close_rsp_msg(uba, msg);
			break;
		default:
			TRACE("Unknown API RSP type = %d", msg->info.api_resp_info.type);
			break;
		}
	} else if (LGSV_LGS_CBK_MSG == msg->type) {
	/** encode the API RSP msg subtype **/
		p8 = ncs_enc_reserve_space(uba, 16);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			goto err;
		}
		ncs_encode_32bit(&p8, msg->info.cbk_info.type);
		ncs_encode_32bit(&p8, msg->info.cbk_info.lgs_client_id);
		ncs_encode_64bit(&p8, msg->info.cbk_info.inv);
		ncs_enc_claim_space(uba, 16);
		total_bytes += 16;
		if (msg->info.cbk_info.type == LGSV_WRITE_LOG_CALLBACK_IND) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8) {
				TRACE("ncs_enc_reserve_space failed");
				goto err;
			}
			ncs_encode_32bit(&p8, msg->info.cbk_info.write_cbk.error);
			total_bytes += 4;
		} else {
			TRACE("unknown callback type %d", msg->info.cbk_info.type);
			goto err;
		}
		TRACE_8("LGSV_WRITE_LOG_CALLBACK_IND");
	} else {
		TRACE("unknown msg type %d", msg->type);
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
	uint8_t *p8;
	lgsv_lgs_evt_t *evt;
	NCS_UBAID *uba = info->info.dec.io_uba;
	uint8_t local_data[20];
	uint32_t total_bytes = 0;

	if (0 == m_NCS_MSG_FORMAT_IS_VALID(info->info.dec.i_msg_fmt_ver,
					   LGS_WRT_LGA_SUBPART_VER_AT_MIN_MSG_FMT,
					   LGS_WRT_LGA_SUBPART_VER_AT_MAX_MSG_FMT, LGS_WRT_LGA_MSG_FMT_ARRAY)) {
		TRACE("Wrong format version");
		goto err;
	}

    /** allocate an LGSV_LGS_EVENT now **/
	if (NULL == (evt = calloc(1, sizeof(lgsv_lgs_evt_t)))) {
		LOG_WA("calloc FAILED");
		goto err;
	}

	/* Assign the allocated event */
	info->info.dec.o_msg = (uint8_t *)evt;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	evt->info.msg.type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	if (LGSV_LGA_API_MSG == evt->info.msg.type) {
		p8 = ncs_dec_flatten_space(uba, local_data, 4);
		evt->info.msg.info.api_info.type = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(uba, 4);
		total_bytes += 4;

		/* FIX error handling for dec functions */
		switch (evt->info.msg.info.api_info.type) {
		case LGSV_INITIALIZE_REQ:
			total_bytes += dec_initialize_msg(uba, &evt->info.msg);
			break;
		case LGSV_FINALIZE_REQ:
			total_bytes += dec_finalize_msg(uba, &evt->info.msg);
			break;
		case LGSV_STREAM_OPEN_REQ:
			total_bytes += dec_lstr_open_sync_msg(uba, &evt->info.msg);
			break;
		case LGSV_STREAM_CLOSE_REQ:
			total_bytes += dec_lstr_close_msg(uba, &evt->info.msg);
			break;
		case LGSV_WRITE_LOG_ASYNC_REQ:
			total_bytes += dec_write_log_async_msg(uba, &evt->info.msg);
			break;
		default:
			TRACE("Unknown API type = %d", evt->info.msg.info.api_info.type);
			break;
		}
		if (total_bytes == 4)
			goto err;
	} else {
		TRACE("unknown msg type = %d", (int)evt->info.msg.type);
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

/**
 * Return mailbox priority depending on message type and stream
 * ID. Finalize and Close always gets same prio as writes so
 * that all writes will be flushed before the stream is closed
 * and Finalized. Writes to the system stream are prioritized
 * before application streams.
 * 
 * @param api_info
 * 
 * @return NCS_IPC_PRIORITY
 */
static NCS_IPC_PRIORITY getmboxprio(const lgsv_api_info_t *api_info)
{
	uint32_t str_id;

	if (api_info->type == LGSV_FINALIZE_REQ) {
		/* Queue at lowest prio so no writes get bypassed.
		 * No access to a stream ID here so use lowest possible prio. */
		return LGS_IPC_PRIO_APP_STREAM;
	}
	else if (api_info->type == LGSV_STREAM_CLOSE_REQ)
		str_id = api_info->param.lstr_close.lstr_id;
	else {
		osafassert(api_info->type == LGSV_WRITE_LOG_ASYNC_REQ);
		str_id = api_info->param.write_log_async.lstr_id;
	}

	/* Set prio depending on stream type */
	if (str_id == 0)
		return LGS_IPC_PRIO_ALARM_STREAM;
	else if (str_id <= 2)
		return LGS_IPC_PRIO_SYS_STREAM;
	else
		return LGS_IPC_PRIO_APP_STREAM;
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
	lgsv_lgs_evt_t *evt = (lgsv_lgs_evt_t *)mds_info->info.receive.i_msg;
	const lgsv_api_info_t *api_info = &evt->info.msg.info.api_info;
	lgsv_api_msg_type_t type = api_info->type;
	NCS_IPC_PRIORITY prio = NCS_IPC_PRIORITY_LOW;
	uint32_t rc;
	static unsigned long silently_discarded[NCS_IPC_PRIORITY_MAX];

	evt->evt_type = LGSV_LGS_LGSV_MSG;
	evt->cb_hdl = (uint32_t)mds_info->i_yr_svc_hdl;
	evt->fr_node_id = mds_info->info.receive.i_node_id;
	evt->fr_dest = mds_info->info.receive.i_fr_dest;
	evt->rcvd_prio = mds_info->info.receive.i_priority;
	evt->mds_ctxt = mds_info->info.receive.i_msg_ctxt;

	// for all msg types but WRITEs, sample curr time and store in msg
	if ((type == LGSV_INITIALIZE_REQ) || (type == LGSV_STREAM_OPEN_REQ)) {
		osafassert(clock_gettime(CLOCK_MONOTONIC, &evt->entered_at) == 0);
		rc = m_NCS_IPC_SEND(&lgs_mbx, evt, LGS_IPC_PRIO_CTRL_MSGS);
		osafassert(rc == NCSCC_RC_SUCCESS);
		return NCSCC_RC_SUCCESS;
	}

	prio = getmboxprio(api_info);

	if ((type == LGSV_FINALIZE_REQ) || (type == LGSV_STREAM_CLOSE_REQ)) {
		osafassert(clock_gettime(CLOCK_MONOTONIC, &evt->entered_at) == 0);
		rc = m_NCS_IPC_SEND(&lgs_mbx, evt, prio);
		if (rc != NCSCC_RC_SUCCESS) {
			/* Bump prio and try again, should succeed! */
			rc = m_NCS_IPC_SEND(&lgs_mbx, evt, LGS_IPC_PRIO_CTRL_MSGS);
			osafassert(rc == NCSCC_RC_SUCCESS);
		}
		return NCSCC_RC_SUCCESS;
	}

	/* Can we leave the mbox FULL state? */
	if (mbox_full[prio] && (mbox_msgs[prio] <= mbox_low[prio])) {
		mbox_full[prio] = false;
		LOG_NO("discarded %lu writes, stream type: %s", silently_discarded[prio],
			   (prio == LGS_IPC_PRIO_APP_STREAM) ? "app" : "sys/not");
		silently_discarded[prio] = 0;
	}

	/* If the mailbox is full, nack or silently drop */
	if (mbox_full[prio]) {
		/* If logger has requested an ack, send one with error code TRYAGAIN */
		if (api_info->param.write_log_async.ack_flags & SA_LOG_RECORD_WRITE_ACK) {
			lgs_send_write_log_ack(api_info->param.write_log_async.client_id,
								   api_info->param.write_log_async.invocation,
								   SA_AIS_ERR_TRY_AGAIN,
								   evt->fr_dest);
		} else
			silently_discarded[prio]++;

		goto donefree;
	}

	/* Can only get here for writes */
	osafassert(api_info->type == LGSV_WRITE_LOG_ASYNC_REQ);

	if (m_NCS_IPC_SEND(&lgs_mbx, evt, prio) == NCSCC_RC_SUCCESS) {
		return NCSCC_RC_SUCCESS;
	} else {
		mbox_full[prio] = true;
		TRACE("FULL, msgs: %u, low: %u, high: %u", mbox_msgs[prio],
			  mbox_low[prio], mbox_high[prio]);

		/* If logger has requested an ack, send one with error code TRYAGAIN */
		if (api_info->param.write_log_async.ack_flags & SA_LOG_RECORD_WRITE_ACK) {
			lgs_send_write_log_ack(api_info->param.write_log_async.client_id,
								   api_info->param.write_log_async.invocation,
								   SA_AIS_ERR_TRY_AGAIN,
								   evt->fr_dest);
		} else
			silently_discarded[prio]++;
	}

donefree:
	lgs_free_write_log(&api_info->param.write_log_async);
	free(evt);
	return NCSCC_RC_SUCCESS;
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
	lgsv_lgs_evt_t *lgsv_evt;

	TRACE_ENTER();

	if (NULL == (lgsv_evt = calloc(1, sizeof(lgsv_lgs_evt_t)))) {
		LOG_WA("calloc FAILED");
		goto err;
	}

	if (lgs_cb->is_quiesced_set == true) {
		/** Initialize the Event here **/
		lgsv_evt->evt_type = LGSV_EVT_QUIESCED_ACK;
		lgsv_evt->cb_hdl = (uint32_t)mds_info->i_yr_svc_hdl;

		/* Push the event and we are done */
		if (NCSCC_RC_FAILURE == m_NCS_IPC_SEND(&lgs_mbx, lgsv_evt, LGS_IPC_PRIO_CTRL_MSGS)) {
			TRACE("ipc send failed");
			lgs_evt_destroy(lgsv_evt);
			goto err;
		}
	} else {
		lgs_evt_destroy(lgsv_evt);
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
	lgsv_lgs_evt_t *evt = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* First make sure that this event is indeed for us */
	if (info->info.svc_evt.i_your_id != NCSMDS_SVC_ID_LGS) {
		TRACE("event not NCSMDS_SVC_ID_LGS");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* If this evt was sent from LGA act on this */
	if (info->info.svc_evt.i_svc_id == NCSMDS_SVC_ID_LGA) {
		if (info->info.svc_evt.i_change == NCSMDS_DOWN) {
			TRACE_8("MDS DOWN dest: %" PRIx64 ", node ID: %x, svc_id: %d",
				info->info.svc_evt.i_dest, info->info.svc_evt.i_node_id, info->info.svc_evt.i_svc_id);

			/* As of now we are only interested in LGA events */
			if (NULL == (evt = calloc(1, sizeof(lgsv_lgs_evt_t)))) {
				LOG_WA("calloc FAILED");
				rc = NCSCC_RC_FAILURE;
				goto done;
			}

			evt->evt_type = LGSV_LGS_EVT_LGA_DOWN;

			/** Initialize the Event Header **/
			evt->cb_hdl = 0;
			evt->fr_node_id = info->info.svc_evt.i_node_id;
			evt->fr_dest = info->info.svc_evt.i_dest;

			/** Initialize the MDS portion of the header **/
			evt->info.mds_info.node_id = info->info.svc_evt.i_node_id;
			evt->info.mds_info.mds_dest_id = info->info.svc_evt.i_dest;

			/* Push to the lowest prio queue to not bypass any pending writes. If that fails
			 * (it is FULL) use the high prio unbounded ctrl msg queue */
			if (m_NCS_IPC_SEND(&lgs_mbx, evt, LGS_IPC_PRIO_APP_STREAM) != NCSCC_RC_SUCCESS) {
				rc = m_NCS_IPC_SEND(&lgs_mbx, evt, LGS_IPC_PRIO_CTRL_MSGS);
				osafassert(rc == NCSCC_RC_SUCCESS);
			}
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
 * Arguments     : cb   : LGS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t mds_callback(struct ncsmds_callback_info *info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
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
	} else {
		TRACE("mds callback out of range");
		rc = NCSCC_RC_FAILURE;
	}

	return rc;
}

/****************************************************************************
 * Name          : mds_vdest_create
 *
 * Description   : This function created the VDEST for LGS
 *
 * Arguments     : 
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mds_vdest_create(lgs_cb_t *lgs_cb)
{
	NCSVDA_INFO vda_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	memset(&vda_info, 0, sizeof(NCSVDA_INFO));
	lgs_cb->vaddr = LGS_VDEST_ID;
	vda_info.req = NCSVDA_VDEST_CREATE;
	vda_info.info.vdest_create.i_create_type = NCSVDA_VDEST_CREATE_SPECIFIC;
	vda_info.info.vdest_create.i_persistent = false;	/* Up-to-the application */
	vda_info.info.vdest_create.i_policy = NCS_VDEST_TYPE_DEFAULT;
	vda_info.info.vdest_create.info.specified.i_vdest = lgs_cb->vaddr;

	/* Create the VDEST address */
	if (NCSCC_RC_SUCCESS != (rc = ncsvda_api(&vda_info))) {
		LOG_ER("VDEST_CREATE_FAILED");
		return rc;
	}

	/* Store the info returned by MDS */
	lgs_cb->mds_hdl = vda_info.info.vdest_create.o_mds_pwe1_hdl;

	return rc;
}

/****************************************************************************
 * Name          : lgs_mds_init
 *
 * Description   : This function creates the VDEST for lgs and installs/suscribes
 *                 into MDS.
 *
 * Arguments     : cb   : LGS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t lgs_mds_init(lgs_cb_t *cb)
{
	NCSMDS_INFO mds_info;
	uint32_t rc;
	MDS_SVC_ID svc = NCSMDS_SVC_ID_LGA;

	TRACE_ENTER();

	/* Create the VDEST for LGS */
	if (NCSCC_RC_SUCCESS != (rc = mds_vdest_create(cb))) {
		LOG_ER(" lgs_mds_init: named vdest create FAILED\n");
		return rc;
	}

	/* Set the role of MDS */
	if (cb->ha_state == SA_AMF_HA_ACTIVE)
		cb->mds_role = V_DEST_RL_ACTIVE;

	if (cb->ha_state == SA_AMF_HA_STANDBY)
		cb->mds_role = V_DEST_RL_STANDBY;

	if (NCSCC_RC_SUCCESS != (rc = lgs_mds_change_role(cb))) {
		LOG_ER("MDS role change to %d FAILED\n", cb->mds_role);
		return rc;
	}

	/* Install your service into MDS */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_LGS;
	mds_info.i_op = MDS_INSTALL;
	mds_info.info.svc_install.i_yr_svc_hdl = 0;
	mds_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;
	mds_info.info.svc_install.i_svc_cb = mds_callback;
	mds_info.info.svc_install.i_mds_q_ownership = false;
	mds_info.info.svc_install.i_mds_svc_pvt_ver = LGS_SVC_PVT_SUBPART_VERSION;

	if (NCSCC_RC_SUCCESS != (rc = ncsmds_api(&mds_info))) {
		LOG_ER("MDS Install FAILED");
		return rc;
	}

	/* Now subscribe for LGS events in MDS, TODO: WHy this? */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_LGS;
	mds_info.i_op = MDS_SUBSCRIBE;
	mds_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	mds_info.info.svc_subscribe.i_num_svcs = 1;
	mds_info.info.svc_subscribe.i_svc_ids = &svc;

	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("MDS subscribe FAILED");
		return rc;
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : lgs_mds_change_role
 *
 * Description   : This function informs mds of our vdest role change 
 *
 * Arguments     : cb   : LGS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t lgs_mds_change_role(lgs_cb_t *cb)
{
	NCSVDA_INFO arg;

	memset(&arg, 0, sizeof(NCSVDA_INFO));

	arg.req = NCSVDA_VDEST_CHG_ROLE;
	arg.info.vdest_chg_role.i_vdest = cb->vaddr;
	arg.info.vdest_chg_role.i_new_role = cb->mds_role;

	return ncsvda_api(&arg);
}

/****************************************************************************
 * Name          : mds_vdest_destroy
 *
 * Description   : This function Destroys the Virtual destination of LGS
 *
 * Arguments     : cb   : LGS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mds_vdest_destroy(lgs_cb_t *lgs_cb)
{
	NCSVDA_INFO vda_info;
	uint32_t rc;

	memset(&vda_info, 0, sizeof(NCSVDA_INFO));
	vda_info.req = NCSVDA_VDEST_DESTROY;
	vda_info.info.vdest_destroy.i_vdest = lgs_cb->vaddr;

	if (NCSCC_RC_SUCCESS != (rc = ncsvda_api(&vda_info))) {
		LOG_ER("NCSVDA_VDEST_DESTROY failed");
		return rc;
	}

	return rc;
}

/****************************************************************************
 * Name          : lgs_mds_finalize
 *
 * Description   : This function un-registers the LGS Service with MDS.
 *
 * Arguments     : Uninstall LGS from MDS.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t lgs_mds_finalize(lgs_cb_t *cb)
{
	NCSMDS_INFO mds_info;
	uint32_t rc;

	/* Un-install LGS service from MDS */
	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_LGS;
	mds_info.i_op = MDS_UNINSTALL;

	rc = ncsmds_api(&mds_info);

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("MDS_UNINSTALL_FAILED");
		return rc;
	}

	/* Destroy the virtual Destination of LGS */
	rc = mds_vdest_destroy(cb);
	return rc;
}

/****************************************************************************
  Name          : lgs_mds_msg_send
 
  Description   : This routine sends a message to LGA. The send 
                  operation may be a 'normal' send or a synchronous call that 
                  blocks until the response is received from LGA.
 
  Arguments     : cb  - ptr to the LGA CB
                  i_msg - ptr to the LGSv message
                  dest  - MDS destination to send to.
                  mds_ctxt - ctxt for synch mds req-resp.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE/timeout
 
  Notes         : None.
******************************************************************************/

uint32_t lgs_mds_msg_send(lgs_cb_t *cb,
		       lgsv_msg_t *msg, MDS_DEST *dest, MDS_SYNC_SND_CTXT *mds_ctxt, MDS_SEND_PRIORITY_TYPE prio)
{
	NCSMDS_INFO mds_info;
	MDS_SEND_INFO *send_info = &mds_info.info.svc_send;
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* populate the mds params */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_LGS;
	mds_info.i_op = MDS_SEND;

	send_info->i_msg = msg;
	send_info->i_to_svc = NCSMDS_SVC_ID_LGA;
	send_info->i_priority = prio;	/* Discuss the priority assignments TBD */

	if (NULL == mds_ctxt || 0 == mds_ctxt->length) {
		/* regular send */
		MDS_SENDTYPE_SND_INFO *send = &send_info->info.snd;

		send_info->i_sendtype = MDS_SENDTYPE_SND;
		send->i_to_dest = *dest;
	} else {
		/* response message (somebody is waiting for it) */
		MDS_SENDTYPE_RSP_INFO *resp = &send_info->info.rsp;

		send_info->i_sendtype = MDS_SENDTYPE_RSP;
		resp->i_sender_dest = *dest;
		resp->i_msg_ctxt = *mds_ctxt;
	}

	/* send the message */
	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_WA("FAILED to send msg to %" PRIx64, *dest);
	}
	return rc;
}
