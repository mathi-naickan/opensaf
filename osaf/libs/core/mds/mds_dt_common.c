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
 * licensing terms.z
 *
 * Author(s): GoAhead Software
 *
 */

#include "mds_dt.h"
#include "mds_log.h"
#include "ncssysf_def.h"
#include "ncssysf_tsk.h"
#include "ncssysf_mem.h"

static SYSF_MBX mdtm_mbx_common;
static MDTM_TX_TYPE mdtm_transport;
static uns32 mdtm_fill_data(MDTM_REASSEMBLY_QUEUE *reassem_queue, uns8 *buffer, uns16 len, uns8 enc_type);
static MDTM_REASSEMBLY_QUEUE *mdtm_check_reassem_queue(uns32 seq_num, MDS_DEST id);
static MDTM_REASSEMBLY_QUEUE *mdtm_add_reassemble_queue(uns32 seq_num, MDS_DEST id);
static uns32 mdtm_del_reassemble_queue(uns32 seq_num, MDS_DEST id);
/****************************************************************************
 *
 * Function Name: mdtm_process_reassem_timer_event
 *
 * Purpose:
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
uns32 mdtm_process_reassem_timer_event(uns32 seq_num, MDS_DEST id)
{
	uns32 status = 0;
	status = mdtm_del_reassemble_queue(seq_num, id);
	return status;
}

/*********************************************************

  Function NAME: mdtm_add_to_ref_tbl

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mdtm_add_to_ref_tbl(MDS_SVC_HDL svc_hdl, MDS_SUBTN_REF_VAL ref)
{
	MDTM_REF_HDL_LIST *ref_ptr = NULL, *mov_ptr = NULL;
	mov_ptr = mdtm_ref_hdl_list_hdr;
	ref_ptr = m_MMGR_ALLOC_HDL_LIST;
	if (ref_ptr == NULL) {
		m_MDS_LOG_ERR("MDTM: Memory allocation failed for HDL list\n");
		return NCSCC_RC_FAILURE;
	}
	memset(ref_ptr, 0, sizeof(MDTM_REF_HDL_LIST));
	ref_ptr->ref_val = ref;
	ref_ptr->svc_hdl = svc_hdl;

	if (mov_ptr == NULL) {
		ref_ptr->next = NULL;
		mdtm_ref_hdl_list_hdr = ref_ptr;
		return NCSCC_RC_SUCCESS;
	}

	/* adding in the beginning */
	ref_ptr->next = mov_ptr;
	mdtm_ref_hdl_list_hdr = ref_ptr;

	m_MDS_LOG_INFO("MDTM: Successfully added in HDL list\n");

	return NCSCC_RC_SUCCESS;
}

/*********************************************************

  Function NAME: mdtm_get_from_ref_tbl

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mdtm_get_from_ref_tbl(MDS_SUBTN_REF_VAL ref_val, MDS_SVC_HDL *svc_hdl)
{
	MDTM_REF_HDL_LIST *mov_ptr = NULL;
	mov_ptr = mdtm_ref_hdl_list_hdr;

	if (mov_ptr == NULL) {
		*svc_hdl = 0;
		return NCSCC_RC_FAILURE;
	}
	while (mov_ptr != NULL) {
		if (ref_val == mov_ptr->ref_val) {
			*svc_hdl = mov_ptr->svc_hdl;
			return NCSCC_RC_SUCCESS;
		}
		mov_ptr = mov_ptr->next;
	}
	*svc_hdl = 0;
	return NCSCC_RC_FAILURE;
}

/*********************************************************

  Function NAME: mdtm_del_from_ref_tbl

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mdtm_del_from_ref_tbl(MDS_SUBTN_REF_VAL ref)
{
	MDTM_REF_HDL_LIST *back, *mov_ptr;

	/* FIX: Earlier loop was not resetting "mdtm_ref_hdl_list_hdr" in 
	 **      all case. Hence, loop rewritten : PM : 13/12/05
	 */
	for (back = NULL, mov_ptr = mdtm_ref_hdl_list_hdr; mov_ptr != NULL; back = mov_ptr, mov_ptr = mov_ptr->next) {	/* Safe because we quit loop after deletion */
		if (ref == mov_ptr->ref_val) {
			/* STEP: Detach "mov_ptr" from linked-list */
			if (back == NULL) {
				/* The head node is being deleted */
				mdtm_ref_hdl_list_hdr = mov_ptr->next;
			} else {
				back->next = mov_ptr->next;
			}

			/* STEP: Detach "mov_ptr" from linked-list */
			m_MMGR_FREE_HDL_LIST(mov_ptr);
			mov_ptr = NULL;
			m_MDS_LOG_INFO("MDTM: Successfully deleted HDL list\n");
			return NCSCC_RC_SUCCESS;
		}
	}
	m_MDS_LOG_ERR("MDTM: No matching entry found in HDL list\n");
	return NCSCC_RC_FAILURE;
}

/*********************************************************

  Function NAME: mdtm_process_recv_message_common

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mdtm_process_recv_message_common(uns8 flag, uns8 *buffer, uns16 len, uns64 transport_adest, uns32 seq_num_check,
				       uns32 *buff_dump)
{
	MDTM_REASSEMBLY_QUEUE *reassem_queue = NULL;
	MDS_PWE_HDL pwe_hdl;
	MDS_SVC_HDL dest_svc_hdl = 0;
	uns32 seq_num = 0;
	uns16 dest_svc_id = 0, src_svc_id = 0;
	uns16 pwe_id = 0;
	uns16 dest_vdest_id = 0, src_vdest_id = 0;
	uns8 msg_snd_type, enc_type;

	uns32 node_status = 0;
	MDS_DEST adest = 0;

	if (MDTM_TX_TYPE_TIPC == mdtm_transport) {
		node_status = m_MDS_CHECK_TIPC_NODE_ID_RANGE((uns32)(transport_adest >> 32));

		if (NCSCC_RC_SUCCESS == node_status) {
			adest =
			    ((((uns64)(m_MDS_GET_NCS_NODE_ID_FROM_TIPC_NODE_ID((NODE_ID)(transport_adest >> 32)))) << 32) |
			     (uns32)(transport_adest));
		} else {
			m_MDS_LOG_ERR
			    ("MDTM: Dropping  the recd message, as the TIPC NODEid is not in the prescribed range=0x%08x",
			     (NODE_ID)(transport_adest >> 32));
			return NCSCC_RC_FAILURE;
		}
	} else if (MDTM_TX_TYPE_TCP == mdtm_transport) {
		adest = transport_adest;
	} else {
		m_MDS_LOG_ERR("MDTM: unsupported transport =%d", mdtm_transport);
		abort();
	}

	if (MDTM_DIRECT == flag) {
		uns32 xch_id = 0;
		uns8 prot_ver = 0;

		/* We receive buffer pointer starting from MDS HDR only */
		uns8 *data = NULL;
		uns32 svc_seq_num = 0;
		uns16 len_mds_hdr = 0;

		data = buffer;

		prot_ver = ncs_decode_8bit(&data);
		len_mds_hdr = ncs_decode_16bit(&data);

		/* Length Checks */

		/* Check whether total lenght of message is not less than MDS header length */
		if (len < len_mds_hdr) {
			m_MDS_LOG_ERR
			    ("MDTM: Message recd (Non Fragmented) len is less than the MDS HDR len  adest=<0x%llx> len =%d len_mds_hdr=%",
			     len, len_mds_hdr, transport_adest);

			return NCSCC_RC_FAILURE;
		}
		/* Check whether mds header length received is not less than mds header length of this instance */
		if (len_mds_hdr < MDS_HDR_LEN) {
			m_MDS_LOG_ERR
			    ("MDTM:Mds hdr len of recd msg (Non frag) = %d is less than local mds hdr len = %d",
			     len_mds_hdr, MDS_HDR_LEN);
			return NCSCC_RC_FAILURE;
		}

		data = &buffer[MDS_HEADER_PWE_ID_POSITION];

		pwe_id = ncs_decode_16bit(&data);

		data = NULL;
		data = &buffer[MDS_HEADER_RCVR_VDEST_ID_POSITION];

		dest_vdest_id = ncs_decode_16bit(&data);

		data = NULL;
		data = &buffer[MDS_HEADER_RCVR_SVC_ID_POSITION];

		dest_svc_id = ncs_decode_16bit(&data);

		data = NULL;
		data = &buffer[MDS_HEADER_MSG_TYPE_POSITION];

		msg_snd_type = (ncs_decode_8bit(&data)) & 0x3f;

		/* Search whether the Destination exists or not , SVC,PWE,VDEST */
		pwe_hdl = m_MDS_GET_PWE_HDL_FROM_VDEST_HDL_AND_PWE_ID((MDS_VDEST_HDL)dest_vdest_id, pwe_id);

		if (NCSCC_RC_SUCCESS != mds_svc_tbl_get_svc_hdl(pwe_hdl, dest_svc_id, &dest_svc_hdl)) {
			*buff_dump = 0;	/* For future hack */
			m_MDS_LOG_ERR("MDTM: Service Doesnt exists for the message recd=%d, adest=<0x%llx>\n",
				      dest_svc_id, transport_adest);
			return NCSCC_RC_FAILURE;
		}

		if (mds_svc_tbl_get_role(dest_svc_hdl) != NCSCC_RC_SUCCESS) {
			switch (msg_snd_type) {
			case MDS_SENDTYPE_SND:
			case MDS_SENDTYPE_SNDRSP:
			case MDS_SENDTYPE_SNDACK:
			case MDS_SENDTYPE_BCAST:
				m_MDS_LOG_ERR
				    ("MDTM: Recd Message SVC is in standby so dropping the message:Dest-Svc = %d,%d\n",
				     dest_svc_id, dest_vdest_id);
				return NCSCC_RC_FAILURE;
				break;
			default:
				break;
			}
		}

		data = NULL;
		data = &buffer[MDS_HEADER_MSG_TYPE_POSITION];

		enc_type = ((ncs_decode_8bit(&data)) & 0xc0) >> 6;

		if (enc_type > MDS_ENC_TYPE_DIRECT_BUFF) {
			*buff_dump = 0;	/* For future hack */
			m_MDS_LOG_ERR("MDTM: Encoding unsupported, adest=<0x%016llx>\n", transport_adest);
			return NCSCC_RC_FAILURE;
		}

		/* Allocate the memory for reassem_queue */
		reassem_queue = m_MMGR_ALLOC_REASSEM_QUEUE;

		if (reassem_queue == NULL) {
			m_MDS_LOG_ERR("MDTM: Memory allocation failed for reassem_queue\n");
			return NCSCC_RC_FAILURE;
		}
		memset(reassem_queue, 0, sizeof(MDTM_REASSEMBLY_QUEUE));

		data = NULL;
		data = &buffer[MDS_HEADER_SNDR_VDEST_ID_POSITION];

		src_vdest_id = ncs_decode_16bit(&data);

		data = NULL;
		data = &buffer[MDS_HEADER_SNDR_SVC_ID_POSITION];

		src_svc_id = ncs_decode_16bit(&data);

		data = NULL;
		data = &buffer[MDS_HEADER_SEQ_NUM_POSITION];

		svc_seq_num = ncs_decode_32bit(&data);

		reassem_queue->key.id = transport_adest;

		reassem_queue->recv.src_adest = adest;

		switch (msg_snd_type) {
		case MDS_SENDTYPE_SNDRSP:
		case MDS_SENDTYPE_SNDRACK:
		case MDS_SENDTYPE_SNDACK:
		case MDS_SENDTYPE_REDRSP:
		case MDS_SENDTYPE_REDRACK:
		case MDS_SENDTYPE_REDACK:
		case MDS_SENDTYPE_ACK:
		case MDS_SENDTYPE_RACK:
		case MDS_SENDTYPE_RSP:
		case MDS_SENDTYPE_RRSP:
			{
				data = NULL;
				data = &buffer[MDS_HEADER_EXCHG_ID_POSITION];
				xch_id = ncs_decode_32bit(&data);
			}
			break;

		default:
			/* do nothing */
			break;
		}

		data = &buffer[MDS_HEADER_APP_VERSION_ID_POSITION];
		reassem_queue->recv.msg_fmt_ver = ncs_decode_16bit(&data);	/* For the version field */

		reassem_queue->recv.exchange_id = xch_id;
		reassem_queue->next_frag_num = 0;
		reassem_queue->recv.dest_svc_hdl = dest_svc_hdl;
		reassem_queue->recv.src_svc_id = src_svc_id;
		reassem_queue->recv.src_pwe_id = pwe_id;
		reassem_queue->recv.src_vdest = src_vdest_id;
		reassem_queue->svc_sequence_num = svc_seq_num;
		reassem_queue->recv.msg.encoding = enc_type;
		reassem_queue->recv.pri = (prot_ver & MDTM_PRI_MASK) + 1;
		reassem_queue->recv.snd_type = msg_snd_type;

		m_MDS_LOG_DBG("MDTM: Recd Unfragmented message with SVC Seq num =%d, from src_Tipc_id=<%llx>",
			      svc_seq_num, transport_adest);

		if (msg_snd_type == MDS_SENDTYPE_ACK) {
			/* NOTE: Version in ACK message is ignored */
			if (len != len_mds_hdr) {
				/* Size of Payload data in ACK message should be zero, If not its an error */
				m_MDS_LOG_ERR("MDTM: ACK message contains payload data, Total Len=%d,  len_mds_hdr=%d",
					      len, len_mds_hdr);
				m_MMGR_FREE_REASSEM_QUEUE(reassem_queue);
				return NCSCC_RC_FAILURE;
			}
		} else {
			/* only for non ack cases */

			/* Drop message if version is not 1 and return */
			/*if(reassem_queue->recv.msg_fmt_ver!=1) 
			   {
			   m_MDS_LOG_ERR("MDTM: Unable to process the recd message due to wrong version=%d",reassem_queue->recv.msg_fmt_ver);
			   m_MMGR_FREE_REASSEM_QUEUE(reassem_queue);
			   return NCSCC_RC_FAILURE;
			   } */

			if (NCSCC_RC_SUCCESS !=
			    mdtm_fill_data(reassem_queue, &buffer[len_mds_hdr], (len - (len_mds_hdr)), enc_type)) {
				m_MDS_LOG_ERR
				    ("MDTM: Unable to process the recd message due to prob in mdtm_fill_data\n");
				m_MMGR_FREE_REASSEM_QUEUE(reassem_queue);
				return NCSCC_RC_FAILURE;
			}

			/* Depending on msg type if flat or full enc apply dec space, for setting the uba to decode by user */
			if (reassem_queue->recv.msg.encoding == MDS_ENC_TYPE_FLAT) {
				ncs_dec_init_space(&reassem_queue->recv.msg.data.flat_uba,
						   reassem_queue->recv.msg.data.flat_uba.start);
			} else if (reassem_queue->recv.msg.encoding == MDS_ENC_TYPE_FULL) {
				ncs_dec_init_space(&reassem_queue->recv.msg.data.fullenc_uba,
						   reassem_queue->recv.msg.data.fullenc_uba.start);
			}
			/* for direct buff and cpy encoding modes we do nothig */
		}

		/* Call upper layer */
		m_MDS_LOG_INFO("MDTM: Sending data to upper layer for a single recd message\n");

		mds_mcm_ll_data_rcv(&reassem_queue->recv);

		/* Free Memory allocated to this structure */
		m_MMGR_FREE_REASSEM_QUEUE(reassem_queue);

		return NCSCC_RC_SUCCESS;

	} else if (MDTM_REASSEMBLE == flag) {
		/*
		   Message seq no. (32-bit) debug assist) | MoreFrags(1-bit)|
		   Fragment Number(15-bit) | Fragment Size(16-bit)
		 */

		uns32 xch_id = 0;
		uns8 prot_ver = 0;

		uns8 *data = NULL;
		uns32 svc_seq_num = 0;
		uns16 len_mds_hdr = 0;
		MDS_TMR_REQ_INFO *tmr_req_info = NULL;

		/* Added for seqnum check */
		data = &buffer[MDTM_FRAG_HDR_LEN];

		prot_ver = ncs_decode_8bit(&data);

		data = NULL;
		data = &buffer[MDTM_FRAG_HDR_LEN + MDS_HEADER_HDR_LEN_POSITION];
		len_mds_hdr = ncs_decode_16bit(&data);

		/* Length Checks */

		/* Check whether total lenght of message is not less than or equal to MDS header length and MDTM frag header length */
		if (len <= (len_mds_hdr + MDTM_FRAG_HDR_LEN)) {
			m_MDS_LOG_ERR
			    ("MDTM: Message recd (Fragmented First Pkt) len is less than or equal to the sum of (len_mds_hdr+MDTM_FRAG_HDR_LEN) len, adest=<0x%llx>",
			     transport_adest);
			return NCSCC_RC_FAILURE;
		}
		/* Check whether mds header length received is not less than mds header length of this instance */
		if (len_mds_hdr < MDS_HDR_LEN) {
			m_MDS_LOG_DBG
			    ("MDTM:Mds hdr len of recd msg(Frag first pkt) = %d is less than local mds hdr len = %d",
			     len_mds_hdr, MDS_HDR_LEN);
			return NCSCC_RC_FAILURE;
		}
		data = &buffer[MDS_HEADER_PWE_ID_POSITION + MDTM_FRAG_HDR_LEN];

		pwe_id = ncs_decode_16bit(&data);

		data = NULL;
		data = &buffer[MDS_HEADER_RCVR_VDEST_ID_POSITION + MDTM_FRAG_HDR_LEN];

		dest_vdest_id = ncs_decode_16bit(&data);

		data = NULL;
		data = &buffer[MDS_HEADER_RCVR_SVC_ID_POSITION + MDTM_FRAG_HDR_LEN];

		dest_svc_id = ncs_decode_16bit(&data);

		data = NULL;
		data = &buffer[MDS_HEADER_MSG_TYPE_POSITION + MDTM_FRAG_HDR_LEN];

		msg_snd_type = (ncs_decode_8bit(&data)) & 0x3f;

		/* Search whether the Destination exists or not , SVC,PWE,VDEST */
		pwe_hdl = m_MDS_GET_PWE_HDL_FROM_VDEST_HDL_AND_PWE_ID((MDS_VDEST_HDL)dest_vdest_id, pwe_id);
		if (NCSCC_RC_SUCCESS != mds_svc_tbl_get_svc_hdl(pwe_hdl, dest_svc_id, &dest_svc_hdl)) {
			*buff_dump = 0;	/* For future hack */
			m_MDS_LOG_ERR("MDTM: Service Doesnt exists for the message recd=%d\n", dest_svc_id);
			return NCSCC_RC_FAILURE;
		}

		if (mds_svc_tbl_get_role(dest_svc_hdl) != NCSCC_RC_SUCCESS) {
			switch (msg_snd_type) {
			case MDS_SENDTYPE_SND:
			case MDS_SENDTYPE_SNDRSP:
			case MDS_SENDTYPE_SNDACK:
			case MDS_SENDTYPE_BCAST:
				m_MDS_LOG_ERR
				    ("MDTM: Recd Message SVC is in standby so dropping the message:Dest-Svc = %d,%d\n",
				     dest_svc_id, dest_vdest_id);
				return NCSCC_RC_FAILURE;
				break;
			default:
				break;
			}
		}

		data = NULL;
		data = &buffer[MDS_HEADER_MSG_TYPE_POSITION + MDTM_FRAG_HDR_LEN];

		enc_type = ((ncs_decode_8bit(&data)) & 0xc0) >> 6;

		if (enc_type > MDS_ENC_TYPE_DIRECT_BUFF) {
			*buff_dump = 0;	/* For future hack */
			m_MDS_LOG_ERR("MDTM: Encoding unsupported\n");
			return NCSCC_RC_FAILURE;
		}

		data = NULL;
		data = buffer;

		seq_num = ncs_decode_32bit(&data);
		reassem_queue = mdtm_add_reassemble_queue(seq_num, transport_adest);

		if (reassem_queue == NULL) {
			m_MDS_LOG_ERR("MDTM: New reassem queue creation failed\n");
			return NCSCC_RC_FAILURE;
		}

		data = NULL;
		data = &buffer[MDS_HEADER_SNDR_VDEST_ID_POSITION + MDTM_FRAG_HDR_LEN];

		src_vdest_id = ncs_decode_16bit(&data);

		data = NULL;
		data = &buffer[MDS_HEADER_SNDR_SVC_ID_POSITION + MDTM_FRAG_HDR_LEN];

		src_svc_id = ncs_decode_16bit(&data);

		data = NULL;
		data = &buffer[MDS_HEADER_SEQ_NUM_POSITION + MDTM_FRAG_HDR_LEN];

		svc_seq_num = ncs_decode_32bit(&data);

		reassem_queue->key.id = transport_adest;

		reassem_queue->recv.src_adest = adest;

		switch (msg_snd_type) {
		case MDS_SENDTYPE_SNDRSP:
		case MDS_SENDTYPE_SNDRACK:
		case MDS_SENDTYPE_SNDACK:
		case MDS_SENDTYPE_REDRSP:
		case MDS_SENDTYPE_REDRACK:
		case MDS_SENDTYPE_REDACK:
		case MDS_SENDTYPE_ACK:
		case MDS_SENDTYPE_RACK:
		case MDS_SENDTYPE_RSP:
		case MDS_SENDTYPE_RRSP:
			{
				data = NULL;
				data = &buffer[MDS_HEADER_EXCHG_ID_POSITION + MDTM_FRAG_HDR_LEN];
				xch_id = ncs_decode_32bit(&data);
			}
			break;

		default:
			/* do nothing */
			break;

		}

		data = &buffer[MDS_HEADER_APP_VERSION_ID_POSITION + MDTM_FRAG_HDR_LEN];
		reassem_queue->recv.msg_fmt_ver = ncs_decode_16bit(&data);	/* For the version field */

		reassem_queue->recv.exchange_id = xch_id;
		reassem_queue->next_frag_num = 2;
		reassem_queue->recv.dest_svc_hdl = dest_svc_hdl;
		reassem_queue->recv.src_svc_id = src_svc_id;
		reassem_queue->recv.src_pwe_id = pwe_id;
		reassem_queue->recv.src_vdest = src_vdest_id;
		reassem_queue->svc_sequence_num = svc_seq_num;
		reassem_queue->key.frag_sequence_num = seq_num;
		reassem_queue->recv.msg.encoding = enc_type;
		reassem_queue->recv.pri = (prot_ver & MDTM_PRI_MASK) + 1;
		reassem_queue->to_be_dropped = FALSE;
		reassem_queue->recv.snd_type = msg_snd_type;

		m_MDS_LOG_INFO("MDTM: Reassembly started\n");

		m_MDS_LOG_DBG
		    ("MDTM: Recd fragmented message(first frag) with Frag Seqnum=%d SVC Seq num =%d, from src_Tipc_id=<%llx>",
		     seq_num, svc_seq_num, transport_adest);

		if ((len - (len_mds_hdr + MDTM_FRAG_HDR_LEN)) > 0) {
			if (NCSCC_RC_SUCCESS != mdtm_fill_data(reassem_queue, &buffer[len_mds_hdr + MDTM_FRAG_HDR_LEN],
							       (len - (len_mds_hdr + MDTM_FRAG_HDR_LEN)), enc_type)) {
				m_MDS_LOG_ERR("MDTM: MDtm fill data failed\n");
				mdtm_del_reassemble_queue(seq_num, transport_adest);
				return NCSCC_RC_FAILURE;
			}
		} else {
			m_MDS_LOG_ERR("MDTM: No Payload Data present, Total Len=%d, sum of frag_hdr and mds_hdr=%d",
				      len, (len_mds_hdr + MDTM_FRAG_HDR_LEN));
			mdtm_del_reassemble_queue(seq_num, transport_adest);
			return NCSCC_RC_FAILURE;
		}

		/*start the timer */
		tmr_req_info = m_MMGR_ALLOC_TMR_INFO;
		if (tmr_req_info == NULL) {
			m_MDS_LOG_ERR("MDTM: Memory allocation for timer request failed\n");
			return NCSCC_RC_FAILURE;
		}
		memset(tmr_req_info, 0, sizeof(MDS_TMR_REQ_INFO));
		tmr_req_info->type = MDS_REASSEMBLY_TMR;
		tmr_req_info->info.reassembly_tmr_info.seq_no = reassem_queue->key.frag_sequence_num;
		tmr_req_info->info.reassembly_tmr_info.id = reassem_queue->key.id;

		reassem_queue->tmr_info = tmr_req_info;

		m_NCS_TMR_CREATE(reassem_queue->tmr, MDTM_REASSEMBLE_TMR_VAL,
				 (TMR_CALLBACK)mds_tmr_callback, (void *)NULL);

		m_MDS_LOG_DBG(" Reassembly tmr=0x%08x", reassem_queue->tmr);
		reassem_queue->tmr_hdl =
		    ncshm_create_hdl(NCS_HM_POOL_ID_COMMON, NCS_SERVICE_ID_COMMON,
				     (NCSCONTEXT)(reassem_queue->tmr_info));

		m_NCS_TMR_START(reassem_queue->tmr, MDTM_REASSEMBLE_TMR_VAL,
				(TMR_CALLBACK)mds_tmr_callback, (void *)(long)(reassem_queue->tmr_hdl));
		m_MDS_LOG_DBG
		    ("MCM_DB:RecvMessage:TimerStart:Reassemble:Hdl=0x%08x:SrcSvcId=%d:SrcVdest=%d,DestSvcHdl=0x%08x\n",
		     reassem_queue->tmr_hdl, src_svc_id, src_vdest_id, dest_svc_hdl);
	}
	return NCSCC_RC_SUCCESS;
}

uns32 mdtm_attach_mbx(SYSF_MBX mbx)
{
	mdtm_mbx_common = mbx;
	return NCSCC_RC_SUCCESS;
}

uns32 mdtm_set_transport(MDTM_TX_TYPE transport)
{
	mdtm_transport = transport;
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * Function Name: mds_tmr_callback
 *
 * Purpose: Used for posting a message when timer expires
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
uns32 mds_tmr_callback(NCSCONTEXT tmr_info_hdl)
{
	/* Now Queue the message in the Mailbox */
	MDS_MBX_EVT_INFO *mbx_tmr_info = NULL;

	mbx_tmr_info = m_MMGR_ALLOC_MBX_EVT_INFO;
	memset(mbx_tmr_info, 0, sizeof(MDS_MBX_EVT_INFO));

	mbx_tmr_info->type = MDS_MBX_EVT_TIMER_EXPIRY;
	mbx_tmr_info->info.tmr_info_hdl = (uns32)((long)tmr_info_hdl);

	if ((m_NCS_IPC_SEND(&mdtm_mbx_common, mbx_tmr_info, NCS_IPC_PRIORITY_NORMAL)) != NCSCC_RC_SUCCESS) {
		/* Message Queuing failed, free the msg. In TDS they are relaseing the task
		 * ,releasing IPC and freeing the TDS CB shall we do that same ....??   */
		/* Do we need to free the UB also??? */
		m_MDS_LOG_ERR("MDTM: Tmr Mailbox IPC_SEND Failed\n");
		m_MMGR_FREE_MBX_EVT_INFO(mbx_tmr_info);
		m_MDS_LOG_ERR("Tmr Mailbox IPC_SEND Failed\n");
		return NCSCC_RC_FAILURE;
	} else {
		m_MDS_LOG_INFO("MDTM: Tmr mailbox IPC_SEND Success\n");
		return NCSCC_RC_SUCCESS;
	}
}

/****************************************************************************
 *
 * Function Name: mds_tmr_mailbox_processing
 *
 * Purpose: Used for processing messages in mailbox
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
uns32 mds_tmr_mailbox_processing(void)
{
	MDS_MBX_EVT_INFO *mbx_evt_info;
	MDS_TMR_REQ_INFO *tmr_req_info = NULL;
	uns32 status = NCSCC_RC_SUCCESS;

	/* Now Parse thru the mailbox and send all the messages */
	mbx_evt_info = (MDS_MBX_EVT_INFO *)(m_NCS_IPC_NON_BLK_RECEIVE(&mdtm_mbx_common, NULL));

	if (mbx_evt_info == NULL) {
		m_MDS_LOG_ERR("MDTM: Tmr Mailbox IPC_NON_BLK_RECEIVE Failed");
		return NCSCC_RC_FAILURE;
	} else if (mbx_evt_info->type == MDS_MBX_EVT_TIMER_EXPIRY) {
		tmr_req_info =
		    (MDS_TMR_REQ_INFO *)ncshm_take_hdl(NCS_SERVICE_ID_COMMON, (uns32)(mbx_evt_info->info.tmr_info_hdl));
		if (tmr_req_info == NULL) {
			m_MDS_LOG_INFO("MDTM: Tmr Mailbox Processing:Handle invalid (=0x%08x)",
					 mbx_evt_info->info.tmr_info_hdl);
			/* return NCSCC_RC_SUCCESS; */	/* Fall through to free memory */
		} else {
			switch (tmr_req_info->type) {
			case MDS_QUIESCED_TMR:
				m_MDS_LOG_DBG("MDTM: Tmr Mailbox Processing:QuiescedTimer Hdl=0x%08x",
					      mbx_evt_info->info.tmr_info_hdl);
				mds_mcm_quiesced_tmr_expiry(tmr_req_info->info.quiesced_tmr_info.vdest_id);
				break;

			case MDS_SUBTN_TMR:
				m_MDS_LOG_DBG("MDTM: Tmr Mailbox Processing:Subtn Tmr Hdl=0x%08x",
					      mbx_evt_info->info.tmr_info_hdl);
				mds_mcm_subscription_tmr_expiry(tmr_req_info->info.subtn_tmr_info.svc_hdl,
								tmr_req_info->info.subtn_tmr_info.sub_svc_id);
				break;

			case MDS_AWAIT_ACTIVE_TMR:
				m_MDS_LOG_DBG("MDTM: Tmr Mailbox Processing:Active Tmr Hdl=0x%08x",
					      mbx_evt_info->info.tmr_info_hdl);
				mds_mcm_await_active_tmr_expiry(tmr_req_info->info.await_active_tmr_info.svc_hdl,
								tmr_req_info->info.await_active_tmr_info.sub_svc_id,
								tmr_req_info->info.await_active_tmr_info.vdest_id);
				break;
			case MDS_REASSEMBLY_TMR:
				m_MDS_LOG_DBG("MDTM: Tmr Mailbox Processing:Reassemble Tmr Hdl=0x%08x",
					      mbx_evt_info->info.tmr_info_hdl);
				mdtm_process_reassem_timer_event(tmr_req_info->info.reassembly_tmr_info.seq_no,
								 tmr_req_info->info.reassembly_tmr_info.id);
				break;

			default:
				m_MDS_LOG_ERR("MDTM: Tmr Mailbox Processing:JunkTmr Hdl=0x%08x",
					      mbx_evt_info->info.tmr_info_hdl);
				break;
			}
			/* Give Handle and Destroy Handle */
			ncshm_give_hdl((uns32)mbx_evt_info->info.tmr_info_hdl);
			ncshm_destroy_hdl(NCS_SERVICE_ID_COMMON, (uns32)mbx_evt_info->info.tmr_info_hdl);

			/* Free timer req info */
			m_MMGR_FREE_TMR_INFO(tmr_req_info);
		}
	} else if (mbx_evt_info->type == MDS_MBX_EVT_DESTROY) {
		/* No destruction processing. Due to already existing implementation, the 
		   destroying thread performs the full destruction. This messagae is merely to 
		   wake up the MDTM thread so that it may process the destroy-command.  
		   We need to acknowledge that this event has been processed */
		m_NCS_SEL_OBJ_IND(mbx_evt_info->info.destroy_ack_obj);
		status = NCSCC_RC_DISABLED;	/* To indicate that thread should destroy itself */
	} else {
		/* Event-type not set. BUG */
		assert(0);
		/* No further processing here. Just fall through and free evt structure */
	}
	/* Free tmr_req_info */
	m_MMGR_FREE_MBX_EVT_INFO(mbx_evt_info);
	return status;
}

/*********************************************************

  Function NAME: mdtm_process_recv_data

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mdtm_process_recv_data(uns8 *buffer, uns16 len, uns64 transport_adest, uns32 *buff_dump)
{
	/*
	   Get the MDS Header from the data received
	   if Destination SVC exists
	   if the message is a fragmented one
	   reassemble  the recd data(is done based on the received data <Sequence number, transport_adest>)
	   An entry for the matching transport_adest is serached in the MDS_ADEST_LIST

	   if found
	   reasesemble is done on the MDS_ADEST_LIST
	   if not found
	   an entry is created and reasesemble is done on the MDS_ADEST_LIST
	   A timer is also started on the reassembly queue (to prevent large timegaps between the
	   fragemented packets)
	   Send the data to the upper
	   (Check the role of Destination Service
	   for some send types before giving the data to upper layer)
	 */

	/* If the data is recd over here, it means its a fragmented or non-fragmented pkt) */
	uns16 pkt_type = 0;
	uns8 *data;

	/* Added for seq number check */
	uns32 temp_frag_seq_num = 0;

	data = &buffer[MDTM_PKT_TYPE_OFFSET];

	pkt_type = ncs_decode_16bit(&data);

	data = NULL;
	data = buffer;
	temp_frag_seq_num = ncs_decode_32bit(&data);

	if (pkt_type == MDTM_NORMAL_PKT) {
		return mdtm_process_recv_message_common(MDTM_DIRECT, &buffer[MDTM_FRAG_HDR_LEN],
							len - MDTM_FRAG_HDR_LEN, transport_adest, temp_frag_seq_num,
							buff_dump);
	} else {
		/* We got a fragmented pkt, reassemble */
		/* Check in reasssebly queue whether any pkts are present */
		uns16 more_frag = 0;
		uns16 frag_num = 0;
		uns32 seq_num = 0;
		MDTM_REASSEMBLY_QUEUE *reassem_queue = NULL;

		more_frag = ((pkt_type & MDTM_CHECK_MORE_FRAG) >> 15) & 0x1;

		frag_num = (pkt_type & 0x7fff);

		data = NULL;
		data = buffer;

		seq_num = ncs_decode_32bit(&data);

		m_MDS_LOG_DBG("MDTM: Recd message with Fragment Seqnum=%d, frag_num=%d, from src_Tipc_id=<0x%08x:%u>",
			      seq_num, frag_num, (uns32)(transport_adest >> 32), (uns32)(transport_adest));

		/* Checking in reassembly queue */
		reassem_queue = mdtm_check_reassem_queue(seq_num, transport_adest);

		if (reassem_queue != NULL) {
			if (len <= MDTM_FRAG_HDR_LEN) {
				m_MDS_LOG_ERR
				    ("MDTM: No payload data present in fragmented msg or incomplete frag hdr=%d", len);
				return NCSCC_RC_FAILURE;
			}

			if (reassem_queue->to_be_dropped == TRUE) {
				/* Check whether we recd the last pkt */
				if (((pkt_type & 0x7fff) > 0) && ((pkt_type & 0x8000) == 0)) {
					/* Last frag in the message recd */

					/* Free memory Allocated to this msg and MDTM_REASSEMBLY_QUEUE */
					mdtm_free_reassem_msg_mem(&reassem_queue->recv.msg);

					/* Destroy Handle */
					ncshm_destroy_hdl(NCS_SERVICE_ID_COMMON, reassem_queue->tmr_hdl);

					/* stop timer and free memory */
					m_NCS_TMR_STOP(reassem_queue->tmr);
					m_NCS_TMR_DESTROY(reassem_queue->tmr);

					m_MMGR_FREE_TMR_INFO(reassem_queue->tmr_info);

					reassem_queue->tmr_info = NULL;

					/* Delete entry from MDTM_REASSEMBLY_QUEUE */
					mdtm_del_reassemble_queue(reassem_queue->key.frag_sequence_num,
								  reassem_queue->key.id);
				}
				*buff_dump = 0;	/* For future use. It can be made 1, easily without having to relink etc. */
				m_MDS_LOG_ERR
				    ("MDTM: Message is dropped as msg is out of seq TRANSPOR-ID=<0x%016llx> \n",
				     transport_adest);
				return NCSCC_RC_FAILURE;
			}

			if (reassem_queue->next_frag_num == frag_num) {
				/* Check SVC_hdl role here */
				if (mds_svc_tbl_get_role(reassem_queue->recv.dest_svc_hdl) != NCSCC_RC_SUCCESS)
					/* fUNTION WILL RETURN success when svc is in active or quiesced */
				{
					switch (reassem_queue->recv.snd_type) {
					case MDS_SENDTYPE_SND:
					case MDS_SENDTYPE_SNDRSP:
					case MDS_SENDTYPE_SNDACK:
					case MDS_SENDTYPE_BCAST:
						{
							reassem_queue->to_be_dropped = TRUE;	/* This is for avoiding the prints of bad spurious fragments */

							if (((pkt_type & 0x7fff) > 0) && ((pkt_type & 0x8000) == 0)) {
								/* Free the queued data depending on the data type */
								mdtm_free_reassem_msg_mem(&reassem_queue->recv.msg);

								/* Destroy Handle */
								ncshm_destroy_hdl(NCS_SERVICE_ID_COMMON,
										  reassem_queue->tmr_hdl);
								/* stop timer and free memory */
								m_NCS_TMR_STOP(reassem_queue->tmr);
								m_NCS_TMR_DESTROY(reassem_queue->tmr);

								m_MMGR_FREE_TMR_INFO(reassem_queue->tmr_info);

								reassem_queue->tmr_info = NULL;
								/* Delete this entry from Global reassembly queue */
								mdtm_del_reassemble_queue(reassem_queue->
											  key.frag_sequence_num,
											  reassem_queue->key.id);
							}
							m_MDS_LOG_ERR
							    ("MDTM: Message is dropped as msg is destined to standby svc\n");
							return NCSCC_RC_FAILURE;
						}
						break;
					default:
						break;
					}
				}

				/* Enqueue the data at the End depending on the data type */
				if (reassem_queue->recv.msg.encoding == MDS_ENC_TYPE_FLAT) {
					m_MDS_LOG_INFO("MDTM: Reassembling in flat UB\n");
					NCS_UBAID ub;
					ncs_enc_init_space_pp(&ub, 0, 0);
					ncs_encode_n_octets_in_uba(&ub, &buffer[MDTM_FRAG_HDR_LEN],
								   (len - MDTM_FRAG_HDR_LEN));

					ncs_enc_append_usrbuf(&reassem_queue->recv.msg.data.flat_uba, ub.start);
				} else if (reassem_queue->recv.msg.encoding == MDS_ENC_TYPE_FULL) {
					m_MDS_LOG_INFO("MDTM: Reassembling in FULL UB\n");
					NCS_UBAID ub;
					ncs_enc_init_space_pp(&ub, 0, 0);
					ncs_encode_n_octets_in_uba(&ub, &buffer[MDTM_FRAG_HDR_LEN],
								   (len - MDTM_FRAG_HDR_LEN));

					ncs_enc_append_usrbuf(&reassem_queue->recv.msg.data.fullenc_uba, ub.start);
				} else {
					return NCSCC_RC_FAILURE;
				}

				/* Reassemble and send to upper layer if last pkt */
				if (((pkt_type & 0x7fff) > 0) && ((pkt_type & 0x8000) == 0)) {
					/* Last frag in the message recd */
					/* Total Data reassembled */

					/* Destroy Handle */
					ncshm_destroy_hdl(NCS_SERVICE_ID_COMMON, reassem_queue->tmr_hdl);

					/* stop timer and free memory */
					m_NCS_TMR_STOP(reassem_queue->tmr);
					m_NCS_TMR_DESTROY(reassem_queue->tmr);

					m_MMGR_FREE_TMR_INFO(reassem_queue->tmr_info);

					reassem_queue->tmr_info = NULL;

					m_MDS_LOG_INFO("MDTM: Sending data to upper layer\n");

					/* Depending on the msg type if flat or full enc apply dec space, for setting the uba to decode by user */
					if (reassem_queue->recv.msg.encoding == MDS_ENC_TYPE_FLAT) {
						ncs_dec_init_space(&reassem_queue->recv.msg.data.flat_uba,
								   reassem_queue->recv.msg.data.flat_uba.start);
					} else if (reassem_queue->recv.msg.encoding == MDS_ENC_TYPE_FULL) {
						ncs_dec_init_space(&reassem_queue->recv.msg.data.fullenc_uba,
								   reassem_queue->recv.msg.data.fullenc_uba.start);
					}
					/* for direct buff and cpy encoding modes we do nothig */
					mds_mcm_ll_data_rcv(&reassem_queue->recv);

					/* Now delete this entry from Global reassembly queue */
					mdtm_del_reassemble_queue(reassem_queue->key.frag_sequence_num,
								  reassem_queue->key.id);

					return NCSCC_RC_SUCCESS;
				} else {
					/* Incrementing for next frag */
					reassem_queue->next_frag_num++;
					return NCSCC_RC_SUCCESS;
				}
			} else {
				/* fragment recd is not next fragment */
				*buff_dump = 0;	/* For future use. It can be made 1, easily without having to relink etc. */
				m_MDS_LOG_ERR("MDTM: Frag recd is not next frag so dropping adest=<0x%016llx>\n",
					      transport_adest);
				reassem_queue->to_be_dropped = TRUE;	/* This is for avoiding the prints of bad spurious fragments */

				if (((pkt_type & 0x7fff) > 0) && ((pkt_type & 0x8000) == 0)) {
					/* Last frag in the message recd */

					/* Free memory Allocated to UB and MDTM_REASSEMBLY_QUEUE */
					mdtm_free_reassem_msg_mem(&reassem_queue->recv.msg);

					/* Destroy Handle */
					ncshm_destroy_hdl(NCS_SERVICE_ID_COMMON, reassem_queue->tmr_hdl);

					/* stop timer and free memory */
					m_NCS_TMR_STOP(reassem_queue->tmr);
					m_NCS_TMR_DESTROY(reassem_queue->tmr);
					m_MMGR_FREE_TMR_INFO(reassem_queue->tmr_info);

					reassem_queue->tmr_info = NULL;

					/* Delete entry from MDTM_REASSEMBLY_QUEUE */
					mdtm_del_reassemble_queue(reassem_queue->key.frag_sequence_num,
								  reassem_queue->key.id);

				}
				return NCSCC_RC_FAILURE;
			}
		} else if (MDTM_FIRST_FRAG_NUM == pkt_type) {
			return mdtm_process_recv_message_common(MDTM_REASSEMBLE, buffer, len, transport_adest,
								temp_frag_seq_num, buff_dump);
		} else {
			*buff_dump = 0;
			/* Some stale message, Log and Drop */
			m_MDS_LOG_ERR("MDTM: Some stale message recd, hence dropping adest=<0x%016llx>\n",
				      transport_adest);
			return NCSCC_RC_FAILURE;
		}
	}			/* ELSE Loop */
	return NCSCC_RC_FAILURE;
}

/*********************************************************

  Function NAME: mdtm_fill_data

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uns32 mdtm_fill_data(MDTM_REASSEMBLY_QUEUE *reassem_queue, uns8 *buffer, uns16 len, uns8 enc_type)
{
	m_MDS_LOG_INFO("MDTM: User Recd msg len=%d", len);
	switch (enc_type) {
	case MDS_ENC_TYPE_CPY:
		/* We will never reach here */
		/* Nothing done here */
		return NCSCC_RC_SUCCESS;
		break;

	case MDS_ENC_TYPE_FLAT:
		{
			ncs_enc_init_space_pp(&reassem_queue->recv.msg.data.flat_uba, 0, 0);
			ncs_encode_n_octets_in_uba(&reassem_queue->recv.msg.data.flat_uba, buffer, len);
			return NCSCC_RC_SUCCESS;
		}
		break;

	case MDS_ENC_TYPE_FULL:
		{
			ncs_enc_init_space_pp(&reassem_queue->recv.msg.data.fullenc_uba, 0, 0);
			ncs_encode_n_octets_in_uba(&reassem_queue->recv.msg.data.fullenc_uba, buffer, len);
			return NCSCC_RC_SUCCESS;
		}
		break;

	case MDS_ENC_TYPE_DIRECT_BUFF:
		{
			reassem_queue->recv.msg.data.buff_info.buff = mds_alloc_direct_buff(len);
			memcpy(reassem_queue->recv.msg.data.buff_info.buff, buffer, len);
			reassem_queue->recv.msg.data.buff_info.len = len;
			return NCSCC_RC_SUCCESS;
		}
		break;

	default:
		return NCSCC_RC_FAILURE;
		break;
	}
	return NCSCC_RC_FAILURE;
}

/*********************************************************

  Function NAME: mdtm_check_reassem_queue

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static MDTM_REASSEMBLY_QUEUE *mdtm_check_reassem_queue(uns32 seq_num, MDS_DEST id)
{
	/*
	   STEP 1: Check whether an entry is present with the seq_num and id,
	   If present
	   return the Pointer
	   else
	   return Null
	 */
	MDTM_REASSEMBLY_QUEUE *reassem_queue = NULL;
	MDTM_REASSEMBLY_KEY reassembly_key;

	memset(&reassembly_key, 0, sizeof(reassembly_key));

	reassembly_key.frag_sequence_num = seq_num;
	reassembly_key.id = id;
	reassem_queue = (MDTM_REASSEMBLY_QUEUE *)ncs_patricia_tree_get(&mdtm_reassembly_list, (uns8 *)&reassembly_key);

	if (reassem_queue == NULL) {
		m_MDS_LOG_DBG("MDS_DT_COMMON : reassembly queue doesnt exist seq_num=%d, transport_adest=<0x%08x,%u",
			      seq_num, (uns32)(id >> 32), (uns32)(id));
		return reassem_queue;
	}
	return reassem_queue;
}

/*********************************************************

  Function NAME: mdtm_add_reassemble_queue

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static MDTM_REASSEMBLY_QUEUE *mdtm_add_reassemble_queue(uns32 seq_num, MDS_DEST id)
{
	/*
	   STEP 1: create an entry in the reassemble queue with parameters as seq_num and id,
	   return the Pointer to the reassembly queue
	 */
	MDTM_REASSEMBLY_QUEUE *reassem_queue = NULL;

	/* Allocate Memory for reassem_queue */
	reassem_queue = m_MMGR_ALLOC_REASSEM_QUEUE;
	if (reassem_queue == NULL) {
		m_MDS_LOG_ERR("MDTM: Memory allocation to reassembly queue failed\n");
		return reassem_queue;
	}

	memset(reassem_queue, 0, sizeof(MDTM_REASSEMBLY_QUEUE));
	reassem_queue->key.frag_sequence_num = seq_num;
	reassem_queue->key.id = id;
	reassem_queue->node.key_info = (uns8 *)&reassem_queue->key;
	ncs_patricia_tree_add(&mdtm_reassembly_list, (NCS_PATRICIA_NODE *)&reassem_queue->node);
	return reassem_queue;
}

/*********************************************************

  Function NAME: mdtm_del_reassemble_queue

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
static uns32 mdtm_del_reassemble_queue(uns32 seq_num, MDS_DEST id)
{
	/*
	   STEP 1: Check whether an entry is present with the seq_num and id,
	   If present
	   delete the node
	   return success
	   else
	   return failure
	 */

	MDTM_REASSEMBLY_QUEUE *reassem_queue = NULL;
	MDTM_REASSEMBLY_KEY reassembly_key;

	memset(&reassembly_key, 0, sizeof(reassembly_key));

	reassembly_key.frag_sequence_num = seq_num;
	reassembly_key.id = id;
	reassem_queue = (MDTM_REASSEMBLY_QUEUE *)ncs_patricia_tree_get(&mdtm_reassembly_list, (uns8 *)&reassembly_key);

	if (reassem_queue == NULL) {
		m_MDS_LOG_DBG("MDTM: Empty Reassembly queue, No Matching found\n");
		return NCSCC_RC_FAILURE;
	}

	if (reassem_queue->tmr_info != NULL) {
		mdtm_free_reassem_msg_mem(&reassem_queue->recv.msg);	/* Found During MSG Size bug Fix */
		m_NCS_TMR_STOP(reassem_queue->tmr);
		m_NCS_TMR_DESTROY(reassem_queue->tmr);
		reassem_queue->tmr_info = NULL;
	}
	ncs_patricia_tree_del(&mdtm_reassembly_list, (NCS_PATRICIA_NODE *)reassem_queue);

	m_MMGR_FREE_REASSEM_QUEUE(reassem_queue);
	return NCSCC_RC_SUCCESS;
}

void mds_buff_dump(uns8 *buff, uns32 len, uns32 max)
{
	int offset;
	uns8 last_line[8];
	/* STEP 1: Print all but the last 8 bytes. 
	   If offset = 0 and len = 8, don't go into for loop below
	   If offset = 1 and len = 7, don't go into for loop below
	   If offset = 0 and len = 9,   do  go into for loop below */

	if (len > max) {
		m_MDS_LOG_ERR("DUMP:Changing dump-extent:buff=0x%08x:max=%d, len=%d\n", buff, max, len);
		len = max;
	}

	for (offset = 0; (len - offset) > 8; offset += 8) {
		m_MDS_LOG_ERR
		    ("DUMP:buff=0x%08x:offset=%3d to %3d:Bytes = 0x%02x 0x%02x 0x%02x 0x%02x : 0x%02x 0x%02x 0x%02x 0x%02x",
		     (uns32)(long)buff, offset, offset + 7, buff[offset], buff[offset + 1], buff[offset + 2],
		     buff[offset + 3], buff[offset + 4], buff[offset + 5], buff[offset + 6], buff[offset + 7]);
	}

	/* STEP 2: Print last  ((len % 8 ) + 1) bytes 
	   Reaching here implies, len - offset <= 8 */

	memset(last_line, 0, 8);
	memcpy(last_line, buff + offset, len - offset);

	m_MDS_LOG_ERR
	    ("DUMP:buff=0x%08x:offset=%3d to %3d:Bytes = 0x%02x 0x%02x 0x%02x 0x%02x : 0x%02x 0x%02x 0x%02x 0x%02x",
	     (uns32)(long)buff, offset, len - 1, last_line[0], last_line[0 + 1], last_line[0 + 2], last_line[0 + 3],
	     last_line[0 + 4], last_line[0 + 5], last_line[0 + 6], last_line[0 + 7]);
}

/*********************************************************

  Function NAME: mdtm_free_reassem_msg_mem

  DESCRIPTION:

  ARGUMENTS:

  RETURNS:  1 - NCSCC_RC_SUCCESS
            2 - NCSCC_RC_FAILURE

*********************************************************/
uns32 mdtm_free_reassem_msg_mem(MDS_ENCODED_MSG *msg)
{
	switch (msg->encoding) {
	case MDS_ENC_TYPE_CPY:
		/* Presently doing nothing */
		return NCSCC_RC_SUCCESS;
		break;

	case MDS_ENC_TYPE_FLAT:
		{
			m_MMGR_FREE_BUFR_LIST(msg->data.flat_uba.start);
			return NCSCC_RC_SUCCESS;
		}
		break;
	case MDS_ENC_TYPE_FULL:
		{
			m_MMGR_FREE_BUFR_LIST(msg->data.fullenc_uba.start);
			return NCSCC_RC_SUCCESS;
		}
		break;

	case MDS_ENC_TYPE_DIRECT_BUFF:
		{
			mds_free_direct_buff(msg->data.buff_info.buff);
			return NCSCC_RC_SUCCESS;
		}
		break;
	default:
		return NCSCC_RC_FAILURE;
		break;
	}
	return NCSCC_RC_SUCCESS;
}

uns16 mds_checksum(uns32 length, uns8 buff[])
{
	uns16 word16 = 0;
	uns32 sum = 0;
	uns32 i;
	uns32 loop_count;

	/* make 16 bit words out of every two adjacent 8 bit words and
	   calculate the sum of all 16 bit words */
	if (length % 2 == 0) {
		loop_count = length;
		for (i = 0; i < loop_count; i = i + 2) {
			word16 = (((uns16)buff[i] << 8) + ((uns16)buff[i + 1]));
			sum = sum + (uns32)word16;
		}
	} else {
		loop_count = length - 2;
		for (i = 0; i < loop_count; i = i + 2) {
			word16 = (((uns16)buff[i] << 8) + ((uns16)buff[i + 1]));
			sum = sum + (uns32)word16;
		}
		word16 = (((uns16)buff[i] << 8) + ((uns16)0));
		sum = sum + (uns32)word16;

	}
	sum = sum + length;

	/* keep only the last 16 bits of the 32 bit calculated sum and add the carries */
	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	/* Take the one's complement of sum */
	sum = ~sum;

	return ((uns16)sum);
}

/****************************************************************************
 *      
 * Function Name: mds_destroy_event
 *              
 * Purpose: Used for posting a message when MDS (thread) is to be destroyed.
 *              
 *              
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *              
 ****************************************************************************/
uns32 mds_destroy_event(NCS_SEL_OBJ destroy_ack_obj)
{
	/* Now Queue the message in the Mailbox */
	MDS_MBX_EVT_INFO *mbx_evt_info = NULL;

	mbx_evt_info = m_MMGR_ALLOC_MBX_EVT_INFO;
	if (mbx_evt_info == NULL)
		return NCSCC_RC_FAILURE;
	memset(mbx_evt_info, 0, sizeof(MDS_MBX_EVT_INFO));

	mbx_evt_info->type = MDS_MBX_EVT_DESTROY;
	mbx_evt_info->info.destroy_ack_obj = destroy_ack_obj;
	if ((m_NCS_IPC_SEND(&mdtm_mbx_common, mbx_evt_info, NCS_IPC_PRIORITY_HIGH)) != NCSCC_RC_SUCCESS) {
		m_MDS_LOG_ERR("MDTM: DESTROY post to Mailbox Failed\n");
		m_MMGR_FREE_MBX_EVT_INFO(mbx_evt_info);
		return NCSCC_RC_FAILURE;
	}
	m_MDS_LOG_INFO("MDTM: DESTROY post to Mailbox Success\n");
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * Function Name: mdtm_mailbox_mbx_cleanup
 *
 * Purpose: Used for cleaning messages in mailbox
 *
 * Return Value:  NCSCC_RC_SUCCESS
 *                NCSCC_RC_FAILURE
 *
 ****************************************************************************/
NCS_BOOL mdtm_mailbox_mbx_cleanup(NCSCONTEXT arg, NCSCONTEXT msg)
{
	MDS_MBX_EVT_INFO *mbx_evt_info = (MDS_MBX_EVT_INFO *)msg;

	switch (mbx_evt_info->type) {
		case MDS_MBX_EVT_TIMER_EXPIRY:
			/* freeing of tmr_req_info and handle is done in mcm, where tmr_running flag is still true */
			break;
		case MDS_MBX_EVT_DESTROY:
			/* Destroy ack object is destroyed by thread posting the destroy-event */
			break;
		default:
			assert(0);
			break;
	}
	m_MMGR_FREE_MBX_EVT_INFO(mbx_evt_info);

	return TRUE;
}

