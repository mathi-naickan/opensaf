/*      -*- OpenSAF  -*-
 * File:   lgs_mbcsv_v1.c
 *
 * (C) Copyright 2014 The OpenSAF Foundation
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

/*
 * Contains functions for encoding/decoding check-point structures for
 * version 1 check-pointing. See also lgs_mbcsv_v1.h
 */

#include "lgs_mbcsv_v1.h"

/****************************************************************************
 * Name          : edp_ed_write_rec_v1
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 lgsv checkpoint write log rec.
 * 
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode. 
 *                 EDU_ERR - out param to indicate errors in processing. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

uint32_t edp_ed_write_rec_v1(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			      NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_ckpt_write_log_v1_t *ckpt_write_msg_ptr = NULL, **ckpt_write_msg_dec_ptr;

	EDU_INST_SET ckpt_write_rec_ed_rules[] = {
		{EDU_START, edp_ed_write_rec_v1, 0, 0, 0, sizeof(lgs_ckpt_write_log_v1_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_write_log_v1_t *)0)->recordId, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_write_log_v1_t *)0)->streamId, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_write_log_v1_t *)0)->curFileSize, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_write_log_v1_t *)0)->logFileCurrent, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_write_msg_ptr = (lgs_ckpt_write_log_v1_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_write_msg_dec_ptr = (lgs_ckpt_write_log_v1_t **)ptr;
		if (*ckpt_write_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_write_msg_dec_ptr, '\0', sizeof(lgs_ckpt_write_log_v1_t));
		ckpt_write_msg_ptr = *ckpt_write_msg_dec_ptr;
	} else {
		ckpt_write_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_write_rec_ed_rules, ckpt_write_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/* End edp_ed_write_rec */

/****************************************************************************
 * Name          : edp_ed_close_stream_rec_v1
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 lgsv checkpoint close_stream log rec.
 * 
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode. 
 *                 EDU_ERR - out param to indicate errors in processing. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

uint32_t edp_ed_close_stream_rec_v1(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				     NCSCONTEXT ptr, uint32_t *ptr_data_len,
				     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_ckpt_stream_close_v1_t *ckpt_close_stream_msg_ptr = NULL, **ckpt_close_stream_msg_dec_ptr;

	EDU_INST_SET ckpt_close_stream_rec_ed_rules[] = {
		{EDU_START, edp_ed_close_stream_rec_v1, 0, 0, 0, sizeof(lgs_ckpt_stream_close_v1_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_close_v1_t *)0)->streamId, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_close_v1_t *)0)->clientId, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_close_stream_msg_ptr = (lgs_ckpt_stream_close_v1_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_close_stream_msg_dec_ptr = (lgs_ckpt_stream_close_v1_t **)ptr;
		if (*ckpt_close_stream_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_close_stream_msg_dec_ptr, '\0', sizeof(lgs_ckpt_stream_close_v1_t));
		ckpt_close_stream_msg_ptr = *ckpt_close_stream_msg_dec_ptr;
	} else {
		ckpt_close_stream_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_close_stream_rec_ed_rules,
				 ckpt_close_stream_msg_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;

}	/* End edp_ed_close_stream_rec */

/****************************************************************************
 * Name          : edp_ed_finalize_rec_v1
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 lgsv checkpoint finalize async updates record.
 * 
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode. 
 *                 EDU_ERR - out param to indicate errors in processing. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

uint32_t edp_ed_finalize_rec_v1(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				 NCSCONTEXT ptr, uint32_t *ptr_data_len,
				 EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_ckpt_finalize_msg_v1_t *ckpt_final_msg_ptr = NULL, **ckpt_final_msg_dec_ptr;

	EDU_INST_SET ckpt_final_rec_ed_rules[] = {
		{EDU_START, edp_ed_finalize_rec_v1, 0, 0, 0, sizeof(lgs_ckpt_finalize_msg_v1_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_finalize_msg_v1_t *)0)->client_id, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_final_msg_ptr = (lgs_ckpt_finalize_msg_v1_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_final_msg_dec_ptr = (lgs_ckpt_finalize_msg_v1_t **)ptr;
		if (*ckpt_final_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_final_msg_dec_ptr, '\0', sizeof(lgs_ckpt_finalize_msg_v1_t));
		ckpt_final_msg_ptr = *ckpt_final_msg_dec_ptr;
	} else {
		ckpt_final_msg_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_final_rec_ed_rules, ckpt_final_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/* End edp_ed_finalize_rec() */

/****************************************************************************
 * Name          : edp_ed_cfg_stream_rec_v1
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 lgsv checkpoint cfg_update_stream log rec.
 * 
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode. 
 *                 EDU_ERR - out param to indicate errors in processing. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

uint32_t edp_ed_cfg_stream_rec_v1(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uint32_t *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_ckpt_stream_cfg_v1_t *ckpt_stream_cfg_msg_ptr = NULL, **ckpt_stream_cfg_msg_dec_ptr;

	EDU_INST_SET ckpt_stream_cfg_rec_ed_rules[] = {
		{EDU_START, edp_ed_cfg_stream_rec_v1, 0, 0, 0, sizeof(lgs_ckpt_stream_cfg_v1_t), 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v1_t *)0)->name, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v1_t *)0)->fileName, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v1_t *)0)->pathName, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v1_t *)0)->maxLogFileSize, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v1_t *)0)->fixedLogRecordSize, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v1_t *)0)->haProperty, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v1_t *)0)->logFullAction, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v1_t *)0)->logFullHaltThreshold, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v1_t *)0)->maxFilesRotated, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v1_t *)0)->logFileFormat, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v1_t *)0)->severityFilter, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v1_t *)0)->logFileCurrent, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_stream_cfg_msg_ptr = (lgs_ckpt_stream_cfg_v1_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_stream_cfg_msg_dec_ptr = (lgs_ckpt_stream_cfg_v1_t **)ptr;
		if (*ckpt_stream_cfg_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_stream_cfg_msg_dec_ptr, '\0', sizeof(lgs_ckpt_stream_cfg_v1_t));
		ckpt_stream_cfg_msg_ptr = *ckpt_stream_cfg_msg_dec_ptr;
	} else {
		ckpt_stream_cfg_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_stream_cfg_rec_ed_rules, ckpt_stream_cfg_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}

/****************************************************************************
 * Name          : edp_ed_ckpt_msg_v1
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 lgsv checkpoint messages. This program runs the 
 *                 edp_ed_hdr_rec program first to decide the
 *                 checkpoint message type based on which it will call the
 *                 appropriate EDU programs for the different checkpoint 
 *                 messages. 
 * 
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode. 
 *                 EDU_ERR - out param to indicate errors in processing. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

uint32_t edp_ed_ckpt_msg_v1(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,
				EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgsv_ckpt_msg_v1_t *ckpt_msg_ptr = NULL, **ckpt_msg_dec_ptr;

	EDU_INST_SET ckpt_msg_ed_rules[] = {
		{EDU_START, edp_ed_ckpt_msg_v1, 0, 0, 0, sizeof(lgsv_ckpt_msg_v1_t), 0, NULL},
		{EDU_EXEC, edp_ed_header_rec, 0, 0, 0, (long)&((lgsv_ckpt_msg_v1_t *)0)->header, 0, NULL},

		{EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((lgsv_ckpt_msg_v1_t *)0)->header, 0,
		 (EDU_EXEC_RTINE)ckpt_msg_test_type},

		/* Reg Record */
		{EDU_EXEC, edp_ed_reg_rec, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_v1_t *)0)->ckpt_rec.initialize_client, 0, NULL},

		/* Finalize record */
		{EDU_EXEC, edp_ed_finalize_rec_v1, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_v1_t *)0)->ckpt_rec.finalize_client, 0, NULL},

		/* write log Record */
		{EDU_EXEC, edp_ed_write_rec_v1, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_v1_t *)0)->ckpt_rec.write_log, 0, NULL},

		/* Open stream */
		{EDU_EXEC, edp_ed_open_stream_rec, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_v1_t *)0)->ckpt_rec.stream_open, 0, NULL},

		/* Close stream */
		{EDU_EXEC, edp_ed_close_stream_rec_v1, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_v1_t *)0)->ckpt_rec.stream_close, 0, NULL},

		/* Agent dest */
		{EDU_EXEC, ncs_edp_mds_dest, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_v1_t *)0)->ckpt_rec.agent_dest, 0, NULL},

		/* Cfg stream */
		{EDU_EXEC, edp_ed_cfg_stream_rec_v1, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_v1_t *)0)->ckpt_rec.stream_cfg, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_msg_ptr = (lgsv_ckpt_msg_v1_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_msg_dec_ptr = (lgsv_ckpt_msg_v1_t **)ptr;
		if (*ckpt_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_msg_dec_ptr, '\0', sizeof(lgsv_ckpt_msg_v1_t));
		ckpt_msg_ptr = *ckpt_msg_dec_ptr;
	} else {
		ckpt_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_msg_ed_rules, ckpt_msg_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;

}	/* End edu_enc_dec_ckpt_msg() */
