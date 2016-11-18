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
*  MODULE NAME:  eda_util.c                                                  *
*                                                                            *
*                                                                            *
*                                                                            *
*  DESCRIPTION                                                               *
*  This module contains utility routines for the NCS Event Service library.  *
*                                                                            *
*****************************************************************************/
#include "evt/agent/eda.h"

/****************************************************************************
  Name          : eda_find_mark_channel_to_ulink
 
  Description   : This routine looks up a channel to unlink based on its
                  name and unlink status and marks it as unlinked.
 
  Arguments     : cb      - ptr to the EDA_CLIENT_HDL_REC
                  SaNameT   *channelName

  Return Values : pointer to EDA_CHANNEL_HDL_REC
 
  Notes         : None
******************************************************************************/
int32_t eda_find_mark_channel_to_ulink(EDA_CLIENT_HDL_REC *eda_hdl_rec, const SaNameT *channelName)
{
	EDA_CHANNEL_HDL_REC *chan_hdl_rec;
	int32_t chan_count_marked = 0;
	TRACE_ENTER2("channel: %s", channelName->value);

	for (chan_hdl_rec = eda_hdl_rec->chan_list; chan_hdl_rec != NULL; chan_hdl_rec = chan_hdl_rec->next) {

		if ((false == chan_hdl_rec->ulink)
		    && (chan_hdl_rec->channel_name.length == channelName->length)
		    && (memcmp(chan_hdl_rec->channel_name.value, channelName->value, channelName->length) == 0)) {
	/** Mark the channel to be unlinked 
         ** as true.
         **/
			chan_hdl_rec->ulink = true;
			chan_count_marked++;
		}
	}

   /** Return if any channel was unlinked 
    ** that has not been already unlinked. 
    **/
	TRACE_LEAVE2("chan_count_marked: %d", chan_count_marked);
	return chan_count_marked;
}

/****************************************************************************
  Name          : eda_validate_eda_client_hdl
 
  Description   : This routine looks up a eda_client_hdl_rec by reg_id
 
  Arguments     : cb      - ptr to the EDA_CHANNEL_HDL_REC
                  sub_id  - subscription id

  Return Values : pointer to EDA_SUBSC_REC
 
  Notes         : None
******************************************************************************/
bool eda_validate_eda_client_hdl(EDA_CB *eda_cb, EDA_CLIENT_HDL_REC *find_hdl_rec)
{
	EDA_CLIENT_HDL_REC *hdl_rec;
	TRACE_ENTER();

	for (hdl_rec = eda_cb->eda_init_rec_list; hdl_rec != NULL; hdl_rec = hdl_rec->next) {
		if (hdl_rec == find_hdl_rec) {
			TRACE_LEAVE2("true");
			return true;
		}
	}

	/* No subscription record match found */
	TRACE_LEAVE2("false");
	return false;
}

/****************************************************************************
  Name          : eda_find_subsc_by_subsc_id
 
  Description   : This routine looks up a subscription rec by subscription id.
 
  Arguments     : cb      - ptr to the EDA_CHANNEL_HDL_REC
                  sub_id  - subscription id

  Return Values : pointer to EDA_SUBSC_REC
 
  Notes         : None
******************************************************************************/
EDA_SUBSC_REC *eda_find_subsc_by_subsc_id(EDA_CHANNEL_HDL_REC *chan_hdl_anc, uint32_t sub_id)
{
	EDA_SUBSC_REC *subsc_rec;
	TRACE_ENTER();

	for (subsc_rec = chan_hdl_anc->subsc_list; subsc_rec != NULL; subsc_rec = subsc_rec->next) {
		if (subsc_rec->subsc_id == sub_id) {
			TRACE_LEAVE();
			return subsc_rec;
		}
	}

	/* No subscription record match found */
	TRACE_LEAVE2("null");
	return NULL;
}

/****************************************************************************
  Name          : eda_find_hdl_rec_by_regid
 
  Description   : This routine looks up a eda_client_hdl_rec by reg_id
 
  Arguments     : cb      - ptr to the EDA control block
                  reg_id  - cluster wide unique allocated by EDS

  Return Values : EDA_CLIENT_HDL_REC * or NULL
 
  Notes         : None
******************************************************************************/
EDA_CLIENT_HDL_REC *eda_find_hdl_rec_by_regid(EDA_CB *eda_cb, uint32_t reg_id)
{
	EDA_CLIENT_HDL_REC *eda_hdl_rec;
	TRACE_ENTER2("reg_id: %u", reg_id);

	for (eda_hdl_rec = eda_cb->eda_init_rec_list; eda_hdl_rec != NULL; eda_hdl_rec = eda_hdl_rec->next) {
		if (eda_hdl_rec->eds_reg_id == reg_id) {
			TRACE_LEAVE2("record found");
			return eda_hdl_rec;
		}
	}

	TRACE_LEAVE2("null");
	return NULL;
}

/****************************************************************************
  Name          : eda_find_chan_hdl_rec_by_chan_id
 
  Description   : This routine looks up a eda_clinet_hdl_rec by 
                  chan_id & chan_open_id
 
  Arguments     : cb      - ptr to the EDA control block
                  reg_id  - cluster wide unique allocated by EDS

  Return Values : EDA_CHANNEL_HDL_REC * or NULL
 
  Notes         : None
******************************************************************************/
EDA_CHANNEL_HDL_REC *eda_find_chan_hdl_rec_by_chan_id(EDA_CLIENT_HDL_REC *eda_hdl_rec,
						      uint32_t chan_id, uint32_t chan_open_id)
{
	EDA_CHANNEL_HDL_REC *chan_hdl_rec;
	TRACE_ENTER();

	for (chan_hdl_rec = eda_hdl_rec->chan_list; chan_hdl_rec != NULL; chan_hdl_rec = chan_hdl_rec->next) {
		if (chan_hdl_rec->eds_chan_id == chan_id && chan_hdl_rec->eds_chan_open_id == chan_open_id) {
			TRACE_LEAVE2("record found");
			return chan_hdl_rec;
		}
	}

	TRACE_LEAVE2("null");
	return NULL;
}

/****************************************************************************
 * Name          : eda_msg_destroy
 *
 * Description   : This is the function which is called to destroy an EDSV msg.
 *
 * Arguments     : EDSV_MSG *.
 *
 * Return Values : NONE
 *
 * Notes         : None.
 *****************************************************************************/
void eda_msg_destroy(EDSV_MSG *msg)
{
	TRACE_ENTER();

	if (EDSV_EDA_API_MSG == msg->type) {
		TRACE("msg type: %u, msg subtype: %u",  msg->type, msg->info.api_info.type);
		if (EDSV_EDA_PUBLISH == msg->info.api_info.type) {
			/* free pattern array */
			if (NULL != msg->info.api_info.param.publish.pattern_array)
				m_MMGR_FREE_EVENT_PATTERN_ARRAY(msg->info.api_info.param.publish.pattern_array);

			/* free event data */
			if (NULL != msg->info.api_info.param.publish.data)
				m_MMGR_FREE_EDSV_EVENT_DATA(msg->info.api_info.param.publish.data);
		} else if (EDSV_EDA_SUBSCRIBE == msg->info.api_info.type) {
			/* free the filter_array */
			if (NULL != msg->info.api_info.param.subscribe.filter_array)
				m_MMGR_FREE_FILTER_ARRAY(msg->info.api_info.param.subscribe.filter_array);
		}
	}

  /** There are no other pointers 
   ** off the evt, so free the evt
   **/
	m_MMGR_FREE_EDSV_MSG(msg);
	msg = NULL;

	TRACE_LEAVE();
	return;
}
