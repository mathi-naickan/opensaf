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

#ifndef LCK_GLSV_LCK_H_
#define LCK_GLSV_LCK_H_

#include "lck/saf/saLck.h"

typedef struct glsv_lock_req_info_tag {
	SaLckLockIdT lockid;	/* index for identifying the lock */
	SaLckLockIdT lcl_lockid;	/* index for identifying the lock */
	SaLckHandleT handleId;	/* source application requesting lock */
	SaInvocationT invocation;
	SaLckLockModeT lock_type;
	SaTimeT timeout;
	SaLckLockFlagsT lockFlags;
	SaLckLockStatusT lockStatus;
	GLSV_CALL_TYPE call_type;
	MDS_DEST agent_mds_dest;
	SaLckWaiterSignalT waiter_signal;
} GLSV_LOCK_REQ_INFO;

typedef enum {
	GLSV_LOCK_NOT_INITIALISED = 0,
	GLSV_LOCK_REQ_PENDING,
	GLSV_UNLOCK_REQ_PENDING,
	GLSV_LOCK_REQ_GRANTED,
	GLSV_LOCK_REQ_BLOCKED
} GLSV_LOCK_STATUS;

typedef struct glnd_lock_list_info_tag {
	GLSV_LOCK_REQ_INFO lock_info;
	struct glnd_lock_list_info_tag *next;
} GLND_LOCK_LIST_INFO;

#endif  // LCK_GLSV_LCK_H_
