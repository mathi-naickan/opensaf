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

  This module contains declarations related to the target system Timer Services.

..............................................................................
*/

/*
 * Module Inclusion Control...
 */
#ifndef BASE_NCSSYSF_TMR_H_
#define BASE_NCSSYSF_TMR_H_

#include "base/ncsgl_defs.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  @
  @              TIMER SUPPORT
  @
  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

/*  10 milli-second timer tick  */

#define SYSF_TMR_TICKS 100      /* seconds <-->ticks    */
#define SYSF_TMR_SCALE 10       /* milli-seconds <--> ticks */

typedef void *tmr_t;

#define TMR_T_NULL  ((tmr_t*)0)

typedef void (*TMR_CALLBACK) (void *);

extern uint32_t gl_tmr_milliseconds;


#ifndef NCS_TMR_STACKSIZE
#define NCS_TMR_STACKSIZE     NCS_STACKSIZE_HUGE
#endif

/** Target system timer support functions...
 **/

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  @
  @              MACRO DEFINITIONS used by TMR Clients
  @
  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

#define m_NCS_TMR_CREATE(tid,prd,cb,arg)   (tid=ncs_tmr_alloc(__FILE__,__LINE__))
#define m_NCS_TMR_START(tid,prd,cb,arg)                 \
  (tid=ncs_tmr_start(tid,prd,cb,arg,__FILE__,__LINE__))

#define m_NCS_TMR_STOP(tid)                      ncs_tmr_stop(tid)
#define m_NCS_TMR_STOP_V2(tid,tmr_arg)            ncs_tmr_stop_v2(tid,tmr_arg)
#define m_NCS_TMR_DESTROY(tid)                   ncs_tmr_free(tid)
#define m_NCS_TMR_MSEC_REMAINING(tid, p_tleft)   ncs_tmr_remaining(tid, p_tleft)

#define m_NCS_TMR_MILLISECONDS  gl_tmr_milliseconds

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  @
  @              FUNCTION PROTOTYPES
  @
  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

tmr_t ncs_tmr_alloc(char *, uint32_t);
tmr_t ncs_tmr_start(tmr_t, int64_t, TMR_CALLBACK, void *, char *, uint32_t);
uint32_t ncs_tmr_stop_v2(tmr_t, void **);
void ncs_tmr_stop(tmr_t);
void ncs_tmr_free(tmr_t);
int64_t ncs_tmr_remaining(tmr_t, int64_t *);

/* Keep old names for Create  and Destroy, as many places call these functions */

bool sysfTmrCreate(void);
bool sysfTmrDestroy(void);

/* For now, I/O is done internally.. Later we can export data and do I/O outside */

uint32_t ncs_tmr_whatsout(void);
uint32_t ncs_tmr_getstats(void);

#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCSSYSF_TMR_H_
