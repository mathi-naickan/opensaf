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
#ifndef AVD_SI_DEP_H
#define AVD_SI_DEP_H

#include <avd_si.h>
#include <avd_sg.h>

typedef enum {
	AVD_SI_DEP_SPONSOR_ASSIGNED = 1,
	AVD_SI_DEP_SPONSOR_UNASSIGNED
} AVD_SI_DEP_SPONSOR_SI_STATE;

/*
 * Following struct is used to make a list which is used while checking the
 * SI-SI dependencies are CYCLIC.
 */
typedef struct avd_si_dep_name_list {
	/* SI name */
	SaNameT si_name;

	struct avd_si_dep_name_list *next;
} AVD_SI_DEP_NAME_LIST;

/*
 * Index data to retrieve a specific SI-SI dep. data record from its
 * database (si_dep: dep_anchor, spons_anchor) defined AvD CB .
 */
typedef struct avd_si_si_dep_indx_tag {
	/* primary-key: sponsor SI name */
	SaNameT si_name_prim;

	/* secondary-key: dependant SI name */
	SaNameT si_name_sec;
} AVD_SI_SI_DEP_INDX;

/*
 * Data structure that holds SI-SI dependency data, same record is updated
 * in two SI_SI dep anch tree's. One of the tree hold sponsor SI name as the
 * primary indx & dependent SI name as secondary indx (as per SI_SI dep cfg).
 * The other one has vis-versa.
 */
typedef struct avd_si_si_dep {
	NCS_PATRICIA_NODE tree_node_imm;
	NCS_PATRICIA_NODE tree_node;

	/* Index info to retrieve the record */
	AVD_SI_SI_DEP_INDX indx_imm;
	AVD_SI_SI_DEP_INDX indx;

	uint32_t si_dep_hdl;
	SaTimeT saAmfToleranceTime;
	AVD_TMR si_dep_timer;	/* SI-SI dep. tol timer */
	bool unassign_event;
} AVD_SI_SI_DEP;

/* Spons-SI node of the spons-list in SI struct */
typedef struct avd_spons_si_tag {
	struct avd_si_tag *si;
	AVD_SI_SI_DEP *sidep_rec;
	struct avd_spons_si_tag *next;
} AVD_SPONS_SI_NODE;

#define AVD_SI_SI_DEP_NULL ((AVD_SI_SI_DEP *)0)

void avd_si_dep_delete(AVD_CL_CB *cb, struct avd_si_tag *si);
void avd_si_dep_spons_list_del(AVD_CL_CB *cb, AVD_SI_SI_DEP *si_dep_rec);
AVD_SI_SI_DEP *avd_si_si_dep_struc_crt(AVD_CL_CB *cb, AVD_SI_SI_DEP_INDX *indx);
AVD_SI_SI_DEP *avd_si_si_dep_find(AVD_CL_CB *cb, AVD_SI_SI_DEP_INDX *indx, bool isImmIdx);
AVD_SI_SI_DEP *avd_si_si_dep_find_next(AVD_CL_CB *cb, AVD_SI_SI_DEP_INDX *indx, bool isImmIdx);
uint32_t avd_si_si_dep_del_row(AVD_CL_CB *cb, AVD_SI_SI_DEP *rec);
void avd_tmr_si_dep_tol_evh(AVD_CL_CB *cb, struct avd_evt_tag *evt);
void avd_si_dep_spons_unassign(AVD_CL_CB *cb, struct avd_si_tag *si, struct avd_si_tag *si_dep);
void avd_process_si_dep_state_evh(AVD_CL_CB *cb, struct avd_evt_tag *evt);
void avd_screen_sponsor_si_state(AVD_CL_CB *cb, struct avd_si_tag *si, bool start_assignment);
void avd_sg_screen_si_si_dependencies(AVD_CL_CB *cb, struct avd_sg_tag *sg);
void avd_si_dep_stop_tol_timer(AVD_CL_CB *cb, struct avd_si_tag *si);
extern SaAisErrorT avd_sidep_config_get(void);
extern void avd_sidep_constructor(void);
extern void avd_sidep_reset_dependents_depstate_in_sufault(struct avd_si_tag *si);
extern void si_dep_state_set(struct avd_si_tag *si, AVD_SI_DEP_STATE state);
extern bool avd_sidep_is_su_failover_possible(struct avd_su_tag *su);
extern bool avd_sidep_is_si_failover_possible(struct avd_si_tag *si, struct avd_su_tag *su);
extern void avd_update_depstate_su_rolefailover(struct avd_su_tag *su);
extern void avd_update_depstate_si_failover(struct avd_si_tag *si, struct avd_su_tag *su);
extern bool avd_si_dependency_exists_within_su(const struct avd_su_tag *su);
extern void send_active_to_dependents(const struct avd_si_tag *si);
extern bool quiesced_done_for_all_dependents(const struct avd_si_tag *si, const struct avd_su_tag *su);
extern void avd_si_dep_spons_state_modif(AVD_CL_CB *cb, struct avd_si_tag *si, struct avd_si_tag *si_dep,
                                         AVD_SI_DEP_SPONSOR_SI_STATE spons_state);
#endif
