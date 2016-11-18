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
H&J Queue Facility Models
------------------------
A generic set of queueing primitives, common to all H&J Sub-systems
is provided for manipulating queues.  It is in the standard
top-level (base-code) area. Macros are provided to use H&J internal
queue facilities.

******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#ifndef BASE_NCS_QUEUE_H_
#define BASE_NCS_QUEUE_H_

#include "base/ncssysfpool.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

  H&J Internal Queue Facility Definitions

  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

/** H&J Queue Element
 ** Note: this queue element exists to force the requirement that the first
 ** field of the object to be queued must be "next" (void *).
 **/
typedef struct ncs_qelem {
  struct ncs_qelem *next;
} NCS_QELEM;

#define NCS_QELEM_NULL ((NCS_QELEM *)0)

/** H&J Queue Control Block
 ** Note: The first field in queued object must be "next" and be a (void *),
 **/
typedef struct ncs_queue {
  unsigned int count;     /* Items in queue */
  NCS_QELEM *head;        /* First (oldest) item on queue */
  NCS_QELEM *tail;        /* Last  (newest) item on queue */
  NCS_LOCK queue_lock;
} NCS_QUEUE;

/** H&J Queue Iterator, for walking the list
 **/

typedef struct ncs_q_itr {
  void *state;
} NCS_Q_ITR;

#define NCS_QUEUE_NULL ((NCS_QUEUE *)0)

/* This function prototype used to find an element in an NCS_QUEUE */

typedef bool (*NCSQ_MATCH) (void *key, void *qelem);

/** H&J Queue Primitives
 **/
void ncs_create_queue(NCS_QUEUE *queue);
void ncs_destroy_queue(NCS_QUEUE *queue);
void *ncs_peek_queue(NCS_QUEUE *queue);
unsigned int ncs_enqueue(NCS_QUEUE *queue, void *item);
unsigned int ncs_enqueue_head(NCS_QUEUE *queue, void *item);

void *ncs_dequeue(NCS_QUEUE *queue);
void *ncs_remove_item(NCS_QUEUE *queue, void *key, NCSQ_MATCH match);
void *ncs_find_item(NCS_QUEUE *queue, void *key, NCSQ_MATCH match);
void *ncs_walk_items(NCS_QUEUE *queue, NCS_Q_ITR *itr);
void *ncs_queue_get_next(NCS_QUEUE *queue, NCS_Q_ITR *itr);

#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCS_QUEUE_H_
