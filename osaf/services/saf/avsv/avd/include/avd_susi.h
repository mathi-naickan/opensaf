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

  This module is the include file for Availability Directors su si relationship
  structures.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVD_SUSI_H
#define AVD_SUSI_H

#include <avd_su.h>
#include <avd_si.h>
#include <avd_ntf.h>

typedef enum {
	AVD_SU_SI_STATE_ABSENT = 1,
	AVD_SU_SI_STATE_ASGN,
	AVD_SU_SI_STATE_ASGND,
	AVD_SU_SI_STATE_UNASGN,
	AVD_SU_SI_STATE_MODIFY
} AVD_SU_SI_STATE;

/* Availability directors SU SI relationship structure(AVD_SU_SI_REL):
 * This data structure lives in the AvD and reflects relationship between
 * the SU and SI on the AvD.
 */

typedef struct avd_su_si_rel_tag {

	AVD_SI *si;		/* SI to which this relationship with SU
				 * exists */
	AVD_SU *su;		/* SU to which this relationship with SI
				 * exists */
	SaAmfHAStateT state;	/* The current HA state for the SI w.r.t SU */
	struct avd_comp_csi_rel_tag *list_of_csicomp;	/* The List of component CSI assignments that
							 * belong to this SU SI relationship */
	AVD_SU_SI_STATE fsm;	/* The SU SI FSM state */
	bool is_per_si;	/* indicates if the susi rel is added as per 
				   the su rank in the per si list */
	struct avd_su_si_rel_tag *su_next;	/* The next element in the list w.r.t to SU */
	struct avd_su_si_rel_tag *si_next;	/* The next element in the list w.r.t to SI */
	/* To check which comp-csi is being added/removed. */
	SaBoolT csi_add_rem;
	SaNameT comp_name;
	SaNameT csi_name;
} AVD_SU_SI_REL;

/* SusperSiRank table index structure */
typedef struct avd_sus_per_si_rank_index_tag {

	SaNameT si_name;
	uint32_t su_rank;	/* The rank of the SU */

} AVD_SUS_PER_SI_RANK_INDX;

/* Availability directors SUs organised per RANK in a SI. 
 * This Data structure lives in the AVD and is maintained as a patricia tree 
 * from the AVD Control Block.
 */
typedef struct avd_sus_per_si_rank_tag {

	NCS_PATRICIA_NODE tree_node;	/* key will be the SI name and Rank */
	AVD_SUS_PER_SI_RANK_INDX indx;	/* Table index */
	SaNameT su_name;	/* name of the SU as SaNameT */
	struct avd_si_tag *sus_per_si_rank_on_si;
	struct avd_sus_per_si_rank_tag *sus_per_si_rank_list_si_next;

} AVD_SUS_PER_SI_RANK;

#define   AVD_SU_PER_SI_RANK_NULL   ((AVD_SUS_PER_SI_RANK *)0)

#define AVD_SU_SI_REL_NULL ((AVD_SU_SI_REL *)0)

#define m_AVD_SU_SI_CHK_QSD_ASGN(l_su,flag) \
{\
   AVD_SU_SI_REL *i_susi;\
   i_susi = (l_su)->list_of_susi;\
   flag = true;\
   while ((i_susi != AVD_SU_SI_REL_NULL) && (flag == true))\
   {\
      if ((i_susi->fsm != AVD_SU_SI_STATE_UNASGN) &&\
         (i_susi->state != SA_AMF_HA_QUIESCED))\
      {\
         flag = false;\
         continue;\
      }\
      i_susi = i_susi->su_next;\
   }\
}

#define m_AVD_SU_SI_CHK_ASGND(l_su,flag) \
{\
   AVD_SU_SI_REL *i_susi;\
   i_susi = (l_su)->list_of_susi;\
   flag = true;\
   while ((i_susi != AVD_SU_SI_REL_NULL) && (flag == true))\
   {\
      if (i_susi->fsm != AVD_SU_SI_STATE_ASGND)\
      {\
         flag = false;\
         continue;\
      }\
      i_susi = i_susi->su_next;\
   }\
}

#define m_AVD_SU_SI_TRG_DEL(cb,susi) \
{\
   susi->csi_add_rem = false;\
   m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, susi, AVSV_CKPT_AVD_SI_ASS);\
   avd_susi_delete(cb,susi,false);\
}

AVD_SU_SI_REL *avd_susi_create(AVD_CL_CB *cb, AVD_SI *si, AVD_SU *su, SaAmfHAStateT state, bool ckpt);
AVD_SU_SI_REL *avd_susi_find(AVD_CL_CB *cb, const SaNameT *su_name, const SaNameT *si_name);
extern void avd_susi_update(AVD_SU_SI_REL *susi, SaAmfHAStateT ha_state);

AVD_SU_SI_REL *avd_su_susi_find(AVD_CL_CB *cb, AVD_SU *su, const SaNameT *si_name);
AVD_SU_SI_REL *avd_susi_find_next(AVD_CL_CB *cb, SaNameT su_name, SaNameT si_name);
uint32_t avd_susi_delete(AVD_CL_CB *cb, AVD_SU_SI_REL *susi, bool ckpt);
extern AVD_SUS_PER_SI_RANK *avd_sirankedsu_getnext_valid(AVD_CL_CB *cb,
	AVD_SUS_PER_SI_RANK_INDX indx, AVD_SU **o_su);
extern AVD_SUS_PER_SI_RANK *avd_sirankedsu_getnext(AVD_CL_CB *cb, AVD_SUS_PER_SI_RANK_INDX indx);
extern SaAisErrorT avd_sirankedsu_config_get(SaNameT *si_name, AVD_SI *si);
extern void avd_sirankedsu_constructor(void);
extern void avd_susi_ha_state_set(AVD_SU_SI_REL *susi, SaAmfHAStateT ha_state);
uint32_t avd_gen_su_ha_state_changed_ntf(AVD_CL_CB *avd_cb, struct avd_su_si_rel_tag *susi);
extern AVD_SU_SI_REL * avd_find_preferred_standby_susi(AVD_SI *si);
extern uint32_t avd_susi_mod_send(AVD_SU_SI_REL *susi, SaAmfHAStateT ha_state);
extern void avd_susi_update_assignment_counters(AVD_SU_SI_REL *susi, AVSV_SUSI_ACT action,
	SaAmfHAStateT current_ha_state, SaAmfHAStateT new_ha_state);
extern uint32_t avd_susi_del_send(AVD_SU_SI_REL *susi);
extern uint32_t avd_susi_role_failover(AVD_SU_SI_REL *sisu, AVD_SU *su);
extern uint32_t avd_susi_role_failover2(AVD_SU_SI_REL *sisu, AVD_SU *su);
extern bool si_assignment_state_check(AVD_SI *si);
extern SaAmfHAStateT avd_su_state_determine(AVD_SU *su);
#endif
