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

#ifndef IMM_IMMD_IMMD_RED_H_
#define IMM_IMMD_IMMD_RED_H_

typedef enum immd_mbcsv_msg_type {
	IMMD_A2S_MSG_BASE = 1,
	IMMD_A2S_MSG_FEVS = IMMD_A2S_MSG_BASE,
	IMMD_A2S_MSG_ADMINIT,
	IMMD_A2S_MSG_IMPLSET,
	IMMD_A2S_MSG_CCBINIT,
	IMMD_A2S_MSG_INTRO_RSP,
	IMMD_A2S_MSG_SYNC_START,
	IMMD_A2S_MSG_DUMP_OK,
	IMMD_A2S_MSG_RESET,
	IMMD_A2S_MSG_SYNC_ABORT,
	IMMD_A2S_MSG_MAX_EVT
} IMMD_MBCSV_MSG_TYPE;

typedef struct immd_mbcsv_msg {
	IMMD_MBCSV_MSG_TYPE type;
	union {
		/* Messages for replication to IMMD stby  */
		IMMSV_FEVS fevsReq;
		uint32_t count;
		IMMSV_D2ND_CONTROL ctrl;
	} info;
} IMMD_MBCSV_MSG;

#endif  // IMM_IMMD_IMMD_RED_H_
