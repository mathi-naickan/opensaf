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

  DESCRIPTION: This file inclused following routines:
   
   mqd_db_node_add......................Routine to add DB node
   mqd_db_node_del......................Routine to del DB node
   mqd_db_node_create...................Routine to create DB node
   mqd_qparam_upd.......................Routine to update queue params
   mqd_track_add........................Routine to add track information
   mqd_track_del........................Routine to delete track information
   mqd_track_obj_cmp....................Routine to compare track objects
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#include "msg/msgd/mqd.h"

#if (NCS_MQD != 0)

/******************************** LOCAL ROUTINES *****************************/
static bool mqd_track_obj_cmp(void *key, void *elem);
/*****************************************************************************/

/****************************************************************************\
   PROCEDURE NAME :  mqd_db_node_add

   DESCRIPTION    :  This routines adds the Object node into the Tree
                   
   ARGUMENTS      :  pMqd  - MQD Controll block pointer
                     pNode - Object Node 

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
uint32_t mqd_db_node_add(MQD_CB *pMqd, MQD_OBJ_NODE *pNode)
{
	/*m_HTON_SANAMET_LEN(pNode->oinfo.name.length); */
	pNode->node.key_info = (uint8_t *)&pNode->oinfo.name;
	return ncs_patricia_tree_add(&pMqd->qdb, (NCS_PATRICIA_NODE *)&pNode->node);
}	/* End of mqd_db_node_add() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_db_node_del

   DESCRIPTION    :  This routines deletes the Object node from the Tree, and 
                     free's all the resources.
                   
   ARGUMENTS      :  pMqd  - MQD Controll block pointer
                     pNode - Object Node 

   RETURNS        :  none
\****************************************************************************/
void mqd_db_node_del(MQD_CB *pMqd, MQD_OBJ_NODE *pNode)
{
	MQD_OBJECT_ELEM *pOelm = NULL;
	MQD_TRACK_OBJ *pObj = NULL;

	while ((pOelm = ncs_dequeue(&pNode->oinfo.ilist))) {
		m_MMGR_FREE_MQD_OBJECT_ELEM(pOelm);
	}

	while ((pObj = ncs_dequeue(&pNode->oinfo.tlist))) {
		m_MMGR_FREE_MQD_TRACK_OBJ(pObj);
	}

	/* Destroy the Object list & Track list */
	ncs_destroy_queue(&pNode->oinfo.ilist);
	ncs_destroy_queue(&pNode->oinfo.tlist);

	/* Remove the object node from the tree */
	ncs_patricia_tree_del(&pMqd->qdb, (NCS_PATRICIA_NODE *)&pNode->node);
	m_MMGR_FREE_MQD_OBJ_NODE(pNode);
}	/* End of mqd_db_node_del() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_db_node_create

   DESCRIPTION    :  This routines create the Object node from the Tree, and 
                     initializes all the resources.
                   
   ARGUMENTS      :  pMqd  - MQD Controll block pointer
                     pNode - Object Node 

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
uint32_t mqd_db_node_create(MQD_CB *pMqd, MQD_OBJ_NODE **o_pnode)
{
	MQD_OBJ_NODE *pNode = 0;
	TRACE_ENTER();

	pNode = m_MMGR_ALLOC_MQD_OBJ_NODE;
	if (!pNode) {
		LOG_CR("MQD_OBJ_NODE Memory Allocation failed");
		return NCSCC_RC_FAILURE;
	}
	memset(pNode, 0, sizeof(MQD_OBJ_NODE));

	/* Initialize the Queue/Group & Track List */
	ncs_create_queue(&pNode->oinfo.ilist);
	ncs_create_queue(&pNode->oinfo.tlist);

	*o_pnode = pNode;
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}	/* End of mqd_db_node_create() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_qparam_upd

   DESCRIPTION    :  This routines updates the queue parameter.
                   
   ARGUMENTS      :  pNode - Object Node
                     param - queue param

   RETURNS        :  none
\****************************************************************************/
void mqd_qparam_upd(MQD_OBJ_NODE *pNode, ASAPi_QUEUE_PARAM *qparam)
{
	/* Update the Queue params */
	pNode->oinfo.info.q.retentionTime = qparam->retentionTime;
	pNode->oinfo.info.q.send_state = qparam->status;
	pNode->oinfo.info.q.owner = qparam->owner;
	pNode->oinfo.info.q.dest = qparam->addr;
	pNode->oinfo.info.q.hdl = qparam->hdl;
	pNode->oinfo.info.q.creationFlags = qparam->creationFlags;
	memcpy(pNode->oinfo.info.q.size, qparam->size, sizeof(SaSizeT) * (SA_MSG_MESSAGE_LOWEST_PRIORITY + 1));
}	/* End of mqd_qparam_upd() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_qparam_fill

   DESCRIPTION    :  This routines fills the queue parameter.
                   
   ARGUMENTS      :  pParam - Param Node
                     o_param - queue param

   RETURNS        :  none
\****************************************************************************/
void mqd_qparam_fill(MQD_QUEUE_PARAM *pParam, ASAPi_QUEUE_PARAM *pQparam)
{
	/* Fill the Queue params */
	pQparam->retentionTime = pParam->retentionTime;
	pQparam->status = pParam->send_state;
	pQparam->owner = pParam->owner;
	pQparam->addr = pParam->dest;
	pQparam->hdl = pParam->hdl;
	pQparam->is_mqnd_down = pParam->is_mqnd_down;
	pQparam->creationFlags = pParam->creationFlags;
	memcpy(pQparam->size, pParam->size, sizeof(SaSizeT) * (SA_MSG_MESSAGE_LOWEST_PRIORITY + 1));

}	/* End of mqd_qparam_fill() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_track_add

   DESCRIPTION    :  This routines and an entry into the track list associated
                     with the object. 
                   
   ARGUMENTS      :  list - Track List
                     dest - destination value 

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
                     <ERR_CODE> Specific errors
\****************************************************************************/
uint32_t mqd_track_add(NCS_QUEUE *list, MDS_DEST *dest, MDS_SVC_ID svc)
{
	MQD_TRACK_OBJ *pObj = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* Check whether the destination object already exist */
	pObj = ncs_find_item(list, dest, mqd_track_obj_cmp);
	if (!pObj) {
		pObj = m_MMGR_ALLOC_MQD_TRACK_OBJ;
		if (!pObj) {
			LOG_CR("ERR_MEMORY: MQD_TRACK_OBJ Memory Allocation failed");
			return SA_AIS_ERR_NO_MEMORY;
		}

		pObj->dest = *dest;	/* Set the destination value */
		pObj->to_svc = svc;	/* Set the service id */

		/* Add the object to the list */
		ncs_enqueue(list, pObj);
	}
	return rc;
}	/* End of mqd_track_add() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_track_del

   DESCRIPTION    :  This routines delets an entry from the track list associated
                     with the object. 
                   
   ARGUMENTS      :  list - Track List
                     dest - destination value 

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
                     <ERR_CODE> Specific errors
\****************************************************************************/
uint32_t mqd_track_del(NCS_QUEUE *list, MDS_DEST *dest)
{
	MQD_TRACK_OBJ *pObj = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* Check whether the destination object already exist */
	pObj = ncs_remove_item(list, dest, mqd_track_obj_cmp);
	if (!pObj) {
		LOG_ER("ERR_NOT_EXIST: Track list associated with the object not found"); 
		return SA_AIS_ERR_NOT_EXIST;
	}

	m_MMGR_FREE_MQD_TRACK_OBJ(pObj);
	return rc;
}	/* End of mqd_track_del() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_track_obj_cmp

   DESCRIPTION    :  This routines is invoked to compare the MDS value of the 
                     track object in the list 
   
   ARGUMENTS      :  key   - what to match
                     elem  - with whom to match
   
   RETURNS        :  true(If sucessfully matched)/FALSE(No match)                     
\****************************************************************************/
static bool mqd_track_obj_cmp(void *key, void *elem)
{
	MQD_TRACK_OBJ *pObj = (MQD_TRACK_OBJ *)elem;

	if (!memcmp(&pObj->dest, (MDS_DEST *)key, sizeof(MDS_DEST))) {
		return true;
	}
	return false;
}	/* End of mqd_track_obj_cmp() */
#else				/* (NCS_MQD != 0) */
extern int dummy;

#endif
