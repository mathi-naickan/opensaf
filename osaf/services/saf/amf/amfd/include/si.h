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

  This module is the include file for handling Availability Directors 
  service Instance structure and its relationship structures.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVD_SI_H
#define AVD_SI_H

#include <saAmf.h>
#include <saImm.h>
#include <app.h>
#include <sg.h>
#include <su.h>
#include <sg.h>
#include <svctype.h>
#include <svctypecstype.h>
#include <sirankedsu.h>
#include <amf_defs.h>
#include <ckpt_msg.h>
#include <vector>
#include <list>

class AVD_APP;

/* Enum values defines different SI-SI dependency FSM states. */
typedef enum {
	AVD_SI_NO_DEPENDENCY = 1,
	AVD_SI_SPONSOR_UNASSIGNED,
	AVD_SI_ASSIGNED,
	AVD_SI_TOL_TIMER_RUNNING,
	AVD_SI_READY_TO_UNASSIGN,
	AVD_SI_UNASSIGNING_DUE_TO_DEP,
	AVD_SI_FAILOVER_UNDER_PROGRESS,
	AVD_SI_READY_TO_ASSIGN,
	AVD_SI_DEP_MAX_STATE
} AVD_SI_DEP_STATE;

/* Availability directors Service Instance structure(AVD_SI):
 * This data structure lives in the AvD and reflects data points 
 * associated with the Service Instance (SI) on the AvD.
 */
class AVD_SI {
public:
	AVD_SI();
	SaNameT name;

	/******************** B.04 model *************************************************/
	SaNameT saAmfSvcType;
	SaNameT saAmfSIProtectedbySG;
	uint32_t saAmfSIRank;
	std::vector<std::string> saAmfSIActiveWeight;
	std::vector<std::string> saAmfSIStandbyWeight;
	uint32_t saAmfSIPrefActiveAssignments;  /* only applicable for the N-way active redundancy model */
	uint32_t saAmfSIPrefStandbyAssignments; /* only applicable for the N-way active redundancy model */
	SaAmfAdminStateT saAmfSIAdminState;
	SaAmfAssignmentStateT saAmfSIAssignmentState;
	uint32_t saAmfSINumCurrActiveAssignments;
	uint32_t saAmfSINumCurrStandbyAssignments;
	/******************** B.04 model *************************************************/

	SaToggleState si_switch;	/* The field that indicates if
					 * the SI needs to be Toggled.
					 * Checkpointing - Updated independently.
					 */

	AVD_SG *sg_of_si;	/* the service group of this SI */
	AVD_CSI *list_of_csi;	/* The list of CSIs in the SI */
	struct avd_su_si_rel_tag *list_of_sisu;	/* the list of su si relationship elements */
	AVD_SI_DEP_STATE si_dep_state;	/* SI-SI dep state of this SI */
	struct avd_spons_si_tag *spons_si_list;
	uint32_t num_dependents; /* number of dependent SIs */
	uint32_t tol_timer_count;
	AVD_SVC_TYPE *svc_type;
	AVD_APP *app;
	AVD_SI *si_list_app_next;
	struct avd_sus_per_si_rank_tag *list_of_sus_per_si_rank;
	avd_sirankedsu_t *rankedsu_list_head;
	SaInvocationT invocation;
	
	bool alarm_sent; /* SI unassigned alarm has been sent */

	void inc_curr_act_ass();
	void dec_curr_act_ass();
	void inc_curr_stdby_ass();
	void dec_curr_stdby_ass();
	void inc_curr_stdby_dec_act_ass();
	void inc_curr_act_dec_std_ass();
	
	void adjust_si_assignments(uint32_t mod_pref_assignments);
	void update_ass_state();

	void set_admin_state(SaAmfAdminStateT state);

	void add_rankedsu(const SaNameT *suname, uint32_t saAmfRank);
	void remove_rankedsu(const SaNameT *suname);
	
	void set_si_switch(AVD_CL_CB *cb, const SaToggleState state);
	
	uint32_t pref_active_assignments() const;
	uint32_t curr_active_assignments() const;
	uint32_t pref_standby_assignments() const;
	uint32_t curr_standby_assignments() const;
	
	void add_csi(AVD_CSI* csi);
	void remove_csi(AVD_CSI *csi);
	
	void delete_assignments(AVD_CL_CB *cb);
	void delete_csis();
	void si_add_to_model();
	
	void arrange_dep_csi(AVD_CSI* csi);
	void add_csi_db(AVD_CSI* csi);
	bool is_sirank_valid(uint32_t newSiRank) const;
	void update_sirank(uint32_t newSiRank);
	bool si_dep_states_check();
	bool is_active() const;
private:
	AVD_SI(const AVD_SI&);
	AVD_SI& operator=(const AVD_SI&);

};

extern AmfDb<std::string, AVD_SI> *si_db;
#define AVD_SI_NULL ((AVD_SI *)0)

extern AVD_SI *avd_si_new(const SaNameT *dn);
extern void avd_si_delete(AVD_SI *si);
extern void avd_si_db_add(AVD_SI *si);
extern AVD_SI *avd_si_get(const SaNameT *si_name);
extern SaAisErrorT avd_si_config_get(AVD_APP *app);
extern void avd_si_constructor(void);

#endif
