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

  H&J InterProcess Communication Facility
  -------------------------------------
  Interprocess and Intraprocess Event Posting.  We use this for posting events
  (received pdu's, API requests) at sub-system  boundaries.  It is also used
  for timer expirations, and posting of events within subsystems.

  ******************************************************************************
  */

/** Module Inclusion Control...
 **/
#ifndef BASE_SYSF_IPC_H_
#define BASE_SYSF_IPC_H_

#include "base/ncssysf_ipc.h"
#include "base/ncssysfpool.h"

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

  Internal Messaging Facility Definitions

  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

typedef struct ncs_ipc_queue {
  NCS_IPC_MSG *head;
  NCS_IPC_MSG *tail;
} NCS_IPC_QUEUE;

typedef struct tag_ncs_ipc {
  NCS_LOCK queue_lock;    /* to protect access to queue */

  NCS_IPC_QUEUE queue[NCS_IPC_PRIO_LEVELS];       /* element 0 for queueing HIGH priority IPC messages */
  /* element 1 for queueing NORMAL priority IPC messages */
  /* element 2 for queueing LOW priority IPC messages */

  uint32_t no_of_msgs[NCS_IPC_PRIO_LEVELS];       /* (priority level message count, used to compare
                                                     with the corresponding threshold value) */

  uint32_t max_no_of_msgs[NCS_IPC_PRIO_LEVELS];   /* (threshold value configured through
                                                     m_NCS_IPC_CONFIG_MAX_MSGS otherwise initialized to zero) */
  uint32_t *usr_counters[NCS_IPC_PRIO_LEVELS];    /* user given 32bit counters are accessed through these pointers */

  unsigned int active_queue;      /* the next queue to check for an IPC message */

  /* Changes to migrate to SAF model:PM:7/Jan/03. */
  NCS_SEL_OBJ sel_obj;
  /* msg_count: Keeps a count of messages queued. When this count is
     non-zero, "sel_obj" will have an "indication" raised against
     it. The "indication" on "sel_obj" will be removed when
     the count goes down to zero.

     This way there need not be an indication raised for every
     message. An indication is raised only per "burst of
     messages"
  */
  uint32_t msg_count;

  /* If "sel_obj" is put to use, the "sem_handle" member will be removed.
     For now it stays */
  void *sem_handle;       /* for blocking/waking IPC msg receiver */
  bool releasing; /* flag from ncs_ipc_release to ncs_ipc_recv */
  uint32_t ref_count;     /* reference count - number of instances attached
                           * to this IPC */
  char *name;             /* mbx task name */
} NCS_IPC;

#endif  // BASE_SYSF_IPC_H_
