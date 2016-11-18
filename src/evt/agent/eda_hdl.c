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

  This file contains library routines for handle database.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

******************************************************************************
*/

#include "evt/agent/eda.h"

static uint32_t eda_hdl_cbk_dispatch_one(EDA_CB *, EDA_CLIENT_HDL_REC *);
static uint32_t eda_hdl_cbk_dispatch_all(EDA_CB *, EDA_CLIENT_HDL_REC *);
static uint32_t eda_hdl_cbk_dispatch_block(EDA_CB *, EDA_CLIENT_HDL_REC *);

/****************************************************************************
 * Name          : eda_clear_mbx
 *
 * Description   : This is the function which deletes all the messages from 
 *                 the mail box.
 *
 * Arguments     : arg     - argument to be passed.
 *                 msg     - Event start pointer.
 *
 * Return Values : true/false
 *
 * Notes         : None.
 *****************************************************************************/
bool eda_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	EDSV_MSG *cbk, *pnext;

	pnext = cbk = (EDSV_MSG *)msg;
	while (pnext) {
		pnext = cbk->next;
		eda_msg_destroy(cbk);
		cbk = pnext;
	}
	return true;
}

/****************************************************************************
  Name          : eda_event_hdl_rec_list_del
 
  Description   : This routine deletes a list of event records.
 
  Arguments     : pointer to the list of event records anchor.
 
  Return Values : None
 
  Notes         : 
******************************************************************************/
static void eda_event_hdl_rec_list_del(EDA_EVENT_HDL_REC **pevent_hdl)
{
	EDA_EVENT_HDL_REC *event_hdl;
	while (NULL != (event_hdl = *pevent_hdl)) {
		*pevent_hdl = event_hdl->next;

		ncshm_destroy_hdl(NCS_SERVICE_ID_EDA, event_hdl->event_hdl);
      /** free pattern_array if any **/
		edsv_free_evt_pattern_array(event_hdl->pattern_array);
      /** free the event data if any **/
		if (event_hdl->evt_data) {
			m_MMGR_FREE_EDSV_EVENT_DATA(event_hdl->evt_data);
			event_hdl->evt_data = NULL;
		}
      /** remove the association with hdl-mngr 
       **/
		m_MMGR_FREE_EDA_EVENT_HDL_REC(event_hdl);
		event_hdl = NULL;
	}

}

/****************************************************************************
  Name          : eda_subsc_rec_list_del
 
  Description   : This routine deletes a list of subscription records.
 
  Arguments     : pointer to the list of subscription records anchor.
 
  Return Values : None
 
  Notes         : 
******************************************************************************/
static void eda_subsc_rec_list_del(EDA_SUBSC_REC **psubsc_rec)
{
	EDA_SUBSC_REC *subsc_rec;
	while ((subsc_rec = *psubsc_rec) != NULL) {
		*psubsc_rec = subsc_rec->next;
      /** remove the association with hdl-mngr 
       **/
		m_MMGR_FREE_EDA_SUBSC_REC(subsc_rec);
		subsc_rec = NULL;
	}
}

/****************************************************************************
  Name          : eda_channel_hdl_rec_list_del
 
  Description   : This routine deletes a list of channel records.
 
  Arguments     : pointer to the list of channel records anchor.
 
  Return Values : None
 
  Notes         : 
******************************************************************************/
static void eda_channel_hdl_rec_list_del(EDA_CHANNEL_HDL_REC **pchan_hdl)
{
	EDA_CHANNEL_HDL_REC *chan_hdl;
	while ((chan_hdl = *pchan_hdl) != NULL) {
		*pchan_hdl = chan_hdl->next;
      /** clean up the event records for this channel 
       **/
		ncshm_destroy_hdl(NCS_SERVICE_ID_EDA, chan_hdl->channel_hdl);
		eda_event_hdl_rec_list_del(&chan_hdl->chan_event_anchor);
      /** clean up the subscription records for this channel
       **/
		eda_subsc_rec_list_del(&chan_hdl->subsc_list);
      /** remove the association with hdl-mngr 
       **/
		m_MMGR_FREE_EDA_CHANNEL_HDL_REC(chan_hdl);
		chan_hdl = NULL;
	}
}

/****************************************************************************
  Name          : eda_hdl_list_del
 
  Description   : This routine deletes all handles for this library.
 
  Arguments     : cb  - ptr to the EDA control block
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
void eda_hdl_list_del(EDA_CLIENT_HDL_REC **p_client_hdl)
{
	EDA_CLIENT_HDL_REC *client_hdl;
	while ((client_hdl = *p_client_hdl) != NULL) {
		*p_client_hdl = client_hdl->next;
		ncshm_destroy_hdl(NCS_SERVICE_ID_EDA, client_hdl->local_hdl);
      /** clean up the channel records for this eda-client
       **/
		eda_channel_hdl_rec_list_del(&client_hdl->chan_list);
      /** remove the association with hdl-mngr 
       **/
		m_MMGR_FREE_EDA_CLIENT_HDL_REC(client_hdl);
		client_hdl = 0;
	}
}

/****************************************************************************
  Name          : eda_event_hdl_rec_del
 
  Description   : This routine deletes the a event handle record from
                  a list of event hdl records. 
 
  Arguments     : EDA_EVENT_HDL_REC **list_head
                  EDA_EVENT_HDL_REC *rm_node

 
  Return Values : None
 
  Notes         : 
******************************************************************************/
uint32_t eda_event_hdl_rec_del(EDA_EVENT_HDL_REC **list_head, EDA_EVENT_HDL_REC *rm_node)
{
	/* Find the event hdl record in the list of records */
	EDA_EVENT_HDL_REC *list_iter = *list_head;

	/* If the to be removed record is the first record */
	if (list_iter == rm_node) {
		*list_head = rm_node->next;
      /** remove the association with hdl-mngr 
       **/
		ncshm_give_hdl(rm_node->event_hdl);
		ncshm_destroy_hdl(NCS_SERVICE_ID_EDA, rm_node->event_hdl);
      /** free pattern_array if any **/
		edsv_free_evt_pattern_array(rm_node->pattern_array);
      /** free the event data if any **/
		if (rm_node->evt_data) {
			m_MMGR_FREE_EDSV_EVENT_DATA(rm_node->evt_data);
			rm_node->evt_data = NULL;
		}
      /** Free the event hdl record 
       **/
		m_MMGR_FREE_EDA_EVENT_HDL_REC(rm_node);
		return NCSCC_RC_SUCCESS;
	} else {		/* find the rec */

		while (NULL != list_iter) {
			if (list_iter->next == rm_node) {
				list_iter->next = rm_node->next;
	    /** remove the association with hdl-mngr 
             **/
				ncshm_give_hdl(rm_node->event_hdl);
				ncshm_destroy_hdl(NCS_SERVICE_ID_EDA, rm_node->event_hdl);
	    /** free pattern_array if any **/
				edsv_free_evt_pattern_array(rm_node->pattern_array);
	    /** free the event data if any **/
				if (rm_node->evt_data) {
					m_MMGR_FREE_EDSV_EVENT_DATA(rm_node->evt_data);
					rm_node->evt_data = NULL;
				}
	    /** Free the event hdl record 
             **/
				m_MMGR_FREE_EDA_EVENT_HDL_REC(rm_node);
				return NCSCC_RC_SUCCESS;
			}

			/* move onto the next one */
			list_iter = list_iter->next;
		}
	}
   /** The node couldn't be deleted **/
	TRACE("eda_event_hdl_rec_del failed");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : eda_del_subsc_rec
 
  Description   : This routine deletes the a channel handle record from
                  a list of channel hdl records. 
 
  Arguments     : EDA_CHANNEL_HDL_REC **list_head
                  EDA_CHANNEL_HDL_REC *rm_node

 
  Return Values : None
 
  Notes         : 
******************************************************************************/
uint32_t eda_del_subsc_rec(EDA_SUBSC_REC **list_head, EDA_SUBSC_REC *rm_node)
{
	/* Find the channel hdl record in the list of records */
	EDA_SUBSC_REC *list_iter = *list_head;

	/* If the to be removed record is the first record */
	if (list_iter == rm_node) {
		*list_head = rm_node->next;
      /** Free the subsc record 
       **/
		m_MMGR_FREE_EDA_SUBSC_REC(rm_node);
		return NCSCC_RC_SUCCESS;
	} else {		/* find the rec */

		while (NULL != list_iter) {
			if (list_iter->next == rm_node) {
				list_iter->next = rm_node->next;
				m_MMGR_FREE_EDA_SUBSC_REC(rm_node);
				return NCSCC_RC_SUCCESS;
			}
			/* move onto the next one */
			list_iter = list_iter->next;
		}
	}

   /** The node couldn't be deleted 
    **/
	TRACE("eda_del_subsc_rec failed");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : eda_channel_hdl_rec_del
 
  Description   : This routine deletes the a channel handle record from
                  a list of channel hdl records. 
 
  Arguments     : EDA_CHANNEL_HDL_REC **list_head
                  EDA_CHANNEL_HDL_REC *rm_node

 
  Return Values : None
 
  Notes         : 
******************************************************************************/
uint32_t eda_channel_hdl_rec_del(EDA_CHANNEL_HDL_REC **list_head, EDA_CHANNEL_HDL_REC *rm_node)
{
	/* Find the channel hdl record in the list of records */
	EDA_CHANNEL_HDL_REC *list_iter = *list_head;

	/* If the to be removed record is the first record */
	if (list_iter == rm_node) {
		*list_head = rm_node->next;
      /** remove the association with hdl-mngr 
       **/
		ncshm_give_hdl(rm_node->channel_hdl);
		ncshm_destroy_hdl(NCS_SERVICE_ID_EDA, rm_node->channel_hdl);
      /** Free the event hdl records off this hdl 
       **/
		eda_event_hdl_rec_list_del(&rm_node->chan_event_anchor);
      /** clean up the subscription records for this channel
       **/
		eda_subsc_rec_list_del(&rm_node->subsc_list);
      /** Free the channel hdl record 
       **/
		m_MMGR_FREE_EDA_CHANNEL_HDL_REC(rm_node);
		return NCSCC_RC_SUCCESS;
	} else {		/* find the rec */

		while (NULL != list_iter) {
			if (list_iter->next == rm_node) {
				list_iter->next = rm_node->next;
	    /** remove the association with hdl-mngr 
             **/
				ncshm_give_hdl(rm_node->channel_hdl);
				ncshm_destroy_hdl(NCS_SERVICE_ID_EDA, rm_node->channel_hdl);
	    /** Free the event hdl records off this hdl  
             **/
				eda_event_hdl_rec_list_del(&rm_node->chan_event_anchor);
	    /** clean up the subscription records for this channel
             **/
				eda_subsc_rec_list_del(&rm_node->subsc_list);
	    /** Free the channel hdl record 
             **/
				m_MMGR_FREE_EDA_CHANNEL_HDL_REC(rm_node);
				return NCSCC_RC_SUCCESS;
			}
			/* move onto the next one */
			list_iter = list_iter->next;
		}
	}

   /** The node couldn't be deleted 
    **/
	TRACE("eda_channel_hdl_rec_del failed");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : eda_hdl_rec_del
 
  Description   : This routine deletes the a client handle record from
                  a list of client hdl records. 
 
  Arguments     : EDA_CLIENT_HDL_REC **list_head
                  EDA_CLIENT_HDL_REC *rm_node
 
  Return Values : None
 
  Notes         : The selection object is destroyed after all the means to 
                  access the handle record (ie. hdl db tree or hdl mngr) is 
                  removed. This is to disallow the waiting thread to access 
                  the hdl rec while other thread executes saAmfFinalize on it.
******************************************************************************/
uint32_t eda_hdl_rec_del(EDA_CLIENT_HDL_REC **list_head, EDA_CLIENT_HDL_REC *rm_node)
{
	/* Find the client hdl record in the list of records */
	EDA_CLIENT_HDL_REC *list_iter = *list_head;

	/* If the to be removed record is the first record */
	if (list_iter == rm_node) {
		*list_head = rm_node->next;

      /** detach & release the IPC 
       **/
		m_NCS_IPC_DETACH(&rm_node->mbx, eda_clear_mbx, NULL);
		m_NCS_IPC_RELEASE(&rm_node->mbx, NULL);

		ncshm_give_hdl(rm_node->local_hdl);
		ncshm_destroy_hdl(NCS_SERVICE_ID_EDA, rm_node->local_hdl);
      /** Free the channel records off this hdl 
       **/
		eda_channel_hdl_rec_list_del(&rm_node->chan_list);

      /** free the hdl rec 
       **/
		m_MMGR_FREE_EDA_CLIENT_HDL_REC(rm_node);

		return NCSCC_RC_SUCCESS;
	} else {		/* find the rec */

		while (NULL != list_iter) {
			if (list_iter->next == rm_node) {
				list_iter->next = rm_node->next;

	    /** detach & release the IPC 
             **/
				m_NCS_IPC_DETACH(&rm_node->mbx, eda_clear_mbx, NULL);
				m_NCS_IPC_RELEASE(&rm_node->mbx, NULL);

				ncshm_give_hdl(rm_node->local_hdl);
				ncshm_destroy_hdl(NCS_SERVICE_ID_EDA, rm_node->local_hdl);
	    /** Free the channel records off this eda_hdl  
             **/
				eda_channel_hdl_rec_list_del(&rm_node->chan_list);

	    /** free the hdl rec 
             **/
				m_MMGR_FREE_EDA_CLIENT_HDL_REC(rm_node);

				return NCSCC_RC_SUCCESS;
			}
			/* move onto the next one */
			list_iter = list_iter->next;
		}
	}

	TRACE("eda_hdl_rec_del failed");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : eda_event_hdl_rec_add
 
  Description   : This routine adds an event rec to the list of event records
                  hanging off a channel. 
 
  Arguments     : EDA_CHANNEL_HDL_REC **chan_hdl_rec
                   
  Return Values : ptr to the eda handle record
 
  Notes         : None
******************************************************************************/
EDA_EVENT_HDL_REC *eda_event_hdl_rec_add(EDA_CHANNEL_HDL_REC **chan_hdl_rec)
{
	EDA_EVENT_HDL_REC *rec = 0;

	/* allocate the event hdl rec */
	if (NULL == (rec = m_MMGR_ALLOC_EDA_EVENT_HDL_REC)) {
		TRACE_4("malloc failed");
		return NULL;
	}

	memset(rec, '\0', sizeof(EDA_EVENT_HDL_REC));

	/* create the association with hdl-mngr */
	if (0 == (rec->event_hdl = ncshm_create_hdl(1, NCS_SERVICE_ID_EDA, (NCSCONTEXT)rec))) {
		TRACE("create handle failed");
		m_MMGR_FREE_EDA_CHANNEL_HDL_REC(rec);
		return NULL;
	}
   /** Initialize the parent channel **/
	rec->parent_chan = *chan_hdl_rec;

  /** Insert this record into the list of channel hdl records
   **/
	rec->next = (*chan_hdl_rec)->chan_event_anchor;
	(*chan_hdl_rec)->chan_event_anchor = rec;

  /** Everything appears fine, so return the 
   ** event hdl.
   **/
	return rec;
};

/****************************************************************************
  Name          : eda_channel_hdl_rec_add
 
  Description   : This routine adds the cghannel handle record to the list
                  of handles in the client hdl record.
 
  Arguments     : EDA_CLIENT_HDL_REC *hdl_rec
                  uint32_t               chan_id
                  uint32_t               chan_open_id
                  uint32_t               channel_open_flags
                  SaNameT             *channelName
 
  Return Values : ptr to the channel handle record
 
  Notes         : None
******************************************************************************/
EDA_CHANNEL_HDL_REC *eda_channel_hdl_rec_add(EDA_CLIENT_HDL_REC **hdl_rec,
					     uint32_t chan_id,
					     uint32_t chan_open_id, uint32_t channel_open_flags, const SaNameT *channelName)
{
	EDA_CHANNEL_HDL_REC *rec = 0;

	/* allocate the channel hdl rec */
	if (NULL == (rec = m_MMGR_ALLOC_EDA_CHANNEL_HDL_REC)) {
		TRACE_4("malloc failed");
		return NULL;
	}

	memset(rec, '\0', sizeof(EDA_CHANNEL_HDL_REC));

	/* create the association with hdl-mngr */
	if (0 == (rec->channel_hdl = ncshm_create_hdl(1, NCS_SERVICE_ID_EDA, (NCSCONTEXT)rec))) {
		TRACE("create handle failed");
		m_MMGR_FREE_EDA_CHANNEL_HDL_REC(rec);
		return NULL;
	}

   /** Initialize the known channel attributes at this point
    **/
	rec->eds_chan_id = chan_id;
	rec->eds_chan_open_id = chan_open_id;
	rec->open_flags = channel_open_flags;
	rec->channel_name.length = channelName->length;
	memcpy((void *)rec->channel_name.value, (void *)channelName->value, channelName->length);
   /** Initialize the parent handle **/
	rec->parent_hdl = *hdl_rec;
   /** Initialize the last Published event id to 1000 0-1000 are for reserved events**/
	rec->last_pub_evt_id = MAX_RESERVED_EVENTID;

   /** Insert this record into the list of channel hdl records
    **/
	rec->next = (*hdl_rec)->chan_list;
	(*hdl_rec)->chan_list = rec;

   /** Everything appears fine, so return the 
    ** channel hdl.
    **/
	return rec;
}

/****************************************************************************
  Name          : eda_hdl_rec_add
 
  Description   : This routine adds the handle record to the eda cb.
 
  Arguments     : cb       - ptr tot he EDA control block
                  reg_cbks - ptr to the set of registered callbacks
                  reg_id   - obtained from EDS.
                  version  - version of the client.
 
  Return Values : ptr to the eda handle record
 
  Notes         : None
******************************************************************************/
EDA_CLIENT_HDL_REC *eda_hdl_rec_add(EDA_CB **eda_cb, const SaEvtCallbacksT *reg_cbks, uint32_t reg_id, SaVersionT version)
{
	EDA_CLIENT_HDL_REC *rec = 0;

	/* allocate the hdl rec */
	if (NULL == (rec = m_MMGR_ALLOC_EDA_CLIENT_HDL_REC)) {
		TRACE_4("malloc failed");
		goto error;
	}

	memset(rec, '\0', sizeof(EDA_CLIENT_HDL_REC));

	/* create the association with hdl-mngr */
	if (0 == (rec->local_hdl = ncshm_create_hdl((*eda_cb)->pool_id, NCS_SERVICE_ID_EDA, (NCSCONTEXT)rec))) {
		TRACE("create handle failed");
		goto error;
	}

	/* store the registered callbacks */
	if (reg_cbks)
		memcpy((void *)&rec->reg_cbk, (void *)reg_cbks, sizeof(SaEvtCallbacksT));

   /** Associate with the reg_id obtained from EDS
    **/
	rec->eds_reg_id = reg_id;
	rec->version.releaseCode = version.releaseCode;
	rec->version.majorVersion = version.majorVersion;
	rec->version.minorVersion = version.minorVersion;

   /** Initialize and attach the IPC/Priority queue
    **/
	if (m_NCS_IPC_CREATE(&rec->mbx) != NCSCC_RC_SUCCESS) {
		TRACE_4("IPC mailbox create failed");
		goto error;
	}
	if (m_NCS_IPC_ATTACH(&rec->mbx) != NCSCC_RC_SUCCESS) {
		TRACE_4("IPC mailbox attach failed");
		goto error;
	}
   /** Add this to the Link List of 
    ** CLIENT_HDL_RECORDS for this EDA_CB 
    **/

	m_NCS_LOCK(&((*eda_cb)->cb_lock), NCS_LOCK_WRITE);
	/* add this to the start of the list */
	rec->next = (*eda_cb)->eda_init_rec_list;
	(*eda_cb)->eda_init_rec_list = rec;
	m_NCS_UNLOCK(&((*eda_cb)->cb_lock), NCS_LOCK_WRITE);

	return rec;

 error:
	if (rec) {
		/* remove the association with hdl-mngr */
		if (rec->local_hdl)
			ncshm_destroy_hdl(NCS_SERVICE_ID_EDA, rec->local_hdl);

      /** detach and release the IPC 
       **/
		m_NCS_IPC_DETACH(&rec->mbx, eda_clear_mbx, NULL);
		m_NCS_IPC_RELEASE(&rec->mbx, NULL);

		m_MMGR_FREE_EDA_CLIENT_HDL_REC(rec);
	}
	return NULL;
}

/****************************************************************************
  Name          : eda_hdl_cbk_dispatch
 
  Description   : This routine dispatches the pending callbacks as per the 
                  dispatch flags.
 
  Arguments     : cb      - ptr to the EDA control block
                  hdl_rec - ptr to the handle record
                  flags   - dispatch flags
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t eda_hdl_cbk_dispatch(EDA_CB *cb, EDA_CLIENT_HDL_REC *hdl_rec, SaDispatchFlagsT flags)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	switch (flags) {
	case SA_DISPATCH_ONE:
		rc = eda_hdl_cbk_dispatch_one(cb, hdl_rec);
		break;

	case SA_DISPATCH_ALL:
		rc = eda_hdl_cbk_dispatch_all(cb, hdl_rec);
		break;

	case SA_DISPATCH_BLOCKING:
		rc = eda_hdl_cbk_dispatch_block(cb, hdl_rec);
		break;

	default:
		TRACE_3("Invalid dispatch type");
		break;
	}			/* switch */

	return rc;
}

/****************************************************************************
  Name          : eda_hdl_cbk_rec_prc
 
  Description   : This routine invokes the registered callback routine & frees
                  the callback record resources.
 
  Arguments     : cb      - ptr to the EDA control block
                  msg     - ptr to the callback message
                  reg_cbk - ptr to the registered callbacks
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
static void eda_hdl_cbk_rec_prc(EDA_CB *cb, EDSV_MSG *msg, SaEvtCallbacksT *reg_cbk)
{
	EDSV_CBK_INFO *info = &msg->info.cbk_info;

	/* invoke the corresponding callback */
	switch (info->type) {
	case EDSV_EDS_CHAN_OPEN:
		{
			EDSV_EDA_CHAN_OPEN_CBK_PARAM *chanopen = &info->param.chan_open_cbk;
			TRACE("Invoking ChannelOpen callback");

			if (reg_cbk->saEvtChannelOpenCallback)
				reg_cbk->saEvtChannelOpenCallback(info->inv, chanopen->eda_chan_hdl, chanopen->error);
		}
		break;

	case EDSV_EDS_DELIVER_EVENT:
		{
			EDSV_EDA_EVT_DELIVER_CBK_PARAM *evtdeliver = &info->param.evt_deliver_cbk;
			TRACE("Invoking EventDeliver callback");
			if (reg_cbk->saEvtEventDeliverCallback)
				reg_cbk->saEvtEventDeliverCallback(evtdeliver->sub_id,
								   evtdeliver->event_hdl, evtdeliver->data_len);
		}
		break;
	default:
		TRACE_3("Invalid callback type");
		break;
	}			/* switch */

	return;
}

/****************************************************************************
 
  Name          : eda_find_subsc_validity
  Description   : This routine finds out whether the callback 
                  is for valid subscriber or not .
 
  Arguments     : cb      - ptr to the EDA control block
                  cbk_msg - ptr to the callback msg 
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t eda_find_subsc_validity(EDA_CB *cb, EDSV_MSG *cbk_msg)
{
	EDA_CHANNEL_HDL_REC *chan_hdl_rec = NULL;
	EDA_EVENT_HDL_REC *evt_hdl_rec = NULL;
	EDSV_EDA_EVT_DELIVER_CBK_PARAM *evt_dlv_param = &cbk_msg->info.cbk_info.param.evt_deliver_cbk;
	SaEvtEventHandleT eventHandle = evt_dlv_param->event_hdl;
   /** Lookup the hdl rec 
    **/
	/* retrieve event hdl record */
	if (NULL == (evt_hdl_rec = (EDA_EVENT_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, eventHandle))) {
		TRACE_4("take handle failed for eventHandle: %llx", eventHandle);
		return NCSCC_RC_FAILURE;
	}
	if (evt_hdl_rec->parent_chan) {	/* Check if channel still exists */
		if (evt_hdl_rec->parent_chan->channel_hdl) {
			/* retrieve the eda channel hdl record */
			if (NULL !=
			    (chan_hdl_rec =
			     (EDA_CHANNEL_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDA,
								   evt_hdl_rec->parent_chan->channel_hdl))) {
				if (NULL != eda_find_subsc_by_subsc_id(chan_hdl_rec, evt_dlv_param->sub_id)) {
					ncshm_give_hdl(eventHandle);
					ncshm_give_hdl(chan_hdl_rec->channel_hdl);
					return NCSCC_RC_SUCCESS;
				} else {
					if (chan_hdl_rec->subsc_list) {
						ncshm_give_hdl(eventHandle);
						ncshm_give_hdl(chan_hdl_rec->channel_hdl);
						return NCSCC_RC_SUCCESS;
					} else {
		    /** Lock EDA_CB synchronize access with MDS thread.
                     **/
						m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE);

		     /** Delete this evt record from the
                      ** list of events
                      **/
						if (NCSCC_RC_SUCCESS !=
						    eda_event_hdl_rec_del(&chan_hdl_rec->chan_event_anchor,
									  evt_hdl_rec)) {
							ncshm_give_hdl(eventHandle);
						}
						m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
						ncshm_give_hdl(chan_hdl_rec->channel_hdl);
						return NCSCC_RC_FAILURE;

					}

				}
			}
		}
	}

	ncshm_give_hdl(eventHandle);
	return NCSCC_RC_FAILURE;

}

/****************************************************************************
  Name          : eda_hdl_cbk_dispatch_one
 
  Description   : This routine dispatches one pending callback.
 
  Arguments     : cb      - ptr to the EDA control block
                  hdl_rec - ptr to the handle record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t eda_hdl_cbk_dispatch_one(EDA_CB *cb, EDA_CLIENT_HDL_REC *hdl_rec)
{
	EDSV_MSG *cbk_msg = NULL;
	uint32_t rc = SA_AIS_OK;

	/* Nonblk receive to obtain the message from priority queue */
	while (NULL != (cbk_msg = (EDSV_MSG *)
			m_NCS_IPC_NON_BLK_RECEIVE(&hdl_rec->mbx, cbk_msg))) {
		if (cbk_msg->info.cbk_info.type == EDSV_EDS_DELIVER_EVENT) {
			if (eda_find_subsc_validity(cb, cbk_msg) == NCSCC_RC_SUCCESS) {
				/* process the callback list record */
				eda_hdl_cbk_rec_prc(cb, cbk_msg, &hdl_rec->reg_cbk);
				eda_msg_destroy(cbk_msg);
				break;
			}
			eda_msg_destroy(cbk_msg);
		} else {
			/* process the callback list record */
			eda_hdl_cbk_rec_prc(cb, cbk_msg, &hdl_rec->reg_cbk);
			eda_msg_destroy(cbk_msg);
			break;
		}
	}
	return rc;
}

/****************************************************************************
  Name          : eda_hdl_cbk_dispatch_all
 
  Description   : This routine dispatches all the pending callbacks.
 
  Arguments     : cb      - ptr to the EDA control block
                  hdl_rec - ptr to the handle record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t eda_hdl_cbk_dispatch_all(EDA_CB *cb, EDA_CLIENT_HDL_REC *hdl_rec)
{
	EDSV_MSG *cbk_msg = NULL;
	uint32_t rc = SA_AIS_OK;

	/* Recv all the cbk notifications from the queue & process them */
	do {
		if (NULL == (cbk_msg = (EDSV_MSG *)m_NCS_IPC_NON_BLK_RECEIVE(&hdl_rec->mbx, cbk_msg)))
			break;
		if (cbk_msg->info.cbk_info.type == EDSV_EDS_DELIVER_EVENT) {
			if (eda_find_subsc_validity(cb, cbk_msg) == NCSCC_RC_SUCCESS) {
				/* process the callback list record */
				eda_hdl_cbk_rec_prc(cb, cbk_msg, &hdl_rec->reg_cbk);
			}
		} else {
			/* process the callback list record */
			eda_hdl_cbk_rec_prc(cb, cbk_msg, &hdl_rec->reg_cbk);
		}
		/* now that we are done with this rec, free the resources */
		eda_msg_destroy(cbk_msg);
	} while (1);

	return rc;
}

/****************************************************************************
  Name          : eda_hdl_cbk_dispatch_block
 
  Description   : This routine blocks forever for receiving indications from 
                  EDS. The routine returns when saEvtFinalize is executed on 
                  the handle.
 
  Arguments     : cb      - ptr to the EDA control block
                  hdl_rec - ptr to the handle record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t eda_hdl_cbk_dispatch_block(EDA_CB *cb, EDA_CLIENT_HDL_REC *hdl_rec)
{
	EDSV_MSG *cbk_msg = NULL;
	uint32_t rc = SA_AIS_OK;

	for (;;) {
		if (NULL != (cbk_msg = (EDSV_MSG *)
			     m_NCS_IPC_RECEIVE(&hdl_rec->mbx, cbk_msg))) {
			if (cbk_msg->info.cbk_info.type == EDSV_EDS_DELIVER_EVENT) {
				if (eda_find_subsc_validity(cb, cbk_msg) == NCSCC_RC_SUCCESS) {
					/* process the callback list record */
					eda_hdl_cbk_rec_prc(cb, cbk_msg, &hdl_rec->reg_cbk);
				}
			} else {
				/* process the callback list record */
				eda_hdl_cbk_rec_prc(cb, cbk_msg, &hdl_rec->reg_cbk);
			}

			/* now that we are done with this rec, free the resources */
			eda_msg_destroy(cbk_msg);
			/* check to see the validity of the hdl. */
			if (false == eda_validate_eda_client_hdl(cb, hdl_rec))
				return rc;
		} else
			return rc;	/* FIX to handle finalize clean up of mbx */
	}

	return rc;
}

/****************************************************************************
  Name          : eda_extract_pattern_from_event
 
  Description   : This routine invokes the registered callback routine & frees
                  the callback record resources.
 
  Arguments     : cb      - ptr to the EDA control block
                  rec     - ptr to the callback record
                  reg_cbk - ptr to the registered callbacks
 
  Return Values : SaAisErrorT 
 
  Notes         : None
******************************************************************************/
SaAisErrorT
eda_extract_pattern_from_event(SaEvtEventPatternArrayT *from_pattern_array, SaEvtEventPatternArrayT **to_pattern_array)
{
	uint32_t n = 0;
	SaEvtEventPatternT *from_pattern, *to_pattern;
	SaAisErrorT error = SA_AIS_OK;

	(*to_pattern_array)->patternsNumber = from_pattern_array->patternsNumber;

   /** For now copy the patterns only if a buffer of a correct
    ** size has been provided.
    **/
	if ((*to_pattern_array)->allocatedNumber < from_pattern_array->patternsNumber) {
		for (n = 0,
		     from_pattern = from_pattern_array->patterns,
		     to_pattern = (*to_pattern_array)->patterns;
		     n < (*to_pattern_array)->allocatedNumber; n++, from_pattern++, to_pattern++) {
			if (to_pattern && from_pattern)
				to_pattern->patternSize = from_pattern->patternSize;
		}

		error = SA_AIS_ERR_NO_SPACE;
		TRACE_2("allocatedNumber < patternsNumber");
		return error;
	}

	for (n = 0,
	     from_pattern = from_pattern_array->patterns,
	     to_pattern = (*to_pattern_array)->patterns;
	     n < from_pattern_array->patternsNumber; n++, from_pattern++, to_pattern++) {
		if ((to_pattern == NULL) || (from_pattern->patternSize > to_pattern->allocatedSize)) {
			error = SA_AIS_ERR_NO_SPACE;
			TRACE_2("Insufficient space. in pattern is NULL or patternsize > allocated size");
		} else {
			memcpy((void *)to_pattern->pattern,
			       (void *)from_pattern->pattern, (uint32_t)from_pattern->patternSize);
		}
		/* pattern size should be set in all cases.'B' spec */
		if (to_pattern != NULL)
			to_pattern->patternSize = from_pattern->patternSize;

	}			/*end for */

	return error;
}

SaAisErrorT
eda_allocate_and_extract_pattern_from_event(SaEvtEventPatternArrayT *from_pattern_array,
					    SaEvtEventPatternArrayT **to_pattern_array)
{
	uint32_t n = 0;
	SaEvtEventPatternT *from_pattern, *to_pattern;
	SaAisErrorT error = SA_AIS_OK;

	(*to_pattern_array)->patternsNumber = from_pattern_array->patternsNumber;

      /** Initialize the  patterns field **/
	if (from_pattern_array->patternsNumber != 0) {
		(*to_pattern_array)->patterns =
		    malloc((*to_pattern_array)->patternsNumber * sizeof(SaEvtEventPatternT));
		if (NULL == (*to_pattern_array)->patterns) {
			error = SA_AIS_ERR_NO_MEMORY;
			TRACE_4("malloc failed");
			return error;
		}
	  /** zero the memory **/
		memset((*to_pattern_array)->patterns, '\0',
		       (*to_pattern_array)->patternsNumber * sizeof(SaEvtEventPatternT));
		for (n = 0, from_pattern = from_pattern_array->patterns, to_pattern = (*to_pattern_array)->patterns;
		     n < from_pattern_array->patternsNumber; n++, from_pattern++, to_pattern++) {
			if (from_pattern != NULL) {
		/** Assign the pattern size **/
				to_pattern->patternSize = from_pattern->patternSize;
		/** Allocate memory for the individual pattern **/
				if (NULL == (to_pattern->pattern = (SaUint8T *)
					     malloc(((uint32_t)from_pattern->patternSize) * sizeof(SaUint8T)))) {
					error = SA_AIS_ERR_NO_MEMORY;
					TRACE_4("malloc failed");
					free((*to_pattern_array)->patterns);
					(*to_pattern_array)->patterns = NULL;
					return error;
				}
		/** Clear memory for the allocated pattern **/
				memset(to_pattern->pattern, '\0', (uint32_t)to_pattern->patternSize);
				memcpy((void *)to_pattern->pattern,
				       (void *)from_pattern->pattern, (uint32_t)from_pattern->patternSize);
			}
		}		/*end for */
	}			/* End if from_pattern_array->patternsNumber != 0 */
	return error;
}
