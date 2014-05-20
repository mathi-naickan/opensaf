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
 * Author(s):  Emerson Network Power
 *
 */

#include <ncsencdec_pub.h>
#include "clms.h"

#define CLMS_SVC_PVT_SUBPART_VERSION 1
#define CLMS_WRT_CLMA_SUBPART_VER_AT_MIN_MSG_FMT 1
#define CLMS_WRT_CLMA_SUBPART_VER_AT_MAX_MSG_FMT 1
#define CLMS_WRT_CLMA_SUBPART_VER_RANGE             \
        (CLMS_WRT_CLMA_SUBPART_VER_AT_MAX_MSG_FMT - \
         CLMS_WRT_CLMA_SUBPART_VER_AT_MIN_MSG_FMT + 1)

static MDS_CLIENT_MSG_FORMAT_VER
 CLMS_WRT_CLMA_MSG_FMT_ARRAY[CLMS_WRT_CLMA_SUBPART_VER_RANGE] = {
	1			/*msg format version for CLMA subpart version 1 */
};

/****************************************************************************
  Name          : clms_dec_initialize_msg
 
  Description   : This routine decodes an initialize API msg
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t clms_dec_initialize_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	clmsv_init_param_t *param = &msg->info.api_info.param.init;
	uint8_t local_data[3];

	/* releaseCode, majorVersion, minorVersion */
	p8 = ncs_dec_flatten_space(uba, local_data, 3);
	param->version.releaseCode = ncs_decode_8bit(&p8);
	param->version.majorVersion = ncs_decode_8bit(&p8);
	param->version.minorVersion = ncs_decode_8bit(&p8);
	ncs_dec_skip_space(uba, 3);
	total_bytes += 3;

	TRACE_8("CLMSV_INITIALIZE_REQ");
	return total_bytes;
}

/****************************************************************************
  Name          : clms_dec_finalize_msg
 
  Description   : This routine decodes a finalize API msg
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t clms_dec_finalize_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	clmsv_finalize_param_t *param = &msg->info.api_info.param.finalize;
	uint8_t local_data[4];

	/* client_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->client_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	TRACE_8("CLMSV_FINALIZE_REQ");
	return total_bytes;
}

/****************************************************************************
  Name          : clms_dec_track_start_msg
                        
  Description   : This routine encodes a track start API msg
                
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t clms_dec_track_start_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	clmsv_track_start_param_t *param = &msg->info.api_info.param.track_start;
	uint8_t local_data[9];
	TRACE_ENTER();

	p8 = ncs_dec_flatten_space(uba, local_data, 9);
	param->client_id = ncs_decode_32bit(&p8);
	param->flags = ncs_decode_8bit(&p8);
	param->sync_resp = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 9);
	total_bytes += 9;

	TRACE_8("CLMSV_TRACK_START_REQ");
	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
  Name          : clms_dec_track_stop_msg
                        
  Description   : This routine encodes a track start API msg
                
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t clms_dec_track_stop_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	clmsv_track_stop_param_t *param = &msg->info.api_info.param.track_stop;
	uint8_t local_data[4];

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->client_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	TRACE_8("CLMSV_TRACK_STOP_REQ");
	return total_bytes;
}

/****************************************************************************
  Name          : clms_dec_node_get_msg
 
  Description   : This routine encodes a finalize API msg
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t clms_dec_node_get_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	clmsv_node_get_param_t *param = &msg->info.api_info.param.node_get;
	uint8_t local_data[8];

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->client_id = ncs_decode_32bit(&p8);
	param->node_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	TRACE_8("CLMSV_NODE_GET_REQ");
	return total_bytes;
}

/****************************************************************************
  Name          : clms_dec_node_get_async_msg
 
  Description   : This routine encodes a node_get_async API msg
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t clms_dec_node_get_async_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	clmsv_node_get_async_param_t *param = &msg->info.api_info.param.node_get_async;
	uint8_t local_data[16];

	p8 = ncs_dec_flatten_space(uba, local_data, 16);
	param->client_id = ncs_decode_32bit(&p8);
	param->inv = ncs_decode_64bit(&p8);
	param->node_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 16);
	total_bytes += 16;

	TRACE_8("CLMSV_NODE_GET_ASYNC_REQ");
	return total_bytes;
}

/****************************************************************************
  Name          : clms_dec_response_msg
 
  Description   : This routine encodes a response API msg
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t clms_dec_response_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	SaUint32T inv1 = 0;
	SaUint32T inv2 = 0;
	uint32_t total_bytes = 0;

	clmsv_clm_response_param_t *param = &msg->info.api_info.param.clm_resp;
	uint8_t local_data[100];

	p8 = ncs_dec_flatten_space(uba, local_data, 16);
	param->client_id = ncs_decode_32bit(&p8);
	param->resp = ncs_decode_32bit(&p8);

	inv1 = ncs_decode_32bit(&p8);
	inv2 = ncs_decode_32bit(&p8);

	TRACE("inv1 %u", inv1);
	TRACE("inv2 %u", inv2);

	param->inv = ((SaUint64T)inv2 << 32) | inv1;
	/* param->inv = ncs_decode_64bit(&p8); */
	TRACE("param->inv %llu", param->inv);
	ncs_dec_skip_space(uba, 16);
	total_bytes += 16;

	TRACE_8("CLMSV_RESPONSE_REQ");
	return total_bytes;
}

/****************************************************************************
  Name          : clms_enc_initialize_rsp_msg
 
  Description   : This routine decodes an initialize sync response message
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t clms_enc_initialize_rsp_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	SaUint32T *client_id = &msg->info.api_resp_info.param.client_id;
	/* client_id */
	p8 = ncs_enc_reserve_space(uba, 4);
	if (p8 == NULL) {
		TRACE("ncs_enc_reserve_space failed");
		goto done;
	}

	ncs_encode_32bit(&p8, *client_id);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

 done:
	TRACE_8("CLMSV_INITIALIZE_RSP");
	return total_bytes;
}

uint32_t encodeNodeAddressT(NCS_UBAID *uba, SaClmNodeAddressT *nodeAddress)
{
	uint8_t *p8 = NULL;
	uint32_t total_bytes = 0;

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	ncs_encode_32bit(&p8, nodeAddress->family);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;
	p8 = ncs_enc_reserve_space(uba, 2);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	if (nodeAddress->length > SA_CLM_MAX_ADDRESS_LENGTH) {
		LOG_ER("SaNameT length too long %hd", nodeAddress->length);
		osafassert(0);
	}
	ncs_encode_16bit(&p8, nodeAddress->length);
	ncs_enc_claim_space(uba, 2);
	total_bytes += 2;
	ncs_encode_n_octets_in_uba(uba, nodeAddress->value, (uint32_t)nodeAddress->length);
	total_bytes += (uint32_t)nodeAddress->length;
	return total_bytes;
}

static uint32_t clms_enc_node_get_msg(NCS_UBAID *uba, SaClmClusterNodeT_4 * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	SaClmClusterNodeT_4 *param = msg;

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	ncs_encode_32bit(&p8, param->nodeId);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	total_bytes += encodeNodeAddressT(uba, &param->nodeAddress);
	total_bytes += clmsv_encodeSaNameT(uba, &param->nodeName);
	total_bytes += clmsv_encodeSaNameT(uba, &param->executionEnvironment);

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	ncs_encode_32bit(&p8, param->member);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	ncs_encode_64bit(&p8, param->bootTimestamp);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	/*ncs_encode_32bit(&p8, param->initialViewNumber); */
	ncs_encode_64bit(&p8, param->initialViewNumber);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	return total_bytes;
}

/****************************************************************************
  Name          : clms_enc_node_get_rsp_msg
 
  Description   : This routine decodes an initialize sync response message
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t clms_enc_node_get_rsp_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	return clms_enc_node_get_msg(uba, &msg->info.api_resp_info.param.node_get);
}

static uint32_t clms_enc_cluster_ntf_buf_msg(NCS_UBAID *uba, SaClmClusterNotificationBufferT_4 * notify_info)
{
	uint8_t *p8;
	uint32_t total_bytes = 0, i;
	SaClmClusterNotificationBufferT_4 *param = notify_info;
	TRACE_ENTER();

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	ncs_encode_64bit(&p8, param->viewNumber);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	ncs_encode_32bit(&p8, param->numberOfItems);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	for (i = 0; i < param->numberOfItems; i++) {
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE("p8 NULL!!!");
			return 0;
		}
		ncs_encode_32bit(&p8, param->notification[i].clusterChange);
		ncs_enc_claim_space(uba, 4);
		total_bytes += 4;
		total_bytes += clms_enc_node_get_msg(uba, &param->notification[i].clusterNode);
	}

	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
  Name          : clms_enc_track_current_rsp_msg
 
  Description   : This routine decodes an initialize sync response message
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t clms_enc_track_current_rsp_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	CLMSV_TRACK_INFO *track = &msg->info.api_resp_info.param.track;
	TRACE_ENTER();

/*	p8 = ncs_enc_reserve_space(uba,4);
        if (!p8) {
                TRACE("p8 NULL!!!");
                return 0;
        }
	ncs_encode_32bit(&p8, track->view_num);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;
*/
	p8 = ncs_enc_reserve_space(uba, 2);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	ncs_encode_16bit(&p8, track->num);
	ncs_enc_claim_space(uba, 2);
	total_bytes += 2;

	total_bytes += clms_enc_cluster_ntf_buf_msg(uba, track->notify_info);

	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
  Name          : clms_enc_node_async_get_cbk_msg 
 
  Description   : This routine encodes track ckb response message
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t clms_enc_node_async_get_cbk_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	CLMSV_NODE_GET_ASYNC_CBK_INFO *node_get = &msg->info.cbk_info.param.node_get;
	TRACE_ENTER();

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	ncs_encode_32bit(&p8, node_get->err);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	if (node_get->err == SA_AIS_OK) {

		p8 = ncs_enc_reserve_space(uba, 8);
		if (!p8) {
			TRACE("p8 NULL!!!");
			return 0;
		}
		ncs_encode_64bit(&p8, node_get->inv);
		ncs_enc_claim_space(uba, 8);
		total_bytes += 8;

		total_bytes += clms_enc_node_get_msg(uba, &node_get->info);
	}

	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
  Name          : clma_dec_track_cbk_msg
 
  Description   : This routine decodes track ckb response message
 
  Arguments     : NCS_UBAID *msg,
                  CLMSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uint32_t clms_enc_track_cbk_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	CLMSV_TRACK_CBK_INFO *track = &msg->info.cbk_info.param.track;
	TRACE_ENTER();

	total_bytes += clms_enc_cluster_ntf_buf_msg(uba, &track->buf_info);

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	ncs_encode_32bit(&p8, track->mem_num);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	ncs_encode_64bit(&p8, track->inv);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	total_bytes += clmsv_encodeSaNameT(uba, track->root_cause_ent);

	p8 = ncs_enc_reserve_space(uba, 24);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	ncs_encode_64bit(&p8, track->cor_ids->rootCorrelationId);
	ncs_encode_64bit(&p8, track->cor_ids->parentCorrelationId);
	ncs_encode_64bit(&p8, track->cor_ids->notificationId);
	ncs_enc_claim_space(uba, 24);
	total_bytes += 24;

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	ncs_encode_32bit(&p8, track->step);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	ncs_encode_32bit(&p8, track->time_super);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	ncs_encode_32bit(&p8, track->err);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
 * Name          : clms_mds_cpy
 *
 * Description   : MDS copy.
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t clms_mds_cpy(struct ncsmds_callback_info *info)
{
	/* TODO; */
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : clms_mds_enc

  Description   : This routine is invoked to encode AVND message.

  Arguments     : cb       - ptr to the AvA control block
                  enc_info - ptr to the MDS encode info

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
uint32_t clms_mds_enc(struct ncsmds_callback_info *info)
{
	CLMSV_MSG *msg;
	NCS_UBAID *uba;
	uint8_t *p8;
	uint32_t total_bytes = 0;
	MDS_CLIENT_MSG_FORMAT_VER msg_fmt_version;

	msg_fmt_version = m_NCS_ENC_MSG_FMT_GET(info->info.enc.i_rem_svc_pvt_ver,
						CLMS_WRT_CLMA_SUBPART_VER_AT_MIN_MSG_FMT,
						CLMS_WRT_CLMA_SUBPART_VER_AT_MAX_MSG_FMT, CLMS_WRT_CLMA_MSG_FMT_ARRAY);
	if (0 == msg_fmt_version) {
		LOG_ER("msg_fmt_version FAILED!");
		return NCSCC_RC_FAILURE;
	}
	info->info.enc.o_msg_fmt_ver = msg_fmt_version;

	msg = (CLMSV_MSG *) info->info.enc.i_msg;
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
	ncs_encode_32bit(&p8, msg->evt_type);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	if (CLMSV_CLMS_TO_CLMA_API_RESP_MSG == msg->evt_type) {
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
		case CLMSV_INITIALIZE_RESP:
			total_bytes += clms_enc_initialize_rsp_msg(uba, msg);
			break;
		case CLMSV_FINALIZE_RESP:
			break;
		case CLMSV_TRACK_CURRENT_RESP:
			total_bytes += clms_enc_track_current_rsp_msg(uba, msg);
			break;
		case CLMSV_TRACK_STOP_RESP:
			break;
		case CLMSV_NODE_GET_RESP:
			if (msg->info.api_resp_info.rc == SA_AIS_OK)
				total_bytes += clms_enc_node_get_rsp_msg(uba, msg);
			break;
		case CLMSV_CLUSTER_JOIN_RESP:
			total_bytes += clmsv_encodeSaNameT(uba, &(msg->info.api_resp_info.param.node_name));
			break;
		case CLMSV_RESPONSE_RESP:
			break;
		default:
			TRACE("Unknown API RSP type = %d", msg->info.api_resp_info.type);
			goto err;

		}
	} else if (CLMSV_CLMS_TO_CLMA_CBK_MSG == msg->evt_type) {

		p8 = ncs_enc_reserve_space(uba, 8);
		if (!p8) {
			TRACE("p8 NULL!!!");
			return 0;
		}
		ncs_encode_32bit(&p8, msg->info.cbk_info.type);
		ncs_encode_32bit(&p8, msg->info.cbk_info.client_id);
		ncs_enc_claim_space(uba, 8);
		total_bytes += 8;

		TRACE_2("CLMSV_CLMS_TO_CLMA_CBK_MSG");
		switch (msg->info.cbk_info.type) {
		case CLMSV_TRACK_CBK:
			TRACE_2("encode Track cbk message");
			total_bytes += clms_enc_track_cbk_msg(uba, msg);
			break;
		case CLMSV_NODE_ASYNC_GET_CBK:
			TRACE_2("encode Ndoe async cbk message");
			total_bytes += clms_enc_node_async_get_cbk_msg(uba, msg);
			break;
		default:
			TRACE_2("Unknown callback type = %d!", msg->info.cbk_info.type);
			goto err;
		}
	} else if(CLMSV_CLMS_TO_CLMA_IS_MEMBER_MSG == msg->evt_type) {

		p8 = ncs_enc_reserve_space(uba, 12);
		if(!p8) {
			TRACE("ncs_enc_reserve_space Failed");
			goto err;
		}
		ncs_encode_32bit(&p8, msg->info.is_member_info.is_member);
		ncs_encode_32bit(&p8, msg->info.is_member_info.is_configured);
		ncs_encode_32bit(&p8, msg->info.is_member_info.client_id);
		ncs_enc_claim_space(uba, 12);
		total_bytes += 12;
	} else {
		TRACE("unknown msg type %d", msg->evt_type);
		goto err;
	}

	if (total_bytes == 0) {
		TRACE("Zero bytes encoded");
		goto err;
	}

	return NCSCC_RC_SUCCESS;

 err:
	return NCSCC_RC_FAILURE;
}

/**
* Decode the nodeup msg
**/
static uint32_t clms_dec_nodeup_msg(NCS_UBAID *uba, CLMSV_MSG * msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	uint8_t local_data[4];
	TRACE_ENTER();

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	msg->info.api_info.param.nodeup_info.node_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;
	total_bytes += clmsv_decodeSaNameT(uba, &(msg->info.api_info.param.nodeup_info.node_name));
	TRACE("nodename %s length %d", msg->info.api_info.param.nodeup_info.node_name.value,
	      msg->info.api_info.param.nodeup_info.node_name.length);

	TRACE("CLMSV_NODE_UP_MSG");
	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
 * Name          : clms_mds_dec
 *
 * Description   : MDS decode
 *
 * Arguments     : pointer to ncsmds_callback_info 
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t clms_mds_dec(struct ncsmds_callback_info *info)
{
	uint8_t *p8;
	CLMSV_CLMS_EVT *evt = NULL;
	NCS_UBAID *uba = info->info.dec.io_uba;
	uint8_t local_data[20];
	uint32_t total_bytes = 0;

	TRACE_ENTER();

	if (0 == m_NCS_MSG_FORMAT_IS_VALID(info->info.dec.i_msg_fmt_ver,
					   CLMS_WRT_CLMA_SUBPART_VER_AT_MIN_MSG_FMT,
					   CLMS_WRT_CLMA_SUBPART_VER_AT_MAX_MSG_FMT, CLMS_WRT_CLMA_MSG_FMT_ARRAY)) {
		TRACE("Wrong format version");
		goto err;
	}

    /* allocate an CLMSV_CLMS_EVENT now */
	if (NULL == (evt = calloc(1, sizeof(CLMSV_CLMS_EVT)))) {
		TRACE("calloc failed");
		goto err;
	}

	/* Assign the allocated event */
	info->info.dec.o_msg = (uint8_t *)evt;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	evt->info.msg.evt_type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	TRACE("evt->info.msg.evt_type %d", evt->info.msg.evt_type);

	if (CLMSV_CLMA_TO_CLMS_API_MSG == evt->info.msg.evt_type) {
		p8 = ncs_dec_flatten_space(uba, local_data, 4);
		evt->info.msg.info.api_info.type = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(uba, 4);
		total_bytes += 4;

		TRACE("evt->info.msg.info.api_info.type %d", evt->info.msg.info.api_info.type);
		/* FIX error handling for dec functions */
		switch (evt->info.msg.info.api_info.type) {
		case CLMSV_INITIALIZE_REQ:
			total_bytes += clms_dec_initialize_msg(uba, &evt->info.msg);
			break;
		case CLMSV_FINALIZE_REQ:
			total_bytes += clms_dec_finalize_msg(uba, &evt->info.msg);
			break;
		case CLMSV_TRACK_START_REQ:
			total_bytes += clms_dec_track_start_msg(uba, &evt->info.msg);
			break;
		case CLMSV_TRACK_STOP_REQ:
			total_bytes += clms_dec_track_stop_msg(uba, &evt->info.msg);
			break;
		case CLMSV_NODE_GET_REQ:
			total_bytes += clms_dec_node_get_msg(uba, &evt->info.msg);
			break;
		case CLMSV_NODE_GET_ASYNC_REQ:
			total_bytes += clms_dec_node_get_async_msg(uba, &evt->info.msg);
			break;
		case CLMSV_RESPONSE_REQ:
			total_bytes += clms_dec_response_msg(uba, &evt->info.msg);
			break;
		case CLMSV_CLUSTER_JOIN_REQ:
			/* Decode the nodeup mesg */
			TRACE("Node up message getting decoded");
			total_bytes += clms_dec_nodeup_msg(uba, &evt->info.msg);
			break;
		default:
			TRACE("Unknown API type = %d", evt->info.msg.info.api_info.type);
			break;
		}
		if (total_bytes == 4)
			goto err;

	} else {
		TRACE("unknown msg type = %d", (int)evt->info.msg.evt_type);
		goto err;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

 err:
	if(evt)
		free(evt);
	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;

}

/****************************************************************************
 * Name          : clms_mds_enc_flat
 *
 * Description   : MDS encode and flatten
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t clms_mds_enc_flat(struct ncsmds_callback_info *info)
{
	uint32_t rc;

	/* Retrieve info from the enc_flat */
	MDS_CALLBACK_ENC_INFO enc = info->info.enc_flat;
	/* Modify the MDS_INFO to populate enc */
	info->info.enc = enc;
	/* Invoke the regular mds_enc routine */
	rc = clms_mds_enc(info);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("mds_enc FAILED");
	}
	return rc;
}

/****************************************************************************
 * Name          : clms_mds_dec_flat
 *
 * Description   : MDS decode and flatten
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t clms_mds_dec_flat(struct ncsmds_callback_info *info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	/* Retrieve info from the dec_flat */
	MDS_CALLBACK_DEC_INFO dec = info->info.dec_flat;
	/* Modify the MDS_INFO to populate dec */
	info->info.dec = dec;
	/* Invoke the regular mds_dec routine */
	rc = clms_mds_dec(info);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("mds_dec FAILED ");
	}
	return rc;
}

/****************************************************************************
 * Name          : clms_mds_node_event
 *
 * Description   : MDS nodeup evt.
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t clms_mds_node_event(struct ncsmds_callback_info *mds_info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	CLMSV_CLMS_EVT *clmsv_evt = NULL;
	uint32_t node_id = 0;
	IPLIST *ip = NULL;
	TRACE_ENTER();

	TRACE("node_id %d,nodeup %d", mds_info->info.node_evt.node_id, mds_info->info.node_evt.node_chg);

	/* Send the message to clms */
	/* Node up Events will be taken care by clms nodeagent */

	node_id =  mds_info->info.node_evt.node_id;
	if (mds_info->info.node_evt.node_chg == NCSMDS_NODE_UP) {
		/* Store the ip info provided by MDS in the list.
		 * The releases < 4.4 were storing default address family
		 * as AF_INET4, any family other than AF_NET* was treated as invalid.
		 * Now, in 4.4 MDS provides info of AF_TIPC also as an address family, but
		 * this cannot yet be passed on to the clients yet because of checks
		 * in the CLM library that prohibits this.
		 * So, for backward compatibility reasons, when MDS provides
		 * AF_TIPC address family, we default it to AF_INET4 before
		 * sending it to the CLM clients.
		 */
		TRACE("Adding ipinformation to the ip list: %u", node_id);
		node_id = mds_info->info.node_evt.node_id;
		if ((ip = (IPLIST *)ncs_patricia_tree_get(&clms_cb->iplist, (uint8_t *)&node_id)) == NULL) {
			IPLIST *ip = (IPLIST *)calloc(1,sizeof(IPLIST));
			TRACE("node_id: %u not found, adding as a new node", ip->node_id);
			ip->node_id = mds_info->info.node_evt.node_id;
			ip->pat_node_id.key_info = (uint8_t *)&(ip->node_id);
			if (mds_info->info.node_evt.addr_family == OSAF_AF_TIPC) {
				ip->addr.family = 1; /* For backward compatibility */
				ip->addr.length = 0;
			} else {
				if (mds_info->info.node_evt.addr_family == AF_INET)
					ip->addr.family = SA_CLM_AF_INET;
				else if (mds_info->info.node_evt.addr_family == AF_INET6)
					ip->addr.family = SA_CLM_AF_INET6;
				else
					LOG_ER("Unsupported address family from MDS: %u",
							mds_info->info.node_evt.addr_family);

				ip->addr.length = mds_info->info.node_evt.length;
				if (ip->addr.length) {
					memcpy(ip->addr.value, mds_info->info.node_evt.ip_addr,
							 mds_info->info.node_evt.length);
				}
			}
			if (ncs_patricia_tree_add(&clms_cb->iplist, &ip->pat_node_id) != NCSCC_RC_SUCCESS)
				TRACE("node add failed for node_id: %u", ip->node_id);
		} else {
			ip->node_id = mds_info->info.node_evt.node_id;
			if (mds_info->info.node_evt.addr_family == OSAF_AF_TIPC) {
				ip->addr.family = 1; /* For backward compatibility */
				ip->addr.length = 0;
			 } else {
				ip->addr.family = mds_info->info.node_evt.addr_family;
				ip->addr.length = mds_info->info.node_evt.length;
				if (ip->addr.length) {
					memcpy(ip->addr.value, mds_info->info.node_evt.ip_addr,
							 mds_info->info.node_evt.length);
				}
			}
		}
	}

	if (mds_info->info.node_evt.node_chg == NCSMDS_NODE_DOWN) {

		if (NULL == (clmsv_evt = calloc(1, sizeof(CLMSV_CLMS_EVT)))) {
			rc = NCSCC_RC_FAILURE;
			TRACE("mem alloc FAILURE  ");
			goto done;
		}

		clmsv_evt->type = CLMSV_CLMS_MDS_NODE_EVT;
		clmsv_evt->info.node_mds_info.node_id = mds_info->info.node_evt.node_id;
		clmsv_evt->info.node_mds_info.nodeup = mds_info->info.node_evt.node_chg;

		rc = m_NCS_IPC_SEND(&clms_cb->mbx, clmsv_evt, MDS_SEND_PRIORITY_MEDIUM);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE("IPC send failed %d", rc);
			free(clmsv_evt);
		}
	}

 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : clms_mds_rcv
 *
 * Description   : MDS rcv evt.
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t clms_mds_rcv(struct ncsmds_callback_info *mds_info)
{
	CLMSV_CLMS_EVT *clmsv_evt = (CLMSV_CLMS_EVT *) mds_info->info.receive.i_msg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("Event type  %u", clmsv_evt->type);

	clmsv_evt->type = CLMSV_CLMS_CLMSV_MSG;
	clmsv_evt->cb_hdl = (uint32_t)mds_info->i_yr_svc_hdl;
	clmsv_evt->fr_node_id = mds_info->info.receive.i_node_id;
	clmsv_evt->fr_dest = mds_info->info.receive.i_fr_dest;
	clmsv_evt->rcvd_prio = mds_info->info.receive.i_priority;
	clmsv_evt->mds_ctxt = mds_info->info.receive.i_msg_ctxt;

	/* Send the message to clms */
	rc = m_NCS_IPC_SEND(&clms_cb->mbx, clmsv_evt, mds_info->info.receive.i_priority);
	if (rc != NCSCC_RC_SUCCESS)
		TRACE("IPC send failed %d", rc);

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : clms_mds_quiesced_ack
 *
 * Description   : MDS quised ack.
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t clms_mds_quiesced_ack(struct ncsmds_callback_info *mds_info)
{
	CLMSV_CLMS_EVT *clmsv_evt;

	if (clms_cb->is_quiesced_set != true)
		return NCSCC_RC_SUCCESS;

    /** allocate an CLMSV_CLMS_EVENT now **/
	if (NULL == (clmsv_evt = calloc(1, sizeof(CLMSV_CLMS_EVT)))) {
		TRACE("memory alloc FAILED");
		goto err;
	}

	/** Initialize the Event here **/
	clmsv_evt->type = CLSMV_CLMS_QUIESCED_ACK;
	clmsv_evt->cb_hdl = (uint32_t)mds_info->i_yr_svc_hdl;

	/* Push the event and we are done */
	if (NCSCC_RC_FAILURE == m_NCS_IPC_SEND(&clms_cb->mbx, clmsv_evt, NCS_IPC_PRIORITY_NORMAL)) {
		TRACE("ipc send failed");
		free(clmsv_evt);
		goto err;
	}

	return NCSCC_RC_SUCCESS;

 err:
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : clms_mds_svc_event
 *
 * Description   : MDS subscription evt.
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t clms_mds_svc_event(struct ncsmds_callback_info *info)
{
	CLMSV_CLMS_EVT *evt = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* First make sure that this event is indeed for us */
	if (info->info.svc_evt.i_your_id != NCSMDS_SVC_ID_CLMS) {
		TRACE("event not NCSMDS_SVC_ID_CLMS");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* If this evt was sent from CLMA act on this */
	if (info->info.svc_evt.i_change == NCSMDS_DOWN) {
			TRACE_8("MDS DOWN for CLMA dest: %" PRIx64 ", node ID: %x, svc_id: %d",
				info->info.svc_evt.i_dest, info->info.svc_evt.i_node_id, info->info.svc_evt.i_svc_id);

			/* As of now we are only interested in CLMA events */
			if (NULL == (evt = calloc(1, sizeof(CLMSV_CLMS_EVT)))) {
				rc = NCSCC_RC_FAILURE;
				TRACE("mem alloc FAILURE  ");
				goto done;
			}

	    /** Initialize the Event Header **/
			evt->cb_hdl = 0;
			evt->fr_node_id = info->info.svc_evt.i_node_id;
			evt->fr_dest = info->info.svc_evt.i_dest;

			if (info->info.svc_evt.i_svc_id == NCSMDS_SVC_ID_CLMA) {
				evt->type = CLMSV_CLMS_CLMA_DOWN;
				evt->info.mds_info.node_id = info->info.svc_evt.i_node_id;
				evt->info.mds_info.mds_dest_id = info->info.svc_evt.i_dest;
			} else if (info->info.svc_evt.i_svc_id == NCSMDS_SVC_ID_AVND) {
				evt->type = CLMSV_AVND_DOWN_EVT;
				evt->info.node_mds_info.node_id = info->info.svc_evt.i_node_id;
				evt->info.node_mds_info.nodeup = false;
			}
			if (evt->type) {
				/* Push the event and we are done */
				if (m_NCS_IPC_SEND(&clms_cb->mbx, evt, NCS_IPC_PRIORITY_HIGH) != NCSCC_RC_SUCCESS) {
					TRACE("ipc send failed");
					free(evt);
					rc = NCSCC_RC_FAILURE;
					goto done;
				}
			}
	}
 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : clms_mds_sys_evt
 *
 * Description   : MDS sys evt .
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t clms_mds_sys_event(struct ncsmds_callback_info *mds_info)
{
	/* Not supported now */
	TRACE("FAILED");
	return NCSCC_RC_FAILURE;
}

static uint32_t clms_mds_dummy(struct ncsmds_callback_info *info)
{

	/* Not supported now */
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : clms_mds_callback
 *
 * Description   : Callback Dispatcher for various MDS operations.
 *
 * Arguments     : cb   : CLMS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t clms_mds_callback(struct ncsmds_callback_info *info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	static NCSMDS_CALLBACK_API cb_set[MDS_CALLBACK_SVC_MAX] = {
		clms_mds_cpy,	/* MDS_CALLBACK_COPY      */
		clms_mds_enc,	/* MDS_CALLBACK_ENC       */
		clms_mds_dec,	/* MDS_CALLBACK_DEC       */
		clms_mds_enc_flat,	/* MDS_CALLBACK_ENC_FLAT  */
		clms_mds_dec_flat,	/* MDS_CALLBACK_DEC_FLAT  */
		clms_mds_rcv,	/* MDS_CALLBACK_RECEIVE   */
		clms_mds_svc_event,	/* MDS_CALLBACK_SVC_EVENT */
		clms_mds_sys_event,	/* MDS_CALLBACK_SYS_EVENT */
		clms_mds_quiesced_ack,	/* MDS_CALLBACK_QUIESCED_ACK */
		clms_mds_dummy,
		clms_mds_node_event
	};

	if (info->i_op <= MDS_CALLBACK_NODE_EVENT) {
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
 * Description   : This function created the VDEST for CLMS
 *
 * Arguments     : 
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mds_vdest_create(CLMS_CB * clms_cb)
{
	NCSVDA_INFO vda_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	memset(&vda_info, 0, sizeof(NCSVDA_INFO));
	clms_cb->vaddr = CLMS_VDEST_ID;
	vda_info.req = NCSVDA_VDEST_CREATE;
	vda_info.info.vdest_create.i_create_type = NCSVDA_VDEST_CREATE_SPECIFIC;
	vda_info.info.vdest_create.i_persistent = false;	/* Up-to-the application */
	vda_info.info.vdest_create.i_policy = NCS_VDEST_TYPE_DEFAULT;
	vda_info.info.vdest_create.info.specified.i_vdest = clms_cb->vaddr;

	/* Create the VDEST address */
	if (NCSCC_RC_SUCCESS != (rc = ncsvda_api(&vda_info))) {
		LOG_ER("VDEST_CREATE_FAILED");
		return rc;
	}

	/* Store the info returned by MDS */
	clms_cb->mds_hdl = vda_info.info.vdest_create.o_mds_pwe1_hdl;

	return rc;
}

/****************************************************************************
 * Name          : clms_mds_init
 *
 * Description   : This function creates the VDEST for clms and installs/suscribes
 *                 into MDS.
 *
 * Arguments     : cb   : CLMS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t clms_mds_init(CLMS_CB * cb)
{
	NCSMDS_INFO mds_info;
	uint32_t rc;
	MDS_SVC_ID svc[] = { NCSMDS_SVC_ID_CLMA, NCSMDS_SVC_ID_CLMNA, NCSMDS_SVC_ID_AVND };

	TRACE_ENTER();

	/* Create the VDEST for CLMS */
	if (NCSCC_RC_SUCCESS != (rc = mds_vdest_create(cb))) {
		LOG_ER(" clms_mds_init: named vdest create FAILED\n");
		return rc;
	}

	/* Set the role of MDS */
	if (cb->ha_state == SA_AMF_HA_ACTIVE)
		cb->mds_role = V_DEST_RL_ACTIVE;

	if (cb->ha_state == SA_AMF_HA_STANDBY)
		cb->mds_role = V_DEST_RL_STANDBY;

	if (NCSCC_RC_SUCCESS != (rc = clms_mds_change_role(cb))) {
		LOG_ER("MDS role change to %d FAILED\n", cb->mds_role);
		return rc;
	}

	/* Install your service into MDS */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CLMS;
	mds_info.i_op = MDS_INSTALL;
	mds_info.info.svc_install.i_yr_svc_hdl = 0;
	mds_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;
	mds_info.info.svc_install.i_svc_cb = clms_mds_callback;
	mds_info.info.svc_install.i_mds_q_ownership = false;
	mds_info.info.svc_install.i_mds_svc_pvt_ver = CLMS_SVC_PVT_SUBPART_VERSION;

	if (NCSCC_RC_SUCCESS != (rc = ncsmds_api(&mds_info))) {
		LOG_ER("MDS Install FAILED");
		return rc;
	}

	/* Now subscribe for CLMS events in MDS, TODO: WHy this? */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CLMS;
	mds_info.i_op = MDS_SUBSCRIBE;
	mds_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	mds_info.info.svc_subscribe.i_num_svcs = 3;
	mds_info.info.svc_subscribe.i_svc_ids = svc;

	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("MDS subscribe FAILED");
		return rc;
	}

	/* Now subscribe for NODEUP events in MDS */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CLMS;
	mds_info.i_op = MDS_NODE_SUBSCRIBE;

	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("MDS subscribe for node info FAILED");
		return rc;
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : clms_mds_change_role
 *
 * Description   : This function informs mds of our vdest role change 
 *
 * Arguments     : cb   : CLMS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t clms_mds_change_role(CLMS_CB * cb)
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
 * Description   : This function Destroys the Virtual destination of CLMS
 *
 * Arguments     : cb   : CLMS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mds_vdest_destroy(CLMS_CB * clms_cb)
{
	NCSVDA_INFO vda_info;
	uint32_t rc;

	memset(&vda_info, 0, sizeof(NCSVDA_INFO));
	vda_info.req = NCSVDA_VDEST_DESTROY;
	vda_info.info.vdest_destroy.i_vdest = clms_cb->vaddr;

	if (NCSCC_RC_SUCCESS != (rc = ncsvda_api(&vda_info))) {
		LOG_ER("NCSVDA_VDEST_DESTROY failed");
		return rc;
	}

	return rc;
}

/****************************************************************************
 * Name          : clms_mds_finalize
 *
 * Description   : This function un-registers the CLMS Service with MDS.
 *
 * Arguments     : Uninstall CLMS from MDS.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t clms_mds_finalize(CLMS_CB * cb)
{
	NCSMDS_INFO mds_info;
	uint32_t rc;

	/* Un-install CLMS service from MDS */
	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CLMS;
	mds_info.i_op = MDS_UNINSTALL;

	rc = ncsmds_api(&mds_info);

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("MDS_UNINSTALL_FAILED");
		return rc;
	}

	/* Destroy the virtual Destination of CLMS */
	rc = mds_vdest_destroy(cb);
	return rc;
}

/****************************************************************************
  Name          : clms_mds_msg_send
 
  Description   : This routine sends a message to CLMA. The send 
                  operation may be a 'normal' send or a synchronous call that 
                  blocks until the response is received from CLMA.
 
  Arguments     : cb  - ptr to the CLMA CB
                  i_msg - ptr to the CLMSv message
                  dest  - MDS destination to send to.
                  mds_ctxt - ctxt for synch mds req-resp.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE/timeout
 
  Notes         : None.
******************************************************************************/

uint32_t clms_mds_msg_send(CLMS_CB * cb,
			CLMSV_MSG * msg,
			MDS_DEST *dest, MDS_SYNC_SND_CTXT *mds_ctxt, MDS_SEND_PRIORITY_TYPE prio, NCSMDS_SVC_ID svc_id)
{
	NCSMDS_INFO mds_info;
	MDS_SEND_INFO *send_info = &mds_info.info.svc_send;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* populate the mds params */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_CLMS;
	mds_info.i_op = MDS_SEND;

	send_info->i_msg = msg;
	send_info->i_to_svc = svc_id;	/* NCSMDS_SVC_ID_CLMNA or NCSMDS_SVC_ID_CLMA */
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
	if (rc != NCSCC_RC_SUCCESS)
		LOG_IN("mds send returned: %u", rc);

	TRACE_LEAVE();
	return rc;
}
