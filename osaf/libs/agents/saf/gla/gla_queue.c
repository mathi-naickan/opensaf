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

  This file contains the routines to handle the callback queue mgmt
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

******************************************************************************
*/

#include "gla.h"

NCS_BOOL gla_client_cleanup_mbx(NCSCONTEXT arg, NCSCONTEXT msg);

/****************************************************************************
  Name          : glsv_gla_callback_queue_init
  
  Description   : This routine is used to initialize the queue for the callbacks.
 
  Arguments     : client_info - pointer to the client info
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 glsv_gla_callback_queue_init(GLA_CLIENT_INFO *client_info)
{
	if (m_NCS_IPC_CREATE(&client_info->callbk_mbx) == NCSCC_RC_SUCCESS) {
		if (m_NCS_IPC_ATTACH(&client_info->callbk_mbx) == NCSCC_RC_SUCCESS) {
			return NCSCC_RC_SUCCESS;
		}
		m_NCS_IPC_RELEASE(&client_info->callbk_mbx, NULL);
	}
	m_LOG_GLA_SYS_CALL(GLA_GET_SEL_OBJ_FAIL, __FILE__, __LINE__, client_info->lock_handle_id);
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : gla_client_cleanup_mbx
  
  Description   : This routine is used to destroy the queue for the callbacks.
 
  Arguments     : client_info - pointer to the client info
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
NCS_BOOL gla_client_cleanup_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	GLSV_GLA_CALLBACK_INFO *prev, *node = (GLSV_GLA_CALLBACK_INFO *)msg;

	/* loop thru the queue and deallocate the nodes */
	while (node) {
		prev = node;
		node = node->next;
		m_MMGR_FREE_GLA_CALLBACK_INFO(prev);
	}
	return TRUE;
}

/****************************************************************************
  Name          : glsv_gla_callback_queue_destroy
  
  Description   : This routine is used to destroy the queue for the callbacks.
 
  Arguments     : client_info - pointer to the client info
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
void glsv_gla_callback_queue_destroy(GLA_CLIENT_INFO *client_info)
{
	/* detach the mail box */
	m_NCS_IPC_DETACH(&client_info->callbk_mbx, gla_client_cleanup_mbx, client_info);

	/* delete the mailbox */
	m_NCS_IPC_RELEASE(&client_info->callbk_mbx, NULL);
}

/****************************************************************************
  Name          : glsv_gla_callback_queue_write
 
  Description   : This routine is used to queue the callbacks to the client 
                  by the MDS.
 
  Arguments     : 
                  gla_cb - pointer to the gla control block
                  handle - handle id of the client
                  clbk_info - pointer to the callback information
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 glsv_gla_callback_queue_write(GLA_CB *gla_cb, SaLckHandleT handle, GLSV_GLA_CALLBACK_INFO *clbk_info)
{
	GLA_CLIENT_INFO *client_info = NULL;
	uns32 rc = NCSCC_RC_FAILURE;

	m_NCS_LOCK(&gla_cb->cb_lock, NCS_LOCK_READ);
	/* Search for the node from the client tree */
	client_info = (GLA_CLIENT_INFO *)ncs_patricia_tree_get(&gla_cb->gla_client_tree, (uns8 *)&handle);

	if (client_info == NULL) {
		/* recieved a callback for an non-existant client. so return failure */
		return rc;
	} else {
		rc = m_NCS_IPC_SEND(&client_info->callbk_mbx, clbk_info, NCS_IPC_PRIORITY_NORMAL);
	}
	m_NCS_UNLOCK(&gla_cb->cb_lock, NCS_LOCK_READ);
	return rc;

}

/****************************************************************************
  Name          : glsv_gla_callback_queue_read
 
  Description   : This routine is used to read from the queue for the callbacks
 
  Arguments     : gla_cb - pointer to the gla control block
                  handle - handle id of the client
 
  Return Values : pointer to the callback
 
  Notes         : None
******************************************************************************/
GLSV_GLA_CALLBACK_INFO *glsv_gla_callback_queue_read(GLA_CLIENT_INFO *client_info)
{
	/* remove it to the queue */
	return (GLSV_GLA_CALLBACK_INFO *)m_NCS_IPC_NON_BLK_RECEIVE(&client_info->callbk_mbx, NULL);
}
