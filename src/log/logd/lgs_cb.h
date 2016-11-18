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

#ifndef LOG_LOGD_LGS_CB_H_
#define LOG_LOGD_LGS_CB_H_

#include <stdbool.h>
#include "log/saf/saLog.h"
#include "imm/saf/saImmOi.h"
#include "clm/saf/saClm.h"
#include "mbc/mbcsv_papi.h"
#include "base/ncs_edu_pub.h"

#include "lgs_stream.h"

/* LGS Recovery states */
typedef enum {
  LGS_NORMAL,     /* No recovery is ongoing. All requests are handled normally */
  LGS_RECOVERY    /* Recover streams if in recovery list when stream open
                   * request with no parameters
                   */
} lgs_state_t;

/* Default HA state assigned locally during lgs initialization */
#define LGS_HA_INIT_STATE 0

/* CHECKPOINT status */
typedef enum checkpoint_status {
  COLD_SYNC_IDLE = 0,
  REG_REC_SENT,
  CHANNEL_REC_SENT,
  SUBSCRIPTION_REC_SENT,
  COLD_SYNC_COMPLETE,
  WARM_SYNC_IDLE,
  WARM_SYNC_CSUM_SENT,
  WARM_SYNC_COMPLETE,
} CHECKPOINT_STATE;

typedef struct lgs_stream_list {
  uint32_t stream_id;
  struct lgs_stream_list *next;
} lgs_stream_list_t;

typedef struct {
  NCS_PATRICIA_NODE pat_node;
  uint32_t client_id;
  uint32_t client_id_net;
  MDS_DEST mds_dest;
  lgs_stream_list_t *stream_list_root;
} log_client_t;

typedef struct lga_down_list_tag {
  MDS_DEST mds_dest;
  struct lga_down_list_tag *next;
} LGA_DOWN_LIST;

typedef struct lgs_cb {
  MDS_HDL mds_hdl;        /* PWE Handle for interacting with LGAs      */
  V_DEST_RL mds_role;     /* Current MDS role - ACTIVE/STANDBY         */
  MDS_DEST vaddr;         /* My identification in MDS                  */
  SaVersionT log_version; /* The version currently supported           */
  NCS_PATRICIA_TREE client_tree;  /* LGA/Library/Client instantiation pat. tree */
  SaNameT comp_name;      /* Components's name LGS                     */
  SaAmfHandleT amf_hdl;   /* AMF handle, obtained thru AMF init        */
  SaSelectionObjectT amfSelectionObject;  /* Selection Object to wait for AMF events */
  SaInvocationT amf_invocation_id;        /* AMF InvocationID - needed to handle Quiesed state */
  bool is_quiesced_set;
  SaImmOiHandleT immOiHandle;     /* IMM OI handle                           */
  SaSelectionObjectT immSelectionObject;  /* Selection Object to wait for IMM events */
  SaSelectionObjectT clmSelectionObject;  /* Selection Object to wait for clms events */
  SaClmHandleT clm_hdl;   /* CLM handle, obtained through CLM init        */
  SaAmfHAStateT ha_state; /* present AMF HA state of the component     */
  uint32_t last_client_id;        /* Value of last client_id assigned          */
  uint32_t async_upd_cnt; /* Async Update Count for Warmsync */
  CHECKPOINT_STATE ckpt_state;    /* Current record that has been checkpointed */
  NCS_MBCSV_HDL mbcsv_hdl;        /* Handle obtained during mbcsv init */
  SaSelectionObjectT mbcsv_sel_obj;       /* Selection object to wait for MBCSv events */
  NCS_MBCSV_CKPT_HDL mbcsv_ckpt_hdl;      /* MBCSv handle obtained during checkpoint open */
  uint16_t mbcsv_peer_version;            /* Checkpoint peer version */
  EDU_HDL edu_hdl;        /* Handle from EDU for encode/decode operations */
  bool fully_initialized;
  LGA_DOWN_LIST *lga_down_list_head;      /* LGA down reccords - Fix for Failover missed
                                             down events Processing */
  LGA_DOWN_LIST *lga_down_list_tail;

  NCS_SEL_OBJ clm_init_sel_obj; /* Selection object for CLM initialization.*/
  bool nid_started;       /**< true if started by NID */
  SaUint32T scAbsenceAllowed; /* OpenSAF global configuration for recovery handling */
  lgs_state_t lgs_recovery_state; /* Indicate current recovery state for the server */

  // Initialize default value in contructor
  lgs_cb() : lgs_recovery_state(LGS_NORMAL) {};
} lgs_cb_t;

extern uint32_t lgs_cb_init(lgs_cb_t *);
extern void lgs_process_mbx(SYSF_MBX *mbx);
extern uint32_t lgs_stream_add(lgs_cb_t *cb, log_stream_t *stream);

#endif  // LOG_LOGD_LGS_CB_H_
