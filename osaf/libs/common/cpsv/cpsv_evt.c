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
  
  This file consists of CPSV routines used for event handling.

    
******************************************************************************
*/

#include "cpsv.h"

FUNC_DECLARATION(CPSV_CKPT_DATA);
static SaCkptSectionIdT *cpsv_evt_dec_sec_id(NCS_UBAID *i_ub, uint32_t svc_id);
static uint32_t cpsv_evt_enc_sec_id(NCS_UBAID *o_ub, SaCkptSectionIdT *sec_id);

/****************************************************************************\
 PROCEDURE NAME : cpsv_evt_cpy

 DESCRIPTION    : This routine will allocate the memory for CPSV_EVT and its
                  internal pointers and copy the contents.

 ARGUMENTS      : cb : CPA control Block.
                  cpy  : copy info.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uint32_t cpsv_evt_cpy(CPSV_EVT *src, CPSV_EVT *dest, uint32_t svc_id)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	dest = m_MMGR_ALLOC_CPSV_EVT(svc_id);

	if (dest) {
		memcpy(dest, src, sizeof(CPSV_EVT));

		/* Copy the internal pointers TBD */
	} else {
		rc = NCSCC_RC_OUT_OF_MEM;
	}

	return rc;

}
uint32_t cpsv_ref_cnt_encode(NCS_UBAID *i_ub, CPSV_A2ND_REFCNTSET *data)
{
   uint32_t rc= NCSCC_RC_SUCCESS;
   uint8_t *pStream = NULL; 
   uint32_t counter =0,array_cnt=0;

   pStream = ncs_enc_reserve_space(i_ub, 4);
   ncs_encode_32bit(&pStream, data->no_of_nodes);
   ncs_enc_claim_space(i_ub, 4);


   counter = data->no_of_nodes;
   while(counter!=0)
   {
     pStream = ncs_enc_reserve_space(i_ub,12 );
     ncs_encode_64bit(&pStream , data->ref_cnt_array[array_cnt].ckpt_id); 
     ncs_encode_32bit(&pStream , data->ref_cnt_array[array_cnt].ckpt_ref_cnt); 
     counter--;
     array_cnt++;
     ncs_enc_claim_space(i_ub, 12);
   }
   
   return rc;
}


uint32_t cpsv_refcnt_ckptid_decode(CPSV_A2ND_REFCNTSET *pdata , NCS_UBAID *io_uba)
{
   uint8_t *pstream = NULL;
   uint8_t local_data[50];
   uint32_t counter=0,array_cnt=0;
 

   pstream = ncs_dec_flatten_space(io_uba,local_data, 4);
   pdata->no_of_nodes = ncs_decode_32bit(&pstream);
   ncs_dec_skip_space(io_uba, 4);

   counter =  pdata->no_of_nodes; 
   while (counter != 0)
   {
    pstream = ncs_dec_flatten_space(io_uba,local_data, 12);
    pdata->ref_cnt_array[array_cnt].ckpt_id = ncs_decode_64bit(&pstream);
    pdata->ref_cnt_array[array_cnt].ckpt_ref_cnt = ncs_decode_32bit(&pstream);
    ncs_dec_skip_space(io_uba, 12);
    counter--;
    array_cnt++;
   }
   return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 PROCEDURE NAME : cpsv_ckpt_data_encode

 DESCRIPTION    : This routine will encode the contents of CPSV_EVT into user buf

 ARGUMENTS      : *data - CPSV_CKPT_DATA
                  *i_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/

uint32_t cpsv_ckpt_data_encode(NCS_UBAID *i_ub, CPSV_CKPT_DATA *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint8_t *header = NULL;
	CPSV_CKPT_DATA *pdata = data;
	uint16_t num_of_nodes = 0;

	header = ncs_enc_reserve_space(i_ub, 2);
	if (!header)
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory alloc failed in cpsv_ckpt_data_encode \n");

	ncs_enc_claim_space(i_ub, 2);

	while (pdata != NULL) {
		cpsv_ckpt_node_encode(i_ub, pdata);
		num_of_nodes++;
		pdata = pdata->next;
	}
	ncs_encode_16bit(&header, num_of_nodes);

	return rc;

}

/****************************************************************************\
 PROCEDURE NAME : cpsv_ckpt_node_encode

 DESCRIPTION    : This routine will encode the contents from CPSV_CKPT_DATA* node pdata 
                  into the io buffer NCS_UBAID *i_ub.

 ARGUMENTS      : i_ub  - NCS_UBAID pointer.
                  pdata - CPSV_CKPT_DATA pointer.

 RETURNS        : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

 NOTES          :
\*****************************************************************************/

uint32_t cpsv_ckpt_node_encode(NCS_UBAID *i_ub, CPSV_CKPT_DATA *pdata)
{
	uint8_t *pStream = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS, size;

	pStream = ncs_enc_reserve_space(i_ub, 2);
	if (!pStream)
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory alloc failed in cpsv_ckpt_node_enoode \n");

/* Section id */
	ncs_encode_16bit(&pStream, pdata->sec_id.idLen);
	ncs_enc_claim_space(i_ub, 2);
	if (pdata->sec_id.idLen)
		ncs_encode_n_octets_in_uba(i_ub, (uint8_t *)pdata->sec_id.id, (uint32_t)pdata->sec_id.idLen);

	size = 8 + 8 + 8;
	pStream = ncs_enc_reserve_space(i_ub, size);
	if (!pStream)
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory alloc failed in cpsv_ckpt_node_encode \n");

	ncs_encode_64bit(&pStream, pdata->expirationTime);
	ncs_encode_64bit(&pStream, pdata->dataSize);
	ncs_encode_64bit(&pStream, pdata->readSize);
	ncs_enc_claim_space(i_ub, size);

	if (pdata->dataSize)
		ncs_encode_n_octets_in_uba(i_ub, pdata->data, (uint32_t)pdata->dataSize);

	pStream = ncs_enc_reserve_space(i_ub, 8);
	if (!pStream)
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory alloc failed in cpsv_ckpt_node_encode\n");

	ncs_encode_64bit(&pStream, pdata->dataOffset);
	ncs_enc_claim_space(i_ub, 8);
	return rc;
}

/****************************************************************************\
 PROCEDURE NAME : cpsv_ckpt_node_encode

 DESCRIPTION    : This routine will encode the contents from CPSV_CKPT_DATA* node pdata 
                  into the io buffer NCS_UBAID *i_ub.

 ARGUMENTS      : io_ub  - NCS_UBAID pointer.
                  pdata - CPSV_CKPT_DATA pointer.

 RETURNS        : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

 NOTES          :
\*****************************************************************************/

uint32_t cpsv_ckpt_access_encode(CPSV_CKPT_ACCESS *ckpt_data, NCS_UBAID *io_uba)
{

	uint32_t rc = NCSCC_RC_SUCCESS;
	uint8_t *pstream = NULL;
	uint32_t space, all_repl_evt_flag = 0, is_ckpt_open = 0;

	space = 4 + 8 + 8 + 8 + 4 + 4;
	pstream = ncs_enc_reserve_space(io_uba, space);
	if (!pstream)
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory alloc failed in cpsv_ckpt_access_encode\n");

	ncs_encode_32bit(&pstream, ckpt_data->type);
	ncs_encode_64bit(&pstream, ckpt_data->ckpt_id);
	ncs_encode_64bit(&pstream, ckpt_data->lcl_ckpt_id);
	ncs_encode_64bit(&pstream, ckpt_data->agent_mdest);
	ncs_encode_32bit(&pstream, ckpt_data->num_of_elmts);
	all_repl_evt_flag = ckpt_data->all_repl_evt_flag;
	ncs_encode_32bit(&pstream, all_repl_evt_flag);

	ncs_enc_claim_space(io_uba, space);

	/* CKPT_DATA LINKED LIST OF EDU */
	cpsv_ckpt_data_encode(io_uba, ckpt_data->data);

	/* Following are Not Required But for Write/Read (Used in checkpoint sync evt) compatiblity with 3.0.2 */
	space = 4 + 1 + 8 + 8 + 8 + 8 + 4 + 8 + 4 + 4 + 1 /*+ MDS_SYNC_SND_CTXT_LEN_MAX */ ;
	pstream = ncs_enc_reserve_space(io_uba, space);
	if (!pstream)
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory alloc failed in cpsv_ckpt_access_encode\n");

	ncs_encode_32bit(&pstream, ckpt_data->seqno);
	ncs_encode_8bit(&pstream, ckpt_data->last_seq);
	ncs_encode_64bit(&pstream, ckpt_data->ckpt_sync.ckpt_id);
	ncs_encode_64bit(&pstream, ckpt_data->ckpt_sync.invocation);
	ncs_encode_64bit(&pstream, ckpt_data->ckpt_sync.lcl_ckpt_hdl);
	ncs_encode_64bit(&pstream, ckpt_data->ckpt_sync.client_hdl);
	is_ckpt_open = ckpt_data->ckpt_sync.is_ckpt_open;
	ncs_encode_32bit(&pstream, is_ckpt_open);
	ncs_encode_64bit(&pstream, ckpt_data->ckpt_sync.cpa_sinfo.dest);
	ncs_encode_32bit(&pstream, ckpt_data->ckpt_sync.cpa_sinfo.stype);
	ncs_encode_32bit(&pstream, ckpt_data->ckpt_sync.cpa_sinfo.to_svc);
	ncs_encode_8bit(&pstream, ckpt_data->ckpt_sync.cpa_sinfo.ctxt.length);
	ncs_enc_claim_space(io_uba, space);
	ncs_encode_n_octets_in_uba(io_uba, ckpt_data->ckpt_sync.cpa_sinfo.ctxt.data, (uint32_t)MDS_SYNC_SND_CTXT_LEN_MAX);
	return rc;
}

/****************************************************************************
  Name          : cpsv_nd2a_read_data_encode
 
  Description   : This function encodes the CPSV_ND2A_READ_DATA.
 
  Arguments     : CPSV_ND2A_READ_DATA *read_data
                  NCS_UBAID *io_uba
                  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

uint32_t cpsv_nd2a_read_data_encode(CPSV_ND2A_READ_DATA *read_data, NCS_UBAID *io_uba)
{
	uint8_t *pstream = NULL;

	pstream = ncs_enc_reserve_space(io_uba, 4);
	if (!pstream)
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory alloc failed in cpsv_nd2a_read_data_encode\n");

	ncs_encode_32bit(&pstream, read_data->read_size);	/* Type */
	ncs_enc_claim_space(io_uba, 4);

	if (read_data->read_size) {
		ncs_encode_n_octets_in_uba(io_uba, read_data->data, (uint32_t)read_data->read_size);
	}

	pstream = ncs_enc_reserve_space(io_uba, 4);
	if (!pstream)
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory alloc failed in cpsv_nd2a_read_data_encode\n");
	ncs_encode_32bit(&pstream, read_data->err);	/* Type */
	ncs_enc_claim_space(io_uba, 4);

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************\
 PROCEDURE NAME : cpsv_data_access_rsp_encode

 DESCRIPTION    : This routine will encode the contents of CPSV_ND2A_DATA_ACCESS_RSP* into user buf

 ARGUMENTS      : *CPSV_ND2A_DATA_ACCESS_RSP *data_rsp .
                  *io_ub  - User Buff.
				  MDS_CLIENT_MSG_FORMAT_VER.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uint32_t cpsv_data_access_rsp_encode(CPSV_ND2A_DATA_ACCESS_RSP *data_rsp, NCS_UBAID *io_uba, MDS_CLIENT_MSG_FORMAT_VER o_msg_fmt_ver)
{

	uint8_t *pstream = NULL;
	uint32_t size, i;
	uint32_t ver_compare = 4; /* CPND_MDS_PVT_SUBPART_VERSION  and CPA_MDS_PVT_SUBPART_VERSION*/
	if (o_msg_fmt_ver  >= ver_compare)
		size = 4 + 4 + 4 + 4 + 8 + 4 + 8 + 8;   
	else
		size = 4 + 4 + 4 + 4 + 8 + 4 + 8;
   pstream = ncs_enc_reserve_space(io_uba, size);
   if(!pstream)
            return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE,"Memory alloc failed in cpsv_data_access_rsp_encode\n");
   ncs_encode_32bit(&pstream , data_rsp->type);
   ncs_encode_32bit(&pstream , data_rsp->num_of_elmts);
   ncs_encode_32bit(&pstream , data_rsp->error);
   ncs_encode_32bit(&pstream , data_rsp->size);
   ncs_encode_64bit(&pstream , data_rsp->ckpt_id);
   ncs_encode_32bit(&pstream , data_rsp->error_index);
   ncs_encode_64bit(&pstream , data_rsp->from_svc);
   if (o_msg_fmt_ver  >= ver_compare) 
	   ncs_encode_64bit(&pstream , data_rsp->lcl_ckpt_id);
   ncs_enc_claim_space(io_uba, size);

	if (data_rsp->type == CPSV_DATA_ACCESS_WRITE_RSP) {

		SaUint32T *write_err_index = data_rsp->info.write_err_index;
		size = data_rsp->size * sizeof(SaUint32T);
		if (size) {
			pstream = ncs_enc_reserve_space(io_uba, size);
			if (!pstream)
				return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE,
						       "Memory alloc failed in cpsv_data_access_rsp_encode\n");
			for (i = 0; i < data_rsp->size; i++) {
				/* Encode Write Error Index */
				ncs_encode_32bit(&pstream, write_err_index[i]);
			}
			ncs_enc_claim_space(io_uba, size);
		}

	} else if ((data_rsp->type == CPSV_DATA_ACCESS_LCL_READ_RSP)
		   || (data_rsp->type == CPSV_DATA_ACCESS_RMT_READ_RSP)) {

		for (i = 0; i < data_rsp->size; i++) {
			cpsv_nd2a_read_data_encode(&data_rsp->info.read_data[i], io_uba);
		}

	} else if (data_rsp->type == CPSV_DATA_ACCESS_OVWRITE_RSP) {

		pstream = ncs_enc_reserve_space(io_uba, 4);
		if (!pstream)
			return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE,
					       "Memory alloc failed in cpsv_data_access_rsp_encode\n");
		ncs_encode_32bit(&pstream, data_rsp->info.ovwrite_error.error);
		ncs_enc_claim_space(io_uba, 4);
	}
	return NCSCC_RC_SUCCESS;

}

/****************************************************************************\
 PROCEDURE NAME : cpsv_evt_enc_flat

 DESCRIPTION    : This routine will encode the contents of CPSV_EVT into user buf

 ARGUMENTS      : *i_evt - Event Struct.
                  *o_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uint32_t cpsv_evt_enc_flat(EDU_HDL *edu_hdl, CPSV_EVT *i_evt, NCS_UBAID *o_ub)
{
	uint32_t size;
	uint32_t rc = NCSCC_RC_SUCCESS;
	size = sizeof(CPSV_EVT);

	/* Encode the Top level evt envolop */
	ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)i_evt, size);

	/* Encode the internal Pointers */
	if (i_evt->type == CPSV_EVT_TYPE_CPA) {
		if (i_evt->info.cpa.type == CPA_EVT_ND2A_CKPT_DATA_RSP) {
			CPSV_ND2A_DATA_ACCESS_RSP *data_rsp = &i_evt->info.cpa.info.sec_data_rsp;
			if ((data_rsp->type == CPSV_DATA_ACCESS_LCL_READ_RSP)
			    || (data_rsp->type == CPSV_DATA_ACCESS_RMT_READ_RSP)) {
				if (data_rsp->num_of_elmts == -1) {
					size = 0;
				} else {
					if (data_rsp->size > 0) {
						uint32_t iter = 0;
						for (; iter < data_rsp->size; iter++) {
							size = sizeof(CPSV_ND2A_READ_DATA);
							ncs_encode_n_octets_in_uba(o_ub,
										   (uint8_t *)&data_rsp->
										   info.read_data[iter]
										   , size);
							if (data_rsp->info.read_data[iter].read_size > 0) {
								size = data_rsp->info.read_data[iter].read_size;
								ncs_encode_n_octets_in_uba(o_ub,
											   (uint8_t *)data_rsp->info.
											   read_data[iter].data, size);
							}
						}
					}
				}
			} else if (data_rsp->type == CPSV_DATA_ACCESS_WRITE_RSP) {
				if (data_rsp->num_of_elmts == -1)
					size = 0;
				else
					size = data_rsp->num_of_elmts * sizeof(SaUint32T);
				if (size)
					ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)data_rsp->info.write_err_index, size);
			}
		} else if (i_evt->info.cpa.type == CPA_EVT_ND2A_CKPT_CLM_NODE_LEFT) {
			/* Do nothing */
		} else if (i_evt->info.cpa.type == CPA_EVT_ND2A_CKPT_CLM_NODE_JOINED) {
			/* Do nothing */
		} else if (i_evt->info.cpa.type == CPA_EVT_ND2A_CKPT_ARRIVAL_NTFY) {
			CPSV_CKPT_DATA *data = i_evt->info.cpa.info.arr_msg.ckpt_data;
			EDU_ERR ederror = 0;
			rc = m_NCS_EDU_EXEC(edu_hdl, FUNC_NAME(CPSV_CKPT_DATA), o_ub, EDP_OP_TYPE_ENC, data, &ederror);
			if (rc != NCSCC_RC_SUCCESS) {
				return rc;
			}
		}

		else if (i_evt->info.cpa.type == CPA_EVT_ND2A_SEC_CREATE_RSP) {
			SaCkptSectionIdT *sec_id = &i_evt->info.cpa.info.sec_creat_rsp.sec_id;
			if (sec_id->idLen) {
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, sec_id->idLen);
			}
		}

		else if (i_evt->info.cpa.type == CPA_EVT_ND2A_SEC_ITER_GETNEXT_RSP) {
			SaCkptSectionIdT *sec_id = &i_evt->info.cpa.info.iter_next_rsp.sect_desc.sectionId;
			if (sec_id->idLen) {
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, sec_id->idLen);
			}
		}

	} else if (i_evt->type == CPSV_EVT_TYPE_CPND) {
		if (i_evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_WRITE) {
			/* CKPT_DATA LINKED LIST OF EDU */
			cpsv_ckpt_data_encode(o_ub, i_evt->info.cpnd.info.ckpt_write.data);

		} else if (i_evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_READ) {
			cpsv_ckpt_data_encode(o_ub, i_evt->info.cpnd.info.ckpt_read.data);
		}
		/* Added for 3.0.B , these events encoding is missing in 3.0.2 */
		else if (i_evt->info.cpnd.type == CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC) {
			cpsv_ckpt_data_encode(o_ub, i_evt->info.cpnd.info.ckpt_nd2nd_sync.data);
		} else if (i_evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ) {
			cpsv_ckpt_data_encode(o_ub, i_evt->info.cpnd.info.ckpt_nd2nd_data.data);
		} else if (i_evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP) {
			CPSV_ND2A_DATA_ACCESS_RSP *data_rsp = &i_evt->info.cpnd.info.ckpt_nd2nd_data_rsp;
			if (data_rsp->type == CPSV_DATA_ACCESS_WRITE_RSP) {
				if (data_rsp->num_of_elmts == -1)
					size = 0;
				else
					size = data_rsp->num_of_elmts * sizeof(SaUint32T);
				if (size)
					ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)data_rsp->info.write_err_index, size);
			}

		} else if (i_evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_CREATE_REQ) {
			CPSV_CKPT_SECT_CREATE *create = &i_evt->info.cpnd.info.active_sec_creat;

			if (create->sec_attri.sectionId) {
				cpsv_evt_enc_sec_id(o_ub, create->sec_attri.sectionId);
			}
			if (create->init_size)
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)create->init_data, create->init_size);
		} else if (i_evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_CREATE_RSP) {
			SaCkptSectionIdT *sec_id = &i_evt->info.cpnd.info.active_sec_creat_rsp.sec_id;
			if (sec_id->idLen) {
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, sec_id->idLen);

			}
		} else if (i_evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ) {
			SaCkptSectionIdT *sec_id = &i_evt->info.cpnd.info.sec_delete_req.sec_id;
			if (sec_id->idLen) {
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, sec_id->idLen);

			}
		}
		/* End of Added for 3.0.B , these events encoding is missing in 3.0.2 */
		else if (i_evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_SECT_CREATE) {
			uint32_t size;
			CPSV_CKPT_SECT_CREATE *create = &i_evt->info.cpnd.info.sec_creatReq;

			if (create->sec_attri.sectionId) {
				cpsv_evt_enc_sec_id(o_ub, create->sec_attri.sectionId);
			}

			size = create->init_size;
			if (size != 0)
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)create->init_data, size);
		} else if (i_evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_ITER_GETNEXT) {
			SaCkptSectionIdT *sec_id = &i_evt->info.cpnd.info.iter_getnext.section_id;
			size = sec_id->idLen;
			if (size)
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, size);
		} else if (i_evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_SECT_DELETE) {
			SaCkptSectionIdT *sec_id = &i_evt->info.cpnd.info.sec_delReq.sec_id;
			if (sec_id->idLen) {
				size = sec_id->idLen;
				if (size)
					ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, size);
			}
		} else if (i_evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_SECT_EXP_SET) {
			SaCkptSectionIdT *sec_id = &i_evt->info.cpnd.info.sec_expset.sec_id;
			if (sec_id->idLen) {
				size = sec_id->idLen;
				if (size)
					ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, size);
			}
		} else if (i_evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_REQ) {
			SaCkptSectionIdT *sec_id = &i_evt->info.cpnd.info.sec_exp_set.sec_id;
			if (sec_id->idLen) {
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, sec_id->idLen);
			}
		} else if (i_evt->info.cpnd.type == CPND_EVT_D2ND_CKPT_INFO) {
			CPSV_CPND_DEST_INFO *dest_list = i_evt->info.cpnd.info.ckpt_info.dest_list;
			if (i_evt->info.cpnd.info.ckpt_info.dest_cnt) {
				size = i_evt->info.cpnd.info.ckpt_info.dest_cnt * sizeof(CPSV_CPND_DEST_INFO);
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)dest_list, size);
			}
		} else if (i_evt->info.cpnd.type == CPND_EVT_D2ND_CKPT_CREATE) {
			CPSV_CPND_DEST_INFO *dest_list = i_evt->info.cpnd.info.ckpt_create.ckpt_info.dest_list;
			if (i_evt->info.cpnd.info.ckpt_create.ckpt_info.dest_cnt) {
				size =
				    i_evt->info.cpnd.info.ckpt_create.ckpt_info.dest_cnt * sizeof(CPSV_CPND_DEST_INFO);
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)dest_list, size);
			}
		} else if (i_evt->info.cpnd.type == CPSV_D2ND_RESTART_DONE) {
			CPSV_CPND_DEST_INFO *dest_list = i_evt->info.cpnd.info.cpnd_restart_done.dest_list;
			if (i_evt->info.cpnd.info.cpnd_restart_done.dest_cnt) {
				size = i_evt->info.cpnd.info.cpnd_restart_done.dest_cnt * sizeof(CPSV_CPND_DEST_INFO);
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)dest_list, size);
			}
		} else if (i_evt->info.cpnd.type == CPND_EVT_D2ND_CKPT_REP_ADD) {
			CPSV_CPND_DEST_INFO *dest_list = i_evt->info.cpnd.info.ckpt_add.dest_list;
			if (i_evt->info.cpnd.info.ckpt_add.dest_cnt) {
				size = i_evt->info.cpnd.info.ckpt_add.dest_cnt * sizeof(CPSV_CPND_DEST_INFO);
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)dest_list, size);
			}
		}
      else if(i_evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_REFCNTSET)
      {
       if(i_evt->info.cpnd.info.refCntsetReq.no_of_nodes)
           cpsv_ref_cnt_encode(o_ub, &i_evt->info.cpnd.info.refCntsetReq);
      }
    }
	return NCSCC_RC_SUCCESS;
}

static uint32_t cpsv_evt_enc_sec_id(NCS_UBAID *o_ub, SaCkptSectionIdT *sec_id)
{
	uint32_t size;

	size = sizeof(SaCkptSectionIdT);
	ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id, size);
	if (sec_id->idLen) {
		size = sec_id->idLen;
		ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, size);
	}
	return NCSCC_RC_SUCCESS;
}

static SaCkptSectionIdT *cpsv_evt_dec_sec_id(NCS_UBAID *i_ub, uint32_t svc_id)
{
	uint32_t size;
	SaCkptSectionIdT *sec_id = NULL;

	size = sizeof(SaCkptSectionIdT);
	sec_id = (SaCkptSectionIdT *)m_MMGR_ALLOC_CPSV_SaCkptSectionIdT(svc_id);

	if (sec_id == NULL) {
		return NULL;
	}

	if (sec_id)
		ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id, size);

	if (sec_id->idLen) {
		size = sec_id->idLen;
		sec_id->id = m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, svc_id);
		if (sec_id->id) {
			ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
			sec_id->id[size] = 0;
		}
	}
	return sec_id;
}

/****************************************************************************\
 PROCEDURE NAME : cpsv_ckpt_data_decode

 DESCRIPTION    : This routine will decode the contents of CPSV_EVT into user buf

 ARGUMENTS      :  CPSV_CKPT_DATA **data.
                  *io_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/

uint32_t cpsv_ckpt_data_decode(CPSV_CKPT_DATA **data, NCS_UBAID *io_uba)
{
	uint8_t *pstream = NULL;
	uint8_t local_data[5];
	uint16_t num_of_nodes;
	uint32_t rc;
	CPSV_CKPT_DATA *pdata;

	pstream = ncs_dec_flatten_space(io_uba, local_data, 2);
	num_of_nodes = ncs_decode_16bit(&pstream);
	ncs_dec_skip_space(io_uba, 2);

	/* For The First Node */
	if (num_of_nodes) {
		*data = m_MMGR_ALLOC_CPSV_CKPT_DATA;
		if (!(*data))
			return NCSCC_RC_FAILURE;
	}

	pdata = *data;
	while (num_of_nodes) {
		memset(pdata, 0, sizeof(CPSV_CKPT_DATA));
		rc = cpsv_ckpt_node_decode(pdata, io_uba);
		if (rc != NCSCC_RC_SUCCESS)
			return NCSCC_RC_FAILURE;
		num_of_nodes--;
		if (num_of_nodes) {	/* For rest of the Nodes of the Linked List */
			pdata->next = m_MMGR_ALLOC_CPSV_CKPT_DATA;
			if (!pdata->next) {
				return NCSCC_RC_FAILURE;
			}
			pdata = pdata->next;
		}
	}

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************\
 PROCEDURE NAME : cpsv_ckpt_node_decode

 DESCRIPTION    : This routine will decode the contents of CPSV_EVT into user buf

 ARGUMENTS      :  CPSV_CKPT_DATA *pdata.
                  *io_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/

uint32_t cpsv_ckpt_node_decode(CPSV_CKPT_DATA *pdata, NCS_UBAID *io_uba)
{
	uint8_t *pstream = NULL;
	uint8_t local_data[50];
	uint32_t size;

	pstream = ncs_dec_flatten_space(io_uba, local_data, 2);
	pdata->sec_id.idLen = ncs_decode_16bit(&pstream);
	ncs_dec_skip_space(io_uba, 2);

	/* Allocate Memory for sec_id.id */
	if (pdata->sec_id.idLen) {
		pdata->sec_id.id = m_MMGR_ALLOC_CPSV_DEFAULT_VAL(pdata->sec_id.idLen + 1, NCS_SERVICE_ID_CPND);
		if (!pdata->sec_id.id) {
			m_MMGR_FREE_CPSV_CKPT_DATA(pdata);
			return NCSCC_RC_FAILURE;
		}
		memset(pdata->sec_id.id, 0, pdata->sec_id.idLen + 1);
		ncs_decode_n_octets_from_uba(io_uba, pdata->sec_id.id, (uint32_t)pdata->sec_id.idLen);
	}
	size = 8 + 8 + 8;
	pstream = ncs_dec_flatten_space(io_uba, local_data, size);
	pdata->expirationTime = ncs_decode_64bit(&pstream);
	pdata->dataSize = ncs_decode_64bit(&pstream);
	pdata->readSize = ncs_decode_64bit(&pstream);
	ncs_dec_skip_space(io_uba, size);

	/* Allocate Memory for the data */
	if (pdata->dataSize) {
		pdata->data = m_MMGR_ALLOC_CPSV_SYS_MEMORY(pdata->dataSize);
		if (!pdata->data) {
			m_MMGR_FREE_CPSV_DEFAULT_VAL(pdata->sec_id.id, NCS_SERVICE_ID_CPND);
			m_MMGR_FREE_CPSV_CKPT_DATA(pdata);
			return NCSCC_RC_FAILURE;
		}
		memset(pdata->data, 0, pdata->dataSize);

		ncs_decode_n_octets_from_uba(io_uba, pdata->data, (uint32_t)pdata->dataSize);
	}
	pstream = ncs_dec_flatten_space(io_uba, local_data, 8);
	pdata->dataOffset = ncs_decode_64bit(&pstream);
	ncs_dec_skip_space(io_uba, 8);

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
  Name          : cpsv_nd2a_read_data_decode

  Description   : This function decodes an events sent to CPA.

  Arguments     : CPSV_ND2A_READ_DATA *read_data
                  NCS_UBAID *io_uba

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : This is done as Replacement for EDU decode for Read Data.
******************************************************************************/

uint32_t cpsv_nd2a_read_data_decode(CPSV_ND2A_READ_DATA *read_data, NCS_UBAID *io_uba)
{

	uint8_t *pstream = NULL;
	uint8_t local_data[10];

	pstream = ncs_dec_flatten_space(io_uba, local_data, 4);
	read_data->read_size = ncs_decode_32bit(&pstream);
	ncs_dec_skip_space(io_uba, 4);

	if (read_data->read_size) {
		/* Allocate Memory For read_data->data */
		read_data->data = m_MMGR_ALLOC_CPSV_DEFAULT_VAL(read_data->read_size, NCS_SERVICE_ID_CPA);
		if (!read_data->data) {
			/* Free the Above Memory and Return */
			m_MMGR_FREE_CPSV_ND2A_READ_DATA(read_data, NCS_SERVICE_ID_CPA);
			return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE,
					       "Memory alloc failed in cpsv_nd2a_read_data_decode \n");
		}
		memset(read_data->data, 0, read_data->read_size);
		ncs_decode_n_octets_from_uba(io_uba, read_data->data, (uint32_t)read_data->read_size);

	}
	pstream = ncs_dec_flatten_space(io_uba, local_data, 4);
	read_data->err = ncs_decode_32bit(&pstream);
	ncs_dec_skip_space(io_uba, 4);

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************\
 PROCEDURE NAME : cpsv_data_access_rsp_decode

 DESCRIPTION    : This routine will decode the contents of CPSV_EVT into user buf

 ARGUMENTS      :  CPSV_ND2A_DATA_ACCESS_RSP *data_rsp .
                  *io_uba  - User Buff.
				  MDS_CLIENT_MSG_FORMAT_VER.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uint32_t cpsv_data_access_rsp_decode(CPSV_ND2A_DATA_ACCESS_RSP *data_rsp, NCS_UBAID *io_uba , MDS_CLIENT_MSG_FORMAT_VER i_msg_fmt_ver)
{
  
   uint8_t local_data[1024];
   uint8_t *pstream;
   uint32_t i,size , rc =NCSCC_RC_SUCCESS;
   uint32_t ver_compare = 4; /* CPND_MDS_PVT_SUBPART_VERSION  and CPA_MDS_PVT_SUBPART_VERSION*/  
   
   if ( i_msg_fmt_ver >= ver_compare)
	   size = 4 + 4 + 4 + 4 + 8 + 4 + 8 + 8;
   else
	   size = 4 + 4 + 4 + 4 + 8 + 4 + 8;
   pstream = ncs_dec_flatten_space(io_uba, local_data , size);
   data_rsp->type      =     ncs_decode_32bit(&pstream);
   data_rsp->num_of_elmts =  ncs_decode_32bit(&pstream);
   data_rsp->error     =     ncs_decode_32bit(&pstream);
   data_rsp->size      =     ncs_decode_32bit(&pstream);
   data_rsp->ckpt_id   =     ncs_decode_64bit(&pstream);
   data_rsp->error_index      =     ncs_decode_32bit(&pstream);
   data_rsp->from_svc  =     ncs_decode_64bit(&pstream);
   if ( i_msg_fmt_ver >= ver_compare)  
	   data_rsp->lcl_ckpt_id   = ncs_decode_64bit(&pstream);
   ncs_dec_skip_space(io_uba, size);
   if(data_rsp->type == CPSV_DATA_ACCESS_WRITE_RSP)
   {
       size = data_rsp->size * sizeof(SaUint32T);
       SaUint32T* write_err_index =NULL;
        /* Allocate Memory for data_rsp->info.write_err_index */
       if(size)
       {
          data_rsp->info.write_err_index = m_MMGR_ALLOC_CPSV_SaUint32T(data_rsp->size, NCS_SERVICE_ID_CPA);
          if(!data_rsp->info.write_err_index)
          {
              return NCSCC_RC_FAILURE;
	      }
           write_err_index = data_rsp->info.write_err_index;
           memset(write_err_index , 0 , size);
           pstream = ncs_dec_flatten_space(io_uba, local_data , size);
           for(i = 0 ; i< data_rsp->size ; i++)
           {
               /* Encode Write Error Index */
               write_err_index[i] = ncs_decode_32bit(&pstream);
           }
           ncs_dec_skip_space(io_uba , size);
         }

	} else if ((data_rsp->type == CPSV_DATA_ACCESS_LCL_READ_RSP)
		   || (data_rsp->type == CPSV_DATA_ACCESS_RMT_READ_RSP)) {
		if (data_rsp->size) {
			data_rsp->info.read_data = m_MMGR_ALLOC_CPSV_ND2A_READ_DATA(data_rsp->size, NCS_SERVICE_ID_CPA);
			if (!data_rsp->info.read_data) {
				return NCSCC_RC_FAILURE;

			}
			memset(data_rsp->info.read_data, 0, data_rsp->size * sizeof(CPSV_ND2A_READ_DATA));

			for (i = 0; i < data_rsp->size; i++)
				rc = cpsv_nd2a_read_data_decode(&data_rsp->info.read_data[i], io_uba);
		}
	} else if (data_rsp->type == CPSV_DATA_ACCESS_OVWRITE_RSP) {
		pstream = ncs_dec_flatten_space(io_uba, local_data, 4);
		data_rsp->info.ovwrite_error.error = ncs_decode_32bit(&pstream);
		ncs_dec_skip_space(io_uba, 4);
	} else {
		rc = NCSCC_RC_FAILURE;
	}

	return rc;

}

/****************************************************************************\
 PROCEDURE NAME : cpsv_evt_dec_flat

 DESCRIPTION    : This routine will decode the contents of CPSV_EVT into user buf

 ARGUMENTS      : *i_evt - Event Struct.
                  *o_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uint32_t cpsv_evt_dec_flat(EDU_HDL *edu_hdl, NCS_UBAID *i_ub, CPSV_EVT *o_evt)
{
	uint32_t size = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	size = sizeof(CPSV_EVT);

	/* Decode the Top level evt envolop */
	ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)o_evt, size);

	/* Decode the internal Pointers */
	if (o_evt->type == CPSV_EVT_TYPE_CPA) {
		switch (o_evt->info.cpa.type) {
		case CPA_EVT_ND2A_CKPT_DATA_RSP:
			{
				CPSV_ND2A_DATA_ACCESS_RSP *data_rsp = &o_evt->info.cpa.info.sec_data_rsp;
				if ((data_rsp->type == CPSV_DATA_ACCESS_LCL_READ_RSP)
				    || (data_rsp->type == CPSV_DATA_ACCESS_RMT_READ_RSP)) {
					if (data_rsp->num_of_elmts == -1) {
						data_rsp->info.read_data = NULL;
					} else {
						data_rsp->info.read_data =
						    m_MMGR_ALLOC_CPSV_ND2A_READ_DATA(data_rsp->num_of_elmts,
										     NCS_SERVICE_ID_CPA);
						if (data_rsp->info.read_data) {
							uint32_t iter = 0;
							for (; iter < data_rsp->size; iter++) {
								size = sizeof(CPSV_ND2A_READ_DATA);
								ncs_decode_n_octets_from_uba(i_ub,
											     (uint8_t *)&data_rsp->
											     info.read_data[iter],
											     size);
								if (data_rsp->info.read_data[iter].read_size > 0) {
									size = data_rsp->info.read_data[iter].read_size;
									data_rsp->info.read_data[iter].data =
									    m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size,
													  NCS_SERVICE_ID_CPA);
									if (data_rsp->info.read_data[iter].data) {
										ncs_decode_n_octets_from_uba(i_ub,
													     data_rsp->info.read_data
													     [iter].data,
													     size);
									}
								}
							}
						}
					}
				} else if (data_rsp->type == CPSV_DATA_ACCESS_WRITE_RSP) {
					if (data_rsp->num_of_elmts == -1) {
						data_rsp->info.write_err_index = NULL;
					} else {
						size = data_rsp->num_of_elmts * sizeof(SaUint32T);
						data_rsp->info.write_err_index =
						    m_MMGR_ALLOC_CPSV_SaUint32T(data_rsp->num_of_elmts,
										NCS_SERVICE_ID_CPA);
						if (data_rsp->info.write_err_index)
							ncs_decode_n_octets_from_uba(i_ub,
										     (uint8_t *)data_rsp->
										     info.write_err_index, size);
					}
				}
				break;
			}
		case CPA_EVT_ND2A_CKPT_CLM_NODE_LEFT:
			{
				/* do nothing */

				break;
			}
		case CPA_EVT_ND2A_CKPT_CLM_NODE_JOINED:
			{
				/*do nothing */

				break;
			}
		case CPA_EVT_ND2A_CKPT_ARRIVAL_NTFY:
			{
				CPSV_CKPT_DATA *data;
				EDU_ERR ederror = 0;

				data = NULL;	/* Explicity set it to NULL */

				rc = m_NCS_EDU_EXEC(edu_hdl, FUNC_NAME(CPSV_CKPT_DATA),
						    i_ub, EDP_OP_TYPE_DEC, &data, &ederror);
				if (rc != NCSCC_RC_SUCCESS) {
					return rc;
				}
				o_evt->info.cpa.info.arr_msg.ckpt_data = data;
				break;
			}

		case CPA_EVT_ND2A_SEC_CREATE_RSP:
			{
				SaCkptSectionIdT *sec_id = &o_evt->info.cpa.info.sec_creat_rsp.sec_id;
				if (sec_id) {
					size = sec_id->idLen;
					if (size) {
						sec_id->id =
						    m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, NCS_SERVICE_ID_CPND);
						if (sec_id->id) {
							ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
							sec_id->id[size] = 0;
						}
					}
				}
				break;
			}
		case CPA_EVT_ND2A_SEC_ITER_GETNEXT_RSP:
			{
				SaCkptSectionIdT *sec_id = &o_evt->info.cpa.info.iter_next_rsp.sect_desc.sectionId;
				if (sec_id) {
					size = sec_id->idLen;
					if (size) {
						sec_id->id =
						    m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, NCS_SERVICE_ID_CPND);
						if (sec_id->id) {
							ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
							sec_id->id[size] = 0;
						}
					}
				}
				break;
			}
		default:
			break;
		}
	} else if (o_evt->type == CPSV_EVT_TYPE_CPND) {
		switch (o_evt->info.cpnd.type) {
		case CPND_EVT_A2ND_CKPT_WRITE:
			cpsv_ckpt_data_decode(&o_evt->info.cpnd.info.ckpt_write.data, i_ub);

			break;

		case CPND_EVT_A2ND_CKPT_READ:
			cpsv_ckpt_data_decode(&o_evt->info.cpnd.info.ckpt_write.data, i_ub);
			break;
			/* Added for 3.0.B , these events decoding is missing in 3.0.2 */
		case CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC:

			cpsv_ckpt_data_decode(&o_evt->info.cpnd.info.ckpt_nd2nd_sync.data, i_ub);
			break;

		case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ:

			cpsv_ckpt_data_decode(&o_evt->info.cpnd.info.ckpt_nd2nd_data.data, i_ub);
			break;

		case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP:
			{
				CPSV_ND2A_DATA_ACCESS_RSP *data_rsp = &o_evt->info.cpnd.info.ckpt_nd2nd_data_rsp;

				if (data_rsp->type == CPSV_DATA_ACCESS_WRITE_RSP) {
					if (data_rsp->num_of_elmts == -1) {
						data_rsp->info.write_err_index = NULL;
					} else {
						size = data_rsp->num_of_elmts * sizeof(SaUint32T);
						if (size) {
							data_rsp->info.write_err_index =
							    m_MMGR_ALLOC_CPSV_SaUint32T(data_rsp->num_of_elmts,
											NCS_SERVICE_ID_CPA);
							if (data_rsp->info.write_err_index)
								ncs_decode_n_octets_from_uba(i_ub,
											     (uint8_t *)data_rsp->
											     info.write_err_index,
											     size);
						} else
							data_rsp->info.write_err_index = NULL;
					}
				}
				break;
			}
		case CPSV_EVT_ND2ND_CKPT_SECT_CREATE_REQ:
			{
				CPSV_CKPT_SECT_CREATE *create = &o_evt->info.cpnd.info.active_sec_creat;

				if (create->sec_attri.sectionId) {
					create->sec_attri.sectionId = cpsv_evt_dec_sec_id(i_ub, NCS_SERVICE_ID_CPND);
				}
				if (create->sec_attri.sectionId == NULL) {
					return NCSCC_RC_FAILURE;
				}

				if (create->init_size) {
					create->init_data =
					    (void *)m_MMGR_ALLOC_CPSV_DEFAULT_VAL(create->init_size,
										  NCS_SERVICE_ID_CPND);
					ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)create->init_data,
								     create->init_size);
				} else
					create->init_data = NULL;
				break;
			}
		case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_CREATE_RSP:
			{
				SaCkptSectionIdT *sec_id = &o_evt->info.cpnd.info.active_sec_creat_rsp.sec_id;
				if (sec_id) {
					size = sec_id->idLen;
					if (size) {
						sec_id->id =
						    m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, NCS_SERVICE_ID_CPND);
						if (sec_id->id) {
							ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
							sec_id->id[size] = 0;
						}
					} else
						sec_id->id = NULL;
				}
				break;
			}
		case CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ:
			{
				SaCkptSectionIdT *sec_id = &o_evt->info.cpnd.info.sec_delete_req.sec_id;
				if (sec_id) {
					size = sec_id->idLen;
					if (size) {
						sec_id->id =
						    m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, NCS_SERVICE_ID_CPND);
						if (sec_id->id) {
							ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
							sec_id->id[size] = 0;
						}
					} else
						sec_id->id = NULL;
				}
				break;
			}
			/* End of - Added for 3.0.B , these events decoding is missing in 3.0.2 */
		case CPND_EVT_A2ND_CKPT_SECT_CREATE:
			{
				uint32_t size;
				CPSV_CKPT_SECT_CREATE *create = &o_evt->info.cpnd.info.sec_creatReq;

				if (create->sec_attri.sectionId) {
					create->sec_attri.sectionId = cpsv_evt_dec_sec_id(i_ub, NCS_SERVICE_ID_CPND);
				}

				size = create->init_size;
				if (size != 0) {
					create->init_data =
					    (void *)m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size, NCS_SERVICE_ID_CPND);
					ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)create->init_data, size);
				}
				break;
			}
		case CPND_EVT_A2ND_CKPT_ITER_GETNEXT:
			{
				uint32_t size = 0;
				SaCkptSectionIdT *sec_id = &o_evt->info.cpnd.info.iter_getnext.section_id;
				if (sec_id) {
					size = sec_id->idLen;
					if (size) {
						sec_id->id =
						    m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, NCS_SERVICE_ID_CPND);
						if (sec_id->id) {
							ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
							sec_id->id[size] = 0;
					}
					} else
						sec_id->id = NULL;
				}
				break;
			}

		case CPND_EVT_A2ND_CKPT_SECT_DELETE:

			if (o_evt->info.cpnd.info.sec_delReq.sec_id.idLen) {
				SaCkptSectionIdT *sec_id = &o_evt->info.cpnd.info.sec_delReq.sec_id;
				uint32_t size = o_evt->info.cpnd.info.sec_delReq.sec_id.idLen;
				if (size) {
					sec_id->id = m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, NCS_SERVICE_ID_CPND);
					if (sec_id->id) {
					ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
						sec_id->id[size] = 0;
					}
				} else
					sec_id->id = NULL;
			}
			break;

		case CPND_EVT_A2ND_CKPT_SECT_EXP_SET:

			if (o_evt->info.cpnd.info.sec_expset.sec_id.idLen) {
				SaCkptSectionIdT *sec_id = &o_evt->info.cpnd.info.sec_expset.sec_id;
				uint32_t size = o_evt->info.cpnd.info.sec_expset.sec_id.idLen;
				if (size) {
					sec_id->id = m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, NCS_SERVICE_ID_CPND);
					if (sec_id->id) {
					ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
						sec_id->id[size] = 0;
					}
				} else
					sec_id->id = NULL;
			}
			break;

		case CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_REQ:

			if (o_evt->info.cpnd.info.sec_exp_set.sec_id.idLen) {
				SaCkptSectionIdT *sec_id = &o_evt->info.cpnd.info.sec_exp_set.sec_id;
				uint32_t size = o_evt->info.cpnd.info.sec_exp_set.sec_id.idLen;
				if (size) {
					sec_id->id = m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, NCS_SERVICE_ID_CPND);
					if (sec_id->id) {
						ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
						sec_id->id[size] = 0;
					}
				} else
					sec_id->id = NULL;

			}
			break;

		case CPND_EVT_D2ND_CKPT_INFO:

			if (o_evt->info.cpnd.info.ckpt_info.dest_cnt) {
				CPSV_CPND_DEST_INFO *dest_list = 0;
				uint32_t size = sizeof(CPSV_CPND_DEST_INFO) * o_evt->info.cpnd.info.ckpt_info.dest_cnt;
				if (size)
					dest_list = m_MMGR_ALLOC_CPSV_SYS_MEMORY(size);
				if (dest_list && size)
					ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)dest_list, size);
				o_evt->info.cpnd.info.ckpt_info.dest_list = dest_list;
			}
			break;

		case CPND_EVT_D2ND_CKPT_CREATE:

			if (o_evt->info.cpnd.info.ckpt_create.ckpt_info.dest_cnt) {
				CPSV_CPND_DEST_INFO *dest_list = 0;
				uint32_t size = sizeof(CPSV_CPND_DEST_INFO) *
				    o_evt->info.cpnd.info.ckpt_create.ckpt_info.dest_cnt;
				if (size)
					dest_list = m_MMGR_ALLOC_CPSV_SYS_MEMORY(size);
				if (dest_list && size)
					ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)dest_list, size);
				o_evt->info.cpnd.info.ckpt_create.ckpt_info.dest_list = dest_list;
			}
			break;

		case CPSV_D2ND_RESTART_DONE:

			if (o_evt->info.cpnd.info.cpnd_restart_done.dest_cnt) {
				CPSV_CPND_DEST_INFO *dest_list = 0;
				uint32_t size = sizeof(CPSV_CPND_DEST_INFO) *
				    o_evt->info.cpnd.info.cpnd_restart_done.dest_cnt;
				if (size)
					dest_list = m_MMGR_ALLOC_CPSV_SYS_MEMORY(size);
				if (dest_list && size)
					ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)dest_list, size);
				o_evt->info.cpnd.info.cpnd_restart_done.dest_list = dest_list;
			}
			break;

		case CPND_EVT_D2ND_CKPT_REP_ADD:

			if (o_evt->info.cpnd.info.ckpt_add.dest_cnt) {
				CPSV_CPND_DEST_INFO *dest_list = 0;
				uint32_t size = sizeof(CPSV_CPND_DEST_INFO) * o_evt->info.cpnd.info.ckpt_add.dest_cnt;
				if (size)
					dest_list = m_MMGR_ALLOC_CPSV_SYS_MEMORY(size);
				if (dest_list && size)
					ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)dest_list, size);
				o_evt->info.cpnd.info.ckpt_add.dest_list = dest_list;
			}
			break;
      case CPND_EVT_A2ND_CKPT_REFCNTSET:
         if(o_evt->info.cpnd.info.refCntsetReq.no_of_nodes)
           cpsv_refcnt_ckptid_decode(&o_evt->info.cpnd.info.refCntsetReq,i_ub ); 
     break;
		default:
			break;
		}
	}

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:    cpsv_dbg_sink

  DESCRIPTION:

   cpsv is instrumented to fall into this debug sink function
   if it hits any if-conditional that fails, but should not fail. This
   is the default implementation of that SINK macro.

  ARGUMENTS:

  uint32_t   l             line # in file
  char*   f             file name where macro invoked
  code    code          Error code value.. Usually FAILURE

  RETURNS:

  code    - just echo'ed back

*****************************************************************************/

#if ((NCS_CPND == 1) || (NCS_CPD == 1))

uint32_t cpsv_dbg_sink(uint32_t l, char *f, uint32_t code, char *str)
{
	TRACE("In file %s at line %d ", f, l);

	if (NULL != str)
		TRACE("Reason : %s ", str);

	return code;
}

#endif
