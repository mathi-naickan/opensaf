/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2014 The OpenSAF Foundation
 * File:   lgs_mbcsv_v2.c
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

#include "lgs_mbcsv_v2.h"
#include "lgs_mbcsv.h"

/****************************************************************************
 * Name          : ckpt_proc_lgs_cfg
 *
 * Description   : This function configures updates log service configuration.
 * 
 * NOTE! Changing lgs config does not work if standby is V1. Cannot be fixed
 *       since V2 has no applier for config object and V1 does not checkpoint
 *       config data!
 *
 * Arguments     : cb - pointer to LGS  ControlBlock.
 *                 data - pointer to  LGS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

uint32_t ckpt_proc_lgs_cfg_v2(lgs_cb_t *cb, void *data)
{
	TRACE_ENTER();
	if (!lgs_is_peer_v2()) {
		LOG_ER("%s ERROR: Called when ceck-point version 1",__FUNCTION__);
		osafassert(0);
	}
	lgsv_ckpt_msg_v2_t *data_v2 = data;
	lgs_ckpt_lgs_cfg_v2_t *param = &data_v2->ckpt_rec.lgs_cfg;
	
	/* Handle log files for new directory if configured for split file system */
	if (lgs_is_split_file_system()) {
		/* Setting new root directory is done in logRootDirectory_filemove() */
		logRootDirectory_filemove(param->logRootDirectory,
				(time_t *) &param->c_file_close_time_stamp);
	} else {
		/* Save new root path */
		lgs_imm_rootpathconf_set(param->logRootDirectory);		
	}
	
	TRACE("Setting new rootpath on standby \"%s\"",param->logRootDirectory);
	
	/* Free strings allocated by the EDU encoder */
	lgs_free_edu_mem(param->logRootDirectory);
	
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : edp_ed_write_rec
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

uint32_t edp_ed_write_rec_v2(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			      NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,
		          EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_ckpt_write_log_v2_t *ckpt_write_msg_ptr = NULL, **ckpt_write_msg_dec_ptr;
	
	EDU_INST_SET ckpt_write_rec_ed_rules[] = {
		{EDU_START, edp_ed_write_rec_v2, 0, 0, 0, sizeof(lgs_ckpt_write_log_v2_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_write_log_v2_t *)0)->recordId, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_write_log_v2_t *)0)->streamId, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_write_log_v2_t *)0)->curFileSize, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_write_log_v2_t *)0)->logFileCurrent, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_write_log_v2_t *)0)->logRecord, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((lgs_ckpt_write_log_v2_t *)0)->c_file_close_time_stamp, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};
	
	if (op == EDP_OP_TYPE_ENC) {
		ckpt_write_msg_ptr = (lgs_ckpt_write_log_v2_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_write_msg_dec_ptr = (lgs_ckpt_write_log_v2_t **)ptr;
		if (*ckpt_write_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_write_msg_dec_ptr, '\0', sizeof(lgs_ckpt_write_log_v2_t));
		ckpt_write_msg_ptr = *ckpt_write_msg_dec_ptr;
	} else {
		ckpt_write_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_write_rec_ed_rules, ckpt_write_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	
	return rc;

}	/* End edp_ed_write_rec */

/****************************************************************************
 * Name          : edp_ed_open_stream_rec
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 lgsv checkpoint open_stream log rec.
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

uint32_t edp_ed_open_stream_rec_v2(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				    NCSCONTEXT ptr, uint32_t *ptr_data_len,
				    EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_ckpt_stream_open_t *ckpt_open_stream_msg_ptr = NULL, **ckpt_open_stream_msg_dec_ptr;

	EDU_INST_SET ckpt_open_stream_rec_ed_rules[] = {
		{EDU_START, edp_ed_open_stream_rec_v2, 0, 0, 0, sizeof(lgs_ckpt_stream_open_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->streamId, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->clientId, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->logFile, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->logPath, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->logFileCurrent, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->maxFileSize, 0, NULL},
		{EDU_EXEC, ncs_edp_int32, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->maxLogRecordSize, 0, NULL},
		{EDU_EXEC, ncs_edp_int32, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->logFileFullAction, 0, NULL},
		{EDU_EXEC, ncs_edp_int32, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->maxFilesRotated, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->fileFmt, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->logStreamName, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->creationTimeStamp, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->numOpeners, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->streamType, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_open_t *)0)->logRecordId, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_open_stream_msg_ptr = (lgs_ckpt_stream_open_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_open_stream_msg_dec_ptr = (lgs_ckpt_stream_open_t **)ptr;
		if (*ckpt_open_stream_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_open_stream_msg_dec_ptr, '\0', sizeof(lgs_ckpt_stream_open_t));
		ckpt_open_stream_msg_ptr = *ckpt_open_stream_msg_dec_ptr;
	} else {
		ckpt_open_stream_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_open_stream_rec_ed_rules, ckpt_open_stream_msg_ptr,
				 ptr_data_len, buf_env, op, o_err);
	return rc;

}	/* End edp_ed_open_stream_rec */

/****************************************************************************
 * Name          : edp_ed_close_stream_rec
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

uint32_t edp_ed_close_stream_rec_v2(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				     NCSCONTEXT ptr, uint32_t *ptr_data_len,
				     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_ckpt_stream_close_v2_t *ckpt_close_stream_msg_ptr = NULL, **ckpt_close_stream_msg_dec_ptr;
	
	EDU_INST_SET ckpt_close_stream_rec_ed_rules[] = {
		{EDU_START, edp_ed_close_stream_rec_v2, 0, 0, 0, sizeof(lgs_ckpt_stream_close_v2_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_close_v2_t *)0)->streamId, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_close_v2_t *)0)->clientId, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((lgs_ckpt_stream_close_v2_t *)0)->c_file_close_time_stamp, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_close_stream_msg_ptr = (lgs_ckpt_stream_close_v2_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_close_stream_msg_dec_ptr = (lgs_ckpt_stream_close_v2_t **)ptr;
		if (*ckpt_close_stream_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_close_stream_msg_dec_ptr, '\0', sizeof(lgs_ckpt_stream_close_v2_t));
		ckpt_close_stream_msg_ptr = *ckpt_close_stream_msg_dec_ptr;
	} else {
		ckpt_close_stream_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_close_stream_rec_ed_rules,
				 ckpt_close_stream_msg_ptr, ptr_data_len, buf_env, op, o_err);
	
	return rc;

}	/* End edp_ed_close_stream_rec */

/****************************************************************************
 * Name          : edp_ed_finalize_rec
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

uint32_t edp_ed_finalize_rec_v2(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				 NCSCONTEXT ptr, uint32_t *ptr_data_len,
				 EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_ckpt_finalize_msg_v2_t *ckpt_final_msg_ptr = NULL, **ckpt_final_msg_dec_ptr;

	EDU_INST_SET ckpt_final_rec_ed_rules[] = {
		{EDU_START, edp_ed_finalize_rec_v2, 0, 0, 0, sizeof(lgs_ckpt_finalize_msg_v2_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_finalize_msg_v2_t *)0)->client_id, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((lgs_ckpt_finalize_msg_v2_t *)0)->c_file_close_time_stamp, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_final_msg_ptr = (lgs_ckpt_finalize_msg_v2_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_final_msg_dec_ptr = (lgs_ckpt_finalize_msg_v2_t **)ptr;
		if (*ckpt_final_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_final_msg_dec_ptr, '\0', sizeof(lgs_ckpt_finalize_msg_v2_t));
		ckpt_final_msg_ptr = *ckpt_final_msg_dec_ptr;
	} else {
		ckpt_final_msg_ptr = ptr;
	}
	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_final_rec_ed_rules, ckpt_final_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}	/* End edp_ed_finalize_rec() */

/****************************************************************************
 * Name          : edp_ed_cfg_stream_rec
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

uint32_t edp_ed_cfg_stream_rec_v2(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uint32_t *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_ckpt_stream_cfg_v2_t *ckpt_stream_cfg_msg_ptr = NULL, **ckpt_stream_cfg_msg_dec_ptr;

	EDU_INST_SET ckpt_stream_cfg_rec_ed_rules[] = {
		{EDU_START, edp_ed_cfg_stream_rec_v2, 0, 0, 0, sizeof(lgs_ckpt_stream_cfg_v2_t), 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v2_t *)0)->name, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v2_t *)0)->fileName, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v2_t *)0)->pathName, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v2_t *)0)->maxLogFileSize, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v2_t *)0)->fixedLogRecordSize, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v2_t *)0)->haProperty, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v2_t *)0)->logFullAction, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v2_t *)0)->logFullHaltThreshold, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v2_t *)0)->maxFilesRotated, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v2_t *)0)->logFileFormat, 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v2_t *)0)->severityFilter, 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v2_t *)0)->logFileCurrent, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((lgs_ckpt_stream_cfg_v2_t *)0)->c_file_close_time_stamp, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_stream_cfg_msg_ptr = (lgs_ckpt_stream_cfg_v2_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_stream_cfg_msg_dec_ptr = (lgs_ckpt_stream_cfg_v2_t **)ptr;
		if (*ckpt_stream_cfg_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_stream_cfg_msg_dec_ptr, '\0', sizeof(lgs_ckpt_stream_cfg_v2_t));
		ckpt_stream_cfg_msg_ptr = *ckpt_stream_cfg_msg_dec_ptr;
	} else {
		ckpt_stream_cfg_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_stream_cfg_rec_ed_rules, ckpt_stream_cfg_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}

/****************************************************************************
 * Name          : edp_ed_lgs_cfg_rec
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 lgsv checkpoint log service configuration update.
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

uint32_t edp_ed_lgs_cfg_rec_v2(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uint32_t *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_ckpt_lgs_cfg_v2_t *ckpt_lgs_cfg_msg_ptr = NULL, **ckpt_lgs_cfg_msg_dec_ptr;

	EDU_INST_SET ckpt_lgs_cfg_rec_ed_rules[] = {
		{EDU_START, edp_ed_lgs_cfg_rec_v2, 0, 0, 0, sizeof(lgs_ckpt_lgs_cfg_v2_t), 0, NULL},
		{EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_lgs_cfg_v2_t *)0)->logRootDirectory, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((lgs_ckpt_lgs_cfg_v2_t *)0)->c_file_close_time_stamp, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_lgs_cfg_msg_ptr = (lgs_ckpt_lgs_cfg_v2_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_lgs_cfg_msg_dec_ptr = (lgs_ckpt_lgs_cfg_v2_t **)ptr;
		if (*ckpt_lgs_cfg_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_lgs_cfg_msg_dec_ptr, '\0', sizeof(lgs_ckpt_lgs_cfg_v2_t));
		ckpt_lgs_cfg_msg_ptr = *ckpt_lgs_cfg_msg_dec_ptr;
	} else {
		ckpt_lgs_cfg_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_lgs_cfg_rec_ed_rules, ckpt_lgs_cfg_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}

/****************************************************************************
 * Name          : edp_ed_agent_down_rec
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 lgsv checkpoint client down.
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

uint32_t edp_ed_agent_down_rec_v2(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uint32_t *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgs_ckpt_agent_down_v2_t *ckpt_agent_down_msg_ptr = NULL, **ckpt_agent_down_dec_ptr;
	 

	EDU_INST_SET ckpt_lgs_agent_down_ed_rules[] = {
		{EDU_START, edp_ed_agent_down_rec_v2, 0, 0, 0, sizeof(lgs_ckpt_agent_down_v2_t), 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((lgs_ckpt_agent_down_v2_t *)0)->agent_dest, 0, NULL},
		{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((lgs_ckpt_agent_down_v2_t *)0)->c_file_close_time_stamp, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_agent_down_msg_ptr = (lgs_ckpt_agent_down_v2_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_agent_down_dec_ptr = (lgs_ckpt_agent_down_v2_t **)ptr;
		if (*ckpt_agent_down_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_agent_down_dec_ptr, '\0', sizeof(lgs_ckpt_agent_down_v2_t));
		ckpt_agent_down_msg_ptr = *ckpt_agent_down_dec_ptr;
	} else {
		ckpt_agent_down_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_lgs_agent_down_ed_rules, ckpt_agent_down_msg_ptr, ptr_data_len,
				 buf_env, op, o_err);
	return rc;

}

/****************************************************************************
 * Name          : edp_ed_ckpt_msg_v2
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

uint32_t edp_ed_ckpt_msg_v2(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,
				EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	lgsv_ckpt_msg_v2_t *ckpt_msg_ptr = NULL, **ckpt_msg_dec_ptr;

	EDU_INST_SET ckpt_msg_ed_rules[] = {
		{EDU_START, edp_ed_ckpt_msg_v2, 0, 0, 0, sizeof(lgsv_ckpt_msg_v2_t), 0, NULL},
		{EDU_EXEC, edp_ed_header_rec, 0, 0, 0, (long)&((lgsv_ckpt_msg_v2_t *)0)->header, 0, NULL},

		{EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((lgsv_ckpt_msg_v2_t *)0)->header, 0,
		 (EDU_EXEC_RTINE)ckpt_msg_test_type},

		/* Reg Record */
		{EDU_EXEC, edp_ed_reg_rec, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_v2_t *)0)->ckpt_rec.initialize_client, 0, NULL},

		/* Finalize record */
		{EDU_EXEC, edp_ed_finalize_rec_v2, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_v2_t *)0)->ckpt_rec.finalize_client, 0, NULL},

		/* write log Record */
		{EDU_EXEC, edp_ed_write_rec_v2, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_v2_t *)0)->ckpt_rec.write_log, 0, NULL},

		/* Open stream */
		{EDU_EXEC, edp_ed_open_stream_rec_v2, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_v2_t *)0)->ckpt_rec.stream_open, 0, NULL},

		/* Close stream */
		{EDU_EXEC, edp_ed_close_stream_rec_v2, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_v2_t *)0)->ckpt_rec.stream_close, 0, NULL},

		/* Agent dest */
		{EDU_EXEC, edp_ed_agent_down_rec_v2, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_v2_t *)0)->ckpt_rec.stream_cfg, 0, NULL},


		/* Cfg stream */
		{EDU_EXEC, edp_ed_cfg_stream_rec_v2, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_v2_t *)0)->ckpt_rec.stream_cfg, 0, NULL},

		/* Lgs cfg */
		{EDU_EXEC, edp_ed_lgs_cfg_rec_v2, 0, 0, EDU_EXIT,
		 (long)&((lgsv_ckpt_msg_v2_t *)0)->ckpt_rec.lgs_cfg, 0, NULL},

		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if (op == EDP_OP_TYPE_ENC) {
		ckpt_msg_ptr = (lgsv_ckpt_msg_v2_t *)ptr;
	} else if (op == EDP_OP_TYPE_DEC) {
		ckpt_msg_dec_ptr = (lgsv_ckpt_msg_v2_t **)ptr;
		if (*ckpt_msg_dec_ptr == NULL) {
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*ckpt_msg_dec_ptr, '\0', sizeof(lgsv_ckpt_msg_v2_t));
		ckpt_msg_ptr = *ckpt_msg_dec_ptr;
	} else {
		ckpt_msg_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_msg_ed_rules, ckpt_msg_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;

}	/* End edu_enc_dec_ckpt_msg() */
