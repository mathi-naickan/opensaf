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

#ifndef LCK_GLSV_DEFS_H_
#define LCK_GLSV_DEFS_H_

/* this needs to be defined */

#define GLSV_LOCK_OPEN_CBK_REG   0x01
#define GLSV_LOCK_GRANT_CBK_REG  0x02
#define GLSV_LOCK_WAITER_CBK_REG 0x04
#define GLSV_LOCK_UNLOCK_CBK_REG 0x08

#define m_GLD_TASK_PRIORITY   (5)
#define m_GLD_STACKSIZE       NCS_STACKSIZE_HUGE

#define m_GLND_TASK_PRIORITY   (5)
#define m_GLND_STACKSIZE      NCS_STACKSIZE_HUGE

#define m_GLSV_CONVERT_SATIME_TEN_MILLI_SEC(t)      (t)/(10000000)	/* 10^7 */

#define m_ASSIGN_LCK_HANDLE_ID(handle_id)       handle_id

/* time out for synchronous blocking calls */
#define GLA_API_RESP_TIME   500	/* 5 secs */

typedef unsigned int GLSV_TIMER_ID;

/* Version Constants */
#define REQUIRED_RELEASECODE    'B'
#define REQUIRED_MAJORVERSION   01
#define REQUIRED_MINORVERSION   01

#define m_GLA_VER_IS_VALID(ver) \
   ((ver->releaseCode == REQUIRED_RELEASECODE) && \
     (ver->majorVersion <= REQUIRED_MAJORVERSION))

#define MSG_FRMT_VER uint32_t
/*** Macro used to get the AMF version used ****/
#define m_GLSV_GET_AMF_VER(amf_ver) amf_ver.releaseCode='B'; amf_ver.majorVersion=0x01; amf_ver.minorVersion=0x01;

/* Enumerators needed by the Global Lock Service */
typedef enum glsv_call_type_tag {
	GLSV_SYNC_CALL = 1,
	GLSV_ASYNC_CALL
} GLSV_CALL_TYPE;

typedef enum {
	GLND_RESOURCE_NOT_INITIALISED = 0,
	GLND_RESOURCE_ACTIVE_MASTER,
	GLND_RESOURCE_ACTIVE_NON_MASTER,
	GLND_RESOURCE_ELECTION_IN_PROGESS,
	GLND_RESOURCE_MASTER_RESTARTED,
	GLND_RESOURCE_MASTER_OPERATIONAL,
	GLND_RESOURCE_ELECTION_COMPLETED
} GLND_RESOURCE_STATUS;

/* typedef enums */
typedef enum {
	GLND_CLIENT_INFO_GET_STATE = 1,
	GLND_RESTART_STATE,
	GLND_OPERATIONAL_STATE,
	GLND_DOWN_STATE
} GLND_NODE_STATUS;

typedef SaUint32T SaLckResourceIdT;

#define GLSV_LOCK_STATUS_RELEASED   100

/* Timeout constants */
#define GLSV_LOCK_DEFAULT_TIMEOUT          10000000000LL
#define GLSV_GLA_TMR_DEFAULT_TIMEOUT       11000000000LL
#define GLSV_CKPT_OPEN_DEFAULT_TIMEOUT     10000000000LL

#define GLND_MAX_RESOURCES_PER_NODE                         1000
#define GLND_RESOURCE_INFO_CKPT_MAX_SECTIONS                GLND_MAX_RESOURCES_PER_NODE
#define GLND_RES_LOCK_INFO_CKPT_MAX_SECTIONS                1000
#define GLND_BACKUP_EVT_CKPT_MAX_SECTIONS                   1000
#define GLND_CKPT_MAX_SECTIONS                              10
#define GLND_CKPT_RETENTION_DURATION                        100000000000LL

/*To increment mds timer value(in milli sec)*/
#define LCK_TIMEOUT_LATENCY			 	    200

#endif  // LCK_GLSV_DEFS_H_
