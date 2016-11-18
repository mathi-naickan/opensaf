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

  GLND communication related definitions.
  
******************************************************************************
*/

#ifndef LCK_LCKND_GLND_MDS_H_
#define LCK_LCKND_GLND_MDS_H_

/*****************************************************************************/

#define SVC_SUBPART_VER uns32
#define GLND_PVT_SUBPART_VERSION 1

/********************Service Sub part Versions*********************************/
/* GLND - GLA */
#define GLND_WRT_GLA_SUBPART_VER_AT_MIN_MSG_FMT 1
#define GLND_WRT_GLA_SUBPART_VER_AT_MAX_MSG_FMT 1
#define GLND_WRT_GLA_SUBPART_VER_RANGE \
            (GLND_WRT_GLA_SUBPART_VER_AT_MAX_MSG_FMT - \
        GLND_WRT_GLA_SUBPART_VER_AT_MIN_MSG_FMT + 1)

/* GLND - GLND */
#define GLND_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT 1
#define GLND_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT 1
#define GLND_WRT_GLND_SUBPART_VER_RANGE \
            (GLND_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT - \
        GLND_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT + 1)

/* GLND - GLD */
#define GLND_WRT_GLD_SUBPART_VER_AT_MIN_MSG_FMT 1
#define GLND_WRT_GLD_SUBPART_VER_AT_MAX_MSG_FMT 1
#define GLND_WRT_GLD_SUBPART_VER_RANGE \
            (GLND_WRT_GLD_SUBPART_VER_AT_MAX_MSG_FMT - \
        GLND_WRT_GLD_SUBPART_VER_AT_MIN_MSG_FMT + 1)
/********************************************************************/
/*** Extern function declarations ***/

uint32_t glnd_mds_register(struct glnd_cb_tag *cb);

void glnd_mds_unregister(struct glnd_cb_tag *cb);

uint32_t glnd_mds_msg_send_gla(struct glnd_cb_tag *cb, GLSV_GLA_EVT *i_evt, MDS_DEST to_mds_dest);

uint32_t glnd_mds_msg_send_rsp_gla(struct glnd_cb_tag *cb,
				GLSV_GLA_EVT *i_evt, MDS_DEST to_mds_dest, MDS_SYNC_SND_CTXT *mds_ctxt);

uint32_t glnd_mds_msg_send_glnd(struct glnd_cb_tag *cb, GLSV_GLND_EVT *i_evt, MDS_DEST to_mds_dest);

uint32_t glnd_mds_msg_send_gld(struct glnd_cb_tag *cb, GLSV_GLD_EVT *i_evt, MDS_DEST to_mds_dest);

uint32_t glnd_evt_local_send(GLND_CB *cb, GLSV_GLND_EVT *evt, uint32_t priority);

/* prototype for event handler */
typedef uint32_t (*GLSV_GLND_EVT_HANDLER) (struct glnd_cb_tag *, GLSV_GLND_EVT *);

/*  macros to fill up the events */

#define m_GLND_RESOURCE_SYNC_LCK_GRANT_FILL(m,err,lockid,status,c_handle) \
do { \
   memset(&(m), 0, sizeof(GLSV_GLA_EVT)); \
   (m).type = GLSV_GLA_API_RESP_EVT; \
   (m).error = err; \
   (m).info.gla_resp_info.type = GLSV_GLA_LOCK_SYNC_LOCK; \
   (m).info.gla_resp_info.param.sync_lock.lockId= lockid; \
   (m).info.gla_resp_info.param.sync_lock.lockStatus  = status; \
   (m).handle = (c_handle); \
} while (0);

#define m_GLND_RESOURCE_ASYNC_LCK_GRANT_FILL(m,err,lockid,lcllockid,mode,resid,invocation1,status,c_handle) \
do { \
   memset(&(m), 0, sizeof(GLSV_GLA_EVT)); \
   (m).type = GLSV_GLA_CALLBK_EVT; \
   (m).error = err; \
   (m).info.gla_clbk_info.callback_type = GLSV_LOCK_GRANT_CBK; \
   (m).info.gla_clbk_info.resourceId = (resid); \
   (m).info.gla_clbk_info.params.lck_grant.error = err; \
   (m).info.gla_clbk_info.params.lck_grant.lockId = (lockid); \
   (m).info.gla_clbk_info.params.lck_grant.lcl_lockId = (lcllockid); \
   (m).info.gla_clbk_info.params.lck_grant.lockMode = (mode); \
   (m).info.gla_clbk_info.params.lck_grant.resourceId = (resid); \
   (m).info.gla_clbk_info.params.lck_grant.invocation = (invocation1); \
   (m).info.gla_clbk_info.params.lck_grant.lockStatus = (status); \
   (m).handle = (c_handle); \
} while (0);

#define m_GLND_RESOURCE_ASYNC_LCK_WAITER_FILL(m,lockid,resid,inv,mode_held,mode_req,c_handle,wait,l_lockid) \
do { \
   memset(&(m), 0, sizeof(GLSV_GLA_EVT)); \
   (m).type = GLSV_GLA_CALLBK_EVT; \
   (m).info.gla_clbk_info.callback_type = GLSV_LOCK_WAITER_CBK; \
   (m).info.gla_clbk_info.resourceId = (resid); \
   (m).info.gla_clbk_info.params.lck_wait.lockId = (lockid); \
   (m).info.gla_clbk_info.params.lck_wait.lcl_lockId = (l_lockid); \
   (m).info.gla_clbk_info.params.lck_wait.resourceId = (resid); \
   (m).info.gla_clbk_info.params.lck_wait.invocation = (inv); \
   (m).info.gla_clbk_info.params.lck_wait.modeHeld = (mode_held); \
   (m).info.gla_clbk_info.params.lck_wait.modeRequested = (mode_req); \
   (m).info.gla_clbk_info.params.lck_wait.wait_signal = (wait); \
   (m).handle = (c_handle); \
} while (0);

#define m_GLND_RESOURCE_NODE_LCK_INFO_FILL(evt,evt_type,resid,lcl_resid,hdl_id,lock_id,l_type,l_flag,l_status,waiter_sig,l_held,l_err,l_lockid,l_invocation)\
do { \
   memset(&(evt), 0, sizeof(GLSV_GLND_EVT)); \
   (evt).type= evt_type; \
   (evt).info.node_lck_info.resource_id = (resid); \
   (evt).info.node_lck_info.lcl_resource_id = (lcl_resid); \
   (evt).info.node_lck_info.client_handle_id = (hdl_id);\
   (evt).info.node_lck_info.lockid = (lock_id); \
   (evt).info.node_lck_info.lcl_lockid = (l_lockid); \
   (evt).info.node_lck_info.lock_type = (l_type); \
   (evt).info.node_lck_info.lockFlags = (l_flag); \
   (evt).info.node_lck_info.lockStatus = (l_status); \
   (evt).info.node_lck_info.waiter_signal = (waiter_sig); \
   (evt).info.node_lck_info.mode_held = (l_held); \
   (evt).info.node_lck_info.error = (l_err); \
   (evt).info.node_lck_info.invocation = (l_invocation); \
} while (0);

#define m_GLND_RESOURCE_LCK_FILL(l_evt,l_evt_type,rid,is_orphan,l_mode) \
do { \
   memset(&(l_evt), 0, sizeof(GLSV_GLD_EVT)); \
   (l_evt).evt_type = l_evt_type; \
   (l_evt).info.rsc_details.rsc_id = rid; \
   (l_evt).info.rsc_details.orphan = (is_orphan); \
   (l_evt).info.rsc_details.lck_mode = (l_mode); \
} while (0);

#define m_GLND_RESOURCE_ASYNC_LCK_UNLOCK_FILL(l_evt,l_err,l_inv,l_resid,l_lockid,l_status) \
do { \
   memset(&(l_evt), 0, sizeof(GLSV_GLA_EVT)); \
   (l_evt).type = GLSV_GLA_CALLBK_EVT; \
   (l_evt).info.gla_clbk_info.callback_type = GLSV_LOCK_UNLOCK_CBK; \
   (l_evt).info.gla_clbk_info.resourceId = (l_resid); \
   (l_evt).info.gla_clbk_info.params.unlock.invocation = l_inv; \
   (l_evt).info.gla_clbk_info.params.unlock.lockId = l_lockid; \
   (l_evt).info.gla_clbk_info.params.unlock.resourceId = (l_resid); \
   (l_evt).info.gla_clbk_info.params.unlock.lockStatus = (l_status); \
   (l_evt).info.gla_clbk_info.params.unlock.error = (l_err); \
} while (0);

#endif  // LCK_LCKND_GLND_MDS_H_
