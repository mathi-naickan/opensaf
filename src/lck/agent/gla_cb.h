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

#ifndef LCK_AGENT_GLA_CB_H_
#define LCK_AGENT_GLA_CB_H_

#include "base/ncssysf_tmr.h"
#include "base/ncssysf_mem.h"
#include "base/ncspatricia.h"

extern uint32_t gl_gla_hdl;
#define m_GLSV_GLA_RETRIEVE_GLA_CB  ncshm_take_hdl(NCS_SERVICE_ID_GLA, gl_gla_hdl)
#define m_GLSV_GLA_GIVEUP_GLA_CB    ncshm_give_hdl(gl_gla_hdl)

#define m_GLSV_MEMSET_SANAME(name) \
{\
   memset((void *)&name->value[name->length],0,(size_t)( SA_MAX_NAME_LENGTH - name->length )); \
}

typedef struct gla_client_info_tag {
	NCS_PATRICIA_NODE patnode;
	SaLckHandleT lock_handle_id;	/* index for the tree at GLND */
	SaLckHandleT lcl_lock_handle_id;	/* index for the tree */
	uint32_t client_context_id;
	SaLckCallbacksT lckCallbk;
	SaTimeT lcktimer;
	/* Mailbox Queue to store the messages for the clients */
	SYSF_MBX callbk_mbx;
	NCS_PATRICIA_TREE client_res_tree;
} GLA_CLIENT_INFO;

typedef struct gla_client_res_info_tag {
	NCS_PATRICIA_NODE patnode;
	SaLckResourceIdT gbl_res_id;
	uint32_t lcl_res_cnt;
} GLA_CLIENT_RES_INFO;
typedef struct glsv_gla_tmr_callback_info {
	GLA_CALLBK_TYPE callback_type;
	SaInvocationT invocation;
	SaLckResourceIdT resourceId;
	SaLckLockIdT lcl_lockId;
} GLSV_GLA_TMR_CALLBACK_INFO;

typedef struct gla_tmr_tag {
	tmr_t tmr_id;
	SaLckHandleT client_hdl;
	bool is_active;
	GLSV_GLA_TMR_CALLBACK_INFO clbk_info;
} GLA_TMR;

typedef struct gla_resource_id_info_tag {
	NCS_PATRICIA_NODE patnode;
	SaLckResourceHandleT lcl_res_id;	/* index for the tree */
	SaLckHandleT lock_handle_id;
	SaLckResourceIdT gbl_res_id;
	GLA_TMR res_async_tmr;
} GLA_RESOURCE_ID_INFO;

typedef struct gla_lock_id_index_tag {
	SaLckHandleT lock_handle_id;
	SaLckResourceIdT gbl_res_id;
	SaLckLockIdT gbl_lock_id;
} GLA_LOCK_ID_INDEX;

typedef struct gla_lock_id_info_tag {
	NCS_PATRICIA_NODE patnode;
	SaLckLockIdT lcl_lock_id;	/* index for the tree */
	SaLckHandleT lock_handle_id;
	SaLckResourceIdT gbl_res_id;
	SaLckResourceIdT lcl_res_id;
	SaLckLockIdT gbl_lock_id;
	SaLckLockModeT mode;
	GLA_TMR lock_async_tmr;
	GLA_TMR unlock_async_tmr;

} GLA_LOCK_ID_INFO;

/*****************************************************************************
 * Data Structure Used to hold GLA control block
 *****************************************************************************/
typedef struct gla_cb_tag {
	/* Identification Information about the GLA */
	uint32_t process_id;
	uint8_t *process_name;
	uint32_t agent_handle_id;
	uint8_t pool_id;
	MDS_HDL gla_mds_hdl;
	MDS_DEST gla_mds_dest;
	NCS_LOCK cb_lock;

	/* Information about GLND */
	MDS_DEST glnd_mds_dest;
	bool glnd_svc_up;
	bool glnd_crashed;

	/* GLA data */
	NCS_PATRICIA_TREE gla_client_tree;	/* GLA_CLIENT_INFO - node */

	uint32_t lcl_res_id_count;
	/* Local to global mapping for Resource Id's */
	NCS_PATRICIA_TREE gla_resource_id_tree;	/* GLA_RESOURCE_ID_INFO */

	/* Local to global mapping for Lock Id's */
	NCS_PATRICIA_TREE gla_lock_id_tree;	/* GLA_LOCK_ID_INFO */
	/* Sync up with GLND ( MDS ) */
	NCS_LOCK glnd_sync_lock;
	bool glnd_sync_awaited;
	NCS_SEL_OBJ glnd_sync_sel;

} GLA_CB;

uint32_t gla_create(NCS_LIB_CREATE *create_info);
uint32_t gla_destroy(NCS_LIB_DESTROY *destroy_info);

/* function prototypes for client handling*/
uint32_t gla_client_tree_init(GLA_CB *cb);
void gla_client_tree_destroy(GLA_CB *gla_cb);
void gla_client_tree_cleanup(GLA_CB *gla_cb);
GLA_CLIENT_INFO *gla_client_tree_find_and_add(GLA_CB *gla_cb, SaLckHandleT hdl_id, bool flag);
uint32_t gla_client_tree_delete_node(GLA_CB *gla_cb, GLA_CLIENT_INFO *client_info, bool give_hdl);
GLA_CLIENT_RES_INFO *gla_client_res_tree_find_and_add(GLA_CLIENT_INFO *client_info, SaLckResourceIdT res_id,
							       bool flag);
uint32_t gla_client_res_tree_destroy(GLA_CLIENT_INFO *client_info);

/* queue prototypes */

uint32_t glsv_gla_callback_queue_init(struct gla_client_info_tag *client_info);
void glsv_gla_callback_queue_destroy(struct gla_client_info_tag *client_info);
uint32_t glsv_gla_callback_queue_write(struct gla_cb_tag *gla_cb,
				    SaLckHandleT handle, struct glsv_gla_callback_info_tag *clbk_info);
GLSV_GLA_CALLBACK_INFO *glsv_gla_callback_queue_read(struct gla_client_info_tag *client_info);

/* callback prototypes */
uint32_t gla_hdl_callbk_dispatch_one(struct gla_cb_tag *cb, struct gla_client_info_tag *client_info);
uint32_t gla_hdl_callbk_dispatch_all(struct gla_cb_tag *cb, struct gla_client_info_tag *client_info);
uint32_t gla_hdl_callbk_dispatch_block(struct gla_cb_tag *gla_cb, struct gla_client_info_tag *client_info);

/* resource table prototypes */
uint32_t gla_res_tree_init(GLA_CB *cb);
void gla_res_tree_destroy(GLA_CB *gla_cb);
void gla_res_tree_cleanup(GLA_CB *gla_cb);
GLA_RESOURCE_ID_INFO *gla_res_tree_find_and_add(GLA_CB *gla_cb, SaLckResourceHandleT res_id, bool flag);
uint32_t gla_res_tree_delete_node(GLA_CB *gla_cb, GLA_RESOURCE_ID_INFO *res_info);
GLA_RESOURCE_ID_INFO *gla_res_tree_reverse_find(GLA_CB *gla_cb, SaLckHandleT handle, SaLckResourceIdT gbl_res);
void gla_res_tree_cleanup_client_down(GLA_CB *gla_cb, SaLckHandleT handle);

/* Lock table prototypes */
uint32_t gla_lock_tree_init(GLA_CB *cb);
void gla_lock_tree_destroy(GLA_CB *gla_cb);
void gla_lock_tree_cleanup(GLA_CB *gla_cb);
GLA_LOCK_ID_INFO *gla_lock_tree_find_and_add(GLA_CB *gla_cb, SaLckLockIdT lock_id, bool flag);
uint32_t gla_lock_tree_delete_node(GLA_CB *gla_cb, GLA_LOCK_ID_INFO *lock_info);
GLA_LOCK_ID_INFO *gla_lock_tree_reverse_find(GLA_CB *gla_cb,
					     SaLckHandleT handle, SaLckResourceIdT gbl_res, SaLckLockIdT gbl_lock);
void gla_lock_tree_cleanup_client_down(GLA_CB *gla_cb, SaLckHandleT handle);
void gla_res_lock_tree_cleanup_client_down(GLA_CB *gla_cb, GLA_RESOURCE_ID_INFO *res_info, SaLckHandleT handle);

uint32_t gla_client_info_send(GLA_CB *gla_cb);

uint32_t gla_start_tmr(GLA_TMR *tmr);
void gla_stop_tmr(GLA_TMR *tmr);
void gla_tmr_exp(NCSCONTEXT uarg);

#endif  // LCK_AGENT_GLA_CB_H_
