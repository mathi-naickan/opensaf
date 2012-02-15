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
*                                                                            *
*  MODULE NAME:  eds_ll.c                                                    *
*                                                                            *
*  DESCRIPTION:                                                              *
*  This module contains linked list routines for the NCS                     *
*  Event Distribution Service Server (EDS).                                  *
*                                                                            *
*****************************************************************************/
#include "eds.h"
#include "logtrace.h"

/****************************************************************************
 *
 * The EDS incorporates 2 groups of lists to achieve its work.
 *
 * The first group is called the workList.
 * The worklist is a doubly linked list ordered by channel id's.
 * Under each worklist structure are all the open channel records for that
 * channel ID. These are contained in a Patricia Tree key'd by the
 * chan_open_id. Under the CHAN_OPEN_REC is a linked list of all the
 * subscriptions for that chan_open_id.
 *
 * Also contained under the worklist is a linked list of retained events
 * for this channel.
 *
 *
 *                          W O R K L I S T
 *
 *
 *                           EDS_WORKLIST
 *                    +------------------------+
 *                    | chan_id                |
 *                    | chan_attrib            |
 *                    | use_cnt                |
 *                    | cname_len              |
 *                    | chan_name *            |
 *                    | NCS_PATRICIA_TREE      |
 *       -------------|  (chan_open_rec)       |
 *      /             | EDS_RETAINED_EVT_REC * |---
 *     |              | prev *                 |   \
 *     |              | next *                 |   |
 *     |              +------------------------+   |
 *     |                                           |
 *     |                                           |
 *     v    CHAN_OPEN_REC                          |    EDS_RETAINED_EVT_REC
 *     +----------------------+                    \->+---------------------+
 *     | NCS_PATRICIA_NODE    |                       | event_id            |
 *     | reg_id               |                       | retd_evt_hdl        |
 *     | chan_id              |                       | priority            |
 *     | chan_open_id         |                       | retentionTime       |
 *     | copen_id_Net         |                       | publishTime         |
 *     | MDS_DEST             |                       | publisherName       |
 *     | SUBSC_REC *          |----------             | patternArray *      |
 *     +----------------------+          \            | data_len            |
 *                                        \           | data *              |
 *                                         |          | retd_chan_open_id   |
 *                           SUBSC_REC     v          | reg_id              |
 *                       +-----------------+          | chan_id             |
 *                       | subscript_id    |          | EDS_TMR             |
 *                       | chan_id         |          | next *              |
 *                       | chan_open_id    |          +---------------------+
 *                       | FilterArray *   |
 *                       | EDA_REG_LIST *  |
 *                       | CHAN_OPEN_REC * |
 *                       | prev *          |
 *                       | next *          |
 *                       +-----------------+
 *                              
 *
 *
 * The second pair of linked lists are primarily a cache of links to
 * subscriptions so that they may be removed quickly (without an exhaustive
 * search of the workList) after an unsubscribe, unregister, or if a process
 * went away unexpectedly. It is ordered by reg_id, and contains all of the
 * open channels and subscriptions for that registration ID.
 *
 * The SUBSC_LIST is simply an encapulation of the subrec pointer address
 * of a SUBSC_REC entry in the workList.
 *
 *
 *                           R E G L I S T
 *         EDS_CB
 *  +-------------------+
 *  | ...               |
 *  | NCS_PATRICIA_TREE |----
 *  |  (eda_reg_list)   |    \
 *  | ...               |     \
 *  +-------------------+      \
 *                             |
 *                             |
 *              EDA_REG_REC    v
 *         +-------------------+
 *         | NCS_PATRICIA_NODE |
 *         | reg_id            |
 *         | reg_id_Net        |
 *         | MDS_DEST          |    CHAN_OPEN_LIST           SUBSC_LIST
 *         | CHAN_OPEN_LIST *  |-->+---------------+     ->+-------------+
 *         +-------------------+   | reg_id        |    /  | SUBSC_REC * |
 *                                 | chan_id       |   /   | next *      |
 *                                 | chan_open_id  |  /    +-------------+
 *                                 | SUBSC_LIST *  |--
 *                                 | next *        |
 *                                 +---------------+
 *
 *
 *
 ***************************************************************************/
static SaAisErrorT create_runtime_object(char *cname, SaTimeT create_time, SaImmOiHandleT immOiHandle)
{
	char *dndup = strdup(cname);
	char *parent_name = strchr(cname, ',');
	char *rdnstr;
	SaNameT parentName;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("channel Name: %s", cname);

	memset(&parentName, 0, sizeof(parentName));
	if (parent_name != NULL) {
		rdnstr = strtok(dndup, ",");
		parent_name++;
		strcpy((char *)parentName.value, parent_name);
		parentName.length = strlen((char *)parent_name);
	} else
		rdnstr = cname;
	void *arr1[] = { &rdnstr };
	const SaImmAttrValuesT_2 attr_safChnl = {
		.attrName = "safChnl",
		.attrValueType = SA_IMM_ATTR_SASTRINGT,
		.attrValuesNumber = 1,
		.attrValues = arr1
	};
	void *arr2[] = { &create_time };

	const SaImmAttrValuesT_2 attr_saEvtChannelCreationTimeStamp = {
		.attrName = "saEvtChannelCreationTimestamp",
		.attrValueType = SA_IMM_ATTR_SATIMET,
		.attrValuesNumber = 1,
		.attrValues = arr2
	};

	const SaImmAttrValuesT_2 *attrValues[] = {
		&attr_safChnl,
		&attr_saEvtChannelCreationTimeStamp,
		NULL
	};

	if ((rc = immutil_saImmOiRtObjectCreate_2(immOiHandle, "SaEvtChannel", &parentName, attrValues)) != SA_AIS_OK)
		LOG_ER("saImmOiRtObjectCreate_2 failed. channel: %s. rc = %d", cname, rc);
	free(dndup);

	TRACE_LEAVE2("retval: %d", rc);
	return rc;

}	/* End create_runtime_object() */

/****************************************************************************
 *
 * eds_add_subrec_entry() - Appends a subRec entry to end of the subRec list.
 *
 * Returns NCSCC_RC_BAD_ATTR if either pointer argument is NULL.
 *
 ***************************************************************************/
static uint32_t eds_add_subrec_entry(CHAN_OPEN_REC *copen_rec, SUBSC_REC *subrec)
{
	TRACE_ENTER2("Adding subscription record");

	/* Sanity check */
	if ((copen_rec == NULL) || (subrec == NULL)
	    ) {
		TRACE_LEAVE2("Input is NULL");
		return (NCSCC_RC_BAD_ATTR);
	}
	TRACE("reg_id: %u, chan_id: %u, chan_open_id: %u, subscription id: %u", copen_rec->reg_id, copen_rec->chan_id,
								 copen_rec->chan_open_id, subrec->subscript_id);
	if (copen_rec->subsc_rec_head == NULL)
		copen_rec->subsc_rec_head = subrec;	/*If this is the first record */
	else {
		copen_rec->subsc_rec_tail->next = subrec;	/* Append at the end */
		subrec->prev = copen_rec->subsc_rec_tail;
	}

	copen_rec->subsc_rec_tail = subrec;

	TRACE_LEAVE();
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 *
 * eds_remove_subrec_entry() - Removes a subRec entry from the subRec list.
 *
 * Returns NCSCC_RC_BAD_ATTR if pointer argument is NULL.
 *
 ***************************************************************************/
static void eds_remove_subrec_entry(EDS_CB *cb, SUBSC_REC **subrec)
{
	SUBSC_REC *p;
	TRACE_ENTER2("Removing subscription entry");

	/* Sanity check */
	if (subrec == NULL) {
		TRACE_LEAVE2("Subscription record is NULL");
		return;
	}

	p = (SUBSC_REC *)*subrec;

	TRACE("chan_id: %u, chan_open_id: %u, subscription id: %u", p->chan_id,
								 p->chan_open_id, p->subscript_id);

	if (p->prev == NULL) {	/* Top entry */
		if (p->next != NULL) {	/* It's not the only element */
			p->next->prev = NULL;	/* Clear prev pointer */
			/* Change root subRec address for this workList element */
			(*subrec)->par_chan_open_inst->subsc_rec_head = p->next;
			edsv_free_evt_filter_array((*subrec)->filters);	/* Free the filters */
			m_MMGR_FREE_EDS_SUBREC(*subrec);	/* free 1st cell */
			*subrec = NULL;
		} else {	/* Removing the only element */

			/* NULL root pointer to subsc_rec linked list */
			(*subrec)->par_chan_open_inst->subsc_rec_head = NULL;
			(*subrec)->par_chan_open_inst->subsc_rec_tail = NULL;
			edsv_free_evt_filter_array((*subrec)->filters);	/* Free the filters */
			m_MMGR_FREE_EDS_SUBREC(*subrec);	/* free 1st cell */
			*subrec = NULL;
		}
	} else if (p->next == NULL) {	/* Removing last element */
		p->prev->next = NULL;	/* Clear next ptr for new last element */
		(*subrec)->par_chan_open_inst->subsc_rec_tail = p->prev;
		edsv_free_evt_filter_array((*subrec)->filters);	/* Free the filters */
		m_MMGR_FREE_EDS_SUBREC(*subrec);	/* free last cell */
		*subrec = NULL;
	} else {		/* All other cases */

		p->next->prev = p->prev;	/* Link previous cell to next one */
		p->prev->next = p->next;	/* Back link next cell to previous */
		edsv_free_evt_filter_array((*subrec)->filters);	/* Free the filters */
		m_MMGR_FREE_EDS_SUBREC(*subrec);	/* free this element */
		*subrec = NULL;
	}

	TRACE_LEAVE();
}

/****************************************************************************
 *
 * eds_add_subscription_to_worklist()
 *
 * Adds a subscription record to the worklist.
 *
 * Search the worklist for a chan_id and chan_open_id matching the one passed
 * in and add the subscription record to the subRec for that channel.
 *
 ***************************************************************************/
static uint32_t eds_add_subscription_to_worklist(EDS_CB *cb, SUBSC_REC *subrec)
{
	uint32_t rs = NCSCC_RC_SUCCESS;
	uint32_t copen_id_Net;
	CHAN_OPEN_REC *co;
	EDS_WORKLIST *wp;
	TRACE_ENTER();

	/* Point to root of worklist */
	wp = cb->eds_work_list;

	/*
	 * Traverse the worklist looking for the correct chan_id.
	 */
	while (wp) {		/* While there are channels... */
		/* Find the chan_id we want */
		if (wp->chan_id == subrec->chan_id) {
			/* Get the chan open rec from the patricia tree */
			copen_id_Net = m_NCS_OS_HTONL(subrec->chan_open_id);
			if (NULL == (co = (CHAN_OPEN_REC *)ncs_patricia_tree_get(&wp->chan_open_rec,
										 (uint8_t *)&copen_id_Net))) {
				TRACE_LEAVE2("channel open record not found: %u", subrec->chan_open_id);
				return NCSCC_RC_FAILURE;
			}

			/* Make sure this is the correct reg_id */
			if (co->reg_id != subrec->reg_list->reg_id) {
				TRACE_LEAVE2("reg id not found: %u", subrec->reg_list->reg_id);
				return (NCSCC_RC_BAD_ATTR);
			}
			/* Set parent chan_open_rec pointer so we can remove root entry later */
			subrec->par_chan_open_inst = co;

			/* Add it! */
			rs = eds_add_subrec_entry(co, subrec);
			TRACE_LEAVE();
			return (rs);
		}
		wp = wp->next;
	}

	LOG_IN("channel id: %u not found in channel worklist", subrec->chan_id);
	TRACE_LEAVE();	
	return (NCSCC_RC_NO_OBJECT);	/* Went through the entire list. Channel not found. */
}

/****************************************************************************
 *
 * eds_add_subscription_to_reglist()
 *
 * Creates a subList record for this subscription in the regList.
 *
 ***************************************************************************/
static uint32_t eds_add_subscription_to_reglist(EDS_CB *cb, uint32_t reg_id, SUBSC_REC *subrec)
{
	EDA_REG_REC *reglist = NULL;
	SUBSC_LIST *sublist = NULL;
	CHAN_OPEN_LIST *cl;
	TRACE_ENTER2("reg_id: %u, chan_id:%u, chan_open_id:%u", reg_id, subrec->chan_id, subrec->chan_open_id);

	reglist = eds_get_reglist_entry(cb, reg_id);
	if (reglist == NULL) {
		TRACE_LEAVE2("regid not found in the reglist");
		return (NCSCC_RC_NO_OBJECT);
	}
	/* Get pointer to start of channelOpen list */
	cl = reglist->chan_open_list;
	if (cl == NULL) {
		TRACE_LEAVE2("channel open list is empty");
		return (NCSCC_RC_BAD_ATTR);
	}

	/* Find matching channel entry */
	while (cl) {
		if (cl->chan_id == subrec->chan_id)
			if (cl->chan_open_id == subrec->chan_open_id) {
				TRACE("chan open record found");
				sublist = m_MMGR_ALLOC_EDS_SUBLIST(sizeof(SUBSC_LIST));
				if (!sublist) {
					LOG_CR("malloc failed for subscription list");
					TRACE_LEAVE();
					return (NCSCC_RC_OUT_OF_MEM);
				}
				memset(sublist, 0, sizeof(SUBSC_LIST));
				sublist->subsc_rec = subrec;

				if (cl->subsc_list_head == NULL)
					cl->subsc_list_head = sublist;	/*If this is the first record */
				else {
					cl->subsc_list_tail->next = sublist;	/* Append at the end */
				}

				cl->subsc_list_tail = sublist;

				break;
			}
		cl = cl->next;
	}

	TRACE_LEAVE();
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 *
 * eds_remove_cname_rec - Deletes a channelname record from the channelname list.
 *
 * Removes a EDS_CNAME_REC from the channelname list .
 *
 ***************************************************************************/
static uint32_t eds_remove_cname_rec(EDS_CB *cb, EDS_WORKLIST *wp)
{
	uint32_t rc;
	EDS_CNAME_REC *rec_to_del;
	SaNameT chan_name_del;
	TRACE_ENTER2("chan_name: %s", wp->cname);

	memset(&chan_name_del, 0, sizeof(SaNameT));
	chan_name_del.length = m_NCS_OS_HTONS(wp->cname_len);
	memcpy(chan_name_del.value, wp->cname, wp->cname_len);

	/* Get the record pointer from the patricia tree */
	if (NULL == (rec_to_del = (EDS_CNAME_REC *)ncs_patricia_tree_get(&cb->eds_cname_list, (uint8_t *)&chan_name_del))) {
		TRACE_LEAVE2("Pat node get failed");
		return (NCSCC_RC_FAILURE);
	}

	/* Delete this record from the tree */
	if (NCSCC_RC_SUCCESS != (rc = ncs_patricia_tree_del(&cb->eds_cname_list, &rec_to_del->pat_node))) {
		TRACE_LEAVE2("Pat node delete failed");
		return (NCSCC_RC_FAILURE);
	}

	/* Free allocated memory and decrement the use counter */
	m_MMGR_FREE_EDS_CNAME_REC(rec_to_del);

	TRACE_LEAVE();
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 *
 * eds_remove_worklist_entry - Removes a workList entry.
 *
 ***************************************************************************/
uint32_t eds_remove_worklist_entry(EDS_CB *cb, uint32_t chan_id)
{
	EDS_WORKLIST *wp = NULL;
	EDS_WORKLIST *save_next;
	TRACE_ENTER2("chan_id: %u", chan_id);

	wp = cb->eds_work_list;

	while (wp) {
		if (wp->chan_id == chan_id) {	/* Found the one we want? */
			TRACE("chan record for :%s, found in worklist", wp->cname);
			/*This functionality is there here before the Bugfix:61494 - 
			   Now unlinked channels will be deleted from the database */
			/*  eds_remove_cname_rec(cb,wp); */	/* remove the entry from the channel name database */

			if (wp->prev == NULL) {	/* Removing 1st element */
				if (wp->next != NULL) {	/* It's not the only element */
					wp->next->prev = NULL;	/* Clear back ptr for new 1st element */
					save_next = wp->next;	/* Save new 1st ptr */
				} else {	/* Removing the only element */

					save_next = NULL;	/* Return a NULL ptr to the list */
				}

				m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				/* Make sure all retained events have been removed */
				eds_remove_retained_events(wp->ret_evt_list_head, wp->ret_evt_list_tail);
				/* Destroy the patricia tree for channel open recs */
				ncs_patricia_tree_destroy(&wp->chan_open_rec);
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				m_MMGR_FREE_EDS_CHAN_NAME(wp->cname);	/* free channelName */
				m_MMGR_FREE_EDS_WORKLIST(cb->eds_work_list);	/* free 1st cell */
				cb->eds_work_list = save_next;	/* Set new 1st element address */
				TRACE_LEAVE();
				return (NCSCC_RC_SUCCESS);
			} else if (wp->next == NULL) {	/* Removing last element */
				wp->prev->next = NULL;	/* Clear next ptr for new last element */
				m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				/* Make sure all retained events have been removed */
				eds_remove_retained_events(wp->ret_evt_list_head, wp->ret_evt_list_tail);
				/* Destroy the patricia tree for channel open recs */
				ncs_patricia_tree_destroy(&wp->chan_open_rec);
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				m_MMGR_FREE_EDS_CHAN_NAME(wp->cname);
				m_MMGR_FREE_EDS_WORKLIST(wp);	/* free Last cell */
				TRACE_LEAVE();
				return (NCSCC_RC_SUCCESS);
			} else {	/* All other cases */

				wp->next->prev = wp->prev;	/* Link previous cell to next one */
				wp->prev->next = wp->next;	/* Back link next cell to previous */
				m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				/* Make sure all retained events have been removed */
				eds_remove_retained_events(wp->ret_evt_list_head, wp->ret_evt_list_tail);
				/* Destroy the patricia tree for channel open recs */
				ncs_patricia_tree_destroy(&wp->chan_open_rec);
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				m_MMGR_FREE_EDS_CHAN_NAME(wp->cname);
				m_MMGR_FREE_EDS_WORKLIST(wp);	/* free the cell */
				TRACE_LEAVE();
				return (NCSCC_RC_SUCCESS);
			}
		} else {
			/* This is an ordered list, so if a cell has a chan_id gtr than
			 * what we're looking for, it's not there.
			 */
			if (wp->chan_id > chan_id) {
				TRACE_LEAVE2("chan record not in worklist");
				return (NCSCC_RC_NO_OBJECT);
			}
		}
		wp = wp->next;	/* Increment to next entry */
	}

	TRACE_LEAVE();
	return (NCSCC_RC_NO_OBJECT);	/* Went through the entire list. Not found. */

}

/****************************************************************************
 *
 * is_active_channel - Determines if chan_name exists and is still linked.
 *
 ***************************************************************************/
static bool is_active_channel(EDS_WORKLIST *wp, uint32_t chan_name_len, uint8_t *chan_name)
{
	TRACE_ENTER2("chan_name: %s", chan_name);

	/* Do the name lengths match? */
	if (wp->cname_len == chan_name_len) {
		/* Do the strings match? */
		if (memcmp(wp->cname, chan_name, chan_name_len) == 0) {
			/* Strings match, now make sure it isn't "unlinked" */
			if (!(wp->chan_attrib & CHANNEL_UNLINKED)) {
				TRACE_LEAVE2("true: channel is not marked as unlinked");
				return (true);
			}
		}
	}
	TRACE_LEAVE2("false: channel is not active");
	return (false);
}

/****************************************************************************
 *
 * eds_add_cname_rec() - Inserts a channelname record into the cname list .
 *
 * Every channel entry has a entry in the channel name tree. 
 *
 ***************************************************************************/
static uint32_t eds_add_cname_rec(EDS_CB *cb, EDS_WORKLIST *wp, uint8_t *chan_name, uint16_t chan_name_len)
{
	EDS_CNAME_REC *cn;
	TRACE_ENTER2("Adding to channel names list. chan_name:%s", chan_name);

	cn = m_MMGR_ALLOC_EDS_CNAME_REC(sizeof(EDS_CNAME_REC));
	if (cn == NULL) {
		LOG_CR("malloc failed for cname rec");
		TRACE_LEAVE();
		return (NCSCC_RC_OUT_OF_MEM);
	}
	memset(cn, 0, sizeof(EDS_CNAME_REC));

	cn->chan_name.length = m_NCS_OS_HTONS(chan_name_len);
	memcpy(cn->chan_name.value, chan_name, chan_name_len);
	cn->wp_rec = wp;
	cn->pat_node.key_info = (uint8_t *)&cn->chan_name;

	/* Insert the record into the patricia tree */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_add(&cb->eds_cname_list, &cn->pat_node)) {
		/* Log it */
		m_MMGR_FREE_EDS_CNAME_REC(cn);
		TRACE_LEAVE2("pat node add failure");
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 *
 * eds_add_chan_open_rec() - Inserts a channel open record.
 *
 * Every channel entry has individual sub-entries for each instance of a
 * channelOpen() call. It is off of these channel open entries that
 * subscriptions for this channel are placed.
 *
 ***************************************************************************/
static uint32_t
eds_add_chan_open_rec(EDS_WORKLIST *wp, uint32_t reg_id, uint32_t chan_id, MDS_DEST dest,
		      uint32_t *chan_open_id, SaAmfHAStateT ha_state, uint32_t open_flags)
{
	CHAN_OPEN_REC *co;
	TRACE_ENTER2("Add a new channel open record. chan name: %s", wp->cname);

	co = m_MMGR_ALLOC_EDS_COPEN_REC(sizeof(CHAN_OPEN_REC));
	if (co == NULL) {
		LOG_CR("malloc failed");
		TRACE_LEAVE();
		return (NCSCC_RC_OUT_OF_MEM);
	}
	memset(co, 0, sizeof(CHAN_OPEN_REC));

	co->reg_id = reg_id;
	co->chan_opener_dest = dest;
	co->chan_id = chan_id;
	co->chan_open_flags = open_flags;

	if (ha_state == SA_AMF_HA_STANDBY) {
		TRACE("Called at STANDBY, simply storing the value received from active");
		co->chan_open_id = *chan_open_id;
		wp->last_copen_id = co->chan_open_id;
		TRACE("chan open id :%u", co->chan_open_id);
	} else {
		co->chan_open_id = ++wp->last_copen_id;
		TRACE("chan_open_id: %u", co->chan_open_id);
	}

	co->copen_id_Net = m_NCS_OS_HTONL(co->chan_open_id);
	co->pat_node.key_info = (uint8_t *)&co->copen_id_Net;

	/* Insert the record into the patricia tree */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_add(&wp->chan_open_rec, &co->pat_node)) {
		TRACE("pat node add failed");
		m_MMGR_FREE_EDS_COPEN_REC(co);
		return NCSCC_RC_FAILURE;
	}

	/* NOTE: Currently incrementing users per channel open instance */
	wp->chan_row.num_users += 1;

	/* Update other objects */
	if (open_flags & SA_EVT_CHANNEL_PUBLISHER)
		wp->chan_row.num_publishers += 1;

	if (open_flags & SA_EVT_CHANNEL_SUBSCRIBER)
		wp->chan_row.num_subscribers += 1;

	/* Return to caller what the channel open id has been set to */
	*chan_open_id = co->chan_open_id;

	TRACE_LEAVE2("num users: %u, num publishers:%u, num subscribers:%u",  wp->chan_row.num_users, \
						wp->chan_row.num_publishers,  wp->chan_row.num_subscribers);
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 *
 * eds_add_chan_open_list - Adds a chanOpenList entry to the reglist
 *
 * This routine is called when a channel is opened and we want to record
 * the channel info in the reglist under the specified reg_id.
 *
 ***************************************************************************/
static uint32_t eds_add_chan_open_list(EDS_CB *cb, uint32_t reg_id, uint32_t chan_id, uint32_t chan_open_id)
{
	EDA_REG_REC *rp;
	CHAN_OPEN_LIST *saved_ptr;;
	TRACE_ENTER2("Associate this chan open with reglist: reg_id:%u, chan_id:%u, chan_open_id:%u", reg_id, \
											chan_id, chan_open_id);

	/* Get the registration list for this reg_id */
	rp = eds_get_reglist_entry(cb, reg_id);
	if (rp == NULL) {
		TRACE_LEAVE2("reg record not found");
		return (NCSCC_RC_FAILURE);
	}

	/* Save current root list pointer */
	saved_ptr = rp->chan_open_list;

	rp->chan_open_list = m_MMGR_ALLOC_EDS_COPEN_LIST(sizeof(CHAN_OPEN_LIST));
	if (rp->chan_open_list == NULL) {
		LOG_CR("malloc failed for chan_open record");
		rp->chan_open_list = saved_ptr;	/* Put original pointer back */
		TRACE_LEAVE();
		return (NCSCC_RC_OUT_OF_MEM);
	}
	memset(rp->chan_open_list, 0, sizeof(CHAN_OPEN_LIST));

	rp->chan_open_list->reg_id = reg_id;
	rp->chan_open_list->chan_id = chan_id;
	rp->chan_open_list->chan_open_id = chan_open_id;
	rp->chan_open_list->next = saved_ptr;	/* Attach saved_ptr to this */

	TRACE_LEAVE();
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 *
 * eds_channel_close_by_regid - Closes all event channels opened
 *                              by a specific reg_id.
 *
 * Cycle through the channel open list and call eds_channel_close()
 * for all entries.
 * 
 ***************************************************************************/
static void eds_channel_close_by_regid(EDS_CB *cb, uint32_t reg_id, bool forced)
{
	EDA_REG_REC *reglist;
	CHAN_OPEN_LIST *cl;
	CHAN_OPEN_LIST *next;
	TRACE_ENTER2("reg_id: %u", reg_id);

	/* Get the registration list for this reg_id */
	reglist = eds_get_reglist_entry(cb, reg_id);
	if (reglist == NULL) {
		TRACE_LEAVE2("reg record not found");
		return;
	}

	/* Close all channels */
	cl = reglist->chan_open_list;
	while (cl) {
		next = cl->next;
		(void)eds_channel_close(cb, cl->reg_id, cl->chan_id, cl->chan_open_id, forced);
		cl = next;
	}
	TRACE_LEAVE();
}

/****************************************************************************
 *
 * eds_remove_chan_open_rec - Deletes a channel open record from the worklist.
 *
 * Removes a CHAN_OPEN_REC from the worklist and decrements the use counter.
 *
 ***************************************************************************/
static uint32_t eds_remove_chan_open_rec(EDS_WORKLIST *wp, CHAN_OPEN_REC *co)
{
	uint32_t rc;
	CHAN_OPEN_REC *rec_to_del;
	TRACE_ENTER2("chan_name:%s, chan_id:%u", wp->cname, co->chan_open_id);

	/* Get the record pointer from the patricia tree */
	if (NULL == (rec_to_del =
		     (CHAN_OPEN_REC *)ncs_patricia_tree_get(&wp->chan_open_rec, (uint8_t *)&co->copen_id_Net))) {
		TRACE_LEAVE2("channel open record not found");
		return (NCSCC_RC_FAILURE);
	}

	if (rec_to_del->chan_open_flags & SA_EVT_CHANNEL_PUBLISHER)
		wp->chan_row.num_publishers -= 1;
	if (rec_to_del->chan_open_flags & SA_EVT_CHANNEL_SUBSCRIBER)
		wp->chan_row.num_subscribers -= 1;

	/* Delete this record from the tree */
	if (NCSCC_RC_SUCCESS != (rc = ncs_patricia_tree_del(&wp->chan_open_rec, &rec_to_del->pat_node))) {
		TRACE_LEAVE2("channel open record delete failed");
		return (NCSCC_RC_FAILURE);
	}

	/* Free allocated memory and decrement the use counter */
	m_MMGR_FREE_EDS_COPEN_REC(rec_to_del);
	wp->use_cnt--;

	TRACE_LEAVE2("use count: %u, num_publishers:%u, num_subscribers:%u", wp->use_cnt, wp->chan_row.num_publishers,\
											  wp->chan_row.num_subscribers);
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 *
 * eds_remove_chan_open_list - Deletes a channel open list from the reglist.
 *
 * Removes a CHAN_OPEN_LIST from the reglist.
 *
 ***************************************************************************/
static void eds_remove_chan_open_list(EDS_CB *cb, uint32_t reg_id, uint32_t chan_id, uint32_t chan_open_id)
{
	EDA_REG_REC *reglist;
	CHAN_OPEN_LIST *cl;
	CHAN_OPEN_LIST *prev;
	TRACE_ENTER2("reg_id: %u chan_id:%u, chan_open_id:%u", reg_id, chan_id, chan_open_id);

	/* Get the specified registration entry out of the regList */
	reglist = eds_get_reglist_entry(cb, reg_id);
	if (reglist == NULL) {
		TRACE_LEAVE2("reg record not found");
		return;		/* No such registration */
	}

	/* Point to root of chan_open_list */
	cl = reglist->chan_open_list;
	if (cl == NULL) {
		TRACE_LEAVE2("chan open list is empty, no channels!!");
		return;		/* No channels */
	}	

	/* Find the right channel open list */
	prev = cl;
	while (cl) {
		if ((cl->chan_id == chan_id) && (cl->chan_open_id == chan_open_id))
			break;
		prev = cl;	/* Save previous pointer if we need to remove next entry */
		cl = cl->next;
	}

	if (!cl) {
		TRACE_LEAVE2("channel open record not found");
		return;
	}

	/* Reset pointers */
	if (cl == reglist->chan_open_list) {	/* 1st in the list? */
		if (cl->next == NULL)	/* Only one in the list? */
			reglist->chan_open_list = NULL;	/* Clear root pointer */
		else {		/* 1st but not only one */

			reglist->chan_open_list = cl->next;	/* Move next one up */
		}
	} else {		/* Not 1st in the list */

		if (prev)
			prev->next = cl->next;	/* Link previous to next */
	}

	/* Free the chan_open_list */
	m_MMGR_FREE_EDS_COPEN_LIST(cl);
	cl = NULL;

	TRACE_LEAVE();
}

/****************************************************************************
 *
 * eds_add_reglist_entry() - Inserts a EDA_REG_REC registration list element.
 *
 ***************************************************************************/
uint32_t eds_add_reglist_entry(EDS_CB *cb, MDS_DEST dest, uint32_t reg_id)
{
	uint32_t rs = NCSCC_RC_SUCCESS;
	EDA_REG_REC *rec;
	TRACE_ENTER2("reg_id: %u, agent dest:%" PRIx64, reg_id, dest);

	if (NULL == (rec = m_MMGR_ALLOC_EDS_REC(sizeof(EDA_REG_REC)))) {
		LOG_CR("malloc failed");
		TRACE_LEAVE();
		return (NCSCC_RC_OUT_OF_MEM);
	}

	memset(rec, 0, sizeof(EDA_REG_REC));
   /** Initialize the record **/
	if (cb->ha_state == SA_AMF_HA_STANDBY)
		cb->last_reg_id = reg_id;
	rec->reg_id = reg_id;
	rec->eda_client_dest = dest;
	rec->reg_id_Net = m_NCS_OS_HTONL(rec->reg_id);
	rec->pat_node.key_info = (uint8_t *)&rec->reg_id_Net;

   /** Insert the record into the patricia tree **/
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_add(&cb->eda_reg_list, &rec->pat_node)) {
		LOG_WA("patricia tree add failed for reg_id: %u, agent dest: %" PRIx64, rec->reg_id, rec->eda_client_dest);
		m_MMGR_FREE_EDS_REC(rec);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return rs;

}

/****************************************************************************
 *
 * eds_del_work_list - Removes all work list records if any.
 *
 ****************************************************************************/
static void eds_del_work_list(EDS_CB *cb, EDS_WORKLIST **p_work_list)
{
	EDS_WORKLIST *work_list;
	TRACE_ENTER2("Deleting worklist");

	while (NULL != (work_list = *p_work_list)) {
		eds_remove_cname_rec(cb, work_list);
		*p_work_list = work_list->next;

		eds_remove_retained_events(work_list->ret_evt_list_head, work_list->ret_evt_list_tail);

	/** We assume that the channel open records must have been
	** erased
	**/
		ncs_patricia_tree_destroy(&work_list->chan_open_rec);

		/* free channelName */
		m_MMGR_FREE_EDS_CHAN_NAME(work_list->cname);
		m_MMGR_FREE_EDS_WORKLIST(work_list);

		work_list = NULL;
	}

	TRACE_LEAVE();
}

/****************************************************************************
 *
 * eds_remove_reglist_entry() - Remove a EDA_REG_REC registration list element
 *                              from an ordered list.
 *
 *  If the regid is zero, which is not a valid regid normally, and the
 *  remove_all flag is true, remove all registrations. This is only called
 *  upon a shutdown of EDS.
 *
 ***************************************************************************/
uint32_t eds_remove_reglist_entry(EDS_CB *cb, uint32_t reg_id, bool remove_all)
{
	EDA_REG_REC *rec_to_del;
	uint32_t status = NCSCC_RC_SUCCESS;
	uint32_t regId_Net;
	EDA_DOWN_LIST *eda_down_rec = NULL, *temp_eda_down_rec = NULL;
	TRACE_ENTER2("Deleting reg record: reg_id: %u", reg_id);

   /** decide if all records are to be deleted **/
	if ((reg_id == 0) && (remove_all == true)) {
		TRACE_1("Deleting all reg records");
		rec_to_del = (EDA_REG_REC *)
		    ncs_patricia_tree_getnext(&cb->eda_reg_list, (uint8_t *)0);

		if (rec_to_del) {
			while (rec_to_del) {
	   /** Close all open channels (and remove subscriptions)
            ** for this registration ID.
            **/
				eds_channel_close_by_regid(cb, rec_to_del->reg_id, remove_all);

	   /** delete the node from the tree 
            **/
				ncs_patricia_tree_del(&cb->eda_reg_list, &rec_to_del->pat_node);

	   /** Store the regId_Net for get Next
            **/
				regId_Net = rec_to_del->reg_id_Net;

	   /** Free the record 
            **/
				m_MMGR_FREE_EDS_REC(rec_to_del);

	   /** Fetch the next record 
            **/
				rec_to_del =
				    (EDA_REG_REC *)ncs_patricia_tree_getnext(&cb->eda_reg_list, (uint8_t *)&regId_Net);
			}
		}

		eda_down_rec = cb->eda_down_list_head;
		while (eda_down_rec) {
			/*Remove the EDA DOWN REC from the EDA_DOWN_LIST */
			/* Free the EDA_DOWN_REC */
			/* Remove this EDA entry from our processing lists */
			temp_eda_down_rec = eda_down_rec;
			eda_down_rec = eda_down_rec->next;
			m_MMGR_FREE_EDA_DOWN_LIST(temp_eda_down_rec);
		}
		cb->eda_down_list_head = NULL;
		cb->eda_down_list_tail = NULL;

      /** Look for retained events 
       ** which can cause some channels to 
       ** be present.
       **/
		eds_del_work_list(cb, &cb->eds_work_list);
	} else {
      /** Remove only one record specified by reg_id 
       **/
		regId_Net = m_NCS_OS_HTONL(reg_id);

      /** Get the node pointer from the patricia tree 
       **/
		if (NULL == (rec_to_del = (EDA_REG_REC *)ncs_patricia_tree_get(&cb->eda_reg_list, (uint8_t *)&regId_Net))) {
			TRACE_LEAVE2("record not found in patricia tree");
			return NCSCC_RC_FAILURE;
		}

      /** Close all open channels (and remove subscriptions)
       ** for this registration ID.
       **/
		eds_channel_close_by_regid(cb, reg_id, remove_all);

		if (NCSCC_RC_SUCCESS != (status = ncs_patricia_tree_del(&cb->eda_reg_list, &rec_to_del->pat_node))) {
			TRACE_LEAVE2("pat node del failed");
			return status;
		}

		/* Free the record */
		m_MMGR_FREE_EDS_REC(rec_to_del);
	}

	TRACE_LEAVE();
	return status;

}

/****************************************************************************
 *
 * eds_eda_entry_valid
 * 
 *  Searches the cb->eda_reg_list for an reg_id entry whos MDS_DEST equals
 *  that passed DEST and returns true if itz found.
 *
 * This routine is typically used to find the validity of the eda down rec from standby 
 * EDA_DOWN_LIST as  EDA client has gone away.
 *
 ****************************************************************************/
bool eds_eda_entry_valid(EDS_CB *cb, MDS_DEST mds_dest)
{
	EDA_REG_REC *rp = NULL;
	TRACE_ENTER2("searching mds_dest:%" PRIx64, mds_dest);

	rp = (EDA_REG_REC *)ncs_patricia_tree_getnext(&cb->eda_reg_list, (uint8_t *)0);

	while (rp != NULL) {
		if (m_NCS_MDS_DEST_EQUAL(&rp->eda_client_dest, &mds_dest)) {
			TRACE_LEAVE2("record found");
			return true;
		}

		rp = (EDA_REG_REC *)ncs_patricia_tree_getnext(&cb->eda_reg_list, (uint8_t *)&rp->reg_id_Net);
	}

	TRACE_LEAVE2("record not found");
	return false;
}

/****************************************************************************
 *
 * eds_remove_eda_down_rec
 * 
 *  Searches the EDA_DOWN_LIST for an entry whos MDS_DEST equals
 *  that passed in and removes the EDA rec.
 *
 * This routine is typically used to remove the eda down rec from standby 
 * EDA_DOWN_LIST as  EDA client has gone away.
 *
 ****************************************************************************/
uint32_t eds_remove_eda_down_rec(EDS_CB *cb, MDS_DEST mds_dest)
{
	EDA_DOWN_LIST *eda_down_rec = cb->eda_down_list_head;
	EDA_DOWN_LIST *prev = NULL;
	TRACE_ENTER2("mds_dest: %" PRIx64, mds_dest);

	while (eda_down_rec) {
		if (m_NCS_MDS_DEST_EQUAL(&eda_down_rec->mds_dest, &mds_dest)) {
			TRACE("record found");
			/* Remove the EDA entry */
			/* Reset pointers */
			if (eda_down_rec == cb->eda_down_list_head) {	/* 1st in the list? */
				if (eda_down_rec->next == NULL) {	/* Only one in the list? */
					cb->eda_down_list_head = NULL;	/* Clear head sublist pointer */
					cb->eda_down_list_tail = NULL;	/* Clear tail sublist pointer */
				} else {	/* 1st but not only one */

					cb->eda_down_list_head = eda_down_rec->next;	/* Move next one up */
				}
			} else {	/* Not 1st in the list */

				if (prev) {
					if (eda_down_rec->next == NULL)
						cb->eda_down_list_tail = prev;
					prev->next = eda_down_rec->next;	/* Link previous to next */
				}
			}

			/* Free the EDA_DOWN_REC */
			m_MMGR_FREE_EDA_DOWN_LIST(eda_down_rec);
			eda_down_rec = NULL;
			break;
		}
		prev = eda_down_rec;	/* Remember address of this entry */
		eda_down_rec = eda_down_rec->next;	/* Go to next entry */
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * eds_remove_regid_by_mds_dest
 *
 *  Searches the reglist for a registration entry whos MDS_DEST equals
 *  that passed in and removes the registration, and all associated records
 *  pertaining to this registration from our internal lists.
 *
 * This routine is typically used to cleanup after being notified that an
 * EDA client has gone away.
 *
 ****************************************************************************/
uint32_t eds_remove_regid_by_mds_dest(EDS_CB *cb, MDS_DEST mds_dest)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDA_REG_REC *rp = NULL;
	uint32_t regId_Net;
	TRACE_ENTER2("mds_dest: %" PRIx64, mds_dest);

	rp = (EDA_REG_REC *)ncs_patricia_tree_getnext(&cb->eda_reg_list, (uint8_t *)0);

	while (rp != NULL) {
      /** Store the regId_Net for get Next
       **/
		regId_Net = rp->reg_id_Net;
		if (m_NCS_MDS_DEST_EQUAL(&rp->eda_client_dest, &mds_dest)) {
			rc = eds_remove_reglist_entry(cb, rp->reg_id, false);
		}

		rp = (EDA_REG_REC *)ncs_patricia_tree_getnext(&cb->eda_reg_list, (uint8_t *)&regId_Net);
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 *
 * eds_get_reglist_entry() - Get a registration list element.
 *
 * Searches a registration list for an entry matching the registration ID
 * passed in.
 *
 * Returns a pointer to the entry, or NULL if not found.
 *
 ***************************************************************************/
EDA_REG_REC *eds_get_reglist_entry(EDS_CB *cb, uint32_t reg_id)
{
	EDA_REG_REC *rp;
	uint32_t regId_Net;
	TRACE_ENTER2("reg_id: %u", reg_id);

	regId_Net = m_NCS_OS_HTONL(reg_id);

	if (NULL == (rp = (EDA_REG_REC *)ncs_patricia_tree_get(&cb->eda_reg_list, (uint8_t *)&regId_Net))) {
		TRACE_LEAVE2("reg record not found");
		return ((EDA_REG_REC *)NULL);
	}

	TRACE_LEAVE2("record found");
	return rp;
}

/****************************************************************************
 *
 * eds_get_worklist_entry() - Get a workList element.
 *
 * Searches a workList for an entry matching the event ID
 * passed in.
 *
 * Returns a pointer to the entry, or NULL if not found.
 *
 ***************************************************************************/
EDS_WORKLIST *eds_get_worklist_entry(EDS_WORKLIST *wp_root, uint32_t chan_id)
{
	EDS_WORKLIST *wp;
	TRACE_ENTER2("chan_id: %u", chan_id);


	if (wp_root == NULL) {
		TRACE_LEAVE2("Worklist is empty");
		return ((EDS_WORKLIST *)NULL);	/* empty worklist */
	}

	/* Loop through the list looking for a matching channel ID */
	wp = wp_root;
	while (wp) {
		if (wp->chan_id == chan_id) {
			TRACE_LEAVE();
			return (wp);
		}
		else {
			/* This is an ordered list, so if a cell has a chan_id gtr than
			 * what we're looking for, it isn't there.
			 */
			if (wp->chan_id > chan_id) {
				TRACE_LEAVE2("record not found");
				return ((EDS_WORKLIST *)NULL);	/* Not found */
			}
		}
		wp = wp->next;
	}

	TRACE_LEAVE2("record not found");
	return ((EDS_WORKLIST *)NULL);	/* Not found */

}

/****************************************************************************
 *
 * eds_add_subscription() - Adds a subscription record to both the worklist
 *                          and reglist.
 *
 ***************************************************************************/
uint32_t eds_add_subscription(EDS_CB *cb, uint32_t reg_id, SUBSC_REC *subrec)
{
	uint32_t rs = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* Add the subscription to the workList */
	rs = eds_add_subscription_to_worklist(cb, subrec);
	if (rs != NCSCC_RC_SUCCESS) {
		TRACE_LEAVE2("Failed to add subscription to worklist");
		return (rs);
	}

	/* Add the subscription to the regList */
	rs = eds_add_subscription_to_reglist(cb, reg_id, subrec);
	if (rs != NCSCC_RC_SUCCESS) {
		TRACE("Failed to add subscription to reglist");
		eds_remove_subrec_entry(cb, &subrec);
	}

	TRACE_LEAVE();
	return (rs);
}

/****************************************************************************
 *
 * eds_remove_subscription() - Removes a subscription from the registration
 *                             list and work lists.
 *
 * Extracts the registration record from the regList, finds the specified
 * subList record, removes the subscription subRec record from the workList,
 * and finally, removes the subList record from the regList.
 *
 * If this is the only subscription for a particular event, the event is
 * removed from the workList as well.
 *
 ***************************************************************************/
uint32_t eds_remove_subscription(EDS_CB *cb, uint32_t reg_id, uint32_t chan_id, uint32_t chan_open_id, uint32_t sub_id)
{
	uint32_t rs = NCSCC_RC_SUCCESS;
	EDA_REG_REC *reglist;
	SUBSC_LIST *sublist = NULL;
	SUBSC_LIST *prev = NULL;
	CHAN_OPEN_LIST *cl;
	TRACE_ENTER2("reg_id:%u, chan_id:%u, chan_open_id:%u, sub_id: %u", reg_id, chan_id, chan_open_id, sub_id);

	/* Get the specified registration entry out of the regList */
	reglist = eds_get_reglist_entry(cb, reg_id);
	if (reglist == NULL) {
		TRACE_LEAVE2("reg record not found");
		return (NCSCC_RC_NO_OBJECT);	/* No such registration */
	}

	/* Point to root of chan_open_list */
	cl = reglist->chan_open_list;
	if (!cl) {
		TRACE_LEAVE2("chan open list is empty");
		return (NCSCC_RC_NO_OBJECT);	/* No channels */
	}

	/* Find the right channel open list */
	while (cl) {
		if ((cl->chan_id == chan_id) && (cl->chan_open_id == chan_open_id)) {
			sublist = cl->subsc_list_head;
			break;
		}
		cl = cl->next;
	}

	/* Search for matching subscription under this channel */
	while (sublist) {
		if (sublist->subsc_rec->subscript_id == sub_id) {
			/* Remove the subRec entry */
			eds_remove_subrec_entry(cb, &sublist->subsc_rec);
			break;
		}
		prev = sublist;	/* Remember address of this entry */
		sublist = sublist->next;	/* Go to next entry */
	}
	if (!sublist) {
		TRACE_LEAVE2("subscription record not found");
		return (NCSCC_RC_NO_OBJECT);	/* No such subscription */
	}

	/* Reset pointers */
	if (sublist == cl->subsc_list_head) {	/* 1st in the list? */
		if (sublist->next == NULL) {	/* Only one in the list? */
			cl->subsc_list_head = NULL;	/* Clear head sublist pointer */
			cl->subsc_list_tail = NULL;	/* Clear tail sublist pointer */
		} else {	/* 1st but not only one */

			cl->subsc_list_head = sublist->next;	/* Move next one up */
		}
	} else {		/* Not 1st in the list */

		if (prev) {
			if (sublist->next == NULL)
				cl->subsc_list_tail = prev;
			prev->next = sublist->next;	/* Link previous to next */
		}
	}

	/* Free the sublist */
	m_MMGR_FREE_EDS_SUBLIST(sublist);
	sublist = NULL;

	TRACE_LEAVE();
	return (rs);
}

/****************************************************************************
 *
 * eds_copen_patricia_init - Init a channel open record patricia tree.
 *
 ****************************************************************************/
uint32_t eds_copen_patricia_init(EDS_WORKLIST *wp)
{
	NCS_PATRICIA_PARAMS param;
	TRACE_ENTER();

	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(uint32_t);

	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&wp->chan_open_rec, &param)) {
		TRACE_LEAVE2("patricia tree init failed");
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 *
 * eds_channel_open - Open an event channel.
 *
 * Search the workList for a EDS_WORKLIST entry with the name passed in.
 * If found, AND NOT UNLINKED, add a channel open record (CHAN_OPEN_REC)
 * for this instance under the EDS_WORKLIST entry. Return a channel ID
 * and channelOpenID. All future references will use this [chan_id/chan_open_id]
 * pairing when referencing this channel instance from now on. The workList
 * channel use count is also incremented.
 * 
 * If not found, or if found but unlinked, create a new entry at the end
 * of the worklist (EDS_WORKLIST), then add a channel open record as above.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 ***************************************************************************/
uint32_t
eds_channel_open(EDS_CB *cb, uint32_t reg_id, uint32_t flags,
		 uint16_t chan_name_len, uint8_t *chan_name, MDS_DEST dest,
		 uint32_t *chan_id, uint32_t *chan_open_id, SaTimeT chan_create_time)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_WORKLIST *wp = NULL;
	EDS_WORKLIST *prevp = NULL;
	/* time_t          time_of_day; */
	SaUint8T list_iter;
	SaAmfHAStateT ha_state;
	CHAN_OPEN_REC *co = NULL;
	uint32_t copen_id_Net;
	TRACE_ENTER2("chan_name: %s", chan_name);

	wp = cb->eds_work_list;	/* Get root pointer to worklist */
	ha_state = cb->ha_state;	/* Get the HA STATE from the CB */

	/* First entry? */
	if (wp == NULL) {
		TRACE("First entry to be added");
		/* Make sure the create flag was specified */
		if (!(flags & SA_EVT_CHANNEL_CREATE)) {
			TRACE_LEAVE2("SA_AIS_ERR_NOT_EXIST: Create flags not specified");
			return (SA_AIS_ERR_NOT_EXIST);
		}
		cb->eds_work_list = m_MMGR_ALLOC_EDS_WORKLIST(sizeof(EDS_WORKLIST));
		if (cb->eds_work_list == NULL) {
			LOG_CR("malloc failed for worklist record");
			TRACE_LEAVE();
			return (SA_AIS_ERR_NO_MEMORY);
		}

		memset(cb->eds_work_list, 0, sizeof(EDS_WORKLIST));
		wp = (EDS_WORKLIST *)cb->eds_work_list;

		if (cb->ha_state == SA_AMF_HA_STANDBY) {
			wp->chan_id = *chan_id;
			TRACE("At standby, just store the value sent from active, chan_id:%u", wp->chan_id);
		}
		else {
			wp->chan_id = 1;
			TRACE("chan_id assigned for this saEvtChannelOpen is: %u", wp->chan_id);
		}

/*      wp->chan_attrib|=flags; */
		wp->cname_len = chan_name_len;
		wp->cname = m_MMGR_ALLOC_EDS_CHAN_NAME(chan_name_len + 1);
		if (wp->cname == NULL) {
			LOG_CR("malloc failed for worklist record");
			TRACE_LEAVE();
			return (SA_AIS_ERR_NO_MEMORY);
		}
		memcpy(wp->cname, chan_name, chan_name_len);
		*(wp->cname + chan_name_len) = '\0';

		/* Update - channel Table objects with default values & creation time stamp */
		EDS_INIT_CHAN_RTINFO(wp, chan_create_time);

		/* Create an IMM runtime object */
		if (cb->ha_state == SA_AMF_HA_ACTIVE) {
			TRACE("I'm ACTIVE eds, create the IMM object for this channel");
			create_runtime_object((char *)wp->cname, wp->chan_row.create_time, cb->immOiHandle);
		}

		/* Initialize the channel open record patricia tree */
		if (eds_copen_patricia_init(wp) != NCSCC_RC_SUCCESS) {
			TRACE_LEAVE2("SA_AIS_ERR_LIBRARY: channel open patricia tree init failed");
			return (SA_AIS_ERR_LIBRARY);
		}
		/* Initialize retevent list to NULL. Fix */
		for (list_iter = SA_EVT_HIGHEST_PRIORITY; list_iter <= SA_EVT_LOWEST_PRIORITY; list_iter++) {
			wp->ret_evt_list_head[list_iter] = NULL;
			wp->ret_evt_list_tail[list_iter] = NULL;
		}

		if (reg_id != 0) {
			wp->use_cnt++;
			TRACE("Incrementing use count: %u", wp->use_cnt);
			/* Assign a new chan_open_id for this open channel instance */
			rc = eds_add_chan_open_rec(wp, reg_id, wp->chan_id, dest, chan_open_id, ha_state, flags);
			if (rc != NCSCC_RC_SUCCESS) {
				TRACE_LEAVE2("SA_AIS_ERR_LIBRARY: channel open record add failed");
				return (SA_AIS_ERR_LIBRARY);
			}
			/* Add an entry to the reglist too */
			rc = eds_add_chan_open_list(cb, reg_id, wp->chan_id, *chan_open_id);
			if (rc != NCSCC_RC_SUCCESS) {
				LOG_IN("channel open to client handle association not found for channel: %s", wp->cname);
				/* Get the chan open rec from the patricia tree */
				copen_id_Net = m_NCS_OS_HTONL(*chan_open_id);
				if (NULL == (co = (CHAN_OPEN_REC *)ncs_patricia_tree_get(&wp->chan_open_rec,
											 (uint8_t *)&copen_id_Net))) {
					TRACE_LEAVE2("chan_open_id: %u not found in pat tree", *chan_open_id);
					return NCSCC_RC_FAILURE;
				}

				eds_remove_chan_open_rec(wp, co);
				TRACE_LEAVE2("SA_AIS_ERR_LIBRARY: channel open add failed");
				return (SA_AIS_ERR_LIBRARY);
			}
		}

		*chan_id = wp->chan_id;	/* Return channel ID #1 */
		/* Revisit this. The channel name list is not necessary */
		eds_add_cname_rec(cb, wp, chan_name, chan_name_len);	/* add the channel name to the cname list */
		TRACE_LEAVE2("channel open success");
		return (SA_AIS_OK);
	}

	/*
	 * Search the worklist for a channel with this name.
	 */
	while (wp) {
		TRACE("Searching the worklist for this channel");
		/* Is this element an active (linked) channel with the correct name? */
		if (is_active_channel(wp, chan_name_len, chan_name)) {
			wp->use_cnt++;	/* Up the use counter */
			TRACE("Incrementing use count: %u", wp->use_cnt);
			/* Assign a new chan_open_id for this open channel instance */
			rc = eds_add_chan_open_rec(wp, reg_id, wp->chan_id, dest, chan_open_id, ha_state, flags);
			if (rc != NCSCC_RC_SUCCESS) {
				TRACE_LEAVE2("SA_AIS_ERR_LIBRARY: channel open record add failed");
				return (SA_AIS_ERR_LIBRARY);
			}
			/* Add an entry to the reglist too */
			rc = eds_add_chan_open_list(cb, reg_id, wp->chan_id, *chan_open_id);
			if (rc != NCSCC_RC_SUCCESS) {
				TRACE("chanopen-reglist association failed");
				/* Get the chan open rec from the patricia tree */
				copen_id_Net = m_NCS_OS_HTONL(*chan_open_id);
				if (NULL == (co = (CHAN_OPEN_REC *)ncs_patricia_tree_get(&wp->chan_open_rec,
											 (uint8_t *)&copen_id_Net))) {
					TRACE_LEAVE2("chan_open_id: %u not found in pat tree", *chan_open_id);
					return NCSCC_RC_FAILURE;
				}

				eds_remove_chan_open_rec(wp, co);
				TRACE_LEAVE2("SA_AIS_ERR_LIBRARY: channel open add failed");
				return (SA_AIS_ERR_LIBRARY);
			}

			*chan_id = wp->chan_id;	/* Return the channel ID */
			TRACE_LEAVE2("channel open success");
			return (SA_AIS_OK);
		}
		prevp = wp;
		wp = wp->next;	/* Increment to next record */
	}

	/*
	 * If we got here we couldn't find a match, or a match was unLinked.
	 * Create a new worklist entry.
	 */

	/* Make sure the create flag was specified */
	if (!(flags & SA_EVT_CHANNEL_CREATE)) {
		TRACE_LEAVE2("SA_AIS_ERR_NOT_EXIST. Channel not found and create flag is not specified");
		return (SA_AIS_ERR_NOT_EXIST);
	}

	if (prevp != NULL) {
		TRACE("Adding a new worklist entry");
		/* Allocate and fill the new workList structure */
		wp = m_MMGR_ALLOC_EDS_WORKLIST(sizeof(EDS_WORKLIST));
		if (wp == NULL) {
			LOG_CR("malloc failed for worklist record");
			TRACE_LEAVE();
			return (SA_AIS_ERR_NO_MEMORY);
		}
		memset(wp, 0, sizeof(EDS_WORKLIST));
		wp->cname_len = chan_name_len;
		wp->cname = m_MMGR_ALLOC_EDS_CHAN_NAME(chan_name_len + 1);
		if (wp->cname == NULL) {
			LOG_CR("malloc failed for channel name record");
			TRACE_LEAVE();
			return (SA_AIS_ERR_NO_MEMORY);
		}

		memcpy(wp->cname, chan_name, chan_name_len);
		*(wp->cname + chan_name_len) = '\0';

		if (cb->ha_state == SA_AMF_HA_STANDBY) {
			wp->chan_id = *chan_id;
			TRACE("At standby, just store the value sent from active, chan_id:%u", wp->chan_id);
		} else {
			wp->chan_id = prevp->chan_id + 1;	/* New ID is previous entry +1 */
			TRACE("chan_id assigned for this saEvtChannelOpen is: %u", wp->chan_id);
		}

		/* initialize channels with default values */
		EDS_INIT_CHAN_RTINFO(wp, chan_create_time);

		/* Create an IMM runtime object */
		if (cb->ha_state == SA_AMF_HA_ACTIVE) {
			TRACE("I'm ACTIVE eds, create the IMM object for this channel");
			create_runtime_object((char *)wp->cname, wp->chan_row.create_time, cb->immOiHandle);
		}

		/* Initialize the channel open record patricia tree */
		if (eds_copen_patricia_init(wp) != NCSCC_RC_SUCCESS) {
			TRACE_LEAVE2("SA_AIS_ERR_LIBRARY: channel open patricia tree init failed");
			return (SA_AIS_ERR_LIBRARY);
		}

		/* Attach the previous/next pointers */
		wp->prev = prevp;
		prevp->next = wp;

		/* Initialize the retained Event List */
		for (list_iter = SA_EVT_HIGHEST_PRIORITY; list_iter <= SA_EVT_LOWEST_PRIORITY; list_iter++) {
			wp->ret_evt_list_head[list_iter] = NULL;
			wp->ret_evt_list_tail[list_iter] = NULL;
		}

		if (reg_id != 0) {
			wp->use_cnt++;
			TRACE("Incrementing use count: %u", wp->use_cnt);
			/* Assign a new chan_open_id for this open channel instance */
			rc = eds_add_chan_open_rec(wp, reg_id, wp->chan_id, dest, chan_open_id, ha_state, flags);
			if (rc != NCSCC_RC_SUCCESS) {
				TRACE_LEAVE2("SA_AIS_ERR_LIBRARY: channel open record add failed");
				return (SA_AIS_ERR_LIBRARY);
			}
			/* Add an entry to the reglist too */
			rc = eds_add_chan_open_list(cb, reg_id, wp->chan_id, *chan_open_id);
			if (rc != NCSCC_RC_SUCCESS) {
				/* Get the chan open rec from the patricia tree */
				copen_id_Net = m_NCS_OS_HTONL(*chan_open_id);
				if (NULL == (co = (CHAN_OPEN_REC *)ncs_patricia_tree_get(&wp->chan_open_rec,
											 (uint8_t *)&copen_id_Net))) {
					TRACE_LEAVE2("chan_open_id: %u not found in pat tree", *chan_open_id);
					return NCSCC_RC_FAILURE;
				}

				eds_remove_chan_open_rec(wp, co);
				TRACE_LEAVE2("SA_AIS_ERR_LIBRARY: channel open add failed");
				return (SA_AIS_ERR_LIBRARY);
			}
		}

		*chan_id = wp->chan_id;	/* Return the channel ID */

		eds_add_cname_rec(cb, wp, chan_name, chan_name_len);	/* add the channel name to cname list */

		TRACE_LEAVE2("channel open success");
		return (SA_AIS_OK);
	}
	TRACE_LEAVE2("SA_AIS_ERR_LIBRARY: channel open failed");
	return (SA_AIS_ERR_LIBRARY);
}

/****************************************************************************
 *
 * eds_channel_close - Close an open instance of an event channel.
 *
 * If this is the last channelOpen entry under the specified channel ID
 * then the channel ID entry will also be removed from the worklist.
 *
 ***************************************************************************/
uint32_t eds_channel_close(EDS_CB *cb, uint32_t reg_id, uint32_t chan_id, uint32_t chan_open_id, bool forced)
{
	uint32_t copen_id_Net;
	EDS_WORKLIST *wp;
	CHAN_OPEN_REC *co;
	SUBSC_REC *subrec;
	SUBSC_REC *next;
	SaNameT chan_name;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("Closing channel open instance: chan_open_id: %u,chan_id: %u,reg_id: %u", chan_open_id, \
												chan_id, reg_id);

	/* Get worklist ptr for this chan_id */
	wp = eds_get_worklist_entry(cb->eds_work_list, chan_id);
	if (!wp) {
		TRACE_LEAVE2("Worklist record not found");
		return (NCSCC_RC_FAILURE);
	}
	TRACE("Channel Name: %s", wp->cname);
	/* Get the chan open rec from the patricia tree */
	copen_id_Net = m_NCS_OS_HTONL(chan_open_id);
	if (NULL == (co = (CHAN_OPEN_REC *)ncs_patricia_tree_get(&wp->chan_open_rec, (uint8_t *)&copen_id_Net))) {
		TRACE_LEAVE2("Channel open record not found");
		return NCSCC_RC_FAILURE;
	}

	/* Make sure all subscriptions have been removed */
	subrec = co->subsc_rec_head;	/* Point to first subscription record */
	while (subrec) {	/* Make sure no subscriptions left */
		next = subrec->next;
		(void)eds_remove_subscription(cb,
					     subrec->reg_list->reg_id,
					     subrec->chan_id, subrec->chan_open_id, subrec->subscript_id);
		subrec = next;
	}

	wp->chan_row.num_users -= 1;

	TRACE("Num users: %u, use count: %u", wp->chan_row.num_users, wp->use_cnt);

	/* Remove the CHAN_OPEN_REC from the worklist */
	eds_remove_chan_open_rec(wp, co);

	/* If no one else interested in this channel, remove it completely */
	if (wp->use_cnt == 0) {
		TRACE("use count is zero");
		chan_name.length = strlen((char *)wp->cname);
		strncpy((char *)chan_name.value, (char *)wp->cname, chan_name.length);
		if ((wp->chan_attrib & CHANNEL_UNLINKED) || (true == forced)) {
			TRACE("forced flag is set 'or' CHANNEL is marked as CHANNEL_UNLINKED");
			if ((true == forced) && (!(wp->chan_attrib & CHANNEL_UNLINKED))) {
				TRACE("forced flag 'and' CHANNEL is marked as CHANNEL_UNLINKED");
				eds_remove_cname_rec(cb, wp);
			}
			if (cb->ha_state == SA_AMF_HA_ACTIVE) {
				TRACE("Delete run time object");
				if ((rc = immutil_saImmOiRtObjectDelete(cb->immOiHandle, &chan_name)) != SA_AIS_OK) {
					LOG_ER("saImmOiRtObjectDelete failed. Channel: %s. rc = %u", chan_name.value, rc);
					return NCSCC_RC_FAILURE;
				}
			}
			eds_remove_worklist_entry(cb, wp->chan_id);
		}
	}

	/* Remove the CHAN_OPEN_LIST entry for this chan from the reglist */
	eds_remove_chan_open_list(cb, reg_id, chan_id, chan_open_id);

	TRACE_LEAVE();
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 *
 * eds_channel_unlink - Handles an unlink request.
 *
 * Anyone may unlink any channel; there are no ownership properties which need
 * to be checked. So search the worklist for a match on the name and if the
 * channel is still linked, set the unlink flag and return. If found but
 * unlinked, keep searching. There can only be one copy of the channel name
 * in the list which is linked, while there may potentially be many channels
 * with the same name which are unlinked.
 *
 ***************************************************************************/
uint32_t eds_channel_unlink(EDS_CB *cb, uint32_t chan_name_len, uint8_t *chan_name)
{
	EDS_WORKLIST *wp;
	SaNameT channel_name;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("channel name: %s", chan_name);

	wp = cb->eds_work_list;	/* Get root pointer to worklist */

	while (wp) {
		TRACE("Use count: %u", wp->use_cnt);
		/* Is this element an active (linked) channel with the correct name? */
		if (is_active_channel(wp, chan_name_len, chan_name)) {
			wp->chan_attrib |= CHANNEL_UNLINKED;	/* Set the unlink flag */
			TRACE("Setting the unlink flag for this channel");

			/* If no one else interested in this channel, remove it completely */
			eds_remove_cname_rec(cb, wp);

			if (wp->use_cnt == 0) {
				TRACE("Use count is zero, delete the and IMM object");
				channel_name.length = strlen((char *)wp->cname);
				strncpy((char *)channel_name.value, (char *)wp->cname, channel_name.length);
				if (cb->ha_state == SA_AMF_HA_ACTIVE) {
					if ((rc = immutil_saImmOiRtObjectDelete(cb->immOiHandle, &channel_name)) != SA_AIS_OK) {
						LOG_ER("saImmOiRtObjectDelete failed. Channel: %s. rc = %u", channel_name.value, rc);
						TRACE_LEAVE();
						return NCSCC_RC_FAILURE;
					}
				}
				eds_remove_worklist_entry(cb, wp->chan_id);
			}
			TRACE_LEAVE();
			return (SA_AIS_OK);
		}
		wp = wp->next;
	}

	TRACE_LEAVE2("SA_AIS_ERR_NOT_EXIST");
	return (SA_AIS_ERR_NOT_EXIST);	/* Went through the entire list. Not found. */
}

static void eds_retd_evt_del(EDS_RETAINED_EVT_REC **, EDS_RETAINED_EVT_REC **, EDS_RETAINED_EVT_REC *, bool);
/****************************************************************************
 *
 * eds_store_retained_event - Adds an event which has the retention timer set
 *                    to the EDS_RETAINED_EVT_REC for the specified channel.
 *
 ****************************************************************************/
uint32_t
eds_store_retained_event(EDS_CB *cb,
			 EDS_WORKLIST *wp,
			 CHAN_OPEN_REC *co, EDSV_EDA_PUBLISH_PARAM *publish_param, SaTimeT orig_publish_time)
{
	EDS_RETAINED_EVT_REC *retained_evt = NULL;
	SaAisErrorT error = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("chan_name: %s", wp->cname);

	retained_evt = m_MMGR_ALLOC_EDS_RETAINED_EVT;
	if (retained_evt == NULL) {
		LOG_CR("malloc failed for retention event record: Number of retained events: %u", \
									wp->chan_row.num_ret_evts);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	memset(retained_evt, '\0', sizeof(EDS_RETAINED_EVT_REC));

	/* create the association with hdl-mngr */
	if (0 == (retained_evt->retd_evt_hdl =
		  ncshm_create_hdl(NCS_HM_POOL_ID_COMMON, NCS_SERVICE_ID_EDS, (NCSCONTEXT)retained_evt))) {
		m_MMGR_FREE_EDS_RETAINED_EVT(retained_evt);
		TRACE_LEAVE2("Create handle failed. Freed the retained event");
		return NCSCC_RC_FAILURE;
	}

	retained_evt->event_id = publish_param->event_id;
	retained_evt->priority = publish_param->priority;
	retained_evt->retentionTime = publish_param->retention_time;
	retained_evt->publishTime = orig_publish_time;

   /** The following fields are required to delete the event
    ** when the timer expires.
    **/
	if (co) {
		retained_evt->reg_id = co->reg_id;
		retained_evt->chan_id = co->chan_id;
		retained_evt->retd_evt_chan_open_id = co->chan_open_id;
	} else {
		retained_evt->reg_id = publish_param->reg_id;
		retained_evt->chan_id = publish_param->chan_id;
		retained_evt->retd_evt_chan_open_id = publish_param->chan_open_id;
	}

	/* Copy the publisher name */
	memcpy(retained_evt->publisherName.value, publish_param->publisher_name.value, SA_MAX_NAME_LENGTH);

	retained_evt->publisherName.length = publish_param->publisher_name.length;

	/* Don't take ownership of PatternArray & data's memory
	 * from original event here. Do it later after
	 * pattern match tests.
	 * NOTE: Mem for pattern array was allocated
	 * when the message was decoded into publish_param.
	 */
	retained_evt->patternArray = publish_param->pattern_array;

	retained_evt->data_len = publish_param->data_len;
	retained_evt->data = publish_param->data;

	/* Attach to rear of list */
	if (wp->ret_evt_list_head[retained_evt->priority] == NULL) {
		wp->ret_evt_list_head[retained_evt->priority] = retained_evt;
	} else {
		wp->ret_evt_list_tail[retained_evt->priority]->next = retained_evt;
	}
	wp->ret_evt_list_tail[retained_evt->priority] = retained_evt;

	/* Start the retention timer now */
	if (retained_evt->retentionTime != SA_TIME_MAX)
		error = eds_start_tmr(cb,
				      &retained_evt->ret_tmr,
				      EDS_RET_EVT_TMR, retained_evt->retentionTime, retained_evt->retd_evt_hdl);

	if (error != NCSCC_RC_SUCCESS) {
		LOG_ER("event retention timer start failed");
		/* This will be from eds_evt_destroy flow */
		retained_evt->patternArray = NULL;
		retained_evt->data_len = 0;
		retained_evt->data = NULL;
		eds_retd_evt_del(&wp->ret_evt_list_head[retained_evt->priority],
				 &wp->ret_evt_list_tail[retained_evt->priority], retained_evt, true);
	} else {
		wp->chan_row.num_ret_evts++;
		TRACE("Number of retained events: %u", wp->chan_row.num_ret_evts);
	}

	TRACE_LEAVE();
	return error;
}

/****************************************************************************
  Name          : eds_retd_evt_del
 
  Description   : This routine deletes the a retd evt record from
                  a list of retd events. 
 
  Arguments     : EDS_RETAINED_EVT_REC **list_head
                  EDS_RETAINED_EVT_REC *rm_node
 
  Return Values : None
 
  Notes         : 
******************************************************************************/
static void
eds_retd_evt_del(EDS_RETAINED_EVT_REC **list_head,
		 EDS_RETAINED_EVT_REC **list_tail, EDS_RETAINED_EVT_REC *rm_node, bool give_hdl)
{
	/* Find the client hdl record in the list of records */
	EDS_RETAINED_EVT_REC *list_iter = *list_head;
	TRACE_ENTER();

	/* If the to be removed record is the first record */
	if (list_iter == rm_node) {
		/* If this is the last node then modify the tail pointer */
		if (*list_tail == rm_node)
			*list_tail = NULL;

		*list_head = rm_node->next;
		/* Free memory associated with this event */
		edsv_free_evt_pattern_array(rm_node->patternArray);
		if (rm_node->data)
			m_MMGR_FREE_EDSV_EVENT_DATA(rm_node->data);

		/* Give the ret evt hdl if it was taken */
		if (give_hdl)
			ncshm_give_hdl(rm_node->retd_evt_hdl);

		/* Gotta destroy the hdl anyway */
		ncshm_destroy_hdl(NCS_SERVICE_ID_EDS, rm_node->retd_evt_hdl);

		/* "STOP" the retention timer only if trigger is thro'
		 * saEvtEventRetentionTimeClear api.
		 * In the timer expiry case, stopping and clearing
		 * timer resources is getting done automatically.
		 */
		if (!give_hdl)
			eds_stop_tmr(&rm_node->ret_tmr);

		m_MMGR_FREE_EDS_RETAINED_EVT(rm_node);
		TRACE_LEAVE();
		return;
	} else {		/* find the rec */

		while (NULL != list_iter) {
			if (list_iter->next == rm_node) {
				/* If this is the last node then modify the tail pointer */
				if (*list_tail == rm_node)
					*list_tail = list_iter;

				list_iter->next = rm_node->next;
				/* Free memory associated with this event */
				edsv_free_evt_pattern_array(rm_node->patternArray);
				if (rm_node->data)
					m_MMGR_FREE_EDSV_EVENT_DATA(rm_node->data);

				/* Give the ret evt hdl if it was taken */
				if (give_hdl)
					ncshm_give_hdl(rm_node->retd_evt_hdl);

				/* Gotta destroy the hdl anyway */
				ncshm_destroy_hdl(NCS_SERVICE_ID_EDS, rm_node->retd_evt_hdl);

				/* stop the retention timer */
				if (!give_hdl)
					eds_stop_tmr(&rm_node->ret_tmr);

				m_MMGR_FREE_EDS_RETAINED_EVT(rm_node);

				TRACE_LEAVE();
				return;
			}

			/* move onto the next one */
			list_iter = list_iter->next;
		}
	}

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : eds_find_retd_evt_by_chan_open_id
 
  Description   : This routine searches a retained event in the
                  list of events.
 
  Arguments     : EDS_WORKLIST *wp, 
                  uint32_t chan_open_id, 
                  uint32_t event_id
 
  Return Values : None
 
  Notes         : 
******************************************************************************/
static EDS_RETAINED_EVT_REC *eds_find_retd_evt_by_chan_open_id(EDS_WORKLIST *wp, uint32_t chan_open_id, uint32_t event_id)
{
	EDS_RETAINED_EVT_REC *retd_evt;
	SaUint8T list_iter;
	TRACE_ENTER2("chan_name: %s, chan_open_id: %u, event_id: %u", wp->cname, chan_open_id, event_id);

	for (list_iter = SA_EVT_HIGHEST_PRIORITY; list_iter <= SA_EVT_LOWEST_PRIORITY; list_iter++) {
		retd_evt = wp->ret_evt_list_head[list_iter];
		while (retd_evt) {
			if (retd_evt->retd_evt_chan_open_id == chan_open_id && retd_evt->event_id == event_id) {
				TRACE_LEAVE();
				return retd_evt;
			}
			retd_evt = retd_evt->next;
		}
	}
	TRACE_LEAVE2("record not found");
	return NULL;
}

/****************************************************************************
 *
 * eds_clear_retained_event - Finds and removes a retained event on a
 *                            specified channel with a specified event_id.
 *
 ****************************************************************************/
uint32_t eds_clear_retained_event(EDS_CB *cb, uint32_t chan_id, uint32_t chan_open_id, uint32_t event_id, bool give_hdl)
{
	EDS_WORKLIST *wp;
	EDS_RETAINED_EVT_REC *retained_evt;
	TRACE_ENTER2("chan_id: %u, chan_open_id: %u", chan_id, chan_open_id);
	
	/* Get worklist ptr for this chan_id */
	wp = eds_get_worklist_entry(cb->eds_work_list, chan_id);
	if (!wp) {
		TRACE_LEAVE2("SA_AIS_ERR_NOT_EXIST: worklist is empty");
		return (SA_AIS_ERR_NOT_EXIST);
	}
	TRACE("chan_name: %s", wp->cname);
   /** Find and delete the retained event **/
	if (NULL != (retained_evt = eds_find_retd_evt_by_chan_open_id(wp, chan_open_id, event_id))) {
		eds_retd_evt_del(&wp->ret_evt_list_head[retained_evt->priority],
				 &wp->ret_evt_list_tail[retained_evt->priority], retained_evt, give_hdl);
		wp->chan_row.num_ret_evts--;
		TRACE("Number of retained events: %u", wp->chan_row.num_ret_evts);
	} else {
		TRACE_LEAVE2("SA_AIS_ERR_NOT_EXIST: Retained event not found");
		return SA_AIS_ERR_NOT_EXIST;
	}
	TRACE_LEAVE();
	return (SA_AIS_OK);
}

/****************************************************************************
 *
 * eds_remove_retained_events - Removes all retained events attached
 *                              to the specified channel open record.
 *
 ****************************************************************************/
void eds_remove_retained_events(EDS_RETAINED_EVT_REC **p_retd_evt_rec, EDS_RETAINED_EVT_REC **list_tail)
{
	EDS_RETAINED_EVT_REC *retd_evt_rec;
	SaUint8T list_iter;
	TRACE_ENTER();

	for (list_iter = SA_EVT_HIGHEST_PRIORITY; list_iter <= SA_EVT_LOWEST_PRIORITY; list_iter++) {
		while (NULL != (retd_evt_rec = *(p_retd_evt_rec + list_iter))) {

			*(p_retd_evt_rec + list_iter) = retd_evt_rec->next;

			if (NULL != retd_evt_rec->patternArray)
				edsv_free_evt_pattern_array(retd_evt_rec->patternArray);

			if (NULL != retd_evt_rec->data)
				m_MMGR_FREE_EDSV_EVENT_DATA(retd_evt_rec->data);

			ncshm_destroy_hdl(NCS_SERVICE_ID_EDS, retd_evt_rec->retd_evt_hdl);

			/* stop the retention timer */
			eds_stop_tmr(&retd_evt_rec->ret_tmr);

			m_MMGR_FREE_EDS_RETAINED_EVT(retd_evt_rec);
			retd_evt_rec = NULL;
		}
		*(list_tail + list_iter) = NULL;
	}
	TRACE_LEAVE();
}

/* End eds_ll.c */
