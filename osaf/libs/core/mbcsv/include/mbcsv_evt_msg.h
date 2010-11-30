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

  
    
      
        REVISION HISTORY:
        
          
            DESCRIPTION:
            
              This module contains declarations and structures related to MBCSv.
              
*******************************************************************************/

/*
* Module Inclusion Control...
*/
#ifndef MBCSV_EVT_MSG_H
#define MBCSV_EVT_MSG_H

#include "mbcsv.h"

#define NCS_MBCSV_MSG_SYNC_SEND_RSP     (NCSMBCSV_EVENT_NOTIFY + 1)
/***********************************************************************************
*
*                        MBCSV event information members
* Format of the message exhanged between two MBCSv peers.
*
***********************************************************************************/
typedef enum {
	MBCSV_EVT_MDS_SUBSCR,
	MBCSV_EVT_TMR,
	MBCSV_EVT_INTERNAL,
} MBCSV_EVT_TYPE;

typedef struct mbcsv_evt_tmr_info {
	uns32 peer_inst_hdl;
	TIMER_TYPE_ENUM type;
} MBCSV_EVT_TMR_INFO;

typedef struct mbcsv_evt_mds_subscr_info {
	NCSMDS_CHG evt_type;
} MBCSV_EVT_MDS_SUBSCR_INFO;

typedef enum {
	MBCSV_EVT_INTERNAL_CLIENT,
	MBCSV_EVT_INTERNAL_PEER_DISC,
	MBCSV_EVT_CHG_ROLE,
	MBCSV_EVT_MBC_ASYNC_SEND
} MBCSV_EVT_INTERNAL_TYPE;

typedef struct mbcsv_client_msg {
	union {
		uns32 raw;
		NCS_MBCSV_MSG_TYPE msg_sub_type;
		NCSMBCSV_EVENTS evt_type;
	} type;
	NCS_UBAID uba;
	NCS_MBCSV_ACT_TYPE action;
	NCS_MBCSV_SND_TYPE snd_type;
	uns32 reo_type;
	uns32 first_rsp;	/* Field indicate that this in first responce message
				   sent in the sequences of responces. Meaningful only
				   in the case of cold, warm and data resp */
} MBCSV_CLIENT_MSG;

typedef struct mbcsv_chg_role_req {
	uns32 ckpt_hdl;
	SaAmfHAStateT new_role;
} MBCSV_CHG_ROLE_REQ;

/***********************************************************************************
*
*                        Peer Discovery  messages
*                Peer discovery messages sent for peer discovery.
*
***********************************************************************************/

/* PEER UP Message */
typedef struct mbcsv_peer_up {
	uns16 peer_version;	/* Software version of the peer */
} MBCSV_PEER_UP;

/* PEER DOWN Message */
typedef struct mbcsv_peer_down {
	uns8 dummy;
} MBCSV_PEER_DOWN;

/* PEER INFO Message */
typedef struct mbcsv_peer_info {
	uns16 peer_version;	/* Software version of the peer */
	uns32 my_peer_inst_hdl;
	uns8 compatible;	/* Flag to tell whether peer is compatible */
} MBCSV_PEER_INFO;

/* PEER INFO RSP Message */
typedef struct mbcsv_peer_info_rsp {
	uns16 peer_version;	/* Software version of the peer */
	uns32 my_peer_inst_hdl;
	uns8 compatible;	/* Flag to tell whether peer is compatible */
} MBCSV_PEER_INFO_RSP;

/* PEER CHG ROLE Message */
typedef struct mbcsv_peer_chg_role {
	uns16 peer_version;	/* Software version of the peer */
} MBCSV_PEER_CHG_ROLE;

typedef enum {
	MBCSV_PEER_UP_MSG,
	MBCSV_PEER_DOWN_MSG,
	MBCSV_PEER_INFO_MSG,
	MBCSV_PEER_INFO_RSP_MSG,
	MBCSV_PEER_CHG_ROLE_MSG,
} PEER_EVT_TYPE;

/***********************************************************************************
*
*                        Peer Event message
*   Peer event data contains information regarding peer entity.
*
***********************************************************************************/
typedef struct mbcsv_peer_disc_msg {
	PEER_EVT_TYPE msg_sub_type;
	SaAmfHAStateT peer_role;	/* Will contain role info */

	union {
		MBCSV_PEER_UP peer_up;
		MBCSV_PEER_DOWN peer_down;
		MBCSV_PEER_INFO peer_info;
		MBCSV_PEER_INFO_RSP peer_info_rsp;
		MBCSV_PEER_CHG_ROLE peer_chg_role;
	} info;

} MBCSV_PEER_DISC_MSG;

typedef struct mbcsv_internal_msg {
	MBCSV_EVT_INTERNAL_TYPE type;

	union {
		MBCSV_PEER_DISC_MSG peer_disc;
		MBCSV_CLIENT_MSG client_msg;
		NCS_MBCSV_SEND_CKPT usr_msg_info;
		MBCSV_CHG_ROLE_REQ chg_role;
	} info;

} MBCSV_INTERNAL_MSG;

typedef struct pwe_svc_id {
	uns32 pwe_hdl;
	SS_SVC_ID svc_id;
	MBCSV_ANCHOR peer_anchor;
} PWE_SVC_ID;

/***********************************************************************************
*
*                        MBCSV event information 
* Format of the message exhanged between two MBCSv peers.
*
***********************************************************************************/
typedef struct peer_inst_key {
	SS_SVC_ID svc_id;
	uns32 pwe_hdl;
	MBCSV_ANCHOR peer_anchor;
	uns32 peer_inst_hdl;
} PEER_INST_KEY;

typedef struct mbcsv_evt {
	struct mbcsv_evt *next;

	/* A key to identify the checkpoint-instance for which the event is
	   being generated. It is populated as follows:
	 */
	PEER_INST_KEY rcvr_peer_key;
	MDS_SYNC_SND_CTXT msg_ctxt;
	/* Message related info */
	MBCSV_EVT_TYPE msg_type;	/*Peer discovery msg OR client info msg */

	union {
		MBCSV_INTERNAL_MSG peer_msg;
		MBCSV_EVT_TMR_INFO tmr_evt;
		MBCSV_EVT_MDS_SUBSCR_INFO mds_sub_evt;
	} info;

} MBCSV_EVT;

/***********************************************************************************

  MBCSv Mailbox manipulation macros
  
***********************************************************************************/
#define m_MBCSV_RCV_MSG(mbx)            (struct mbcsv_evt *)m_NCS_IPC_NON_BLK_RECEIVE(mbx,NULL)
#define m_MBCSV_SND_MSG(mbx, msg, pri)  m_NCS_IPC_SEND(mbx, msg, pri)
#define m_MBCSV_BLK_RCV_MSG(mbx)        (struct mbcsv_evt *)m_NCS_IPC_RECEIVE(mbx,NULL)

#define m_MBCSV_SEND_CLIENT_MSG(peer, evt, act) \
mbcsv_send_client_msg(peer, evt, act)

#endif
