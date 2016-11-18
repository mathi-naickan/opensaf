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

#ifndef LCK_LCKND_GLND_CLIENT_H_
#define LCK_LCKND_GLND_CLIENT_H_

typedef struct glnd_client_list_resource_lock_req_tag {
	GLND_RES_LOCK_LIST_INFO *lck_req;
	struct glnd_client_list_resource_lock_req_tag *prev, *next;
} GLND_CLIENT_LIST_RESOURCE_LOCK_REQ;

typedef struct glnd_client_list_resource_tag {
	struct glnd_resource_info_tag *rsc_info;
	uint32_t open_ref_cnt;
	GLND_CLIENT_LIST_RESOURCE_LOCK_REQ *lck_list;
	struct glnd_client_list_resource_tag *prev, *next;
} GLND_CLIENT_LIST_RESOURCE;

typedef struct glnd_client_info_tag {
	NCS_PATRICIA_NODE patnode;
	SaLckHandleT app_handle_id;	/* index for the client tree */
	MDS_DEST agent_mds_dest;	/* mds dest of the agent */
	uint32_t app_proc_id;
	SaVersionT version;
	uint16_t cbk_reg_info;	/* bit-wise data */
	GLND_CLIENT_LIST_RESOURCE *res_list;	/* this list will be used for the deadlock algo */
} GLND_CLIENT_INFO;

/*prototypes */
GLND_CLIENT_INFO *glnd_client_node_find(GLND_CB *glnd_cb, SaLckHandleT handle_id);

GLND_CLIENT_INFO *glnd_client_node_find_next(GLND_CB *glnd_cb,
						      SaLckHandleT handle_id, MDS_DEST agent_mds_dest);

GLND_CLIENT_INFO *glnd_client_node_add(GLND_CB *glnd_cb, MDS_DEST agent_mds_dest, SaLckHandleT app_handle_id);

uint32_t glnd_client_node_del(GLND_CB *glnd_cb, GLND_CLIENT_INFO *client_info);

uint32_t glnd_client_node_resource_add(GLND_CLIENT_INFO *client_info, struct glnd_resource_info_tag *res_info);

uint32_t glnd_client_node_resource_del(GLND_CB *glnd_cb,
					     GLND_CLIENT_INFO *client_info, struct glnd_resource_info_tag *res_info);

uint32_t glnd_client_node_lcl_resource_del(GLND_CB *glnd_cb,
						 GLND_CLIENT_INFO *client_info,
						 struct glnd_resource_info_tag *res_info,
						 SaLckResourceIdT lcl_resource_id,
						 uint32_t lcl_res_id_count, bool *resource_del_flag);

uint32_t glnd_client_node_resource_lock_req_add(GLND_CLIENT_INFO *client_info,
						      struct glnd_resource_info_tag *res_info,
						      GLND_RES_LOCK_LIST_INFO *lock_req_info);

uint32_t glnd_client_node_resource_lock_req_del(GLND_CLIENT_INFO *client_info,
						      GLND_CLIENT_LIST_RESOURCE *res_list,
						      GLND_CLIENT_LIST_RESOURCE_LOCK_REQ *lock_req_list);

GLND_CLIENT_LIST_RESOURCE_LOCK_REQ *glnd_client_node_resource_lock_req_find(GLND_CLIENT_INFO *client_info,
										     SaLckResourceIdT res_id,
										     SaLckLockIdT lockid);

uint32_t glnd_client_node_resource_lock_req_find_and_del(GLND_CLIENT_INFO *client_info,
							       SaLckResourceIdT res_id,
							       SaLckLockIdT lockid, SaLckResourceIdT lcl_res_id);
uint32_t glnd_client_node_resource_lock_find_duplicate_ex(GLND_CLIENT_INFO *client_info,
								SaLckResourceIdT res_id, SaLckResourceIdT lcl_res_id);

#endif  // LCK_LCKND_GLND_CLIENT_H_
