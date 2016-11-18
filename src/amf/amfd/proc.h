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

  This module is the  include file for Availability Directors processing module.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AMF_AMFD_PROC_H_
#define AMF_AMFD_PROC_H_

#include "amf/amfd/cb.h"
#include "amf/amfd/evt.h"
#include "amf/amfd/susi.h"

typedef void (*AVD_EVT_HDLR) (AVD_CL_CB *, AVD_EVT *);

void avd_su_oper_state_evh(AVD_CL_CB *cb, struct AVD_EVT *evt);
void avd_su_si_assign_evh(AVD_CL_CB *cb, struct AVD_EVT *evt);
uint32_t avd_new_assgn_susi(AVD_CL_CB *cb, AVD_SU *su, AVD_SI *si,
				  SaAmfHAStateT role, bool ckpt, AVD_SU_SI_REL **ret_ptr);
void su_try_repair(const AVD_SU *su);
void avd_sg_app_node_su_inst_func(AVD_CL_CB *cb, AVD_AVND *avnd);
uint32_t avd_sg_app_su_inst_func(AVD_CL_CB *cb, AVD_SG *sg);
uint32_t avd_sg_su_oper_list_add(AVD_CL_CB *cb, AVD_SU *su, bool ckpt, bool wrt_to_imm = true);
uint32_t avd_sg_su_oper_list_del(AVD_CL_CB *cb, AVD_SU *su, bool ckpt, bool wrt_to_imm = true);
uint32_t avd_sg_app_sg_admin_func(AVD_CL_CB *cb, AVD_SG *sg);
uint32_t avd_sg_su_si_mod_snd(AVD_CL_CB *cb, AVD_SU *su, SaAmfHAStateT state);
uint32_t avd_sg_susi_mod_snd_honouring_si_dependency(AVD_SU *su, SaAmfHAStateT state);
uint32_t avd_sg_su_si_del_snd(AVD_CL_CB *cb, AVD_SU *su);

/* The following are for N-Way redundancy model */
uint32_t avd_sg_nway_si_assign(AVD_CL_CB *, AVD_SG *);

/* The following are for N-way Active redundancy model */
AVD_SU *avd_sg_nacvred_su_chose_asgn(AVD_CL_CB *cb, AVD_SG *sg);

uint32_t avd_count_node_up(AVD_CL_CB *cb);
uint32_t avd_evt_queue_count(AVD_CL_CB *cb);
uint32_t avd_count_sync_node_size(AVD_CL_CB *cb);
void avd_process_state_info_queue(AVD_CL_CB *cb);
void avd_node_up_evh(AVD_CL_CB *cb, struct AVD_EVT *evt);
void avd_reg_su_evh(AVD_CL_CB *cb, struct AVD_EVT *evt);
void avd_oper_req_evh(AVD_CL_CB *cb, struct AVD_EVT *evt);
void avd_mds_avnd_up_evh(AVD_CL_CB *cb, struct AVD_EVT *evt);
void avd_ack_nack_evh(AVD_CL_CB *cb, AVD_EVT *evt);
void avd_comp_validation_evh(AVD_CL_CB *cb, AVD_EVT *evt);
void avd_fail_over_event(AVD_CL_CB *cb);
void avd_mds_avnd_down_evh(AVD_CL_CB *cb, struct AVD_EVT *evt);
void avd_data_update_req_evh(AVD_CL_CB *cb, AVD_EVT *evt);
void avd_role_switch_ncs_su_evh(AVD_CL_CB *cb, AVD_EVT *evt);
void avd_mds_qsd_role_evh(AVD_CL_CB *cb, AVD_EVT *evt);
void avd_node_down_appl_susi_failover(AVD_CL_CB *cb, AVD_AVND *avnd);
void avd_node_down_mw_susi_failover(AVD_CL_CB *cb, AVD_AVND *avnd);
void avd_node_down_func(AVD_CL_CB *cb, AVD_AVND *avnd);
void avd_nd_sisu_state_info_evh(AVD_CL_CB *cb, struct AVD_EVT *evt);
void avd_nd_compcsi_state_info_evh(AVD_CL_CB *cb, struct AVD_EVT *evt);
void avd_avnd_mds_info_evh(AVD_CL_CB *cb, AVD_EVT *evt);
uint32_t avd_node_down(AVD_CL_CB *cb, SaClmNodeIdT node_id);
AVD_AVND *avd_msg_sanity_chk(AVD_EVT *evt, SaClmNodeIdT node_id,
	AVSV_DND_MSG_TYPE msg_typ, uint32_t msg_id);
void avd_nd_ncs_su_assigned(AVD_CL_CB *cb, AVD_AVND *avnd);
void avd_nd_ncs_su_failed(AVD_CL_CB *cb, AVD_AVND *avnd);
void avd_rcv_hb_d_evh(AVD_CL_CB *cb, struct AVD_EVT *evt);
void avd_process_hb_event(AVD_CL_CB *cb_now, struct AVD_EVT *evt);
extern void avd_node_mark_absent(AVD_AVND *node);
extern void avd_tmr_snd_hb_evh(AVD_CL_CB *cb, AVD_EVT *evt);
extern void avd_node_failover(AVD_AVND *node);
extern AVD_SU *get_other_su_from_oper_list(AVD_SU *su);
extern void su_complete_admin_op(AVD_SU *su, SaAisErrorT result);
extern void comp_complete_admin_op(AVD_COMP *comp, SaAisErrorT result);
extern void su_disable_comps(AVD_SU *su, SaAisErrorT result);
extern bool cluster_su_instantiation_done(AVD_CL_CB *cb, AVD_SU *su);
extern void cluster_startup_expiry_event_generate(AVD_CL_CB *cb);

#endif  // AMF_AMFD_PROC_H_
