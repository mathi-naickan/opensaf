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

  DESCRIPTION: Service Unit class definition
  
******************************************************************************
*/

#ifndef AMF_AMFD_SU_H_
#define AMF_AMFD_SU_H_

#include "amf/saf/saAmf.h"
#include "amf/amfd/def.h"
#include "amf/amfd/cb.h"
#include "amf/amfd/node.h"
#include "amf/amf_defs.h"
#include "amf/amfd/msg.h"
#include "amf/amfd/comp.h"
#include "amf/amf_db_template.h"

class AVD_SG;
class AVD_SUTYPE;

/**
 * AMF director Service Unit representation.
 */
//TODO: all attributes that have a setter should probably have an getter
class AVD_SU {
 public:
     	AVD_SU();
	explicit AVD_SU(const std::string& dn);
	~AVD_SU() {};

	std::string name;
	std::string saAmfSUType;
	uint32_t saAmfSURank;
	std::string saAmfSUHostNodeOrNodeGroup;
	bool saAmfSUFailover;
	/* true when user has configured saAmfSUFailover */
	bool saAmfSUFailover_configured;
	std::string saAmfSUMaintenanceCampaign;

	/* runtime attributes */
	SaBoolT saAmfSUPreInstantiable;  // TODO(hafe) change to bool
	SaAmfOperationalStateT saAmfSUOperState;
	SaAmfAdminStateT saAmfSUAdminState;
	SaAmfReadinessStateT saAmfSuReadinessState;
	SaAmfPresenceStateT saAmfSUPresenceState;
	std::string saAmfSUHostedByNode;
	SaUint32T saAmfSUNumCurrActiveSIs;
	SaUint32T saAmfSUNumCurrStandbySIs;
	SaUint32T saAmfSURestartCount;

	AVD_ADMIN_OPER_CBK pend_cbk;	/* Stores zero invocation value of imm adm cbk
					 * when no admin operation is going on.
					 */

	bool term_state;	/* admin state to terminate the
				 * service unit.
				 * Checkpointing - Updated independently.
				 */

	SaToggleState su_switch;	/* The field that indicates if
					 * the active SIs assigned to 
					 * this SU needs to be Toggled.
					 * Checkpointing - Updated independently.
					 */

	bool su_is_external;	/* indicates if this SU is external */

	int su_act_state; // not used, kept for EDU, remove later

	AVD_SG *sg_of_su;	/* the service group of this SU */
	AVD_AVND *su_on_node;	/*  the node on which this SU resides */
	struct avd_su_si_rel_tag *list_of_susi;	/* the list of su si relationship elements */

	std::vector<AVD_COMP*> list_of_comp;	/* the list of  components in this SU */

	AVD_SUTYPE *su_type;
	AVD_SU *su_list_su_type_next;

	void set_su_failover(bool value);
	void dec_curr_stdby_si();
	void inc_curr_stdby_si();
	void inc_curr_act_si();
	void dec_curr_act_si();
	int hastate_assignments_count(SaAmfHAStateT ha_state);
	void add_comp(AVD_COMP *comp);
	void remove_comp(AVD_COMP *comp);
	void set_admin_state(SaAmfAdminStateT admin_state);
	void set_pres_state(SaAmfPresenceStateT state);
	void set_readiness_state(SaAmfReadinessStateT readiness_state);
	void set_oper_state(SaAmfOperationalStateT state);
	void set_oper_state(uint32_t state) {
		set_oper_state(static_cast<SaAmfOperationalStateT>(state));
	};
	void delete_all_susis(void);
	void set_all_susis_assigned_quiesced(void);
	void set_all_susis_assigned(void);
	void set_term_state(bool state);
	void remove_from_model();
	void set_su_switch(SaToggleState state, bool wrt_to_imm = true);
	AVD_AVND *get_node_ptr(void);
	bool is_in_service(void);
	bool is_instantiable(void);
	void reset_all_comps_assign_flag();
	AVD_COMP *find_unassigned_comp_that_provides_cstype(const std::string& cstype);
	void disable_comps(SaAisErrorT result);
	void complete_admin_op(SaAisErrorT result);
	void unlock(SaImmOiHandleT immoi_handle, SaInvocationT invocation);
	void lock_instantiation(SaImmOiHandleT immoi_handle,
		SaInvocationT invocation);
	void unlock_instantiation(SaImmOiHandleT immoi_handle,
		SaInvocationT invocation);
	void repaired(SaImmOiHandleT immoi_handle, SaInvocationT invocation);
	void restart(SaImmOiHandleT immoi_handle, SaInvocationT invocation);
	void shutdown(SaImmOiHandleT immoi_handle, SaInvocationT invocation);
	void lock(SaImmOiHandleT immoi_handle, SaInvocationT invocation,
		SaAmfAdminStateT adm_state);
	bool any_susi_fsm_in(uint32_t check_fsm);
	SaAisErrorT check_su_stability();
	uint32_t curr_num_standby_sis();
	uint32_t curr_num_active_sis();
	bool su_any_comp_undergoing_restart_admin_op();
	AVD_COMP *su_get_comp_undergoing_restart_admin_op();
	bool su_all_comps_restartable();
	bool is_any_non_restartable_comp_assigned();
	bool all_pi_comps_restartable();
	bool all_pi_comps_nonrestartable();
	SaAmfAdminOperationIdT get_admin_op_id() const;
	bool all_comps_in_presence_state(SaAmfPresenceStateT pres) const;
	void set_surestart(bool state);
	bool get_surestart() const;

 private:
	void initialize();
	bool surestart; /* used during surestart recovery and restart op on non restartable comp*/
	void send_attribute_update(AVSV_AMF_SU_ATTR_ID attrib_id);
	void set_saAmfSUPreInstantiable(bool value);

	// disallow copy and assign, TODO(hafe) add common macro for this
	AVD_SU(const AVD_SU&);
	void operator=(const AVD_SU&);
};

extern AmfDb<std::string, AVD_SU> *su_db;

/**
 * Get SUs from IMM and create internal objects
 * 
 * @return SaAisErrorT
 */
extern SaAisErrorT avd_su_config_get(const std::string& sg_name, AVD_SG *sg);

/**
 * Class constructor, must be called before any other function
 */
extern void avd_su_constructor(void);
extern void su_ccb_apply_delete_hdlr(struct CcbUtilOperationData *opdata);
void avd_su_read_headless_cached_rta(AVD_CL_CB *cb);
#endif  // AMF_AMFD_SU_H_
