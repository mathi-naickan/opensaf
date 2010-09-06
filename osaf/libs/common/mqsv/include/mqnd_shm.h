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
  
  Shared memory function definitions

******************************************************************************
*/

/* Storing queue stats in shared memory segment */
#ifndef MQSV_SHM_QUEUESTATS
#define MQSV_SHM_QUEUESTATS

#include <sys/mman.h>

/*defines*/
#define SHM_QUEUE_INFO_VALID 1
#define SHM_QUEUE_INFO_INVALID 0
#define SHM_NAME "NCS_MQND_QUEUE_CKPT_INFO"

EXTERN_C uns32 mqnd_shm_create(MQND_CB *cb);
EXTERN_C uns32 mqnd_shm_destroy(MQND_CB *cb);
EXTERN_C uns32 mqnd_find_shm_ckpt_empty_section(MQND_CB *cb, uns32 *index);
EXTERN_C uns32 mqnd_send_msg_update_stats_shm(MQND_CB *cb, MQND_QUEUE_NODE *qnode, SaSizeT size, SaUint8T priority);
EXTERN_C uns32 mqnd_shm_queue_ckpt_section_invalidate(MQND_CB *cb, MQND_QUEUE_NODE *qnode);
EXTERN_C void mqnd_reset_queue_stats(MQND_CB *cb, uns32 index);

#endif
