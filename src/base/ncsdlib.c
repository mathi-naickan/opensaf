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
  FILE NAME: NCSDLIB.C

  DESCRIPTION: This module contains the doubly Link list library. Using this 
  library, we can perform Queue/Stack/Ordered List operations.

  FUNCTIONS INCLUDED in this module:  

  ncs_db_link_list_remove ......... Remove the list from DB.
  ncs_db_link_list_delink ......... Delink the node from DB.
  ncs_db_link_list_del ............ Delete DB node from the list.
  ncs_db_link_list_find ........... Find the node in the list.
  db_link_list_add_ascend ......... Add the node in the ascending order.
  db_link_list_insert_node ........ Insert the node in the list.
  db_link_list_add_descend ........ Add the node in the descending order.
  db_link_list_add_first .......... Add the node to the first of the list.
  ncs_db_link_list_add ............ Add the node to the list.
  ncs_db_link_list_find_next ...... Find the next element in the list
  ncs_db_link_list_find_prev ...... Find the prev element in the list
  ncs_db_link_list_enqeue ......... Enqueue the data.
  ncs_db_link_list_push ........... Push the data.
  ncs_db_link_list_dequeue ........ Dequeue the data.
  ncs_db_link_list_pop ............ POP the data.  

******************************************************************************/
#include "base/ncsdlib.h"

static uint32_t
db_link_list_insert_node(NCS_DB_LINK_LIST *list_ptr,
			 NCS_DB_LINK_LIST_NODE *add_node, NCS_DB_LINK_LIST_NODE *found_node);
/****************************************************************************
 * Name          : ncs_db_link_list_remove
 *
 * Description   : This function removes the node from the list. It dosen't 
 *                 free the node.
 *
 * Arguments     : list_ptr  - Doubly Linked List pointer.
 *                 key       - Key for which the node need to be removed.
 *
 * Return Values : NULL on failure/NCS_DB_LINK_LIST_NODE
 *
 * Notes         : None.
 *****************************************************************************/
NCS_DB_LINK_LIST_NODE *ncs_db_link_list_remove(NCS_DB_LINK_LIST *list_ptr, uint8_t *key)
{
	NCS_DB_LINK_LIST_NODE *del_node;

	del_node = ncs_db_link_list_find(list_ptr, key);

	if (del_node == NULL) {
		return (NULL);
	}

	if (ncs_db_link_list_delink(list_ptr, del_node) != NCSCC_RC_SUCCESS) {
		return (NULL);
	}

	return (del_node);
}

/****************************************************************************
 * Name          : ncs_db_link_list_delink
 *
 * Description   : This function delinks the node from the list. It dosen't 
 *                 free the node.
 *
 * Arguments     : list_ptr  - Doubly Linked List pointer.
 *                 del_node  - Node pointer which needs to be delinked from 
                               the list.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t ncs_db_link_list_delink(NCS_DB_LINK_LIST *list_ptr, NCS_DB_LINK_LIST_NODE *del_node)
{
	if (del_node == NULL) {
		return NCSCC_RC_FAILURE;
	}

	if (del_node->next) {
		if ((del_node->next->prev = del_node->prev) == NULL) {
			list_ptr->start_ptr = del_node->next;
		}
	} else {
		list_ptr->end_ptr = del_node->prev;
	}

	if (del_node->prev) {
		if ((del_node->prev->next = del_node->next) == NULL)
			list_ptr->end_ptr = del_node->prev;
	} else {
		list_ptr->start_ptr = del_node->next;
	}
	list_ptr->n_nodes--;

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ncs_db_link_list_del
 *
 * Description   : This function deletes the node from the list.
 *
 * Arguments     : list_ptr  - Doubly Linked List pointer.
 *                 key       - Key for which the node need to be deleted.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : This API will fail, if you fail to register the free 
 *                 callback function. If you just want to remove the node 
 *                 from the list then call the API "ncs_db_link_list_remove()".
 *****************************************************************************/
uint32_t ncs_db_link_list_del(NCS_DB_LINK_LIST *list_ptr, uint8_t *key)
{
	NCS_DB_LINK_LIST_NODE *del_node = NULL;

	if (list_ptr->free_cookie == NULL) {
		return (NCSCC_RC_FAILURE);
	}

	del_node = ncs_db_link_list_remove(list_ptr, key);

	if (del_node == NULL) {
		return (NCSCC_RC_FAILURE);
	}
	list_ptr->free_cookie(del_node);
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : ncs_db_link_list_find
 *
 * Description   : This function finds the node in the list for the given key.
 *
 * Arguments     : list_ptr  - Doubly Linked List pointer.
 *                 key       - Key for which the node need to be removed.
 *
 * Return Values : NULL on failure/NCS_DB_LINK_LIST_NODE
 *
 * Notes         : None.
 *****************************************************************************/
NCS_DB_LINK_LIST_NODE *ncs_db_link_list_find(NCS_DB_LINK_LIST *list_ptr, uint8_t *key)
{
	NCS_DB_LINK_LIST_NODE *start_ptr = list_ptr->start_ptr;

	if (list_ptr->cmp_cookie == NULL) {
		return (NULL);
	}

	while (start_ptr) {
		if (list_ptr->cmp_cookie((uint8_t *)start_ptr->key, key) == 0)
			break;
		start_ptr = start_ptr->next;
	}
	return (start_ptr);
}

/****************************************************************************
 * Name          : db_link_list_add_ascend
 *
 * Description   : This function adds the node in the ascending order.
 *
 * Arguments     : list_ptr  - Doubly Linked List pointer.
 *                 node_ptr  - Node pointer which needs to be added in the list.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t db_link_list_add_ascend(NCS_DB_LINK_LIST *list_ptr, NCS_DB_LINK_LIST_NODE *node_ptr)
{
	NCS_DB_LINK_LIST_NODE *start_ptr = list_ptr->start_ptr;

	if (list_ptr->cmp_cookie == NULL) {
		return (NCSCC_RC_FAILURE);
	}

	if (start_ptr == NULL) {
		/* This is the first node */
		list_ptr->start_ptr = node_ptr;
		list_ptr->end_ptr = node_ptr;
	} else {
		while (start_ptr && (list_ptr->cmp_cookie((uint8_t *)start_ptr->key, (uint8_t *)node_ptr->key) == 2)) {
			start_ptr = start_ptr->next;
		}

		db_link_list_insert_node(list_ptr, node_ptr, start_ptr);
	}
	list_ptr->n_nodes++;
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : db_link_list_insert_node
 *
 * Description   : This function insert a node in the list.
 *
 * Arguments     : list_ptr  - Doubly Linked List pointer.
 *                 add_node  - Node pointer which needs to be added in the list.
 *                 found_node- Present Node pointer found in the list.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t
db_link_list_insert_node(NCS_DB_LINK_LIST *list_ptr, NCS_DB_LINK_LIST_NODE *add_node, NCS_DB_LINK_LIST_NODE *found_node)
{
	if (found_node == NULL) {
		add_node->prev = list_ptr->end_ptr;
		if (list_ptr->end_ptr != NULL)
			list_ptr->end_ptr->next = add_node;
		list_ptr->end_ptr = add_node;

	} else {
		if (found_node == list_ptr->start_ptr) {
			/* First node has been altered */
			list_ptr->start_ptr = add_node;
		} else {
			/* This is the intermediate node, where previous pointer is not NULL */
			found_node->prev->next = add_node;
			add_node->prev = found_node->prev;
		}
		found_node->prev = add_node;
		add_node->next = found_node;
	}
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : db_link_list_add_descend
 *
 * Description   : This function adds the node in the descending order.
 *
 * Arguments     : list_ptr  - Doubly Linked List pointer.
 *                 node_ptr  - Node pointer which needs to be added in the list.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t db_link_list_add_descend(NCS_DB_LINK_LIST *list_ptr, NCS_DB_LINK_LIST_NODE *node_ptr)
{
	NCS_DB_LINK_LIST_NODE *start_ptr = list_ptr->start_ptr;

	if (list_ptr->cmp_cookie == NULL) {
		return (NCSCC_RC_FAILURE);
	}

	if (start_ptr == NULL) {
		/* This is the first node */
		list_ptr->start_ptr = node_ptr;
		list_ptr->end_ptr = node_ptr;
	} else {
		while (start_ptr && (list_ptr->cmp_cookie((uint8_t *)start_ptr->key, node_ptr->key) == 1)) {
			start_ptr = start_ptr->next;
		}

		db_link_list_insert_node(list_ptr, node_ptr, start_ptr);
	}
	list_ptr->n_nodes++;
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : db_link_list_add_first
 *
 * Description   : This function adds the node in the first of the list.
 *
 * Arguments     : list_ptr  - Doubly Linked List pointer.
 *                 node_ptr  - Node pointer which needs to be added in the list.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t db_link_list_add_first(NCS_DB_LINK_LIST *list_ptr, NCS_DB_LINK_LIST_NODE *node_ptr)
{
	if (list_ptr->start_ptr == NULL) {
		list_ptr->start_ptr = node_ptr;
		list_ptr->end_ptr = node_ptr;
	} else {
		list_ptr->start_ptr->prev = node_ptr;
		node_ptr->next = list_ptr->start_ptr;
		list_ptr->start_ptr = node_ptr;
	}
	list_ptr->n_nodes++;
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : ncs_db_link_list_add
 *
 * Description   : This function adds the node in the list according to the 
 *                 order which user specified during registeration.
 *
 * Arguments     : list_ptr  - Doubly Linked List pointer.
 *                 node_ptr  - Node pointer which needs to be added in the list.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t ncs_db_link_list_add(NCS_DB_LINK_LIST *list_ptr, NCS_DB_LINK_LIST_NODE *node_ptr)
{
	uint32_t res;

	if ((list_ptr == NULL) || (node_ptr == NULL)) {
		return (NCSCC_RC_FAILURE);
	}

	switch (list_ptr->order) {
	case NCS_DBLIST_ASSCEND_ORDER:
		res = db_link_list_add_ascend(list_ptr, node_ptr);
		break;

	case NCS_DBLIST_DESCEND_ORDER:
		res = db_link_list_add_descend(list_ptr, node_ptr);
		break;

	default:
		res = db_link_list_add_first(list_ptr, node_ptr);
		break;
	}
	return (res);
}

/****************************************************************************
 * Name          : ncs_db_link_list_find_next
 *
 * Description   : This function finds the next node for the given key.
 *
 * Arguments     : list_ptr  - Doubly Linked List pointer.
 *                 prev_key  - Previous Node pointer.
 *
 * Return Values : NULL on failure/NCS_DB_LINK_LIST_NODE
 *
 * Notes         : None.
 *****************************************************************************/
NCS_DB_LINK_LIST_NODE *ncs_db_link_list_find_next(NCS_DB_LINK_LIST *list_ptr, uint8_t *prev_key)
{
	NCS_DB_LINK_LIST_NODE *node_ptr;

	node_ptr = ncs_db_link_list_find(list_ptr, prev_key);
	if (node_ptr == NULL)
		return (NULL);
	return (m_NCS_DBLIST_FIND_NEXT(node_ptr));
}

/****************************************************************************
 * Name          : ncs_db_link_list_find_prev
 *
 * Description   : This function finds the previos node for the given key.
 *
 * Arguments     : list_ptr  - Doubly Linked List pointer.
 *                 pres_key  - Present Node pointer.
 *
 * Return Values : NULL on failure/NCS_DB_LINK_LIST_NODE
 *
 * Notes         : None.
 *****************************************************************************/
NCS_DB_LINK_LIST_NODE *ncs_db_link_list_find_prev(NCS_DB_LINK_LIST *list_ptr, uint8_t *pres_key)
{
	NCS_DB_LINK_LIST_NODE *node_ptr;

	node_ptr = ncs_db_link_list_find(list_ptr, pres_key);
	if (node_ptr == NULL)
		return (NULL);
	return (m_NCS_DBLIST_FIND_PREV(node_ptr));
}

/****************************************************************************
 * Name          : ncs_db_link_list_enqeue
 *
 * Description   : This function enqueue the node in the list.
 *
 * Arguments     : list_ptr  - Doubly Linked List pointer.
 *                 node_ptr  - Node pointer to be enqueued.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t ncs_db_link_list_enqeue(NCS_DB_LINK_LIST *list_ptr, NCS_DB_LINK_LIST_NODE *node_ptr)
{
	uint32_t res;

	res = db_link_list_add_first(list_ptr, node_ptr);

	return (res);
}

/****************************************************************************
 * Name          : ncs_db_link_list_push
 *
 * Description   : This function push the node in the list.
 *
 * Arguments     : list_ptr  - Doubly Linked List pointer.
 *                 node_ptr  - Node pointer to be enqueued.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t ncs_db_link_list_push(NCS_DB_LINK_LIST *list_ptr, NCS_DB_LINK_LIST_NODE *node_ptr)
{
	uint32_t res;

	res = db_link_list_add_first(list_ptr, node_ptr);

	return (res);
}

/****************************************************************************
 * Name          : ncs_db_link_list_dequeue
 *
 * Description   : This function dequeues the node from the list.
 *
 * Arguments     : list_ptr  - Doubly Linked List pointer. 
 *
 * Return Values : NULL on failure/NCS_DB_LINK_LIST_NODE
 *
 * Notes         : None.
 *****************************************************************************/
NCS_DB_LINK_LIST_NODE *ncs_db_link_list_dequeue(NCS_DB_LINK_LIST *list_ptr)
{
	NCS_DB_LINK_LIST_NODE *node_ptr = NULL;

	if (list_ptr->end_ptr != NULL) {
		node_ptr = ncs_db_link_list_remove(list_ptr, list_ptr->end_ptr->key);
	}
	return (node_ptr);
}

/****************************************************************************
 * Name          : ncs_db_link_list_pop
 *
 * Description   : This function POP the node from the list.
 *
 * Arguments     : list_ptr  - Doubly Linked List pointer. 
 *
 * Return Values : NULL on failure/NCS_DB_LINK_LIST_NODE
 *
 * Notes         : None.
 *****************************************************************************/
NCS_DB_LINK_LIST_NODE *ncs_db_link_list_pop(NCS_DB_LINK_LIST *list_ptr)
{
	NCS_DB_LINK_LIST_NODE *node_ptr = NULL;

	if (list_ptr->start_ptr != NULL) {
		node_ptr = ncs_db_link_list_remove(list_ptr, list_ptr->start_ptr->key);
	}
	return (node_ptr);
}
