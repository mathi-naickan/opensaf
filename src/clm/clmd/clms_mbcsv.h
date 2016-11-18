/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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

#ifndef CLM_CLMD_CLMS_MBCSV_H_
#define CLM_CLMD_CLMS_MBCSV_H_

#include <stdint.h>
#include "amf/saf/saAmf.h"

#define CLMS_MBCSV_VERSION 1
#define CLMS_MBCSV_VERSION_MIN 1

/* CLMS->CLMS ACTIVE->STANDBY checkpoint message types */
typedef enum clms_ckpt_rec_type {
  CLMS_CKPT_CLUSTER_REC = 0,
  CLMS_CKPT_CLIENT_INFO_REC,
  CLMS_CKPT_FINALIZE_REC,
  CLMS_CKPT_TRACK_CHANGES_REC,
  CLMS_CKPT_NODE_REC,     /*For Cold Sync */
  CLMS_CKPT_NODE_CONFIG_REC,      /* For CCB create */
  CLMS_CKPT_NODE_RUNTIME_REC,     /* For Async updates */
  CLMS_CKPT_NODE_DEL_REC, /* For CCB delete */
  CLMS_CKPT_AGENT_DOWN_REC,
  CLMS_CKPT_NODE_DOWN_REC,
  CLMS_CKPT_MSG_MAX
} CLMS_CKPT_REC_TYPE;

/* Checkpoint structure for client record */
typedef struct clms_ckpt_client {
  SaUint32T client_id;
  MDS_DEST mds_dest;
  SaUint8T track_flags;
} CLMSV_CKPT_CLIENT_INFO;

/* Checkpoint structure for finalize record */
typedef struct clms_ckpt_finalize_rec {
  SaUint32T client_id;
} CLMSV_CKPT_FINALIZE_INFO;

/* Checkpoint structure for node's runtime data */
typedef struct clms_ckpt_node_rec {
  SaClmNodeIdT node_id;
  SaNameT node_name;
  SaBoolT member;
  SaTimeT boot_time;
  SaUint64T init_view;
  SaClmAdminStateT admin_state;
  ADMIN_OP admin_op;      /*plm or clm operation */
  SaClmClusterChangesT change;
  SaBoolT nodeup;         /*Check for the connectivity */
#ifdef ENABLE_AIS_PLM
  SaPlmReadinessStateT ee_red_state;
#endif
} CLMSV_CKPT_NODE_RUNTIME_INFO;

typedef struct clms_ckpt_node_down_rec {
  SaClmNodeIdT node_id;
} CLMSV_CKPT_NODE_DOWN_INFO;

/* Checkpoint structure for node's config data */
typedef struct clms_ckpt_node_config_rec {
  SaNameT node_name;
  SaNameT ee_name;
  SaClmNodeAddressT node_addr;
  SaBoolT disable_reboot;
  SaTimeT lck_cbk_timeout;
  SaClmAdminStateT admin_state;
} CLMSV_CKPT_NODE_CONFIG_REC;

/* Checkpoint structure for node - cold sync */
typedef struct clms_ckpt_node {
  SaClmNodeIdT node_id;
  SaClmNodeAddressT node_addr;
  SaNameT node_name;
  SaNameT ee_name;
  SaBoolT member;
  SaTimeT boot_time;
  SaUint64T init_view;
  SaBoolT disable_reboot;
  SaTimeT lck_cbk_timeout;
  SaClmAdminStateT admin_state;
  SaClmClusterChangesT change;
#ifdef ENABLE_AIS_PLM
  SaPlmReadinessStateT ee_red_state;
#endif
  SaBoolT nodeup;         /*Check for the connectivity */
  SaInvocationT curr_admin_inv;
  SaBoolT stat_change;    /*Required to check for the number of nodes on which change has occured */
  ADMIN_OP admin_op;      /*plm or clm operation */
  SaInvocationT plm_invid;        /*plmtrack callback invocation id */
} CLMSV_CKPT_NODE;

/* Checkpoint structure for node's delete data */
typedef struct clms_ckpt_node_del_rec {
  SaNameT node_name;      /* Async update for CCB node delete */
} CLMSV_CKPT_NODE_DEL_REC;

/* Checkpoint structure for tracklist per node */
typedef struct clms_ckpt_tracklist {
  MDS_DEST client_dest;
  SaUint32T client_id;
  SaInvocationT inv_id;
} CLMSV_CKPT_TRACKLIST;

/* Checkpoint structure for cluster runtime data */
typedef struct clms_ckpt_cluster {
  SaUint32T num_nodes;
  SaTimeT init_time;
  SaUint64T cluster_view_num;
} CLMSV_CKPT_CLUSTER_INFO;

/* Checkpoint structure for agent_down async updates */
typedef struct clms_ckpt_agent_down {
  MDS_DEST mds_dest;
} CLMSV_CKPT_AGENT_DOWN_REC;

/* Checkpoint message containing clms data of a particular type.
 * Used during cold and async updates.
 */
typedef struct clms_ckpt_header_t {
  CLMS_CKPT_REC_TYPE type;        /* Checkpoint record type */
  uint32_t num_ckpt_records;
  uint32_t data_len;              /* Total length of encoded checkpoint data of this type */
} CLMSV_CKPT_HEADER;

/* Top level structure for Checkpointing */
typedef struct clms_ckpt_rec_t {
  CLMSV_CKPT_HEADER header;
  union {
    CLMSV_CKPT_CLUSTER_INFO cluster_rec;
    CLMSV_CKPT_CLIENT_INFO client_rec;
    CLMSV_CKPT_FINALIZE_INFO finalize_rec;
    CLMSV_CKPT_NODE_RUNTIME_INFO node_rec;
    CLMSV_CKPT_NODE node_csync_rec;
    CLMSV_CKPT_NODE_CONFIG_REC node_config_rec;
    CLMSV_CKPT_NODE_DEL_REC node_del_rec;
    CLMSV_CKPT_TRACKLIST track_rec;
    CLMSV_CKPT_AGENT_DOWN_REC agent_rec;
    CLMSV_CKPT_NODE_DOWN_INFO node_down_rec;
  } param;
} CLMS_CKPT_REC;

typedef uint32_t (*CLMS_CKPT_HDLR) (CLMS_CB * cb, CLMS_CKPT_REC * data);
extern uint32_t clms_mbcsv_change_HA_state(CLMS_CB * cb, SaAmfHAStateT ha_state);
extern uint32_t clms_send_async_update(CLMS_CB * cb, CLMS_CKPT_REC * ckpt_rec, uint32_t action);
extern void prepare_ckpt_node(CLMSV_CKPT_NODE * node, CLMS_CLUSTER_NODE * cluster_node);
extern void prepare_cluster_node(CLMS_CLUSTER_NODE * node, CLMSV_CKPT_NODE * cluster_node);
extern void prepare_ckpt_config_node(CLMSV_CKPT_NODE_CONFIG_REC * node, CLMS_CLUSTER_NODE * cluster_node);
extern uint32_t clms_mbcsv_init(CLMS_CB * cb, SaAmfHAStateT ha_state);
extern uint32_t clms_mbcsv_dispatch(NCS_MBCSV_HDL mbcsv_hdl);
extern void prepare_ckpt_to_ckpt_node(CLMSV_CKPT_NODE * node, CLMSV_CKPT_NODE * cluster_node);
extern void prepare_ckpt_to_ckpt_config_node(CLMSV_CKPT_NODE_CONFIG_REC * node,
                                             CLMSV_CKPT_NODE_CONFIG_REC * cluster_node);
extern uint32_t encodeNodeAddressT(NCS_UBAID *uba, SaClmNodeAddressT *nodeAddress);

#endif  // CLM_CLMD_CLMS_MBCSV_H_
