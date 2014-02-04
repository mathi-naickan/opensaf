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

#ifndef FM_EVT_H
#define FM_EVT_H

/* EVT from other GFM over MDS.*/
typedef enum {
	GFM_GFM_EVT_NODE_INFO_EXCHANGE,
	GFM_GFM_EVT_MAX
} GFM_GFM_MSG_TYPE;

typedef struct gfm_node_info {
	NODE_ID node_id;
	SaNameT node_name;
} GFM_NODE_INFO;

typedef struct gfm_gfm_msg {
	GFM_GFM_MSG_TYPE msg_type;

	union {
		GFM_NODE_INFO node_info;
	} info;

} GFM_GFM_MSG;

typedef struct fm_rda_info_t {
	PCS_RDA_ROLE role;
} FM_RDA_INFO;

/* FM generated events.*/
typedef enum {
	FM_EVT_TMR_EXP,
	FM_EVT_NODE_DOWN,
	FM_EVT_PEER_UP,
	FM_EVT_RDA_ROLE,
	FM_EVT_SVC_DOWN,
	FM_FSM_EVT_MAX
} FM_FSM_EVT_CODE;

typedef struct fm_evt {
	struct fm_evt *next;
	NODE_ID node_id;
	FM_FSM_EVT_CODE evt_code;
	NCS_IPC_PRIORITY priority;
	MDS_SYNC_SND_CTXT mds_ctxt;
	MDS_DEST fr_dest;
	NCSMDS_SVC_ID svc_id;
	union {
		FM_TMR *fm_tmr;
		GFM_GFM_MSG gfm_msg;
		FM_RDA_INFO rda_info;
	} info;
} FM_EVT;

void fm_mbx_evt_handler(FM_CB *fm_cb, FM_EVT *fm_evt);

#endif
