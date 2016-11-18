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

  DESCRIPTION:

  This module contains declarations and definitions related to the
  USRBUF (USerBUFfer) memory element model used in H&J software subsystems.

..............................................................................
*/

/*
 * Module Inclusion Control...
 */
#ifndef BASE_USRBUF_H_
#define BASE_USRBUF_H_

#include "base/ncs_osprm.h"
#include "base/ncsusrbuf.h"
#include "base/ncssysf_lck.h"

/***************************************************************************
 * NCSUB_POOL holds all info associated with a particular USRBUF pool
 ***************************************************************************/

typedef struct ncsub_pool {
  bool busy;
  uint8_t pool_id;
  NCS_POOL_MALLOC mem_alloc;
  NCS_POOL_MFREE mem_free;
  uint32_t hdr_reserve;
  uint32_t trlr_reserve;
} NCSUB_POOL;

/***************************************************************************
 * UB_POOL_MGR is the set of pool entry functions registered w/USRBUF pool.
 ***************************************************************************/

typedef struct ub_pool_mgr {
  NCSUB_POOL pools[UB_MAX_POOLS];
  NCS_LOCK lock;

} UB_POOL_MGR;

/****************************************************************************

  P O O L  M A N A G E R   L A Y E R - M G M T   A P I s

  The standard union of structs of all possible Layer Management
  functions that can be performed on the USRBUF Pool manager.

*****************************************************************************/

/***************************************************************************
 * NCSMMGR_UB_INIT Put USRBUF pool management svcs in start state
 ***************************************************************************/

typedef struct ncsmmgr_ub_init {
  void *i_meaningless;

} NCSMMGR_UB_INIT;

/***************************************************************************
 * NCSMMGR_UB_DELETE  Disengage all USRBUF pools and recover resources
 ***************************************************************************/

typedef struct ncsmmgr_ub_delete {
  void *i_meaningless;

} NCSMMGR_UB_DELETE;

/***************************************************************************
 * NCSMMGR_UB_REGISTER register pool properties in USRBUF service area.
 ***************************************************************************/

typedef struct ncsmmgr_ub_register {
  uint8_t i_pool_id;
  NCS_POOL_MALLOC i_mem_alloc;
  NCS_POOL_MFREE i_mem_free;
} NCSMMGR_UB_REGISTER;

/***************************************************************************
 * NCSMMGR_UB_DEREGISTER disengage a particular pool in USRBUF svc area
 ***************************************************************************/

typedef struct ncsmmgr_ub_deregister {
  uint8_t i_pool_id;

} NCSMMGR_UB_DEREGISTER;

/***************************************************************************
 * The NCSMMGR_UB_OP operations set
 ***************************************************************************/

typedef enum ncsmmgr_ub_op {
  NCSMMGR_LM_OP_INIT,
  NCSMMGR_LM_OP_DELETE,
  NCSMMGR_LM_OP_REGISTER,
  NCSMMGR_LM_OP_DEREGISTER
} NCSMMGR_UB_OP;

/***************************************************************************
 * NCSMMGR_UB_LM_ARG the set of structures that capture all operations
 ***************************************************************************/

typedef struct ncsmmgr_ub_lm_arg {
  NCSMMGR_UB_OP i_op;
  union {
    NCSMMGR_UB_INIT init;
    NCSMMGR_UB_REGISTER reg;
    NCSMMGR_UB_DEREGISTER dereg;
    NCSMMGR_UB_DELETE del;

  } info;

} NCSMMGR_UB_LM_ARG;

/***************************************************************************
 m_NCSMMGR_UB_LM  is the macro abstraction advertised as the NetPlane API
                 that leads to some function that honors the semantics of
                 the USRBUF Layer Management services.

                 The NetPlane reference implementation that honors these
                 semantics is a function called ncsmmgr_ub_lm().

  m_NCSMMGR_UB_GETPOOL is the macro abstraction advertised as the NetPlane
                 API that knows how to fetch the pointer to pool info
                 based on passed in pool_id.

                 The NetPlane reference implemetnation that honors these
                 semantics is a function called ncsmmgr_ub_getpool()

***************************************************************************/

uint32_t ncsmmgr_ub_lm(NCSMMGR_UB_LM_ARG *arg);
NCSUB_POOL *ncsmmgr_ub_getpool(uint8_t pool_id);

#define m_NCSMMGR_UB_LM(a)           ncsmmgr_ub_lm(a)
#define m_NCSMMGR_UB_GETPOOL(id)     ncsmmgr_ub_getpool(id)

/************************************************************************

  Pool Manager Locks

  NOTE: By default, NetPlane demos and the like work with the statically
  configured 'gl_ub_pool_mgr', which is fixed at compile time. As such,
  NO LOCKS are needed (pool entries do not come and go).

  If your system can dynamically configure (add/remove them) then you
  may need locks. Even in this case, the most likely scenerio is that
  pools will be introduced at initialization time and removed at
  tear-down time and left alone at runtime. In this case, locks are
  some insurance for a highly unlikely event. You may consider 'risking'
  it in this case.

*************************************************************************/

#ifndef NCSPMGR_USE_LOCK_TYPE
#define NCSPMGR_USE_LOCK_TYPE PMGR_NO_LOCKS
#endif

#if (NCSPMGR_USE_LOCK_TYPE == PMGR_NO_LOCKS)    /* NO Locks */

#define m_PMGR_LK_CREATE(lk)
#define m_PMGR_LK_INIT
#define m_PMGR_LK(lk)
#define m_PMGR_UNLK(lk)
#define m_PMGR_LK_DLT(lk)
#elif (NCSPMGR_USE_LOCK_TYPE == PMGR_TASK_LOCKS)        /* Task Locks */

#define m_PMGR_LK_CREATE(lk)
#define m_PMGR_LK_INIT            m_INIT_CRITICAL
#define m_PMGR_LK(lk)             m_START_CRITICAL
#define m_PMGR_UNLK(lk)           m_END_CRITICAL
#define m_PMGR_LK_DLT(lk)
#elif (NCSPMGR_USE_LOCK_TYPE == PMGR_OBJ_LOCKS) /* Object Locks */

#define m_PMGR_LK_CREATE(lk)      m_NCS_LOCK_INIT_V2(lk,0,0)
#define m_PMGR_LK_INIT
#define m_PMGR_LK(lk)             m_NCS_LOCK_V2(lk,   NCS_LOCK_WRITE,0, 0)
#define m_PMGR_UNLK(lk)           m_NCS_UNLOCK_V2(lk, NCS_LOCK_WRITE, 0, 0)
#define m_PMGR_LK_DLT(lk)         m_NCS_LOCK_DESTROY_V2(lk, 0, 0)
#endif

#endif  // BASE_USRBUF_H_
