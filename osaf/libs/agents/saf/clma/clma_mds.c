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

#include <stdlib.h>
#include "clma.h"
#include "clmsv_enc_dec.h"

static MDS_CLIENT_MSG_FORMAT_VER
 CLMA_WRT_CLMS_MSG_FMT_ARRAY[CLMA_WRT_CLMS_SUBPART_VER_RANGE] = {
	1			/*msg format version for CLMA subpart version 1 */
};

/****************************************************************************
  Name          : clma_enc_initialize_msg
  
  Description   : This routine encodes an initialize API msg 
                
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uint32_t
                
  Notes         : None.
******************************************************************************/
static uint32_t clma_enc_initialize_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	clmsv_init_param_t *param = &msg->info.api_info.param.init;

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
  Name          : clma_enc_finalize_msg
 
  Description   : This routine encodes a finalize API msg
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t clma_enc_finalize_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	clmsv_finalize_param_t *param = &msg->info.api_info.param.finalize;

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
  Name          : clma_enc_track_start_msg
                        
  Description   : This routine encodes a track start API msg
                
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t clma_enc_track_start_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	clmsv_track_start_param_t *param = &msg->info.api_info.param.track_start;

	TRACE_ENTER();

	osafassert(uba != NULL);

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 9);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_8bit(&p8, param->flags);
	ncs_encode_32bit(&p8, param->sync_resp);
	ncs_enc_claim_space(uba, 9);
	total_bytes += 9;

	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
  Name          : clma_enc_track_stop_msg
                        
  Description   : This routine encodes a track start API msg
                
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t clma_enc_track_stop_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	clmsv_track_stop_param_t *param = &msg->info.api_info.param.track_stop;

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
  Name          : clma_enc_node_get_msg
 
  Description   : This routine encodes a finalize API msg
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t clma_enc_node_get_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	clmsv_node_get_param_t *param = &msg->info.api_info.param.node_get;

	TRACE_ENTER();

	osafassert(uba != NULL);

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_32bit(&p8, param->node_id);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
  Name          : clma_enc_node_get_async_msg
 
  Description   : This routine encodes a node_get_async API msg
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t clma_enc_node_get_async_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	clmsv_node_get_async_param_t *param = &msg->info.api_info.param.node_get_async;

	TRACE_ENTER();

	osafassert(uba != NULL);

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 16);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_64bit(&p8, param->inv);
	ncs_encode_32bit(&p8, param->node_id);
	ncs_enc_claim_space(uba, 16);
	total_bytes += 16;

	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
  Name          : clma_enc_response_msg
 
  Description   : This routine encodes a response API msg
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t clma_enc_response_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	SaUint32T inv1 = 0;
	SaUint32T inv2 = 0;
	clmsv_clm_response_param_t *param = &msg->info.api_info.param.clm_resp;

	TRACE_ENTER();

	osafassert(uba != NULL);

	inv1 = (param->inv & 0x00000000ffffffff);
	inv2 = (param->inv >> 32);

	TRACE("inv1 %u", inv1);
	TRACE("inv2 %u", inv2);

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 16);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_32bit(&p8, param->resp);
	/*ncs_encode_64bit(&p8, param->inv); */
	ncs_encode_32bit(&p8, inv1);
	ncs_encode_32bit(&p8, inv2);
	ncs_enc_claim_space(uba, 16);
	total_bytes += 16;

	TRACE("param->inv %llu", param->inv);

	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
  Name          : clma_mds_enc
 
  Description   : This is a callback routine that is invoked to encode CLMS
                  messages.
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t clma_mds_enc(struct ncsmds_callback_info *info)
{
	CLMSV_MSG *msg;
	NCS_UBAID *uba;
	uint8_t *p8;
	uint32_t total_bytes = 0, ret_bytes = 0;
	TRACE_ENTER();

	MDS_CLIENT_MSG_FORMAT_VER msg_fmt_version;

	msg_fmt_version = m_NCS_ENC_MSG_FMT_GET(info->info.enc.i_rem_svc_pvt_ver,
						CLMA_WRT_CLMS_SUBPART_VER_AT_MIN_MSG_FMT,
						CLMA_WRT_CLMS_SUBPART_VER_AT_MAX_MSG_FMT, CLMA_WRT_CLMS_MSG_FMT_ARRAY);
	if (0 == msg_fmt_version) {
		TRACE("Wrong msg_fmt_version!!\n");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	info->info.enc.o_msg_fmt_ver = msg_fmt_version;

	msg = (CLMSV_MSG *) info->info.enc.i_msg;
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
	ncs_encode_32bit(&p8, msg->evt_type);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	TRACE_2("msgtype: %d", msg->evt_type);

	if (CLMSV_CLMA_TO_CLMS_API_MSG == msg->evt_type) {
	/** encode the API msg subtype **/
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE("encode API msg subtype FAILED");
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
		ncs_encode_32bit(&p8, msg->info.api_info.type);
		ncs_enc_claim_space(uba, 4);
		total_bytes += 4;

		TRACE_2("api_info.type: %d\n", msg->info.api_info.type);
		switch (msg->info.api_info.type) {
		case CLMSV_INITIALIZE_REQ:
			ret_bytes = clma_enc_initialize_msg(uba, msg);
			break;
		case CLMSV_FINALIZE_REQ:
			ret_bytes = clma_enc_finalize_msg(uba, msg);
			break;
		case CLMSV_TRACK_START_REQ:
			ret_bytes = clma_enc_track_start_msg(uba, msg);
			break;
		case CLMSV_TRACK_STOP_REQ:
			ret_bytes = clma_enc_track_stop_msg(uba, msg);
			break;
		case CLMSV_NODE_GET_REQ:
			ret_bytes = clma_enc_node_get_msg(uba, msg);
			break;
		case CLMSV_NODE_GET_ASYNC_REQ:
			ret_bytes = clma_enc_node_get_async_msg(uba, msg);
			break;
		case CLMSV_RESPONSE_REQ:
			ret_bytes = clma_enc_response_msg(uba, msg);
			break;
		default:
			TRACE("Unknown API type = %d", msg->info.api_info.type);
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}

		if (ret_bytes != 0) {
			total_bytes += ret_bytes;
		} else {
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}
	} else {
		TRACE("Wrong event type!!\n");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : clma_dec_initialize_rsp_msg
 
  Description   : This routine decodes an initialize sync response message
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t clma_dec_initialize_rsp_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	SaUint32T *client_id = &msg->info.api_resp_info.param.client_id;
	uint8_t local_data[100];

	osafassert(uba != NULL);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	*client_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	return total_bytes;
}

static uint32_t clma_dec_node_get_msg(NCS_UBAID *uba, SaClmClusterNodeT_4 * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	SaClmClusterNodeT_4 *param = msg;
	uint8_t local_data[100];

	osafassert(uba != NULL);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->nodeId = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	total_bytes += clmsv_decodeNodeAddressT(uba, &param->nodeAddress);
	total_bytes += clmsv_decodeSaNameT(uba, &param->nodeName);
	total_bytes += clmsv_decodeSaNameT(uba, &param->executionEnvironment);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->member = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->bootTimestamp = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->initialViewNumber = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	return total_bytes;
}

/****************************************************************************
  Name          : clma_dec_node_get_rsp_msg
 
  Description   : This routine decodes an initialize sync response message
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t clma_dec_node_get_rsp_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	return clma_dec_node_get_msg(uba, &msg->info.api_resp_info.param.node_get);
}

static uint32_t clma_dec_cluster_ntf_buf_msg(NCS_UBAID *uba, SaClmClusterNotificationBufferT_4 * notify_info)
{
	uint8_t *p8;
	uint32_t total_bytes = 0, i;
	SaClmClusterNotificationBufferT_4 *param = notify_info;
	uint8_t local_data[100];
	TRACE_ENTER();

	osafassert(uba != NULL);

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->viewNumber = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->numberOfItems = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	param->notification = (SaClmClusterNotificationT_4 *)
	    malloc(sizeof(SaClmClusterNotificationT_4) * param->numberOfItems);
	if (param->notification == NULL) {
		TRACE("Can not allocate memory notification!!!\n");
		TRACE_LEAVE();
		return 0;
	}

	for (i = 0; i < param->numberOfItems; i++) {
		p8 = ncs_dec_flatten_space(uba, local_data, 4);
		param->notification[i].clusterChange = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(uba, 4);
		total_bytes += 4;
		total_bytes += clma_dec_node_get_msg(uba, &param->notification[i].clusterNode);
	}

	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
  Name          : clma_dec_track_current_rsp_msg
 
  Description   : This routine decodes an initialize sync response message
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t clma_dec_track_current_rsp_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	CLMSV_TRACK_INFO *track = &msg->info.api_resp_info.param.track;
	uint8_t local_data[100];
	TRACE_ENTER();
	osafassert(uba != NULL);

	p8 = ncs_dec_flatten_space(uba, local_data, 2);
	track->num = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, 2);
	total_bytes += 2;

	track->notify_info = (SaClmClusterNotificationBufferT_4 *)
	    malloc(sizeof(SaClmClusterNotificationBufferT_4));
	if (track->notify_info == NULL) {
		TRACE("Can not allocate memory notify_info!!!\n");
		TRACE_LEAVE();
		return 0;
	}
	total_bytes += clma_dec_cluster_ntf_buf_msg(uba, track->notify_info);
	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
  Name          : clma_dec_track_cbk_msg
 
  Description   : This routine decodes track ckb response message
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t clma_dec_track_cbk_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	CLMSV_TRACK_CBK_INFO *track = &msg->info.cbk_info.param.track;
	uint8_t local_data[100];
	TRACE_ENTER();
	osafassert(uba != NULL);

	total_bytes += clma_dec_cluster_ntf_buf_msg(uba, &track->buf_info);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	track->mem_num = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;
	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	track->inv = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	TRACE("track->inv %llu", track->inv);
	track->root_cause_ent = (SaNameT *)malloc(sizeof(SaNameT));
	if (track->root_cause_ent == NULL) {
		TRACE("Can not allocate memory notification!!!\n");
		TRACE_LEAVE();
		return 0;
	}
	total_bytes += clmsv_decodeSaNameT(uba, track->root_cause_ent);
	track->cor_ids = (SaNtfCorrelationIdsT *) malloc(sizeof(SaNtfCorrelationIdsT));
	if (track->cor_ids == NULL) {
		TRACE("Can not allocate memory notification!!!\n");
		TRACE_LEAVE();
		return 0;
	}
	p8 = ncs_dec_flatten_space(uba, local_data, 24);
	track->cor_ids->rootCorrelationId = ncs_decode_64bit(&p8);
	track->cor_ids->parentCorrelationId = ncs_decode_64bit(&p8);
	track->cor_ids->notificationId = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 24);
	total_bytes += 24;
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	track->step = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;
	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	track->time_super = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	track->err = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
  Name          : clma_dec_node_async_get_cbk_msg 
 
  Description   : This routine decodes track ckb response message
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t clma_dec_node_async_get_cbk_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	CLMSV_NODE_GET_ASYNC_CBK_INFO *node_get = &msg->info.cbk_info.param.node_get;
	uint8_t local_data[100];
	TRACE_ENTER();
	osafassert(uba != NULL);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	node_get->err = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	if (node_get->err == SA_AIS_OK) {

		p8 = ncs_dec_flatten_space(uba, local_data, 8);
		node_get->inv = ncs_decode_64bit(&p8);
		ncs_dec_skip_space(uba, 8);
		total_bytes += 8;
		total_bytes += clma_dec_node_get_msg(uba, &node_get->info);
	}

	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
  Name          : clma_mds_dec
 
  Description   : This is a callback routine that is invoked to decode CLMS
                  messages.
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t clma_mds_dec(struct ncsmds_callback_info *info)
{
	uint8_t *p8;
	CLMSV_MSG *msg;
	NCS_UBAID *uba = info->info.dec.io_uba;
	uint8_t local_data[20];
	uint32_t total_bytes = 0, ret_bytes = 0;
	TRACE_ENTER();

	if (0 == m_NCS_MSG_FORMAT_IS_VALID(info->info.dec.i_msg_fmt_ver,
					   CLMA_WRT_CLMS_SUBPART_VER_AT_MIN_MSG_FMT,
					   CLMA_WRT_CLMS_SUBPART_VER_AT_MAX_MSG_FMT, CLMA_WRT_CLMS_MSG_FMT_ARRAY)) {
		TRACE("Invalid message format!!!\n");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

    /** Allocate a new msg in both sync/async cases 
     **/
	if (NULL == (msg = calloc(1, sizeof(CLMSV_MSG)))) {
		TRACE("calloc failed\n");
		return NCSCC_RC_FAILURE;
	}

	info->info.dec.o_msg = (uint8_t *)msg;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	msg->evt_type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	switch (msg->evt_type) {
	case CLMSV_CLMS_TO_CLMA_API_RESP_MSG:
		{
			p8 = ncs_dec_flatten_space(uba, local_data, 8);
			msg->info.api_resp_info.type = ncs_decode_32bit(&p8);
			msg->info.api_resp_info.rc = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 8);
			total_bytes += 8;
			TRACE_2("CLMSV_CLMA_API_RESP_MSG rc = %d", (int)msg->info.api_resp_info.rc);

			switch (msg->info.api_resp_info.type) {
			case CLMSV_INITIALIZE_RESP:
				total_bytes += clma_dec_initialize_rsp_msg(uba, msg);
				break;
			case CLMSV_FINALIZE_RESP:
				break;
			case CLMSV_TRACK_CURRENT_RESP:
				ret_bytes = clma_dec_track_current_rsp_msg(uba, msg);
				if (ret_bytes != 0) {
					total_bytes += ret_bytes;
				} else {
					/* Malloc failure */
					free(msg);
					return NCSCC_RC_FAILURE;
				}
				break;
			case CLMSV_TRACK_STOP_RESP:
				break;
			case CLMSV_NODE_GET_RESP:
				if (msg->info.api_resp_info.rc == SA_AIS_OK)
					total_bytes += clma_dec_node_get_rsp_msg(uba, msg);
				break;
			case CLMSV_RESPONSE_RESP:
				break;
			default:
				TRACE_2("Unknown API RSP type %d", msg->info.api_resp_info.type);
				free(msg);
				return NCSCC_RC_FAILURE;
			}
		}
		break;
	case CLMSV_CLMS_TO_CLMA_CBK_MSG:
		{
			p8 = ncs_dec_flatten_space(uba, local_data, 8);
			msg->info.cbk_info.type = ncs_decode_32bit(&p8);
			msg->info.cbk_info.client_id = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 8);
			total_bytes += 8;
			TRACE_2("CLMSV_CLMS_TO_CLMA_CBK_MSG");
			switch (msg->info.cbk_info.type) {
			case CLMSV_TRACK_CBK:
				TRACE_2("decode Track cbk message");
				ret_bytes += clma_dec_track_cbk_msg(uba, msg);
				if (ret_bytes != 0) {
					total_bytes += ret_bytes;
				} else {
					/* Malloc failure */
					free(msg);
					return NCSCC_RC_FAILURE;
				}
				break;
			case CLMSV_NODE_ASYNC_GET_CBK:
				TRACE_2("decode Ndoe async cbk message");
				total_bytes += clma_dec_node_async_get_cbk_msg(uba, msg);
				break;
			default:
				TRACE_2("Unknown callback type = %d!", msg->info.cbk_info.type);
				free(msg);
				return NCSCC_RC_FAILURE;
			}
		}
		break;
	case CLMSV_CLMS_TO_CLMA_IS_MEMBER_MSG:
		{
			TRACE_2("CLMSV_CLMS_TO_CLMA_IS_MEMBER_MSG");
			p8 = ncs_dec_flatten_space(uba, local_data, 12);
			msg->info.is_member_info.is_member = ncs_decode_32bit(&p8);
			msg->info.is_member_info.is_configured = ncs_decode_32bit(&p8);
			msg->info.is_member_info.client_id = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 12);
			total_bytes += 12;
		}
		break;
	default:
		TRACE("Unknown MSG type %d", msg->evt_type);
		free(msg);
		return NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : clma_clms_msg_proc
 
  Description   : This routine is used to process the ASYNC incoming
                  CLMS messages. 
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t clma_clms_msg_proc(clma_cb_t * cb, CLMSV_MSG * clmsv_msg, MDS_SEND_PRIORITY_TYPE prio)
{
	TRACE_ENTER();

	switch (clmsv_msg->evt_type) {
	case CLMSV_CLMS_TO_CLMA_CBK_MSG:
		switch (clmsv_msg->info.cbk_info.type) {
		case CLMSV_TRACK_CBK:
		case CLMSV_NODE_ASYNC_GET_CBK:
			{
				clma_client_hdl_rec_t *clma_hdl_rec;

				TRACE_2("CLMSV_TRACK_CBK: " " client_id = %d", (int)clmsv_msg->info.cbk_info.client_id);

			/** Create the chan hdl record here before 
                         ** queing this message onto the priority queue
                         ** so that the dispatch by the application to fetch
                         ** the callback is instantaneous.
                         **/

			/** Lookup the hdl rec by client_id  **/
				if (NULL == (clma_hdl_rec =
					     clma_find_hdl_rec_by_client_id(cb, clmsv_msg->info.cbk_info.client_id))) {
					TRACE("client_id not found");
					TRACE_LEAVE();
					return NCSCC_RC_FAILURE;
				}

				TRACE_2("clmsv_msg->evt_type=  %d", clmsv_msg->evt_type);
			/** enqueue this message  **/
				if (NCSCC_RC_SUCCESS != m_NCS_IPC_SEND(&clma_hdl_rec->mbx, clmsv_msg, prio)) {
					TRACE("IPC SEND FAILED");
					TRACE_LEAVE();
					return NCSCC_RC_FAILURE;
				}
			}
			break;

		default:
			TRACE("unknown type %d", clmsv_msg->info.cbk_info.type);
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
			break;
		}
		break;
	case CLMSV_CLMS_TO_CLMA_IS_MEMBER_MSG:
		{
			clma_client_hdl_rec_t *clma_hdl_rec;
			TRACE_2("CLMSV_CLMS_TO_CLMA_IS_MEMBER_MSG: " " client_id = %d", (int)clmsv_msg->info.is_member_info.client_id);
			/** Lookup the hdl rec by client_id  **/
			if (NULL == (clma_hdl_rec =
						clma_find_hdl_rec_by_client_id(cb, clmsv_msg->info.is_member_info.client_id))) {
				TRACE("client_id not found");
				TRACE_LEAVE();
				return NCSCC_RC_FAILURE;
			}
			clma_hdl_rec->is_member = clmsv_msg->info.is_member_info.is_member;
			clma_hdl_rec->is_configured = clmsv_msg->info.is_member_info.is_configured;
		}
		break;
	default:
	    /** Unexpected message **/
		TRACE_2("Unexpected message type: %d", clmsv_msg->evt_type);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
		break;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : clma_mds_rcv
 
  Description   : This is a callback routine that is invoked when CLMA message
                  is received from CLMS.
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

static uint32_t clma_mds_rcv(struct ncsmds_callback_info *mds_cb_info)
{
	CLMSV_MSG *clmsv_msg = (CLMSV_MSG *) mds_cb_info->info.receive.i_msg;
	uint32_t rc;
	TRACE_ENTER();
	pthread_mutex_lock(&clma_cb.cb_lock);

	TRACE_2("msg->evt_type= : %d", clmsv_msg->evt_type);
	/* process the message */
	rc = clma_clms_msg_proc(&clma_cb, clmsv_msg, mds_cb_info->info.receive.i_priority);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_2("clma_clms_msg_proc returned: %d", rc);
		clma_msg_destroy(clmsv_msg);	/*need to do */
	}

	pthread_mutex_unlock(&clma_cb.cb_lock);

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : clma_mds_svc_evt
 
  Description   : This is a callback routine that is invoked to inform CLMA 
                  of MDS events. CLMA had subscribed to these events during
                  through MDS subscription.
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t clma_mds_svc_evt(struct ncsmds_callback_info *mds_cb_info)
{
	TRACE_2("CLMA Rcvd MDS subscribe evt from svc %d \n", mds_cb_info->info.svc_evt.i_svc_id);

	if (mds_cb_info->info.svc_evt.i_svc_id != NCSMDS_SVC_ID_CLMS) {
		TRACE("Unknown svc_id");
		return NCSCC_RC_SUCCESS;
	}

	switch (mds_cb_info->info.svc_evt.i_change) {
	case NCSMDS_NO_ACTIVE:
	case NCSMDS_DOWN:
		/** TBD what to do if CLMS goes down
                 ** Hold on to the subscription if possible
                 ** to send them out if CLMS comes back up
                 **/
		TRACE("CLMS down");
		pthread_mutex_lock(&clma_cb.cb_lock);
		memset(&clma_cb.clms_mds_dest, 0, sizeof(MDS_DEST));
		clma_cb.clms_up = 0;
		pthread_mutex_unlock(&clma_cb.cb_lock);
		break;
	case NCSMDS_NEW_ACTIVE:
	case NCSMDS_UP:
		    /** Store the MDS DEST of the CLMS 
                     **/
		TRACE_2("MSG from CLMS NCSMDS_NEW_ACTIVE/UP");
		pthread_mutex_lock(&clma_cb.cb_lock);
		clma_cb.clms_mds_dest = mds_cb_info->info.svc_evt.i_dest;
		clma_cb.clms_up = 1;
		if (clma_cb.clms_sync_awaited) {
			/* signal waiting thread */
			m_NCS_SEL_OBJ_IND(&clma_cb.clms_sync_sel);
		}
		pthread_mutex_unlock(&clma_cb.cb_lock);
		break;
	default:
		break;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : clma_mds_enc_flat
 *
 * Description   : MDS encode and flatten
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t clma_mds_enc_flat(struct ncsmds_callback_info *info)
{
	/* Modify the MDS_INFO to populate enc */
	info->info.enc = info->info.enc_flat;
	/* Invoke the regular mds_enc routine  */
	return clma_mds_enc(info);
}

/****************************************************************************
 * Name          : clma_mds_dec_flat
 *
 * Description   : MDS decode and flatten
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t clma_mds_dec_flat(struct ncsmds_callback_info *info)
{
	uint32_t rc;

	/* Modify the MDS_INFO to populate dec */
	info->info.dec = info->info.dec_flat;
	/* Invoke the regular mds_dec routine  */
	rc = clma_mds_dec(info);
	if (rc != NCSCC_RC_SUCCESS)
		TRACE("clma_mds_dec rc = %d", rc);

	return rc;
}

/****************************************************************************
  Name          : clma_mds_cpy
 
  Description   : This function copies an events sent from CLMS.
 
  Arguments     :pointer to struct ncsmds_callback_info
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t clma_mds_cpy(struct ncsmds_callback_info *info)
{
	TRACE_ENTER();
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : clma_mds_callback
 *
 * Description   : Callback Dispatcher for various MDS operations.
 *
 * Arguments     : info  : ptr to ncsmds_callback_info structure.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *      
 * Notes         : None.
 *****************************************************************************/
static uint32_t clma_mds_callback(struct ncsmds_callback_info *info)
{
	uint32_t rc;

	static NCSMDS_CALLBACK_API cb_set[MDS_CALLBACK_SVC_MAX] = {
		clma_mds_cpy,	/* MDS_CALLBACK_COPY      0 */
		clma_mds_enc,	/* MDS_CALLBACK_ENC       1 */
		clma_mds_dec,	/* MDS_CALLBACK_DEC       2 */
		clma_mds_enc_flat,	/* MDS_CALLBACK_ENC_FLAT  3 */
		clma_mds_dec_flat,	/* MDS_CALLBACK_DEC_FLAT  4 */
		clma_mds_rcv,	/* MDS_CALLBACK_RECEIVE   5 */
		clma_mds_svc_evt	/* MDS_CALLBACK_SVC_EVENT 6 */
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
  Name          : clma_mds_init
 
  Description   : This routine registers the CLMA Service with MDS.
 
  Arguments     : cb - ptr to the CLMA control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t clma_mds_init(clma_cb_t * cb)
{
	NCSADA_INFO ada_info;
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;
	MDS_SVC_ID svc = NCSMDS_SVC_ID_CLMS;

	TRACE_ENTER();
    /** Create the ADEST for CLMA and get the pwe hdl**/
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
	mds_info.i_svc_id = NCSMDS_SVC_ID_CLMA;
	mds_info.i_op = MDS_INSTALL;

	mds_info.info.svc_install.i_yr_svc_hdl = 0;
	mds_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;	/* PWE scope */
	mds_info.info.svc_install.i_svc_cb = clma_mds_callback;	/* callback */	/*need to do */
	mds_info.info.svc_install.i_mds_q_ownership = false;	/* CLMA doesn't own the mds queue */
	mds_info.info.svc_install.i_mds_svc_pvt_ver = CLMA_SVC_PVT_SUBPART_VERSION;

	if ((rc = ncsmds_api(&mds_info)) != NCSCC_RC_SUCCESS) {
		TRACE("MDS registration failed");
		return NCSCC_RC_FAILURE;
	}

	/* Now subscribe for events that will be generated by MDS */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CLMA;
	mds_info.i_op = MDS_SUBSCRIBE;

	mds_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	mds_info.info.svc_subscribe.i_num_svcs = 1;
	mds_info.info.svc_subscribe.i_svc_ids = &svc;

	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("MDS subscription failed");
		return rc;
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : clma_mds_finalize
 
  Description   : This routine unregisters the CLMA Service from MDS.
 
  Arguments     : cb - ptr to the CLMA control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
void clma_mds_finalize(clma_cb_t * cb)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	/* Un-install your service into MDS. 
	   No need to cancel the services that are subscribed */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CLMA;
	mds_info.i_op = MDS_UNINSTALL;

	if ((rc = ncsmds_api(&mds_info)) != NCSCC_RC_SUCCESS) {
		TRACE("MDS API Call Failed");
	}

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : clma_mds_msg_sync_send
 
  Description   : This routine sends the CLMA message to CLMS. The send 
                  operation is a synchronous call that 
                  blocks until the response is received from CLMS.
 
  Arguments     : cb  - ptr to the CLMA CB
                  i_msg - ptr to the CLMSv message
                  o_msg - double ptr to CLMSv message response
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE/timeout
 
  Notes         : None.
******************************************************************************/
uint32_t clma_mds_msg_sync_send(clma_cb_t * cb, CLMSV_MSG * i_msg, CLMSV_MSG ** o_msg, uint32_t timeout)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	osafassert(cb != NULL && i_msg != NULL && o_msg != NULL);

	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CLMA;
	mds_info.i_op = MDS_SEND;

	/* Fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_msg;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_CLMS;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SNDRSP;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;	/* fixme? */
	/* fill the sub send rsp strcuture */
	mds_info.info.svc_send.info.sndrsp.i_time_to_wait = timeout;	/* timeto wait in 10ms FIX!!! */
	mds_info.info.svc_send.info.sndrsp.i_to_dest = cb->clms_mds_dest;

	/* send the message */
	if (NCSCC_RC_SUCCESS == (rc = ncsmds_api(&mds_info))) {
		/* Retrieve the response and take ownership of the memory  */
		*o_msg = (CLMSV_MSG *) mds_info.info.svc_send.info.sndrsp.o_rsp;
		mds_info.info.svc_send.info.sndrsp.o_rsp = NULL;
	} else
		TRACE("clma_mds_msg_sync_send FAILED");

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : clma_mds_msg_async_send
 
  Description   : This routine sends the CLMA message to CLMS.
 
  Arguments     : cb  - ptr to the CLMA CB
                  i_msg - ptr to the CLMSv message
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t clma_mds_msg_async_send(clma_cb_t * cb, CLMSV_MSG * i_msg, uint32_t prio)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	osafassert(cb != NULL && i_msg != NULL);

	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CLMA;
	mds_info.i_op = MDS_SEND;

	/* fill the main send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_msg;
	mds_info.info.svc_send.i_priority = prio;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_CLMS;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;

	/* fill the sub send strcuture */
	mds_info.info.svc_send.info.snd.i_to_dest = cb->clms_mds_dest;

	/* send the message */
	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS)
		TRACE("failed");

	TRACE_LEAVE();
	return rc;
}
