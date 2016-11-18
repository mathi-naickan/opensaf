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

  DESCRIPTION: Contains structures related to RP timer libirary

  ******************************************************************************
  */

#ifndef BASE_NCS_TMR_H_
#define BASE_NCS_TMR_H_

#include "base/ncsgl_defs.h"
#include "base/ncs_osprm.h"
#include "base/ncs_svd.h"
#include "base/ncssysf_def.h"
#include "base/ncssysf_lck.h"
#include "base/ncssysfpool.h"
#include "base/ncssysf_tmr.h"
#include "base/ncssysf_mem.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef NCSCONTEXT NCS_RP_TMR_HDL;      /* Datatype is opaque to user */

/* 1 sec default */
#define m_RP_TMR_DEF_PERIOD (1)

typedef void (*RP_TMR_CALLBACK) (void *);

typedef struct ncs_rp_tmr_cb_tag {
  uint32_t tmr_ganularity;        /* minimum granularity is 1 sec */
  tmr_t tmr_id;   /* Maps to OS timer ID */
  TMR_CALLBACK tmr_callback;      /* call back which will be used to registered with the OS timer */
  void *callback_arg;     /* argument for the above call back */
  NCS_SERVICE_ID svc_id;  /* service id of the RP */
  uint32_t svc_sub_id;    /* sub id for the above service ID */
  struct ncs_rp_tmr_info_tag *start_list; /* hold the pointer of the starting tmr list */
  time_t last_trig_tmr;   /* this will hold the time stamp of last trigered value */
  bool active;
  NCS_LOCK tmr_lock;
} NCS_RP_TMR_CB;

typedef struct ncs_rp_tmr_info_tag {
  struct ncs_rp_tmr_info_tag *pnext;
  struct ncs_rp_tmr_info_tag *pprev;
  RP_TMR_CALLBACK rp_tmr_callback;        /* this is the timer call back which the RP has to register, which will be invoked when the particular timer expires */
  void *callback_arg;     /* this is the argument, which is used by the above callback */
  int64_t tmr_value;     /* this is the tmr value which the RP needs to register for */
  time_t tmr_trig_at;     /* this is the system timer, exactly when it has trigered this timer */
} NCS_RP_TMR_INFO;

typedef struct ncs_rp_tmr_init_tag {
  uint32_t tmr_ganularity;
  TMR_CALLBACK tmr_callback;
  void *callback_arg;
  NCS_SERVICE_ID svc_id;
  uint32_t svc_sub_id;
} NCS_RP_TMR_INIT;


#undef RP_LOG_ENB
#ifdef RP_LOG_ENB
#define m_RP_TMR_LOG_MSG(str, val) printf("%s :: %x\n", str, val);
#else
#define m_RP_TMR_LOG_MSG(str, val)
#endif
/* m_NCS_RP_TMR_INIT should be done only once, when the system is comming up.
 * This macro will init the RP timer, cb. This will initalize the locks which
 * it requires for safe garding its data structure.
 */
#define m_NCS_RP_TMR_INIT(tmr_init_info) ncs_rp_tmr_init(tmr_init_info)

/* m_NCS_RP_TMR_CREATE is a mcaro which will allocate the timer info and
 * initalize the timer info and return the pointer as the timer id */
#define m_NCS_RP_TMR_CREATE(tmr_cb) ncs_rp_tmr_create(tmr_cb)

/* m_NCS_RP_TMR_START should be used to trigger a timer for the RP. This will
 * inturn will fit in to one of the RP timer bucket, and starts the OS timer,
 * if required. This macro will return timer id, which is of uint32_t, this
 * should be to refer the timer. RP timer will initiate only one OS timer
 * irrespective of RP starting number of timers, so when the RP receives the
 * OS timer expiry, then it should call the RP timer mcaro m_NCS_RP_TMR_EXP with
 * the appropriate timer cb. If we wants to restart the timer we can use the
 * timer id and restart the timer. This should be done only after
 * m_NCS_RP_TMR_CREATE is called. timer id should be valid.
 */
#define m_NCS_RP_TMR_START(tmr_cb, tmr_id, period, callbk, arg) ncs_rp_tmr_start(tmr_cb, tmr_id, period, callbk, arg)

/* m_NCS_RP_TMR_STOP is used to stop the timer and remove it from the RP
 * timer list, this will not frees structures whatever the m_NCS_RP_TMR_START
 * has allocated to fit in the timer entry. timer id should be valid.
 */
#define m_NCS_RP_TMR_STOP(tmr_cb, tmr_id) ncs_rp_tmr_stop(tmr_cb, tmr_id)

/* m_NCS_RP_TMR_DELETE is the used to free the memory which m_NCS_RP_TMR_START
 * has allocated for starting the timer, this should done if that particular
 * timer has already expired or after m_NCS_RP_TMR_STOP. This will return
 * failure, if that timer is already in use and if RP is trying to delete it.
 */
#define m_NCS_RP_TMR_DELETE(tmr_cb, tmr_id) ncs_rp_tmr_delete(tmr_cb, tmr_id)

/* m_NCS_RP_TMR_DESTROY is used to destroy the whole RP timer for the give
 * timer cb. This macro will run through all the RP timer strucutres and
 * free the structures and even stops the Os trigered timer.
 */
#define m_NCS_RP_TMR_DESTROY(tmr_cb) ncs_rp_tmr_destory(tmr_cb)

/* m_NCS_RP_TMR_EXP is the macro should be called when the RP receives timer
 * expiry from the OS timer with the appropriate RP timer CB. so that RP
 * timer will run through it timer list and calls all the callback functions.
 */
#define m_NCS_RP_TMR_EXP(tmr_cb) ncs_rp_tmr_exp(tmr_cb)

/* function prototypes. */
NCS_RP_TMR_CB *ncs_rp_tmr_init(NCS_RP_TMR_INIT *tmr_init_info);

NCS_RP_TMR_HDL
ncs_rp_tmr_create(NCS_RP_TMR_CB *tmr_cb);

uint32_t
ncs_rp_tmr_start(NCS_RP_TMR_CB *tmr_cb, NCS_RP_TMR_HDL tmr_id, int64_t period, RP_TMR_CALLBACK callbk,
                 void *arg);

uint32_t
ncs_rp_tmr_stop(NCS_RP_TMR_CB *tmr_cb, NCS_RP_TMR_HDL tmr_id);

uint32_t
ncs_rp_tmr_delete(NCS_RP_TMR_CB *tmr_cb, NCS_RP_TMR_HDL tmr_id);

uint32_t
ncs_rp_tmr_destory(NCS_RP_TMR_CB **tmr_cb);

uint32_t
ncs_rp_tmr_exp(NCS_RP_TMR_CB *tmr_cb);

int64_t
ncs_rp_tmr_left_over(NCS_RP_TMR_CB *tmr_cb, NCS_RP_TMR_HDL tmr_id);

/* This is the utill function which will give the time left by taking in the
 * time at which that particular timer is triggered
 */
int64_t
rp_tmr_time_left_in_sec(time_t tmr_trig_at, int64_t tmr_val);

#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCS_TMR_H_
