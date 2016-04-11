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
#include "ntfs.h"
#include "ntfsv_enc_dec.h"

#define NTFS_SVC_PVT_SUBPART_VERSION 1
#define NTFS_WRT_NTFA_SUBPART_VER_AT_MIN_MSG_FMT 1
#define NTFS_WRT_NTFA_SUBPART_VER_AT_MAX_MSG_FMT 1
#define NTFS_WRT_NTFA_SUBPART_VER_RANGE             \
        (NTFS_WRT_NTFA_SUBPART_VER_AT_MAX_MSG_FMT - \
         NTFS_WRT_NTFA_SUBPART_VER_AT_MIN_MSG_FMT + 1)

static MDS_CLIENT_MSG_FORMAT_VER
 NTFS_WRT_NTFA_MSG_FMT_ARRAY[NTFS_WRT_NTFA_SUBPART_VER_RANGE] = {
	1			/*msg format version for NTFA subpart version 1 */
};

/****************************************************************************
 * Name          : ntfs_evt_destroy
 * 
 * Description   : This is the function which is called to destroy an event.
 * 
 * Arguments     : NTFSV_NTFS_EVT *
 * 
 * Return Values : NONE
 * 
 * Notes         : None.
 * 
 * @param evt
 */
void ntfs_evt_destroy(ntfsv_ntfs_evt_t *evt)
{
	osafassert(evt != NULL);
	free(evt);
}

/****************************************************************************
  Name          : dec_initialize_msg
 
  Description   : This routine decodes an initialize API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t dec_initialize_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_initialize_req_t *param = &msg->info.api_info.param.init;
	uint8_t local_data[3];

	/* releaseCode, majorVersion, minorVersion */
	p8 = ncs_dec_flatten_space(uba, local_data, 3);
	param->version.releaseCode = ncs_decode_8bit(&p8);
	param->version.majorVersion = ncs_decode_8bit(&p8);
	param->version.minorVersion = ncs_decode_8bit(&p8);
	ncs_dec_skip_space(uba, 3);

	TRACE_8("NTFSV_INITIALIZE_REQ");
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : dec_finalize_msg
 
  Description   : This routine decodes a finalize API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t dec_finalize_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_finalize_req_t *param = &msg->info.api_info.param.finalize;
	uint8_t local_data[4];

	/* client_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->client_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

	TRACE_8("NTFSV_FINALIZE_REQ");
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : dec_subscribe_msg
 
  Description   : This routine decodes a ntf subscribe  API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t dec_subscribe_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	ntfsv_subscribe_req_t *param = &msg->info.api_info.param.subscribe;
	return ntfsv_dec_subscribe_msg(uba, param);

}

/****************************************************************************
  Name          : dec_unsubscribe_msg
 
  Description   : This routine decodes a ntf unsubscribe API msg
 
  Arguments     : NCS_UBAID *uba,
                  NTFSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t dec_unsubscribe_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	ntfsv_unsubscribe_req_t *param = &msg->info.api_info.param.unsubscribe;
	return ntfsv_dec_unsubscribe_msg(uba, param);
}

/****************************************************************************
  Name          : dec_write_ntf_async_msg
 
  Description   : This routine decodes a notificaton API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t dec_send_not_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	ntfsv_send_not_req_t *param;
	/* deallocated in NtfNotification class */
	param = calloc(1, sizeof(ntfsv_send_not_req_t));
	if (param == NULL) {
		return SA_AIS_ERR_NO_MEMORY;
	}
	msg->info.api_info.param.send_notification = param;
	return ntfsv_dec_not_msg(uba, param);
}

/****************************************************************************
  Name          : dec_reader_initialize_msg
 
  Description   : This routine decodes an reader_initialize API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t dec_reader_initialize_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	return ntfsv_dec_reader_initialize_msg(uba, msg);
}

/****************************************************************************
  Name          : dec_reader_initialize_msg_2
 
  Description   : This routine decodes an reader_initialize API msg
                  filter is added in reader_initialize_msg_2
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t dec_reader_initialize_msg_2(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	return ntfsv_dec_reader_initialize_msg_2(uba, msg);
}

/****************************************************************************
  Name          : dec_reader_finalize_msg
 
  Description   : This routine decodes an reader_finalize API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t dec_reader_finalize_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_reader_finalize_req_t *param = &msg->info.api_info.param.reader_finalize;
	uint8_t local_data[8];

	/* releaseCode, majorVersion, minorVersion */
	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->client_id = ncs_decode_32bit(&p8);
	param->readerId = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 8);

	TRACE_8("NTFSV_reader_finalize_REQ");
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : dec_read_next_msg
 
  Description   : This routine decodes an read_next API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t dec_read_next_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_read_next_req_t *param = &msg->info.api_info.param.read_next;
	uint8_t local_data[10];

	/* releaseCode, majorVersion, minorVersion */
	p8 = ncs_dec_flatten_space(uba, local_data, 10);
	param->client_id = ncs_decode_32bit(&p8);
	param->searchDirection = ncs_decode_8bit(&p8);
	param->readerId = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 10);

	TRACE_8("NTFSV_read_next_REQ");
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : enc_initialize_rsp_msg
 
  Description   : This routine encodes an initialize resp msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t enc_initialize_rsp_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_initialize_rsp_t *param = &msg->info.api_resp_info.param.init_rsp;
	uint32_t rc = NCSCC_RC_SUCCESS;
	/* client_id */
	p8 = ncs_enc_reserve_space(uba, 4);
	if (p8 == NULL) {
		TRACE("ncs_enc_reserve_space failed");
		rc = NCSCC_RC_OUT_OF_MEM;
		goto done;
	}

	ncs_encode_32bit(&p8, param->client_id);
	ncs_enc_claim_space(uba, 4);

 done:
	TRACE_8("NTFSV_INITIALIZE_RSP");
	return rc;
}

/****************************************************************************
  Name          : enc_subscribe_rsp_msg
 
  Description   : This routine decodes a ntf subscribe msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t enc_subscribe_rsp_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_subscribe_rsp_t *param = &msg->info.api_resp_info.param.subscribe_rsp;
	uint32_t rc = NCSCC_RC_SUCCESS;
	
	/* lstr_id */
	p8 = ncs_enc_reserve_space(uba, 8);
	if (p8 == NULL) {
		TRACE("ncs_enc_reserve_space failed");
		rc = NCSCC_RC_OUT_OF_MEM;
		goto done;
	}
	ncs_encode_32bit(&p8, param->subscriptionId);
	ncs_enc_claim_space(uba, 4);

 done:
	TRACE_8("NTFSV_SUBSCRIBE_RSP");
	return rc;
}

/****************************************************************************
  Name          : enc_send_not_rsp_msg
 
  Description   : This routine decodes a ntf notification resp msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t enc_send_not_rsp_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_send_not_rsp_t *param = &msg->info.api_resp_info.param.send_not_rsp;
	uint32_t rc = NCSCC_RC_SUCCESS;
	/* lstr_id */
	p8 = ncs_enc_reserve_space(uba, 8);
	if (p8 == NULL) {
		TRACE("ncs_enc_reserve_space failed");
		rc = NCSCC_RC_OUT_OF_MEM;
		goto done;
	}
	ncs_encode_64bit(&p8, param->notificationId);
	ncs_enc_claim_space(uba, 8);

 done:
	TRACE_8("NTFSV_SEND_NOT_RSP");
	return rc;
}

/****************************************************************************
  Name          : enc_send_not_cbk_msg
 
  Description   : This routine encodes a notification callback API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t enc_send_not_cbk_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	ntfsv_send_not_req_t *param = msg->info.cbk_info.param.notification_cbk;
	return ntfsv_enc_not_msg(uba, param);
}

/****************************************************************************
  Name          : enc_send_discard_cbk_msg
 
  Description   : This routine encodes discarded callback API msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t enc_send_discard_cbk_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	ntfsv_discarded_info_t *param = &msg->info.cbk_info.param.discarded_cbk;
	return ntfsv_enc_discard_msg(uba, param);
}

/****************************************************************************
  Name          : enc_reader_initialize_rsp_msg
 
  Description   : This routine encodes an reader_initialize resp msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t enc_reader_initialize_rsp_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_reader_init_rsp_t *param = &msg->info.api_resp_info.param.reader_init_rsp;
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* client_id */
	p8 = ncs_enc_reserve_space(uba, 4);
	if (p8 == NULL) {
		TRACE("ncs_enc_reserve_space failed");
		rc = NCSCC_RC_OUT_OF_MEM;
		goto done;
	}

	ncs_encode_32bit(&p8, param->readerId);
	ncs_enc_claim_space(uba, 4);

 done:
	TRACE_8("NTFSV_reader_initialize_RSP");
	return rc;
}

/****************************************************************************
  Name          : enc_reader_finalize_rsp_msg
 
  Description   : This routine encodes an reader_finalize resp msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t enc_reader_finalize_rsp_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_reader_finalize_rsp_t *param = &msg->info.api_resp_info.param.reader_finalize_rsp;
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* client_id */
	p8 = ncs_enc_reserve_space(uba, 4);
	if (p8 == NULL) {
		TRACE("ncs_enc_reserve_space failed");
		rc = NCSCC_RC_OUT_OF_MEM;
		goto done;
	}

	ncs_encode_32bit(&p8, param->reader_id);
	ncs_enc_claim_space(uba, 4);

 done:
	TRACE_8("NTFSV_reader_finalize_RSP");
	return rc;
}

/****************************************************************************
  Name          : enc_read_next_rsp_msg
 
  Description   : This routine encodes an read_next resp msg
 
  Arguments     : NCS_UBAID *msg,
                  NTFSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t enc_read_next_rsp_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	ntfsv_read_next_rsp_t *param = &msg->info.api_resp_info.param.read_next_rsp;
	TRACE_8("NTFSV_read_next_RSP");
	if (msg->info.api_resp_info.rc != SA_AIS_OK) {
		return NCSCC_RC_SUCCESS;
	}
	return ntfsv_enc_not_msg(uba, param->readNotification);
}

/**
 * @brief Encodes CLM node status callback msg. 
 *         
 * @param  ptr to NCS_UBAID.
 * @param  ptr to ntfsv_msg_t.
 *
 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 */
static uint32_t enc_send_clm_node_status_cbk_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	ntfsv_ntfa_clm_status_cbk_t *param = &msg->info.cbk_info.param.clm_node_status_cbk;
	uint8_t *p8;
	uint32_t rc = NCSCC_RC_SUCCESS;

	p8 = ncs_enc_reserve_space(uba, 2);
	if (p8 == NULL) {
		TRACE("ncs_enc_reserve_space failed");
		rc = NCSCC_RC_OUT_OF_MEM;
		goto done;
	}
	ncs_encode_16bit(&p8, param->clm_node_status);
	ncs_enc_claim_space(uba, 2);

 done:
        TRACE_8("Encode CLM node status msg");
        return rc;
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
	ntfsv_msg_t *msg;
	NCS_UBAID *uba;
	uint8_t *p8;
	uint32_t rc = NCSCC_RC_SUCCESS;
	
	MDS_CLIENT_MSG_FORMAT_VER msg_fmt_version;

	msg_fmt_version = m_NCS_ENC_MSG_FMT_GET(info->info.enc.i_rem_svc_pvt_ver,
						NTFS_WRT_NTFA_SUBPART_VER_AT_MIN_MSG_FMT,
						NTFS_WRT_NTFA_SUBPART_VER_AT_MAX_MSG_FMT, NTFS_WRT_NTFA_MSG_FMT_ARRAY);
	if (0 == msg_fmt_version) {
		LOG_ER("msg_fmt_version FAILED!");
		return NCSCC_RC_FAILURE;
	}
	info->info.enc.o_msg_fmt_ver = msg_fmt_version;

	msg = (ntfsv_msg_t *)info->info.enc.i_msg;
	uba = info->info.enc.io_uba;

	if (uba == NULL) {
		LOG_ER("uba == NULL");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

    /** encode the type of message **/
	p8 = ncs_enc_reserve_space(uba, 4);
	if (p8 == NULL) {
		TRACE("ncs_enc_reserve_space failed");
		rc = NCSCC_RC_OUT_OF_MEM;
		goto done;
	}
	ncs_encode_32bit(&p8, msg->type);
	ncs_enc_claim_space(uba, 4);

	if (NTFSV_NTFA_API_RESP_MSG == msg->type) {
	/** encode the API RSP msg subtype **/
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			rc = NCSCC_RC_OUT_OF_MEM;
			goto done;
		}
		ncs_encode_32bit(&p8, msg->info.api_resp_info.type);
		ncs_enc_claim_space(uba, 4);

		/* rc */
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			rc = NCSCC_RC_OUT_OF_MEM;
			goto done;
		}
		ncs_encode_32bit(&p8, msg->info.api_resp_info.rc);
		ncs_enc_claim_space(uba, 4);

		switch (msg->info.api_resp_info.type) {
		case NTFSV_INITIALIZE_RSP:
			rc = enc_initialize_rsp_msg(uba, msg);
			break;
		case NTFSV_SUBSCRIBE_RSP:
			rc = enc_subscribe_rsp_msg(uba, msg);
			break;
		case NTFSV_UNSUBSCRIBE_RSP:
			rc = NCSCC_RC_SUCCESS;
			break;
		case NTFSV_SEND_NOT_RSP:
			rc = enc_send_not_rsp_msg(uba, msg);
			break;
		case NTFSV_READER_INITIALIZE_RSP:
			rc = enc_reader_initialize_rsp_msg(uba, msg);
			break;
		case NTFSV_READER_FINALIZE_RSP:
			rc = enc_reader_finalize_rsp_msg(uba, msg);
			break;
		case NTFSV_READ_NEXT_RSP:
			rc = enc_read_next_rsp_msg(uba, msg);
			break;
		case NTFSV_FINALIZE_RSP:
			break;
		default:
			TRACE("Unknown API RSP type = %d", msg->info.api_resp_info.type);
			rc = NCSCC_RC_FAILURE;
			break;
		}
	} else if (NTFSV_NTFS_CBK_MSG == msg->type) {
	/** encode the API RSP msg subtype **/
		p8 = ncs_enc_reserve_space(uba, 12);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			rc = NCSCC_RC_OUT_OF_MEM;		   
			goto done;
		}
		ncs_encode_32bit(&p8, msg->info.cbk_info.type);
		ncs_encode_32bit(&p8, msg->info.cbk_info.ntfs_client_id);
		ncs_encode_32bit(&p8, msg->info.cbk_info.subscriptionId);
		ncs_enc_claim_space(uba, 12);
		if (msg->info.cbk_info.type == NTFSV_NOTIFICATION_CALLBACK) {
			rc = enc_send_not_cbk_msg(uba, msg);
		} else if (msg->info.cbk_info.type == NTFSV_DISCARDED_CALLBACK) {
			rc = enc_send_discard_cbk_msg(uba, msg);
		} else if (msg->info.cbk_info.type == NTFSV_CLM_NODE_STATUS_CALLBACK) {
			rc = enc_send_clm_node_status_cbk_msg(uba, msg);
		} else {
			TRACE("unknown callback type %d", msg->info.cbk_info.type);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		TRACE_8("enc NTFSV_NTFS_CBK_MSG");
	} else {
		TRACE("unknown msg type %d", msg->type);
		rc = NCSCC_RC_FAILURE;
		goto done;
	}
done:
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_2("enc mgs type %d, api type %d, returned err rc = %u", msg->type, msg->info.api_resp_info.type, rc);
	}
	return rc;
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
	ntfsv_ntfs_evt_t *evt = NULL;
	NCS_UBAID *uba = info->info.dec.io_uba;
	uint8_t local_data[20];
	uint32_t rc;

	if (0 == m_NCS_MSG_FORMAT_IS_VALID(info->info.dec.i_msg_fmt_ver,
					   NTFS_WRT_NTFA_SUBPART_VER_AT_MIN_MSG_FMT,
					   NTFS_WRT_NTFA_SUBPART_VER_AT_MAX_MSG_FMT, NTFS_WRT_NTFA_MSG_FMT_ARRAY)) {
		TRACE("Wrong format version");
		rc = NCSCC_RC_FAILURE;		   		
		goto done;
	}

    /** allocate an NTFSV_NTFS_EVENT now **/
	if (NULL == (evt = calloc(1, sizeof(ntfsv_ntfs_evt_t)))) {
		TRACE("calloc failed");
		rc = NCSCC_RC_OUT_OF_MEM;		   
		goto done;
	}

	/* Assign the allocated event */
	info->info.dec.o_msg = (uint8_t *)evt;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	evt->info.msg.type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

	if (NTFSV_NTFA_API_MSG == evt->info.msg.type) {
		p8 = ncs_dec_flatten_space(uba, local_data, 4);
		evt->info.msg.info.api_info.type = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(uba, 4);

		/* FIX error handling for dec functions */
		switch (evt->info.msg.info.api_info.type) {
		case NTFSV_INITIALIZE_REQ:
			rc = dec_initialize_msg(uba, &evt->info.msg);
			break;
		case NTFSV_FINALIZE_REQ:
			rc = dec_finalize_msg(uba, &evt->info.msg);
			break;
		case NTFSV_SUBSCRIBE_REQ:
			rc = dec_subscribe_msg(uba, &evt->info.msg);
			break;
		case NTFSV_UNSUBSCRIBE_REQ:
			rc = dec_unsubscribe_msg(uba, &evt->info.msg);
			break;
		case NTFSV_SEND_NOT_REQ:
			rc = dec_send_not_msg(uba, &evt->info.msg);
			break;
		case NTFSV_READER_INITIALIZE_REQ:
			rc = dec_reader_initialize_msg(uba, &evt->info.msg);
			break;
		case NTFSV_READER_INITIALIZE_REQ_2:
			rc = dec_reader_initialize_msg_2(uba, &evt->info.msg);
			break;
		case NTFSV_READER_FINALIZE_REQ:
			rc = dec_reader_finalize_msg(uba, &evt->info.msg);
			break;
		case NTFSV_READ_NEXT_REQ:
			rc = dec_read_next_msg(uba, &evt->info.msg);
			break;
		default:
			TRACE("Unknown API type = %d", evt->info.msg.info.api_info.type);
			rc = NCSCC_RC_FAILURE;			
			break;
		}
	} else {
		TRACE("unknown msg type = %d", (int)evt->info.msg.type);
		rc = NCSCC_RC_FAILURE;			
	} 
	
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_2("dec mgs type %d, api type %d, returned err rc = %u",
			     evt->info.msg.type, evt->info.msg.info.api_info.type, rc);
	}
done:
	return rc;
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
	ntfsv_ntfs_evt_t *ntfsv_evt = (ntfsv_ntfs_evt_t *)mds_info->info.receive.i_msg;
	uint32_t rc = NCSCC_RC_SUCCESS;

	ntfsv_evt->evt_type = NTFSV_NTFS_NTFSV_MSG;
	ntfsv_evt->cb_hdl = (uint32_t)mds_info->i_yr_svc_hdl;
	ntfsv_evt->fr_node_id = mds_info->info.receive.i_node_id;
	ntfsv_evt->fr_dest = mds_info->info.receive.i_fr_dest;
	ntfsv_evt->rcvd_prio = mds_info->info.receive.i_priority;
	ntfsv_evt->mds_ctxt = mds_info->info.receive.i_msg_ctxt;

	/* Send the message to ntfs */
	rc = m_NCS_IPC_SEND(&ntfs_cb->mbx, ntfsv_evt, mds_info->info.receive.i_priority);
	if (rc != NCSCC_RC_SUCCESS)
		TRACE("IPC send failed %d", rc);

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
	ntfsv_ntfs_evt_t *ntfsv_evt;

    /** allocate an NTFSV_NTFS_EVENT now **/
	if (NULL == (ntfsv_evt = calloc(1, sizeof(ntfsv_ntfs_evt_t)))) {
		TRACE("memory alloc FAILED");
		goto err;
	}

	if (ntfs_cb->is_quisced_set == true) {
	/** Initialize the Event here **/
		ntfsv_evt->evt_type = NTFSV_EVT_QUIESCED_ACK;
		ntfsv_evt->cb_hdl = (uint32_t)mds_info->i_yr_svc_hdl;

		/* Push the event and we are done */
		if (NCSCC_RC_FAILURE == m_NCS_IPC_SEND(&ntfs_cb->mbx, ntfsv_evt, NCS_IPC_PRIORITY_NORMAL)) {
			TRACE("ipc send failed");
			ntfs_evt_destroy(ntfsv_evt);
			goto err;
		}
	} else {
		ntfs_evt_destroy(ntfsv_evt);
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
	ntfsv_ntfs_evt_t *evt = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE("MDS SVC event.");
	/* First make sure that this event is indeed for us */
	if (info->info.svc_evt.i_your_id != NCSMDS_SVC_ID_NTFS) {
		TRACE("event not NCSMDS_SVC_ID_NTFS");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* If this evt was sent from NTFA act on this */
	if (info->info.svc_evt.i_svc_id == NCSMDS_SVC_ID_NTFA) {
		if (info->info.svc_evt.i_change == NCSMDS_DOWN) {
			TRACE_8("MDS DOWN dest: %" PRIx64 ", node ID: %x, svc_id: %d",
				info->info.svc_evt.i_dest, info->info.svc_evt.i_node_id, info->info.svc_evt.i_svc_id);

			/* As of now we are only interested in NTFA events */
			if (NULL == (evt = calloc(1, sizeof(ntfsv_ntfs_evt_t)))) {
				rc = NCSCC_RC_FAILURE;
				TRACE("mem alloc FAILURE  ");
				goto done;
			}

			evt->evt_type = NTFSV_NTFS_EVT_NTFA_DOWN;

	    /** Initialize the Event Header **/
			evt->cb_hdl = 0;
			evt->fr_node_id = info->info.svc_evt.i_node_id;
			evt->fr_dest = info->info.svc_evt.i_dest;

	    /** Initialize the MDS portion of the header **/
			evt->info.mds_info.node_id = info->info.svc_evt.i_node_id;
			evt->info.mds_info.mds_dest_id = info->info.svc_evt.i_dest;

			/* Push the event and we are done */
			if (m_NCS_IPC_SEND(&ntfs_cb->mbx, evt, NCS_IPC_PRIORITY_NORMAL) != NCSCC_RC_SUCCESS) {
				TRACE("ipc send failed");
				ntfs_evt_destroy(evt);
				rc = NCSCC_RC_FAILURE;
				goto done;
			}
		}
	} else if (info->info.svc_evt.i_svc_id == NCSMDS_SVC_ID_AVD) {
		if (info->info.svc_evt.i_change == NCSMDS_UP) {
			TRACE_8("MDS UP dest: %" PRIx64 ", node ID: %x, svc_id: %d",
					info->info.svc_evt.i_dest, info->info.svc_evt.i_node_id, info->info.svc_evt.i_svc_id);
			//Subscribed for only INTRA NODE, only one ADEST will come.
			if (m_MDS_DEST_IS_AN_ADEST(info->info.svc_evt.i_dest)) {
				TRACE_8("AVD ADEST UP");
				ncs_sel_obj_ind(&ntfs_cb->usr2_sel_obj);
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
 * Arguments     : cb   : NTFS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t mds_callback(struct ncsmds_callback_info *info)
{
	uint32_t rc;
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
		rc = (*cb_set[info->i_op]) (info);
		if (rc == NCSCC_RC_OUT_OF_MEM) {
			LOG_ER("Exiting: out of memory");
			exit (EXIT_FAILURE);
		}
		return rc;
	} else {
		TRACE("mds callback out of range op: %d", info->i_op);
		rc = NCSCC_RC_FAILURE;
	}
	return rc;
}

/****************************************************************************
 * Name          : mds_vdest_create
 *
 * Description   : This function created the VDEST for NTFS
 *
 * Arguments     : 
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mds_vdest_create(ntfs_cb_t *ntfs_cb)
{
	NCSVDA_INFO vda_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	memset(&vda_info, 0, sizeof(NCSVDA_INFO));
	ntfs_cb->vaddr = NTFS_VDEST_ID;
	vda_info.req = NCSVDA_VDEST_CREATE;
	vda_info.info.vdest_create.i_create_type = NCSVDA_VDEST_CREATE_SPECIFIC;
	vda_info.info.vdest_create.i_persistent = false;	/* Up-to-the application */
	vda_info.info.vdest_create.i_policy = NCS_VDEST_TYPE_DEFAULT;
	vda_info.info.vdest_create.info.specified.i_vdest = ntfs_cb->vaddr;

	/* Create the VDEST address */
	if (NCSCC_RC_SUCCESS != (rc = ncsvda_api(&vda_info))) {
		LOG_ER("VDEST_CREATE_FAILED");
		return rc;
	}

	/* Store the info returned by MDS */
	ntfs_cb->mds_hdl = vda_info.info.vdest_create.o_mds_pwe1_hdl;

	return rc;
}

/****************************************************************************
 * Name          : ntfs_mds_init
 *
 * Description   : This function creates the VDEST for ntfs and installs/suscribes
 *                 into MDS.
 *
 * Arguments     : cb   : NTFS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t ntfs_mds_init(ntfs_cb_t *cb, SaAmfHAStateT ha_state)
{
	NCSMDS_INFO mds_info;
	uint32_t rc;
	MDS_SVC_ID svc = NCSMDS_SVC_ID_NTFA;

	TRACE_ENTER();

	/* Create the VDEST for NTFS */
	if (NCSCC_RC_SUCCESS != (rc = mds_vdest_create(cb))) {
		LOG_ER(" ntfs_mds_init: named vdest create FAILED\n");
		return rc;
	}

	/* Set the role of MDS */
	if (ha_state == SA_AMF_HA_ACTIVE)
		cb->mds_role = V_DEST_RL_ACTIVE;

	if (ha_state == SA_AMF_HA_STANDBY)
		cb->mds_role = V_DEST_RL_STANDBY;

	if (NCSCC_RC_SUCCESS != (rc = ntfs_mds_change_role())) {
		LOG_ER("MDS role change to %d FAILED\n", cb->mds_role);
		return rc;
	}

	/* Install your service into MDS */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_NTFS;
	mds_info.i_op = MDS_INSTALL;
	mds_info.info.svc_install.i_yr_svc_hdl = 0;
	mds_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;
	mds_info.info.svc_install.i_svc_cb = mds_callback;
	mds_info.info.svc_install.i_mds_q_ownership = false;
	mds_info.info.svc_install.i_mds_svc_pvt_ver = NTFS_SVC_PVT_SUBPART_VERSION;

	if (NCSCC_RC_SUCCESS != (rc = ncsmds_api(&mds_info))) {
		LOG_ER("MDS Install FAILED");
		return rc;
	}

	/* Now subscribe for NTFS events in MDS, TODO: WHy this? */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_NTFS;
	mds_info.i_op = MDS_SUBSCRIBE;
	mds_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	mds_info.info.svc_subscribe.i_num_svcs = 1;
	mds_info.info.svc_subscribe.i_svc_ids = &svc;

	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("MDS subscribe FAILED");
		return rc;
	}

	svc = NCSMDS_SVC_ID_AVD;
	/* Now subscribe for AVD events in MDS. This will be 
	   used for CLM registration.*/
        memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
        mds_info.i_mds_hdl = cb->mds_hdl;
        mds_info.i_svc_id = NCSMDS_SVC_ID_NTFS;
        mds_info.i_op = MDS_SUBSCRIBE;
        mds_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_INTRANODE;
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
 * Name          : ntfs_mds_change_role
 *
 * Description   : This function informs mds of our vdest role change 
 *
 * Arguments     : cb   : NTFS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t ntfs_mds_change_role()
{
	NCSVDA_INFO arg;

	memset(&arg, 0, sizeof(NCSVDA_INFO));

	arg.req = NCSVDA_VDEST_CHG_ROLE;
	arg.info.vdest_chg_role.i_vdest = ntfs_cb->vaddr;
	arg.info.vdest_chg_role.i_new_role = ntfs_cb->mds_role;

	return ncsvda_api(&arg);
}

/****************************************************************************
 * Name          : mds_vdest_destroy
 *
 * Description   : This function Destroys the Virtual destination of NTFS
 *
 * Arguments     : cb   : NTFS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mds_vdest_destroy(ntfs_cb_t *ntfs_cb)
{
	NCSVDA_INFO vda_info;
	uint32_t rc;

	memset(&vda_info, 0, sizeof(NCSVDA_INFO));
	vda_info.req = NCSVDA_VDEST_DESTROY;
	vda_info.info.vdest_destroy.i_vdest = ntfs_cb->vaddr;

	if (NCSCC_RC_SUCCESS != (rc = ncsvda_api(&vda_info))) {
		LOG_ER("NCSVDA_VDEST_DESTROY failed");
		return rc;
	}

	return rc;
}

/****************************************************************************
 * Name          : ntfs_mds_finalize
 *
 * Description   : This function un-registers the NTFS Service with MDS.
 *
 * Arguments     : Uninstall NTFS from MDS.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t ntfs_mds_finalize(ntfs_cb_t *cb)
{
	NCSMDS_INFO mds_info;
	uint32_t rc;

	/* Un-install NTFS service from MDS */
	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_NTFS;
	mds_info.i_op = MDS_UNINSTALL;

	rc = ncsmds_api(&mds_info);

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("MDS_UNINSTALL_FAILED");
		return rc;
	}

	/* Destroy the virtual Destination of NTFS */
	rc = mds_vdest_destroy(cb);
	return rc;
}

/****************************************************************************
  Name          : ntfs_mds_msg_send
 
  Description   : This routine sends a message to NTFA. The send 
                  operation may be a 'normal' send or a synchronous call that 
                  blocks until the response is received from NTFA.
 
  Arguments     : cb  - ptr to the NTFA CB
                  i_msg - ptr to the NTFSv message
                  dest  - MDS destination to send to.
                  mds_ctxt - ctxt for synch mds req-resp.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE/timeout
 
  Notes         : None.
******************************************************************************/

uint32_t ntfs_mds_msg_send(ntfs_cb_t *cb,
			ntfsv_msg_t *msg, MDS_DEST *dest, MDS_SYNC_SND_CTXT *mds_ctxt, MDS_SEND_PRIORITY_TYPE prio)
{
	NCSMDS_INFO mds_info;
	MDS_SEND_INFO *send_info = &mds_info.info.svc_send;
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* populate the mds params */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_NTFS;
	mds_info.i_op = MDS_SEND;

	send_info->i_msg = msg;
	send_info->i_to_svc = NCSMDS_SVC_ID_NTFA;
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
		LOG_ER("ntfs_mds_msg_send FAILED");
	}
	return rc;
}
