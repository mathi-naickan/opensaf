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

  DESCRIPTION: Abstractions and APIs for NCS_HDL service, an access-safe,
  use-safe means to fetch and use fleeting and/or volitile objects.

  Abstractions and APIs for the 'Local Persistence Guard' service which is
  also an access-safe, use-safe mens to fetch and use a fleeting object
  that happens to live off another object. It provides that same type of
  protection as the Handle Manager, but does not require the de-coupling
  of handle and object.


  ******************************************************************************
  */

/*
 * Module Inclusion Control...
 */
#ifndef BASE_NCS_HDL_H_
#define BASE_NCS_HDL_H_

#include "base/ncs_hdl_pub.h"

/* NCS_LOCK now defined from below include file */
#include "base/ncssysf_lck.h"
#include "base/ncssysf_mem.h"

/***************************************************************************

  H J _ H D L   P r i v a t e   D a t a   S t r u c t u r e s

  G O A L :

  - Guarentees that an object will not be destroyed by another thread while
    a take()r has the object but has not give()n it back.

  - IF the object take()n has critical regions that must be guarded (linked
    lists and the like), it is up to the take()r to employ its own locking
    scheme.

  R U L E S :

  - Hdl Mgr will allow any number of take()r threads to have the data at
    the same time. This increments a refcount.

  - A take()r will fail if the handle referenced has been destroy()ed or is
    in the process of being destroy()ed.

  - A successful take()r must give() back when done. This decriments
    a refcount.

  - A destroy()er thread shall be blocked if the refcount of the handle is
    greater than one. When the refcount is one, the destroy()er thread
    is unblocked, the object is removed and the handle is destroyed.

     WARNING: Failure to give() for every take() will cause an infinite
              blocking of the remove()r's thread!!!

  D E A D L O C K   I S S U E :

  - The general algorythm used to avoid deadlock in a subsystem is as
    follows:

    1) take()   the object using the handle
    2) lock()   access to the object according to subsystem rules.
    3) unlock() the object
    4) give()   the object (decriment refcnt) using the handle

  - This should be no problem for any subsystems that uses the Hdl Mgr from
    the start, as the Hdl Mgr serves as the primary means to access objects.

  - Legacy subsystems, that already have locks and access mechanisms in
    place are more likely to run into deadlock, as the Hdl Mgr is used for
    a subset of its object access issues while the existing object access
    scheme is used by the bulk of the code. In such cases, a deadlock can
    easily arrise when one thread utilizes the sequence expressed above while
    another thread uses the sequence below...

    A) lock()    acess to the object according to subsystem rules
    B) destroy() the handle using the lock

    If thread dualing causes the sequence to go 1 - A - B - 2...

    Then there is deadlock, since:
    - B will freeze waiting for the the give() which was take()n at 1 and
    - 2 will freeze waiting to get the lock secured at A.

  C O M M O N   H A N D L E S  @ P R I M A R Y   A N D   B A C K U P

  Handle Pools

    The Handle Manager comes pre-configured with 9 pools. See ncs_hdl.c for
    table and sizes (other pool numbers and breakdown sizes can be configured).

    The zeroeth pool is intended for local services such as MDS and has a
    relatively low number of unique handle names at about 2.1 million.

    The other 8 pools each have about 33.5 million handle names each.

  Sharing Handle Names between Primary and Backup

    A Handle manager handle can map to the same object in different memory
    spaces that are set up for redundancy as follows:

    1 - The Primary and Backup sides agree to use the same Handle Manager
        pool Ids in both places, scoped by their common VCARD id value.

    2 - Pools with the same ID must be the same shape (have the same name
        space) across these two different memory spaces.

    3 - The primary side ALWAYS invokes ncshm_create_hdl() to get a handle.

    4 - A backup, typically at checkpoint data arrival time shall ALWAYS
        invoke ncshm_declare_hdl() to DECLARE to the handle manager what
        handle value it MUST have for this particular object.

    NOTE: A Handle Manager client must have the discipline to keep the
    correlation between Primary-CREATE-handle-to-object mapping 1-to-1 with
    the Backup-DECLARE-handle-to-object.

    NOTE: By following these rules, the Handle Manager SHOULD NOT run
    into a name clash (declared name already in-use) at ncshm_declare_hdl()
    time.


***************************************************************************/

/***************************************************************************
 * General handle definition, handed to clients; They don't know or care
 ***************************************************************************/

typedef struct hm_hdl {
  uint32_t seq_id:4;              /* Sequence ID that must match  'cell' value */
  uint32_t idx1:8;                /* Navigational marker for first array */
  uint32_t idx2:8;                /* Navigational marker for second array */
  uint32_t idx3:12;               /* Navigational marker for third array */

} HM_HDL;

/***************************************************************************
 * Internal CELL stores private state info and client data mapped to handle
 ***************************************************************************/

typedef struct hm_cell {
  NCSCONTEXT data;        /* This is the stored data thing */

  uint32_t seq_id:4;              /* sequence ID must match for valid key find */
  uint32_t svc_id:12;     /* Service ID of owning subsystem */
  uint32_t busy:1;                /* sub-type of datat stored in context */
  uint32_t use_ct:11;     /* Use Count; Multiple readers, once created */

} HM_CELL;

/***************************************************************************
 * When not a CELL, a CELL is on the free-cell list and looks like this
 ***************************************************************************/

typedef struct hm_free {
  struct hm_free *next;   /* linked list of free/available cells */
  HM_HDL hdl;             /* The place where this memory lives */

} HM_FREE;

/***************************************************************************
 * CELLs are allocated in hunks
 ***************************************************************************/

#define HM_CELL_CNT  4096

typedef struct hm_cells {
  HM_CELL cell[HM_CELL_CNT];      /* twelve bits of counter */

} HM_CELLS;

/***************************************************************************
 * Handle Manager has BANKS of CELLs that emerge dyanamically
 ***************************************************************************/

#define HM_BANK_CNT  256        /* FF  */

typedef struct hm_unit {
  uint8_t curr;           /* next idx to fill in */
  HM_CELLS *cells[HM_BANK_CNT];   /* when the cell-bank itself */

} HM_UNIT;

/***************************************************************************

  H a n d l e   M a n a g e r   P o o l s

  - There are 256 name-space units of cells available in the cell pool
  - Each name-space unit can distinguish 1,048,576 distinct handles
  - A handle pool can consist of one or more name-space units
  - There can be any number of handle pools up to the 256 name space units
  - LEAP ships with 9 distinct pools called poolID 0 to poolID 8.
  - poolID 0 will be used, when needed, by LEAP basic services like MDS.
  - The other 8 pools are used by various applications

***************************************************************************/

#define HM_POOL_CNT  9

typedef struct hm_pool {
  int32_t min;            /* min name-sapce unit inclusive */
  int32_t max;            /* max name-space unit inclusive */

} HM_POOL;

/***************************************************************************
 * Handle Manager Pool Manager block.. manage a pool of handles
 ***************************************************************************/

typedef struct hm_pmgr {
  HM_FREE *free_pool;     /* the free ones go here */
  uint32_t in_q;          /* current handles in this queue */
  uint32_t in_use;                /* current handles in world from this pool */
  uint32_t curr;          /* current unit-id we are working on */
  uint32_t max;           /* max unit-id that this pool owns */

} HM_PMGR;

/***************************************************************************
 * Handle Manager Core data structure manages the works...
 ***************************************************************************/

#define HM_UNIT_CNT  256        /* FF  */

typedef struct hm_core {
  NCS_LOCK lock[HM_POOL_CNT];     /* Lock for each pool */
  HM_UNIT *unit[HM_UNIT_CNT];     /* Name space units */
  HM_PMGR pool[HM_POOL_CNT];      /* pools of name space units */

  uint32_t woulda_crashed;        /* # times destroy thread blocked */

} HM_CORE;

/***************************************************************************

  H a n d l e   M a n a g e r    M a c r o    A s s i s t s

***************************************************************************/

#define HM_STATS   1            /* enable/disable Handle Manager Stats counts */

/***************************************************************************
 * Handle Manager Statistic counters
 ***************************************************************************/

#if HM_STATS == 1

#define m_HM_STAT_CRASH(n)      n++
#define m_HM_STAT_ADD_TO_Q(n)   n++
#define m_HM_STAT_RMV_FR_Q(n)   n--
#define m_HM_STAT_ADD_IN_USE(n) n++
#define m_HM_STAT_RMV_IN_USE(r,n) if(r) n--
#else

#define m_HM_STAT_CRASH(n)
#define m_HM_STAT_ADD_TO_Q(n)
#define m_HM_STAT_RMV_FR_Q(n)
#define m_HM_STAT_ADD_IN_USE(n)
#define m_HM_STAT_RMV_IN_USE(r,n)
#endif

/***************************************************************************
 *
 * P r i v a t e  (i n t e r n a l)  H a n d l e   M g r   F u n c t i o n s
 *
 ***************************************************************************/

uint32_t hm_pool_id(uint8_t unit);

uint32_t hm_init_pools(HM_PMGR *pmgr, HM_POOL *pool);

HM_FREE *hm_alloc_cell(uint8_t id);

HM_CELL *hm_find_cell(HM_HDL *hdl);

HM_FREE *hm_target_cell(HM_HDL *hdl);

void hm_free_cell(HM_CELL *cell, HM_HDL *hdl, bool recycle);

uint32_t hm_make_free_cells(HM_PMGR *pmgr);

void hm_block_me(HM_CELL *cell, uint8_t pool_id);

void hm_unblock_him(HM_CELL *cell);

/***************************************************************************

  H J L P G   P r i v a t e   D a t a   S t r u c t u r e s

  L P G : Local Persistence Guard

  G O A L :

  - Provide the same object persistence guarentees as the Handle Manger, but
    at a cheaper cost, in those cases where an 'anchor object' is known to
    exist, and the thing-that-must-be-persistent is embedded or hanging off
    of this known anchor object.

  - Guarentees that an object will not be destroyed by another thread while
    a take()r has the object but has not give()n it back.

  - IF the object take()n has critical regions that must be guarded (linked
    lists and the like), it is up to the take()r to employ its own locking
    scheme.

  R U L E S :

  - LPG will allow any number of take()r threads to have the data at
    the same time. This increments a refcount.

  - A take()r will fail if the referenced thing has been destroy()ed or is
    in the process of being destroy()ed.

  - A successful take()r must give() back when done. This decriments
    a refcount.

  - A destroy()er thread shall be blocked if the refcount of the handle is
    greater than one. When the refcount is one, the destroy()er thread
    is unblocked, the object not removed, but set to 'closed' state, thus
    assuring that future 'take()rs will not be allowed to enter.

     WARNING: Failure to give() for every take() will cause an infinite
              blocking of the remove()r's thread!!!

  D E A D L O C K   I S S U E :

  - See notes for the Hdl Manager, as they apply here as well.

  C O M M O N   H A N D L E S  @ P R I M A R Y   A N D   B A C K U P

  - LPG 'locks' are only ever local. This category of functionality does
    not exist.



***************************************************************************/

#endif  // BASE_NCS_HDL_H_
