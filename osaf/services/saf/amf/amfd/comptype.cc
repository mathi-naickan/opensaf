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

#include <saImmOm.h>
#include <immutil.h>
#include <logtrace.h>

#include <amf_util.h>
#include <comp.h>
#include <imm.h>

/* Global variable for the singleton object used by comp class */
AVD_COMP_GLOBALATTR avd_comp_global_attrs;

static NCS_PATRICIA_TREE comptype_db;

static void comptype_db_add(AVD_COMP_TYPE *compt)
{
	unsigned int rc = ncs_patricia_tree_add(&comptype_db, &compt->tree_node);
	osafassert (rc == NCSCC_RC_SUCCESS);
}

AVD_COMP_TYPE *avd_comptype_get(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_COMP_TYPE *)ncs_patricia_tree_get(&comptype_db, (uint8_t *)&tmp);
}

static void comptype_delete(AVD_COMP_TYPE *avd_comp_type)
{
	unsigned int rc;

	osafassert(NULL == avd_comp_type->list_of_comp);

	rc = ncs_patricia_tree_del(&comptype_db, &avd_comp_type->tree_node);
	osafassert(rc == NCSCC_RC_SUCCESS);

	delete avd_comp_type;
}

void avd_comptype_add_comp(AVD_COMP *comp)
{
	comp->comp_type_list_comp_next = comp->comp_type->list_of_comp;
	comp->comp_type->list_of_comp = comp;
}

void avd_comptype_remove_comp(AVD_COMP *comp)
{
	AVD_COMP *i_comp = NULL;
	AVD_COMP *prev_comp = NULL;

	if (comp->comp_type != NULL) {
		i_comp = comp->comp_type->list_of_comp;

		while ((i_comp != NULL) && (i_comp != comp)) {
			prev_comp = i_comp;
			i_comp = i_comp->comp_type_list_comp_next;
		}

		if (i_comp == comp) {
			if (prev_comp == NULL) {
				comp->comp_type->list_of_comp = comp->comp_type_list_comp_next;
			} else {
				prev_comp->comp_type_list_comp_next = comp->comp_type_list_comp_next;
			}

			comp->comp_type_list_comp_next = NULL;
			comp->comp_type = NULL;
		}
	}
}

static AVD_COMP_TYPE *comptype_create(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes)
{
	AVD_COMP_TYPE *compt;
	const char *str;
	SaAisErrorT error;

	TRACE_ENTER2("'%s'", dn->value);

	compt = new AVD_COMP_TYPE();

	memcpy(compt->name.value, dn->value, dn->length);
	compt->name.length = dn->length;
	compt->tree_node.key_info = (uint8_t *)&(compt->name);

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtCompCategory"), attributes, 0, &compt->saAmfCtCompCategory);
	osafassert(error == SA_AIS_OK);

	(void)immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtSwBundle"), attributes, 0, &compt->saAmfCtSwBundle);

	if ((str = immutil_getStringAttr(attributes, "saAmfCtDefCmdEnv", 0)) != NULL)
		strcpy(compt->saAmfCtDefCmdEnv, str);
	(void)immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefClcCliTimeout"), attributes, 0, &compt->saAmfCtDefClcCliTimeout);
	(void)immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefCallbackTimeout"), attributes, 0, &compt->saAmfCtDefCallbackTimeout);

	if ((str = immutil_getStringAttr(attributes, "saAmfCtRelPathInstantiateCmd", 0)) != NULL)
		strcpy(compt->saAmfCtRelPathInstantiateCmd, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtDefInstantiateCmdArgv", 0)) != NULL)
		strcpy(compt->saAmfCtDefInstantiateCmdArgv, str);

	(void)immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefInstantiationLevel"), attributes, 0, &compt->saAmfCtDefInstantiationLevel);

	if ((str = immutil_getStringAttr(attributes, "saAmfCtRelPathTerminateCmd", 0)) != NULL)
		strcpy(compt->saAmfCtRelPathTerminateCmd, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtDefTerminateCmdArgv", 0)) != NULL)
		strcpy(compt->saAmfCtDefTerminateCmdArgv, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtRelPathCleanupCmd", 0)) != NULL)
		strcpy(compt->saAmfCtRelPathCleanupCmd, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtDefCleanupCmdArgv", 0)) != NULL)
		strcpy(compt->saAmfCtDefCleanupCmdArgv, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtRelPathAmStartCmd", 0)) != NULL)
		strcpy(compt->saAmfCtRelPathAmStartCmd, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtDefAmStartCmdArgv", 0)) != NULL)
		strcpy(compt->saAmfCtDefAmStartCmdArgv, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtRelPathAmStopCmd", 0)) != NULL)
		strcpy(compt->saAmfCtRelPathAmStopCmd, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtDefAmStopCmdArgv", 0)) != NULL)
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
			   SA_AMF_COMPONENT_FAILOVER, SA_AMF_NO_RECOMMENDATION, dn->value);
	}

	(void)immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefDisableRestart"), attributes, 0, &compt->saAmfCtDefDisableRestart);

	return compt;
}

/**
 * reports path validation errors
 * @param opdata
 * @param attr_name
 */
static inline void report_path_validation_err(CcbUtilOperationData_t *opdata,
                                              const char *attr_name) {
	report_ccb_validation_error(opdata,
		"%s does not contain an absolute path and "
		"attribute saAmfCtSwBundle is not configured for '%s'",
		attr_name, opdata->objectName.value);
}

/**
 * Validates new component type in CCB
 * @param dn
 * @param attributes
 * @param opdata
 * @return true if valid
 */
static bool config_is_valid(const SaNameT *dn,
                            const SaImmAttrValuesT_2 **attributes,
                            CcbUtilOperationData_t *opdata)
{
	SaUint32T category;
	SaUint32T value;
	char *parent;
	SaTimeT time;
	SaAisErrorT rc;
	const char *cmd;
	const char *attr_name;

	if ((parent = strchr((char*)dn->value, ',')) == NULL) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn->value);
		return false;
	}

	/* Should be children to the Comp Base type */
	if (strncmp(++parent, "safCompType=", 12) != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", parent, dn->value);
		return false;
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtCompCategory"), attributes, 0, &category);
	osafassert(rc == SA_AIS_OK);

	/* We do not support Proxy, Container and Contained as of now. */
	if (IS_COMP_PROXY(category) || IS_COMP_CONTAINER(category)|| IS_COMP_CONTAINED(category)) {
		report_ccb_validation_error(opdata, "Unsupported saAmfCtCompCategory value '%u' for '%s'",
				category, dn->value);
		return false;
	}

	/*
	** The saAmfCtDefClcCliTimeout attribute is "mandatory for all components except for
	** external components"
	*/
	if (IS_COMP_LOCAL(category) &&
	    (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefClcCliTimeout"), attributes, 0, &time) != SA_AIS_OK)) {
		report_ccb_validation_error(opdata, "Required attribute saAmfCtDefClcCliTimeout not configured for '%s'",
				dn->value);
		return false;
	}

	/*
	** The saAmfCtDefCallbackTimeout attribute "mandatory for all components except for
	** non-proxied, non-SA-aware components"
	*/
	if ((IS_COMP_PROXIED(category) || IS_COMP_SAAWARE(category)) &&
	    (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefCallbackTimeout"), attributes, 0, &time) != SA_AIS_OK)) {
		report_ccb_validation_error(opdata, "Required attribute saAmfCtDefCallbackTimeout not configured for '%s'",
				dn->value);
		return false;
	}

	/*
	** The saAmfCtDefQuiescingCompleteTimeout attribute "is actually mandatory for SA-aware and proxied, 
	** pre-instantiable components"
	*/
	if ((IS_COMP_SAAWARE(category) || IS_COMP_PROXIED_PI(category)) &&
	    (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefQuiescingCompleteTimeout"), attributes, 0, &time) != SA_AIS_OK)) {
		report_ccb_validation_error(opdata, "Required attribute saAmfCtDefQuiescingCompleteTimeout not configured"
				" for '%s'", dn->value);

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
		if (cmd == NULL) {
			report_ccb_validation_error(opdata,
				"Required attribute %s not configured for '%s'",
				attr_name, opdata->objectName.value);
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
		if (cmd == NULL) {
			report_ccb_validation_error(opdata,
				"Required attribute %s not configured for '%s'",
				attr_name, opdata->objectName.value);
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
		if (cmd == NULL) {
			report_ccb_validation_error(opdata,
				"Required attribute %s not configured for '%s'",
				attr_name, opdata->objectName.value);
			return false;
		}

		if ((cmd[0] != '/') && (bundle_configured == false)) {
			report_path_validation_err(opdata, attr_name);
			return false;
		}
	}

	attr_name = "saAmfCtRelPathAmStartCmd";
	cmd = immutil_getStringAttr(attributes, attr_name, 0);
	if (cmd != NULL) {
		if ((cmd[0] != '/') && (bundle_configured == false)) {
			report_path_validation_err(opdata, attr_name);
			return false;
		}
	}

	attr_name = "saAmfCtRelPathAmStopCmd";
	cmd = immutil_getStringAttr(attributes, attr_name, 0);
	if (cmd != NULL) {
		if ((cmd[0] != '/') && (bundle_configured == false)) {
			report_path_validation_err(opdata, attr_name);
			return false;
		}
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefRecoveryOnError"), attributes, 0, &value);
	osafassert(rc == SA_AIS_OK);

	if ((value < SA_AMF_NO_RECOMMENDATION) || (value > SA_AMF_NODE_FAILFAST)) {
		report_ccb_validation_error(opdata, "Illegal/unsupported saAmfCtDefRecoveryOnError value %u for '%s'",
				value, dn->value);
		return false;
	}

	if (value == SA_AMF_NO_RECOMMENDATION)
		LOG_NO("Invalid configuration, saAmfCtDefRecoveryOnError=NO_RECOMMENDATION(%u) for '%s'",
			   value, dn->value);

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefDisableRestart"), attributes, 0, &value);
	if ((rc == SA_AIS_OK) && (value > SA_TRUE)) {
		report_ccb_validation_error(opdata, "Illegal saAmfCtDefDisableRestart value %u for '%s'",
			   value, dn->value);
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

	error = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, NULL,
		SA_IMM_SUBTREE, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR,
		&searchParam, NULL, &searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("saImmOmSearchInitialize_2 failed: %u", error);
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		if (config_is_valid(&dn, attributes, NULL) == false)
			goto done2;
		if ((comp_type = avd_comptype_get(&dn)) == NULL) {
			if ((comp_type = comptype_create(&dn, attributes)) == NULL)
				goto done2;

			comptype_db_add(comp_type);
		}

		if (avd_ctcstype_config_get(&dn, comp_type) != SA_AIS_OK)
			goto done2;
	}

	rc = SA_AIS_OK;

done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
done1:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

static void comptype_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_COMP_TYPE *comp_type;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		comp_type = comptype_create(&opdata->objectName,
			opdata->param.create.attrValues);
		osafassert(comp_type);
		comptype_db_add(comp_type);
		break;
	case CCBUTIL_DELETE:
		comptype_delete(static_cast<AVD_COMP_TYPE*>(opdata->userData));
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE();
}

static SaAisErrorT comptype_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	AVD_COMP_TYPE *comp_type;
	AVD_COMP *comp;
	SaBoolT comp_exist = SA_FALSE;
	CcbUtilOperationData_t *t_opData;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (config_is_valid(&opdata->objectName,
				opdata->param.create.attrValues, opdata) == true)
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		report_ccb_validation_error(opdata, "Modification of SaAmfCompType not supported");
		break;
	case CCBUTIL_DELETE:
		comp_type = avd_comptype_get(&opdata->objectName);
		if (NULL != comp_type->list_of_comp) {
			/* check whether there exists a delete operation for 
			 * each of the Comp in the comp_type list in the current CCB
			 */
			comp = comp_type->list_of_comp;
			while (comp != NULL) {
				t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, &comp->comp_info.name);
				if ((t_opData == NULL) || (t_opData->operationType != CCBUTIL_DELETE)) {
					comp_exist = SA_TRUE;
					break;
				}
				comp = comp->comp_type_list_comp_next;
			}
			if (comp_exist == SA_TRUE) {
				report_ccb_validation_error(opdata, "SaAmfCompType '%s' is in use",comp_type->name.value);
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
	NCS_PATRICIA_PARAMS patricia_params;

	patricia_params.key_size = sizeof(SaNameT);
	osafassert(ncs_patricia_tree_init(&comptype_db, &patricia_params) == NCSCC_RC_SUCCESS);

	avd_class_impl_set(const_cast<SaImmClassNameT>("SaAmfCompBaseType"), NULL, NULL, avd_imm_default_OK_completed_cb, NULL);
	avd_class_impl_set(const_cast<SaImmClassNameT>("SaAmfCompType"), NULL, NULL,	comptype_ccb_completed_cb, comptype_ccb_apply_cb);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

SaAisErrorT avd_compglobalattrs_config_get(void)
{
	SaAisErrorT rc;
	const SaImmAttrValuesT_2 **attributes;
	SaImmAccessorHandleT accessorHandle;
	SaNameT dn = {0, "safRdn=compGlobalAttributes,safApp=safAmfService" };

	dn.length = strlen((char *)dn.value);

	immutil_saImmOmAccessorInitialize(avd_cb->immOmHandle, &accessorHandle);
	rc = immutil_saImmOmAccessorGet_2(accessorHandle, &dn, NULL, (SaImmAttrValuesT_2 ***)&attributes);
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOmAccessorGet_2 FAILED %u", rc);
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto done;
	}

	TRACE("'%s'", dn.value);

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

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_MODIFY:
		while ((attrMod = opdata->param.modify.attrMods[i++]) != NULL) {
			if (!strcmp("saAmfNumMaxInstantiateWithoutDelay", attrMod->modAttr.attrName)) {
				TRACE("saAmfNumMaxInstantiateWithoutDelay modified from '%u' to '%u'",
						avd_comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay, 
						*((SaUint32T *)attrMod->modAttr.attrValues[0]));
				avd_comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay =
					*((SaUint32T *)attrMod->modAttr.attrValues[0]);
			}
			if (!strcmp("saAmfNumMaxInstantiateWithDelay", attrMod->modAttr.attrName)) {
				TRACE("saAmfNumMaxInstantiateWithDelay modified from '%u' to '%u'",
						avd_comp_global_attrs.saAmfNumMaxInstantiateWithDelay, 
						*((SaUint32T *)attrMod->modAttr.attrValues[0]));
				avd_comp_global_attrs.saAmfNumMaxInstantiateWithDelay =
					*((SaUint32T *)attrMod->modAttr.attrValues[0]);
			}
			if (!strcmp("saAmfNumMaxAmStartAttempts", attrMod->modAttr.attrName)) {
				TRACE("saAmfNumMaxAmStartAttempts modified from '%u' to '%u'",
						avd_comp_global_attrs.saAmfNumMaxAmStartAttempts, 
						*((SaUint32T *)attrMod->modAttr.attrValues[0]));
				avd_comp_global_attrs.saAmfNumMaxAmStartAttempts =
				    *((SaUint32T *)attrMod->modAttr.attrValues[0]);
			}
			if (!strcmp("saAmfNumMaxAmStopAttempts", attrMod->modAttr.attrName)) {
				TRACE("saAmfNumMaxAmStopAttempts modified from '%u' to '%u'",
						avd_comp_global_attrs.saAmfNumMaxAmStopAttempts, 
						*((SaUint32T *)attrMod->modAttr.attrValues[0]));
				avd_comp_global_attrs.saAmfNumMaxAmStopAttempts =
					*((SaUint32T *)attrMod->modAttr.attrValues[0]);
			}
			if (!strcmp("saAmfDelayBetweenInstantiateAttempts", attrMod->modAttr.attrName)) {
				TRACE("saAmfDelayBetweenInstantiateAttempts modified from '%llu' to '%llu'",
						avd_comp_global_attrs.saAmfDelayBetweenInstantiateAttempts, 
						*((SaTimeT *)attrMod->modAttr.attrValues[0]));
				avd_comp_global_attrs.saAmfDelayBetweenInstantiateAttempts =
					*((SaTimeT *)attrMod->modAttr.attrValues[0]);
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

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

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
	avd_class_impl_set(const_cast<SaImmClassNameT>("SaAmfCompGlobalAttributes"), NULL, NULL,
		avd_compglobalattrs_ccb_completed_cb, avd_compglobalattrs_ccb_apply_cb);
}

