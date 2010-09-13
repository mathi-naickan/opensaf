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
  Service group structure.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVD_SG_H
#define AVD_SG_H

#include <saAmf.h>
#include <ncspatricia.h>
#include <avd_su.h>
#include <avd_si.h>
#include <avsv_defs.h>

/* The valid SG FSM states. */
typedef enum {
	AVD_SG_FSM_STABLE = 0,
	AVD_SG_FSM_SG_REALIGN,
	AVD_SG_FSM_SU_OPER,
	AVD_SG_FSM_SI_OPER,
	AVD_SG_FSM_SG_ADMIN
} AVD_SG_FSM_STATE;

/* The structure used for containing the list of SUs
 * undergoing operations on them.
 */
typedef struct avd_sg_oper_tag {
	struct avd_su_tag *su;	/* SU undergoing operation */
	struct avd_sg_oper_tag *next;	/* The next SU undergoing operation. */
} AVD_SG_OPER;

/* Availability directors Service Group structure(AVD_SG):
 * This data structure lives in the AvD and reflects data points
 * associated with the Service group (SG).
 */
typedef struct avd_sg_tag {
	NCS_PATRICIA_NODE tree_node;	/* key will be the SG name */

	SaNameT name;		/* the service group DN used as the index.
				 * Checkpointing - Sent as a one time update.
				 */

   /******************** B.04 model *************************************************/
	SaNameT saAmfSGType;	/* Network order. */
	SaNameT saAmfSGSuHostNodeGroup;	/* Network order. */
	SaBoolT saAmfSGAutoRepair;
	SaBoolT saAmfSGAutoAdjust;

	SaUint32T saAmfSGNumPrefActiveSUs;	/* the N value in the redundancy model, where
						 * applicable.
						 * Checkpointing - Sent as a one time update.
						 */

	SaUint32T saAmfSGNumPrefStandbySUs;	/* the M value in the redundancy model, where
						 * applicable.
						 * Checkpointing - Sent as a one time update.
						 */

	SaUint32T saAmfSGNumPrefInserviceSUs;	/* The preferred number of in service
						 * SUs.
						 * Checkpointing - Sent as a one time update.
						 */

	SaUint32T saAmfSGNumPrefAssignedSUs;	/* The number of active SU assignments
						 * an SI can have.
						 * Checkpointing - Sent as a one time update.
						 */

	SaUint32T saAmfSGMaxActiveSIsperSU;	/* The maximum number of active 
						 * instance of Sis that can be 
						 * assigned to an SU.
						 * Checkpointing - Sent as a one time update.
						 */

	SaUint32T saAmfSGMaxStandbySIsperSU;	/* The maximum number of standby 
						 * instance of Sis that can be
						 * assigned to an SU.
						 * Checkpointing - Sent as a one time update.
						 */
	SaTimeT saAmfSGAutoAdjustProb;
	SaTimeT saAmfSGCompRestartProb;	/* component restart probation period
					 * Checkpointing - Sent as a one time update.
					 */
	SaUint32T saAmfSGCompRestartMax;	/* max component restart count 
						 * Checkpointing - Sent as a one time update.
						 */
	SaTimeT saAmfSGSuRestartProb;	/* SU restart probation period 
					 * Checkpointing - Sent as a one time update.
					 */

	SaUint32T saAmfSGSuRestartMax;	/* max SU restart count
					 * Checkpointing - Sent as a one time update.
					 */

	SaAmfAdminStateT saAmfSGAdminState;	/* admin state of the group.
						 * Checkpointing - Updated independently.
						 */

	SaUint32T saAmfSGNumCurrAssignedSUs;	/* Num of Sus that have been assigned a SI 
						 * Checkpointing - Updated independently.
						 */

	SaUint32T saAmfSGNumCurrInstantiatedSpareSUs;	/* Num of Sus that are in service but 
							 * not yet assigned a SI.
							 * Checkpointing - Updated independently.
							 */

	SaUint32T saAmfSGNumCurrNonInstantiatedSpareSUs;	/* Num of Sus that have been 
								 * configured but not yet instantiated.
								 * Checkpointing - Updated independently.
								 */

   /******************** B.04 model *************************************************/

	SaAdjustState adjust_state;	/* Field to re adjust the SG.
					 * Checkpointing - Updated independently.
					 */

	SaBoolT sg_ncs_spec;	/* This is set to true if the SG
				 * is a NCS specific SG.
				 * Checkpointing - Sent as a one time update.
				 */

	AVD_SG_FSM_STATE sg_fsm_state;	/* The different flows of the SU SI
					 * transitions for the SUs and SIs
					 * in the SG is orchestrated based on
					 * this FSM.
					 * Checkpointing - Updated independently.
					 */

	struct avd_si_tag *admin_si;	/* Applicable when sg_fsm_state has
					 * AVD_SG_FSM_SI_OPER.It will contain
					 * the SI undergoing admin
					 * operation.
					 */

	AVD_SG_OPER su_oper_list;	/* The list of SUs that have operations
					 * happening on them used in parallel
					 * with sg_fsm_state.
					 * Checkpointing - Sent as a one time update.
					 */
	SaAmfRedundancyModelT sg_redundancy_model;	/* the redundancy model in the service group 
							 * see sec 4.7 for values
							 * Checkpointing - Sent as a one time update.
							 */

	struct avd_su_tag *list_of_su;	/* the list of service units in this
					 * group in the descending order of
					 * the rank.
					 */
	struct avd_si_tag *list_of_si;	/* the list of service instances in 
					 * this group in the descending order 
					 * of the rank.
					 */

	struct avd_sg_tag *sg_list_sg_type_next;
	struct avd_amf_sg_type_tag *sg_type;
	struct avd_sg_tag *sg_list_app_next;
	struct avd_app_tag *app;
} AVD_SG;

typedef struct avd_amf_sg_type_tag {
	NCS_PATRICIA_NODE tree_node;	/* key will be sg type name */
	SaNameT name;

   /******************** B.04 model *************************************************/
	SaNameT *saAmfSGtValidSuTypes;	/* array of DNs, size in number_su_type */
	SaAmfRedundancyModelT saAmfSgtRedundancyModel;
	SaNameT *saAmfStgValidSuTypes;
	SaBoolT saAmfSgtDefAutoRepair;
	SaBoolT saAmfSgtDefAutoAdjust;
	SaTimeT saAmfSgtDefAutoAdjustProb;
	SaTimeT saAmfSgtDefCompRestartProb;
	SaUint32T saAmfSgtDefCompRestartMax;
	SaTimeT saAmfSgtDefSuRestartProb;
	SaUint32T saAmfSgtDefSuRestartMax;
   /******************** B.04 model *************************************************/

	uns32 number_su_type;	/* size of array saAmfSGtValidSuTypes */
	struct avd_sg_tag *list_of_sg;

} AVD_AMF_SG_TYPE;

#define m_AVD_SET_SG_ADJUST(cb,sg,state) {\
	TRACE("adjust_state %u => %u", sg->adjust_state, state); \
	sg->adjust_state = state;\
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, sg, AVSV_CKPT_SG_ADJUST_STATE);\
}

#define m_AVD_SET_SG_FSM(cb,sg,state) {\
	if (sg->sg_fsm_state != state) { \
		TRACE("sg_fsm_state %u => %u", sg->sg_fsm_state, state); \
		sg->sg_fsm_state = state;\
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, sg, AVSV_CKPT_SG_FSM_STATE);\
	}\
	if (state == AVD_SG_FSM_STABLE) \
		avd_sg_screen_si_si_dependencies(cb, sg); \
}

#define m_AVD_SET_SG_ADMIN_SI(cb,si) {\
	TRACE("admin_si set to %s", si->name.value); \
	si->sg_of_si->admin_si = si;\
	m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(cb, (si->sg_of_si), AVSV_CKPT_AVD_SG_ADMIN_SI);\
}

#define m_AVD_CLEAR_SG_ADMIN_SI(cb,sg) {\
	if (sg->admin_si != AVD_SI_NULL) {\
		TRACE("admin_si cleared"); \
		m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, sg, AVSV_CKPT_AVD_SG_ADMIN_SI);\
		sg->admin_si = AVD_SI_NULL;\
	}\
}

/*****************************************************************************
 * Macro: m_AVD_CHK_OPLIST
 *
 * Purpose:  This macro will search the SU operation list in the SG to
 * identify if the given SU is present in the list
 *
 * Input: su - the pointer to the SU to be checked in the list.
 *        flag - the NCS_BOOL field indicating if found or not.
 *        
 *
 * Return: none.
 *
 * NOTES: 
 *
 * 
 **************************************************************************/
#define m_AVD_CHK_OPLIST(i_su,flag) \
{\
   AVD_SG_OPER *l_suopr;\
   flag = FALSE;\
   if (i_su->sg_of_su->su_oper_list.su == i_su)\
   {\
      flag = TRUE;\
   }else if (i_su->sg_of_su->su_oper_list.next != NULL)\
   {\
      l_suopr = i_su->sg_of_su->su_oper_list.next;\
      while (l_suopr != NULL)\
      {\
         if (l_suopr->su == i_su)\
         {\
            flag = TRUE;\
            l_suopr = NULL;\
            continue; \
         }\
         l_suopr = l_suopr->next;\
      }\
   }\
}

extern AVD_SG *avd_sg_new(const SaNameT *dn);
extern void avd_sg_delete(AVD_SG *sg);
extern void avd_sg_db_add(AVD_SG *sg);
extern void avd_sg_db_remove(AVD_SG *sg);
extern AVD_SG *avd_sg_get(const SaNameT *sg_name);
extern AVD_SG *avd_sg_getnext(const SaNameT *sg_name);
extern void avd_sg_add_si(AVD_SG *sg, struct avd_si_tag *si);
extern void avd_sg_remove_si(AVD_SG *sg, struct avd_si_tag *si);
extern SaAisErrorT avd_sg_config_get(const SaNameT *app_dn, struct avd_app_tag *app);
extern void avd_sg_add_su(struct avd_su_tag* su);
extern void avd_sg_remove_su(struct avd_su_tag *su);
extern void avd_sg_constructor(void);

extern SaAisErrorT avd_sgtype_config_get(void);
extern AVD_AMF_SG_TYPE *avd_sgtype_get(const SaNameT *dn);
extern void avd_sgtype_add_sg(AVD_SG *sg);
extern void avd_sgtype_remove_sg(AVD_SG *sg);
extern void avd_sgtype_constructor(void);
extern void avd_sg_admin_state_set(AVD_SG* sg, SaAmfAdminStateT state);

#endif
