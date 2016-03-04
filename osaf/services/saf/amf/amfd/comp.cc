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
 *            Ericsson
 *
 */

/*****************************************************************************

  DESCRIPTION:This module deals with the creation, accessing and deletion of
  the component database on the AVD.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#include <saImmOm.h>
#include <immutil.h>
#include <logtrace.h>
#include <amf_util.h>
#include <util.h>
#include <comp.h>
#include <imm.h>
#include <node.h>
#include <csi.h>
#include <proc.h>
#include <ckpt_msg.h>

AmfDb<std::string, AVD_COMP> *comp_db = NULL;

void avd_comp_db_add(AVD_COMP *comp)
{
	unsigned int rc;

	if (comp_db->find(Amf::to_string(&comp->comp_info.name)) == NULL) {
		rc = comp_db->insert(Amf::to_string(&comp->comp_info.name), comp);
		osafassert(rc == NCSCC_RC_SUCCESS);
	}
}

//
void AVD_COMP::initialize() {
  comp_info = {};
  comp_info.cap = SA_AMF_COMP_ONE_ACTIVE_OR_ONE_STANDBY;
  comp_info.category = AVSV_COMP_TYPE_NON_SAF;
  comp_info.def_recvr = SA_AMF_COMPONENT_RESTART;
  comp_info.inst_level = 1;
  comp_info.comp_restart = true;
  nodefail_cleanfail = false;
  saAmfCompOperState = SA_AMF_OPERATIONAL_DISABLED;
  saAmfCompReadinessState = SA_AMF_READINESS_OUT_OF_SERVICE;
  saAmfCompPresenceState = SA_AMF_PRESENCE_UNINSTANTIATED;
  inst_retry_delay = {};
  max_num_inst_delay = {};
  max_num_csi_actv = {};
  max_num_csi_stdby = {};
  curr_num_csi_actv = {};
  curr_num_csi_stdby = {};
  comp_proxy_csi = {};
  comp_container_csi = {};
  saAmfCompRestartCount = {};
  saAmfCompCurrProxyName = {};
  saAmfCompCurrProxiedNames = {};
  assign_flag = false;
  comp_type = {};
  comp_type_list_comp_next = {};
  su = {};
  admin_pend_cbk = {};
}

//
AVD_COMP::AVD_COMP() {
  initialize();
}

//
AVD_COMP::AVD_COMP(const SaNameT *dn) {
  initialize();

  memcpy(&comp_info.name.value, dn->value, dn->length);
  comp_info.name.length = dn->length;
}

//
AVD_COMP *avd_comp_new(const SaNameT *dn)
{
	AVD_COMP *comp;

	comp = new AVD_COMP(dn);

        return comp;
}

/**
 * Set the presence state of a component. Updates the IMM copy. If newstate is
 * TERM-FAILED an alarm is sent and possibly a node reboot request is ordered.
 *
 * @param comp
 * @param pres_state
 */
void avd_comp_pres_state_set(AVD_COMP *comp, SaAmfPresenceStateT pres_state)
{
	AVD_AVND *node = comp->su->su_on_node;
	SaAmfPresenceStateT old_state = comp->saAmfCompPresenceState;

	osafassert(pres_state <= SA_AMF_PRESENCE_TERMINATION_FAILED);
	TRACE_ENTER2("'%s' %s => %s", comp->comp_info.name.value,
		avd_pres_state_name[comp->saAmfCompPresenceState],
		avd_pres_state_name[pres_state]);

	if ((comp->saAmfCompPresenceState == SA_AMF_PRESENCE_TERMINATION_FAILED) &&
			(pres_state == SA_AMF_PRESENCE_UNINSTANTIATED)){
		avd_alarm_clear(&comp->comp_info.name,
			SA_AMF_NTFID_COMP_CLEANUP_FAILED, SA_NTF_SOFTWARE_ERROR);
	}

	if ((comp->saAmfCompPresenceState == SA_AMF_PRESENCE_INSTANTIATION_FAILED) &&
			(pres_state == SA_AMF_PRESENCE_UNINSTANTIATED)){
		avd_alarm_clear(&comp->comp_info.name,
				SA_AMF_NTFID_COMP_INSTANTIATION_FAILED, SA_NTF_SOFTWARE_ERROR);
	}


	comp->saAmfCompPresenceState = pres_state;
	avd_saImmOiRtObjectUpdate(&comp->comp_info.name, "saAmfCompPresenceState",
		SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompPresenceState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, comp, AVSV_CKPT_COMP_PRES_STATE);

	if (comp->saAmfCompPresenceState == SA_AMF_PRESENCE_INSTANTIATION_FAILED)
		avd_send_comp_inst_failed_alarm(&comp->comp_info.name, &node->name);
	else if (comp->saAmfCompPresenceState == SA_AMF_PRESENCE_TERMINATION_FAILED)
		avd_send_comp_clean_failed_alarm(&comp->comp_info.name, &node->name);

	if ((comp->su->sg_of_su->saAmfSGAutoRepair == true) &&
		(node->saAmfNodeAutoRepair == true) &&
		(((node->saAmfNodeFailfastOnTerminationFailure == true) && 
		 (comp->saAmfCompPresenceState == SA_AMF_PRESENCE_TERMINATION_FAILED)) ||
		 ((node->saAmfNodeFailfastOnInstantiationFailure == true) && 
		  (comp->saAmfCompPresenceState == SA_AMF_PRESENCE_INSTANTIATION_FAILED)))) {

		saflog(LOG_NOTICE, amfSvcUsrName, "%s PresenceState %s => %s",
				comp->comp_info.name.value, avd_pres_state_name[old_state],
				avd_pres_state_name[pres_state]);
		saflog(LOG_NOTICE, amfSvcUsrName,
				"Ordering reboot of '%s' as repair action",
				node->name.value);
		LOG_NO("Node Failfast for '%s' as '%s' enters Term/Inst Failed state",
				node->name.value,comp->comp_info.name.value);
		avd_d2n_reboot_snd(node);
	}
	TRACE_LEAVE();
}

void avd_comp_oper_state_set(AVD_COMP *comp, SaAmfOperationalStateT oper_state)
{
	osafassert(oper_state <= SA_AMF_OPERATIONAL_DISABLED);
	TRACE_ENTER2("'%s' %s => %s",
		comp->comp_info.name.value, avd_oper_state_name[comp->saAmfCompOperState], avd_oper_state_name[oper_state]);

	comp->saAmfCompOperState = oper_state;
	avd_saImmOiRtObjectUpdate(&comp->comp_info.name, "saAmfCompOperState",
		SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompOperState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, comp, AVSV_CKPT_COMP_OPER_STATE);
	TRACE_LEAVE();
}

void avd_comp_readiness_state_set(AVD_COMP *comp, SaAmfReadinessStateT readiness_state)
{
	if (comp->saAmfCompReadinessState == readiness_state)
		return;

	osafassert(readiness_state <= SA_AMF_READINESS_STOPPING);
	TRACE_ENTER2("'%s' %s => %s",
		comp->comp_info.name.value,
		avd_readiness_state_name[comp->saAmfCompReadinessState], avd_readiness_state_name[readiness_state]);
	comp->saAmfCompReadinessState = readiness_state;
	if (comp->su->surestart == false)
		avd_saImmOiRtObjectUpdate(&comp->comp_info.name, "saAmfCompReadinessState",
				SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompReadinessState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, comp, AVSV_CKPT_COMP_READINESS_STATE);
	TRACE_LEAVE();
}

void avd_comp_proxy_status_change(AVD_COMP *comp, SaAmfProxyStatusT proxy_status)
{
	osafassert(proxy_status <= SA_AMF_PROXY_STATUS_PROXIED);
	TRACE_ENTER2("'%s' ProxyStatus is now %s", comp->comp_info.name.value, avd_proxy_status_name[proxy_status]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s ProxyStatus is now %s", 
			comp->comp_info.name.value, avd_proxy_status_name[proxy_status]);

	/* alarm & notifications */
	if(proxy_status == SA_AMF_PROXY_STATUS_UNPROXIED)
		avd_send_comp_proxy_status_unproxied_alarm(&comp->comp_info.name);
	else if(proxy_status == SA_AMF_PROXY_STATUS_PROXIED)
		avd_send_comp_proxy_status_proxied_ntf(&comp->comp_info.name, 
		                                       SA_AMF_PROXY_STATUS_UNPROXIED, 
		                                       SA_AMF_PROXY_STATUS_PROXIED);

}

void avd_comp_delete(AVD_COMP *comp)
{
	AVD_SU *su = comp->su;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, comp, AVSV_CKPT_AVD_COMP_CONFIG);
	su->remove_comp(comp);
	avd_comptype_remove_comp(comp);
	comp_db->erase(Amf::to_string(&comp->comp_info.name));
	delete comp;
}

/**
 * Add comp to model
 * @param comp
 */
static void comp_add_to_model(AVD_COMP *comp)
{
	SaNameT dn;
	AVD_SU *su = comp->su;

	TRACE_ENTER2("%s", comp->comp_info.name.value);

	/* Check parent link to see if it has been added already */
	if (su != NULL) {
		TRACE("already added");
		goto done;
	}

	avsv_sanamet_init(&comp->comp_info.name, &dn, "safSu");
	su = comp->su = su_db->find(Amf::to_string(&dn));

	avd_comp_db_add(comp);
	comp->comp_type = comptype_db->find(Amf::to_string(&comp->saAmfCompType));
	osafassert(comp->comp_type);
	avd_comptype_add_comp(comp);
	su->add_comp(comp);

	/* check if the
	 * corresponding node is UP send the component information
	 * to the Node.
	 */
	if (su->su_is_external) {
		/* This is an external SU, so there is no node assigned to it.
		   For some purpose of validations and sending SU/Comps info to
		   hosting node (Controllers), we can take use of the hosting
		   node. */
		if ((AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE ==
		     comp->comp_info.category) ||
		    (AVSV_COMP_TYPE_EXTERNAL_NON_PRE_INSTANTIABLE == comp->comp_info.category)) {
			/* This is a valid external component. Ext comp is in ext
			   SU. */
		} else {
			/* This is not a valid external component. External SU has 
			   been assigned a cluster component. */
			su->remove_comp(comp);
			LOG_ER("Not A Valid External Component: Category '%u'configured",
				comp->comp_info.category);
			return;
		}
	}

	/* This is a case of adding a component in SU which is instantiated
	   state. This could be used in upgrade scenarios. When components
	   are added, it is sent to Amfnd for instantiation and Amfnd
	   instantiates it. Amfnd has capability for finding which component
	   has been added newly. Allow live component instantiation only for
	   middleware component to support older campaign to work for
	   application in compatible manner.*/
	if ((su->sg_of_su->sg_ncs_spec == true) &&
			(su->saAmfSUAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
			(su->saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATED) && 
			(avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE)) {
		AVD_AVND *node = su->su_on_node;
		if ((node->node_state == AVD_AVND_STATE_PRESENT) ||
				(node->node_state == AVD_AVND_STATE_NO_CONFIG) ||
				(node->node_state == AVD_AVND_STATE_NCS_INIT)) {
			if (avd_snd_su_msg(avd_cb, su) != NCSCC_RC_SUCCESS) {
				LOG_ER("SU '%s', Comp '%s': avd_snd_su_msg failed %s", __FUNCTION__,
						su->name.value, comp->comp_info.name.value);
				goto done;
			}
		}
	}

	if ((comp->su->su_on_node->node_state == AVD_AVND_STATE_PRESENT) ||
			(comp->su->su_on_node->node_state == AVD_AVND_STATE_NO_CONFIG) ||
			(comp->su->su_on_node->node_state == AVD_AVND_STATE_NCS_INIT))
		avd_comp_oper_state_set(comp, SA_AMF_OPERATIONAL_ENABLED);

	/* Set runtime cached attributes. */
	avd_saImmOiRtObjectUpdate(&comp->comp_info.name, "saAmfCompReadinessState",
		SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompReadinessState);

	avd_saImmOiRtObjectUpdate(&comp->comp_info.name, "saAmfCompOperState",
		SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompOperState);
	
	avd_saImmOiRtObjectUpdate(&comp->comp_info.name, "saAmfCompPresenceState",
		SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompPresenceState);

	m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(avd_cb, comp, AVSV_CKPT_AVD_COMP_CONFIG);

done:
	TRACE_LEAVE();
}

/**
 * Validate configuration attributes for an AMF Comp object
 * @param comp
 * 
 * @return int
 */
static int is_config_valid(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;
	SaNameT aname;
	char *parent;
	SaUint32T value;

	if ((parent = strchr((char*)dn->value, ',')) == NULL) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn->value);
		return 0;
	}

	if (strncmp(++parent, "safSu=", 6) != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", parent, dn->value);
		return 0;
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompType"), attributes, 0, &aname);
	osafassert(rc == SA_AIS_OK);

	if (comptype_db->find(Amf::to_string(&aname)) == NULL) {
		/* Comp type does not exist in current model, check CCB */
		if (opdata == NULL) {
			report_ccb_validation_error(opdata, "'%s' does not exist in model", aname.value);
			return 0;
		}

		if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &aname) == NULL) {
			report_ccb_validation_error(opdata, "'%s' does not exist in existing model or in CCB", aname.value);
			return 0;
		}
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompRecoveryOnError"), attributes, 0, &value);
	if (rc == SA_AIS_OK) {
		if ((value < SA_AMF_NO_RECOMMENDATION) || (value > SA_AMF_NODE_FAILFAST)) {
			report_ccb_validation_error(opdata, "Illegal/unsupported saAmfCompRecoveryOnError value %u for '%s'",
				   value, dn->value);
			return 0;
		}

		if (value == SA_AMF_NO_RECOMMENDATION)
			LOG_NO("Invalid configuration, saAmfCompRecoveryOnError=NO_RECOMMENDATION(%u) for '%s'",
				   value, dn->value);
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompDisableRestart"), attributes, 0, &value);
	if ((rc == SA_AIS_OK) && (value > SA_TRUE)) {
		report_ccb_validation_error(opdata, "Illegal saAmfCompDisableRestart value %u for '%s'",
			   value, dn->value);
		return 0;
	}

#if 0
	if ((comp->comp_info.category == AVSV_COMP_TYPE_SA_AWARE) && (comp->comp_info.init_len == 0)) {
		LOG_ER("Sa Aware Component: instantiation command not configured");
		return -1;
	} else if ((comp->comp_info.category == AVSV_COMP_TYPE_NON_SAF) &&
		   ((comp->comp_info.init_len == 0) || (comp->comp_info.term_len == 0))) {
		LOG_ER("Non-SaAware Component: instantiation or termination command not configured");
		return -1;
	}

	if ((AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE == comp->comp_info.category) ||
	    (AVSV_COMP_TYPE_EXTERNAL_NON_PRE_INSTANTIABLE == comp->comp_info.category)) {
		if ((comp->comp_info.init_len == 0) ||
		    (comp->comp_info.term_len == 0) || (comp->comp_info.clean_len == 0)) {
			/* For external component, the following fields should not be 
			   filled. */
		} else {
			LOG_ER("External Component: instantiation or termination not configured");
			return -1;
		}
	} else {
		if (comp->comp_info.clean_len == 0) {
			LOG_ER("Cluster Component: Cleanup script not configured");
			return -1;
		}

		if (comp->comp_info.max_num_inst == 0) {
			LOG_ER("Cluster Component: Max num inst not configured");
			return -1;
		}
	}

	if ((comp->comp_info.category == AVSV_COMP_TYPE_SA_AWARE) ||
	    (comp->comp_info.category == AVSV_COMP_TYPE_PROXIED_LOCAL_PRE_INSTANTIABLE) ||
	    (comp->comp_info.category == AVSV_COMP_TYPE_PROXIED_LOCAL_NON_PRE_INSTANTIABLE)) {

		if (comp->comp_info.cap == NCS_COMP_CAPABILITY_1_ACTIVE_OR_Y_STANDBY) {
			comp->max_num_csi_actv = 1;
		} else if ((comp->comp_info.cap == NCS_COMP_CAPABILITY_1_ACTIVE_OR_1_STANDBY) ||
			   (comp->comp_info.cap == NCS_COMP_CAPABILITY_1_ACTIVE)) {
			comp->max_num_csi_actv = 1;
			comp->max_num_csi_stdby = 1;
		} else if (comp->comp_info.cap == NCS_COMP_CAPABILITY_X_ACTIVE) {
			comp->max_num_csi_stdby = comp->max_num_csi_actv;
		}

		if ((comp->max_num_csi_actv == 0) || (comp->max_num_csi_stdby == 0)) {
			LOG_ER("Max Act Csi or Max Stdby Csi not configured");
			return -1;
		}
	}

	sg_red_model = su->sg_of_su->sg_redundancy_model;

	/* Check illegal component capability/category wrt SG red model */
	if (((sg_red_model == SA_AMF_N_WAY_REDUNDANCY_MODEL) &&
	     ((comp->comp_info.cap != NCS_COMP_CAPABILITY_X_ACTIVE_AND_Y_STANDBY) ||
	      (comp->comp_info.category == AVSV_COMP_TYPE_NON_SAF)))) {
		LOG_ER("Illegal category %u or cap %u for SG red model %u",
			comp->comp_info.category, comp->comp_info.cap, sg_red_model);
		return -1;
	}

	/* Check illegal component capability wrt SG red model */
	if ((sg_red_model == SA_AMF_NPM_REDUNDANCY_MODEL) &&
	    (comp->comp_info.cap != NCS_COMP_CAPABILITY_1_ACTIVE_OR_1_STANDBY)) {
		LOG_ER("Illegal capability %u for SG red model %u",
			comp->comp_info.cap, sg_red_model);
		return -1;
	}

	/* Verify that the SU can contain this component */
	{
		AVD_SUTCOMP_TYPE *sutcomptype;
		SaNameT sutcomptype_name;

		avd_create_association_class_dn(&comp->saAmfCompType, &su->saAmfSUType,
			"safMemberCompType", &sutcomptype_name);
		sutcomptype = avd_sutcomptype_get(&sutcomptype_name);
		if (sutcomptype == NULL) {
			LOG_ER("Not found '%s'", sutcomptype_name.value);
			return -1;
		}

		if (sutcomptype->curr_num_components == sutcomptype->saAmfSutMaxNumComponents) {
			LOG_ER("SU '%s' cannot contain more components of this type '%s*",
				su->name.value, comp->saAmfCompType.value);
			return -1;
		}
	}

	if (true == su->su_is_external) {
		if ((true == comp->comp_info.am_enable) ||
		    (0 != comp->comp_info.amstart_len) || (0 != comp->comp_info.amstop_len)) {
			LOG_ER("External Component: Active monitoring configured");
			return -1;
		} else {
			/* There are default values assigned to amstart_time, 
			   amstop_time and clean_time. Since these values are not 
			   used for external components, so we will reset it. */
			comp->comp_info.amstart_time = 0;
			comp->comp_info.amstop_time = 0;
			comp->comp_info.clean_time = 0;
			comp->comp_info.max_num_amstart = 0;
		}
	}
#endif
	return 1;
}

static AVD_COMP *comp_create(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes)
{
	int rc = -1;
	AVD_COMP *comp;
	char *cmd_argv;
	const char *str;
	const AVD_COMP_TYPE *comptype;
	SaAisErrorT error;

	TRACE_ENTER2("'%s'", dn->value);

	/*
	** If called at new active at failover, the object is found in the DB
	** but needs to get configuration attributes initialized.
	*/
	if (NULL == (comp = comp_db->find(Amf::to_string(dn)))) {
		if ((comp = avd_comp_new(dn)) == NULL)
			goto done;
	}
	else
		TRACE("already created, refreshing config...");

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompType"), attributes, 0, &comp->saAmfCompType);
	osafassert(error == SA_AIS_OK);

	if ((comptype = comptype_db->find(Amf::to_string(&comp->saAmfCompType))) == NULL) {
		LOG_ER("saAmfCompType '%s' does not exist", comp->saAmfCompType.value);
		goto done;
	}

	/*  TODO clean this up! */
	comp->comp_info.category = avsv_amfcompcategory_to_avsvcomptype(comptype->saAmfCtCompCategory);
	if (comp->comp_info.category == AVSV_COMP_TYPE_INVALID) {
		LOG_ER("Comp category %x invalid for '%s'", comp->comp_info.category, comp->saAmfCompType.value);
		goto done;
	}

	if (strlen(comptype->saAmfCtRelPathInstantiateCmd) > 0) {
		strcpy(comp->comp_info.init_info, comptype->saAmfCtRelPathInstantiateCmd);
		cmd_argv = comp->comp_info.init_info + strlen(comp->comp_info.init_info);
		*cmd_argv++ = 0x20;	/* Insert SPACE between cmd and args */

		if ((str = immutil_getStringAttr(attributes, "saAmfCompInstantiateCmdArgv", 0)) == NULL)
			str = comptype->saAmfCtDefInstantiateCmdArgv;

		if (str != NULL)
			strcpy(cmd_argv, str);

		comp->comp_info.init_len = strlen(comp->comp_info.init_info);
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompInstantiateTimeout"), attributes, 0, &comp->comp_info.init_time) != SA_AIS_OK)
		comp->comp_info.init_time = comptype->saAmfCtDefClcCliTimeout;

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompInstantiationLevel"), attributes, 0, &comp->comp_info.inst_level) != SA_AIS_OK)
		comp->comp_info.inst_level = comptype->saAmfCtDefInstantiationLevel;

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompNumMaxInstantiateWithoutDelay"), attributes,
			    0, &comp->comp_info.max_num_inst) != SA_AIS_OK)
		comp->comp_info.max_num_inst = avd_comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay;

	/*  TODO what is implemented? With or without delay? */

#if 0
	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompNumMaxInstantiateWithDelay"), attributes,
			    0, &comp->max_num_inst_delay) != SA_AIS_OK)
		comp->comp_info.max_num_inst = avd_comp_global_attrs.saAmfNumMaxInstantiateWithDelay;
#endif

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompDelayBetweenInstantiateAttempts"), attributes,
			    0, &comp->inst_retry_delay) != SA_AIS_OK)
		comp->inst_retry_delay = avd_comp_global_attrs.saAmfDelayBetweenInstantiateAttempts;

	if (strlen(comptype->saAmfCtRelPathTerminateCmd) > 0) {
		strcpy(comp->comp_info.term_info, comptype->saAmfCtRelPathTerminateCmd);
		cmd_argv = comp->comp_info.term_info + strlen(comp->comp_info.term_info);
		*cmd_argv++ = 0x20;	/* Insert SPACE between cmd and args */

		if ((str = immutil_getStringAttr(attributes, "saAmfCompTerminateCmdArgv", 0)) == NULL)
			str = comptype->saAmfCtDefTerminateCmdArgv;

		if (str != NULL)
			strcpy(cmd_argv, str);

		comp->comp_info.term_len = strlen(comp->comp_info.term_info);
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompTerminateTimeout"), attributes,
			    0, &comp->comp_info.terminate_callback_timeout) != SA_AIS_OK)
		comp->comp_info.terminate_callback_timeout = comptype->saAmfCtDefCallbackTimeout;

	if (strlen(comptype->saAmfCtRelPathCleanupCmd) > 0) {
		strcpy(comp->comp_info.clean_info, comptype->saAmfCtRelPathCleanupCmd);
		cmd_argv = comp->comp_info.clean_info + strlen(comp->comp_info.clean_info);
		*cmd_argv++ = 0x20;	/* Insert SPACE between cmd and args */

		if ((str = immutil_getStringAttr(attributes, "saAmfCompCleanupCmdArgv", 0)) == NULL)
			str = comptype->saAmfCtDefCleanupCmdArgv;

		if (str != NULL)
			strcpy(cmd_argv, str);

		comp->comp_info.clean_len = strlen(comp->comp_info.clean_info);
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompCleanupTimeout"), attributes, 0, &comp->comp_info.clean_time) != SA_AIS_OK)
		comp->comp_info.clean_time = comptype->saAmfCtDefClcCliTimeout;

	if (strlen(comptype->saAmfCtRelPathAmStartCmd) > 0) {
		strcpy(comp->comp_info.amstart_info, comptype->saAmfCtRelPathAmStartCmd);
		cmd_argv = comp->comp_info.amstart_info + strlen(comp->comp_info.amstart_info);
		*cmd_argv++ = 0x20;	/* Insert SPACE between cmd and args */

		if ((str = immutil_getStringAttr(attributes, "saAmfCompAmStartCmdArgv", 0)) == NULL)
			str = comptype->saAmfCtDefAmStartCmdArgv;

		if (str != NULL)
			strcpy(cmd_argv, str);

		comp->comp_info.amstart_len = strlen(comp->comp_info.amstart_info);
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompAmStartTimeout"), attributes, 0, &comp->comp_info.amstart_time) != SA_AIS_OK)
		comp->comp_info.amstart_time = comptype->saAmfCtDefClcCliTimeout;

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompNumMaxAmStartAttempts"), attributes,
			    0, &comp->comp_info.max_num_amstart) != SA_AIS_OK)
		comp->comp_info.max_num_amstart = avd_comp_global_attrs.saAmfNumMaxAmStartAttempts;

	if (strlen(comptype->saAmfCtRelPathAmStopCmd) > 0) {
		strcpy(comp->comp_info.amstop_info, comptype->saAmfCtRelPathAmStopCmd);
		cmd_argv = comp->comp_info.amstop_info + strlen(comp->comp_info.amstop_info);
		*cmd_argv++ = 0x20;	/* Insert SPACE between cmd and args */

		if ((str = immutil_getStringAttr(attributes, "saAmfCompAmStopCmdArgv", 0)) == NULL)
			str = comptype->saAmfCtDefAmStopCmdArgv;

		if (str != NULL)
			strcpy(cmd_argv, str);
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompAmStopTimeout"), attributes, 0, &comp->comp_info.amstop_time) != SA_AIS_OK)
		comp->comp_info.amstop_time = comptype->saAmfCtDefClcCliTimeout;

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompNumMaxAmStopAttempts"), attributes,
			    0, &comp->comp_info.max_num_amstop) != SA_AIS_OK)
		comp->comp_info.max_num_amstop = avd_comp_global_attrs.saAmfNumMaxAmStopAttempts;

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompCSISetCallbackTimeout"), attributes,
			    0, &comp->comp_info.csi_set_callback_timeout) != SA_AIS_OK)
		comp->comp_info.csi_set_callback_timeout = comptype->saAmfCtDefCallbackTimeout;

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompCSIRmvCallbackTimeout"), attributes,
			    0, &comp->comp_info.csi_rmv_callback_timeout) != SA_AIS_OK)
		comp->comp_info.csi_rmv_callback_timeout = comptype->saAmfCtDefCallbackTimeout;

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompQuiescingCompleteTimeout"), attributes,
			    0, &comp->comp_info.quiescing_complete_timeout) != SA_AIS_OK)
		comp->comp_info.quiescing_complete_timeout = comptype->saAmfCtDefQuiescingCompleteTimeout;

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompRecoveryOnError"), attributes, 0, &comp->comp_info.def_recvr) != SA_AIS_OK)
		comp->comp_info.def_recvr = comptype->saAmfCtDefRecoveryOnError;

	if (comp->comp_info.def_recvr == SA_AMF_NO_RECOMMENDATION) {
		comp->comp_info.def_recvr = SA_AMF_COMPONENT_FAILOVER;
		LOG_NO("COMPONENT_FAILOVER(%u) used instead of NO_RECOMMENDATION(%u) for '%s'",
			   SA_AMF_COMPONENT_FAILOVER, SA_AMF_NO_RECOMMENDATION, comp->comp_info.name.value);
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCompDisableRestart"), attributes, 0, &comp->comp_info.comp_restart) != SA_AIS_OK)
		comp->comp_info.comp_restart = comptype->saAmfCtDefDisableRestart;

	comp->max_num_csi_actv = -1;	// TODO
	comp->max_num_csi_stdby = -1;	// TODO

	rc = 0;
done:
	if (rc != 0) {
		avd_comp_delete(comp);
		comp = NULL;
	}

	TRACE_LEAVE();
	return comp;
}

/**
 * Get configuration for all SaAmfComp objects from IMM and
 * create AVD internal objects.
 * @param cb
 * 
 * @return int
 */
SaAisErrorT avd_comp_config_get(const SaNameT *su_name, AVD_SU *su)
{
	SaAisErrorT rc, error = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT comp_name;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfComp";
	AVD_COMP *comp;
	unsigned int num_of_comp_in_su = 0;
	SaImmAttrNameT configAttributes[] = {
		const_cast<SaImmAttrNameT>("saAmfCompType"),
		const_cast<SaImmAttrNameT>("saAmfCompCmdEnv"),
		const_cast<SaImmAttrNameT>("saAmfCompInstantiateCmdArgv"),
		const_cast<SaImmAttrNameT>("saAmfCompInstantiateTimeout"),
		const_cast<SaImmAttrNameT>("saAmfCompInstantiationLevel"),
		const_cast<SaImmAttrNameT>("saAmfCompNumMaxInstantiateWithoutDelay"),
		const_cast<SaImmAttrNameT>("saAmfCompNumMaxInstantiateWithDelay"),
		const_cast<SaImmAttrNameT>("saAmfCompDelayBetweenInstantiateAttempts"),
		const_cast<SaImmAttrNameT>("saAmfCompTerminateCmdArgv"),
		const_cast<SaImmAttrNameT>("saAmfCompTerminateTimeout"),
		const_cast<SaImmAttrNameT>("saAmfCompCleanupCmdArgv"),
		const_cast<SaImmAttrNameT>("saAmfCompCleanupTimeout"),
		const_cast<SaImmAttrNameT>("saAmfCompAmStartCmdArgv"),
		const_cast<SaImmAttrNameT>("saAmfCompAmStartTimeout"),
		const_cast<SaImmAttrNameT>("saAmfCompNumMaxAmStartAttempts"),
		const_cast<SaImmAttrNameT>("saAmfCompAmStopCmdArgv"),
		const_cast<SaImmAttrNameT>("saAmfCompAmStopTimeout"),
		const_cast<SaImmAttrNameT>("saAmfCompNumMaxAmStopAttempts"),
		const_cast<SaImmAttrNameT>("saAmfCompCSISetCallbackTimeout"),
		const_cast<SaImmAttrNameT>("saAmfCompCSIRmvCallbackTimeout"),
		const_cast<SaImmAttrNameT>("saAmfCompQuiescingCompleteTimeout"),
		const_cast<SaImmAttrNameT>("saAmfCompRecoveryOnError"),
		const_cast<SaImmAttrNameT>("saAmfCompDisableRestart"),
		NULL
	};

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if ((rc = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, su_name,
		SA_IMM_SUBTREE, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR,
		&searchParam, configAttributes, &searchHandle)) != SA_AIS_OK) {

		LOG_ER("%s: saImmOmSearchInitialize_2 failed: %u", __FUNCTION__, rc);
		goto done1;
	}

	while ((rc = immutil_saImmOmSearchNext_2(searchHandle, &comp_name,
		(SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK) {

		if (!is_config_valid(&comp_name, attributes, NULL))
			goto done2;

		if ((comp = comp_create(&comp_name, attributes)) == NULL)
			goto done2;

		num_of_comp_in_su ++;
		comp_add_to_model(comp);

		if (avd_compcstype_config_get(&comp_name, comp) != SA_AIS_OK)
			goto done2;
	}

	/* If there are no component in the SU, we treat it as invalid configuration. */
	if (0 == num_of_comp_in_su) {
		LOG_ER("There is no component configured for SU '%s'", su_name->value);
		goto done2;
	}

	osafassert(rc == SA_AIS_ERR_NOT_EXIST);
	error = SA_AIS_OK;

done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

/**
 * Handle admin operations on SaAmfComp objects.
 *      
 * @param immOiHandle             
 * @param invocation
 * @param objectName
 * @param operationId
 * @param params
 */
static void comp_admin_op_cb(SaImmOiHandleT immOiHandle, SaInvocationT invocation,
	const SaNameT *objectName, SaImmAdminOperationIdT opId,
	const SaImmAdminOperationParamsT_2 **params)
{
        TRACE_ENTER2("%llu, '%s', %llu", invocation, objectName->value, opId);

	AVD_COMP *comp = comp_db->find(Amf::to_string(objectName));
	osafassert(comp != NULL);

	switch (opId) {
		/* Valid B.04 AMF comp admin operations */
	case SA_AMF_ADMIN_RESTART:
		if (comp->admin_pend_cbk.invocation != 0) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
					"Component undergoing admin operation '%s'", objectName->value);
		} else if ((comp->su->sg_of_su->sg_ncs_spec == true) &&
				(comp->su->sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL)) {
                        report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
                                        "Not allowed on comp of middleware 2N SU : %s, op_id: %llu",
                                        objectName->value, opId);
		}
		else if (comp->su->pend_cbk.invocation != 0) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
					"SU undergoing admin operation '%s'", objectName->value);
		}
		else if (comp->su->su_on_node->admin_node_pend_cbk.invocation != 0) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
					"Node undergoing admin operation '%s'", objectName->value);
		}
		else if (comp->saAmfCompPresenceState != SA_AMF_PRESENCE_INSTANTIATED) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
					"Component not instantiated '%s'", objectName->value);
		}
		else if (comp->saAmfCompOperState == SA_AMF_OPERATIONAL_DISABLED) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
					"Component disabled, first repair su or check node status'%s'", objectName->value);
		} else {
			comp->admin_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(opId);
			comp->admin_pend_cbk.invocation = invocation;

			if ((comp->comp_info.comp_restart == true) &&
					(is_comp_assigned_any_csi(comp) == true)) {
				/* Atleast one non-restartable (saAmfCompDisableRestart or
				   saAmfCtDefDisableRestart is true) comp is assigned. 
				   First gracefully  switch-over its assignments to comp in 
				   other SU. At present assignment of whole SU will be gracefully
				   reassigned.
				   Thus PI applications modeled on NWay and Nway Active model
				   this is spec deviation.
				 */
				if (comp->su->saAmfSUPreInstantiable == true) {
					TRACE("surestart flag in '%s' is set to true",comp->su->name.value);
					comp->su->surestart = true;
				}
				comp->su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
				comp->su->sg_of_su->su_fault(avd_cb, comp->su);
			} else {
				/* For a non restartable comp, amfd has no role in reassignment.
                                   AMFND will take care of reassignment.*/

				/* prepare the admin op req message and queue it */
				if (avd_admin_op_msg_snd(&comp->comp_info.name, AVSV_SA_AMF_COMP,
							static_cast<SaAmfAdminOperationIdT>(opId),
							comp->su->su_on_node) != NCSCC_RC_SUCCESS) {
					report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TIMEOUT, NULL,
							"Admin op request send failed '%s'", objectName->value);
					comp->admin_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(0);
					comp->admin_pend_cbk.invocation = 0;
				}
			}
		}
		break;

	case SA_AMF_ADMIN_EAM_START:
	case SA_AMF_ADMIN_EAM_STOP:
	default:
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NOT_SUPPORTED, NULL,
				"Unsupported admin operation '%llu'", opId);
		break;
	}

	TRACE_LEAVE();
}

static SaAisErrorT comp_rt_attr_cb(SaImmOiHandleT immOiHandle,
	const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	AVD_COMP *comp = comp_db->find(Amf::to_string(objectName));
	SaImmAttrNameT attributeName;
	int i = 0;

	TRACE_ENTER2("'%s'", objectName->value);
	osafassert(comp != NULL);

	while ((attributeName = attributeNames[i++]) != NULL) {
		if (!strcmp("saAmfCompRestartCount", attributeName)) {
			avd_saImmOiRtObjectUpdate_sync(objectName, attributeName,
				SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompRestartCount);
		} else if (!strcmp("saAmfCompCurrProxyName", attributeName)) {
			avd_saImmOiRtObjectUpdate_sync(objectName, attributeName,
				SA_IMM_ATTR_SANAMET, &comp->saAmfCompCurrProxyName);
			/* TODO */
		} else if (!strcmp("saAmfCompCurrProxiedNames", attributeName)) {
			/* TODO */
		} else {
			LOG_ER("Ignoring unknown attribute '%s'", attributeName);
		}
	}

	return SA_AIS_OK;
}

static SaAisErrorT ccb_completed_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;
	AVD_COMP *comp;
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	bool value_is_deleted;

	TRACE_ENTER();

	comp = comp_db->find(Amf::to_string(&opdata->objectName));

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
		const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;
		void *value = NULL;

		 if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || (attribute->attrValues == NULL)) {
                        /* Attribute value is deleted, revert to default value */
                        value_is_deleted = true;
                } else {
                        /* Attribute value is modified */
                        value_is_deleted = false;
                        value = attribute->attrValues[0];
                }

		if (!strcmp(attribute->attrName, "saAmfCompType")) {
			if (value_is_deleted == true)
				continue;
			SaNameT dn = *((SaNameT*)value);
			if (NULL == comptype_db->find(Amf::to_string(&dn))) {
				report_ccb_validation_error(opdata, "saAmfCompType '%s' not found", dn.value);
				goto done;
			}
			/*
			   This new compType exists in the AMF. Before modifying compType attribute in a comp,
			   one more check on the validity of SaAmfCtCsType is still required which means 
			   association object (object of SaAmfCtCsType) between this new comptype and cstype
			   in SaAmfCompCsType (in case such an object exists for this component) must also 
			   exists in the system. If ctcstype does not exist then there will be problem in 
			   deciding components capability for a given cstype. So reject the modify ccb if 
			   ctcstype does not exist with any cstypes supported by this component via compcstype.
			 */ 

			for (std::map<std::string, AVD_COMPCS_TYPE*>::const_iterator it = compcstype_db->begin();
					it != compcstype_db->end(); it++) {
				AVD_COMPCS_TYPE *compcstype = it->second;
				if (compcstype->comp == comp) {
					SaNameT cstype_name;
					AVD_CTCS_TYPE *ctcstype = NULL;
					AVD_CS_TYPE *cst = NULL;
					get_child_dn_from_ass_dn(&compcstype->name, &cstype_name);
					//First check if this cstype exists in the sustem.
					if ((cst = cstype_db->find(Amf::to_string(&cstype_name))) == NULL) {
						LOG_WA("cstype of '%s' is not preseint in AMF database",
								compcstype->name.value);
						continue;
					}
					//ctcstype relationship should exists with all the cstypes.
					if ((ctcstype = get_ctcstype(&dn, &cstype_name)) == NULL) {	
						report_ccb_validation_error(opdata, "ctcstype relationship " 
								"between new comptype and cstype from"
								"component's compcstype(s) does not exist");
						goto done;
					}
				}
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiateCmdArgv")) {
			if (value_is_deleted == true)
				continue;
			char *param_val = *((char **)value);
			if (NULL == param_val) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompInstantiateCmdArgv Fail, NULL arg");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiateTimeout")) {
			if (value_is_deleted == true)
				continue;
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompInstantiateTimeout Fail, Zero Timeout");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiationLevel")) {
			if (value_is_deleted == true)
				continue;
			uint32_t num_inst = *((SaUint32T *)value);
			if (num_inst == 0) {
				report_ccb_validation_error(opdata, "Modification of saAmfCompInstantiationLevel Fail,"
						" Zero InstantiationLevel");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxInstantiateWithoutDelay")) {
			if (value_is_deleted == true)
				continue;
			uint32_t num_inst = *((SaUint32T *)value);
			if (num_inst == 0) {
				report_ccb_validation_error(opdata, "Modification of saAmfCompNumMaxInstantiateWithoutDelay"
						" Fail, Zero withoutDelay");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxInstantiateWithDelay")) {
			if (value_is_deleted == true)
				continue;
			uint32_t num_inst = *((SaUint32T *)value);
			if (num_inst == 0) {
				report_ccb_validation_error(opdata, "Modification of saAmfCompNumMaxInstantiateWithDelay"
						" Fail, Zero withDelay");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompDelayBetweenInstantiateAttempts")) {
			if (value_is_deleted == true)
				continue;
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				report_ccb_validation_error(opdata, "Modification of "
						"saAmfCompDelayBetweenInstantiateAttempts Fail, Zero Delay");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompTerminateCmdArgv")) {
			if (value_is_deleted == true)
				continue;
			char *param_val = *((char **)value);
			if (NULL == param_val) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompTerminateCmdArgv Fail, NULL arg");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompTerminateTimeout")) {
			if (value_is_deleted == true)
				continue;
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompTerminateTimeout Fail, Zero timeout");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompCleanupCmdArgv")) {
			if (value_is_deleted == true)
				continue;
			char *param_val = *((char **)value);
			if (NULL == param_val) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompCleanupCmdArgv Fail, NULL arg");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompCleanupTimeout")) {
			if (value_is_deleted == true)
				continue;
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompCleanupTimeout Fail, Zero Timeout");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompAmStartCmdArgv")) {
			if (value_is_deleted == true)
				continue;
			char *param_val = *((char **)value);
			if (NULL == param_val) {
				report_ccb_validation_error(opdata, 
						"Modification of saAmfCompAmStartCmdArgv Fail, NULL arg");
				goto done;
			}
			if (true == comp->su->su_is_external) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompAmStartCmdArgv Fail, Comp su_is_external");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompAmStartTimeout")) {
			if (value_is_deleted == true)
				continue;
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompAmStartTimeout Fail, Zero Timeout");
				goto done;
			}
			if (true == comp->su->su_is_external) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompAmStartTimeout Fail, Comp su_is_external");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxAmStartAttempt")) {
			if (value_is_deleted == true)
				continue;
			uint32_t num_am_start = *((SaUint32T *)value);
			if (true == comp->su->su_is_external) {
				report_ccb_validation_error(opdata, "Modification of saAmfCompNumMaxAmStartAttempt Fail,"
						" Comp su_is_external");
				goto done;
			}
			if (num_am_start == 0) {
				report_ccb_validation_error(opdata, "Modification of saAmfCompNumMaxAmStartAttempt Fail,"
						" Zero num_am_start");
				goto done;
			} 
		} else if (!strcmp(attribute->attrName, "saAmfCompAmStopCmdArgv")) {
			if (value_is_deleted == true)
				continue;
			char *param_val = *((char **)value);
			if (true == comp->su->su_is_external) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompAmStopCmdArgv Fail, Comp su_is_external");
				goto done;
			}
			if (NULL == param_val) {
				report_ccb_validation_error(opdata, "Modification of saAmfCompAmStopCmdArgv Fail, NULL arg");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompAmStopTimeout")) {
			if (value_is_deleted == true)
				continue;
			SaTimeT timeout;
			if (true == comp->su->su_is_external) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompAmStopTimeout Fail, Comp su_is_external");
				goto done;
			}
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompAmStopTimeout Fail, Zero Timeout");
				goto done;
			}		
		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxAmStopAttempt")) {	
			if (value_is_deleted == true)
				continue;
			uint32_t num_am_stop = *((SaUint32T *)value);
			if (true == comp->su->su_is_external) {
				report_ccb_validation_error(opdata, "Modification of saAmfCompNumMaxAmStopAttempt Fail,"
						" Comp su_is_external");
				goto done;
			}
			if (num_am_stop == 0) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompNumMaxAmStopAttempt Fail, Zero num_am_stop");
				goto done;
			}		
		} else if (!strcmp(attribute->attrName, "saAmfCompCSISetCallbackTimeout")) {
			if (value_is_deleted == true)
				continue;
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompCSISetCallbackTimeout Fail, Zero Timeout");
				goto done;
			}		
		} else if (!strcmp(attribute->attrName, "saAmfCompCSIRmvCallbackTimeout")) {
			if (value_is_deleted == true)
				continue;
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompCSIRmvCallbackTimeout Fail, Zero Timeout");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompQuiescingCompleteTimeout")) {
			if (value_is_deleted == true)
				continue;
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompQuiescingCompleteTimeout Fail, Zero Timeout");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompRecoveryOnError")) {
			if (value_is_deleted == true)
				continue;
			uint32_t recovery = *((SaUint32T *)value);
			if ((recovery < SA_AMF_NO_RECOMMENDATION) || (recovery > SA_AMF_CONTAINER_RESTART )) {
				report_ccb_validation_error(opdata, "Modification of saAmfCompRecoveryOnError Fail,"
						" Invalid recovery =%d",recovery);
				goto done;
			} 
		} else if (!strcmp(attribute->attrName, "saAmfCompDisableRestart")) {
			if (value_is_deleted == true)
				continue;
			SaBoolT val = *((SaBoolT *)value);
			if ((val != SA_TRUE) && (val != SA_FALSE)) {
				report_ccb_validation_error(opdata,
						"Modification of saAmfCompDisableRestart Fail, Invalid Input %d",val);
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompProxyCsi")) {
			if (value_is_deleted == true)
				continue;
			SaNameT name;
			name = *((SaNameT *)value);
			if (name.length == 0) {
				report_ccb_validation_error(opdata, "Modification of saAmfCompProxyCsi Fail, Length Zero");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompContainerCsi")) {
			if (value_is_deleted == true)
				continue;
			SaNameT name;
			name = *((SaNameT *)value);
			if (name.length == 0) {
				report_ccb_validation_error(opdata, 
						"Modification of saAmfCompContainerCsi Fail, Length Zero");
				goto done;
			}
		} else {
			report_ccb_validation_error(opdata, "Modification of attribute '%s' not supported", 
					attribute->attrName);
			goto done;
		}
	}

	rc = SA_AIS_OK;

done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT comp_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(&opdata->objectName, opdata->param.create.attrValues, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		rc = ccb_completed_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		rc = SA_AIS_OK;
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

static void comp_ccb_apply_modify_hdlr(struct CcbUtilOperationData *opdata)
{
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;
	AVD_COMP *comp;
	bool node_present = false;
	AVD_AVND *su_node_ptr = NULL;
	AVD_COMP_TYPE *comp_type;
	AVSV_PARAM_INFO param;
	bool value_is_deleted;

	TRACE_ENTER();

	memset(((uint8_t *)&param), '\0', sizeof(AVSV_PARAM_INFO));
	param.class_id = AVSV_SA_AMF_COMP;
	param.act = AVSV_OBJ_OPR_MOD;

	comp = comp_db->find(Amf::to_string(&opdata->objectName));
	param.name = comp->comp_info.name;
	comp_type = comptype_db->find(Amf::to_string(&comp->saAmfCompType));

	su_node_ptr = comp->su->get_node_ptr();

	if ((su_node_ptr->node_state == AVD_AVND_STATE_PRESENT) ||
	    (su_node_ptr->node_state == AVD_AVND_STATE_NO_CONFIG) ||
	    (su_node_ptr->node_state == AVD_AVND_STATE_NCS_INIT)) {
		node_present = true;
	}

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
		const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;
		void *value = NULL;

		if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) ||	(attribute->attrValues == NULL)) {
			/* Attribute value is deleted, revert to default value */
			value_is_deleted = true;
		} else {
			/* Attribute value is modified */
			value_is_deleted = false;
			value = attribute->attrValues[0];
		}

		TRACE("modType %u, %s, %u", attr_mod->modType, attribute->attrName, attribute->attrValuesNumber);

		if (!strcmp(attribute->attrName, "saAmfCompType")) {

			SaNameT *dn = (SaNameT*) value;
			const std::string oldType(Amf::to_string(&comp->saAmfCompType));
			if (oldType.compare(Amf::to_string(dn)) == 0) {
				// ignore 'change' if it's being set to the same value
				TRACE("saAmfCompType '%s' unchanged for '%s'", comp->saAmfCompType.value,
					opdata->objectName.value);
				continue;
			}

			TRACE("saAmfCompType modified from '%s' to '%s' for '%s'", comp->saAmfCompType.value, dn->value,
					opdata->objectName.value);
			avd_comptype_remove_comp(comp);
			comp->saAmfCompType = *dn;
			comp->comp_type = comptype_db->find(Amf::to_string(dn));
			avd_comptype_add_comp(comp);
			param.attr_id = saAmfCompType_ID;
			param.name_sec = *dn;

		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiateCmdArgv")) {

			/* Node director will reread configuration from IMM */
			param.attr_id = saAmfCompInstantiateCmd_ID;
			TRACE("saAmfCompInstantiateCmdArgv modified.");

		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiateTimeout")) {

			SaTimeT timeout;
			SaTimeT temp_timeout;

			if (value_is_deleted)
				value = &comp_type->saAmfCtDefClcCliTimeout;

			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompInstantiateTimeout_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			TRACE("saAmfCompInstantiationLevel modified from '%llu' to '%llu' for '%s'",
					comp->comp_info.init_time, *((SaTimeT *)value), comp->comp_info.name.value);
			comp->comp_info.init_time = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiationLevel")) {

			AVD_SU *su = comp->su;

			if (value_is_deleted)
				value = &comp_type->saAmfCtDefInstantiationLevel;

			param.attr_id = saAmfCompInstantiationLevel_ID;
			param.value_len = sizeof(uint32_t);
			memcpy(&param.value[0],(SaUint32T *)value , param.value_len);
			TRACE("saAmfCompInstantiationLevel modified from '%u' to '%u' for '%s'",
					comp->comp_info.inst_level, *((SaUint32T *)value), comp->comp_info.name.value);
			comp->comp_info.inst_level = *((SaUint32T *)value);
			
			su->remove_comp(comp);
			comp->su = su;
			su->add_comp(comp);
			
		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxInstantiateWithoutDelay")) {

			uint32_t num_inst;

			if (value_is_deleted)
				value = &avd_comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay;

			num_inst = *((SaUint32T *)value);
			param.attr_id = saAmfCompNumMaxInstantiate_ID;
			param.value_len = sizeof(uint32_t);
			num_inst = htonl(num_inst);
			memcpy(&param.value[0], &num_inst, param.value_len);
			TRACE("saAmfCompNumMaxInstantiateWithoutDelay modified from '%u' to '%u' for '%s'",
					comp->comp_info.max_num_inst, *((SaUint32T *)value), 
					comp->comp_info.name.value);
			comp->comp_info.max_num_inst = *((SaUint32T *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxInstantiateWithDelay")) {

			uint32_t num_inst;

			if (value_is_deleted)
				value = &avd_comp_global_attrs.saAmfNumMaxInstantiateWithDelay;
			
			num_inst = *((SaUint32T *)value);
			param.attr_id = saAmfCompNumMaxInstantiateWithDelay_ID;
			param.value_len = sizeof(uint32_t);
			num_inst = htonl(num_inst);
			memcpy(&param.value[0], &num_inst, param.value_len);
			TRACE("saAmfCompNumMaxInstantiateWithDelay modified from '%u' to '%u' for '%s'",
					comp->max_num_inst_delay, *((SaUint32T *)value), comp->comp_info.name.value);
			comp->max_num_inst_delay = *((SaUint32T *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompDelayBetweenInstantiateAttempts")) {

			SaTimeT timeout;
			SaTimeT temp_timeout;

			if (value_is_deleted)
				value = &avd_comp_global_attrs.saAmfDelayBetweenInstantiateAttempts;

			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompDelayBetweenInstantiateAttempts_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			TRACE("saAmfCompDelayBetweenInstantiateAttempts modified from '%llu' to '%llu' for '%s'",
					comp->inst_retry_delay, *((SaTimeT *)value), comp->comp_info.name.value);
			comp->inst_retry_delay = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompTerminateCmdArgv")) {

			/* Node director will refresh from IMM */
			param.attr_id = saAmfCompTerminateCmd_ID;
			TRACE("saAmfCompTerminateCmdArgv modified.");

		} else if (!strcmp(attribute->attrName, "saAmfCompTerminateTimeout")) {

			SaTimeT timeout;
			SaTimeT temp_timeout;

			if (value_is_deleted)
				value = &comp_type->saAmfCtDefCallbackTimeout;

			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompTerminateTimeout_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			TRACE("saAmfCompTerminateTimeout modified from '%llu' to '%llu' for '%s'",
					comp->comp_info.term_time, *((SaTimeT *)value), comp->comp_info.name.value);
			comp->comp_info.term_time = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompCleanupCmdArgv")) {

			/* Node director will reread configuration from IMM */
			param.attr_id = saAmfCompCleanupCmd_ID;
			TRACE("saAmfCompCleanupCmdArgv modified.");

		} else if (!strcmp(attribute->attrName, "saAmfCompCleanupTimeout")) {
			SaTimeT timeout;
			SaTimeT temp_timeout;

			if (value_is_deleted)
				value = &comp_type->saAmfCtDefClcCliTimeout;

			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompCleanupTimeout_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			TRACE("saAmfCompCleanupTimeout modified from '%llu' to '%llu' for '%s'",
					comp->comp_info.clean_time, *((SaTimeT *)value), comp->comp_info.name.value);
			comp->comp_info.clean_time = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompAmStartCmdArgv")) {

			/* Node director will reread configuration from IMM */
			param.attr_id = saAmfCompAmStartCmd_ID;
			TRACE("saAmfCompAmStartCmdArgv modified.");

		} else if (!strcmp(attribute->attrName, "saAmfCompAmStartTimeout")) {

			SaTimeT timeout;
			SaTimeT temp_timeout;

			if (value_is_deleted)
				value = &comp_type->saAmfCtDefClcCliTimeout;

			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompAmStartTimeout_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			TRACE("saAmfCompAmStartTimeout modified from '%llu' to '%llu' for '%s'",
					comp->comp_info.amstart_time, *((SaTimeT *)value), comp->comp_info.name.value);
			comp->comp_info.amstart_time = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxAmStartAttempt")) {

			uint32_t num_am_start;

			if (value_is_deleted)
				value = &avd_comp_global_attrs.saAmfNumMaxAmStartAttempts;

			num_am_start = *((SaUint32T *)value);
			param.attr_id = saAmfCompNumMaxAmStartAttempts_ID;
			param.value_len = sizeof(uint32_t);
			num_am_start = htonl(num_am_start);
			memcpy(&param.value[0], &num_am_start, param.value_len);
			TRACE("saAmfCompNumMaxAmStartAttempt modified from '%u' to '%u' for '%s'",
					comp->comp_info.max_num_amstart, *((SaUint32T *)value), 
					comp->comp_info.name.value);
			comp->comp_info.max_num_amstart = *((SaUint32T *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompAmStopCmdArgv")) {

			/* Node director will reread configuration from IMM */
			param.attr_id = saAmfCompAmStopCmd_ID;
			TRACE("saAmfCompAmStopCmdArgv modified.");

		} else if (!strcmp(attribute->attrName, "saAmfCompAmStopTimeout")) {

			SaTimeT timeout;
			SaTimeT temp_timeout;

			if (value_is_deleted)
				value = &comp_type->saAmfCtDefClcCliTimeout;

			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompAmStopTimeout_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			TRACE("saAmfCompAmStopTimeout modified from '%llu' to '%llu' for '%s'",
					comp->comp_info.amstop_time, *((SaTimeT *)value), comp->comp_info.name.value);
			comp->comp_info.amstop_time = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxAmStopAttempt")) {

			uint32_t num_am_stop;

			if (value_is_deleted)
				value = &avd_comp_global_attrs.saAmfNumMaxAmStopAttempts;

			num_am_stop = *((SaUint32T *)value);
			param.attr_id = saAmfCompNumMaxAmStopAttempts_ID;
			param.value_len = sizeof(uint32_t);
			num_am_stop = htonl(num_am_stop);
			memcpy(&param.value[0], &num_am_stop, param.value_len);
			TRACE("saAmfCompNumMaxAmStopAttempt modified from '%u' to '%u' for '%s'",
					comp->comp_info.max_num_amstop, *((SaUint32T *)value), 
					comp->comp_info.name.value);
			comp->comp_info.max_num_amstop = *((SaUint32T *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompCSISetCallbackTimeout")) {

			SaTimeT timeout;
			SaTimeT temp_timeout;

			if (value_is_deleted)
				value = &comp_type->saAmfCtDefCallbackTimeout;

			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompCSISetCallbackTimeout_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			TRACE("saAmfCompCSISetCallbackTimeout modified from '%llu' to '%llu' for '%s'",
					comp->comp_info.csi_set_callback_timeout, *((SaTimeT *)value), 
					comp->comp_info.name.value);
			comp->comp_info.csi_set_callback_timeout = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompCSIRmvCallbackTimeout")) {

			SaTimeT timeout;
			SaTimeT temp_timeout;

			if (value_is_deleted)
				value = &comp_type->saAmfCtDefCallbackTimeout;

			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompCSIRmvCallbackTimeout_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			TRACE("saAmfCompCSIRmvCallbackTimeout modified from '%llu' to '%llu' for '%s'",
					comp->comp_info.csi_rmv_callback_timeout, *((SaTimeT *)value), 
					comp->comp_info.name.value);
			comp->comp_info.csi_rmv_callback_timeout = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompQuiescingCompleteTimeout")) {

			SaTimeT timeout;
			SaTimeT temp_timeout;

			if (value_is_deleted)
				value = &comp_type->saAmfCtDefQuiescingCompleteTimeout;

			timeout = *((SaTimeT *)value);
			m_NCS_OS_HTONLL_P(&temp_timeout, timeout);

			param.attr_id = saAmfCompQuiescingCompleteTimeout_ID;
			param.value_len = sizeof(SaTimeT);
			memcpy(&param.value[0], &temp_timeout, param.value_len);
			TRACE("saAmfCompQuiescingCompleteTimeout modified from '%llu' to '%llu' for '%s'",
					comp->comp_info.quiescing_complete_timeout, *((SaTimeT *)value), 
					comp->comp_info.name.value);
			comp->comp_info.quiescing_complete_timeout = *((SaTimeT *)value);
		} else if (!strcmp(attribute->attrName, "saAmfCompRecoveryOnError")) {
			uint32_t recovery;

			if (value_is_deleted)
				value = &comp_type->saAmfCtDefRecoveryOnError;

			recovery = *((SaUint32T *)value);
			if (recovery == SA_AMF_NO_RECOMMENDATION)
				recovery = SA_AMF_COMPONENT_FAILOVER;
			param.attr_id = saAmfCompRecoveryOnError_ID;
			param.value_len = sizeof(uint32_t);
			recovery = htonl(recovery);
			memcpy(&param.value[0], &recovery, param.value_len);
			TRACE("saAmfCompRecoveryOnError modified from '%u' to '%u' for '%s'",
					comp->comp_info.def_recvr, *((SaUint32T *)value), comp->comp_info.name.value);
			comp->comp_info.def_recvr = static_cast<SaAmfRecommendedRecoveryT>(*((SaUint32T *)value));
			if (comp->comp_info.def_recvr == SA_AMF_NO_RECOMMENDATION) {
				comp->comp_info.def_recvr = SA_AMF_COMPONENT_FAILOVER;
				LOG_NO("COMPONENT_FAILOVER(%u) used instead of NO_RECOMMENDATION(%u) for '%s'",
					   SA_AMF_COMPONENT_FAILOVER, SA_AMF_NO_RECOMMENDATION, comp->comp_info.name.value);
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompDisableRestart")) {
			if (value_is_deleted) {
				TRACE("saAmfCompDisableRestart modified from '%u' to '%u' for '%s'",
						comp->comp_info.comp_restart, comp_type->saAmfCtDefDisableRestart,
						comp->comp_info.name.value);
				comp->comp_info.comp_restart = comp_type->saAmfCtDefDisableRestart;
			} else {
				TRACE("saAmfCompDisableRestart modified from '%u' to '%u' for '%s'",
						comp->comp_info.comp_restart, *((SaUint32T *)value),
						comp->comp_info.name.value);
				comp->comp_info.comp_restart = *((SaUint32T *)value);
			}

			param.attr_id = saAmfCompDisableRestart_ID;
			param.value_len = sizeof(uint32_t);
			uint32_t restart = htonl(comp->comp_info.comp_restart);
			memcpy(&param.value[0], &restart, param.value_len);

		} else if (!strcmp(attribute->attrName, "saAmfCompProxyCsi")) {
			if (value_is_deleted)
				memset(&comp->comp_proxy_csi, 0, sizeof(comp->comp_proxy_csi));
			else
				comp->comp_proxy_csi = *((SaNameT *)value);
		} else if (!strcmp(attribute->attrName, "saAmfCompContainerCsi")) {
			if (value_is_deleted)
				memset(&comp->comp_proxy_csi, 0, sizeof(comp->comp_container_csi));
			else
				comp->comp_container_csi = *((SaNameT *)value);
		} else {
			osafassert(0);
		}

		if (true == node_present)
			avd_snd_op_req_msg(avd_cb, su_node_ptr, &param);
	}

	TRACE_LEAVE();
}

void comp_ccb_apply_delete_hdlr(struct CcbUtilOperationData *opdata)
{
	TRACE_ENTER();

	AVD_COMP *comp = comp_db->find(Amf::to_string(&opdata->objectName));
	/* comp should be found in the database even if it was 
	 * due to parent su delete the changes are applied in 
	 * bottom up order so all the component deletes are applied 
	 * first and then parent SU delete is applied
	 * just doing sanity check here
	 **/
	osafassert(comp != NULL);

	// send message to ND requesting delete of the component
	AVD_AVND *su_node_ptr = comp->su->get_node_ptr();
	if ((su_node_ptr->node_state == AVD_AVND_STATE_PRESENT) ||
	    (su_node_ptr->node_state == AVD_AVND_STATE_NO_CONFIG) ||
	    (su_node_ptr->node_state == AVD_AVND_STATE_NCS_INIT)) {
		AVSV_PARAM_INFO param;
		memset(((uint8_t *)&param), '\0', sizeof(AVSV_PARAM_INFO));
		param.act = AVSV_OBJ_OPR_DEL;
		param.name = comp->comp_info.name;
		param.class_id = AVSV_SA_AMF_COMP;
		avd_snd_op_req_msg(avd_cb, su_node_ptr, &param);
	}

	avd_comp_delete(comp);

	TRACE_LEAVE();
}

static void comp_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_COMP *comp;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		comp = comp_create(&opdata->objectName, opdata->param.create.attrValues);
		osafassert(comp);
		comp_add_to_model(comp);
		break;
	case CCBUTIL_MODIFY:
		comp_ccb_apply_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		comp_ccb_apply_delete_hdlr(opdata);
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE();
}

/**
 * Return an Comp object if it exist, otherwise create it and
 * return a reference to the new object.
 * @param dn
 *
 * @return AVD_COMP*
 */
AVD_COMP *avd_comp_get_or_create(const SaNameT *dn)
{
	AVD_COMP *comp = comp_db->find(Amf::to_string(dn));

	if (!comp) {
		TRACE("'%s' does not exist, creating it", dn->value);
		comp = avd_comp_new(dn);
		osafassert(comp != NULL);
		avd_comp_db_add(comp);
	}

	return comp;
}

void avd_comp_constructor(void)
{

	comp_db = new AmfDb<std::string, AVD_COMP>;
	avd_class_impl_set("SaAmfComp", comp_rt_attr_cb, comp_admin_op_cb,
		comp_ccb_completed_cb, comp_ccb_apply_cb);
}

/**
 * Returns true if the component is pre-instantiable
 * @param comp
 * @return
 */
bool comp_is_preinstantiable(const AVD_COMP *comp) {
	AVSV_COMP_TYPE_VAL category = comp->comp_info.category;
	return ((category == AVSV_COMP_TYPE_SA_AWARE) ||
			(category == AVSV_COMP_TYPE_PROXIED_LOCAL_PRE_INSTANTIABLE) ||
			(category == AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE));
}

/**
 * @brief  Returns true if the component is assigned any CSI. 
	   Note:comp->assign_flag is not always reliable to 
 *         check if a component is assigned or not as this flag 
           is always reset before going for new assignments. 
 * @param comp.
 * @return true/false.
 */

bool is_comp_assigned_any_csi(AVD_COMP *comp)
{
	for (const auto& si : comp->su->sg_of_su->list_of_si) {
		for (AVD_CSI *csi = si->list_of_csi; csi; csi = csi->si_list_of_csi_next) {
			for (AVD_COMP_CSI_REL *compcsi = csi->list_compcsi; compcsi; compcsi = compcsi->csi_csicomp_next) {
				if (compcsi->comp == comp)
					return true;
			}
		}
	}
	return false;

}

/**
 * @brief  Verify if component is undergoing any admin operation.
 * @param  ptr to component(AVD_COMP).
 * @Return SA_AIS_OK/SA_AIS_ERR_TRY_AGAIN.
 */
SaAisErrorT check_comp_stability(const AVD_COMP *comp)
{
        if (comp->admin_pend_cbk.invocation != 0) {
                LOG_NO("Component undergoing admin operation '%s'", comp->comp_info.name.value);
                return SA_AIS_ERR_TRY_AGAIN;
        }
        return SA_AIS_OK;
}
/**
 * @brief  CHeck if component is SA_AWARE. 
 * @Return true/false.
 */
bool AVD_COMP::saaware() const
{
        AVD_COMP_TYPE *comptype = comptype_db->find(Amf::to_string(&saAmfCompType));
        return (IS_COMP_SAAWARE(comptype->saAmfCtCompCategory));
}

