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

  This module contains declarations related to target system Memory Mgmt
  service.

..............................................................................
*/

/*
 * Module Inclusion Control...
 */
#ifndef BASE_NCSSYSF_MEM_H_
#define BASE_NCSSYSF_MEM_H_

#include "base/ncs_osprm.h"
#include "base/ncssysf_lck.h"
#include "base/ncsencdec_pub.h"
#include "base/ncssysfpool.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  @
  @          MEMORY MANAGER PRIMITIVES (MACROS)...
  @          Populate accordingly during portation.
  @
  @
  @  The following macros provide the front-end to the target system memory
  @  manager. These macros must be changed such that the desired operation is
  @  accomplished by using the target system memory manager primitives.
  @
  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

/**
 ** NOTES:
 ** The arguments used by the m_MMGR_* macros defined below are as follows:
 **
 **     p and p2 - a pointer to a USRBUF (USRBUF *).
 **     pp - a pointer to a pointer to a USRBUF (USRBUF **).
 **     t - a data type that will be used for type casting.
 **     n - a numeric value that identifies an octet count (unsigned int).
 **     s - a pointer to an array of octets (uint8_t *).
 **     c - a character value.
 **     v - a generic pointer (void *).
 **     b - a boolean bit value (1 or 0).
 **     m - a 32-bit number (uint32_t)
 **     i - a pool identifier, as per target system; default 0
 **     pr - some notion of priority, as per target system; default 0
 **/

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  @
  @          REQUIRED MEMORY MANAGER (PACKET BUFFER) PRIMITIVES (MACROS)...
  @          Populate accordingly during portation.
  @
  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

/** Macro to allocate a USRBUF (packet buffer) from a particular pool
 ** at a particular priority. Subsystems that are 'USRBUF pool ready' will
 ** invoke this macro rather than m_MMGR_ALLOC_BUFR.
 ** The USRBUF should be empty and properly initialized. You may elect
 ** to reserve a few octets at the beginning to optimize pre-pend operations.
 **
 ** "i" pool id of where memory is to come from
 ** "pr" priority relative to some notion of 'scarce resources'
 **
 ** The macro must return a pointer to the USRBUF.
 **
 ** NOTE: This implementation dabbles in preserving the pool_id and using it
 **       when new USRBUFs are chained off the original. Howerver, this
 **       implementation does nothing with the priority. This is an exercise
 **       left to the user (you)..
 **/
#define m_MMGR_ALLOC_POOLBUFR(i,pr) (sysf_alloc_pkt(i,pr,0, __LINE__,__FILE__))

/** Macro to allocate a USRBUF (packet buffer)...
 ** The USRBUF should be empty and properly initialized. You may elect
 ** to reserve a few octets at the beginning to optimize pre-pend operations.
 **
 ** "n" is always passed in as "sizeof(USRBUF)"
 **
 ** The macro must return a pointer to the USRBUF.
 **/
#define m_MMGR_ALLOC_BUFR(n)  (sysf_alloc_pkt(0,0,n,__LINE__,__FILE__)) /* Alloc short-term buffer */

/** Macro to free a USRBUF (packet buffer)...
 **/
#define m_MMGR_FREE_BUFR(ub)  (sysf_free_pkt(ub))       /* Free short-term buffer */

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  @
  @          MEMORY MANAGER (PACKET BUFFER) Function Prototypes...
  @
  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

void *sysf_leap_alloc(uint32_t b, uint8_t pool_id, uint8_t pri);
void sysf_leap_free(void *data, uint8_t pool_id);
void *sysf_heap_alloc(uint32_t b, uint8_t pool_id, uint8_t pri);
void sysf_heap_free(void *data, uint8_t pool_id);
void *sysf_stub_alloc(uint32_t b, uint8_t pool_id, uint8_t pri);
void sysf_stub_free(void *data, uint8_t pool_id);

/* free the user frame data info */
#define m_MMGR_FREE_BUFR_FRAMES(ptr) m_NCS_MEM_FREE(ptr->bufp, NCS_MEM_REGION_TRANSIENT, \
                                                    NCS_SERVICE_ID_SOCKET, 0); \
  m_NCS_MEM_FREE(ptr, NCS_MEM_REGION_TRANSIENT,                         \
                 NCS_SERVICE_ID_SOCKET, 0);

/** Macro to fetch the beginning of "available" payload in a given USRBUF...
 **
 ** "p" is a given USRBUF
 ** "t" is an explicit type-cast of the pointer
 **
 ** NOTE: pool_id == 0 is default pool always
 **
 **/

#define m_MMGR_DATA(p, t)     ((t)((p)->payload->Data + (p)->start))

/** Macro to fetch a pointer to payload at a specified offset in a given USRBUF...
 **
 ** "p" is a given USRBUF
 ** "o" is the number of bytes offset from the start of data within the USRBUF's payload
 ** "t" is an explicit type-cast of the pointer
 **
 **/
#define m_MMGR_DATA_AT_OFFSET(p, o, t)     ((t)((p)->payload->Data + (p)->start + o))

/** Macro to fetch the count of octets that may be prepended to the
 ** beginning of payload within a USRBUF...In some implementations
 ** this may translate to offset to the beginning of actual payload
 ** from the beginning of payload space...
 **
 ** "p" is a given USRBUF
 **
 ** This macro must return an unsigned int.
 **/
#define m_MMGR_HEADROOM(p)    (p)->start

/** Macro to fetch the count of octets that may be appended to the
 ** end of payload within a USRBUF...This may translate to offset to
 ** the end of actual payloadto the end of payload space...
 **
 ** "p" is a given USRBUF
 **
 ** This macro must return an unsigned int.
 **/
#define m_MMGR_TAILROOM(p)    (PAYLOAD_BUF_SIZE - (p)->start - (p)->count)

/** Macro to fetch the count of payload data in a given USRBUF (chain)...
 **
 ** "p" is a given USRBUF
 **
 ** This macro must return an unsigned int.
 **/
#define m_MMGR_LINK_DATA_LEN(p)  sysf_get_chain_len(p)

/** Macro to calculate the cksum of payload data in a given USRBUF (chain)...
 **
 ** "p" is a given USRBUF
 ** "len" is the length of the datum
 ** "cksumVar" is the place to store the calculated checksum
 **
 ** This macro has a void return.
 **/
#define m_MMGR_BUFR_CALC_CKSUM(p, len, cksumVar)  sysf_calc_usrbuf_cksum_1s_comp(p, len, cksumVar)

/** Macro to duplicate a USRBUF...In many implementations this may translate
 ** into merely creating an additional packet buffer descriptor and pointing
 ** to the packet buffer content.
 **
 ** "p" is a given USRBUF
 **
 ** This macro must return a pointer to the duplicate USRBUF.
 **/
#define m_MMGR_DITTO_BUFR(p)     sysf_ditto_pkt(p)

/** Macro to copy a USRBUF...This translates into creating additional
 ** packet buffer descriptor(s) and payload area(s).
 **
 ** "p" is a given USRBUF
 **
 ** This macro must return a pointer to the copy of the USRBUF.
 **/
#define m_MMGR_COPY_BUFR(p)     sysf_copy_pkt(p)

/** Macro to free a USRBUF (chain)...
 **
 ** "p" is a given USRBUF
 **
 ** The return value from this macro is irrelevant.
 **/

#define m_MMGR_FREE_BUFR_LIST(p)                \
  {                                             \
    USRBUF *pn;                                 \
                                                \
    for (; (p);)                                \
    {                                           \
      pn = (p)->link;                           \
      sysf_free_pkt(p);                         \
      p = pn;                                   \
    }                                           \
  }

/** Macro to fetch the next USRBUF in a queue of USRBUFs.
 **
 ** "p" is a given USRBUF
 **
 ** This macro is exclusively used by SSCOP for enqueuing and
 ** dequeueing of USRBUFs in certain queues.
 **/
#define m_MMGR_NEXT(p)        ((p)->next)

/** Reserves n bytes immediately following the
 ** existing payload of the USRBUF (chain) pointed to by the
 ** contents of the USRBUF** pp.  The size of the payload of
 ** the USRBUF (chain) is increased by n.  The bytes must
 ** be contiguous, and this macro evaluates to a pointer to this
 ** contiguous memory block (cast as a type t).  In order to
 ** supply a contiguous memory block, this macro may
 ** (depending on portation specifics) be required to allocate a
 ** new USRBUF, and link it to the end of the chain.  When
 ** this occurs, the macro should modify the value pointed to
 ** by pp, setting *pp to be a pointer to the newly allocated
 ** USRBUF.
 **/
#define m_MMGR_RESERVE_AT_END(pp, n, t) ((t)sysf_reserve_at_end(pp, n))

/** This is similar to m_MMGR_RESERVE_AT_END except that it tries
 ** to use any space available in the current buffer before going
 ** for a new one. In that process, it may actually reserve a
 ** few bytes lesser than requested. This is only useful when
 ** doing an "encode_n_octets" kind of thing.
 **/
#define m_MMGR_RESERVE_AT_END_AMAP(pp, pn, t) ((t)sysf_reserve_at_end_amap(pp, pn, false))

/** Removes n bytes from the end of the the existing
 ** payload of the USRBUF (chain) pointed to by the
 ** USRBUF* p.  The size of the payload of the USRBUF
 ** (chain) is decreased by n.  This macro should (depending
 ** on portation specifics) de-link and free subsequent
 ** USRBUFs which no longer contain data.  However the
 ** macro should not free USRBUF p, even if it results in an
 ** empty USRBUF.  This macro does not evaluate to any
 ** value.
 **/
#define m_MMGR_REMOVE_FROM_END(p, n) sysf_remove_from_end(p, n)

/** Reserves n bytes immediately in front of the the
 ** existing payload of the USRBUF (chain) pointed to by the
 ** contents of the USRBUF** pp.  The size of the payload of
 ** the USRBUF (chain) is increased by n.  The bytes must
 ** be contiguous, and this macro evaluates to a pointer to this
 ** contiguous memory block (cast as a type t).  In order to
 ** supply a contiguous memory block, this macro may
 ** (depending on portation specifics) be required to allocate a
 ** new USRBUF, and link it to the original packet head (thus
 ** causing a new packet head).  When this occurs, the macro
 ** should modify the value pointed to by pp, setting *pp to be
 ** a pointer to the newly allocated USRBUF.
 **/
#define m_MMGR_RESERVE_AT_START(pp, n, t) ((t)sysf_reserve_at_start(pp, n))

/** Removes n bytes from the start of the existing
 ** payload of the USRBUF (chain) pointed to by the value
 ** pointed to by the USRBUF** pp.  The size of the payload
 ** of the USRBUF (chain) is decreased by n.  This macro
 ** should (depending on portation specifics) de-link and free
 ** USRBUFs which no longer contain data.  When this
 ** occurs, the packet has a new head USRBUF, and the
 ** macro should return the pointer to the (new) first
 ** USRBUF for the packet by modifying the value pointed to
 ** by pp.  If all bytes are removed, the macro may free all
 ** USRBUFs and set *pp to be BNULL.
 **/
#define m_MMGR_REMOVE_FROM_START(pp, n) sysf_remove_from_start(pp, n)

/** Evaluates to a pointer (char*) to the last n bytes in the
 ** current payload area in the USRBUF (chain) headed by p.
 ** The pointer value returned by this macro must point to a
 ** contiguous block of memory, usually to some point within
 ** the USRBUF chain (payload).  However, if the last n bytes
 ** in the chain are not contiguous, the macro must copy the
 ** data to the area indicated by the char* s (which is
 ** guaranteed by the caller to be at least n bytes long).  In
 ** this case, the macro should evaluate to the value of s.
 **/
#define m_MMGR_DATA_AT_END(p, n, s) sysf_data_at_end(p, n, s)

/** Evaluates to a pointer (char*) to the first n bytes in the
 ** current payload area in the USRBUF (chain) headed by p.
 ** The pointer value returned by this macro must point to a
 ** contiguous block of memory, usually to some point within
 ** the USRBUF chain (payload).  However, if the first n
 ** bytes in the chain are not contiguous, the macro must copy
 ** the data to the area indicated by the char* s (which is
 ** guaranteed by the caller to be at least n bytes long).  In
 ** this case, the macro should evaluate to the value of s.
 **/
#define m_MMGR_DATA_AT_START(p, n, s) sysf_data_at_start(p, n, s)

/** Evaluates to a pointer (char*) to the n bytes at offset o in the
 ** current payload area in the USRBUF (chain) headed by p.
 ** The pointer value returned by this macro must point to a
 ** contiguous block of memory, usually to some point within
 ** the USRBUF chain (payload).  However, if the n bytes in the
 ** chain at offset o are not contiguous, the macro must copy
 ** the data to the area indicated by the char* s (which is
 ** guaranteed by the caller to be at least n bytes long).  In
 ** this case, the macro should evaluate to the value of char* s.
 ** If offset o is beyond the end of the current payload the macro
 ** evaluates to a NULL pointer. Likewise, if there is not enough
 ** payload to satisfy a request for n bytes the macro evaluates to
 ** a NULL pointer; although a partial copy of data to char* s may
 ** have taken place.
 **/

#define m_MMGR_PTR_MID_DATA(p, o, n, s)                                 \
  sysf_data_in_mid(p, (unsigned int) o, (unsigned int) n, (char*)(s), false)

    /** Evaluates to a pointer (char*) to a copy of the n bytes at
     ** offset o in the current payload area in the USRBUF (chain)
     ** headed by p.  The bytes are copied to the area indicated by
     ** the char* s (which is guaranteed by the caller to be at least
     ** uint* n bytes long).  The macro will evaluate to char* s unless the
     ** offset o is beyond the end of the current payload. In that case
     ** the macro evaluates to a NULL pointer. Likewise, if there is not
     ** enough payload to satisfy a request for n bytes the macro evaluates
     ** to a NULL pointer; although a partial copy of data to char* s may
     ** have taken place.
     **/
#define m_MMGR_COPY_MID_DATA(p, o, n, s)                                \
    sysf_data_in_mid(p,(unsigned int) o, (unsigned int) n, (char*)s, true)

/*
** Evaluates to a pointer (char *) which is 'o' bytes from the
** start of the payload of USRBUF 'p'.  Copies 'i' bytes of data
** from buffer 's' to area starting at 'o' bytes from the start
** of the payload area of USRBUF 'p'.  Macro evaluates to zero
** if unable to allocate space for the copy.
**/
#define m_MMGR_INSERT_IN_MIDDLE(p, o, i, s)                             \
  sysf_insert_in_mid(p, (unsigned int) o, (unsigned int) i, s );

/*
** Evaluates to a pointer (char *) which is the same as the
** 's' parameter.  Copies 'i' bytes of data from buffer 's' to
** area starting at 'o' bytes from the start of the payload area
** of USRBUF 'p'.  Macro evaluates to zero if unable to copy to
** the payload area.
**/
#define m_MMGR_WRITE_IN_MIDDLE(p, o, i, s)                              \
  sysf_write_in_mid(p, (unsigned int) o, (unsigned int) i, (char*)s );

/**
 ** This macro appends the data from buffer 2 to the end of
 ** the data in buffer 1.  This may be accomplished by chaining
 ** buffer 2 onto buffer 1.  This is used to extend a frame.
 ** After this macro is called, buffer 2 is no longer valid and
 ** should not be accessed.  Macro has no return value.
 **/
#define m_MMGR_APPEND_DATA(p1, p2)     sysf_append_data(p1, p2)

    /**
     ** The macro fragments the existing payload of the USRBUF (chain)
     ** pointed to by the contents of the USRBUF *ppb.  Each fragment
     ** is of size frag_size, except for the last one, which might
     ** be the same or smaller. It returns the number of fragments
     ** created and all fragments are added to the SYSF_UBQ.
     ** A zero value indicates a failure.
     **
     ** Note: If the frame passed in happens to be in a queue
     ** (its next pointer is not NULL), this macro will only
     ** fragment the first frame.
     **
     ** The user of this macro is responsible for freeing all created
     ** fragments so far in case of errors.
     **
     **/
#define m_MMGR_FRAG_BUFR(pbuf, n, ubq)   sysf_frag_bufr(pbuf, n, &ubq)

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  @
  @      OPTIONAL MEMORY MANAGER (PACKET BUFFER) PRIMITIVES (MACROS) USED
  @          BY H&J SIGNALLING SUBSYSTEM ("SSS") and RCP (that is, SSCOP)
  @          Populate accordingly during portation.
  @
  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

/** Macro to fetch the "Poll-Sequence Number" for a given USRBUF...
 **
 ** "p" is a given USRBUF
 **
 ** This macro should return an uns32.
 **/
#define m_MMGR_FETCH_NPS(p)   ((p)->specific.uni_sig_3x.nps)

/** Macro to set the "Poll-Sequence Number" for a given USRBUF...
 **
 ** "p" is a given USRBUF
 ** "m" is the NPS value (32-bit) for the USRBUF.
 **
 ** The return value from this macro is irrelevant.
 **/
#define m_MMGR_SET_NPS(p, v)  ((p)->specific.uni_sig_3x.nps = (v))

/** Macro to fetch the "Retransmission Bit" for a given USRBUF...
 **
 ** "p" is a given USRBUF
 **
 ** This macro should return 1 or 0.
 **/
#define m_MMGR_FETCH_RTS(p)   ((p)->specific.uni_sig_3x.rts)

/** Macro to set the "Retransmission Bit" for a given USRBUF...
 **
 ** "p" is a given USRBUF
 ** "b" is a boolean value 1 or 0.
 **
 ** The return value from this macro is irrelevant.
 **/
#define m_MMGR_SET_RTS(p, v)  ((p)->specific.uni_sig_3x.rts = (v))

/** The following macros are used to access a singly linked list of USRBUFs.
 ** This function has been abstracted to a set of macros so that in systems
 ** where the USRBUF does not have a NEXT field, the NQ and DQ routines can
 ** be made to allocate a small structure to hold a ptr. to the USRBUF and
 ** a NEXT pointer.
 **/

typedef struct sysf_ubq {
  unsigned int count;
  USRBUF *head;
  USRBUF *tail;
  NCS_LOCK lock;
} SYSF_UBQ;

#define m_MMGR_UBQ_CREATE(ubq)                                          \
  {                                                                     \
    (ubq).count = 0;                                                    \
    (ubq).head = BNULL;                                                 \
    (ubq).tail = BNULL;                                                 \
    m_NCS_LOCK_INIT_V2(&((ubq).lock), NCS_SERVICE_ID_COMMON, 1);        \
  }

#define m_MMGR_UBQ_RELEASE(ubq)                                         \
  m_NCS_LOCK_DESTROY_V2(&((ubq).lock), NCS_SERVICE_ID_COMMON, 1);

#define m_MMGR_UBQ_COUNT(ubq)   (ubq).count

#define m_MMGR_UBQ_NQ_TAIL(ubq, pbuf)           \
  {                                             \
    ++((ubq).count);                            \
    pbuf->next = BNULL;                         \
    if ((ubq).tail != BNULL)                    \
    {                                           \
      (ubq).tail->next = pbuf;                  \
      (ubq).tail = pbuf;                        \
    }                                           \
    else                                        \
      (ubq).head = (ubq).tail = pbuf;           \
  }

#define m_MMGR_UBQ_DQ_HEAD(ubq)  sysf_ubq_dq_head(&ubq)

/** this macro should search the queue and dequeue the USRBUF
 ** identified by pbuf.  There is no return value.
 **/
#define m_MMGR_UBQ_DQ_SPECIFIC(ubq, pbuf) sysf_ubq_dq_specific(&ubq, pbuf)

/** this macro should scan the queue for the specified USRBUF,
 ** and if found, return a pointer to it without altering the
 ** contents of the queue.
 **/
#define m_MMGR_UBQ_SCAN_SPECIFIC(ubq, pbuf)  sysf_ubq_scan_specific(&ubq, pbuf)

/** this macro should scan the queue for the specified USRBUF,
 ** and if found, return a pointer to the NEXT  USRBUF on the
 ** queue.  If pbuf is BNULL, then return the first USRBUF on
 ** the queue.
 **/
#define m_MMGR_UBQ_NEXT(ubq, pbuf)  m_MMGR_NEXT(pbuf)

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  @
  @              FUNCTION PROTOTYPES
  @
  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

void sysf_free_pkt(USRBUF *pb);

USRBUF *sysf_alloc_pkt(unsigned char pool_id,
  unsigned char priority, int num, unsigned int line, char *file);

char *sysf_reserve_at_end(USRBUF **ppb, unsigned int size);
char *sysf_reserve_at_end_amap(USRBUF **ppb, unsigned int *io_size, bool total);
void sysf_remove_from_end(USRBUF *pb, unsigned int size);
char *sysf_reserve_at_start(USRBUF **ppb, unsigned int size);
void sysf_remove_from_start(USRBUF **ppb, unsigned int size);
char *sysf_data_at_end(const USRBUF *pb, unsigned int size, char *spare);
char *sysf_data_at_start(const USRBUF *pb, unsigned int size, char *spare);
USRBUF *sysf_ditto_pkt(USRBUF *);
USRBUF *sysf_copy_pkt(USRBUF *dup_me);
char *sysf_data_in_mid(USRBUF *pb,
  unsigned int offset,
  unsigned int size, char *spare, unsigned int copy_flag);
char *sysf_write_in_mid(USRBUF *pb, unsigned int offset, unsigned int size, char *cdata);
char *sysf_insert_in_mid(USRBUF *pb,
  unsigned int offset, unsigned int size, char *ins_data);
unsigned int sysf_frag_bufr(USRBUF *ppb, unsigned int frag_sz, SYSF_UBQ *q);
void sysf_append_data(USRBUF *p1, USRBUF *p2);
USRBUF *sysf_ubq_dq_head(SYSF_UBQ *q);
void sysf_ubq_dq_specific(SYSF_UBQ *q, USRBUF *ub);
USRBUF *sysf_ubq_scan_specific(SYSF_UBQ *q, USRBUF *ub);

uint32_t sysf_copy_from_usrbuf(USRBUF *packet, uint8_t *buffer, uint32_t buff_len);
USRBUF *sysf_copy_to_usrbuf(uint8_t *packet, unsigned int length);

/** Computational routines **/
uint32_t sysf_get_chain_len(const USRBUF *);
void sysf_calc_usrbuf_cksum_1s_comp(USRBUF *const, unsigned int, uint16_t *const);

void sysf_usrbuf_hexdump(USRBUF *buf, char *fname);

uint32_t sysf_str_hexdump(uint8_t *data, uint32_t size, char *fname);

uint32_t sysf_pick_output(char *str, char *fname);

#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCSSYSF_MEM_H_
