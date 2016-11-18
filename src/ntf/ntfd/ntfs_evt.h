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
 * Author(s): Ericsson AB
 *
 */

#ifndef NTF_NTFD_NTFS_EVT_H_
#define NTF_NTFD_NTFS_EVT_H_

#include "rde/agent/rda_papi.h"

typedef enum ntfsv_ntfs_evt_type {
  NTFSV_NTFS_NTFSV_MSG = 1,
  NTFSV_NTFS_EVT_NTFA_UP = 2,
  NTFSV_NTFS_EVT_NTFA_DOWN = 3,
  NTFSV_EVT_QUIESCED_ACK = 4,
  NTFSV_EVT_RDA = 5,
  NTFSV_NTFS_EVT_MAX
} NTFSV_NTFS_EVT_TYPE;

typedef struct ntfsv_ntfs_mds_info {
  uint32_t node_id;
  MDS_DEST mds_dest_id;
} ntfsv_ntfs_mds_info_t;

typedef struct {
  PCS_RDA_ROLE io_role;
} ntfsv_rda_info_t;

typedef struct ntfsv_ntfs_evt {
  struct ntfsv_ntfs_evt *next;
  uint32_t cb_hdl;
  MDS_SYNC_SND_CTXT mds_ctxt;     /* Relevant when this event has to be responded to
                                   * in a synchronous fashion.
                                   */
  MDS_DEST fr_dest;
  NODE_ID fr_node_id;
  MDS_SEND_PRIORITY_TYPE rcvd_prio;       /* Priority of the recvd evt */
  NTFSV_NTFS_EVT_TYPE evt_type;
  union {
    ntfsv_msg_t msg;
    ntfsv_ntfs_mds_info_t mds_info;
    ntfsv_rda_info_t rda_info;
  } info;
} ntfsv_ntfs_evt_t;

/* These are the function prototypes for event handling */
typedef uint32_t (*NTFSV_NTFS_NTFA_API_MSG_HANDLER) (ntfs_cb_t *, ntfsv_ntfs_evt_t *evt);
typedef uint32_t (*NTFSV_NTFS_EVT_HANDLER) (ntfsv_ntfs_evt_t *evt);

#endif  // NTF_NTFD_NTFS_EVT_H_
