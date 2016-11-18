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
#ifndef AMF_AMFD_SUSI_H_
#define AMF_AMFD_SUSI_H_

#include "amf/amfd/su.h"
#include "amf/amfd/si.h"
#include "amf/amfd/ntf.h"

typedef enum {
	AVD_SU_SI_STATE_BASE = AVSV_SUSI_ACT_BASE,
	AVD_SU_SI_STATE_ABSENT,
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
	std::string comp_name;
	std::string csi_name;
	bool absent;
} AVD_SU_SI_REL;

/* SusperSiRank table index structure */
typedef struct avd_sus_per_si_rank_index_tag {

	std::string si_name;
	uint32_t su_rank;	/* The rank of the SU */

} AVD_SUS_PER_SI_RANK_INDX;

/* Availability directors SUs organised per RANK in a SI. 
 * This Data structure lives in the AVD.
 */
typedef struct avd_sus_per_si_rank_tag {
	AVD_SUS_PER_SI_RANK_INDX indx;	/* Table index */
	std::string su_name;	/* name of the SU as std::string */
	AVD_SI *sus_per_si_rank_on_si;
	struct avd_sus_per_si_rank_tag *sus_per_si_rank_list_si_next;

} AVD_SUS_PER_SI_RANK;

extern AmfDb<std::pair<std::string, uint32_t>, AVD_SUS_PER_SI_RANK> *sirankedsu_db;

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
   susi->csi_add_rem = static_cast<SaBoolT>(false);			\
   m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, susi, AVSV_CKPT_AVD_SI_ASS);\
   avd_susi_delete(cb,susi,false);\
}

AVD_SU_SI_REL *avd_susi_create(AVD_CL_CB *cb, AVD_SI *si, AVD_SU *su, SaAmfHAStateT state,
								bool ckpt, AVSV_SUSI_ACT default_act = AVSV_SUSI_ACT_BASE,
								AVD_SU_SI_STATE default_fsm = AVD_SU_SI_STATE_ABSENT);
AVD_SU_SI_REL *avd_susi_find(AVD_CL_CB *cb, const std::string& su_name, const std::string& si_name);
void avd_susi_update_fsm(AVD_SU_SI_REL *susi, AVD_SU_SI_STATE new_fsm_state);
bool avd_susi_validate_headless_cached_rta(AVD_SU_SI_REL *present_susi,
		SaAmfHAStateT ha_fr_imm, AVD_SU_SI_STATE fsm_fr_imm);
void avd_susi_read_headless_cached_rta(AVD_CL_CB *cb);
extern void avd_susi_update(AVD_SU_SI_REL *susi, SaAmfHAStateT ha_state);

AVD_SU_SI_REL *avd_su_susi_find(AVD_CL_CB *cb, AVD_SU *su, const std::string& si_name);
uint32_t avd_susi_delete(AVD_CL_CB *cb, AVD_SU_SI_REL *susi, bool ckpt);
extern SaAisErrorT avd_sirankedsu_config_get(const std::string& si_name, AVD_SI *si);
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
extern AVD_SU_SI_REL *avd_siass_next_susi_to_quiesce(const AVD_SU_SI_REL *susi);
extern bool avd_susi_quiesced_canbe_given(const AVD_SU_SI_REL *susi);
SaAisErrorT avd_susi_recreate(AVSV_N2D_ND_SISU_STATE_MSG_INFO*);

#endif  // AMF_AMFD_SUSI_H_
