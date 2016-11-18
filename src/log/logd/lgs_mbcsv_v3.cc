/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2015 The OpenSAF Foundation
 * File:   lgs_mbcsv_v3.c
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
 * version 3 check-pointing. See also lgs_mbcsv_v3.h
 */

#include "lgs_mbcsv_v3.h"
#include "lgs_mbcsv_v2.h"
#include "lgs_mbcsv.h"

/****************************************************************************
 * Name          : ckpt_proc_lgs_cfg
 *
 * Description   : This function configures updates log service configuration.
 *
 * Arguments     : cb - pointer to LGS  ControlBlock.
 *                 data - pointer to  LGS_CHECKPOINT_DATA.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

uint32_t ckpt_proc_lgs_cfg_v3(lgs_cb_t *cb, void *data) {
  TRACE_ENTER();
  const char *logsv_root_dir = static_cast<const char *>(lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));
  const char *logsv_data_groupname = static_cast<const char *>(lgs_cfg_get(LGS_IMM_DATA_GROUPNAME));

  if (!lgs_is_peer_v4()) {
    /* Should never enter here */
    LOG_ER("%s: Called when peer is not version 4. We should never enter here",__FUNCTION__);
    osafassert(0);
  }

  lgsv_ckpt_msg_v3_t *data_v3 = static_cast<lgsv_ckpt_msg_v3_t *>(data);
  lgs_ckpt_lgs_cfg_v3_t *param = &data_v3->ckpt_rec.lgs_cfg;

  if (strcmp(param->logRootDirectory, logsv_root_dir) != 0) {
    /* Root directory changed */
    TRACE("Setting new root directory on standby %s",param->logRootDirectory);
    if (lgs_is_split_file_system()) {
      /* Move log files on standby also */
      const char *new_root_path = param->logRootDirectory;
      const char *old_root_path = static_cast<const char *>(lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));
      /* Update configuration with new root path */
      lgs_rootpathconf_set(param->logRootDirectory);
      logRootDirectory_filemove(new_root_path, old_root_path,
                                (time_t *) &param->c_file_close_time_stamp);
    } else {
      /* Save new directory */
      lgs_rootpathconf_set(param->logRootDirectory);
    }
  }

  if (! param->logDataGroupname) {
    /* Attribute value deletion */
    if (strcmp(logsv_data_groupname, "") != 0) {
      TRACE("Deleting data group on standby");
      lgs_groupnameconf_set("");
    }
  } else if (strcmp(param->logDataGroupname, logsv_data_groupname) != 0) {
    /* Data group changed */
    TRACE("Setting new data group on standby %s",param->logDataGroupname);
    if (lgs_is_split_file_system()) {
      /* Reown log files on standby also */
      logDataGroupname_fileown(param->logDataGroupname);
    } else {
      /* Save new group */
      lgs_groupnameconf_set(param->logDataGroupname);
    }
  }

  /* Free strings allocated by the EDU encoder */
  lgs_free_edu_mem(param->logRootDirectory);
  lgs_free_edu_mem(param->logDataGroupname);

  TRACE_LEAVE();
  return NCSCC_RC_SUCCESS;
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

uint32_t edp_ed_lgs_cfg_rec_v3(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
                               NCSCONTEXT ptr, uint32_t *ptr_data_len,
                               EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err) {
  uint32_t rc = NCSCC_RC_SUCCESS;
  void *ckpt_lgs_cfg_msg_ptr = NULL;
  lgs_ckpt_lgs_cfg_v3_t **ckpt_lgs_cfg_msg_dec_ptr_v3;

  EDU_INST_SET ckpt_lgs_cfg_rec_ed_rules[] = {
    {EDU_START, edp_ed_lgs_cfg_rec_v3, 0, 0, 0, sizeof(lgs_ckpt_lgs_cfg_v3_t), 0, NULL},
    {EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_lgs_cfg_v3_t *)0)->logRootDirectory, 0, NULL},
    {EDU_EXEC, ncs_edp_string, 0, 0, 0, (long)&((lgs_ckpt_lgs_cfg_v3_t *)0)->logDataGroupname, 0, NULL},
    {EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((lgs_ckpt_lgs_cfg_v3_t *)0)->c_file_close_time_stamp, 0, NULL},
    {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
  };
  if (op == EDP_OP_TYPE_ENC) {
    ckpt_lgs_cfg_msg_ptr = static_cast<lgs_ckpt_lgs_cfg_v3_t *>(ptr);
  } else if (op == EDP_OP_TYPE_DEC) {
    ckpt_lgs_cfg_msg_dec_ptr_v3 = static_cast<lgs_ckpt_lgs_cfg_v3_t **>(ptr);
    if (*ckpt_lgs_cfg_msg_dec_ptr_v3 == NULL) {
      *o_err = EDU_ERR_MEM_FAIL;
      return NCSCC_RC_FAILURE;
    }
    memset(*ckpt_lgs_cfg_msg_dec_ptr_v3, '\0', sizeof(lgs_ckpt_lgs_cfg_v3_t));
    ckpt_lgs_cfg_msg_ptr = *ckpt_lgs_cfg_msg_dec_ptr_v3;
  } else {
    ckpt_lgs_cfg_msg_ptr = ptr;
  }

  rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn,
                           ckpt_lgs_cfg_rec_ed_rules,
                           ckpt_lgs_cfg_msg_ptr,
                           ptr_data_len,
                           buf_env, op, o_err);
  return rc;
}

/****************************************************************************
 * Name          : edp_ed_ckpt_msg_v3
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

uint32_t edp_ed_ckpt_msg_v3(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
                            NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,
                            EDP_OP_TYPE op, EDU_ERR *o_err) {
  uint32_t rc = NCSCC_RC_SUCCESS;
  lgsv_ckpt_msg_v3_t *ckpt_msg_ptr = NULL, **ckpt_msg_dec_ptr;

  EDU_INST_SET ckpt_msg_ed_rules[] = {
    {EDU_START, edp_ed_ckpt_msg_v3, 0, 0, 0, sizeof(lgsv_ckpt_msg_v3_t), 0, NULL},
    {EDU_EXEC, edp_ed_header_rec, 0, 0, 0, (long)&((lgsv_ckpt_msg_v3_t *)0)->header, 0, NULL},

    {EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((lgsv_ckpt_msg_v3_t *)0)->header, 0,
     (EDU_EXEC_RTINE)ckpt_msg_test_type},

    /* Reg Record */
    {EDU_EXEC, edp_ed_reg_rec, 0, 0, static_cast<int>(EDU_EXIT),
     (long)&((lgsv_ckpt_msg_v3_t *)0)->ckpt_rec.initialize_client, 0, NULL},

    /* Finalize record */
    {EDU_EXEC, edp_ed_finalize_rec_v2, 0, 0, static_cast<int>(EDU_EXIT),
     (long)&((lgsv_ckpt_msg_v3_t *)0)->ckpt_rec.finalize_client, 0, NULL},

    /* write log Record */
    {EDU_EXEC, edp_ed_write_rec_v2, 0, 0, static_cast<int>(EDU_EXIT),
     (long)&((lgsv_ckpt_msg_v3_t *)0)->ckpt_rec.write_log, 0, NULL},

    /* Open stream */
    {EDU_EXEC, edp_ed_open_stream_rec, 0, 0, static_cast<int>(EDU_EXIT),
     (long)&((lgsv_ckpt_msg_v3_t *)0)->ckpt_rec.stream_open, 0, NULL},

    /* Close stream */
    {EDU_EXEC, edp_ed_close_stream_rec_v2, 0, 0, static_cast<int>(EDU_EXIT),
     (long)&((lgsv_ckpt_msg_v3_t *)0)->ckpt_rec.stream_close, 0, NULL},

    /* Agent dest */
    {EDU_EXEC, edp_ed_agent_down_rec_v2, 0, 0, static_cast<int>(EDU_EXIT),
     (long)&((lgsv_ckpt_msg_v3_t *)0)->ckpt_rec.stream_cfg, 0, NULL},


    /* Cfg stream */
    {EDU_EXEC, edp_ed_cfg_stream_rec_v2, 0, 0, static_cast<int>(EDU_EXIT),
     (long)&((lgsv_ckpt_msg_v3_t *)0)->ckpt_rec.stream_cfg, 0, NULL},

    /* Lgs cfg */
    {EDU_EXEC, edp_ed_lgs_cfg_rec_v3, 0, 0, static_cast<int>(EDU_EXIT),
     (long)&((lgsv_ckpt_msg_v3_t *)0)->ckpt_rec.lgs_cfg, 0, NULL},

    {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
  };

  if (op == EDP_OP_TYPE_ENC) {
    ckpt_msg_ptr = static_cast<lgsv_ckpt_msg_v3_t *>(ptr);
  } else if (op == EDP_OP_TYPE_DEC) {
    ckpt_msg_dec_ptr = static_cast<lgsv_ckpt_msg_v3_t **>(ptr);
    if (*ckpt_msg_dec_ptr == NULL) {
      *o_err = EDU_ERR_MEM_FAIL;
      return NCSCC_RC_FAILURE;
    }
    memset(*ckpt_msg_dec_ptr, '\0', sizeof(lgsv_ckpt_msg_v3_t));
    ckpt_msg_ptr = *ckpt_msg_dec_ptr;
  } else {
    ckpt_msg_ptr = static_cast<lgsv_ckpt_msg_v3_t *>(ptr);
  }

  rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn,
                           ckpt_msg_ed_rules,
                           ckpt_msg_ptr,
                           ptr_data_len,
                           buf_env, op, o_err);
  return rc;
}       /* End edu_enc_dec_ckpt_msg() */
