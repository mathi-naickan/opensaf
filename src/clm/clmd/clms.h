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

#ifndef CLM_CLMD_CLMS_H_
#define CLM_CLMD_CLMS_H_

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#ifdef HAVE_CONFIG_H
#include "osaf/config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include "amf/saf/saAmf.h"

#include "base/ncsgl_defs.h"
#include "base/ncs_lib.h"
#include "mds/mds_papi.h"
#include "base/ncs_main_papi.h"
#include "base/ncs_mda_pvt.h"
#include "mbc/mbcsv_papi.h"
#include "base/ncs_edu_pub.h"
#include "base/ncs_util.h"
#include "base/logtrace.h"
#include "osaf/immutil/immutil.h"
#include "imm/saf/saImmOi.h"
#include "clm/saf/saClm.h"
#include "rde/agent/rda_papi.h"
#include "ntf/saf/saNtf.h"
#include "base/ncssysf_def.h"
#ifdef ENABLE_AIS_PLM
#include "plm/saf/saPlm.h"
#endif
#include "osaf/saflog/saflog.h"

/* CLMS files */
#include "clm/clmsv_defs.h"
#include "clm/clmsv_msg.h"
#include "clm/clmsv_enc_dec.h"
#include "clms_cb.h"
#include "clms_mbcsv.h"
#include "clms_evt.h"
#include "clms_imm.h"
#include "clms_ntf.h"
#include "base/saf_error.h"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

#define sec_to_nanosec 1000000000L
#define OSAF_AF_TIPC 30

extern CLMS_CB *clms_cb;
extern CLMS_CLUSTER_INFO *osaf_cluster;
extern const SaNameT *clmSvcUsrName;

extern uint32_t initialize_for_assignment(CLMS_CB *cb, SaAmfHAStateT ha_state);
extern uint32_t clms_amf_init(CLMS_CB *);
extern uint32_t clms_mds_init(CLMS_CB * cb);
extern uint32_t clms_cb_init(CLMS_CB * clms_cb);
extern uint32_t clms_mds_finalize(CLMS_CB * cb);
extern uint32_t clms_mds_change_role(CLMS_CB * cb);

extern uint32_t clms_mds_msg_send(CLMS_CB * cb,
                                  CLMSV_MSG * msg,
                                  MDS_DEST *dest,
                                  MDS_SYNC_SND_CTXT *mds_ctxt, MDS_SEND_PRIORITY_TYPE prio, NCSMDS_SVC_ID svc_id);

extern uint32_t clms_mds_msg_bcast(CLMS_CB *cb, CLMSV_MSG *bcast_msg);
extern SaAisErrorT clms_imm_activate(CLMS_CB * cb);
extern uint32_t clms_node_trackresplist_empty(CLMS_CLUSTER_NODE * op_node);
extern uint32_t clms_send_cbk_start_sub(CLMS_CB * cb, CLMS_CLUSTER_NODE * node);
extern void clms_clear_node_dep_list(CLMS_CLUSTER_NODE * node);
extern uint32_t clms_client_del_trackresp(SaUint32T client_id);
extern CLMS_CLUSTER_NODE *clms_node_get_by_name(const SaNameT *name);
extern CLMS_CLUSTER_NODE *clms_node_getnext_by_name(const SaNameT *name);
extern CLMS_CLUSTER_NODE *clms_node_get_by_id(SaUint32T nodeid);
/* extern void clms_imm_impl_set(CLMS_CB *cb); */
extern SaAisErrorT clms_imm_init(CLMS_CB * cb);
#ifdef ENABLE_AIS_PLM
extern SaAisErrorT clms_plm_init(CLMS_CB * cb);
#endif
extern void clms_node_add_to_model(CLMS_CLUSTER_NODE * node);
extern SaTimeT clms_get_SaTime(void);
extern SaTimeT clms_get_BootTime(void);
extern void clms_imm_impl_set(CLMS_CB * cb);
extern uint32_t clms_rda_init(CLMS_CB * cb);
extern void clms_adminop_pending(void);
extern void ckpt_node_rec(CLMS_CLUSTER_NODE * node);
extern void ckpt_node_down_rec(CLMS_CLUSTER_NODE * node);
extern void ckpt_cluster_rec(void);
extern void clms_cb_dump(void);
extern uint32_t clms_send_is_member_info(CLMS_CB * cb, SaClmNodeIdT node_id,  SaBoolT member, SaBoolT is_configured);
extern void clm_imm_reinit_bg(CLMS_CB * cb);
extern void proc_downs_during_rolechange (void);
extern void clms_cluster_reboot(void);
#endif  // CLM_CLMD_CLMS_H_
