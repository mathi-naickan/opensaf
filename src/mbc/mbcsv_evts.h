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

  Date     Version  Name          Description
  -------- -------  ------------  --------------------------------------------
  07-13-98 1.00A    H&J (PB)      Original

..............................................................................

  DESCRIPTION:

  This module contains MBC Event related information.

*******************************************************************************/

/*
 * Module Inclusion Control...
 */
#ifndef MBC_MBCSV_EVTS_H_
#define MBC_MBCSV_EVTS_H_

typedef enum ncsmbcsv_events {

  NCSMBCSV_EVENT_NULL = 0,

  /*
   *  Events associated with the MBCSv FSM Engine
   ***********************************************************
   ***********************************************************
   *
   * The events determine the nature of the packets to be generated
   * as a response and the actions to be taken on either side (ACTIVE
   * or STANDBY).
   */

  /* Unsolicited update from ACTIVE to keep the STANDBY synchronized */
  /* Direction: ACTIVE ----> STANDBY */
  NCSMBCSV_EVENT_ASYNC_UPDATE = 0x01,

  /* Initial request for flood of updates so STANDBY can synchronize itself  */
  /* Direction: STANDBY ----> ACTIVE */
  NCSMBCSV_EVENT_COLD_SYNC_REQ = 0x02,

  /* Response to a cold sync request, this could be one of many packets */
  /* Direction: ACTIVE ----> STANDBY */
  NCSMBCSV_EVENT_COLD_SYNC_RESP = 0x03,

  /* This should be the last of a multi-part response to a cold sync req */
  /* Direction: ACTIVE ----> STANDBY */
  NCSMBCSV_EVENT_COLD_SYNC_RESP_COMPLETE = 0x04,

  /* Periodical optional request to verify that STANDBY is synchronized */
  /* Direction: STANDBY ----> ACTIVE */
  NCSMBCSV_EVENT_WARM_SYNC_REQ = 0x05,

  /* Response to a warm sync request, this could be one of many packets */
  /* Direction: ACTIVE ----> STANDBY */
  NCSMBCSV_EVENT_WARM_SYNC_RESP = 0x06,

  /* This should be the last of a multi-part response to a warm sync req */
  /* Direction: ACTIVE ----> STANDBY */
  NCSMBCSV_EVENT_WARM_SYNC_RESP_COMPLETE = 0x07,

  /* Request for data when STANDBY is out-of-sync */
  /* Direction: STANDBY ----> ACTIVE */
  NCSMBCSV_EVENT_DATA_REQ = 0x08,

  /* Response to a data request, this could be one of many packets */
  /* Direction: ACTIVE ----> STANDBY */
  NCSMBCSV_EVENT_DATA_RESP = 0x09,

  /* This should be the last of a multi-part response to a data req */
  /* Direction: ACTIVE ----> STANDBY */
  NCSMBCSV_EVENT_DATA_RESP_COMPLETE = 0x0A,

  /* This message may be received in any state.  It is completely */
  /* transparent to the MBC protocol */
  /* Bidirectional */
  NCSMBCSV_EVENT_NOTIFY = 0x0B,

  /** Locally generated events
   **/
  NCSMBCSV_SEND_COLD_SYNC_REQ,    /* 12 */
  NCSMBCSV_SEND_WARM_SYNC_REQ,    /* 13 */
  NCSMBCSV_SEND_ASYNC_UPDATE,     /* 14 */
  NCSMBCSV_SEND_DATA_REQ, /* 15 */
  NCSMBCSV_SEND_COLD_SYNC_RESP,   /* 16 */
  NCSMBCSV_SEND_COLD_SYNC_RESP_COMPLETE,  /* 17 */
  NCSMBCSV_SEND_WARM_SYNC_RESP,   /* 18 */
  NCSMBCSV_SEND_WARM_SYNC_RESP_COMPLETE,  /* 19 */
  NCSMBCSV_SEND_DATA_RESP,        /* 20 */
  NCSMBCSV_SEND_DATA_RESP_COMPLETE,       /* 21 */
  NCSMBCSV_ENTITY_IN_SYNC,        /* 22 */
  NCSMBCSV_SEND_NOTIFY,   /* 23 */

  /** Timer expiration events
   **/
  NCSMBCSV_EVENT_TMR_MIN, /* 24 */
  NCSMBCSV_EVENT_TMR_SEND_COLD_SYNC = NCSMBCSV_EVENT_TMR_MIN,     /* 24 */
  NCSMBCSV_EVENT_TMR_SEND_WARM_SYNC,      /* 25 */
  NCSMBCSV_EVENT_TMR_COLD_SYNC_CMPLT,     /* 26 */
  NCSMBCSV_EVENT_TMR_WARM_SYNC_CMPLT,     /* 27 */
  NCSMBCSV_EVENT_TMR_DATA_RESP_CMPLT,     /* 28 */
  NCSMBCSV_EVENT_TMR_TRANSMIT,    /* 29 */

  /* Peer discovery events */
  NCSMBCSV_EVENT_MULTIPLE_ACTIVE, /* 30 */
  NCSMBCSV_EVENT_STATE_TO_WAIT_FOR_CW_SYNC,       /* 31 */
  NCSMBCSV_EVENT_STATE_TO_KEEP_STBY_SYNC, /* 32 */

  NCSMBCSV_NUM_EVENTS     /* The De-limiter */
} NCSMBCSV_EVENTS;

#define NCSMBCSV_MAX_MESSAGE_TYPE   11
#define NCSMBCSV_MAX_SUBSCRIBE_EVT  NCSMBCSV_NUM_EVENTS

#endif  // MBC_MBCSV_EVTS_H_
