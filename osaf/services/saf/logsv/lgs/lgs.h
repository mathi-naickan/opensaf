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
 * Author(s): Ericsson AB
 *
 */

#ifndef __LGS_H
#define __LGS_H

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <ncsgl_defs.h>
#include <ncs_lib.h>
#include <mds_papi.h>
#include <ncs_mda_pvt.h>
#include <mbcsv_papi.h>
#include <ncs_edu_pub.h>
#include <ncs_util.h>
#include <logtrace.h>
#include <limits.h>
#include <saAis.h>

/* LGS files */
#include "lgsv_defs.h"
#include "lgs_cb.h"
#include "lgsv_msg.h"
#include "lgs_evt.h"
#include "lgs_mbcsv.h"
#include "lgs_util.h"
#include "lgs_stream.h"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* Hard Coded in test!!! FIX */
#define ALARM_LOG_STREAM_ID 0
#define NOTIFICATION_LOG_STREAM_ID 1
#define SYSTEM_LOG_STREAM_ID 2
#define APPLICATION_LOG_STREAM_ID 3
#define LGS_FORMAT_EXP_MAX 256
#define DEFAULT_FIXED_LOG_RECORD_SIZE 150
#define DEFAULT_ALM_ACTION "ROTATE"
#define DEFAULT_ALM_ACTION_ROTATE 3
#define DEFAULT_MAX_LOG_FILE_SIZE 500000
#define DEFAULT_MAX_FILES_ROTATED 4
#define DEFAULT_APP_MAX_LOG_FILE_SIZE 800000
#define DEFAULT_APP_NUM_ROTATIONS 4

/* IPC priorities used when adding messages to the LOG mailbox */
#define LGS_IPC_PRIO_CTRL_MSGS NCS_IPC_PRIORITY_VERY_HIGH
#define LGS_IPC_PRIO_ALARM_STREAM NCS_IPC_PRIORITY_HIGH
#define LGS_IPC_PRIO_SYS_STREAM NCS_IPC_PRIORITY_NORMAL
#define LGS_IPC_PRIO_APP_STREAM NCS_IPC_PRIORITY_LOW

/* The name of log service config object */    
#define LGS_IMM_LOG_CONFIGURATION	"logConfig=1,safApp=safLogService"

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* Used with function lgs_imm_logconf_get(...) */
typedef enum {
	LGS_IMM_LOG_ROOT_DIRECTORY,
	LGS_IMM_LOG_MAX_LOGRECSIZE,
	LGS_IMM_LOG_STREAM_SYSTEM_HIGH_LIMIT,
	LGS_IMM_LOG_STREAM_SYSTEM_LOW_LIMIT,
	LGS_IMM_LOG_STREAM_APP_HIGH_LIMIT,
	LGS_IMM_LOG_STREAM_APP_LOW_LIMIT,
	LGS_IMM_LOG_MAX_APPLICATION_STREAMS,

	LGS_IMM_LOG_NUMBER_OF_PARAMS,
	LGS_IMM_LOG_OPENSAFLOGCONFIG_CLASS_EXIST,

	LGS_IMM_LOG_NUMEND
} lgs_logconfGet_t;

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */
extern lgs_cb_t *lgs_cb;
extern SYSF_MBX lgs_mbx;
extern uint32_t mbox_high[NCS_IPC_PRIORITY_MAX];
extern uint32_t mbox_msgs[NCS_IPC_PRIORITY_MAX];
extern bool mbox_full[NCS_IPC_PRIORITY_MAX];
extern uint32_t mbox_low[NCS_IPC_PRIORITY_MAX];

extern uint32_t lgs_amf_init(lgs_cb_t *);
extern uint32_t lgs_mds_init(lgs_cb_t *cb);
extern uint32_t lgs_mds_finalize(lgs_cb_t *cb);
extern uint32_t lgs_mds_change_role(lgs_cb_t *cb);
extern uint32_t lgs_mds_msg_send(lgs_cb_t *cb,
			      lgsv_msg_t *msg,
			      MDS_DEST *dest, MDS_SYNC_SND_CTXT *mds_ctxt, MDS_SEND_PRIORITY_TYPE prio);

extern void lgs_giveup_imm_applier(lgs_cb_t *cb);
extern SaAisErrorT lgs_become_imm_applier(lgs_cb_t *cb);
extern SaAisErrorT lgs_imm_activate(lgs_cb_t *cb);
extern void lgs_imm_impl_set(lgs_cb_t *cb);
extern SaAisErrorT lgs_imm_init(lgs_cb_t *cb);
extern const void *lgs_imm_logconf_get(lgs_logconfGet_t param, bool *noteflag);

#endif   /* ifndef __LGS_H */
