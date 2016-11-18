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
#include <set>
#include <string.h>
#include "amf/amfd/node.h"
#include "imm/saf/saImmOm.h"
#include "osaf/immutil/immutil.h"
#include "base/logtrace.h"

#include "amf/amf_util.h"
#include "amf/amfd/comp.h"
#include "amf/amfd/imm.h"
#include "amf/amfd/util.h"

/* Global variable for the singleton object used by comp class */
AVD_COMP_GLOBALATTR avd_comp_global_attrs;

AmfDb<std::string, AVD_COMP_TYPE> *comptype_db = nullptr;

static void comptype_db_add(AVD_COMP_TYPE *compt)
{
	unsigned int rc = comptype_db->insert(compt->name,compt);
	osafassert (rc == NCSCC_RC_SUCCESS);
}

static void comptype_delete(AVD_COMP_TYPE *avd_comp_type)
{
	osafassert(nullptr == avd_comp_type->list_of_comp);
	comptype_db->erase(avd_comp_type->name);
	delete avd_comp_type;
}

void avd_comptype_add_comp(AVD_COMP *comp)
{
	comp->comp_type_list_comp_next = comp->comp_type->list_of_comp;
	comp->comp_type->list_of_comp = comp;
}

void avd_comptype_remove_comp(AVD_COMP *comp)
{
	AVD_COMP *i_comp = nullptr;
	AVD_COMP *prev_comp = nullptr;

	if (comp->comp_type != nullptr) {
		i_comp = comp->comp_type->list_of_comp;

		while ((i_comp != nullptr) && (i_comp != comp)) {
			prev_comp = i_comp;
			i_comp = i_comp->comp_type_list_comp_next;
		}

		if (i_comp == comp) {
			if (prev_comp == nullptr) {
				comp->comp_type->list_of_comp = comp->comp_type_list_comp_next;
			} else {
				prev_comp->comp_type_list_comp_next = comp->comp_type_list_comp_next;
			}

			comp->comp_type_list_comp_next = nullptr;
			comp->comp_type = nullptr;
		}
	}
}

//
AVD_COMP_TYPE::AVD_COMP_TYPE(const std::string& dn) :
	name(dn)
{
}

static AVD_COMP_TYPE *comptype_create(const std::string& dn, const SaImmAttrValuesT_2 **attributes)
{
	AVD_COMP_TYPE *compt;
	const char *str;
	SaAisErrorT error;
	SaNameT ct_sw_bundle;

	TRACE_ENTER2("'%s'", dn.c_str());

	compt = new AVD_COMP_TYPE(dn);

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtCompCategory"), attributes, 0, &compt->saAmfCtCompCategory);
	osafassert(error == SA_AIS_OK);

	(void)immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtSwBundle"), attributes, 0, &ct_sw_bundle);
	compt->saAmfCtSwBundle = Amf::to_string(&ct_sw_bundle);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtDefCmdEnv", 0)) != nullptr)
		strcpy(compt->saAmfCtDefCmdEnv, str);
	(void)immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefClcCliTimeout"), attributes, 0, &compt->saAmfCtDefClcCliTimeout);
	(void)immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefCallbackTimeout"), attributes, 0, &compt->saAmfCtDefCallbackTimeout);

	if ((str = immutil_getStringAttr(attributes, "saAmfCtRelPathInstantiateCmd", 0)) != nullptr)
		strcpy(compt->saAmfCtRelPathInstantiateCmd, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtDefInstantiateCmdArgv", 0)) != nullptr)
		strcpy(compt->saAmfCtDefInstantiateCmdArgv, str);

	(void)immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefInstantiationLevel"), attributes, 0, &compt->saAmfCtDefInstantiationLevel);

	if ((str = immutil_getStringAttr(attributes, "saAmfCtRelPathTerminateCmd", 0)) != nullptr)
		strcpy(compt->saAmfCtRelPathTerminateCmd, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtDefTerminateCmdArgv", 0)) != nullptr)
		strcpy(compt->saAmfCtDefTerminateCmdArgv, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtRelPathCleanupCmd", 0)) != nullptr)
		strcpy(compt->saAmfCtRelPathCleanupCmd, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtDefCleanupCmdArgv", 0)) != nullptr)
		strcpy(compt->saAmfCtDefCleanupCmdArgv, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtRelPathAmStartCmd", 0)) != nullptr)
		strcpy(compt->saAmfCtRelPathAmStartCmd, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtDefAmStartCmdArgv", 0)) != nullptr)
		strcpy(compt->saAmfCtDefAmStartCmdArgv, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtRelPathAmStopCmd", 0)) != nullptr)
		strcpy(compt->saAmfCtRelPathAmStopCmd, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtDefAmStopCmdArgv", 0)) != nullptr)
		strcpy(compt->saAmfCtDefAmStopCmdArgv, str);

	if ((IS_COMP_SAAWARE(compt->saAmfCtCompCategory) || IS_COMP_PROXIED_PI(compt->saAmfCtCompCategory)) &&
	    (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefQuiescingCompleteTimeout"), attributes, 0,
						 &compt->saAmfCtDefQuiescingCompleteTimeout) != SA_AIS_OK)) {
			compt->saAmfCtDefQuiescingCompleteTimeout = compt->saAmfCtDefCallbackTimeout;
	}

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefRecoveryOnError"), attributes, 0, &compt->saAmfCtDefRecoveryOnError);
	osafassert(error == SA_AIS_OK);

	if (compt->saAmfCtDefRecoveryOnError == SA_AMF_NO_RECOMMENDATION) {
		compt->saAmfCtDefRecoveryOnError = SA_AMF_COMPONENT_FAILOVER;
		LOG_NO("COMPONENT_FAILOVER(%u) used instead of NO_RECOMMENDATION(%u) for '%s'",
			   SA_AMF_COMPONENT_FAILOVER, SA_AMF_NO_RECOMMENDATION, dn.c_str());
	}

	(void)immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefDisableRestart"), attributes, 0, &compt->saAmfCtDefDisableRestart);

	TRACE_LEAVE();
	return compt;
}

/**
 * reports path validation errors
 * @param opdata
 * @param attr_name
 */
static inline void report_path_validation_err(CcbUtilOperationData_t *opdata,
                                              const char *attr_name) {
	if (opdata != nullptr) {
		report_ccb_validation_error(opdata,
			"%s does not contain an absolute path and "
			"attribute saAmfCtSwBundle is not configured for '%s'",
			 attr_name, osaf_extended_name_borrow(&opdata->objectName));
	} else {
		report_ccb_validation_error(opdata,
			"%s does not contain an absolute path and "
			"attribute saAmfCtSwBundle is not configured",
			 attr_name);
	}
}

/**
 * Validates new component type in CCB
 * @param dn
 * @param attributes
 * @param opdata
 * @return true if valid
 */
static bool config_is_valid(const std::string& dn,
                            const SaImmAttrValuesT_2 **attributes,
                            CcbUtilOperationData_t *opdata)
{
	SaUint32T category;
	SaUint32T value;
	SaTimeT time;
	SaAisErrorT rc;
	const char *cmd;
	const char *attr_name;
	std::string::size_type pos;

	if ((pos = dn.find(',')) == std::string::npos) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn.c_str());
		return false;
	}

	/* Should be children to the Comp Base type */
	if (dn.compare(pos + 1, 12, "safCompType=") != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", dn.substr(pos + 1).c_str(), dn.c_str());
		return false;
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtCompCategory"), attributes, 0, &category);
	osafassert(rc == SA_AIS_OK);

	/* We do not support Proxy, Container and Contained as of now. */
	if (IS_COMP_PROXY(category) || IS_COMP_CONTAINER(category)|| IS_COMP_CONTAINED(category)) {
		report_ccb_validation_error(opdata, "Unsupported saAmfCtCompCategory value '%u' for '%s'",
				category, dn.c_str());
		return false;
	}

	/*
	** The saAmfCtDefClcCliTimeout attribute is "mandatory for all components except for
	** external components"
	*/
	if (IS_COMP_LOCAL(category) &&
	    (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefClcCliTimeout"), attributes, 0, &time) != SA_AIS_OK)) {
		report_ccb_validation_error(opdata, "Required attribute saAmfCtDefClcCliTimeout not configured for '%s'",
				dn.c_str());
		return false;
	}

	/*
	** The saAmfCtDefCallbackTimeout attribute "mandatory for all components except for
	** non-proxied, non-SA-aware components"
	*/
	if ((IS_COMP_PROXIED(category) || IS_COMP_SAAWARE(category)) &&
	    (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefCallbackTimeout"), attributes, 0, &time) != SA_AIS_OK)) {
		report_ccb_validation_error(opdata, "Required attribute saAmfCtDefCallbackTimeout not configured for '%s'",
				dn.c_str());
		return false;
	}

	/*
	** The saAmfCtDefQuiescingCompleteTimeout attribute "is actually mandatory for SA-aware and proxied, 
	** pre-instantiable components"
	*/
	if ((IS_COMP_SAAWARE(category) || IS_COMP_PROXIED_PI(category)) &&
	    (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefQuiescingCompleteTimeout"), attributes, 0, &time) != SA_AIS_OK)) {
		report_ccb_validation_error(opdata, "Required attribute saAmfCtDefQuiescingCompleteTimeout not configured"
				" for '%s'", dn.c_str());

		// this is OK for backwards compatibility reasons
	}

	SaNameT bundle_name = {0};
	bool bundle_configured = false;
	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtSwBundle"),
			attributes, 0, &bundle_name) == SA_AIS_OK) {
		bundle_configured = true;
	}

	/* 
	** The saAmfCtRelPathInstantiateCmd "attribute is mandatory for all
	** non-proxied local components".
	*/
	if (!(IS_COMP_PROXIED(category) || IS_COMP_PROXIED_NPI(category)) && IS_COMP_LOCAL(category)) {
		attr_name = "saAmfCtRelPathInstantiateCmd";

		cmd = immutil_getStringAttr(attributes, attr_name, 0);
		if (cmd == nullptr) {
			report_ccb_validation_error(opdata,
				"Required attribute %s not configured for '%s'",
				attr_name, osaf_extended_name_borrow(&opdata->objectName));
			return false;
		}

		if ((cmd[0] != '/') && (bundle_configured == false)) {
			report_path_validation_err(opdata, attr_name);
			return false;
		}
	}

	/*
	** The saAmfCtRelPathTerminateCmd "attribute is mandatory for local
	** non-proxied, non-SA-aware components".
	*/
	if (IS_COMP_LOCAL(category) && !(IS_COMP_PROXIED(category) ||
			IS_COMP_PROXIED_NPI(category)) && !IS_COMP_SAAWARE(category)) {
		attr_name = "saAmfCtRelPathTerminateCmd";

		cmd = immutil_getStringAttr(attributes, attr_name, 0);
		if (cmd == nullptr) {
			report_ccb_validation_error(opdata,
				"Required attribute %s not configured for '%s'",
				attr_name, osaf_extended_name_borrow(&opdata->objectName));
			return false;
		}

		if ((cmd[0] != '/') && (bundle_configured == false)) {
			report_path_validation_err(opdata, attr_name);
			return false;
		}
	}

	/*
	** The saAmfCtRelPathCleanupCmd "attribute is mandatory for all local
	** components (proxied or non-proxied)"
	*/
	if (IS_COMP_LOCAL(category)) {
		attr_name = "saAmfCtRelPathCleanupCmd";

		cmd = immutil_getStringAttr(attributes, attr_name, 0);
		if (cmd == nullptr) {
			report_ccb_validation_error(opdata,
				"Required attribute %s not configured for '%s'",
				attr_name, osaf_extended_name_borrow(&opdata->objectName));
			return false;
		}

		if ((cmd[0] != '/') && (bundle_configured == false)) {
			report_path_validation_err(opdata, attr_name);
			return false;
		}
	}

	attr_name = "saAmfCtRelPathAmStartCmd";
	cmd = immutil_getStringAttr(attributes, attr_name, 0);
	if (cmd != nullptr) {
		if ((cmd[0] != '/') && (bundle_configured == false)) {
			report_path_validation_err(opdata, attr_name);
			return false;
		}
	}

	attr_name = "saAmfCtRelPathAmStopCmd";
	cmd = immutil_getStringAttr(attributes, attr_name, 0);
	if (cmd != nullptr) {
		if ((cmd[0] != '/') && (bundle_configured == false)) {
			report_path_validation_err(opdata, attr_name);
			return false;
		}
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefRecoveryOnError"), attributes, 0, &value);
	osafassert(rc == SA_AIS_OK);

	if ((value < SA_AMF_NO_RECOMMENDATION) || (value > SA_AMF_NODE_FAILFAST)) {
		report_ccb_validation_error(opdata, "Illegal/unsupported saAmfCtDefRecoveryOnError value %u for '%s'",
				value, dn.c_str());
		return false;
	}

	if (value == SA_AMF_NO_RECOMMENDATION)
		LOG_NO("Invalid configuration, saAmfCtDefRecoveryOnError=NO_RECOMMENDATION(%u) for '%s'",
			   value, dn.c_str());

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefDisableRestart"), attributes, 0, &value);
	if ((rc == SA_AIS_OK) && (value > SA_TRUE)) {
		report_ccb_validation_error(opdata, "Illegal saAmfCtDefDisableRestart value %u for '%s'",
			   value, dn.c_str());
		return false;
	}

	return true;
}

/**
 * Get configuration for all SaAmfCompType objects from IMM and
 * create AVD internal objects.
 * @param cb
 * 
 * @return int
 */
SaAisErrorT avd_comptype_config_get(void)
{
	SaAisErrorT error, rc = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfCompType";
	AVD_COMP_TYPE *comp_type;

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, nullptr,
		SA_IMM_SUBTREE, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR,
		&searchParam, nullptr, &searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("saImmOmSearchInitialize_2 failed: %u", error);
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		if (config_is_valid(Amf::to_string(&dn), attributes, nullptr) == false)
			goto done2;
		if ((comp_type = comptype_db->find(Amf::to_string(&dn))) == nullptr) {
			if ((comp_type = comptype_create(Amf::to_string(&dn), attributes)) == nullptr)
				goto done2;

			comptype_db_add(comp_type);
		}

		if (avd_ctcstype_config_get(Amf::to_string(&dn), comp_type) != SA_AIS_OK)
			goto done2;
	}

	rc = SA_AIS_OK;

done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
done1:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

static void ccb_apply_modify_hdlr(const CcbUtilOperationData_t *opdata)
{
	const SaImmAttrModificationT_2 *attr_mod;
	int i;
	AVD_COMP_TYPE *comp_type;
	SaNameT comp_type_name;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	// input example: opdata.objectName.value, safVersion=1,safCompType=AmfDemo1
	comp_type_name = opdata->objectName;

	if ((comp_type = comptype_db->find(Amf::to_string(&comp_type_name))) == 0) {
		LOG_ER("Internal error: %s not found", osaf_extended_name_borrow(&comp_type_name));
		return;
	}

	// Create a set of nodes where components "may" be using the given comp_type attributes.
	// A msg will be sent to the related node regarding this change. If a component has an 
	// comp_type attribute that overrides this comp_type attribute it will be handled by the amfnd.
	std::set<AVD_AVND*, NodeNameCompare> node_set;

	AVD_COMP *comp = comp_type->list_of_comp;
	while (comp != nullptr) {
		node_set.insert(comp->su->su_on_node);
		TRACE("comp name %s on node %s", osaf_extended_name_borrow(&comp->comp_info.name),  comp->su->su_on_node->name.c_str());
		comp = comp->comp_type_list_comp_next;
	}			
		
	std::set<AVD_AVND*>::iterator it;
	for (it = node_set.begin(); it != node_set.end(); ++it) {
		i = 0;
		while ((attr_mod = opdata->param.modify.attrMods[i++]) != nullptr) {
			AVSV_PARAM_INFO param;
			memset(&param, 0, sizeof(param));
			param.class_id = AVSV_SA_AMF_COMP_TYPE;
			param.act = AVSV_OBJ_OPR_MOD;
			param.name = opdata->objectName;
			const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;

			if (!strcmp(attribute->attrName, "saAmfCtDefCallbackTimeout")) {
				SaTimeT *param_val = (SaTimeT *)attribute->attrValues[0];
				TRACE("saAmfCtDefCallbackTimeout to '%llu' for compType '%s' on node '%s'", *param_val, 
					osaf_extended_name_borrow(&opdata->objectName), (*it)->name.c_str());
				param.value_len = sizeof(*param_val);
				memcpy(param.value, param_val, param.value_len);
				param.attr_id = saAmfCtDefCallbackTimeout_ID;
				avd_snd_op_req_msg(avd_cb, *it, &param);
			} else if (!strcmp(attribute->attrName, "saAmfCtDefClcCliTimeout")) {
				SaTimeT *param_val = (SaTimeT *)attribute->attrValues[0];
				TRACE("saAmfCtDefClcCliTimeout to '%llu' for compType '%s' on node '%s'", *param_val, 
					osaf_extended_name_borrow(&opdata->objectName), (*it)->name.c_str());
				param.value_len = sizeof(*param_val);
				memcpy(param.value, param_val, param.value_len);
				param.attr_id = saAmfCtDefClcCliTimeout_ID;
				avd_snd_op_req_msg(avd_cb, *it, &param);
			} else if (!strcmp(attribute->attrName, "saAmfCtDefQuiescingCompleteTimeout")) {
				SaTimeT *param_val = (SaTimeT *)attribute->attrValues[0];
				TRACE("saAmfCtDefQuiescingCompleteTimeout to '%llu' for compType '%s' on node '%s'", *param_val, 
					osaf_extended_name_borrow(&opdata->objectName), (*it)->name.c_str());
				param.value_len = sizeof(*param_val);
				memcpy(param.value, param_val, param.value_len);
				param.attr_id = saAmfCtDefQuiescingCompleteTimeout_ID;
				avd_snd_op_req_msg(avd_cb, *it, &param);
			} else if (!strcmp(attribute->attrName, "saAmfCtDefInstantiationLevel")) {
				SaUint32T param_val; 
				SaUint32T old_value = comp_type->saAmfCtDefInstantiationLevel;
				if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || 
						(attribute->attrValues == nullptr)) {
					param_val = 0; //Default value as per Section 8.13.1 (B0401)
				} else {
					param_val = *(SaUint32T *)attribute->attrValues[0];
				}
				TRACE("saAmfCtDefInstantiationLevel to '%u' for compType '%s' on node '%s'", param_val, 
					osaf_extended_name_borrow(&opdata->objectName), (*it)->name.c_str());
				param.value_len = sizeof(param_val);
				memcpy(param.value, &param_val, param.value_len);
				param.attr_id = saAmfCtDefInstantiationLevel_ID;
				comp_type->saAmfCtDefInstantiationLevel = param_val;
				if (old_value != param_val)
					avd_snd_op_req_msg(avd_cb, *it, &param);	
			} else if (!strcmp(attribute->attrName, "saAmfCtDefRecoveryOnError")) {
				SaAmfRecommendedRecoveryT *param_val = (SaAmfRecommendedRecoveryT *)attribute->attrValues[0];
				TRACE("saAmfCtDefRecoveryOnError to '%u' for compType '%s' on node '%s'", *param_val, 
					osaf_extended_name_borrow(&opdata->objectName), (*it)->name.c_str());
				param.value_len = sizeof(*param_val);
				memcpy(param.value, param_val, param.value_len);
				param.attr_id = saAmfCtDefRecoveryOnError_ID;
				avd_snd_op_req_msg(avd_cb, *it, &param);
			} else if (!strcmp(attribute->attrName, "saAmfCtDefDisableRestart")) {
				SaBoolT param_val; 
				SaUint32T old_value = comp_type->saAmfCtDefDisableRestart;
				if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || 
						(attribute->attrValues == nullptr)) {
					param_val = static_cast<SaBoolT>(0); //Default value as per Section 8.13.1 (B0401)
				} else {
					param_val = *(SaBoolT *)attribute->attrValues[0];
				}
				TRACE("saAmfCtDefDisableRestart to '%u' for compType '%s' on node '%s'", param_val, 
					osaf_extended_name_borrow(&opdata->objectName), (*it)->name.c_str());
				param.value_len = sizeof(param_val);
				memcpy(param.value, &param_val, param.value_len);
				param.attr_id = saAmfCtDefDisableRestart_ID;
				comp_type->saAmfCtDefDisableRestart = param_val;
				if (old_value != param_val)
					avd_snd_op_req_msg(avd_cb, *it, &param);
			} else
				LOG_WA("Unexpected attribute name: %s", attribute->attrName);
		}
	}	
}	

static void comptype_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_COMP_TYPE *comp_type;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		comp_type = comptype_create(Amf::to_string(&opdata->objectName),
			opdata->param.create.attrValues);
		osafassert(comp_type);
		comptype_db_add(comp_type);
		break;
	case CCBUTIL_DELETE:
		comptype_delete(static_cast<AVD_COMP_TYPE*>(opdata->userData));
		break;
	case CCBUTIL_MODIFY:
		ccb_apply_modify_hdlr(opdata);
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE();
}

/**
 * Validates proposed change in comptype
 */
static SaAisErrorT ccb_completed_modify_hdlr(const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_OK;
	const SaImmAttrModificationT_2 *mod;
	int i = 0;
	const char *dn = (char*)osaf_extended_name_borrow(&opdata->objectName);
	bool value_is_deleted = false;

	while ((mod = opdata->param.modify.attrMods[i++]) != nullptr) {

		if ((mod->modType == SA_IMM_ATTR_VALUES_DELETE) || (mod->modAttr.attrValues == nullptr)) {
			/* Attribute value is deleted, revert to default value if applicable*/
			value_is_deleted = true;
		} else {
			/* Attribute value is modified */
			value_is_deleted = false;
		}

		if (strcmp(mod->modAttr.attrName, "saAmfCtDefClcCliTimeout") == 0) {
			// if it exist it cannot be removed, just changed
			if (value_is_deleted == true) {
				report_ccb_validation_error(opdata,
					"Value deletion for '%s' is not supported", mod->modAttr.attrName);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}

			SaTimeT value = *((SaTimeT *)mod->modAttr.attrValues[0]);
			if (value < SA_TIME_ONE_SECOND) {
				report_ccb_validation_error(opdata,
					"Invalid saAmfCtDefClcCliTimeout for '%s'", dn);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else if (strcmp(mod->modAttr.attrName, "saAmfCtDefCallbackTimeout") == 0) {
			// if it exist it cannot be removed, just changed
			if (value_is_deleted == true) {
				report_ccb_validation_error(opdata,
					"Value deletion for '%s' is not supported", mod->modAttr.attrName);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}

			SaTimeT value = *((SaTimeT *)mod->modAttr.attrValues[0]);
			if (value < 100 * SA_TIME_ONE_MILLISECOND) {
				report_ccb_validation_error(opdata,
					"Invalid saAmfCtDefCallbackTimeout for '%s'", dn);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else if (strcmp(mod->modAttr.attrName, "saAmfCtDefInstantiateCmdArgv") == 0) {
			; // Allow modification, no validation can be done
		} else if (strcmp(mod->modAttr.attrName, "saAmfCtDefTerminateCmdArgv") == 0) {
			; // Allow modification, no validation can be done
		} else if (strcmp(mod->modAttr.attrName, "saAmfCtDefCleanupCmdArgv") == 0) {
			; // Allow modification, no validation can be done
		} else if (strcmp(mod->modAttr.attrName, "saAmfCtDefAmStartCmdArgv") == 0) {
			; // Allow modification, no validation can be done
		} else if (strcmp(mod->modAttr.attrName, "saAmfCtDefAmStopCmdArgv") == 0) {
			; // Allow modification, no validation can be done
		} else if (strcmp(mod->modAttr.attrName, "osafAmfCtDefHcCmdArgv") == 0) {
			; // Allow modification, no validation can be done
		} else if (strcmp(mod->modAttr.attrName, "saAmfCtDefQuiescingCompleteTimeout") == 0) {
			if (value_is_deleted == true) {
				report_ccb_validation_error(opdata,
					"Value deletion for '%s' is not supported", mod->modAttr.attrName);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
			SaTimeT value = *((SaTimeT *)mod->modAttr.attrValues[0]);
			if (value < 100 * SA_TIME_ONE_MILLISECOND) {
				report_ccb_validation_error(opdata,
					"Invalid saAmfCtDefQuiescingCompleteTimeout for '%s'", dn);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else if (!strcmp(mod->modAttr.attrName, "saAmfCtDefInstantiationLevel")) {
			if (value_is_deleted == true)
				continue;
			uint32_t num_inst = *((SaUint32T *)mod->modAttr.attrValues[0]);
			if (num_inst == 0) {
				report_ccb_validation_error(opdata, "Modification of saAmfCtDefInstantiationLevel Fail,"
						" Zero InstantiationLevel");
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else if (strcmp(mod->modAttr.attrName, "saAmfCtDefRecoveryOnError") == 0) {
			if (value_is_deleted == true) {
				report_ccb_validation_error(opdata,
					"Value deletion for '%s' is not supported", mod->modAttr.attrName);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
			uint32_t value = *((SaUint32T *)mod->modAttr.attrValues[0]);
			if ((value < SA_AMF_COMPONENT_RESTART) || (value > SA_AMF_NODE_FAILFAST)) {
				report_ccb_validation_error(opdata,
					"Invalid saAmfCtDefRecoveryOnError for '%s'", dn);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else if (strcmp(mod->modAttr.attrName, "saAmfCtDefDisableRestart") == 0) {
			if (value_is_deleted == true)
				continue;
			uint32_t value = *((SaUint32T *)mod->modAttr.attrValues[0]);
			if (value > SA_TRUE) {
				report_ccb_validation_error(opdata,
					"Invalid saAmfCtDefDisableRestart for '%s'", dn);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else {
			// catch all for non supported changes
			report_ccb_validation_error(opdata,
				"Modification of attribute '%s' not supported",
				mod->modAttr.attrName);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}
	}

done:
	return rc;
}

static SaAisErrorT comptype_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	AVD_COMP_TYPE *comp_type;
	AVD_COMP *comp;
	bool comp_exist = false;
	CcbUtilOperationData_t *t_opData;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (config_is_valid(Amf::to_string(&opdata->objectName),
				opdata->param.create.attrValues, opdata) == true)
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		rc = ccb_completed_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		comp_type = comptype_db->find(Amf::to_string(&opdata->objectName));
		if (nullptr != comp_type->list_of_comp) {
			/* check whether there exists a delete operation for 
			 * each of the Comp in the comp_type list in the current CCB
			 */
			comp = comp_type->list_of_comp;
			while (comp != nullptr) {
				t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, &comp->comp_info.name);
				if ((t_opData == nullptr) || (t_opData->operationType != CCBUTIL_DELETE)) {
					comp_exist = true;
					break;
				}
				comp = comp->comp_type_list_comp_next;
			}
			if (comp_exist == true) {
				report_ccb_validation_error(opdata, "SaAmfCompType '%s' is in use",comp_type->name.c_str());
				goto done;
			}
		}
		opdata->userData = comp_type;
		rc = SA_AIS_OK;
		break;
	default:
		osafassert(0);
		break;
	}

done:
	return rc;
}

void avd_comptype_constructor(void)
{
	comptype_db = new AmfDb<std::string, AVD_COMP_TYPE>;
	avd_class_impl_set("SaAmfCompBaseType", nullptr, nullptr,
		avd_imm_default_OK_completed_cb, nullptr);
	avd_class_impl_set("SaAmfCompType", nullptr, nullptr,
		comptype_ccb_completed_cb, comptype_ccb_apply_cb);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

SaAisErrorT avd_compglobalattrs_config_get(void)
{
	SaAisErrorT rc;
	const SaImmAttrValuesT_2 **attributes;
	SaImmAccessorHandleT accessorHandle;
	const std::string dn{"safRdn=compGlobalAttributes,safApp=safAmfService"};

	immutil_saImmOmAccessorInitialize(avd_cb->immOmHandle, &accessorHandle);
	rc = immutil_saImmOmAccessorGet_o2(accessorHandle, dn.c_str(), nullptr, const_cast<SaImmAttrValuesT_2 ***>(&attributes));
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOmAccessorGet_2 FAILED %u", rc);
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto done;
	}

	TRACE("'%s'", dn.c_str());

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNumMaxInstantiateWithoutDelay"), attributes, 0,
			    &avd_comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay) != SA_AIS_OK) {
		avd_comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay = 2;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNumMaxInstantiateWithDelay"), attributes, 0,
			    &avd_comp_global_attrs.saAmfNumMaxInstantiateWithDelay) != SA_AIS_OK) {
		avd_comp_global_attrs.saAmfNumMaxInstantiateWithDelay = 0;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNumMaxAmStartAttempts"), attributes, 0,
			    &avd_comp_global_attrs.saAmfNumMaxAmStartAttempts) != SA_AIS_OK) {
		avd_comp_global_attrs.saAmfNumMaxAmStartAttempts = 2;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNumMaxAmStopAttempts"), attributes, 0,
			    &avd_comp_global_attrs.saAmfNumMaxAmStopAttempts) != SA_AIS_OK) {
		avd_comp_global_attrs.saAmfNumMaxAmStopAttempts = 2;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfDelayBetweenInstantiateAttempts"), attributes, 0,
			    &avd_comp_global_attrs.saAmfDelayBetweenInstantiateAttempts) != SA_AIS_OK) {
		avd_comp_global_attrs.saAmfDelayBetweenInstantiateAttempts = 0;
	}

	immutil_saImmOmAccessorFinalize(accessorHandle);

	rc = SA_AIS_OK;

 done:
	return rc;
}

static void avd_compglobalattrs_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	int i = 0;
	const SaImmAttrModificationT_2 *attrMod;
	bool value_is_deleted;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_MODIFY:
		while ((attrMod = opdata->param.modify.attrMods[i++]) != nullptr) {
			void *value = nullptr;

			if ((attrMod->modType == SA_IMM_ATTR_VALUES_DELETE) ||
					(attrMod->modAttr.attrValues == nullptr)) {
				/* Attribute value is deleted, revert to default value */
				value_is_deleted = true;
			} else {
				/* Attribute value is modified */
				value_is_deleted = false;
				value = attrMod->modAttr.attrValues[0];
			}

			if (!strcmp("saAmfNumMaxInstantiateWithoutDelay", attrMod->modAttr.attrName)) {
				SaUint32T old_value = avd_comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay;
				if (value_is_deleted == true) {
					 //Default value as per Section 8.14 (B0401).
					avd_comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay = 2;
				} else {
					avd_comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay =
						*((SaUint32T *)value);
				}
				TRACE("saAmfNumMaxInstantiateWithoutDelay modified from '%u' to '%u'",
						old_value,
						avd_comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay); 
			}
			if (!strcmp("saAmfNumMaxInstantiateWithDelay", attrMod->modAttr.attrName)) {
				SaUint32T old_value = avd_comp_global_attrs.saAmfNumMaxInstantiateWithDelay;
				if (value_is_deleted == true) {
					//Default value as per Section 8.14 (B0401).
					avd_comp_global_attrs.saAmfNumMaxInstantiateWithDelay = 0;
				} else {
					avd_comp_global_attrs.saAmfNumMaxInstantiateWithDelay =
						*((SaUint32T *)value);
				}
				TRACE("saAmfNumMaxInstantiateWithDelay modified from '%u' to '%u'",
						old_value,
						avd_comp_global_attrs.saAmfNumMaxInstantiateWithDelay); 
			}
			if (!strcmp("saAmfNumMaxAmStartAttempts", attrMod->modAttr.attrName)) {
				SaUint32T old_value = avd_comp_global_attrs.saAmfNumMaxAmStartAttempts;
				if (value_is_deleted == true) {
					//Default value as per Section 8.14 (B0401).
					avd_comp_global_attrs.saAmfNumMaxAmStartAttempts = 2;
				} else {
					avd_comp_global_attrs.saAmfNumMaxAmStartAttempts =
						*((SaUint32T *)value);
				}
				TRACE("saAmfNumMaxAmStartAttempts modified from '%u' to '%u'",
						old_value, 
						avd_comp_global_attrs.saAmfNumMaxAmStartAttempts);
			}
			if (!strcmp("saAmfNumMaxAmStopAttempts", attrMod->modAttr.attrName)) {
				SaUint32T old_value = avd_comp_global_attrs.saAmfNumMaxAmStartAttempts;
				if (value_is_deleted == true) {
					//Default value as per Section 8.14 (B0401).
					avd_comp_global_attrs.saAmfNumMaxAmStopAttempts = 2;
				} else {
					avd_comp_global_attrs.saAmfNumMaxAmStopAttempts =
						*((SaUint32T *)value);
				}
				TRACE("saAmfNumMaxAmStopAttempts modified from '%u' to '%u'",
						old_value, 
						avd_comp_global_attrs.saAmfNumMaxAmStopAttempts);
			}
			if (!strcmp("saAmfDelayBetweenInstantiateAttempts", attrMod->modAttr.attrName)) {
				SaTimeT old_value = avd_comp_global_attrs.saAmfDelayBetweenInstantiateAttempts;
				if (value_is_deleted == true) {
					//Default value as per Section 8.14 (B0401).
					avd_comp_global_attrs.saAmfDelayBetweenInstantiateAttempts = 0;
				} else {
					avd_comp_global_attrs.saAmfDelayBetweenInstantiateAttempts =
						*((SaTimeT*)value);
				}
				TRACE("saAmfDelayBetweenInstantiateAttempts modified from '%llu' to '%llu'",
						old_value, 
						avd_comp_global_attrs.saAmfDelayBetweenInstantiateAttempts);
			}
		}
		break;
	default:
		osafassert(0);
		break;
	}
}

static SaAisErrorT avd_compglobalattrs_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		report_ccb_validation_error(opdata, "SaAmfCompGlobalAttributes already created");
		break;
	case CCBUTIL_MODIFY:
		rc = SA_AIS_OK;
		break;
	case CCBUTIL_DELETE:
		report_ccb_validation_error(opdata, "SaAmfCompGlobalAttributes cannot be deleted");
		break;
	default:
		osafassert(0);
		break;
	}

	return rc;
}

void avd_compglobalattrs_constructor(void)
{
	avd_class_impl_set("SaAmfCompGlobalAttributes", nullptr, nullptr,
		avd_compglobalattrs_ccb_completed_cb, avd_compglobalattrs_ccb_apply_cb);
}

