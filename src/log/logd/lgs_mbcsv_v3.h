/*      -*- OpenSAF  -*-
 * File:   lgs_mbcsv_v3.h
 *
 * (C) Copyright 2015 The OpenSAF Foundation
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

#ifndef LOG_LOGD_LGS_MBCSV_V3_H_
#define LOG_LOGD_LGS_MBCSV_V3_H_

#include "log/logd/lgs.h"
#include "lgs_mbcsv.h"
#include "lgs_mbcsv_v2.h"

/* Structures for Checkpoint data ver 3 (to be replicated at the standby) */

typedef struct {
  /* Attributes that can be updated */
  char *logRootDirectory;
  char *logDataGroupname;
  uint64_t c_file_close_time_stamp; /* Time in sec for file rename */
} lgs_ckpt_lgs_cfg_v3_t;

typedef struct {
  lgsv_ckpt_header_t header;
  union {
    lgs_ckpt_initialize_msg_t initialize_client;
    lgs_ckpt_finalize_msg_v2_t finalize_client;
    lgs_ckpt_write_log_v2_t write_log;
    lgs_ckpt_agent_down_v2_t agent_down;
    lgs_ckpt_stream_open_t stream_open;
    lgs_ckpt_stream_close_v2_t stream_close;
    lgs_ckpt_stream_cfg_v2_t stream_cfg;
    lgs_ckpt_lgs_cfg_v3_t lgs_cfg;
  } ckpt_rec;
} lgsv_ckpt_msg_v3_t;

uint32_t ckpt_proc_lgs_cfg_v3(lgs_cb_t *cb, void *data);
uint32_t edp_ed_lgs_cfg_rec_v3(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
                               NCSCONTEXT ptr, uint32_t *ptr_data_len,
                               EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t edp_ed_ckpt_msg_v3(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
                            NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

#endif  // LOG_LOGD_LGS_MBCSV_V3_H_
