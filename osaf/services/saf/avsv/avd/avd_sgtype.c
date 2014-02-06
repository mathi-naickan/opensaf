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
 *            Ericsson AB
 *
 */

#include <ncspatricia.h>
#include <logtrace.h>
#include <avd_cluster.h>
#include <avd_app.h>
#include <avd_imm.h>
#include <avd_sutype.h>
#include <avd_ckpt_msg.h>
#include <avd_ntf.h>
#include <avd_sg.h>
#include <avd_proc.h>

static NCS_PATRICIA_TREE sgtype_db;

void avd_sgtype_add_sg(AVD_SG *sg)
{
	sg->sg_list_sg_type_next = sg->sg_type->list_of_sg;
	sg->sg_type->list_of_sg = sg;
}

void avd_sgtype_remove_sg(AVD_SG *sg)
{
	AVD_SG *i_sg = NULL;
	AVD_SG *prev_sg = NULL;

	if (sg->sg_type != NULL) {
		i_sg = sg->sg_type->list_of_sg;

		while ((i_sg != NULL) && (i_sg != sg)) {
			prev_sg = i_sg;
			i_sg = i_sg->sg_list_sg_type_next;
		}

		if (i_sg != sg) {
			/* Log a fatal error */
			osafassert(0);
		} else {
			if (prev_sg == NULL) {
				sg->sg_type->list_of_sg = sg->sg_list_sg_type_next;
			} else {
				prev_sg->sg_list_sg_type_next = sg->sg_list_sg_type_next;
			}
		}

		sg->sg_list_sg_type_next = NULL;
		sg->sg_type = NULL;
	}
}

AVD_AMF_SG_TYPE *avd_sgtype_get(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_AMF_SG_TYPE *)ncs_patricia_tree_get(&sgtype_db, (uint8_t *)&tmp);
}

static void sgtype_delete(AVD_AMF_SG_TYPE *sg_type)
{
	(void)ncs_patricia_tree_del(&sgtype_db, &sg_type->tree_node);
	free(sg_type->saAmfSGtValidSuTypes);
	free(sg_type);
}

/**
 * Add the SG type to the DB
 * @param sgt
 */
static void sgtype_add_to_model(AVD_AMF_SG_TYPE *sgt)
{
	unsigned int rc;

	rc = ncs_patricia_tree_add(&sgtype_db, &sgt->tree_node);
	osafassert(rc == NCSCC_RC_SUCCESS);
}

/**
 * Validate configuration attributes for an SaAmfSGType object
 * 
 * @param dn
 * @param sgt
 * @param opdata
 * 
 * @return int
 */
static int is_config_valid(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes,
	const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;
	int i = 0, j;
	char *parent;
	SaUint32T value;
	SaBoolT abool;
	struct avd_sutype *sut;
	const SaImmAttrValuesT_2 *attr;

	if ((parent = strchr((char*)dn->value, ',')) == NULL) {
		LOG_ER("No parent to '%s' ", dn->value);
		return 0;
	}

	/* SGType should be children to SGBaseType */
	if (strncmp(++parent, "safSgType=", 10) != 0) {
		LOG_ER("Wrong parent '%s' to '%s' ", parent, dn->value);
		return 0;
	}

	rc = immutil_getAttr("saAmfSgtRedundancyModel", attributes, 0, &value);
	osafassert(rc == SA_AIS_OK);

	if ((value < SA_AMF_2N_REDUNDANCY_MODEL) || (value > SA_AMF_NO_REDUNDANCY_MODEL)) {
		LOG_ER("Invalid saAmfSgtRedundancyModel %u for '%s'",
			value, dn->value);
		return 0;
	}

	while ((attr = attributes[i++]) != NULL)
		if (!strcmp(attr->attrName, "saAmfSgtValidSuTypes"))
			break;

	osafassert(attr);
	osafassert(attr->attrValuesNumber > 0);

	for (j = 0; j < attr->attrValuesNumber; j++) {
		SaNameT *name = (SaNameT *)attr->attrValues[j];
		sut = avd_sutype_get(name);
		if (sut == NULL) {
			if (opdata == NULL) {
				LOG_ER("'%s' does not exist in model", name->value);
				return 0;
			}
			
			/* SG type does not exist in current model, check CCB */
			if (ccbutil_getCcbOpDataByDN(opdata->ccbId, name) == NULL) {
				LOG_ER("'%s' does not exist either in model or CCB", name->value);
				return 0;
			}
		}
	}

	if (j == 0) {
		LOG_ER("No SU types configured for '%s'", dn->value);
		return 0;
	}

	if ((immutil_getAttr("saAmfSgtDefAutoRepair", attributes, 0, &abool) == SA_AIS_OK) &&
	    (abool > SA_TRUE)) {
		LOG_ER("Invalid saAmfSgtDefAutoRepair %u for '%s'", abool, dn->value);
		return 0;
	}

	if ((immutil_getAttr("saAmfSgtDefAutoAdjust", attributes, 0, &abool) == SA_AIS_OK) &&
	    (abool > SA_TRUE)) {
		LOG_ER("Invalid saAmfSgtDefAutoAdjust %u for '%s'", abool, dn->value);
		return 0;
	}

	return 1;
}

/**
 * Create a new SG type object and initialize its attributes from the attribute list.
 * @param dn
 * @param attributes
 * 
 * @return AVD_AMF_SG_TYPE*
 */
static AVD_AMF_SG_TYPE *sgtype_create(SaNameT *dn, const SaImmAttrValuesT_2 **attributes)
{
	int i = 0, rc = -1, j;
	AVD_AMF_SG_TYPE *sgt;
	const SaImmAttrValuesT_2 *attr;
	SaAisErrorT error;

	TRACE_ENTER2("'%s'", dn->value);

	if ((sgt = calloc(1, sizeof(AVD_AMF_SG_TYPE))) == NULL) {
		LOG_ER("calloc failed");
		return NULL;
	}

	memcpy(sgt->name.value, dn->value, dn->length);
	sgt->name.length = dn->length;
	sgt->tree_node.key_info = (uint8_t *)&(sgt->name);

	error = immutil_getAttr("saAmfSgtRedundancyModel", attributes, 0, &sgt->saAmfSgtRedundancyModel);
	osafassert(error == SA_AIS_OK);

	while ((attr = attributes[i++]) != NULL)
		if (!strcmp(attr->attrName, "saAmfSgtValidSuTypes"))
			break;

	osafassert(attr);
	osafassert(attr->attrValuesNumber > 0);

	sgt->number_su_type = attr->attrValuesNumber;
	sgt->saAmfSGtValidSuTypes = malloc(attr->attrValuesNumber * sizeof(SaNameT));
	for (j = 0; j < attr->attrValuesNumber; j++)
		sgt->saAmfSGtValidSuTypes[j] = *((SaNameT *)attr->attrValues[j]);

	if (immutil_getAttr("saAmfSgtDefAutoRepair", attributes, 0, &sgt->saAmfSgtDefAutoRepair) != SA_AIS_OK) {
		sgt->saAmfSgtDefAutoRepair = SA_TRUE;
	}

	if (immutil_getAttr("saAmfSgtDefAutoAdjust", attributes, 0, &sgt->saAmfSgtDefAutoAdjust) != SA_AIS_OK) {
		sgt->saAmfSgtDefAutoAdjust = SA_FALSE;
	}

	if (immutil_getAttr("saAmfSgtDefAutoAdjustProb", attributes, 0, &sgt->saAmfSgtDefAutoAdjustProb) != SA_AIS_OK) {
		LOG_ER("Get saAmfSgtDefAutoAdjustProb FAILED for '%s'", dn->value);
		goto done;
	}

	if (immutil_getAttr("saAmfSgtDefCompRestartProb", attributes, 0, &sgt->saAmfSgtDefCompRestartProb) != SA_AIS_OK) {
		LOG_ER("Get saAmfSgtDefCompRestartProb FAILED for '%s'", dn->value);
		goto done;
	}

	if (immutil_getAttr("saAmfSgtDefCompRestartMax", attributes, 0, &sgt->saAmfSgtDefCompRestartMax) != SA_AIS_OK) {
		LOG_ER("Get saAmfSgtDefCompRestartMax FAILED for '%s'", dn->value);
		goto done;
	}

	if (immutil_getAttr("saAmfSgtDefSuRestartProb", attributes, 0, &sgt->saAmfSgtDefSuRestartProb) != SA_AIS_OK) {
		LOG_ER("Get saAmfSgtDefSuRestartProb FAILED for '%s'", dn->value);
		goto done;
	}

	if (immutil_getAttr("saAmfSgtDefSuRestartMax", attributes, 0, &sgt->saAmfSgtDefSuRestartMax) != SA_AIS_OK) {
		LOG_ER("Get saAmfSgtDefSuRestartMax FAILED for '%s'", dn->value);
		goto done;
	}

	rc = 0;

 done:
	if (rc != 0) {
		free(sgt->saAmfSGtValidSuTypes);
		free(sgt);
		sgt = NULL;
	}

	return sgt;
}

/**
 * Get configuration for all SaAmfSGType objects from IMM and create AVD internal objects.
 * @param cb
 * 
 * @return int
 */
SaAisErrorT avd_sgtype_config_get(void)
{
	SaAisErrorT error = SA_AIS_ERR_FAILED_OPERATION;
	AVD_AMF_SG_TYPE *sgt;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfSGType";

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, NULL, SA_IMM_SUBTREE,
						  SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
						  NULL, &searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("saImmOmSearchInitialize_2 failed: %u", error);
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		if (!is_config_valid(&dn, attributes, NULL))
			goto done2;
		if (( sgt = avd_sgtype_get(&dn)) == NULL) {
			if ((sgt = sgtype_create(&dn, attributes)) == NULL)
				goto done2;

			sgtype_add_to_model(sgt);
		}
	}

	error = SA_AIS_OK;

 done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
 done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

/**
 * Validate SaAmfSGType CCB completed
 * @param opdata
 * 
 * @return SaAisErrorT
 */
static SaAisErrorT sgtype_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	AVD_AMF_SG_TYPE *sgt;
	AVD_SG *sg; 
	SaBoolT sg_exist = SA_FALSE;
	CcbUtilOperationData_t *t_opData;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(&opdata->objectName, opdata->param.create.attrValues, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		LOG_ER("Modification of SaAmfSGType not supported");
		goto done;
		break;
	case CCBUTIL_DELETE:
		sgt = avd_sgtype_get(&opdata->objectName);
		if (sgt->list_of_sg != NULL) {
			/* check whether there exists a delete operation for 
			 * each of the SG in the sg_type list in the current CCB 
			 */                      
			sg = sgt->list_of_sg;
			while (sg != NULL) {  
				t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, &sg->name);
				if ((t_opData == NULL) || (t_opData->operationType != CCBUTIL_DELETE)) {
					sg_exist = SA_TRUE;   
					break;                  
				}                       
				sg = sg->sg_list_sg_type_next;
			}                       
			if (sg_exist == SA_TRUE) {
				LOG_ER("SGs exist of this SG type '%s'",sgt->name.value);
				goto done;
			}                       
		}
		opdata->userData = sgt;	/* Save for later use in apply */
		rc = SA_AIS_OK;
		break;
	default:
		osafassert(0);
		break;
	}

done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/**
 * Apply SaAmfSGType CCB changes
 * @param opdata
 * 
 * @return SaAisErrorT
 */
static void sgtype_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_AMF_SG_TYPE *sgt;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		sgt = sgtype_create(&opdata->objectName, opdata->param.create.attrValues);
		osafassert(sgt);
		sgtype_add_to_model(sgt);
		break;
	case CCBUTIL_DELETE:
		sgtype_delete(opdata->userData);
		break;
	default:
		osafassert(0);
	}

	TRACE_LEAVE();
}

void avd_sgtype_constructor(void)
{
	NCS_PATRICIA_PARAMS patricia_params;

	patricia_params.key_size = sizeof(SaNameT);
	osafassert(ncs_patricia_tree_init(&sgtype_db, &patricia_params) == NCSCC_RC_SUCCESS);

	avd_class_impl_set("SaAmfSGType", NULL, NULL, sgtype_ccb_completed_cb, sgtype_ccb_apply_cb);
	avd_class_impl_set("SaAmfSGBaseType", NULL, NULL, avd_imm_default_OK_completed_cb, NULL);
}

