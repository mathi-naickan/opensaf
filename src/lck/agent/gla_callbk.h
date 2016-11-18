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

#ifndef LCK_AGENT_GLA_CALLBK_H_
#define LCK_AGENT_GLA_CALLBK_H_

typedef enum gla_callbk_type_tag {
	GLSV_LOCK_RES_OPEN_CBK = 1,
	GLSV_LOCK_GRANT_CBK,
	GLSV_LOCK_WAITER_CBK,
	GLSV_LOCK_UNLOCK_CBK
} GLA_CALLBK_TYPE;

typedef struct glsv_lock_res_open_param_tag {
	SaInvocationT invocation;
	SaLckResourceIdT resourceId;
	SaAisErrorT error;
} GLSV_LOCK_RES_OPEN_PARAM;

typedef struct glsv_lock_grant_param_tag {
	SaInvocationT invocation;
	SaLckResourceIdT resourceId;
	SaLckLockIdT lockId;
	SaLckLockIdT lcl_lockId;
	SaLckLockModeT lockMode;
	SaLckLockStatusT lockStatus;
	SaAisErrorT error;
} GLSV_LOCK_GRANT_PARAM;

typedef struct glsv_lock_waiter_param_tag {
	SaInvocationT invocation;
	SaLckResourceIdT resourceId;
	SaLckLockIdT lockId;
	SaLckLockIdT lcl_lockId;
	SaLckLockModeT modeHeld;
	SaLckLockModeT modeRequested;
	SaLckWaiterSignalT wait_signal;
} GLSV_LOCK_WAITER_PARAM;

typedef struct glsv_lock_unlock_param_tag {
	SaInvocationT invocation;
	SaLckResourceIdT resourceId;
	SaLckLockIdT lockId;
	SaLckLockStatusT lockStatus;
	SaAisErrorT error;
} GLSV_LOCK_UNLOCK_PARAM;

typedef struct glsv_gla_callback_info_tag {
	struct glsv_gla_callback_info_tag *next;	/* for the mailbox manipulation */
	GLA_CALLBK_TYPE callback_type;
	SaLckResourceIdT resourceId;
	union {
		GLSV_LOCK_RES_OPEN_PARAM res_open;
		GLSV_LOCK_GRANT_PARAM lck_grant;
		GLSV_LOCK_WAITER_PARAM lck_wait;
		GLSV_LOCK_UNLOCK_PARAM unlock;
	} params;
} GLSV_GLA_CALLBACK_INFO;

#endif  // LCK_AGENT_GLA_CALLBK_H_
