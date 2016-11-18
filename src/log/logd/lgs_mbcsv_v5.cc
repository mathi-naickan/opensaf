/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2015 The OpenSAF Foundation
 * File:   lgs_mbcsv_v5.c
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

/* NOTE: For version 4 (V4) see logDataGroupname in V3 handling
 *
 * V5 applies to log server configuration check-pointing
 */

#include "lgs_mbcsv_v5.h"

/****************************************************************************
 * Name          : ckpt_proc_lgs_cfg
 *
 * Description   : This function updates log service configuration on
 *                 standby node
 *
 * Arguments     : cb - pointer to LGS  ControlBlock.
 *                 data - pointer to  LGS_CHECKPOINT_DATA.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : The configuration store on standby must always be updated
 *                 when there are changes in log service configuration.
 *                 This is done by calling lgs_cfg_update() function.
 *
 ****************************************************************************/

uint32_t ckpt_proc_lgs_cfg_v5(lgs_cb_t *cb, void *data) {
  lgs_config_chg_t cfg_buffer = {NULL, 0};
  char *next_ptr = NULL;
  char *name_str = NULL;
  char *value_str = NULL;
  char *saved_buf = NULL;
  int rc = 0;

  /* Flag set if any of the mailbox limit values have changed */
  bool mailbox_lim_upd = false;

  if (!lgs_is_peer_v5()) {
    /* Should never enter here */
    LOG_ER("%s: Called when peer is not version 5. "
           "We should never enter here",__FUNCTION__);
    osafassert(0);
  }

  lgsv_ckpt_msg_v5_t *data_v5 = static_cast<lgsv_ckpt_msg_v5_t *>(data);
  lgs_ckpt_lgs_cfg_v5_t *param = &data_v5->ckpt_rec.lgs_cfg;

  /* Act on configuration changes
   */
  /* Initiate cfg data */
  cfg_buffer.ckpt_buffer_ptr = param->buffer;
  cfg_buffer.ckpt_buffer_size = param->buffer_size;
  next_ptr = cfg_buffer.ckpt_buffer_ptr;

  /* Save the content of the buffer since the content is changed
   * by the lgs_cfgupd_buffer_read() function.
   */
  saved_buf = static_cast<char *>(calloc(1,param->buffer_size));
  if (saved_buf == NULL) {
    LOG_ER("%s calloc() error", __FUNCTION__);
    osafassert(0);
  }

  (void) memcpy(saved_buf, param->buffer, param->buffer_size);

  /* Get first parameter */
  next_ptr = lgs_cfgupd_list_read(&name_str, &value_str,
                                  next_ptr,
                                  &cfg_buffer);

  while (next_ptr != NULL) {
    /* Attribute changes that needs more than to be updated in the
     * configuration store are handled here */
    if ((strcmp(name_str, LOG_ROOT_DIRECTORY) == 0) &&
        (lgs_is_split_file_system() == true)){
      const char *new_root_path = value_str;
      const char *old_root_path = static_cast<const char *>(
          lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));
      logRootDirectory_filemove(new_root_path, old_root_path,
                                (time_t *) &param->c_file_close_time_stamp);
    } else if ((strcmp(name_str, LOG_DATA_GROUPNAME) == 0) &&
               (lgs_is_split_file_system() == true)) {
      logDataGroupname_fileown(value_str);
    } else if (!strcmp(name_str, LOG_STREAM_SYSTEM_HIGH_LIMIT)) {
      mailbox_lim_upd = true;
    } else if (!strcmp(name_str, LOG_STREAM_SYSTEM_LOW_LIMIT)) {
      mailbox_lim_upd = true;
    } else if (!strcmp(name_str, LOG_STREAM_APP_HIGH_LIMIT)) {
      mailbox_lim_upd = true;
    } else if (!strcmp(name_str, LOG_STREAM_APP_LOW_LIMIT)) {
      mailbox_lim_upd = true;
    }

    next_ptr = lgs_cfgupd_list_read(&name_str, &value_str,
                                    next_ptr, &cfg_buffer);
  }

  /* Update configuration structure */
  /* NOTE: The saved buffer must be used since the data in the received
   *       buffer is changed by the lgs_cfgupd_buffer_read() function
   */
  cfg_buffer.ckpt_buffer_ptr = saved_buf;
  rc = lgs_cfg_update(&cfg_buffer);
  if (rc == -1) {
    /* Shall not happen */
    LOG_ER("%s lgs_cfg_update Fail", __FUNCTION__);
    osafassert(0);
  }
  free(saved_buf);

  /* Update mailbox limits if any of them has changed
   * Note: Must be done after configuration data store is updated
   */
  if (mailbox_lim_upd == true) {
    TRACE("\tUpdating mailbox limits");
    (void) lgs_configure_mailbox();
  }

  /* Free buffer allocated by the EDU encoder */
  if (param->buffer == NULL) {
    LOG_ER("%s chkpt_buffer_ptr is NULL!", __FUNCTION__);
  }
  lgs_free_edu_mem(param->buffer);

  lgs_trace_config();

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

uint32_t edp_ed_lgs_cfg_rec_v5(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
                               NCSCONTEXT ptr, uint32_t *ptr_data_len,
                               EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err) {
  uint32_t rc = NCSCC_RC_SUCCESS;
  void *ckpt_lgs_cfg_msg_ptr = NULL;
  lgs_ckpt_lgs_cfg_v5_t **ckpt_lgs_cfg_msg_dec_ptr_v5;

  /* Configuration data to check-point is stored in a variable length
   * buffer. Note the construction consisting of the two first EDU_EXEC
   * and the EDU_EXEC_EXT rules.
   * The c_file_close_time_stamp variable is not part of the configuration
   * data and is handled separately.
   */
  EDU_INST_SET ckpt_lgs_cfg_rec_ed_rules[] = {
    {EDU_START, edp_ed_lgs_cfg_rec_v5, 0, 0, 0, sizeof(lgs_ckpt_lgs_cfg_v5_t), 0, NULL},
    {EDU_EXEC, ncs_edp_uns64, 0, 0, 0,
     (long)&((lgs_ckpt_lgs_cfg_v5_t *)0)->buffer_size, 0, NULL},
    {EDU_EXEC, ncs_edp_uns8, EDQ_VAR_LEN_DATA, ncs_edp_uns64, 0,
     (long)&((lgs_ckpt_lgs_cfg_v5_t *)0)->buffer,
     (long)&((lgs_ckpt_lgs_cfg_v5_t *)0)->buffer_size, NULL},
    {EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_LGS /* Svc-ID */ , NULL, 0, 0 /* Sub-ID */ , 0, NULL},
    {EDU_EXEC, ncs_edp_uns64, 0, 0, 0,
     (long)&((lgs_ckpt_lgs_cfg_v5_t *)0)->c_file_close_time_stamp, 0, NULL},
    {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
  };

  if (op == EDP_OP_TYPE_ENC) {
    ckpt_lgs_cfg_msg_ptr = static_cast<lgs_ckpt_lgs_cfg_v5_t *>(ptr);
  } else if (op == EDP_OP_TYPE_DEC) {
    ckpt_lgs_cfg_msg_dec_ptr_v5 = static_cast<lgs_ckpt_lgs_cfg_v5_t **>(ptr);
    if (*ckpt_lgs_cfg_msg_dec_ptr_v5 == NULL) {
      *o_err = EDU_ERR_MEM_FAIL;
      return NCSCC_RC_FAILURE;
    }
    memset(*ckpt_lgs_cfg_msg_dec_ptr_v5, '\0', sizeof(lgs_ckpt_lgs_cfg_v5_t));
    ckpt_lgs_cfg_msg_ptr = *ckpt_lgs_cfg_msg_dec_ptr_v5;
  } else {
    ckpt_lgs_cfg_msg_ptr = ptr;
  }

  rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_lgs_cfg_rec_ed_rules,
                           ckpt_lgs_cfg_msg_ptr, ptr_data_len, buf_env, op, o_err);

  return rc;
}

/****************************************************************************
 * Name          : edp_ed_ckpt_msg_v5
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

uint32_t edp_ed_ckpt_msg_v5(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
                            NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,
                            EDP_OP_TYPE op, EDU_ERR *o_err) {
  uint32_t rc = NCSCC_RC_SUCCESS;
  lgsv_ckpt_msg_v5_t *ckpt_msg_ptr = NULL, **ckpt_msg_dec_ptr;

  EDU_INST_SET ckpt_msg_ed_rules[] = {
    {EDU_START, edp_ed_ckpt_msg_v5, 0, 0, 0, sizeof(lgsv_ckpt_msg_v5_t), 0, NULL},
    {EDU_EXEC, edp_ed_header_rec, 0, 0, 0, (long)&((lgsv_ckpt_msg_v5_t *)0)->header, 0, NULL},

    {EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((lgsv_ckpt_msg_v5_t *)0)->header, 0,
     (EDU_EXEC_RTINE)ckpt_msg_test_type},

    /* Reg Record */
    {EDU_EXEC, edp_ed_reg_rec, 0, 0, static_cast<int>(EDU_EXIT),
     (long)&((lgsv_ckpt_msg_v5_t *)0)->ckpt_rec.initialize_client, 0, NULL},

    /* Finalize record */
    {EDU_EXEC, edp_ed_finalize_rec_v2, 0, 0, static_cast<int>(EDU_EXIT),
     (long)&((lgsv_ckpt_msg_v5_t *)0)->ckpt_rec.finalize_client, 0, NULL},

    /* write log Record */
    {EDU_EXEC, edp_ed_write_rec_v2, 0, 0, static_cast<int>(EDU_EXIT),
     (long)&((lgsv_ckpt_msg_v5_t *)0)->ckpt_rec.write_log, 0, NULL},

    /* Open stream */
    {EDU_EXEC, edp_ed_open_stream_rec, 0, 0, static_cast<int>(EDU_EXIT),
     (long)&((lgsv_ckpt_msg_v5_t *)0)->ckpt_rec.stream_open, 0, NULL},

    /* Close stream */
    {EDU_EXEC, edp_ed_close_stream_rec_v2, 0, 0, static_cast<int>(EDU_EXIT),
     (long)&((lgsv_ckpt_msg_v5_t *)0)->ckpt_rec.stream_close, 0, NULL},

    /* Agent dest */
    {EDU_EXEC, edp_ed_agent_down_rec_v2, 0, 0, static_cast<int>(EDU_EXIT),
     (long)&((lgsv_ckpt_msg_v5_t *)0)->ckpt_rec.stream_cfg, 0, NULL},

    /* Cfg stream */
    {EDU_EXEC, edp_ed_cfg_stream_rec_v2, 0, 0, static_cast<int>(EDU_EXIT),
     (long)&((lgsv_ckpt_msg_v5_t *)0)->ckpt_rec.stream_cfg, 0, NULL},

    /* Lgs cfg */
    {EDU_EXEC, edp_ed_lgs_cfg_rec_v5, 0, 0, static_cast<int>(EDU_EXIT),
     (long)&((lgsv_ckpt_msg_v5_t *)0)->ckpt_rec.lgs_cfg, 0, NULL},

    {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
  };

  if (op == EDP_OP_TYPE_ENC) {
    ckpt_msg_ptr = static_cast<lgsv_ckpt_msg_v5_t *>(ptr);
  } else if (op == EDP_OP_TYPE_DEC) {
    ckpt_msg_dec_ptr = static_cast<lgsv_ckpt_msg_v5_t **>(ptr);
    if (*ckpt_msg_dec_ptr == NULL) {
      *o_err = EDU_ERR_MEM_FAIL;
      return NCSCC_RC_FAILURE;
    }
    memset(*ckpt_msg_dec_ptr, '\0', sizeof(lgsv_ckpt_msg_v5_t));
    ckpt_msg_ptr = *ckpt_msg_dec_ptr;
  } else {
    ckpt_msg_ptr = static_cast<lgsv_ckpt_msg_v5_t *>(ptr);
  }

  rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, ckpt_msg_ed_rules,
                           ckpt_msg_ptr, ptr_data_len, buf_env, op, o_err);

  return rc;

}       /* End edu_enc_dec_ckpt_msg() */
