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
#ifndef AMF_AMFD_COMP_H_
#define AMF_AMFD_COMP_H_

#include <vector>

#include "amf/saf/saAmf.h"
#include "imm/saf/saImm.h"
#include "amf/amf_d2nmsg.h"
#include "amf/amfd/cb.h"
#include "amf/amf_db_template.h"

class AVD_SU;
class AVD_COMP_TYPE;

/* AMF Class SaAmfCompGlobalAttributes */
typedef struct {
	SaUint32T saAmfNumMaxInstantiateWithoutDelay;
	SaUint32T saAmfNumMaxInstantiateWithDelay;
	SaUint32T saAmfNumMaxAmStartAttempts;
	SaUint32T saAmfNumMaxAmStopAttempts;
	SaTimeT saAmfDelayBetweenInstantiateAttempts;
} AVD_COMP_GLOBALATTR;

/* AMF Class SaAmfCompType */
class AVD_COMP {
 public:
  AVD_COMP();
  explicit AVD_COMP(const std::string& dn);
  ~AVD_COMP();

/**
 * Set the presence state of the specified component, log, update IMM & check point to peer
 * @param comp
 * @param pres_state
 */
void avd_comp_pres_state_set(SaAmfPresenceStateT pres_state);

/**
 * Set the operational state of the specified component, log, update IMM & check point to peer
 * @param comp
 * @param oper_state
 */
void avd_comp_oper_state_set(SaAmfOperationalStateT oper_state);

/**
 * Set the readindess state of the specified component, log, update IMM & check point to peer
 * @param comp
 * @param readiness_state
 */
void avd_comp_readiness_state_set(SaAmfReadinessStateT readiness_state);

/**
 * Information regarding change of proxy status of the specified component,
 * log, update IMM & check point to peer
 * @param comp
 * @param proxy_status
 */
void avd_comp_proxy_status_change(SaAmfProxyStatusT proxy_status);

bool is_preinstantiable() const;
bool is_comp_assigned_any_csi() const;
SaAisErrorT check_comp_stability() const;

  std::string saAmfCompType;

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
  std::string comp_proxy_csi;
  std::string comp_container_csi;

  /* runtime attributes */
  SaAmfOperationalStateT saAmfCompOperState;	
  SaAmfReadinessStateT   saAmfCompReadinessState;
  SaAmfPresenceStateT    saAmfCompPresenceState;
  SaUint32T              saAmfCompRestartCount;
  std::string            saAmfCompCurrProxyName;
  std::vector<std::string> saAmfCompCurrProxiedNames;

  bool assign_flag;	/* Flag used while assigning. to mark this
                         * comp has been assigned a CSI from
                         * current SI being assigned
                         */
  AVD_COMP_TYPE *comp_type;
  AVD_COMP *comp_type_list_comp_next;
  AVD_SU *su;		/* SU to which this component belongs */
  AVD_ADMIN_OPER_CBK admin_pend_cbk;  /* holds callback invocation for admin operation */

  void set_assigned(bool assigned) {assign_flag = assigned;}
  bool assigned() const {return assign_flag;}
  bool saaware() const;
  bool proxied_pi() const;
  bool proxied_npi() const;
 private:
  void initialize();
  // disallow copy and assign
  AVD_COMP(const AVD_COMP&);
  void operator=(const AVD_COMP&);

};

extern AmfDb<std::string, AVD_COMP> *comp_db;

/* AMF Class SaAmfCompType */
class AVD_COMP_TYPE {
 public:
  explicit AVD_COMP_TYPE(const std::string& dn);
  std::string name {};
  SaUint32T saAmfCtCompCategory {};
  std::string saAmfCtSwBundle {};
  char saAmfCtDefCmdEnv[AVSV_MISC_STR_MAX_SIZE] {};
  SaTimeT saAmfCtDefClcCliTimeout {};
  SaTimeT saAmfCtDefCallbackTimeout {};
  char saAmfCtRelPathInstantiateCmd[AVSV_MISC_STR_MAX_SIZE] {};
  char saAmfCtDefInstantiateCmdArgv[AVSV_MISC_STR_MAX_SIZE] {};
  SaUint32T saAmfCtDefInstantiationLevel {};
  char saAmfCtRelPathTerminateCmd[AVSV_MISC_STR_MAX_SIZE] {};
  char saAmfCtDefTerminateCmdArgv[AVSV_MISC_STR_MAX_SIZE] {};
  char saAmfCtRelPathCleanupCmd[AVSV_MISC_STR_MAX_SIZE] {};
  char saAmfCtDefCleanupCmdArgv[AVSV_MISC_STR_MAX_SIZE] {};
  char saAmfCtRelPathAmStartCmd[AVSV_MISC_STR_MAX_SIZE] {};
  char saAmfCtDefAmStartCmdArgv[AVSV_MISC_STR_MAX_SIZE] {};
  char saAmfCtRelPathAmStopCmd[AVSV_MISC_STR_MAX_SIZE] {};
  char saAmfCtDefAmStopCmdArgv[AVSV_MISC_STR_MAX_SIZE] {};
  SaTimeT saAmfCtDefQuiescingCompleteTimeout {};
  SaAmfRecommendedRecoveryT saAmfCtDefRecoveryOnError {};
  SaBoolT saAmfCtDefDisableRestart {};

  AVD_COMP* list_of_comp {};
 private:
  AVD_COMP_TYPE();
  // disallow copy and assign
  AVD_COMP_TYPE(const AVD_COMP_TYPE&);
  void operator=(const AVD_COMP_TYPE&);
};
extern  AmfDb<std::string, AVD_COMP_TYPE> *comptype_db;

/* AMF Class SaAmfCompCsType */
class AVD_COMPCS_TYPE {
public:
	AVD_COMPCS_TYPE();

	explicit AVD_COMPCS_TYPE(const std::string& dn);

	std::string name {};
	SaUint32T saAmfCompNumMaxActiveCSIs {};
	SaUint32T saAmfCompNumMaxStandbyCSIs {};
	SaUint32T saAmfCompNumCurrActiveCSIs {};
	SaUint32T saAmfCompNumCurrStandbyCSIs {};
	std::vector<std::string> saAmfCompAssignedCsi {};
	AVD_COMP *comp {};
private:
	// disallow copy and assign
	AVD_COMPCS_TYPE(const AVD_COMPCS_TYPE&);
	void operator=(const AVD_COMPCS_TYPE&);
};
extern  AmfDb<std::string, AVD_COMPCS_TYPE> *compcstype_db;

/* AMF Class SaAmfCtCsType */
class AVD_CTCS_TYPE {
 public:
  explicit AVD_CTCS_TYPE(const std::string& dn);

  std::string name {};
  SaAmfCompCapabilityModelT saAmfCtCompCapability {};
  SaUint32T saAmfCtDefNumMaxActiveCSIs {};
  SaUint32T saAmfCtDefNumMaxStandbyCSIs {};
  AVD_COMP_TYPE *comptype {};
 private:
  AVD_CTCS_TYPE();
  // disallow copy and assign
  AVD_CTCS_TYPE(const AVD_CTCS_TYPE&);
  void operator=(const AVD_CTCS_TYPE&);
};

extern AmfDb<std::string, AVD_CTCS_TYPE> *ctcstype_db;

extern AVD_COMP_GLOBALATTR avd_comp_global_attrs;

extern void avd_comp_db_add(AVD_COMP *comp);

extern AVD_COMP *avd_comp_new(const std::string& dn);
extern void avd_comp_delete(AVD_COMP *comp);
extern void avd_su_remove_comp(AVD_COMP* comp);
extern SaAisErrorT avd_comp_config_get(const std::string& su_name, AVD_SU *su);
extern void avd_comp_constructor(void);

extern SaAisErrorT avd_comptype_config_get(void);
extern AVD_COMP_TYPE *avd_comptype_get(const std::string& comp_type_name);
extern void avd_comptype_add_comp(AVD_COMP *comp);
extern void avd_comptype_remove_comp(AVD_COMP *comp);
extern void avd_comptype_constructor(void);

extern SaAisErrorT avd_compglobalattrs_config_get(void);
extern void avd_compglobalattrs_constructor(void);

extern SaAisErrorT avd_ctcstype_config_get(const std::string& comp_type_dn, AVD_COMP_TYPE *comp_type);
extern void avd_ctcstype_constructor(void);

extern AVD_COMPCS_TYPE *avd_compcstype_new(const std::string& dn);
extern void avd_compcstype_delete(AVD_COMPCS_TYPE **cst);
extern void avd_compcstype_db_add(AVD_COMPCS_TYPE *cst);
extern SaAisErrorT avd_compcstype_config_get(const std::string& comp_name, AVD_COMP *comp);
extern AVD_COMPCS_TYPE *avd_compcstype_create(const std::string& dn, const SaImmAttrValuesT_2 **attributes);
extern AVD_COMPCS_TYPE *avd_compcstype_get(const std::string& dn);
extern AVD_COMPCS_TYPE *avd_compcstype_getnext(const std::string& dn);
extern AVD_COMPCS_TYPE * avd_compcstype_find_match(const std::string& csi, const AVD_COMP *comp);
extern void avd_compcstype_constructor(void);
extern AVD_COMP *avd_comp_get_or_create(const std::string& dn);
bool comp_is_preinstantiable(const AVD_COMP *comp);
extern bool is_comp_assigned_any_csi(AVD_COMP *comp);
extern SaAisErrorT check_comp_stability(const AVD_COMP*);
extern AVD_CTCS_TYPE *get_ctcstype(const std::string& comptype_name, const std::string& cstype_name);
extern void comp_ccb_apply_delete_hdlr(struct CcbUtilOperationData *opdata);
#endif  // AMF_AMFD_COMP_H_
