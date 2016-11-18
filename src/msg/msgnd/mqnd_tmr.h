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

#ifndef MSG_MSGND_MQND_TMR_H_
#define MSG_MSGND_MQND_TMR_H_
#define MQND_MQA_EXPIRY_TIMER 10
#define MQND_QTRANSFER_REQ_TIMER 999	/*  999 Ten milli seconds = 9.99 sec because timer at mqa is 10 sec */
typedef enum mqndq_tmr_type {
	MQND_TMR_TYPE_RETENTION = 1,
	MQND_TMR_TYPE_MQA_EXPIRY,
	MQND_TMR_TYPE_NODE1_QTRANSFER,
	MQND_TMR_TYPE_NODE2_QTRANSFER,
	MQND_TMR_TYPE_MAX = MQND_TMR_TYPE_MQA_EXPIRY
} MQND_TMR_TYPE;

typedef struct mqnd_tmr {
	MQND_TMR_TYPE type;
	tmr_t tmr_id;
	SaMsgQueueHandleT qhdl;
	uint32_t uarg;
	bool is_active;
} MQND_TMR;

uint32_t mqnd_tmr_start(MQND_TMR *tmr, SaTimeT duration);
void mqnd_tmr_stop(MQND_TMR *tmr);

#endif  // MSG_MSGND_MQND_TMR_H_
