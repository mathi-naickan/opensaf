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
  FILE NAME: mqsv_db.c

  DESCRIPTION: MQND Data base routines.

******************************************************************************/

#include "mqnd.h"

/****************************************************************************
 * Name          : mqnd_queue_node_get
 *
 * Description   : Function to get the queue node from Tree.
 *
 * Arguments     : MQND_CB *cb, - MQND Control Block
 *                 uint32_t qhdl - queue handle
 *                 
 * Return Values : MQND_QUEUE_NODE** o_qnode - Queu Node at MQND
 *
 * Notes         : None.
 *****************************************************************************/
void mqnd_queue_node_get(MQND_CB *cb, SaMsgQueueHandleT qhdl, MQND_QUEUE_NODE **o_qnode)
{
	if (cb->is_qhdl_db_up)
		*o_qnode = (MQND_QUEUE_NODE *)ncs_patricia_tree_get(&cb->qhndl_db, (uint8_t *)&qhdl);
	return;
}

/****************************************************************************
 * Name          : mqnd_queue_node_getnext
 *
 * Description   : Function to get the queue node from Tree.
 *
 * Arguments     : MQND_CB *cb, - MQND Control Block
 *                 uint32_t qhdl - queue handle
 *                 
 * Return Values : MQND_QUEUE_NODE** o_qnode - Queu Node at MQND
 *
 * Notes         : None.
 *****************************************************************************/
void mqnd_queue_node_getnext(MQND_CB *cb, SaMsgQueueHandleT qhdl, MQND_QUEUE_NODE **o_qnode)
{
	if (cb->is_qhdl_db_up) {
		if (qhdl)
			*o_qnode = (MQND_QUEUE_NODE *)ncs_patricia_tree_getnext(&cb->qhndl_db, (uint8_t *)&qhdl);
		else
			*o_qnode = (MQND_QUEUE_NODE *)ncs_patricia_tree_getnext(&cb->qhndl_db, (uint8_t *)NULL);
	}
	return;
}

/****************************************************************************
 * Name          : mqnd_queue_node_add
 *
 * Description   : Function to Add the queue node from Tree.
 *
 * Arguments     : MQND_CB *cb, - MQND Control Block
 *                 MQND_QUEUE_NODE *qnode - queue node at MQND
 *                 
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE  
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t mqnd_queue_node_add(MQND_CB *cb, MQND_QUEUE_NODE *qnode)
{
	uint32_t rc = NCSCC_RC_FAILURE;

	qnode->pnode.key_info = (uint8_t *)&(qnode->qinfo.queueHandle);

	if (cb->is_qhdl_db_up)
		rc = ncs_patricia_tree_add(&cb->qhndl_db, (NCS_PATRICIA_NODE *)qnode);
	if (rc != NCSCC_RC_SUCCESS)
		LOG_ER("Adding the queue node to Tree failed");
	return rc;
}

/****************************************************************************
 * Name          : mqnd_queue_node_del
 *
 * Description   : Function to Delete the queue node from Tree.
 *
 * Arguments     : MQND_CB *cb, - MQND Control Block
 *                 uint32_t qhdl - queue handle
 *                 
 * Return Values : MQND_QUEUE_NODE** o_qnode - Queu Node at MQND
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t mqnd_queue_node_del(MQND_CB *cb, MQND_QUEUE_NODE *qnode)
{
	uint32_t rc = NCSCC_RC_FAILURE;

	if ((!(qnode->qinfo.queueStatus.creationFlags & SA_MSG_QUEUE_PERSISTENT)) || (qnode->qinfo.tmr.is_active))
		mqnd_tmr_stop(&qnode->qinfo.tmr);

	if (qnode->qinfo.qtransfer_complete_tmr.is_active)
		mqnd_tmr_stop(&qnode->qinfo.qtransfer_complete_tmr);

	if (cb->is_qhdl_db_up)
		rc = ncs_patricia_tree_del(&cb->qhndl_db, (NCS_PATRICIA_NODE *)qnode);
	if (rc != NCSCC_RC_SUCCESS)
		LOG_ER("Deleting the queue node from Tree failed");
	return rc;
}

/****************************************************************************
 * Name          : mqnd_qevt_node_get
 *
 * Description   : Function to get the queue event from Tree.
 *
 * Arguments     : MQND_CB *cb, - MQND Control Block
 *                 uint32_t qhdl - queue handle
 *                 
 * Return Values : MQND_QTRANSFER_EVT_NODE ** o_qnode - Queue Event Node at MQND
 *
 * Notes         : None.
 *****************************************************************************/
void mqnd_qevt_node_get(MQND_CB *cb, SaMsgQueueHandleT qhdl, MQND_QTRANSFER_EVT_NODE **o_qnode)
{
	if (cb->is_qevt_hdl_db_up)
		*o_qnode = (MQND_QTRANSFER_EVT_NODE *)ncs_patricia_tree_get(&cb->q_transfer_evt_db, (uint8_t *)&qhdl);
	return;
}

/****************************************************************************
 * Name          : mqnd_qevt_node_getnext
 *
 * Description   : Function to get the queue event node from Tree.
 *
 * Arguments     : MQND_CB *cb, - MQND Control Block
 *                 uint32_t qhdl - queue handle
 *                 
 * Return Values : MQND_QTRANSFER_EVT_NODE** o_qnode - Queu event Node at MQND
 *
 * Notes         : None.
 *****************************************************************************/
void mqnd_qevt_node_getnext(MQND_CB *cb, SaMsgQueueHandleT qhdl, MQND_QTRANSFER_EVT_NODE **o_qnode)
{
	if (cb->is_qevt_hdl_db_up) {
		if (qhdl)
			*o_qnode = (MQND_QTRANSFER_EVT_NODE *)ncs_patricia_tree_getnext(&cb->q_transfer_evt_db,
											(uint8_t *)&qhdl);
		else
			*o_qnode = (MQND_QTRANSFER_EVT_NODE *)ncs_patricia_tree_getnext(&cb->q_transfer_evt_db,
											(uint8_t *)NULL);
	}
	return;
}

/****************************************************************************
 * Name          : mqnd_qevt_node_add
 *
 * Description   : Function to Add the queue event node to Tree.
 *
 * Arguments     : MQND_CB *cb, - MQND Control Block
 *                 MQND_QTRANSFER_EVT_NODE**qnode - queue event node at MQND
 *                 
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE  
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t mqnd_qevt_node_add(MQND_CB *cb, MQND_QTRANSFER_EVT_NODE *qevt_node)
{
	uint32_t rc = NCSCC_RC_FAILURE;

	qevt_node->evt.key_info = (uint8_t *)&(qevt_node->tmr.qhdl);

	if (cb->is_qevt_hdl_db_up)
		rc = ncs_patricia_tree_add(&cb->q_transfer_evt_db, (NCS_PATRICIA_NODE *)qevt_node);
	if (rc != NCSCC_RC_SUCCESS)
		LOG_ER("Adding the queue event node to Tree failed");
	return rc;
}

/****************************************************************************
 * Name          : mqnd_qevt_node_del
 *
 * Description   : Function to Delete the queue event node from Tree.
 *
 * Arguments     : MQND_CB *cb, - MQND Control Block
 *                 uint32_t qhdl - queue handle
 *                 
 * Return Values : MQND_QTRANSFER_EVT_NODE** o_qnode - Queue Event Node at MQND
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t mqnd_qevt_node_del(MQND_CB *cb, MQND_QTRANSFER_EVT_NODE *qevt_node)
{
	uint32_t rc = NCSCC_RC_FAILURE;

	if (cb->is_qevt_hdl_db_up)
		rc = ncs_patricia_tree_del(&cb->q_transfer_evt_db, (NCS_PATRICIA_NODE *)qevt_node);
	if (rc != NCSCC_RC_SUCCESS)
		LOG_ER("Deleting the queue event node from Tree failed");
	return rc;
}
