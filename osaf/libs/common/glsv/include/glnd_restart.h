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

#ifndef GLND_RESTART_H
#define GLND_RESTART_H

#include "glnd_evt.h"

#include <sys/mman.h>

/*defines*/
#define GLND_SHM_INFO_VALID 1
#define GLND_SHM_INFO_INVALID 0
#define LCL_LOCK_REQ_LIST 1
#define LOCK_MASTER_LIST  2

#define RES_SHM_NAME "NCS_GLND_RES_CKPT_INFO"
#define LCK_SHM_NAME "NCS_GLND_LCK_CKPT_INFO"
#define EVT_SHM_NAME "NCS_GLND_EVT_CKPT_INFO"

EXTERN_C uns32 glnd_find_res_shm_ckpt_empty_section(GLND_CB *cb, uns32 *index);
EXTERN_C uns32 glnd_find_lck_shm_ckpt_empty_section(GLND_CB *cb, uns32 *index);
EXTERN_C uns32 glnd_find_evt_shm_ckpt_empty_section(GLND_CB *cb, uns32 *index);
EXTERN_C uns32 glnd_res_shm_section_invalidate(GLND_CB *cb, GLND_RESOURCE_INFO *res_info);
EXTERN_C uns32 glnd_lck_shm_section_invalidate(GLND_CB *cb, GLND_RES_LOCK_LIST_INFO *lock_info);
EXTERN_C uns32 glnd_evt_shm_section_invalidate(GLND_CB *cb, GLSV_GLND_EVT *glnd_evt);

EXTERN_C GLND_RESOURCE_INFO *glnd_restart_client_resource_node_add(GLND_CB *glnd_cb, SaLckResourceIdT resource_id);
EXTERN_C uns32 glnd_restart_build_database(GLND_CB *glnd_cb);

EXTERN_C uns32 glnd_restart_resource_info_ckpt_write(GLND_CB *glnd_cb, GLND_RESOURCE_INFO *res_info);
EXTERN_C uns32 glnd_restart_resource_info_ckpt_overwrite(GLND_CB *glnd_cb, GLND_RESOURCE_INFO *res_info);

EXTERN_C uns32 glnd_restart_lock_event_info_ckpt_write(GLND_CB *glnd_cb,
						       GLSV_RESTART_BACKUP_EVT_INFO restart_backup_evt);
EXTERN_C uns32 glnd_restart_res_lock_list_ckpt_write(GLND_CB *glnd_cb, GLND_RES_LOCK_LIST_INFO *res_lock_list,
						     SaLckResourceIdT res_id, SaLckHandleT app_handle_id,
						     uns8 to_which_list);

EXTERN_C uns32 glnd_restart_res_lock_list_ckpt_overwrite(GLND_CB *glnd_cb, GLND_RES_LOCK_LIST_INFO *res_lock_list,
							 SaLckResourceIdT res_id, SaLckHandleT app_handle_id,
							 uns8 to_which_list);

EXTERN_C uns32 glnd_restart_res_lock_ckpt_read(GLND_CB *glnd_cb, GLND_RESTART_RES_LOCK_LIST_INFO *restart_res_lock_info,
					       uns32 offset);

EXTERN_C uns32 glnd_restart_resource_ckpt_read(GLND_CB *glnd_cb, GLND_RESTART_RES_INFO *restart_resource_info,
					       uns32 offset);

EXTERN_C uns32 glnd_restart_backup_event_read(GLND_CB *glnd_cb, GLSV_RESTART_BACKUP_EVT_INFO *restart_backup_evt,
					      uns32 offset);
EXTERN_C GLND_RESOURCE_INFO *glnd_restart_client_resource_node_add(GLND_CB *glnd_cb, SaLckResourceIdT resource_id);

#endif
