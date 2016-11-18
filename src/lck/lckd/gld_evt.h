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

#ifndef LCK_LCKD_GLD_EVT_H_
#define LCK_LCKD_GLD_EVT_H_

/*****************************************************************************
 * Message Type of GLND 
 *****************************************************************************/
typedef enum glsv_gld_evt_type {
	GLSV_GLD_EVT_BASE,
	GLSV_GLD_EVT_RSC_OPEN = GLSV_GLD_EVT_BASE,
	GLSV_GLD_EVT_RSC_CLOSE,
	GLSV_GLD_EVT_SET_ORPHAN,
	GLSV_GLD_EVT_GLND_DOWN,
	GLSV_GLD_EVT_GLND_OPERATIONAL,
	/*debug events */
	GLSV_GLD_EVT_CB_DUMP,

	/* Timer Timeout Events */
	GLSV_GLD_EVT_REELECTION_TIMEOUT,
	GLSV_GLD_EVT_RESTART_TIMEOUT,
	GLSV_GLD_EVT_QUISCED_STATE,

	GLSV_GLD_EVT_MAX
} GLSV_GLD_EVT_TYPE;

/*****************************************************************************
 * The messages used by GLD
 *****************************************************************************/
typedef struct glsv_rsc_open_info {
	SaNameT rsc_name;
	SaLckResourceIdT rsc_id;
	SaLckResourceOpenFlagsT flag;
} GLSV_RSC_OPEN_INFO;

typedef struct glsv_rsc_details {
	SaLckResourceIdT rsc_id;
	bool orphan;
	SaLckLockModeT lck_mode;
	uint32_t lcl_ref_cnt;
} GLSV_RSC_DETAILS;

typedef struct glsv_gld_glnd_mds_info_tag {
	MDS_DEST mds_dest_id;
} GLSV_GLD_GLND_MDS_INFO;

/* timer event definition */
typedef struct gld_evt_tmr_tag {
	MDS_DEST mdest_id;
	SaLckResourceIdT resource_id;
	uint32_t opq_hdl;
} GLD_EVT_TMR;

/*****************************************************************************
 * GLD msg data structure.
 *****************************************************************************/
typedef struct glsv_gld_evt_tag {
	struct glsv_gld_evt_tag *next;	/* Has to be first member. See hj_ipc_send() */
	NCSCONTEXT gld_cb;	/* Control block pointer                     */
	GLSV_GLD_EVT_TYPE evt_type;
	MDS_DEST fr_dest_id;
	union {
		GLSV_RSC_OPEN_INFO rsc_open_info;
		GLSV_RSC_DETAILS rsc_details;
		GLSV_GLD_GLND_MDS_INFO glnd_mds_info;
		GLD_EVT_TMR tmr;
	} info;
} GLSV_GLD_EVT;

#define GLSV_GLD_EVT_NULL (GLSV_GLD_EVT *)NULL

/* This is the function prototype for event handling */
typedef uint32_t (*GLSV_GLD_EVT_HANDLER) (struct glsv_gld_evt_tag * evt);

void gld_evt_destroy(GLSV_GLD_EVT *evt);
uint32_t gld_process_evt(GLSV_GLD_EVT *evt);

#endif  // LCK_LCKD_GLD_EVT_H_
