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

#include <avsv_util.h>
#include <avd_comp.h>
#include <avd_imm.h>

/* Global variable for the singleton object used by comp class */
AVD_COMP_GLOBALATTR avd_comp_global_attrs;

static NCS_PATRICIA_TREE comptype_db;

static void comptype_db_add(AVD_COMP_TYPE *compt)
{
	unsigned int rc = ncs_patricia_tree_add(&comptype_db, &compt->tree_node);
	assert (rc == NCSCC_RC_SUCCESS);
}

AVD_COMP_TYPE *avd_comptype_get(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_COMP_TYPE *)ncs_patricia_tree_get(&comptype_db, (uns8 *)&tmp);
}

static void comptype_delete(AVD_COMP_TYPE *avd_comp_type)
{
	unsigned int rc;

	assert(NULL == avd_comp_type->list_of_comp);

	rc = ncs_patricia_tree_del(&comptype_db, &avd_comp_type->tree_node);
	assert(rc == NCSCC_RC_SUCCESS);

	free(avd_comp_type);
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

	if ((compt = calloc(1, sizeof(AVD_COMP_TYPE))) == NULL) {
		LOG_ER("calloc FAILED");
		return NULL;
	}

	memcpy(compt->name.value, dn->value, dn->length);
	compt->name.length = dn->length;
	compt->tree_node.key_info = (uns8 *)&(compt->name);

	error = immutil_getAttr("saAmfCtCompCategory", attributes, 0, &compt->saAmfCtCompCategory);
	assert(error == SA_AIS_OK);

	(void)immutil_getAttr("saAmfCtSwBundle", attributes, 0, &compt->saAmfCtSwBundle);

	if (!IS_COMP_PROXIED(compt->saAmfCtCompCategory) && IS_COMP_LOCAL(compt->saAmfCtCompCategory)) {
		error = immutil_getAttr("saAmfCtSwBundle", attributes, 0, &compt->saAmfCtSwBundle);
		assert(error == SA_AIS_OK);
	}

	if ((str = immutil_getStringAttr(attributes, "saAmfCtDefCmdEnv", 0)) != NULL)
		strcpy(compt->saAmfCtDefCmdEnv, str);
	(void)immutil_getAttr("saAmfCtDefClcCliTimeout", attributes, 0, &compt->saAmfCtDefClcCliTimeout);
	(void)immutil_getAttr("saAmfCtDefCallbackTimeout", attributes, 0, &compt->saAmfCtDefCallbackTimeout);

	if ((str = immutil_getStringAttr(attributes, "saAmfCtRelPathInstantiateCmd", 0)) != NULL)
		strcpy(compt->saAmfCtRelPathInstantiateCmd, str);
	if ((str = immutil_getStringAttr(attributes, "saAmfCtDefInstantiateCmdArgv", 0)) != NULL)
		strcpy(compt->saAmfCtDefInstantiateCmdArgv, str);

	(void)immutil_getAttr("saAmfCtDefInstantiationLevel", attributes, 0, &compt->saAmfCtDefInstantiationLevel);

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

	(void)immutil_getAttr("saAmfCtDefQuiescingCompleteTimeout", attributes, 0,
			      &compt->saAmfCompQuiescingCompleteTimeout);

	error = immutil_getAttr("saAmfCtDefRecoveryOnError", attributes, 0, &compt->saAmfCtDefRecoveryOnError);
	assert(error == SA_AIS_OK);

	(void)immutil_getAttr("saAmfCtDefDisableRestart", attributes, 0, &compt->saAmfCtDefDisableRestart);

	return compt;
}

static int is_config_valid(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
	SaUint32T category;
	SaUint32T uint32;
	char *parent;
	SaNameT name;
	SaTimeT time;
	SaAisErrorT rc;

	if ((parent = strchr((char*)dn->value, ',')) == NULL) {
		LOG_ER("No parent to '%s' ", dn->value);
		return 0;
	}

	/* Should be children to the Comp Base type */
	if (strncmp(++parent, "safCompType=", 12) != 0) {
		LOG_ER("Wrong parent '%s' to '%s' ", parent, dn->value);
		return 0;
	}

	rc = immutil_getAttr("saAmfCtCompCategory", attributes, 0, &category);
	assert(rc = SA_AIS_OK);

	/* We do not support Proxy, Container and Contained as of now. */
	if (IS_COMP_PROXY(category) || IS_COMP_CONTAINER(category)|| IS_COMP_CONTAINED(category)) {
		LOG_ER("Unsupported saAmfCtCompCategory value '%u' for '%s'", category, dn->value);
		return 0;	
	}

	/*                                                                                                  
        ** The saAmfCtDefClcCliTimeout attribute is "mandatory for all components except for
	** external components"
        */
	if (!IS_COMP_LOCAL(category) &&
	    (immutil_getAttr("saAmfCtDefClcCliTimeout", attributes, 0, &time) != SA_AIS_OK)) {
		LOG_ER("Required attribute saAmfCtDefClcCliTimeout not configured for '%s'", dn->value);
		return 0;	
	}

	/*                                                                                                  
        ** The saAmfCtDefCallbackTimeout attribute "mandatory for all components except for
	** non-proxied, non-SA-aware components"                                                                                                    
        */
	if (!IS_COMP_PROXIED(category) && !IS_COMP_SAAWARE(category) &&
	    (immutil_getAttr("saAmfCtDefCallbackTimeout", attributes, 0, &time) != SA_AIS_OK)) {
		LOG_ER("Required attribute saAmfCtDefCallbackTimeout not configured for '%s'", dn->value);
		return 0;	
	}

	/* 
	** The saAmfCtSwBundle/saAmfCtRelPathInstantiateCmd "attribute is mandatory for all
	** non-proxied local components".
	*/
	if (!(IS_COMP_PROXIED(category) || IS_COMP_PROXIED_NPI(category)) && IS_COMP_LOCAL(category)) {

		if (immutil_getAttr("saAmfCtSwBundle", attributes, 0, &name) != SA_AIS_OK) {
			LOG_ER("Required attribute saAmfCtSwBundle not configured for '%s'", dn->value);
			return 0;	
		}

		if (immutil_getStringAttr(attributes, "saAmfCtRelPathInstantiateCmd", 0) == NULL) {
			LOG_ER("Required attribute saAmfCtRelPathInstantiateCmd not configured for '%s'", dn->value);
			return 0;	
		}
	}

	/*
	** The saAmfCtDefTerminateCmdArgv "attribute is mandatory for local non-proxied,
	** non-SA-aware components".
	*/
	if (IS_COMP_LOCAL(category) && !(IS_COMP_PROXIED(category) || IS_COMP_PROXIED_NPI(category)) && !IS_COMP_SAAWARE(category) &&
	    (immutil_getStringAttr(attributes, "saAmfCtDefTerminateCmdArgv", 0) == NULL)) {
		LOG_ER("Required attribute saAmfCtDefTerminateCmdArgv not configured for '%s', cat=%x", dn->value, category);
		return 0;	
	}

	/*
	** The saAmfCtRelPathCleanupCmd "attribute is mandatory for all local components (proxied or
	** non-proxied)"
	*/
	if (IS_COMP_LOCAL(category) && 
	    (immutil_getStringAttr(attributes, "saAmfCtRelPathCleanupCmd", 0) == NULL)) {
		LOG_ER("Required attribute saAmfCtRelPathCleanupCmd not configured for '%s'", dn->value);
		return 0;	
	}

	rc = immutil_getAttr("saAmfCtDefRecoveryOnError", attributes, 0, &uint32);
	assert(rc == SA_AIS_OK);

	if (uint32 > SA_AMF_CONTAINER_RESTART) {
		LOG_ER("Wrong saAmfCtDefRecoveryOnError value '%u'", uint32);
		return 0;
	}

	return 1;
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

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
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
		if (!is_config_valid(&dn, attributes, NULL))
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
		assert(comp_type);
		comptype_db_add(comp_type);
		break;
	case CCBUTIL_DELETE:
		comptype_delete(opdata->userData);
		break;
	default:
		assert(0);
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
		if (is_config_valid(&opdata->objectName, opdata->param.create.attrValues, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		LOG_ER("Modification of SaAmfCompType not supported");
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
				LOG_ER("SaAmfCompType '%s' is in use",comp_type->name.value);
				goto done;
			}
		}
		opdata->userData = comp_type;
		rc = SA_AIS_OK;
		break;
	default:
		assert(0);
		break;
	}

done:
	return rc;
}

void avd_comptype_constructor(void)
{
	NCS_PATRICIA_PARAMS patricia_params;

	patricia_params.key_size = sizeof(SaNameT);
	assert(ncs_patricia_tree_init(&comptype_db, &patricia_params) == NCSCC_RC_SUCCESS);

	avd_class_impl_set("SaAmfCompBaseType", NULL, NULL, avd_imm_default_OK_completed_cb, NULL);
	avd_class_impl_set("SaAmfCompType", NULL, NULL,	comptype_ccb_completed_cb, comptype_ccb_apply_cb);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

SaAisErrorT avd_compglobalattrs_config_get(void)
{
	SaAisErrorT rc;
	const SaImmAttrValuesT_2 **attributes;
	SaImmAccessorHandleT accessorHandle;
	SaNameT dn = {.value = "safRdn=compGlobalAttributes,safApp=safAmfService" };

	dn.length = strlen((char *)dn.value);

	immutil_saImmOmAccessorInitialize(avd_cb->immOmHandle, &accessorHandle);
	rc = immutil_saImmOmAccessorGet_2(accessorHandle, &dn, NULL, (SaImmAttrValuesT_2 ***)&attributes);
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOmAccessorGet_2 FAILED %u", rc);
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto done;
	}

	TRACE("'%s'", dn.value);

	if (immutil_getAttr("saAmfNumMaxInstantiateWithoutDelay", attributes, 0,
			    &avd_comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay) != SA_AIS_OK) {
		avd_comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay = 2;
	}

	if (immutil_getAttr("saAmfNumMaxInstantiateWithDelay", attributes, 0,
			    &avd_comp_global_attrs.saAmfNumMaxInstantiateWithDelay) != SA_AIS_OK) {
		avd_comp_global_attrs.saAmfNumMaxInstantiateWithDelay = 0;
	}

	if (immutil_getAttr("saAmfNumMaxAmStartAttempts", attributes, 0,
			    &avd_comp_global_attrs.saAmfNumMaxAmStartAttempts) != SA_AIS_OK) {
		avd_comp_global_attrs.saAmfNumMaxAmStartAttempts = 2;
	}

	if (immutil_getAttr("saAmfNumMaxAmStopAttempts", attributes, 0,
			    &avd_comp_global_attrs.saAmfNumMaxAmStopAttempts) != SA_AIS_OK) {
		avd_comp_global_attrs.saAmfNumMaxAmStopAttempts = 2;
	}

	if (immutil_getAttr("saAmfDelayBetweenInstantiateAttempts", attributes, 0,
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
				avd_comp_global_attrs.saAmfNumMaxInstantiateWithoutDelay =
				    *((SaUint32T *)attrMod->modAttr.attrValues[0]);
			}
			if (!strcmp("saAmfNumMaxInstantiateWithDelay", attrMod->modAttr.attrName)) {
				avd_comp_global_attrs.saAmfNumMaxInstantiateWithDelay =
				    *((SaUint32T *)attrMod->modAttr.attrValues[0]);
			}
			if (!strcmp("saAmfNumMaxAmStartAttempts", attrMod->modAttr.attrName)) {
				avd_comp_global_attrs.saAmfNumMaxAmStartAttempts =
				    *((SaUint32T *)attrMod->modAttr.attrValues[0]);
			}
			if (!strcmp("saAmfNumMaxAmStopAttempts", attrMod->modAttr.attrName)) {
				avd_comp_global_attrs.saAmfNumMaxAmStopAttempts =
				    *((SaUint32T *)attrMod->modAttr.attrValues[0]);
			}
			if (!strcmp("saAmfDelayBetweenInstantiateAttempts", attrMod->modAttr.attrName)) {
				avd_comp_global_attrs.saAmfDelayBetweenInstantiateAttempts =
				    *((SaTimeT *)attrMod->modAttr.attrValues[0]);
			}
		}
		break;
	default:
		assert(0);
		break;
	}
}

static SaAisErrorT avd_compglobalattrs_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		LOG_ER("SaAmfCompGlobalAttributes already created");
		break;
	case CCBUTIL_MODIFY:
		rc = SA_AIS_OK;
		break;
	case CCBUTIL_DELETE:
		LOG_ER("SaAmfCompGlobalAttributes cannot be deleted");
		break;
	default:
		assert(0);
		break;
	}

	return rc;
}

void avd_compglobalattrs_constructor(void)
{
	avd_class_impl_set("SaAmfCompGlobalAttributes", NULL, NULL,
		avd_compglobalattrs_ccb_completed_cb, avd_compglobalattrs_ccb_apply_cb);
}

