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
.............................................................................

  DESCRIPTION:

  EDA CB related definitions.
  
*******************************************************************************/
#ifndef EVT_AGENT_EDA_CB_H_
#define EVT_AGENT_EDA_CB_H_

#include "clm/saf/saClm.h"

#include "evt/agent/eda.h"

/*  EDA control block is the master anchor structure for all
 *  EDA instantiations within a process
 */

struct event_handle_rec_tag;
struct eda_subsc_rec_tag;
struct eda_channel_hdl_rec_tag;
struct eda_client_hdl_rec_tag;

#define EDA_EVT_PUBLISHED 0x1
#define EDA_EVT_RECEIVED  0x2

typedef struct event_handle_rec_tag {
	uint32_t event_hdl;	/* The hdl for this event allocated by the hdl-mgr */
	uint8_t priority;
	SaTimeT retention_time;
	SaTimeT publish_time;
	SaNameT publisher_name;
	SaEvtEventPatternArrayT *pattern_array;
	SaSizeT event_data_size;	/* size of the evt data */
	uint8_t *evt_data;		/* Event-specific data  */
	uint8_t evt_type;		/* published or rcvd     */
	struct eda_channel_hdl_rec_tag *parent_chan;
	struct event_handle_rec_tag *next;
	uint32_t pub_evt_id;	/*The event ID sent to EDS ias part of Publish */
	uint32_t del_evt_id;	/* The event ID got from EDS as part of Delivery to this EDA */

} EDA_EVENT_HDL_REC;

typedef struct eda_subsc_rec_tag {
	uint32_t subsc_id;
	struct eda_subsc_rec_tag *next;
} EDA_SUBSC_REC;

/* Channel Handle Definition */
typedef struct eda_channel_hdl_rec_tag {
	uint32_t channel_hdl;	/* Channel HDL from handle mgr */
	SaNameT channel_name;	/* channel name mentioned during open channel */
	uint32_t open_flags;	/* channel open flags as defined in AIS.01.01 */
	uint32_t eds_chan_id;	/* server reference for this channel */
	uint32_t eds_chan_open_id;	/* server reference for this instance of channel open */
	uint32_t last_pub_evt_id;	/* Last Published event ID */
	uint8_t ulink;
	struct event_handle_rec_tag *chan_event_anchor;
	struct eda_subsc_rec_tag *subsc_list;	/* List of subscriptions off this channel */
	struct eda_channel_hdl_rec_tag *next;
	struct eda_client_hdl_rec_tag *parent_hdl;	/* Back Pointer to the of the EDA client instantiation */
} EDA_CHANNEL_HDL_REC;

/* EDA handle database records */
typedef struct eda_client_hdl_rec_tag {
	uint32_t eds_reg_id;	/* handle value returned by EDS for this instantiation */
	uint32_t local_hdl;	/* EVT handle (derived from hdl-mngr) */
	SaVersionT version;
	SaEvtCallbacksT reg_cbk;	/* callbacks registered by the application */
	EDA_CHANNEL_HDL_REC *chan_list;
	SYSF_MBX mbx;		/* priority q mbx b/w MDS & Library */
	struct eda_client_hdl_rec_tag *next;
} EDA_CLIENT_HDL_REC;

typedef struct eda_eds_intf_tag {
	MDS_HDL mds_hdl;	/* mds handle */
	MDS_DEST eda_mds_dest;	/* EDA absolute address */
	MDS_DEST eds_mds_dest;	/* EDS absolute/virtual address */
	bool eds_up;	/* Boolean to indicate that MDS subscription
				 * is complete
				 */
	bool eds_up_publish;	/* Boolean to indicate that EDS is down */
} EDA_EDS_INTF;

typedef struct eda_cb_tag {
	uint32_t cb_hdl;		/* CB hdl returned by hdl mngr */
	uint8_t pool_id;		/* pool-id used by hdl mngr */
	uint32_t prc_id;		/* process identifier */
	EDA_EDS_INTF eds_intf;	/* EDS interface (mds address & hdl) */
	NCS_LOCK cb_lock;	/* CB lock */
	EDA_CLIENT_HDL_REC *eda_init_rec_list;	/* EDA client handle database */
	/* EDS EDA sync params */
	bool eds_sync_awaited;
	NCS_LOCK eds_sync_lock;
	NCS_SEL_OBJ eds_sync_sel;
	SaClmClusterChangesT node_status;
} EDA_CB;

/*** Extern function declarations ***/

uint32_t ncs_eda_lib_req(NCS_LIB_REQ_INFO *);
unsigned int ncs_eda_startup(void);
unsigned int ncs_eda_shutdown(void);
uint32_t eda_create(NCS_LIB_CREATE *);
void eda_destroy(NCS_LIB_DESTROY *);
bool eda_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg);

#endif  // EVT_AGENT_EDA_CB_H_
