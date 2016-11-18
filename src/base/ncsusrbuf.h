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

    @     @  @@@@@  @@@@@@  @@@@@@  @     @ @@@@@@@         @     @
    @     @ @     @ @     @ @     @ @     @ @               @     @
    @     @ @       @     @ @     @ @     @ @               @     @
    @     @  @@@@@  @@@@@@  @@@@@@  @     @ @@@@@           @@@@@@@
    @     @       @ @   @   @     @ @     @ @         @@@   @     @
    @     @ @     @ @    @  @     @ @     @ @         @@@   @     @
     @@@@@   @@@@@  @     @ @@@@@@   @@@@@  @         @@@   @     @

..............................................................................

  DESCRIPTION:

  This module contains declarations and definitions related to the
  USRBUF (USerBUFfer) memory element model used in H&J software subsystems.

..............................................................................
*/

/*
 * Module Inclusion Control...
 */
#ifndef BASE_NCSUSRBUF_H_
#define BASE_NCSUSRBUF_H_

#include "base/ncsgl_defs.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define PAYLOAD_BUF_SIZE 8000   /* default size of packet_data bufrs */

/*****************************************************************************

 OSE Link Handler stuff for USRDATA_EXTEND

 In fact, this is not OSE specific: Its a means to pass USRBUF
 fragmentation and re-assembly info in the passed USRDATA.

*****************************************************************************/

#if (NCSMDS_OSE_UD == 1)        /* Context of OSE's Signal memory pool */

typedef struct lh_ose { /* OSE Extended Userdata info             */
  SIGSELECT sig_no;       /* Signal number              (OSE based) */
  uint32_t key;   /* distinguishing attribute               */
  uint8_t pool_id;        /* id must align across environments      */
  uint16_t ttl;   /* total bytes in this USRDATA            */
  uint16_t cur;
  uint32_t used_octets;
  uint32_t d_start;

} LH_OSE;               /* Link Handler for OSE */
#endif

/****************************************************************************
 *  typedef USRDATA_EXTEND
 *
 * Specialty fields needed in specific contexts. Set up just like
 * USEBUF_EXTEND. Read below.
 */

#if (NCSMDS_OSE_UD == 1)

typedef struct usrdata_extent {

#if (NCSMDS_OSE_UD == 1)

  LH_OSE ose;     /* OSE specific data traveling with the packet */
#endif   /* NCSMDS_OSE_UD == 1) */

} USRDATA_EXTENT;       /* userdata extensions within USRDATA */
#else

typedef void *USRDATA_EXTENT;
#endif   /* (NCSMDS_OSE_UD == 1) */

/****************************************************************************
 *  typedef USRDATA
 *
 * In NetPlane reference implementation, the USRDATA houses the 'payload'
 * data. It also maintains a ref-count of how many USRBUFs are pointing to
 * it, which assists in the zero-copy paradigm and informs the memory free
 * routine if we really want to give the memory back to the owning pool.
 */

typedef struct usrdata {
  USRDATA_EXTENT ue;      /* UserData Extensions                  */
  uint32_t RefCnt;        /* # of USRBUFs pointing to this block. */
  char Data[PAYLOAD_BUF_SIZE];    /* payload area ie. The Data  . */

} USRDATA;

/****************************************************************************
 *  typedef USRBUF_EXTEND
 *
 * Specialty fields needed in specific contexts. Read below.
 */
typedef void *USRBUF_EXTENT;

/****************************************************************************
 *  typedef USRBUF
 *
 * In NetPlane reference implementation, the USRBUF organizes payload chains
 * on behalf of NetPlane code. NetPlane code only ever experiences:
 * - USRBUF pointers (never any fields that a USRBUF might be made up of)
 * - A set of USRBUF macros as per sysf_mem.h, the underside of which is
 *   the NetPlane Reference implementation., as per sysf_mem.c.
 *
 * Customers may redefine the USRBUF structure any way they wish, to better
 * 'fit' to any particular OS or target system, as long as the semantics of
 * the USRBUF macros are honored.
 */

typedef struct usrbuf {
  /** Pointer to the next usrbuf structure (used in queueing).
   **/
  struct usrbuf *next;

  /** Pointer to a 'linked' usrbuf structure - used whenever
   ** the user frame spans multiple usrbufs. All linked usrbuf
   ** structures comprise a single upper layer frame.
   **/
  struct usrbuf *link;

  /** The total number of octets used (for this usrbuf) in the data
   ** area pointed to by 'payload'.
   **/
  unsigned int count;

  /** An offset in the 'payload' buffer where the upper layer's data
   ** begins. This can be represented as *(payload+start).
   **/
  unsigned int start;

  /** pool memory operations for pool_id
   **/
  struct ncsub_pool *pool_ops;

  /**
   ** SPECIFICS:
   ** ---------
   **
   ** nps:
   ** A pre-assigned storage space for saving the POLL SEQUENCE #
   ** associated with an SD PDU. This sequence number is used by
   ** the retransmission logic in SSCOP (UNI Signalling).
   **
   ** rts:
   ** The rts flag is the current retransmission state associated
   ** with each SD or SDP PDU.  Upon initial transmission, it is set
   ** to 0.  Upon retransmission in response to a USTAT PDU, it is
   ** set to 1.  If a SD or SDP PDU is requested for retransmission
   ** based on a USTAT PDU request and its rts is 1, it shall not
   ** be retransmitted. This flag is used by SSCOP (UNI Signalling).
   **
   **/

  union {
    uint32_t opaque;
    struct sig_3x {
      uint32_t nps;
      uint8_t rts;
    } uni_sig_3x;
  } specific;

  /** This member is intended for use by the target system if it requires
   ** implementation specific data to be associated with this structure.
   ** The base implementation code does not maintain or reference this
   ** structure member. It must be maintained by target system code.
   **
   ** WARNING: If this field is used to reference user allocated data,
   **          the memory associated with this pointer should be freed BEFORE
   **          calling m_MMGR_FREE_BUFR to free the USRBUF structure.
   **/
  USRBUF_EXTENT usrbuf_extent;

  USRDATA *payload;       /* pointer to TOP of USRDATA structure. */

} USRBUF;

#define BNULL    (USRBUF *)0    /* a null usrbuf pointer */

/****************************************************************************

  U S R B U F    L A Y E R   M G M T   S E R V I C E   D E F I N I T I O N

  USRBUFs may need to come from specific memory areas to better accomodate
  particular transport services.

  This USRBUF Layer Management interface allows a target system to 'bind' a
  particular pool_id to a set of malloc/free routines. This binding would
  typically happen at system initialization time.

  Note that the default implementation has a set of preconfigured memory
  pools built right in and will not allow one to register on top of those
  pre-assigned pools.

  At runtime, clients that need to use specific memory from a specific pool
  will request memory by pool_id. The USRBUF facilities will use this value
  to determine which malloc function to call. A USRBUF will track where
  the memory came from so it is returned to the proper pool.

*****************************************************************************/

#define UB_MAX_POOLS   5        /* Max Pools for this reference implementation */

#define NCSUB_LEAP_POOL 0       /* Default mem from m_NCS_MEM_ALLOC in OS Svcs  */
#define NCSUB_HEAP_POOL 1       /* Mapped to m_NCS_OS_MEMALLOC in OS Prims      */
#define NCSUB_UDEF_POOL 2       /* User Defined m_OS_UBMEM_ALLOC in OS Defs     */
#define NCSUB_MDS_POOL  3       /* Pool for MDS messages                        */
#define NCSUB_DMY2_POOL 4       /* paradigm extended, but this is a dummy       */

/****************************************************************************

   Memory Supply Priority Scheme

  Conceptually, pools may have limited resources and that should be used
  wisely. Accordingly, requestors should rank their need against the
  'common health and welfare' of the entire system by fairly characterizing
  the urgency of their request using the priority scheme below.

  NCSMEM_HI_PRI    If mem available, provide it
  NCSMEM_MED_PRI   If conserving mem and low (say 15%), don't provide it
  NCSMEM_LOW_PRI   if conserving mem and low (say 30%), don't provide it

  NOTE: To date (6/2001)....
  - Most NetPlane subsystems shall submit HI_PRI requests by default, as
  they are not currently sensitive to these differences.
  - No NetPlane reference implementations make decisions based on priority.
  It is provided so that customers may implement such distinctions, if
  needed.
  - The notion of 15% and 30% as percent ranges is for example purposes
  and may not reflect what may be appropriate for any particular system.

***************************************************************************/

#define NCSMEM_HI_PRI     3
#define NCSMEM_MED_PRI    2
#define NCSMEM_LOW_PRI    1

#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCSUSRBUF_H_
