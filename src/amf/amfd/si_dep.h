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

  DESCRIPTION:

  This module is the include file for handling Availability Directors
  service Instance structure and its relationship structures.

******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AMF_AMFD_SI_DEP_H_
#define AMF_AMFD_SI_DEP_H_

#include "amf/amfd/si.h"
#include "amf/amfd/sg.h"

/*
 * Following struct is used to make a list which is used while checking the
 * SI-SI dependencies are CYCLIC.
 */
typedef struct avd_si_dep_name_list {
	/* SI name */
	std::string si_name;

	struct avd_si_dep_name_list *next;
} AVD_SI_DEP_NAME_LIST;

class AVD_SI_DEP {
public:
        std::string name;
        AVD_SI *spons_si;
        std::string spons_name;
        AVD_SI *dep_si;
        std::string dep_name;
        SaTimeT saAmfToleranceTime;
        AVD_TMR si_dep_timer;
};

/* Spons-SI node of the spons-list in SI struct */
typedef struct avd_spons_si_tag {
	AVD_SI *si;
	AVD_SI_DEP *sidep_rec;
	struct avd_spons_si_tag *next;
} AVD_SPONS_SI_NODE;


#define AVD_SI_DEP_NULL ((AVD_SI_DEP *)0)

extern AmfDb<std::pair<std::string, std::string>, AVD_SI_DEP> *sidep_db;
void sidep_spons_list_del(AVD_CL_CB *cb, AVD_SI_DEP *si_dep_rec);
AVD_SI_DEP *avd_sidep_find(AVD_SI *spons_si, AVD_SI *dep_si);
void avd_sidep_tol_tmr_evh(AVD_CL_CB *cb, struct AVD_EVT *evt);
void avd_sidep_assign_evh(AVD_CL_CB *cb, struct AVD_EVT *evt);
void avd_sidep_unassign_evh(AVD_CL_CB *cb, struct AVD_EVT *evt);
void sidep_si_screen_si_dependencies(AVD_SI *si);
void avd_sidep_update_si_dep_state_for_all_sis(AVD_SG *sg);
void sidep_stop_tol_timer(AVD_CL_CB *cb, AVD_SI *si);
extern SaAisErrorT avd_sidep_config_get(void);
extern void avd_sidep_constructor(void);
extern void avd_sidep_reset_dependents_depstate_in_sufault(AVD_SI *si);
extern void avd_sidep_si_dep_state_set(AVD_SI *si, AVD_SI_DEP_STATE state);
extern bool avd_sidep_is_su_failover_possible(AVD_SU *su);
extern bool avd_sidep_is_si_failover_possible(AVD_SI *si, AVD_SU *su);
extern void avd_sidep_update_depstate_su_rolefailover(AVD_SU *su);
extern void avd_sidep_update_depstate_si_failover(AVD_SI *si, AVD_SU *su);
extern bool avd_sidep_si_dependency_exists_within_su(const AVD_SU *su);
extern void avd_sidep_send_active_to_dependents(const AVD_SI *si);
extern bool avd_sidep_quiesced_done_for_all_dependents(const AVD_SI *si, const AVD_SU *su);
extern void sidep_take_action_on_dependents(AVD_SI *si);
extern bool avd_sidep_sponsors_assignment_states(AVD_SI *si);
extern void sidep_si_take_action(AVD_SI *si);
extern void sidep_update_si_self_dep_state(AVD_SI *si);
extern void sidep_update_dependents_states(AVD_SI *si);
extern void sidep_process_ready_to_unassign_depstate(AVD_SI *dep_si);
extern void avd_sidep_sg_take_action(AVD_SG *sg);
extern void get_dependent_si_list(const std::string &spons_si_name, std::list<AVD_SI*>& depsi_list);
extern void avd_sidep_activ_amfd_tol_timer_expiry(AVD_SI *spons_si, AVD_SI *dep_si);
#endif  // AMF_AMFD_SI_DEP_H_
