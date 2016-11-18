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

#ifndef CLM_CLMD_CLMS_EVT_H_
#define CLM_CLMD_CLMS_EVT_H_

typedef enum clmsv_clms_evt_type {
  CLMSV_CLMS_CLMSV_MSG = 0,
  CLMSV_CLMS_CLMA_UP = 1,
  CLMSV_CLMS_CLMA_DOWN = 2,
  CLSMV_CLMS_QUIESCED_ACK = 3,
  CLMSV_CLMS_NODE_LOCK_TMR_EXP = 4,
  CLMSV_CLMS_MDS_NODE_EVT,
  CLMSV_CLMS_RDA_EVT,
  CLMSV_CLMS_RT_UPDATE,
  CLMSV_AVND_DOWN_EVT,
  CLMSV_CLMS_EVT_MAX,
} CLMSV_CLMS_EVT_TYPE;

typedef struct clmsv_clms_mds_info_t {
  SaUint32T node_id;
  MDS_DEST mds_dest_id;
} CLMSV_CLMS_MDS_INFO;

typedef struct clmsv_clms_node_mds_info_t {
  SaUint32T node_id;
  SaBoolT nodeup;
  SaClmNodeAddressT addr;
} CLMSV_CLMS_MDS_NODE_INFO;

typedef struct clmsv_clms_rda_info_t {
  PCS_RDA_ROLE io_role;
} CLMSV_CLMS_RDA_INFO;

typedef struct clmsv_clms_rt_node_info_t {
  CLMS_CLUSTER_NODE *node;
} CLMSV_CLMS_RT_INFO;

typedef struct clmsv_clms_evt_t {
  struct clmsv_clms_evt_t *next;  /* For MailBox Processing */
  SaUint32T cb_hdl;       /* Pointer to the CLMS Control Block */
  MDS_SYNC_SND_CTXT mds_ctxt;     /* For MDS synch response */
  MDS_DEST fr_dest;       /* Address of sender from which this event was sent */
  NODE_ID fr_node_id;     /* Node id from which this event was sent */
  MDS_SEND_PRIORITY_TYPE rcvd_prio;       /* MDS Priority of the received event */
  CLMSV_CLMS_EVT_TYPE type;
  union {
    CLMSV_MSG msg;  /* Message 'received'/'to be sent' from CLMA */
    CLMSV_CLMS_MDS_INFO mds_info;   /* CLMA `MDS reachability information */
    CLMSV_CLMS_MDS_NODE_INFO node_mds_info;
    CLMSV_CLMS_RDA_INFO rda_info;
    CLMS_LOCK_TMR tmr_info;
    CLMSV_CLMS_RT_INFO rt_node_info;
  } info;
} CLMSV_CLMS_EVT;

/* Function prototype */
typedef SaUint32T (*CLMSV_CLMS_CLMA_API_MSG_HANDLER) (CLMS_CB *, CLMSV_CLMS_EVT * evt);
typedef SaUint32T (*CLMSV_CLMS_EVT_HANDLER) (CLMSV_CLMS_EVT * evt);
/* Macros to pack and unpack invocation id*/
#define m_CLMSV_PACK_INV(inv, nodeid) ((((SaUint64T) inv) << 32) | nodeid)
#define  m_CLMSV_INV_UNPACK_INVID(inv) ((inv) >> 32)
#define  m_CLMSV_INV_UNPACK_NODEID(inv) ((inv) & 0x00000000ffffffff)

extern CLMS_CLIENT_INFO *clms_client_new(MDS_DEST mds_dest, uint32_t client_id);
extern CLMS_CLIENT_INFO *clms_client_get_by_id(uint32_t client_id);
extern CLMS_CLIENT_INFO *clms_client_getnext_by_id(uint32_t client_id);
extern uint32_t clms_client_delete_by_mds_dest(MDS_DEST mds_dest);
extern uint32_t clms_client_delete(uint32_t client_id);
extern CLMS_CLUSTER_NODE *clms_node_getnext_by_id(SaUint32T node_id);
extern uint32_t clms_clmresp_error(CLMS_CB * cb, CLMS_CLUSTER_NODE * node);
extern uint32_t clms_clmresp_rejected(CLMS_CB * cb, CLMS_CLUSTER_NODE * node, CLMS_TRACK_INFO * trkrec);
extern uint32_t clms_clmresp_ok(CLMS_CB * cb, CLMS_CLUSTER_NODE * op_node, CLMS_TRACK_INFO * trkrec);
extern uint32_t clms_remove_clma_down_rec(CLMS_CB * cb, MDS_DEST mds_dest);
extern void clms_remove_node_down_rec(SaClmNodeIdT node_id);
extern uint32_t clms_node_add(CLMS_CLUSTER_NODE * node, int i);
extern void clms_clmresp_error_timeout(CLMS_CB * cb, CLMS_CLUSTER_NODE * node);
extern bool clms_clma_entry_valid(CLMS_CB * cb, MDS_DEST mds_dest);
extern void clms_process_mbx(SYSF_MBX *mbx);
extern void clms_evt_destroy(CLMSV_CLMS_EVT * evt);
extern bool ip_matched(uint16_t family1, uint8_t *ip1, uint16_t family2, uint8_t *ip2);

#endif  // CLM_CLMD_CLMS_EVT_H_
