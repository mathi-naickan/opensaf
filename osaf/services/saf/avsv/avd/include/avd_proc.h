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
#ifndef AVD_PROC_H
#define AVD_PROC_H

#include <avd_cb.h>
#include <avd_evt.h>
#include <avd_susi.h>

typedef void (*AVD_EVT_HDLR) (AVD_CL_CB *, AVD_EVT *);

void avd_main_proc(void);

void avd_su_oper_state_evh(AVD_CL_CB *cb, struct avd_evt_tag *evt);
void avd_su_si_assign_evh(AVD_CL_CB *cb, struct avd_evt_tag *evt);
uint32_t avd_new_assgn_susi(AVD_CL_CB *cb, AVD_SU *su, AVD_SI *si,
				  SaAmfHAStateT role, bool ckpt, AVD_SU_SI_REL **ret_ptr);
void avd_sg_app_node_su_inst_func(AVD_CL_CB *cb, AVD_AVND *avnd);
uint32_t avd_sg_app_su_inst_func(AVD_CL_CB *cb, AVD_SG *sg);
uint32_t avd_sg_su_oper_list_add(AVD_CL_CB *cb, AVD_SU *su, bool ckpt);
uint32_t avd_sg_su_oper_list_del(AVD_CL_CB *cb, AVD_SU *su, bool ckpt);
uint32_t avd_sg_su_asgn_del_util(AVD_CL_CB *cb, AVD_SU *su, bool del_flag, bool q_flag);
uint32_t avd_sg_app_sg_admin_func(AVD_CL_CB *cb, AVD_SG *sg);
uint32_t avd_sg_su_si_mod_snd(AVD_CL_CB *cb, AVD_SU *su, SaAmfHAStateT state);
uint32_t avd_sg_susi_mod_snd_honouring_si_dependency(AVD_SU *su, SaAmfHAStateT state);
uint32_t avd_sg_su_si_del_snd(AVD_CL_CB *cb, AVD_SU *su);

/* The following are for 2N redundancy model */
uint32_t avd_sg_2n_si_func(AVD_CL_CB *cb, AVD_SI *si);
uint32_t avd_sg_2n_su_insvc_func(AVD_CL_CB *cb, AVD_SU *su);
extern SaAisErrorT avd_sg_2n_siswap_func(AVD_SI *si, SaInvocationT invocation);
uint32_t avd_sg_2n_su_fault_func(AVD_CL_CB *cb, AVD_SU *su);
uint32_t avd_sg_2n_susi_sucss_func(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
					 AVSV_SUSI_ACT act, SaAmfHAStateT state);
uint32_t avd_sg_2n_susi_fail_func(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
					AVSV_SUSI_ACT act, SaAmfHAStateT state);
uint32_t avd_sg_2n_realign_func(AVD_CL_CB *cb, AVD_SG *sg);
uint32_t avd_sg_2n_su_admin_fail(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd);
uint32_t avd_sg_2n_si_admin_down(AVD_CL_CB *cb, AVD_SI *si);
uint32_t avd_sg_2n_sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg);
void avd_sg_2n_node_fail_func(AVD_CL_CB *cb, AVD_SU *su);
void avd_sg_2n_init(AVD_SG *sg);

/* The following are for N-Way redundancy model */
uint32_t avd_sg_nway_si_assign(AVD_CL_CB *, AVD_SG *);
uint32_t avd_sg_nway_si_func(AVD_CL_CB *cb, AVD_SI *si);
uint32_t avd_sg_nway_su_insvc_func(AVD_CL_CB *cb, AVD_SU *su);
uint32_t avd_sg_nway_siswitch_func(AVD_CL_CB *cb, AVD_SI *si);
uint32_t avd_sg_nway_su_fault_func(AVD_CL_CB *cb, AVD_SU *su);
uint32_t avd_sg_nway_susi_sucss_func(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
					   AVSV_SUSI_ACT act, SaAmfHAStateT state);
uint32_t avd_sg_nway_susi_fail_func(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
					  AVSV_SUSI_ACT act, SaAmfHAStateT state);
uint32_t avd_sg_nway_realign_func(AVD_CL_CB *cb, AVD_SG *sg);
uint32_t avd_sg_nway_su_admin_fail(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd);
uint32_t avd_sg_nway_si_admin_down(AVD_CL_CB *cb, AVD_SI *si);
uint32_t avd_sg_nway_sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg);
void avd_sg_nway_node_fail_func(AVD_CL_CB *cb, AVD_SU *su);
void avd_sg_nway_init(AVD_SG *sg);

/* The following are for N+M redundancy model */
uint32_t avd_sg_npm_si_func(AVD_CL_CB *cb, AVD_SI *si);
uint32_t avd_sg_npm_su_insvc_func(AVD_CL_CB *cb, AVD_SU *su);
uint32_t avd_sg_npm_siswitch_func(AVD_CL_CB *cb, AVD_SI *si);
uint32_t avd_sg_npm_su_fault_func(AVD_CL_CB *cb, AVD_SU *su);
uint32_t avd_sg_npm_susi_sucss_func(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
					  AVSV_SUSI_ACT act, SaAmfHAStateT state);
uint32_t avd_sg_npm_susi_fail_func(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
					 AVSV_SUSI_ACT act, SaAmfHAStateT state);
uint32_t avd_sg_npm_realign_func(AVD_CL_CB *cb, AVD_SG *sg);
uint32_t avd_sg_npm_su_admin_fail(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd);
uint32_t avd_sg_npm_si_admin_down(AVD_CL_CB *cb, AVD_SI *si);
uint32_t avd_sg_npm_sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg);
void avd_sg_npm_node_fail_func(AVD_CL_CB *cb, AVD_SU *su);
void avd_sg_npm_init(AVD_SG *sg);

/* The following are for No redundancy model */
uint32_t avd_sg_nored_si_func(AVD_CL_CB *cb, AVD_SI *si);
uint32_t avd_sg_nored_su_insvc_func(AVD_CL_CB *cb, AVD_SU *su);
uint32_t avd_sg_nored_su_fault_func(AVD_CL_CB *cb, AVD_SU *su);
uint32_t avd_sg_nored_susi_sucss_func(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
					    AVSV_SUSI_ACT act, SaAmfHAStateT state);
uint32_t avd_sg_nored_susi_fail_func(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
					   AVSV_SUSI_ACT act, SaAmfHAStateT state);
uint32_t avd_sg_nored_realign_func(AVD_CL_CB *cb, AVD_SG *sg);
uint32_t avd_sg_nored_su_admin_fail(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd);
uint32_t avd_sg_nored_si_admin_down(AVD_CL_CB *cb, AVD_SI *si);
uint32_t avd_sg_nored_sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg);
void avd_sg_nored_node_fail_func(AVD_CL_CB *cb, AVD_SU *su);
void avd_sg_nored_init(AVD_SG *sg);

/* The following are for N-way Active redundancy model */
AVD_SU *avd_sg_nacvred_su_chose_asgn(AVD_CL_CB *cb, AVD_SG *sg);
uint32_t avd_sg_nacvred_si_func(AVD_CL_CB *cb, AVD_SI *si);
uint32_t avd_sg_nacvred_su_insvc_func(AVD_CL_CB *cb, AVD_SU *su);
uint32_t avd_sg_nacvred_su_fault_func(AVD_CL_CB *cb, AVD_SU *su);
uint32_t avd_sg_nacvred_susi_sucss_func(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
					      AVSV_SUSI_ACT act, SaAmfHAStateT state);
uint32_t avd_sg_nacvred_susi_fail_func(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi,
					     AVSV_SUSI_ACT act, SaAmfHAStateT state);
uint32_t avd_sg_nacvred_realign_func(AVD_CL_CB *cb, AVD_SG *sg);
uint32_t avd_sg_nacvred_su_admin_fail(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd);
uint32_t avd_sg_nacvred_si_admin_down(AVD_CL_CB *cb, AVD_SI *si);
uint32_t avd_sg_nacvred_sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg);
void avd_sg_nacvred_node_fail_func(AVD_CL_CB *cb, AVD_SU *su);
void avd_sg_nacv_init(AVD_SG *sg);

void avd_node_up_evh(AVD_CL_CB *cb, struct avd_evt_tag *evt);
void avd_reg_su_evh(AVD_CL_CB *cb, struct avd_evt_tag *evt);
void avd_oper_req_evh(AVD_CL_CB *cb, struct avd_evt_tag *evt);
void avd_mds_avnd_up_evh(AVD_CL_CB *cb, struct avd_evt_tag *evt);
void avd_ack_nack_evh(AVD_CL_CB *cb, AVD_EVT *evt);
void avd_comp_validation_evh(AVD_CL_CB *cb, AVD_EVT *evt);
void avd_fail_over_event(AVD_CL_CB *cb);
void avd_mds_avnd_down_evh(AVD_CL_CB *cb, struct avd_evt_tag *evt);
void avd_data_update_req_evh(AVD_CL_CB *cb, AVD_EVT *evt);
void avd_role_switch_ncs_su_evh(AVD_CL_CB *cb, AVD_EVT *evt);
void avd_mds_qsd_role_evh(AVD_CL_CB *cb, AVD_EVT *evt);
void avd_node_down_appl_susi_failover(AVD_CL_CB *cb, AVD_AVND *avnd);
void avd_node_down_mw_susi_failover(AVD_CL_CB *cb, AVD_AVND *avnd);
void avd_node_down_func(AVD_CL_CB *cb, AVD_AVND *avnd);
uint32_t avd_node_down(AVD_CL_CB *cb, SaClmNodeIdT node_id);
AVD_AVND *avd_msg_sanity_chk(AVD_EVT *evt, SaClmNodeIdT node_id,
	AVSV_DND_MSG_TYPE msg_typ, uint32_t msg_id);
void avd_nd_ncs_su_assigned(AVD_CL_CB *cb, AVD_AVND *avnd);
void avd_nd_ncs_su_failed(AVD_CL_CB *cb, AVD_AVND *avnd);
void avd_rcv_hb_d_evh(AVD_CL_CB *cb, struct avd_evt_tag *evt);
void avd_process_hb_event(AVD_CL_CB *cb_now, struct avd_evt_tag *evt);
extern void avd_node_mark_absent(AVD_AVND *node);
extern void avd_tmr_snd_hb_evh(AVD_CL_CB *cb, AVD_EVT *evt);
extern void avd_node_failover(AVD_AVND *node);
extern AVD_SU *get_other_su_from_oper_list(AVD_SU *su);

#endif
