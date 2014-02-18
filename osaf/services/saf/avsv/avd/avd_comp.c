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
#include <avsv_util.h>
#include <avd_util.h>
#include <avd_comp.h>
#include <avd_imm.h>
#include <avd_node.h>
#include <avd_csi.h>
#include <avd_proc.h>
#include <avd_ckpt_msg.h>

static NCS_PATRICIA_TREE comp_db;

void avd_comp_db_add(AVD_COMP *comp)
{
	unsigned int rc;

	if (avd_comp_get(&comp->comp_info.name) == NULL) {
		rc = ncs_patricia_tree_add(&comp_db, &comp->tree_node);
		osafassert(rc == NCSCC_RC_SUCCESS);
	}
}

AVD_COMP *avd_comp_new(const SaNameT *dn)
{
	AVD_COMP *comp;

	if ((comp = calloc(1, sizeof(AVD_COMP))) == NULL) {
		LOG_ER("calloc FAILED");
		return NULL;
	}
	
	memcpy(comp->comp_info.name.value, dn->value, dn->length);
	comp->comp_info.name.length = dn->length;
	comp->tree_node.key_info = (uint8_t *)&(comp->comp_info.name);
	comp->comp_info.cap = SA_AMF_COMP_ONE_ACTIVE_OR_ONE_STANDBY;
	comp->comp_info.category = AVSV_COMP_TYPE_NON_SAF;
	comp->comp_info.def_recvr = SA_AMF_COMPONENT_RESTART;
	comp->comp_info.inst_level = 1;
	comp->comp_info.comp_restart = true;
	comp->nodefail_cleanfail = false;
	comp->saAmfCompOperState = SA_AMF_OPERATIONAL_DISABLED;
	comp->saAmfCompReadinessState = SA_AMF_READINESS_OUT_OF_SERVICE;
	comp->saAmfCompPresenceState = SA_AMF_PRESENCE_UNINSTANTIATED;

	return comp;
}

void avd_comp_pres_state_set(AVD_COMP *comp, SaAmfPresenceStateT pres_state)
{
	osafassert(pres_state <= SA_AMF_PRESENCE_TERMINATION_FAILED);
	TRACE_ENTER2("'%s' %s => %s",
		comp->comp_info.name.value, avd_pres_state_name[comp->saAmfCompPresenceState],
		avd_pres_state_name[pres_state]);

	comp->saAmfCompPresenceState = pres_state;
	avd_saImmOiRtObjectUpdate(&comp->comp_info.name,
		"saAmfCompPresenceState", SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompPresenceState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, comp, AVSV_CKPT_COMP_PRES_STATE);

	/* alarm & notifications */
	if(comp->saAmfCompPresenceState == SA_AMF_PRESENCE_INSTANTIATION_FAILED)
		avd_send_comp_inst_failed_alarm(&comp->comp_info.name, &comp->su->su_on_node->name);
	else if(comp->saAmfCompPresenceState ==  SA_AMF_PRESENCE_TERMINATION_FAILED)
		avd_send_comp_clean_failed_alarm(&comp->comp_info.name, &comp->su->su_on_node->name);
}

void avd_comp_oper_state_set(AVD_COMP *comp, SaAmfOperationalStateT oper_state)
{
	osafassert(oper_state <= SA_AMF_OPERATIONAL_DISABLED);
	TRACE_ENTER2("'%s' %s => %s",
		comp->comp_info.name.value, avd_oper_state_name[comp->saAmfCompOperState], avd_oper_state_name[oper_state]);

	comp->saAmfCompOperState = oper_state;
	avd_saImmOiRtObjectUpdate(&comp->comp_info.name,
		"saAmfCompOperState", SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompOperState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, comp, AVSV_CKPT_COMP_OPER_STATE);
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
	avd_saImmOiRtObjectUpdate(&comp->comp_info.name,
		 "saAmfCompReadinessState", SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompReadinessState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, comp, AVSV_CKPT_COMP_READINESS_STATE);
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

AVD_COMP *avd_comp_get(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_COMP *)ncs_patricia_tree_get(&comp_db, (uint8_t *)&tmp);
}

AVD_COMP *avd_comp_getnext(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_COMP *)ncs_patricia_tree_getnext(&comp_db, (uint8_t *)&tmp);
}

void avd_comp_delete(AVD_COMP *comp)
{
	m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, comp, AVSV_CKPT_AVD_COMP_CONFIG);
	avd_su_remove_comp(comp);
	avd_comptype_remove_comp(comp);
	(void)ncs_patricia_tree_del(&comp_db, &comp->tree_node);
	free(comp);
}

/**
 * Add comp to model
 * @param comp
 */
static void comp_add_to_model(AVD_COMP *comp)
{
	SaNameT dn;

	TRACE_ENTER2("%s", comp->comp_info.name.value);

	/* Check parent link to see if it has been added already */
	if (comp->su != NULL) {
		TRACE("already added");
		goto done;
	}

	avsv_sanamet_init(&comp->comp_info.name, &dn, "safSu");
	comp->su = avd_su_get(&dn);

	avd_comp_db_add(comp);
	comp->comp_type = avd_comptype_get(&comp->saAmfCompType);
	osafassert(comp->comp_type);
	avd_comptype_add_comp(comp);
	avd_su_add_comp(comp);

	/* check if the
	 * corresponding node is UP send the component information
	 * to the Node.
	 */
	if (comp->su->su_is_external) {
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
			avd_su_remove_comp(comp);
			LOG_ER("Not A Valid External Component: Category '%u'configured",
				comp->comp_info.category);
			return;
		}
	}

	/* Verify if the SUs preinstan value need to be changed */
	if ((comp->comp_info.category == AVSV_COMP_TYPE_SA_AWARE) ||
	    (comp->comp_info.category == AVSV_COMP_TYPE_PROXIED_LOCAL_PRE_INSTANTIABLE) ||
	    (comp->comp_info.category == AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE)) {
		comp->su->saAmfSUPreInstantiable = true;
	}

	if ((comp->su->su_on_node->node_state == AVD_AVND_STATE_PRESENT) ||
			(comp->su->su_on_node->node_state == AVD_AVND_STATE_NO_CONFIG) ||
			(comp->su->su_on_node->node_state == AVD_AVND_STATE_NCS_INIT))
		avd_comp_oper_state_set(comp, SA_AMF_OPERATIONAL_ENABLED);

	/* Set runtime cached attributes. */
	avd_saImmOiRtObjectUpdate(&comp->su->name, "saAmfSUPreInstantiable",
		SA_IMM_ATTR_SAUINT32T, &comp->su->saAmfSUPreInstantiable);

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
		LOG_ER("No parent to '%s' ", dn->value);
		return 0;
	}

	if (strncmp(++parent, "safSu=", 6) != 0) {
		LOG_ER("Wrong parent '%s' to '%s' ", parent, dn->value);
		return 0;
	}

	rc = immutil_getAttr("saAmfCompType", attributes, 0, &aname);
	osafassert(rc == SA_AIS_OK);

	if (avd_comptype_get(&aname) == NULL) {
		/* Comp type does not exist in current model, check CCB */
		if (opdata == NULL) {
			LOG_ER("'%s' does not exist in model", aname.value);
			return 0;
		}

		if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &aname) == NULL) {
			LOG_ER("'%s' does not exist in existing model or in CCB", aname.value);
			return 0;
		}
	}

	rc = immutil_getAttr("saAmfCompRecoveryOnError", attributes, 0, &value);
	if (rc == SA_AIS_OK) {
		if ((value < SA_AMF_NO_RECOMMENDATION) || (value > SA_AMF_NODE_FAILFAST)) {
			LOG_ER("Illegal/unsupported saAmfCompRecoveryOnError value %u for '%s'",
				   value, dn->value);
			return 0;
		}

		if (value == SA_AMF_NO_RECOMMENDATION)
			LOG_NO("Invalid configuration, saAmfCompRecoveryOnError=NO_RECOMMENDATION(%u) for '%s'",
				   value, dn->value);
	}

	rc = immutil_getAttr("saAmfCompDisableRestart", attributes, 0, &value);
	if ((rc == SA_AIS_OK) && (value > SA_TRUE)) {
		LOG_ER("Illegal saAmfCompDisableRestart value %u for '%s'",
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
	if (NULL == (comp = avd_comp_get(dn))) {
		if ((comp = avd_comp_new(dn)) == NULL)
			goto done;
	}
	else
		TRACE("already created, refreshing config...");

	error = immutil_getAttr("saAmfCompType", attributes, 0, &comp->saAmfCompType);
	osafassert(error == SA_AIS_OK);

	if ((comptype = avd_comptype_get(&comp->saAmfCompType)) == NULL) {
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

	if (immutil_getAttr("saAmfCompInstantiateTimeout", attributes, 0, &comp->comp_info.init_time) != SA_AIS_OK)
		comp->comp_info.init_time = comptype->saAmfCtDefClcCliTimeout;

	if (immutil_getAttr("saAmfCompInstantiationLevel", attributes, 0, &comp->comp_info.inst_level) != SA_AIS_OK)
		comp->comp_info.inst_level = comptype->saAmfCtDefInstantiationLevel;

	if (immutil_getAttr("saAmfCompNumMaxInstantiateWithoutDelay", attributes,
			    0, &comp->comp_info.max_num_inst) != SA_AIS_OK)
		comp->comp_info.max_num_inst = avd_comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay;

	/*  TODO what is implemented? With or without delay? */

#if 0
	if (immutil_getAttr("saAmfCompNumMaxInstantiateWithDelay", attributes,
			    0, &comp->max_num_inst_delay) != SA_AIS_OK)
		comp->comp_info.max_num_inst = avd_comp_global_attrs.saAmfNumMaxInstantiateWithDelay;
#endif

	if (immutil_getAttr("saAmfCompDelayBetweenInstantiateAttempts", attributes,
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

	if (immutil_getAttr("saAmfCompTerminateTimeout", attributes,
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

	if (immutil_getAttr("saAmfCompCleanupTimeout", attributes, 0, &comp->comp_info.clean_time) != SA_AIS_OK)
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

	if (immutil_getAttr("saAmfCompAmStartTimeout", attributes, 0, &comp->comp_info.amstart_time) != SA_AIS_OK)
		comp->comp_info.amstart_time = comptype->saAmfCtDefClcCliTimeout;

	if (immutil_getAttr("saAmfCompNumMaxAmStartAttempts", attributes,
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

	if (immutil_getAttr("saAmfCompAmStopTimeout", attributes, 0, &comp->comp_info.amstop_time) != SA_AIS_OK)
		comp->comp_info.amstop_time = comptype->saAmfCtDefClcCliTimeout;

	if (immutil_getAttr("saAmfCompNumMaxAmStopAttempts", attributes,
			    0, &comp->comp_info.max_num_amstop) != SA_AIS_OK)
		comp->comp_info.max_num_amstop = avd_comp_global_attrs.saAmfNumMaxAmStopAttempts;

	if (immutil_getAttr("saAmfCompCSISetCallbackTimeout", attributes,
			    0, &comp->comp_info.csi_set_callback_timeout) != SA_AIS_OK)
		comp->comp_info.csi_set_callback_timeout = comptype->saAmfCtDefCallbackTimeout;

	if (immutil_getAttr("saAmfCompCSIRmvCallbackTimeout", attributes,
			    0, &comp->comp_info.csi_rmv_callback_timeout) != SA_AIS_OK)
		comp->comp_info.csi_rmv_callback_timeout = comptype->saAmfCtDefCallbackTimeout;

	if (immutil_getAttr("saAmfCompQuiescingCompleteTimeout", attributes,
			    0, &comp->comp_info.quiescing_complete_timeout) != SA_AIS_OK)
		comp->comp_info.quiescing_complete_timeout = comptype->saAmfCtDefQuiescingCompleteTimeout;

	if (immutil_getAttr("saAmfCompRecoveryOnError", attributes, 0, &comp->comp_info.def_recvr) != SA_AIS_OK)
		comp->comp_info.def_recvr = comptype->saAmfCtDefRecoveryOnError;

	if (comp->comp_info.def_recvr == SA_AMF_NO_RECOMMENDATION) {
		comp->comp_info.def_recvr = SA_AMF_COMPONENT_FAILOVER;
		LOG_NO("COMPONENT_FAILOVER(%u) used instead of NO_RECOMMENDATION(%u) for '%s'",
			   SA_AMF_COMPONENT_FAILOVER, SA_AMF_NO_RECOMMENDATION, comp->comp_info.name.value);
	}

	if (immutil_getAttr("saAmfCompDisableRestart", attributes, 0, &comp->comp_info.comp_restart) != SA_AIS_OK)
		comp->comp_info.comp_restart = comptype->saAmfCtDefDisableRestart;

	comp->max_num_csi_actv = -1;	// TODO
	comp->max_num_csi_stdby = -1;	// TODO

	rc = 0;
done:
	if (rc != 0) {
		avd_comp_delete(comp);
		comp = NULL;
	}

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
		"saAmfCompType",
		"saAmfCompCmdEnv",
		"saAmfCompInstantiateCmdArgv",
		"saAmfCompInstantiateTimeout",
		"saAmfCompInstantiationLevel",
		"saAmfCompNumMaxInstantiateWithoutDelay",
		"saAmfCompNumMaxInstantiateWithDelay",
		"saAmfCompDelayBetweenInstantiateAttempts",
		"saAmfCompTerminateCmdArgv",
		"saAmfCompTerminateTimeout",
		"saAmfCompCleanupCmdArgv",
		"saAmfCompCleanupTimeout",
		"saAmfCompAmStartCmdArgv",
		"saAmfCompAmStartTimeout",
		"saAmfCompNumMaxAmStartAttempts",
		"saAmfCompAmStopCmdArgv",
		"saAmfCompAmStopTimeout",
		"saAmfCompNumMaxAmStopAttempts",
		"saAmfCompCSISetCallbackTimeout",
		"saAmfCompCSIRmvCallbackTimeout",
		"saAmfCompQuiescingCompleteTimeout",
		"saAmfCompRecoveryOnError",
		"saAmfCompDisableRestart",
		NULL
	};

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
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
	SaAisErrorT rc = SA_AIS_OK;

        TRACE_ENTER2("%llu, '%s', %llu", invocation, objectName->value, opId);

	AVD_COMP *comp = avd_comp_get(objectName);
	osafassert(comp != NULL);

	switch (opId) {
		/* Valid B.04 AMF comp admin operations */
	case SA_AMF_ADMIN_RESTART:
		if (comp->comp_info.comp_restart == true) {
			LOG_WA("Component Restart disabled '%s'", objectName->value);
			rc = SA_AIS_ERR_NOT_SUPPORTED;
		}
		else if (comp->admin_pend_cbk.invocation != 0) {
			LOG_WA("Component undergoing admin operation '%s'", objectName->value);
			rc = SA_AIS_ERR_TRY_AGAIN;
		}
		else if (comp->su->pend_cbk.invocation != 0) {
			LOG_WA("SU undergoing admin operation '%s'", objectName->value);
			rc = SA_AIS_ERR_TRY_AGAIN;
		}
		else if (comp->su->su_on_node->admin_node_pend_cbk.invocation != 0) {
			LOG_WA("Node undergoing admin operation '%s'", objectName->value);
			rc = SA_AIS_ERR_TRY_AGAIN;
		}
		else if (comp->saAmfCompPresenceState != SA_AMF_PRESENCE_INSTANTIATED) {
			LOG_WA("Component not instantiated '%s'", objectName->value);
			rc = SA_AIS_ERR_BAD_OPERATION;
		}
		else {
			/* prepare the admin op req message and queue it */
			if (avd_admin_op_msg_snd(&comp->comp_info.name, AVSV_SA_AMF_COMP,
				opId, comp->su->su_on_node) == NCSCC_RC_SUCCESS) {
				comp->admin_pend_cbk.admin_oper = opId;
				comp->admin_pend_cbk.invocation = invocation;
				rc = SA_AIS_OK;
			}
			else {
				LOG_WA("Admin op request send failed '%s'", objectName->value);
				rc = SA_AIS_ERR_TIMEOUT;
			}
		}
		break;

	case SA_AMF_ADMIN_EAM_START:
	case SA_AMF_ADMIN_EAM_STOP:
	default:
		LOG_WA("Unsupported admin operation '%llu'", opId);
		rc = SA_AIS_ERR_NOT_SUPPORTED;
		break;
	}

	if (rc != SA_AIS_OK)
		avd_saImmOiAdminOperationResult(immOiHandle, invocation, rc);

	TRACE_LEAVE2("(%u)",rc);
}

static SaAisErrorT comp_rt_attr_cb(SaImmOiHandleT immOiHandle,
	const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	AVD_COMP *comp = avd_comp_get(objectName);
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
		} else
			osafassert(0);
	}

	return SA_AIS_OK;
}

static SaAisErrorT ccb_completed_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;
	AVD_COMP *comp;
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER();

	comp = avd_comp_get(&opdata->objectName);

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
		const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;
		void *value;

		/* Attribute value removed */
		if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || (attribute->attrValues == NULL))
			continue;

		value = attribute->attrValues[0];

		if (!strcmp(attribute->attrName, "saAmfCompType")) {
			SaNameT dn = *((SaNameT*)value);
			if (NULL == avd_comptype_get(&dn)) {
				LOG_ER("saAmfCompType '%s' not found", dn.value);
				goto done;
			}

		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiateCmdArgv")) {
			char *param_val = *((char **)value);
			if (NULL == param_val) {
				LOG_ER("Modification of saAmfCompInstantiateCmdArgv Fail, NULL arg");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiateTimeout")) {
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompInstantiateTimeout Fail, Zero Timeout");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiationLevel")) {
			uint32_t num_inst = *((SaUint32T *)value);
			if (num_inst == 0) {
				LOG_ER("Modification of saAmfCompInstantiationLevel Fail, Zero InstantiationLevel");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxInstantiateWithoutDelay")) {
			uint32_t num_inst = *((SaUint32T *)value);
			if (num_inst == 0) {
				LOG_ER("Modification of saAmfCompNumMaxInstantiateWithoutDelay Fail, Zero withoutDelay");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxInstantiateWithDelay")) {
			uint32_t num_inst = *((SaUint32T *)value);
			if (num_inst == 0) {
				LOG_ER("Modification of saAmfCompNumMaxInstantiateWithDelay Fail, Zero withDelay");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompDelayBetweenInstantiateAttempts")) {
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompDelayBetweenInstantiateAttempts Fail, Zero Delay");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompTerminateCmdArgv")) {
			char *param_val = *((char **)value);
			if (NULL == param_val) {
				LOG_ER("Modification of saAmfCompTerminateCmdArgv Fail, NULL arg");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompTerminateTimeout")) {
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompTerminateTimeout Fail, Zero timeout");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompCleanupCmdArgv")) {
			char *param_val = *((char **)value);
			if (NULL == param_val) {
				LOG_ER("Modification of saAmfCompCleanupCmdArgv Fail, NULL arg");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompCleanupTimeout")) {
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompCleanupTimeout Fail, Zero Timeout");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompAmStartCmdArgv")) {
			char *param_val = *((char **)value);
			if (NULL == param_val) {
				LOG_ER("Modification of saAmfCompAmStartCmdArgv Fail, NULL arg");
				goto done;
			}
			if (true == comp->su->su_is_external) {
				LOG_ER("Modification of saAmfCompAmStartCmdArgv Fail, Comp su_is_external");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompAmStartTimeout")) {
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompAmStartTimeout Fail, Zero Timeout");
				goto done;
			}
			if (true == comp->su->su_is_external) {
				LOG_ER("Modification of saAmfCompAmStartTimeout Fail, Comp su_is_external");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxAmStartAttempt")) {
			uint32_t num_am_start = *((SaUint32T *)value);
			if (true == comp->su->su_is_external) {
				LOG_ER("Modification of saAmfCompNumMaxAmStartAttempt Fail, Comp su_is_external");
				goto done;
			}
			if (num_am_start == 0) {
				LOG_ER("Modification of saAmfCompNumMaxAmStartAttempt Fail, Zero num_am_start");
				goto done;
			} 
		} else if (!strcmp(attribute->attrName, "saAmfCompAmStopCmdArgv")) {
			char *param_val = *((char **)value);
			if (true == comp->su->su_is_external) {
				LOG_ER("Modification of saAmfCompAmStopCmdArgv Fail, Comp su_is_external");
				goto done;
			}
			if (NULL == param_val) {
				LOG_ER("Modification of saAmfCompAmStopCmdArgv Fail, NULL arg");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompAmStopTimeout")) {
			SaTimeT timeout;
			if (true == comp->su->su_is_external) {
				LOG_ER("Modification of saAmfCompAmStopTimeout Fail, Comp su_is_external");
				goto done;
			}
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompAmStopTimeout Fail, Zero Timeout");
				goto done;
			}		
		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxAmStopAttempt")) {	
			uint32_t num_am_stop = *((SaUint32T *)value);
			if (true == comp->su->su_is_external) {
				LOG_ER("Modification of saAmfCompNumMaxAmStopAttempt Fail, Comp su_is_external");
				goto done;
			}
			if (num_am_stop == 0) {
				LOG_ER("Modification of saAmfCompNumMaxAmStopAttempt Fail, Zero num_am_stop");
				goto done;
			}		
		} else if (!strcmp(attribute->attrName, "saAmfCompCSISetCallbackTimeout")) {
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompCSISetCallbackTimeout Fail, Zero Timeout");
				goto done;
			}		
		} else if (!strcmp(attribute->attrName, "saAmfCompCSIRmvCallbackTimeout")) {
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompCSIRmvCallbackTimeout Fail, Zero Timeout");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompQuiescingCompleteTimeout")) {
			SaTimeT timeout;
			m_NCS_OS_HTONLL_P(&timeout, (*((SaTimeT *)value)));
			if (timeout == 0) {
				LOG_ER("Modification of saAmfCompQuiescingCompleteTimeout Fail, Zero Timeout");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompRecoveryOnError")) {
			uint32_t recovery = *((SaUint32T *)value);
			if ((recovery < SA_AMF_NO_RECOMMENDATION) || (recovery > SA_AMF_CONTAINER_RESTART )) {
				LOG_ER("Modification of saAmfCompRecoveryOnError Fail, Invalid recovery =%d",recovery);
				goto done;
			} 
		} else if (!strcmp(attribute->attrName, "saAmfCompDisableRestart")) {
			SaBoolT val = *((SaBoolT *)value);
			if ((val != SA_TRUE) && (val != SA_FALSE)) {
				LOG_ER("Modification of saAmfCompDisableRestart Fail, Invalid Input %d",val);
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompProxyCsi")) {
			SaNameT name;
			name = *((SaNameT *)value);
			if (name.length == 0) {
				LOG_ER("Modification of saAmfCompProxyCsi Fail, Length Zero");
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompContainerCsi")) {
			SaNameT name;
			name = *((SaNameT *)value);
			if (name.length == 0) {
				LOG_ER("Modification of saAmfCompContainerCsi Fail, Length Zero");
				goto done;
			}
		} else {
			LOG_ER("Modification of attribute '%s' not supported", attribute->attrName);
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

	comp = avd_comp_get(&opdata->objectName);
	param.name = comp->comp_info.name;
	comp_type = avd_comptype_get(&comp->saAmfCompType);

	m_AVD_GET_SU_NODE_PTR(avd_cb, comp->su, su_node_ptr);

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

			avd_comptype_remove_comp(comp);
			comp->saAmfCompType = *dn;
			comp->comp_type = avd_comptype_get(dn);
			avd_comptype_add_comp(comp);
			param.attr_id = saAmfCompType_ID;
			param.name_sec = *dn;
			TRACE("saAmfCompType changed to '%s' for '%s'", dn->value, opdata->objectName.value);

		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiateCmdArgv")) {

			/* Node director will reread configuration from IMM */
			param.attr_id = saAmfCompInstantiateCmd_ID;

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
			comp->comp_info.init_time = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompInstantiationLevel")) {

			AVD_SU *su = comp->su;

			if (value_is_deleted)
				value = &comp_type->saAmfCtDefInstantiationLevel;

			param.attr_id = saAmfCompInstantiationLevel_ID;
			param.value_len = sizeof(uint32_t);
			memcpy(&param.value[0],(SaUint32T *)value , param.value_len);
			comp->comp_info.inst_level = *((SaUint32T *)value);
			
			avd_su_remove_comp(comp);
			comp->su = su;
			avd_su_add_comp(comp);
			
		} else if (!strcmp(attribute->attrName, "saAmfCompNumMaxInstantiateWithoutDelay")) {

			uint32_t num_inst;

			if (value_is_deleted)
				value = &avd_comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay;

			num_inst = *((SaUint32T *)value);
			param.attr_id = saAmfCompNumMaxInstantiate_ID;
			param.value_len = sizeof(uint32_t);
			num_inst = htonl(num_inst);
			memcpy(&param.value[0], &num_inst, param.value_len);
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
			comp->inst_retry_delay = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompTerminateCmdArgv")) {

			/* Node director will refresh from IMM */
			param.attr_id = saAmfCompTerminateCmd_ID;

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
			comp->comp_info.term_time = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompCleanupCmdArgv")) {

			/* Node director will reread configuration from IMM */
			param.attr_id = saAmfCompCleanupCmd_ID;

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
			comp->comp_info.clean_time = *((SaTimeT *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompAmStartCmdArgv")) {

			/* Node director will reread configuration from IMM */
			param.attr_id = saAmfCompAmStartCmd_ID;

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
			comp->comp_info.max_num_amstart = *((SaUint32T *)value);

		} else if (!strcmp(attribute->attrName, "saAmfCompAmStopCmdArgv")) {

			/* Node director will reread configuration from IMM */
			param.attr_id = saAmfCompAmStopCmd_ID;

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
			comp->comp_info.def_recvr = *((SaUint32T *)value);
			if (comp->comp_info.def_recvr == SA_AMF_NO_RECOMMENDATION) {
				comp->comp_info.def_recvr = SA_AMF_COMPONENT_FAILOVER;
				LOG_NO("COMPONENT_FAILOVER(%u) used instead of NO_RECOMMENDATION(%u) for '%s'",
					   SA_AMF_COMPONENT_FAILOVER, SA_AMF_NO_RECOMMENDATION, comp->comp_info.name.value);
			}
		} else if (!strcmp(attribute->attrName, "saAmfCompDisableRestart")) {
			if (value_is_deleted)
				comp->comp_info.comp_restart = comp_type->saAmfCtDefDisableRestart;
			else
				comp->comp_info.comp_restart = *((SaUint32T *)value);

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

static void comp_ccb_apply_delete_hdlr(struct CcbUtilOperationData *opdata)
{
	AVD_COMP *comp = NULL, *i_comp = NULL;
	bool isPre;
	AVD_AVND *su_node_ptr = NULL;
	AVSV_PARAM_INFO param;
	SaBoolT old_val;
	SaBoolT su_delete = SA_FALSE;
	struct CcbUtilOperationData *t_opData;

	TRACE_ENTER();

	comp = avd_comp_get(&opdata->objectName);
	/* comp should be found in the database even if it was 
	 * due to parent su delete the changes are applied in 
	 * bottom up order so all the component deletes are applied 
	 * first and then parent SU delete is applied
	 * just doing sanity check here
	 **/
	osafassert(comp != NULL);

	old_val = comp->su->saAmfSUPreInstantiable;

	/* Verify if the SUs preinstan value need to be changed */
	if ((AVSV_COMP_TYPE_SA_AWARE == comp->comp_info.category) ||
	    (AVSV_COMP_TYPE_PROXIED_LOCAL_PRE_INSTANTIABLE == comp->comp_info.category) ||
	    (AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE == comp->comp_info.category)) {
		isPre = false;
		i_comp = comp->su->list_of_comp;
		while (i_comp) {
			if (((AVSV_COMP_TYPE_SA_AWARE == i_comp->comp_info.category) ||
			     (AVSV_COMP_TYPE_PROXIED_LOCAL_PRE_INSTANTIABLE == i_comp->comp_info.category) ||
			     (AVSV_COMP_TYPE_EXTERNAL_PRE_INSTANTIABLE == i_comp->comp_info.category))
			    && (i_comp != comp)) {
				isPre = true;
				break;
			}
			i_comp = i_comp->su_comp_next;
		}		/* end while */

		if (isPre == true) {
			comp->su->saAmfSUPreInstantiable = true;
		} else {
			comp->su->saAmfSUPreInstantiable = false;
		}
	}

	/* check whether the SU is also undergoing delete operation */
	t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, &comp->su->name);
	if (t_opData && t_opData->operationType == CCBUTIL_DELETE) {
		su_delete = SA_TRUE;
	}

	/* if SU is not being deleted and the PreInstantiable state has changed
	 * then update the IMM with the new value for saAmfSUPreInstantiable */
	if (su_delete == SA_FALSE && old_val != comp->su->saAmfSUPreInstantiable) {
		avd_saImmOiRtObjectUpdate(&comp->su->name, "saAmfSUPreInstantiable", SA_IMM_ATTR_SAUINT32T,
				&comp->su->saAmfSUPreInstantiable);
	}

	/* send a message to the AVND deleting the
	 * component.
	 */
	m_AVD_GET_SU_NODE_PTR(avd_cb, comp->su, su_node_ptr);
	if ((su_node_ptr->node_state == AVD_AVND_STATE_PRESENT) ||
	    (su_node_ptr->node_state == AVD_AVND_STATE_NO_CONFIG) ||
	    (su_node_ptr->node_state == AVD_AVND_STATE_NCS_INIT)) {
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
	AVD_COMP *comp = avd_comp_get(dn);

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
	NCS_PATRICIA_PARAMS patricia_params;

	patricia_params.key_size = sizeof(SaNameT);
	osafassert(ncs_patricia_tree_init(&comp_db, &patricia_params) == NCSCC_RC_SUCCESS);

	avd_class_impl_set("SaAmfComp", comp_rt_attr_cb, comp_admin_op_cb,
		comp_ccb_completed_cb, comp_ccb_apply_cb);
}

