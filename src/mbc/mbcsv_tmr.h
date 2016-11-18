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
   This module contains declarations and structures related to MBCSv timers.

*******************************************************************************/

/*
 * Module Inclusion Control...
 */
#ifndef MBC_MBCSV_TMR_H_
#define MBC_MBCSV_TMR_H_

#include "mbcsv.h"
#include "base/ncssysf_tmr.h"

/*
 * Bit masks for timers
 */
#define NCS_MBCSV_TMR_CLEAR_ALL_BIT          0x00
#define NCS_MBCSV_TMR_SEND_COLD_SYNC_BIT     0x01       /* Send cold sync when timer expires */
#define NCS_MBCSV_TMR_SEND_WARM_SYNC_BIT     0x02       /* Send warm sync when timer expires */
#define NCS_MBCSV_TMR_COLD_SYNC_CMPLT_BIT    0x04       /* Warn target that cold sync not cmplt */
#define NCS_MBCSV_TMR_WARM_SYNC_CMPLT_BIT    0x08       /* Watchdog for warm sync responses */
#define NCS_MBCSV_TMR_DATA_RESP_CMPLT_BIT    0x10       /* Watchdog for data requests */
#define NCS_MBCSV_TMR_TRANSMIT_BIT           0x20       /* The transmit timer */
#define NCS_MBCSV_TMR_GRAVEYARD_BIT          0x40       /* The graveyard timer */

/*
 * Timer periods for MBCSv
 */
#define NCS_MBCSV_TMR_SEND_COLD_SYNC_PERIOD       900   /* Send cold sync when timer expires */
#define NCS_MBCSV_TMR_SEND_WARM_SYNC_PERIOD      6000   /* Send warm sync when timer expires */
#define NCS_MBCSV_TMR_COLD_SYNC_CMPLT_PERIOD   360000   /* Warn target that cold sync not cmplt */
#define NCS_MBCSV_TMR_WARM_SYNC_CMPLT_PERIOD    60000   /* Watchdog for warm sync responses */
#define NCS_MBCSV_TMR_DATA_RESP_CMPLT_PERIOD    60000   /* Watchdog for data requests */
#define NCS_MBCSV_TMR_TRANSMIT_PERIOD              10   /* The transmit timer; very brief pause */

#define NCS_MBCSV_MIN_SEND_WARM_SYNC_TIME        1000   /*Minimum value can be set */
#define NCS_MBCSV_MAX_SEND_WARM_SYNC_TIME      360000   /*Maximum value can be set */

/* type to house NCS_MBCSV_TMR handle(xdb) */
typedef unsigned long NCS_MBCSV_TMR_HDL;

/*
 * NCS_MBCSV TIMER INFO STRUCTURE
 */
/* Caution - Do *NOT* perform encode/decode or processing based upon the sizeof
 * of the memeber xdb in the following structure.
 * xdb is a placeholder for user handles(from handle manager) and pointers.
 */
typedef struct ncs_mbcsv_tmr {
  tmr_t tmr_id;
  NCS_MBCSV_TMR_HDL xdb;
  uint32_t period;
  uint16_t is_active;
  uint8_t curr_exp_count;
  uint8_t has_expired;
  uint8_t type;
} NCS_MBCSV_TMR;

typedef struct ncs_mbcsv_tmr_db {
  char name[12];
  TMR_CALLBACK cb_func;
  uint8_t event;
} NCS_MBCSV_TMR_DB;

/*
 * Timer types for MBCSv
 */
typedef enum {
  NCS_MBCSV_TMR_SEND_COLD_SYNC = 0,
  NCS_MBCSV_TMR_SEND_WARM_SYNC = 1,
  NCS_MBCSV_TMR_COLD_SYNC_CMPLT = 2,
  NCS_MBCSV_TMR_WARM_SYNC_CMPLT = 3,
  NCS_MBCSV_TMR_DATA_RESP_CMPLT = 4,
  NCS_MBCSV_TMR_TRANSMIT = 5,
} TIMER_TYPE_ENUM;

#define NCS_MBCSV_MAX_TMRS               6

/***********************************************************************************

  TIMER MACROS

***********************************************************************************/

#define m_INIT_NCS_MBCSV_TMR(i,t,p)             \
  {                                             \
    (i)->tmr[t].is_active      = false;         \
    (i)->tmr[t].has_expired    = false;         \
    (i)->tmr[t].curr_exp_count = 0;             \
    (i)->tmr[t].tmr_id         = TMR_T_NULL;    \
    (i)->tmr[t].period         = p;             \
    (i)->tmr[t].type           = t;             \
    (i)->tmr[t].xdb            = (i)->hdl;      \
  }

#define m_SET_NCS_MBCSV_TMR_INACTIVE(i,t)  ((i)->tmr[t].is_active = false)

#define m_SET_NCS_MBCSV_TMR_ACTIVE(i,t)    ((i)->tmr[t].is_active = true)

#define m_IS_TMR_NCS_MBCSV_ACTIVE(i,t)     ((i)->tmr[t].is_active == true)

/*
 * The following macros are used to confirm that between the time a
 * timer event expired and the time the action routine is actually
 * serviced, the timer-action is still valid.
 */
#define m_SET_NCS_MBCSV_TMR_EXP_OFF(i,t)  ((i)->tmr[t].has_expired = false)

#define m_SET_NCS_MBCSV_TMR_EXP_ON(i,t)   ((i)->tmr[t].has_expired = true)

#define m_IS_NCS_MBCSV_TMR_EXP_VALID(i,t) ((i)->tmr[t].has_expired == true)

#endif  // MBC_MBCSV_TMR_H_
