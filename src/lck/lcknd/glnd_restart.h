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

#ifndef LCK_LCKND_GLND_RESTART_H_
#define LCK_LCKND_GLND_RESTART_H_

#include "lck/lcknd/glnd_evt.h"

#include <sys/mman.h>

/*defines*/
#define GLND_SHM_INFO_VALID 1
#define GLND_SHM_INFO_INVALID 0
#define LCL_LOCK_REQ_LIST 1
#define LOCK_MASTER_LIST  2

#define RES_SHM_NAME "NCS_GLND_RES_CKPT_INFO"
#define LCK_SHM_NAME "NCS_GLND_LCK_CKPT_INFO"
#define EVT_SHM_NAME "NCS_GLND_EVT_CKPT_INFO"

uint32_t glnd_find_res_shm_ckpt_empty_section(GLND_CB *cb, uint32_t *index);
uint32_t glnd_find_lck_shm_ckpt_empty_section(GLND_CB *cb, uint32_t *index);
uint32_t glnd_find_evt_shm_ckpt_empty_section(GLND_CB *cb, uint32_t *index);
uint32_t glnd_res_shm_section_invalidate(GLND_CB *cb, GLND_RESOURCE_INFO *res_info);
uint32_t glnd_lck_shm_section_invalidate(GLND_CB *cb, GLND_RES_LOCK_LIST_INFO *lock_info);
uint32_t glnd_evt_shm_section_invalidate(GLND_CB *cb, GLSV_GLND_EVT *glnd_evt);

GLND_RESOURCE_INFO *glnd_restart_client_resource_node_add(GLND_CB *glnd_cb, SaLckResourceIdT resource_id);
uint32_t glnd_restart_build_database(GLND_CB *glnd_cb);

uint32_t glnd_restart_resource_info_ckpt_write(GLND_CB *glnd_cb, GLND_RESOURCE_INFO *res_info);
uint32_t glnd_restart_resource_info_ckpt_overwrite(GLND_CB *glnd_cb, GLND_RESOURCE_INFO *res_info);

uint32_t glnd_restart_lock_event_info_ckpt_write(GLND_CB *glnd_cb,
						       GLSV_RESTART_BACKUP_EVT_INFO restart_backup_evt);
uint32_t glnd_restart_res_lock_list_ckpt_write(GLND_CB *glnd_cb, GLND_RES_LOCK_LIST_INFO *res_lock_list,
						     SaLckResourceIdT res_id, SaLckHandleT app_handle_id,
						     uint8_t to_which_list);

uint32_t glnd_restart_res_lock_list_ckpt_overwrite(GLND_CB *glnd_cb, GLND_RES_LOCK_LIST_INFO *res_lock_list,
							 SaLckResourceIdT res_id, SaLckHandleT app_handle_id,
							 uint8_t to_which_list);

uint32_t glnd_restart_res_lock_ckpt_read(GLND_CB *glnd_cb, GLND_RESTART_RES_LOCK_LIST_INFO *restart_res_lock_info,
					       uint32_t offset);

uint32_t glnd_restart_resource_ckpt_read(GLND_CB *glnd_cb, GLND_RESTART_RES_INFO *restart_resource_info,
					       uint32_t offset);

uint32_t glnd_restart_backup_event_read(GLND_CB *glnd_cb, GLSV_RESTART_BACKUP_EVT_INFO *restart_backup_evt,
					      uint32_t offset);
GLND_RESOURCE_INFO *glnd_restart_client_resource_node_add(GLND_CB *glnd_cb, SaLckResourceIdT resource_id);

#endif  // LCK_LCKND_GLND_RESTART_H_
