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

  DESCRIPTION:

  This file contains functions which will search an MBCSv mailbox from the the 
  MBCSv - MDS registration list. It contains function for adding an entry to 
  this list and deleating entry from the list. The lock on this list should be 
  taken from this file only. The functions in this file
  are only used by mbcsv_mds.c. 

  FUNCTIONS INCLUDED in this module:

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/
#include "mbcsv.h"

typedef struct mbcsv_peer_list {
	NCS_PATRICIA_NODE pat_node;
	MBCSV_PEER_KEY key;
} MBCSV_PEER_LIST;

/*****************************************************************************\
*
*  PROCEDURE          :    mbcsv_add_new_pwe_anc
*
*  DESCRIPTION:       Add new entry into the list. 
*
*  RETURNS:           SUCCESS - All went well
*                     FAILURE - fail to add new entry.
*
*****************************************************************************/
uint32_t mbcsv_add_new_pwe_anc(uint32_t pwe_hdl, MBCSV_ANCHOR anchor)
{
	MBCSV_PEER_KEY key;
	MBCSV_PEER_LIST *new_entry;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&key, '\0', sizeof(MBCSV_PEER_KEY));

	key.pwe_hdl = pwe_hdl;
	key.anchor = anchor;

	m_NCS_LOCK(&mbcsv_cb.peer_list_lock, NCS_LOCK_WRITE);

	if (NULL != ncs_patricia_tree_get(&mbcsv_cb.peer_list, (const uint8_t *)&key)) {
 		TRACE_4("Unable to add new entry in the peer's list.");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	if (NULL == (new_entry = m_MMGR_ALLOC_PEER_LIST_IN)) {
 		TRACE_4("malloc failed");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	memset(new_entry, '\0', sizeof(MBCSV_PEER_LIST));

	new_entry->key.pwe_hdl = pwe_hdl;
	new_entry->key.anchor = anchor;
	new_entry->pat_node.key_info = (uint8_t *)&new_entry->key;

	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_add(&mbcsv_cb.peer_list, (NCS_PATRICIA_NODE *)new_entry)) {
		m_MMGR_FREE_PEER_LIST_IN(new_entry);
 		TRACE_4("pat node add failed");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

 done:
	m_NCS_UNLOCK(&mbcsv_cb.peer_list_lock, NCS_LOCK_WRITE);

	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************\
*
*  PROCEDURE          :    mbcsv_rmv_pwe_anc_entry
*
*
*  DESCRIPTION:       Remove entry from the list. 
*
*
*  RETURNS:           SUCCESS - All went well
*                     FAILURE - fail to add new entry.
*
*****************************************************************************/
uint32_t mbcsv_rmv_pwe_anc_entry(uint32_t pwe_hdl, MBCSV_ANCHOR anchor)
{
	MBCSV_PEER_KEY key;
	MBCSV_PEER_LIST *tree_entry;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&key, '\0', sizeof(MBCSV_PEER_KEY));

	key.pwe_hdl = pwe_hdl;
	key.anchor = anchor;

	m_NCS_LOCK(&mbcsv_cb.peer_list_lock, NCS_LOCK_WRITE);

	if (NULL == (tree_entry = (MBCSV_PEER_LIST *)ncs_patricia_tree_get(&mbcsv_cb.peer_list, (const uint8_t *)&key))) {
 		TRACE_4("Unable to remove entry from the peer list");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	ncs_patricia_tree_del(&mbcsv_cb.peer_list, (NCS_PATRICIA_NODE *)tree_entry);

	m_MMGR_FREE_PEER_LIST_IN(tree_entry);

 done:
	m_NCS_UNLOCK(&mbcsv_cb.peer_list_lock, NCS_LOCK_WRITE);

	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************\
*
*  PROCEDURE          :    mbcsv_initialize_peer_list
*
*
*  DESCRIPTION:       Create and initialize peer list 
*
*  RETURNS:           SUCCESS - All went well
*
*****************************************************************************/
uint32_t mbcsv_initialize_peer_list(void)
{
	NCS_PATRICIA_PARAMS pt_params;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* 
	 * Create patricia tree for the peer list 
	 */
	pt_params.key_size = sizeof(MBCSV_PEER_KEY);

	if (ncs_patricia_tree_init(&mbcsv_cb.peer_list, &pt_params) != NCSCC_RC_SUCCESS) {
 		TRACE_4("pat tree init failed");
		rc = NCSCC_RC_FAILURE;
	}

	m_NCS_LOCK_INIT(&mbcsv_cb.peer_list_lock);

	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************\
*
*  PROCEDURE          :    mbcsv_destroy_peer_list
*
*
*  DESCRIPTION:       Removes all the entries of the list and then destroy the
*                     peer entry list. 
*
*
*  RETURNS:           SUCCESS - All went well
*
*****************************************************************************/
uint32_t mbcsv_destroy_peer_list(void)
{
	MBCSV_PEER_KEY key;
	MBCSV_PEER_LIST *tree_entry;
	TRACE_ENTER();

	memset(&key, '\0', sizeof(MBCSV_PEER_KEY));

	key.pwe_hdl = 0;
	key.anchor = 0;

	while (NULL != (tree_entry =
			(MBCSV_PEER_LIST *)ncs_patricia_tree_getnext(&mbcsv_cb.peer_list, (const uint8_t *)&key))) {
		key = tree_entry->key;

		ncs_patricia_tree_del(&mbcsv_cb.peer_list, (NCS_PATRICIA_NODE *)tree_entry);

		m_MMGR_FREE_PEER_LIST_IN(tree_entry);
	}

	ncs_patricia_tree_destroy(&mbcsv_cb.peer_list);

	m_NCS_LOCK_DESTROY(&mbcsv_cb.peer_list_lock);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************\
*
*  PROCEDURE          :    mbcsv_get_next_anchor_for_pwe
*
*  DESCRIPTION:       Search the list for this PWE id and find the next anchor
*                     entry for this PWE.
*
*  RETURNS:           SUCCESS - All went well
*                     FAILURE - fail to get entry.
*
*****************************************************************************/
uint32_t mbcsv_get_next_anchor_for_pwe(uint32_t pwe_hdl, MBCSV_ANCHOR *anchor)
{
	MBCSV_PEER_KEY key;
	MBCSV_PEER_LIST *tree_entry;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&key, '\0', sizeof(MBCSV_PEER_KEY));

	key.pwe_hdl = pwe_hdl;
	key.anchor = *anchor;

	m_NCS_LOCK(&mbcsv_cb.peer_list_lock, NCS_LOCK_WRITE);

	if ((NULL == (tree_entry = (MBCSV_PEER_LIST *)ncs_patricia_tree_getnext(&mbcsv_cb.peer_list,
										(const uint8_t *)&key)))
	    || (tree_entry->key.pwe_hdl != pwe_hdl)) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	*anchor = tree_entry->key.anchor;

 done:
	m_NCS_UNLOCK(&mbcsv_cb.peer_list_lock, NCS_LOCK_READ);

	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************\
*
*  PROCEDURE          :    mbcsv_send_brodcast_msg
*
*  DESCRIPTION:       Search the list for this PWE id and find the next anchor
*                     entry for this PWE. And now send the messages to all the 
*                     anchors of this PWE.
*
*  RETURNS:           SUCCESS - All went well
*                     FAILURE - fail to get entry.
*
*****************************************************************************/
uint32_t mbcsv_send_brodcast_msg(uint32_t pwe_hdl, MBCSV_EVT *msg, CKPT_INST *ckpt)
{
	MBCSV_ANCHOR anchor = 0;
	TRACE_ENTER();

	while (NCSCC_RC_SUCCESS == mbcsv_get_next_anchor_for_pwe(pwe_hdl, &anchor)) {
		if (NCSCC_RC_SUCCESS != m_NCS_MBCSV_MDS_ASYNC_SEND(msg, ckpt, anchor)) {
			TRACE_4("Message brodcast failed");
		}
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************\
*
*  PROCEDURE          :    mbcsv_rmv_ancs_for_pwe
*
*  DESCRIPTION:       Search the list for this PWE id and find the next anchor
*                     entry for this PWE. Remove all the anchor entries of this
*                     pwe.
*
*  RETURNS:           SUCCESS - All went well
*                     FAILURE - fail to get entry.
*
*****************************************************************************/
uint32_t mbcsv_rmv_ancs_for_pwe(uint32_t pwe_hdl)
{
	MBCSV_ANCHOR anchor = 0;
	MBCSV_PEER_LIST *tree_entry;
	MBCSV_PEER_KEY key;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&key, '\0', sizeof(MBCSV_PEER_KEY));

	key.pwe_hdl = pwe_hdl;

	m_NCS_LOCK(&mbcsv_cb.peer_list_lock, NCS_LOCK_WRITE);

	while (NCSCC_RC_SUCCESS == mbcsv_get_next_anchor_for_pwe(pwe_hdl, &anchor)) {
		key.anchor = anchor;

		if (NULL == (tree_entry = (MBCSV_PEER_LIST *)ncs_patricia_tree_get(&mbcsv_cb.peer_list,
										   (const uint8_t *)&key))) {
 			TRACE_4("Unable to remove entry from the peer list");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

		ncs_patricia_tree_del(&mbcsv_cb.peer_list, (NCS_PATRICIA_NODE *)tree_entry);

		m_MMGR_FREE_PEER_LIST_IN(tree_entry);
	}

 done:
	m_NCS_UNLOCK(&mbcsv_cb.peer_list_lock, NCS_LOCK_WRITE);
	TRACE_LEAVE();
	return rc;
}
