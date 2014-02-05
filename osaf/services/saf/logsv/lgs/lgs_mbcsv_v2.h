/*      -*- OpenSAF  -*-
 * File:   lgs_mbcsv_v2.h
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

#ifndef LGS_MBCSV_V2_H
#define LGS_MBCSV_V2_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "lgs.h"
#include "lgs_mbcsv.h"

/* Structures for Checkpoint data ver 2 (to be replicated at the standby) */

/* Finalize checkpoint record, used in cold/async checkpoint updates */
typedef struct {
	uint32_t client_id;	/* Client Id at Active */
	uint64_t c_file_close_time_stamp; /* Time in sec for file rename */
} lgs_ckpt_finalize_msg_v2_t;

typedef struct {
	uint32_t streamId;
	uint32_t recordId;
	uint32_t curFileSize;
	char *logFileCurrent;
	char *logRecord;
	uint64_t c_file_close_time_stamp; /* Time in sec for file rename on Active */
} lgs_ckpt_write_log_v2_t;

typedef struct {
	uint32_t streamId;
	uint32_t clientId;
	uint64_t c_file_close_time_stamp; /* Time in sec for file rename on Active */
} lgs_ckpt_stream_close_v2_t;

typedef struct {
	char *name;
	char *fileName;
	char *pathName;
	SaUint64T maxLogFileSize;
	SaUint32T fixedLogRecordSize;
	SaBoolT haProperty;	/* app log stream only */
	SaLogFileFullActionT logFullAction;
	SaUint32T logFullHaltThreshold;	/* !app log stream */
	SaUint32T maxFilesRotated;
	char *logFileFormat;
	SaUint32T severityFilter;
	char *logFileCurrent;
	uint64_t c_file_close_time_stamp; /* Time in sec for file rename on Active */
} lgs_ckpt_stream_cfg_v2_t;

typedef struct {
	/* Only attribute that can be updated */
	char *logRootDirectory;
	uint64_t c_file_close_time_stamp; /* Time in sec for file rename */
} lgs_ckpt_lgs_cfg_v2_t;

typedef struct {
	MDS_DEST agent_dest; /* uint64_t */
	uint64_t c_file_close_time_stamp; /* Time in sec for file rename (int64_t) */
} lgs_ckpt_agent_down_v2_t;

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
		lgs_ckpt_lgs_cfg_v2_t lgs_cfg;
	} ckpt_rec;
} lgsv_ckpt_msg_v2_t;

/* Not used in check pointing ver 1 */
uint32_t ckpt_proc_lgs_cfg_v2(lgs_cb_t *cb, void *data);

/* Variant for check pointing ver 2 */
uint32_t edp_ed_write_rec_v2(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			      NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,
		          EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t edp_ed_close_stream_rec_v2(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				     NCSCONTEXT ptr, uint32_t *ptr_data_len,
				     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t edp_ed_finalize_rec_v2(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				 NCSCONTEXT ptr, uint32_t *ptr_data_len,
				 EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t edp_ed_cfg_stream_rec_v2(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uint32_t *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t edp_ed_lgs_cfg_rec_v2(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uint32_t *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);
uint32_t edp_ed_agent_down_rec_v2(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
				   NCSCONTEXT ptr, uint32_t *ptr_data_len,
				   EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

uint32_t ckpt_decode_cold_sync_v2(lgs_cb_t *cb, NCS_MBCSV_CB_ARG *cbk_arg);

uint32_t edp_ed_ckpt_msg_v2(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			     NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE op, EDU_ERR *o_err);

#ifdef	__cplusplus
}
#endif

#endif	/* LGS_MBCSV_V2_H */

