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

  This module is the include file for Availability Director for message 
  processing module.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVD_UTIL_H
#define AVD_UTIL_H

#include <string>

#include <amf_d2nmsg.h>
#include <cb.h>
#include <amf_util.h>
#include <immutil.h>
#include "msg.h"
#include "role.h"

class AVD_SU;

extern const SaNameT *amfSvcUsrName;
extern const char *avd_adm_state_name[];
extern const char *avd_pres_state_name[];
extern const char *avd_oper_state_name[];
extern const char *avd_readiness_state_name[];
extern const char *avd_ass_state[];
extern const char *avd_ha_state[];
extern const char *avd_proxy_status_name[];
extern const char *amf_recovery[];

struct cl_cb_tag;
class AVD_AVND;
struct avd_hlt_tag;
struct avd_su_si_rel_tag;
class AVD_COMP;
struct avd_comp_csi_rel_tag;
class AVD_CSI;

int get_child_dn_from_ass_dn(const SaNameT *ass_dn, SaNameT *child_dn);
void avd_d2n_reboot_snd(AVD_AVND *node);
bool admin_op_is_valid(SaImmAdminOperationIdT opId, AVSV_AMF_CLASS_ID class_id);
void amflog(int priority, const char *format, ...);

uint32_t avd_snd_node_ack_msg(struct cl_cb_tag *cb, AVD_AVND *avnd, uint32_t msg_id);
uint32_t avd_snd_node_data_verify_msg(struct cl_cb_tag *cb, AVD_AVND *avnd);
uint32_t avd_snd_node_up_msg(struct cl_cb_tag *cb, AVD_AVND *avnd, uint32_t msg_id_ack);
uint32_t avd_snd_presence_msg(struct cl_cb_tag *cb, AVD_SU *su, bool term_state);
uint32_t avd_snd_oper_state_msg(struct cl_cb_tag *cb, AVD_AVND *avnd, uint32_t msg_id_ack);
uint32_t avd_snd_op_req_msg(struct cl_cb_tag *cb, AVD_AVND *avnd, AVSV_PARAM_INFO *param_info);
uint32_t avd_snd_su_reg_msg(struct cl_cb_tag *cb, AVD_AVND *avnd, bool fail_over);
uint32_t avd_snd_su_msg(struct cl_cb_tag *cb, AVD_SU *su);
uint32_t avd_snd_susi_msg(struct cl_cb_tag *cb, AVD_SU *su, struct avd_su_si_rel_tag *susi,
				AVSV_SUSI_ACT actn, bool single_csi, struct avd_comp_csi_rel_tag*);
uint32_t avd_snd_set_leds_msg(struct cl_cb_tag *cb, AVD_AVND *avnd);

uint32_t avd_snd_pg_resp_msg(struct cl_cb_tag *, AVD_AVND *, AVD_CSI *,
				   AVSV_N2D_PG_TRACK_ACT_MSG_INFO *);
uint32_t avd_snd_pg_upd_msg(struct cl_cb_tag *, AVD_AVND *, struct avd_comp_csi_rel_tag *,
				  SaAmfProtectionGroupChangesT, SaNameT *);
uint32_t avd_snd_comp_validation_resp(struct cl_cb_tag *cb, AVD_AVND *avnd,
					    AVD_COMP *comp_ptr, AVD_DND_MSG *n2d_msg);
std::string to_string(const SaNameT &s);
extern int avd_admin_state_is_valid(SaAmfAdminStateT state, const CcbUtilOperationData_t *opdata);
extern SaAisErrorT avd_object_name_create(SaNameT *rdn_attr_value, SaNameT *parentName, SaNameT *object_name);
int amfd_file_dump(const char* filename);
extern int avd_admin_op_msg_snd(const SaNameT *dn, AVSV_AMF_CLASS_ID class_id,
	SaAmfAdminOperationIdT opId, AVD_AVND *node);
extern void d2n_msg_free(AVSV_DND_MSG *msg);
extern const char* avd_getparent(const char* dn);
extern bool object_exist_in_imm(const SaNameT *dn);
extern const char *admin_op_name(SaAmfAdminOperationIdT opid);
void avd_association_namet_init(const SaNameT *associate_dn, SaNameT *child,
                SaNameT *parent, AVSV_AMF_CLASS_ID parent_class_id);
#endif
