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
  service unit structure.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVD_SU_H
#define AVD_SU_H

#include <saAmf.h>
#include <ncspatricia.h>
#include <avd_def.h>
#include <avd_cb.h>
#include <avd_node.h>
#include <avd_sg.h>
#include <avsv_defs.h>
#include <avd_msg.h>
#include <avd_comp.h>

/* The semantics the SU is undergoing. */
typedef enum {
	AVD_SU_NO_STATE = 0,
	AVD_SU_OPER,
	AVD_SU_NODE_OPER
} AVD_SU_STATE;

/* Avialability directors Service Unit structure(AVD_SU):
 * This data structure lives in the AvD and reflects data points
 * associated with the Service Unit (SU) on the AvD.
 */
typedef struct avd_su_tag {

	NCS_PATRICIA_NODE tree_node;	/* key will be the SU name */

	SaNameT name;
	SaNameT saAmfSUType;
	uns32 saAmfSURank;
	SaNameT saAmfSUHostNodeOrNodeGroup;
	SaBoolT saAmfSUFailover;
	SaNameT saAmfSUMaintenanceCampaign;

	/* runtime attributes */
	SaBoolT saAmfSUPreInstantiable;
	SaAmfOperationalStateT saAmfSUOperState;
	SaAmfAdminStateT saAmfSUAdminState;
	SaAmfReadinessStateT saAmfSuReadinessState;
	SaAmfPresenceStateT saAmfSUPresenceState;
	SaNameT **saAmfSUAssignedSIs;
	SaNameT saAmfSUHostedByNode;
	SaUint32T saAmfSUNumCurrActiveSIs;
	SaUint32T saAmfSUNumCurrStandbySIs;
	SaUint32T saAmfSURestartCount;	// TODO use this!

	AVD_ADMIN_OPER_CBK pend_cbk;	/* Stores zero invocation value of imm adm cbk
					 * when no admin operation is going on.
					 */

	uns32 num_of_comp;	/* The number of components
				 * that will make up this SU
				 * Checkpointing - Sent as a one time update.
				 */

	uns32 curr_num_comp;	/* The current number of components
				 * configured.
				 * Checkpointing - Calculated at Standby.
				 */

	uns32 si_max_active;	/* The maximum number of active instance of
				 * Sis that can be assigned to this SU.
				 * Checkpointing - Sent as a one time update.
				 */

	uns32 si_max_standby;	/* The maximum number of standby instance of
				 * Sis that can be assigned to this SU.
				 * Checkpointing - Sent as a one time update.
				 */

	NCS_BOOL term_state;	/* admin state to terminate the
				 * service unit.
				 * Checkpointing - Updated independently.
				 */

	SaToggleState su_switch;	/* The field that indicates if
					 * the active SIs assigned to 
					 * this SU needs to be Toggled.
					 * Checkpointing - Updated independently.
					 */

	NCS_BOOL su_is_external;	/* indicates if this SU is external */

	AVD_SU_STATE su_act_state;	/* The current action flow of
					 * the SU w.r.t to the node.
					 * Checkpointing - Updated independently.
					 */

	struct avd_sg_tag *sg_of_su;	/* the service group of this SU */
	struct avd_avnd_tag *su_on_node;	/*  the node on which this SU resides */
	struct avd_su_si_rel_tag *list_of_susi;	/* the list of su si relationship elements */
	struct avd_comp_tag *list_of_comp;	/* the list of  components in this SU */
	struct avd_su_tag *sg_list_su_next;	/* the next SU in the SG */
	struct avd_su_tag *avnd_list_su_next;	/* the next SU in the AvND */
	struct avd_sg_tag *sg;
	struct avd_sutype *su_type;
	struct avd_su_tag *su_list_su_type_next;
} AVD_SU;

typedef struct {
	NCS_PATRICIA_NODE tree_node;	/* key is name */
	SaNameT name;
	SaUint32T saAmfSutMaxNumComponents;
	SaUint32T saAmfSutMinNumComponents;

	SaUint32T curr_num_components;

} AVD_SUTCOMP_TYPE;

#define m_AVD_SET_SU_TERM(cb,su,state) {\
su->term_state = state;\
m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVSV_CKPT_SU_TERM_STATE);\
}

#define m_AVD_SET_SU_SWITCH(cb,su,state) {\
su->su_switch = state;\
m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVSV_CKPT_SU_SWITCH);\
}

#define m_AVD_APP_SU_IS_INSVC(i_su,su_node_ptr) \
((su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_UNLOCKED) && \
(su_node_ptr->saAmfNodeOperState == SA_AMF_OPERATIONAL_ENABLED) && \
(i_su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_UNLOCKED) &&\
(i_su->saAmfSUAdminState == SA_AMF_ADMIN_UNLOCKED) &&\
(i_su->saAmfSUOperState == SA_AMF_OPERATIONAL_ENABLED)\
)

#define m_AVD_GET_SU_NODE_PTR(avd_cb,i_su,su_node_ptr)  \
 if(TRUE == i_su->su_is_external) su_node_ptr = avd_cb->ext_comp_info.local_avnd_node; \
 else su_node_ptr = i_su->su_on_node;

/**
 * Allocate SU memory and initialize attributes to defaults
 * @param dn
 * 
 * @return AVD_SU*
 */
extern AVD_SU *avd_su_new(const SaNameT *dn);

/**
 * Free SU memory
 * @param su
 */
extern void avd_su_delete(AVD_SU *su);

/**
 * Add SU to SU DB.
 * @param su
 */
extern void avd_su_db_add(AVD_SU *su);

/**
 * Remove SU from SU DB
 * @param su
 */
extern void avd_su_db_remove(AVD_SU *su);

/**
 * Get SU object from DB using given key
 * 
 * @param dn
 * 
 * @return struct AVD_SU*
 */
extern AVD_SU *avd_su_get(const SaNameT *dn);

/**
 * Get next SU object from DB using given key
 * 
 * @param dn
 * 
 * @return struct AVD_SU*
 */
extern AVD_SU *avd_su_getnext(const SaNameT *dn);

extern void avd_su_remove_comp(struct avd_comp_tag *comp);
extern void avd_su_add_comp(struct avd_comp_tag *comp);

EXTERN_C void avd_su_add_sg_list(AVD_CL_CB *cb, AVD_SU *su);
EXTERN_C void avd_su_del_sg_list(AVD_CL_CB *cb, AVD_SU *su);
EXTERN_C void avd_su_del_avnd_list(AVD_CL_CB *cb, AVD_SU *su);

/**
 * Get SUs from IMM and create internal objects
 * 
 * @return SaAisErrorT
 */
extern SaAisErrorT avd_su_config_get(const SaNameT *sg_name, struct avd_sg_tag *sg);

extern void avd_su_pres_state_set(AVD_SU *su, SaAmfPresenceStateT pres_state);
extern void avd_su_oper_state_set(AVD_SU *su, SaAmfOperationalStateT oper_state);
extern void avd_su_readiness_state_set(AVD_SU *su, SaAmfReadinessStateT readiness_state);
extern void avd_su_admin_state_set(AVD_SU *su, SaAmfAdminStateT admin_state);

/**
 * Class constructor, must be called before any other function
 */
extern void avd_su_constructor(void);

/**
 * Get SaAmfSutCompType object from DB using given key
 * 
 * @param dn
 * 
 * @return AVD_SUTCOMP_TYPE*
 */
extern AVD_SUTCOMP_TYPE *avd_sutcomptype_get(const SaNameT *dn);

/**
 * Get configuration for all SaAmfSutCompType objects from IMM and
 * create AVD internal objects.
 * @param cb
 * 
 * @return int
 */
extern SaAisErrorT avd_sutcomptype_config_get(SaNameT *sutype_name, struct avd_sutype *sut);

/**
 * Class constructor, must be called before any other function
 */
extern void avd_sutcomptype_constructor(void);

extern void avd_su_inc_curr_act_si(AVD_SU *su);
extern void avd_su_dec_curr_act_si(AVD_SU *su);
extern void avd_su_inc_curr_stdby_si(AVD_SU *su);
extern void avd_su_dec_curr_stdby_si(AVD_SU *su);

#endif
