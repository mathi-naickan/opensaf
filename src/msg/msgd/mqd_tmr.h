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

#ifndef MSG_MSGD_MQD_TMR_H_
#define MSG_MSGD_MQD_TMR_H_

typedef enum mqdq_tmr_type {
	MQD_TMR_TYPE_RETENTION = 1,
	MQD_ND_TMR_TYPE_EXPIRY,
	MQD_CTRL_EVT_TMR_EXPIRY,
	MQD_TMR_TYPE_MAX = MQD_CTRL_EVT_TMR_EXPIRY
} MQD_TMR_TYPE;

typedef struct mqd_tmr {
	MQD_TMR_TYPE type;
	tmr_t tmr_id;
	NODE_ID nodeid;
	uint32_t uarg;
	bool is_active;
	bool is_expired;
} MQD_TMR;

uint32_t mqd_tmr_start(MQD_TMR *tmr, SaTimeT duration);
void mqd_tmr_stop(MQD_TMR *tmr);
void mqd_timer_expiry(NCSCONTEXT uarg);

#endif  // MSG_MSGD_MQD_TMR_H_
