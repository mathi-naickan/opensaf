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

#ifndef CKPT_AGENT_CPA_TMR_H_
#define CKPT_AGENT_CPA_TMR_H_

#include "base/ncssysf_tmr.h"

#define CPA_OPEN_ASYNC_WAIT_TIME    1000	/* Milli Seconds */
#define CPA_SYNC_ASYNC_WAIT_TIME    1000	/* Milli Seconds */

typedef enum cpa_tmr_type {
	CPA_TMR_TYPE_CPND_RETENTION = 1,
	CPA_TMR_TYPE_OPEN,
	CPA_TMR_TYPE_SYNC,
	CPA_TMR_TYPE_MAX = CPA_TMR_TYPE_SYNC
} CPA_TMR_TYPE;

typedef struct cpa_tmr {
	CPA_TMR_TYPE type;
	tmr_t tmr_id;
	uint32_t uarg;
	bool is_active;
	union {
		struct {
			SaCkptCheckpointHandleT lcl_ckpt_hdl;
			SaCkptHandleT client_hdl;
			SaInvocationT invocation;
		} ckpt;
		struct {
			uint32_t dummy;
		} cpnd;
	} info;
} CPA_TMR;

uint32_t cpa_tmr_start(CPA_TMR *tmr, SaTimeT duration);
void cpa_tmr_stop(CPA_TMR *tmr);
void cpa_cb_dump(void);
void cpa_timer_expiry(NCSCONTEXT uarg);

#endif  // CKPT_AGENT_CPA_TMR_H_
