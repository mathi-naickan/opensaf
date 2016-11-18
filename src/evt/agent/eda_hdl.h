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

  EDA handle database related definitions.
  
*******************************************************************************/

#ifndef EVT_AGENT_EDA_HDL_H_
#define EVT_AGENT_EDA_HDL_H_

#include "evt/agent/eda.h"

uint32_t eda_hdl_cbk_dispatch(EDA_CB *, EDA_CLIENT_HDL_REC *, SaDispatchFlagsT);

EDA_CLIENT_HDL_REC *eda_hdl_rec_add(EDA_CB **eda_cb,
					     const SaEvtCallbacksT *reg_cbks, uint32_t reg_id, SaVersionT version);

EDA_CHANNEL_HDL_REC *eda_channel_hdl_rec_add(EDA_CLIENT_HDL_REC **hdl_rec,
						      uint32_t chan_id,
						      uint32_t chan_open_id,
						      uint32_t channel_open_flags, const SaNameT *channelName);

EDA_EVENT_HDL_REC *eda_event_hdl_rec_add(EDA_CHANNEL_HDL_REC **);

void eda_hdl_list_del(EDA_CLIENT_HDL_REC **);
uint32_t eda_hdl_rec_del(EDA_CLIENT_HDL_REC **, EDA_CLIENT_HDL_REC *);
uint32_t eda_channel_hdl_rec_del(EDA_CHANNEL_HDL_REC **, EDA_CHANNEL_HDL_REC *);
uint32_t eda_del_subsc_rec(EDA_SUBSC_REC **list_head, EDA_SUBSC_REC *rm_node);
uint32_t eda_event_hdl_rec_del(EDA_EVENT_HDL_REC **, EDA_EVENT_HDL_REC *);

int32_t eda_find_mark_channel_to_ulink(EDA_CLIENT_HDL_REC *eda_hdl_rec, const SaNameT *channelName);

bool eda_validate_eda_client_hdl(EDA_CB *eda_cb, EDA_CLIENT_HDL_REC *find_hdl_rec);

EDA_SUBSC_REC *eda_find_subsc_by_subsc_id(EDA_CHANNEL_HDL_REC *, uint32_t sub_id);

EDA_CLIENT_HDL_REC *eda_find_hdl_rec_by_regid(EDA_CB *eda_cb, uint32_t reg_id);

EDA_CHANNEL_HDL_REC *eda_find_chan_hdl_rec_by_chan_id(EDA_CLIENT_HDL_REC *eda_hdl_rec,
							       uint32_t chan_id, uint32_t chan_open_id);

void eda_msg_destroy(EDSV_MSG *msg);

uint32_t eda_extract_pattern_from_event(SaEvtEventPatternArrayT *from_pattern_array,
					      SaEvtEventPatternArrayT **to_pattern_array);

uint32_t eda_allocate_and_extract_pattern_from_event(SaEvtEventPatternArrayT *from_pattern_array,
							   SaEvtEventPatternArrayT **to_pattern_array);

#endif  // EVT_AGENT_EDA_HDL_H_
