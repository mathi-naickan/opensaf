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

  This module is the include file for Availability Node Directors processing
  module.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVND_PROC_H
#define AVND_PROC_H

struct avnd_cb_tag;
struct avnd_evt_tag;
struct avnd_comp_tag;
struct avnd_pxied_rec;

typedef uns32 (*AVND_EVT_HDLR) (struct avnd_cb_tag *, struct avnd_evt_tag *);

EXTERN_C void avnd_main_process(void);

EXTERN_C uns32 avnd_evt_avd_node_up_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_avd_reg_su_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_avd_reg_comp_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_comp_proxied_add(struct avnd_cb_tag *, struct avnd_comp_tag *, struct avnd_comp_tag *, NCS_BOOL);;
EXTERN_C uns32 avnd_comp_proxied_del(struct avnd_cb_tag *, struct avnd_comp_tag *, struct avnd_comp_tag *, NCS_BOOL,
				     struct avnd_pxied_rec *);
EXTERN_C uns32 avnd_evt_avd_info_su_si_assign_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_avd_pg_track_act_rsp_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_avd_pg_upd_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_avd_operation_request_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_avd_su_pres_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_avd_oper_state_resp_msg(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_avd_comp_validation_resp_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_avd_role_change_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);

EXTERN_C uns32 avnd_evt_ava_finalize_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_ava_comp_reg_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_ava_comp_unreg_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_ava_pm_start_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_ava_pm_stop_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_ava_hc_start_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_ava_hc_stop_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_ava_hc_confirm_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_ava_csi_quiescing_compl_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_ava_ha_get_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_ava_pg_start_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_ava_pg_stop_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_ava_err_rep_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_ava_err_clear_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_ava_resp_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);

EXTERN_C uns32 avnd_evt_cla_finalize(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_cla_track_start(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_cla_track_stop(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_cla_node_get(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_cla_node_async_get(struct avnd_cb_tag *, struct avnd_evt_tag *);

EXTERN_C uns32 avnd_evt_tmr_hc_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_tmr_cbk_resp_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_tmr_rcv_msg_rsp_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_tmr_clc_comp_reg_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_tmr_su_err_esc_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_tmr_node_err_esc_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_tmr_clc_pxied_comp_inst_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_tmr_clc_pxied_comp_reg_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_tmr_qscing_cmpl_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);

extern void avnd_send_node_up_msg(void);
EXTERN_C uns32 avnd_evt_mds_avd_up_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_mds_avd_dn_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_mds_ava_dn_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_mds_cla_dn_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_mds_avnd_up_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_mds_avnd_dn_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);

EXTERN_C uns32 avnd_evt_clc_resp_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);

EXTERN_C uns32 avnd_evt_comp_pres_fsm_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_avd_shutdown_app_su_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_avd_verify_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_avd_ack_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_avd_clm_node_on_fover(struct avnd_cb_tag *cb, struct avnd_evt_tag *evt);

EXTERN_C uns32 avnd_evt_last_step_term_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);

EXTERN_C uns32 avnd_evt_avd_set_leds_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_destroy(void);
EXTERN_C uns32 avnd_evt_avnd_avnd_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_ha_state_change_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_comp_admin_op_req (struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_avd_admin_op_req_evh(struct avnd_cb_tag *cb, struct avnd_evt_tag *evt);
EXTERN_C uns32 avnd_evt_avd_hb_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_tmr_avd_hb_duration_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);
EXTERN_C uns32 avnd_evt_avd_reboot_evh(struct avnd_cb_tag *, struct avnd_evt_tag *);

#endif
