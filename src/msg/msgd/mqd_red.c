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
   
   mqd_red_db_node_add......................Routine to add DB node
   mqd_red_db_node_del......................Routine to del DB node
   mqd_red_db_node_create...................Routine to create DB node
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#include "msg/msgd/mqd.h"

#ifndef NCS_MQD_RED
#define NCS_MQD_RED

/******************************** LOCAL ROUTINES *****************************/
/*****************************************************************************/

/****************************************************************************\
   PROCEDURE NAME :  mqd_red_db_node_add

   DESCRIPTION    :  This routines adds the Object node into the Tree
                   
   ARGUMENTS      :  pMqd  - MQD Controll block pointer
                     pNode - Nodeinfo Node 

   RETURNS        :  SUCCESS - All went well
                     FAILURE - internal processing didn't like something.
\****************************************************************************/
uint32_t mqd_red_db_node_add(MQD_CB *pMqd, MQD_ND_DB_NODE *pNode)
{
	/*m_HTON_SANAMET_LEN(pNode->info.nodeid); */
	pNode->node.key_info = (uint8_t *)&pNode->info.nodeid;
	return ncs_patricia_tree_add(&pMqd->node_db, (NCS_PATRICIA_NODE *)&pNode->node);
}	/* End of mqd_red_db_node_add() */

/****************************************************************************\
   PROCEDURE NAME :  mqd_red_db_node_del

   DESCRIPTION    :  This routines deletes the Object node from the Tree, and 
                     free's all the resources.
                   
   ARGUMENTS      :  pMqd  - MQD Controll block pointer
                     pNode - Object Node 

   RETURNS        :  none
\****************************************************************************/
void mqd_red_db_node_del(MQD_CB *pMqd, MQD_ND_DB_NODE *pNode)
{
	/* Remove the object node from the tree */
	ncs_patricia_tree_del(&pMqd->node_db, (NCS_PATRICIA_NODE *)&pNode->node);
	m_MMGR_FREE_MQD_ND_DB_NODE(pNode);
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
uint32_t mqd_red_db_node_create(MQD_CB *pMqd, MQD_ND_DB_NODE **o_pnode)
{
	MQD_ND_DB_NODE *pNode = 0;

	pNode = m_MMGR_ALLOC_MQD_ND_DB_NODE;
	if (!pNode) {
		LOG_CR("%s:%u: Failed To Allocate Memory", __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
	memset(pNode, 0, sizeof(MQD_ND_DB_NODE));

	*o_pnode = pNode;
	return NCSCC_RC_SUCCESS;
}	/* End of mqd_red_db_node_create() */

#endif
