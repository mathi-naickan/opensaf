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
  this list and deleting entry from the list. The lock on this list should be 
  taken from this file only.

  ..............................................................................

  FUNCTIONS INCLUDED in this module:
   mbcsv_add_new_mbx             - Add new entry in the list.
   mbcsv_rmv_entry               - Remove entry from the list.
   mbcsv_get_mbx                 - Get mbx from the list.
   mbcsv_get_next_entry_for_pwe  - Get next mbx entry for this pwe.
   mbcsv_initialize_mbx_list     - Initialize mbx list.
   mbcsv_destroy_mbx_list        - Destroy mbx list.

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/
#include "mbcsv.h"

typedef struct {
	uint32_t pwe_hdl;		/* Handle supplied by application with OPEN call */
	SS_SVC_ID svc_id;
} MBCSV_MBX_KEY;

typedef struct mbcsv_mbx_info {
	NCS_PATRICIA_NODE pat_node;
	MBCSV_MBX_KEY key;
	SYSF_MBX mbx;

} MBCSV_MBX_INFO;

/*****************************************************************************\
*
*  PROCEDURE          :    mbcsv_add_new_entry
*
*  DESCRIPTION:       Add new entry into the list.
*
*  RETURNS:           SUCCESS - All went well
*                     FAILURE - fail to add new entry. 
*
*****************************************************************************/
uint32_t mbcsv_add_new_mbx(uint32_t pwe_hdl, SS_SVC_ID svc_id, SYSF_MBX mbx)
{
	MBCSV_MBX_KEY key;
	MBCSV_MBX_INFO *new_entry;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&key, '\0', sizeof(MBCSV_MBX_KEY));

	key.pwe_hdl = pwe_hdl;
	key.svc_id = svc_id;

	m_NCS_LOCK(&mbcsv_cb.mbx_list_lock, NCS_LOCK_WRITE);

	if (NULL != ncs_patricia_tree_get(&mbcsv_cb.mbx_list, (const uint8_t *)&key)) {
 		TRACE_4("Unable to add new mailbox entry for service:%u", svc_id);
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	if (NULL == (new_entry = m_MMGR_ALLOC_MBX_INFO)) {
 		TRACE_4("malloc failed");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	memset(new_entry, '\0', sizeof(MBCSV_MBX_INFO));

	new_entry->key.pwe_hdl = pwe_hdl;
	new_entry->key.svc_id = svc_id;
	new_entry->pat_node.key_info = (uint8_t *)&new_entry->key;
	new_entry->mbx = mbx;

	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_add(&mbcsv_cb.mbx_list, (NCS_PATRICIA_NODE *)new_entry)) {
		m_MMGR_FREE_MBX_INFO(new_entry);
 		TRACE_4("Failed to add new mbx in tree");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

 done:
	m_NCS_UNLOCK(&mbcsv_cb.mbx_list_lock, NCS_LOCK_WRITE);

	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************\
*
*  PROCEDURE          :    mbcsv_rmv_entry
*
*
*  DESCRIPTION:       Remove entry from the list. 
*
*
*  RETURNS:           SUCCESS - All went well
*                     FAILURE - fail to add new entry. 
*
*****************************************************************************/
uint32_t mbcsv_rmv_entry(uint32_t pwe_hdl, SS_SVC_ID svc_id)
{
	MBCSV_MBX_KEY key;
	MBCSV_MBX_INFO *tree_entry;
	TRACE_ENTER();

	memset(&key, '\0', sizeof(MBCSV_MBX_KEY));

	key.pwe_hdl = pwe_hdl;
	key.svc_id = svc_id;

	m_NCS_LOCK(&mbcsv_cb.mbx_list_lock, NCS_LOCK_WRITE);

	if (NULL == (tree_entry = (MBCSV_MBX_INFO *)ncs_patricia_tree_get(&mbcsv_cb.mbx_list, (const uint8_t *)&key))) {
		TRACE("Mailbox entry for this pwe and service ID:%u does not exist", svc_id);
		goto done;
	}

	ncs_patricia_tree_del(&mbcsv_cb.mbx_list, (NCS_PATRICIA_NODE *)tree_entry);

	m_MMGR_FREE_MBX_INFO(tree_entry);

 done:
	m_NCS_UNLOCK(&mbcsv_cb.mbx_list_lock, NCS_LOCK_WRITE);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************\
*
*  PROCEDURE          :    mbcsv_initialize_mbx_list
*
*
*  DESCRIPTION:       Create and initialize mailbox list 
*
*  RETURNS:           SUCCESS - All went well
*
*****************************************************************************/
uint32_t mbcsv_initialize_mbx_list(void)
{
	NCS_PATRICIA_PARAMS pt_params;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* 
	 * Create patricia tree for the mail boxes 
	 */
	pt_params.key_size = sizeof(MBCSV_MBX_KEY);

	if (ncs_patricia_tree_init(&mbcsv_cb.mbx_list, &pt_params) != NCSCC_RC_SUCCESS) {
 		TRACE_4("pat tree init failed.");
		rc = NCSCC_RC_FAILURE;
	}

	m_NCS_LOCK_INIT(&mbcsv_cb.mbx_list_lock);

	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************\
*
*  PROCEDURE          :    mbcsv_destroy_mbx_list
*
*
*  DESCRIPTION:       Remove all the entries of this list and then destroy the list.. 
*
*
*  RETURNS:           SUCCESS - All went well
*
*****************************************************************************/
uint32_t mbcsv_destroy_mbx_list(void)
{
	MBCSV_MBX_KEY key;
	MBCSV_MBX_INFO *tree_entry;
	TRACE_ENTER();

	memset(&key, '\0', sizeof(MBCSV_MBX_KEY));

	key.pwe_hdl = 0;
	key.svc_id = 0;

	while (NULL != (tree_entry = (MBCSV_MBX_INFO *)ncs_patricia_tree_get(&mbcsv_cb.mbx_list, (const uint8_t *)&key))) {
		key = tree_entry->key;

		ncs_patricia_tree_del(&mbcsv_cb.mbx_list, (NCS_PATRICIA_NODE *)tree_entry);

		m_MMGR_FREE_MBX_INFO(tree_entry);
	}

	ncs_patricia_tree_destroy(&mbcsv_cb.mbx_list);

	m_NCS_LOCK_DESTROY(&mbcsv_cb.mbx_list_lock);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************\
*
*  PROCEDURE          :    mbcsv_get_mbx
*
*  DESCRIPTION:       Search the list for this PWE id and service ID and get
*                     entry from the list. 
*
*  RETURNS:           mbx - mailbox or NULL if unable to find the entry.
*
*****************************************************************************/
SYSF_MBX mbcsv_get_mbx(uint32_t pwe_hdl, SS_SVC_ID svc_id)
{
	SYSF_MBX mbx;
	MBCSV_MBX_KEY key;
	MBCSV_MBX_INFO *tree_entry;

	memset(&key, '\0', sizeof(MBCSV_MBX_KEY));

	key.pwe_hdl = pwe_hdl;
	key.svc_id = svc_id;

	m_NCS_LOCK(&mbcsv_cb.mbx_list_lock, NCS_LOCK_READ);

	if (NULL == (tree_entry = (MBCSV_MBX_INFO *)ncs_patricia_tree_get(&mbcsv_cb.mbx_list, (const uint8_t *)&key))) {

		m_NCS_UNLOCK(&mbcsv_cb.mbx_list_lock, NCS_LOCK_READ);
		TRACE_LEAVE2("Mailbox entry for this pwe and service ID:%u does not exist", svc_id);
		return 0;
	}

	mbx = tree_entry->mbx;

	m_NCS_UNLOCK(&mbcsv_cb.mbx_list_lock, NCS_LOCK_READ);

	return mbx;
}

/*****************************************************************************\
*
*  PROCEDURE          :    mbcsv_get_next_entry_for_pwe
*
*  DESCRIPTION:       Search the list for this PWE id and find the next service
*                     entry for this PWE. This is used for posting the MDS up/down
*                     event to all the services registered on the VDEST.
*
*  INPUT              pwe_hdl - PWE handle for which the next entry to be searched.
*                     svc_id - Service ID of the service and this function will 
*                              return next service id in the list for this PWE.
*
*  RETURNS:           mbx - mailbox or NULL if unable to find the next entry with 
*                           same pwe_hdl.
*
*****************************************************************************/
SYSF_MBX mbcsv_get_next_entry_for_pwe(uint32_t pwe_hdl, SS_SVC_ID *svc_id)
{
	SYSF_MBX mbx;
	MBCSV_MBX_KEY key;
	MBCSV_MBX_INFO *tree_entry;

	memset(&key, '\0', sizeof(MBCSV_MBX_KEY));

	key.pwe_hdl = pwe_hdl;
	key.svc_id = *svc_id;

	m_NCS_LOCK(&mbcsv_cb.mbx_list_lock, NCS_LOCK_READ);

	if ((NULL == (tree_entry = (MBCSV_MBX_INFO *)ncs_patricia_tree_getnext(&mbcsv_cb.mbx_list,
									       (const uint8_t *)&key)))
	    || (tree_entry->key.pwe_hdl != pwe_hdl)) {
		m_NCS_UNLOCK(&mbcsv_cb.mbx_list_lock, NCS_LOCK_READ);
		TRACE_LEAVE();
		return 0;
	}

	mbx = tree_entry->mbx;
	*svc_id = tree_entry->key.svc_id;

	m_NCS_UNLOCK(&mbcsv_cb.mbx_list_lock, NCS_LOCK_READ);

	return mbx;
}
