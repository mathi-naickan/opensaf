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

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:

  This include file contains the event definitions for GLA. That will be used 
  by GLND for communication with GLA.
  
******************************************************************************
*/

#ifndef LCK_AGENT_GLA_EVT_H_
#define LCK_AGENT_GLA_EVT_H_

typedef enum glsv_gla_evt_type {
	GLSV_GLA_CALLBK_EVT,
	GLSV_GLA_API_RESP_EVT,
	GLSV_GLA_EVT_MAX
} GLSV_GLA_EVT_TYPE;

typedef enum glsv_gla_api_resp_evt_type_tag {
	GLSV_GLA_LOCK_INITIALIZE = 1,
	GLSV_GLA_LOCK_FINALIZE,
	GLSV_GLA_LOCK_RES_OPEN,
	GLSV_GLA_LOCK_RES_CLOSE,
	GLSV_GLA_LOCK_SYNC_LOCK,
	GLSV_GLA_LOCK_SYNC_UNLOCK,
	GLSV_GLA_NODE_OPERATIONAL,
	GLSV_GLA_LOCK_PURGE
} GLSV_GLA_API_RESP_EVT_TYPE;

typedef struct glsv_gla_evt_lock_initialise_param_tag {
	uint32_t dummy;
} GLSV_GLA_EVT_LOCK_INITIALISE_PARAM;

typedef struct glsv_gla_evt_lock_finalize_param_tag {
	uint32_t dummy;
} GLSV_GLA_EVT_LOCK_FINALIZE_PARAM;

typedef struct glsv_gla_evt_lock_res_open_param_tag {
	SaLckResourceIdT resourceId;
} GLSV_GLA_EVT_LOCK_RES_OPEN_PARAM;

typedef struct glsv_gla_evt_lock_res_close_param_tag {
	uint32_t dummy;
} GLSV_GLA_EVT_LOCK_RES_CLOSE_PARAM;

typedef struct glsv_gla_evt_lock_sync_lock_param_tag {
	SaLckLockIdT lockId;
	SaLckLockStatusT lockStatus;
} GLSV_GLA_EVT_LOCK_SYNC_LOCK_PARAM;

typedef struct glsv_gla_evt_lock_sync_unlock_param_tag {
	uint32_t dummy;
} GLSV_GLA_EVT_LOCK_SYNC_UNLOCK_PARAM;

/* API Resp param definition */
typedef struct glsv_gla_api_resp_info {
	uint32_t prc_id;		/* process id */
	GLSV_GLA_API_RESP_EVT_TYPE type;	/* api type */
	union {
		GLSV_GLA_EVT_LOCK_INITIALISE_PARAM lck_init;
		GLSV_GLA_EVT_LOCK_FINALIZE_PARAM lck_fin;
		GLSV_GLA_EVT_LOCK_RES_OPEN_PARAM res_open;
		GLSV_GLA_EVT_LOCK_RES_CLOSE_PARAM res_close;
		GLSV_GLA_EVT_LOCK_SYNC_LOCK_PARAM sync_lock;
		GLSV_GLA_EVT_LOCK_SYNC_UNLOCK_PARAM sync_unlock;
	} param;
} GLSV_GLA_API_RESP_INFO;

/* For GLND to GLA communication */
typedef struct glsv_gla_evt {
	SaLckHandleT handle;
	SaAisErrorT error;
	GLSV_GLA_EVT_TYPE type;	/* message type */
	union {
		GLSV_GLA_CALLBACK_INFO gla_clbk_info;	/* callbk info */
		GLSV_GLA_API_RESP_INFO gla_resp_info;	/* api response info */
	} info;
} GLSV_GLA_EVT;

#endif  // LCK_AGENT_GLA_EVT_H_
