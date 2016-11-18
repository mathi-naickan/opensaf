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

  This file contains the routines to handle the event backup queue
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

******************************************************************************
*/

#include "lck/lcknd/glnd.h"
#include <string.h>

/****************************************************************************
  Name          : glnd_evt_backup_queue_add
  
  Description   : This routine is used to queue the backup events
 
  Arguments     : glnd-cb - pointer to the control block
                  glnd_evt - pointer to the glnd_evt
                   
  Return Values : none
 
  Notes         : None
******************************************************************************/
void glnd_evt_backup_queue_add(GLND_CB *glnd_cb, GLSV_GLND_EVT *glnd_evt)
{
	GLSV_GLND_EVT *evt = m_MMGR_ALLOC_GLND_EVT;

	if (!evt) {
		LOG_CR("GLND evt alloc failed: Error %s", strerror(errno));
		assert(0);
	}
	memcpy(evt, glnd_evt, sizeof(GLSV_GLND_EVT));

	evt->next = NULL;
	/* add it to the queue */
	if (glnd_cb->evt_bckup_q == NULL)
		glnd_cb->evt_bckup_q = evt;
	else {
		evt->next = glnd_cb->evt_bckup_q;
		glnd_cb->evt_bckup_q = evt;
	}

	return;
}

/****************************************************************************
  Name          : glnd_evt_backup_queue_delete_lock_req
  
  Description   : This routine is used to delete the node from the queue 
 
  Arguments     : glnd_cb - pointer to the control block
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t glnd_evt_backup_queue_delete_lock_req(GLND_CB *glnd_cb,
					    SaLckHandleT hldId, SaLckResourceIdT resId, SaLckLockModeT type)
{
	GLSV_GLND_EVT *prev, *evt = glnd_cb->evt_bckup_q;

	for (prev = NULL; evt != NULL; evt = evt->next) {
		if (evt->type == GLSV_GLND_EVT_LCK_REQ &&
		    evt->info.node_lck_info.lock_type == type &&
		    evt->info.node_lck_info.client_handle_id == hldId && evt->info.node_lck_info.resource_id == resId) {
			/* delete the evt */
			if (prev) {
				prev->next = evt->next;
			} else {
				glnd_cb->evt_bckup_q = evt->next;
			}
			/* Delete the shared memory section corresponding to this backup event */
			glnd_evt_shm_section_invalidate(glnd_cb, evt);
			glnd_evt_destroy(evt);
			return NCSCC_RC_SUCCESS;
		}
		prev = evt;
	}

	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : glnd_evt_backup_queue_delete_unlock_req
  
  Description   : This routine is used to delete the node from the queue 
 
  Arguments     : glnd_cb - pointer to the control block
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
void glnd_evt_backup_queue_delete_unlock_req(GLND_CB *glnd_cb,
					     SaLckLockIdT lockid, SaLckHandleT hldId, SaLckResourceIdT resId)
{
	GLSV_GLND_EVT *prev, *evt = glnd_cb->evt_bckup_q;

	for (prev = NULL; evt != NULL; evt = evt->next) {
		if (evt->type == GLSV_GLND_EVT_UNLCK_REQ &&
		    evt->info.node_lck_info.lockid == lockid &&
		    evt->info.node_lck_info.client_handle_id == hldId && evt->info.node_lck_info.resource_id == resId) {
			/* delete the evt */
			if (prev) {
				prev->next = evt->next;
			} else {
				glnd_cb->evt_bckup_q = evt->next;
			}
			glnd_evt_destroy(evt);
			break;
		}
		prev = evt;
	}
	return;
}

/****************************************************************************
  Name          : glnd_evt_backup_queue_destroy
  
  Description   : This routine is used to destroy the  backup events queue
 
  Arguments     : glnd-cb - pointer to the control block
                  
                   
  Return Values : none
 
  Notes         : None
******************************************************************************/
void glnd_evt_backup_queue_destroy(GLND_CB *glnd_cb)
{
	GLSV_GLND_EVT *evt, *del_node;

	for (evt = glnd_cb->evt_bckup_q; evt != NULL;) {
		del_node = evt;
		evt = evt->next;
		glnd_evt_destroy(del_node);
	}
	glnd_cb->evt_bckup_q = NULL;
	return;
}

/****************************************************************************
  Name          : glnd_re_send_evt_backup_queue
  
  Description   : This routine is used to re-send the  backup events queue
 
  Arguments     : glnd-cb - pointer to the control block
                  
                   
  Return Values : none
 
  Notes         : None
******************************************************************************/
void glnd_re_send_evt_backup_queue(GLND_CB *glnd_cb)
{
	GLSV_GLND_EVT *evt, *prev = NULL, *next = NULL;
	GLND_RESOURCE_INFO *res_info = NULL;
	for (evt = glnd_cb->evt_bckup_q; evt != NULL;) {
		next = evt->next;
		switch (evt->type) {
		case GLSV_GLND_EVT_LCK_REQ:
		case GLSV_GLND_EVT_UNLCK_REQ:
		case GLSV_GLND_EVT_LCK_PURGE:
		case GLSV_GLND_EVT_LCK_REQ_CANCEL:
			if (evt->type != GLSV_GLND_EVT_LCK_PURGE)
				res_info = glnd_resource_node_find(glnd_cb, evt->info.node_lck_info.resource_id);
			else
				res_info = glnd_resource_node_find(glnd_cb, evt->info.rsc_info.resource_id);
			if (res_info) {
				if (res_info->status != GLND_RESOURCE_ELECTION_IN_PROGESS) {
					glnd_mds_msg_send_glnd(glnd_cb, evt, res_info->master_mds_dest);
					if (evt == glnd_cb->evt_bckup_q)
						glnd_cb->evt_bckup_q = evt->next;
					glnd_evt_destroy(evt);
					if (prev)
						prev->next = next;
				}
			} else {
				if (evt == glnd_cb->evt_bckup_q)
					glnd_cb->evt_bckup_q = evt->next;
				glnd_evt_destroy(evt);
				if (prev)
					prev->next = next;

			}
			break;
		default:
			break;
		}
		prev = next;
		evt = next;
	}
	return;
}
