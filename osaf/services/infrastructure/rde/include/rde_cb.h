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

#ifndef RDE_CB_H
#define RDE_CB_H

#include <configmake.h>
#include <mds_papi.h>

#include "rde_rda_common.h"
#include "rde_amf.h"
#include "rde_rda.h"
#include "rda_papi.h"
#include "osaf_utility.h"

/* The value to put in the PATH environment variable when calling the
 SC active script */
#define SC_ACTIVE_PATH_ENV "/usr/local/sbin:/usr/local/bin:/usr/sbin:" \
        "/usr/bin:/sbin:/bin"

/*
 **  RDE_CONTROL_BLOCK
 **
 **  Structure containing all state information for RDE
 **
 */

struct RDE_CONTROL_BLOCK {
  SYSF_MBX mbx;
  const char *prog_name;
  NCSCONTEXT task_handle;
  bool task_terminate;
  bool fabric_interface;
  uint32_t select_timeout;
  RDE_RDA_CB rde_rda_cb;
  RDE_AMF_CB rde_amf_cb;
};

enum RDE_MSG_TYPE {
  RDE_MSG_PEER_UP = 1,
  RDE_MSG_PEER_DOWN = 2,
  RDE_MSG_PEER_INFO_REQ = 3,
  RDE_MSG_PEER_INFO_RESP = 4,
};

struct rde_peer_info {
  PCS_RDA_ROLE ha_role;
};

struct rde_msg {
  struct rde_msg *next;
  RDE_MSG_TYPE type;
  MDS_DEST fr_dest;
  NODE_ID fr_node_id;
  union {
    struct rde_peer_info peer_info;
  } info;
};

extern const char *rde_msg_name[];

/*****************************************************************************\
*                                                                             *
*                       Function Prototypes                                   *
*                                                                             *
\*****************************************************************************/

extern RDE_CONTROL_BLOCK *rde_get_control_block();
extern uint32_t rde_mds_register();
extern uint32_t rde_mds_unregister();
extern uint32_t rde_mds_send(struct rde_msg *msg, MDS_DEST to_dest);
extern uint32_t rde_set_role(PCS_RDA_ROLE role);

#endif   /* RDE_CB_H */
