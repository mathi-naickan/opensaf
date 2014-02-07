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
  component structure.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVD_COMP_H
#define AVD_COMP_H

#include <saAmf.h>
#include <saImm.h>
#include <ncspatricia.h>
#include <avsv_d2nmsg.h>
#include <avd_su.h>

/* AMF Class SaAmfCompGlobalAttributes */
typedef struct {
	SaUint32T saAmfNumMaxInstantiateWithoutDelay;
	SaUint32T saAmfNumMaxInstantiateWithDelay;
	SaUint32T saAmfNumMaxAmStartAttempts;
	SaUint32T saAmfNumMaxAmStopAttempts;
	SaTimeT saAmfDelayBetweenInstantiateAttempts;
} AVD_COMP_GLOBALATTR;

/* AMF Class SaAmfCompType */
typedef struct avd_comp_tag {

	NCS_PATRICIA_NODE tree_node;	/* key will be the component name */
	SaNameT saAmfCompType;

	/* Detailed as in data structure definition */
	AVSV_COMP_INFO comp_info;	/* component name field with 
					 * the length field in the 
					 * network order is used as the
					 * index. */
	SaTimeT inst_retry_delay;	/* Delay interval after which
					 * the component is reinstantiated.
					 * Checkpointing - Sent as a one time update.
					 */

	bool nodefail_cleanfail;	/* If flag set to true node will
					 * be considered failed when the
					 * cleanup script fails.
					 * Checkpointing - Sent as a one time update.
					 */

	uint32_t max_num_inst_delay;	/* the maximum number of times
					 * AMF tries to instantiate
					 * the component with delay.
					 * Checkpointing - Sent as a one time update.
					 */

	SaUint32T max_num_csi_actv;	/* number of CSI relationships that can be
					 * assigned active to this component 
					 * Checkpointing - Sent as a one time update.
					 */

	SaUint32T max_num_csi_stdby;	/* number of CSI relationships that can be
					 * assigned standby to this component 
					 * Checkpointing - Sent as a one time update.
					 */

	SaUint32T curr_num_csi_actv;	/* the number of CSI relationships that have
					 * been assigned active to this component
					 * Checkpointing - Sent update independently.
					 */

	SaUint32T curr_num_csi_stdby;	/* the number of CSI relationships that have
					 * been assigned standby to this component
					 * Checkpointing - Sent update independently.
					 */
	SaNameT comp_proxy_csi;
	SaNameT comp_container_csi;

	/* runtime attributes */
	SaAmfOperationalStateT saAmfCompOperState;	
	SaAmfReadinessStateT   saAmfCompReadinessState;
	SaAmfPresenceStateT    saAmfCompPresenceState;
	SaUint32T              saAmfCompRestartCount;
	SaNameT                saAmfCompCurrProxyName;
	SaNameT              **saAmfCompCurrProxiedNames;

	bool assign_flag;	/* Flag used while assigning. to mark this
				 * comp has been assigned a CSI from
				 * current SI being assigned
				 */
	struct avd_amf_comp_type_tag *comp_type;
	struct avd_comp_tag *comp_type_list_comp_next;
	struct avd_su_tag *su;		/* SU to which this component belongs */
	struct avd_comp_tag *su_comp_next;	/* the next component in list of  components
						 * in this SU */
	AVD_ADMIN_OPER_CBK admin_pend_cbk;  /* holds callback invocation for admin operation */
} AVD_COMP;

/* AMF Class SaAmfCompType */
typedef struct avd_amf_comp_type_tag {

	NCS_PATRICIA_NODE tree_node;	/* name is key */
	SaNameT name;
	SaUint32T saAmfCtCompCategory;
	SaNameT saAmfCtSwBundle;
	char saAmfCtDefCmdEnv[AVSV_MISC_STR_MAX_SIZE];
	SaTimeT saAmfCtDefClcCliTimeout;
	SaTimeT saAmfCtDefCallbackTimeout;
	char saAmfCtRelPathInstantiateCmd[AVSV_MISC_STR_MAX_SIZE];
	char saAmfCtDefInstantiateCmdArgv[AVSV_MISC_STR_MAX_SIZE];
	SaUint32T saAmfCtDefInstantiationLevel;
	char saAmfCtRelPathTerminateCmd[AVSV_MISC_STR_MAX_SIZE];
	char saAmfCtDefTerminateCmdArgv[AVSV_MISC_STR_MAX_SIZE];
	char saAmfCtRelPathCleanupCmd[AVSV_MISC_STR_MAX_SIZE];
	char saAmfCtDefCleanupCmdArgv[AVSV_MISC_STR_MAX_SIZE];
	char saAmfCtRelPathAmStartCmd[AVSV_MISC_STR_MAX_SIZE];
	char saAmfCtDefAmStartCmdArgv[AVSV_MISC_STR_MAX_SIZE];
	char saAmfCtRelPathAmStopCmd[AVSV_MISC_STR_MAX_SIZE];
	char saAmfCtDefAmStopCmdArgv[AVSV_MISC_STR_MAX_SIZE];
	SaTimeT saAmfCtDefQuiescingCompleteTimeout;
	SaAmfRecommendedRecoveryT saAmfCtDefRecoveryOnError;
	SaBoolT saAmfCtDefDisableRestart;

	struct avd_comp_tag *list_of_comp;

} AVD_COMP_TYPE;

/* AMF Class SaAmfCompCsType */
typedef struct avd_comp_cs_type_tag {
	NCS_PATRICIA_NODE tree_node;	/* name is key */
	SaNameT name;
	SaUint32T saAmfCompNumMaxActiveCSIs;
	SaUint32T saAmfCompNumMaxStandbyCSIs;
	SaUint32T saAmfCompNumCurrActiveCSIs;
	SaUint32T saAmfCompNumCurrStandbyCSIs;
	uint32_t   number_comp;     /* number of element in saAmfCompAssignedCsi */
	SaNameT *saAmfCompAssignedCsi;
	AVD_COMP *comp;
} AVD_COMPCS_TYPE;

/* AMF Class SaAmfCtCsType */
typedef struct {
	NCS_PATRICIA_NODE tree_node;	/* name is key */
	SaNameT name;
	SaAmfCompCapabilityModelT saAmfCtCompCapability;
	SaUint32T saAmfCtDefNumMaxActiveCSIs;
	SaUint32T saAmfCtDefNumMaxStandbyCSIs;
	AVD_COMP_TYPE *comptype;
} AVD_CTCS_TYPE;

extern AVD_COMP_GLOBALATTR avd_comp_global_attrs;

/**
 * Set the presence state of the specified component, log, update IMM & check point to peer
 * @param comp
 * @param pres_state
 */
extern void avd_comp_pres_state_set(AVD_COMP *comp, SaAmfPresenceStateT pres_state);

/**
 * Set the operational state of the specified component, log, update IMM & check point to peer
 * @param comp
 * @param oper_state
 */
extern void avd_comp_oper_state_set(AVD_COMP *comp, SaAmfOperationalStateT oper_state);

/**
 * Set the readindess state of the specified component, log, update IMM & check point to peer
 * @param comp
 * @param readiness_state
 */
extern void avd_comp_readiness_state_set(AVD_COMP *comp, SaAmfReadinessStateT readiness_state);

/**
 * Information regarding change of proxy status of the specified component,
 * log, update IMM & check point to peer
 * @param comp
 * @param proxy_status
 */
extern void avd_comp_proxy_status_change(AVD_COMP *comp, SaAmfProxyStatusT proxy_status);

extern void avd_comp_db_add(AVD_COMP *comp);

extern AVD_COMP *avd_comp_new(const SaNameT *dn);
extern void avd_comp_delete(AVD_COMP *comp);
extern AVD_COMP *avd_comp_get(const SaNameT *comp_name);
extern AVD_COMP *avd_comp_getnext(const SaNameT *comp_name);
extern void avd_su_remove_comp(AVD_COMP* comp);
extern SaAisErrorT avd_comp_config_get(const SaNameT* su_name, struct avd_su_tag* su);
extern void avd_comp_constructor(void);

extern SaAisErrorT avd_comptype_config_get(void);
extern AVD_COMP_TYPE *avd_comptype_get(const SaNameT *comp_type_name);
extern void avd_comptype_add_comp(AVD_COMP *comp);
extern void avd_comptype_remove_comp(AVD_COMP *comp);
extern void avd_comptype_constructor(void);

extern SaAisErrorT avd_compglobalattrs_config_get(void);
extern void avd_compglobalattrs_constructor(void);

extern SaAisErrorT avd_ctcstype_config_get(const SaNameT *comp_type_dn, AVD_COMP_TYPE *comp_type);
extern AVD_CTCS_TYPE *avd_ctcstype_get(const SaNameT *dn);
extern void avd_ctcstype_constructor(void);

extern AVD_COMPCS_TYPE *avd_compcstype_new(const SaNameT *dn);
extern void avd_compcstype_delete(AVD_COMPCS_TYPE **cst);
extern void avd_compcstype_db_add(AVD_COMPCS_TYPE *cst);
extern SaAisErrorT avd_compcstype_config_get(SaNameT *comp_name, AVD_COMP *comp);
extern AVD_COMPCS_TYPE *avd_compcstype_create(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes);
extern AVD_COMPCS_TYPE *avd_compcstype_get(const SaNameT *dn);
extern AVD_COMPCS_TYPE *avd_compcstype_getnext(const SaNameT *dn);
extern AVD_COMPCS_TYPE * avd_compcstype_find_match(const SaNameT *csi, const AVD_COMP *comp);
extern void avd_compcstype_constructor(void);
extern AVD_COMP *avd_comp_get_or_create(const SaNameT *dn);

#endif
