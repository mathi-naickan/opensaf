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
#include <avd_util.h>
#include <avd_comp.h>
#include <avd_imm.h>
#include <avd_node.h>
#include <avd_csi.h>
#include <avd_proc.h>
#include <avd_ckpt_msg.h>

static NCS_PATRICIA_TREE compcstype_db;

void avd_compcstype_db_add(AVD_COMPCS_TYPE *cst)
{
	unsigned int rc;

	if (avd_compcstype_get(&cst->name) == NULL) {
		rc = ncs_patricia_tree_add(&compcstype_db, &cst->tree_node);
		osafassert(rc == NCSCC_RC_SUCCESS);
	}
}

AVD_COMPCS_TYPE *avd_compcstype_new(const SaNameT *dn)
{
	AVD_COMPCS_TYPE *compcstype;

	if ((compcstype = calloc(1, sizeof(*compcstype))) == NULL) {
		LOG_ER("calloc FAILED");
		return NULL;
	}
	
	memcpy(compcstype->name.value, dn->value, dn->length);
	compcstype->name.length = dn->length;
	compcstype->tree_node.key_info = (uint8_t *)&(compcstype->name);

	return compcstype;
}

void avd_compcstype_delete(AVD_COMPCS_TYPE **cst)
{
	unsigned int rc;

	rc = ncs_patricia_tree_del(&compcstype_db, &(*cst)->tree_node);
	osafassert(rc == NCSCC_RC_SUCCESS);
	free(*cst);
	*cst = NULL;
}

static void compcstype_add_to_model(AVD_COMPCS_TYPE *cst)
{
	/* Check comp link to see if it has been added already */
	if (cst->comp == NULL) {
		SaNameT dn;
		avd_compcstype_db_add(cst);
		avsv_sanamet_init(&cst->name, &dn, "safComp=");
		cst->comp = avd_comp_get(&dn);
	}
}

/*****************************************************************************
 * Function: avd_compcstype_find_match
 *  
 * Purpose:  This function will verify the the component and CSI are related
 *  in the table.
 *
 * Input: csi -  The CSI whose type need to be matched with the components CSI types list
 *        comp - The component whose list need to be searched.
 *
 * Returns: NCSCC_RC_SUCCESS, NCS_RC_FAILURE.
 *
 * NOTES: This function will be used only while assigning new susi.
 *        In this funtion we will find a match only if the matching comp_cs_type
 *        row status is active.
 **************************************************************************/

AVD_COMPCS_TYPE * avd_compcstype_find_match(const SaNameT *cstype_name, const AVD_COMP *comp)
{
	AVD_COMPCS_TYPE *cst = NULL;
	SaNameT dn;

	TRACE_ENTER();
	avsv_create_association_class_dn(cstype_name, &comp->comp_info.name, "safSupportedCsType", &dn);
	TRACE("'%s'", dn.value);
	cst = (AVD_COMPCS_TYPE *)ncs_patricia_tree_get(&compcstype_db, (uint8_t *)&dn);
	return cst;
}

AVD_COMPCS_TYPE *avd_compcstype_get(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_COMPCS_TYPE *)ncs_patricia_tree_get(&compcstype_db, (uint8_t *)&tmp);
}

AVD_COMPCS_TYPE *avd_compcstype_getnext(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, dn->length);

	return (AVD_COMPCS_TYPE *)ncs_patricia_tree_getnext(&compcstype_db, (uint8_t *)&tmp);
}

/**
 * Validate configuration attributes for an SaAmfCompCsType object
 * @param cst
 * 
 * @return int
 */
static int is_config_valid(const SaNameT *dn, CcbUtilOperationData_t *opdata)
{
	SaNameT comp_name;
	const SaNameT *comptype_name;
	SaNameT ctcstype_name = {0};
	char *parent;
	AVD_COMP *comp;
	char *cstype_name = strdup((char*)dn->value);
	char *p;

	/* This is an association class, the parent (SaAmfComp) should come after the second comma */
	if ((parent = strchr((char*)dn->value, ',')) == NULL) {
		LOG_ER("No parent to '%s' ", dn->value);
		goto free_cstype_name;
	}

	parent++;

	/* Second comma should be the parent */
	if ((parent = strchr(parent, ',')) == NULL) {
		LOG_ER("No parent to '%s' ", dn->value);
		goto free_cstype_name;
	}

	parent++;

	/* Should be children to SaAmfComp */
	if (strncmp(parent, "safComp=", 8) != 0) {
		LOG_ER("Wrong parent '%s'", parent);
		goto free_cstype_name;
	}

	/* 
	** Validate that the corresponding SaAmfCtCsType exist. This is required
	** to create the SaAmfCompCsType instance later.
	*/

	avsv_sanamet_init(dn, &comp_name, "safComp=");
	comp = avd_comp_get(&comp_name);

	if (comp != NULL)
		comptype_name = &comp->saAmfCompType;
	else {
		CcbUtilOperationData_t *tmp;

		if (opdata == NULL) {
			LOG_ER("'%s' does not exist in model", comp_name.value);
			goto free_cstype_name;
		}

		if ((tmp = ccbutil_getCcbOpDataByDN(opdata->ccbId, &comp_name)) == NULL) {
			LOG_ER("'%s'does not exist in existing model or in CCB", comp_name.value);
			osafassert(0);
			goto free_cstype_name;
		}

		comptype_name = immutil_getNameAttr(tmp->param.create.attrValues, "saAmfCompType", 0);
	}
	osafassert(comptype_name);

	p = strchr(cstype_name, ',') + 1;
	p = strchr(p, ',');
	*p = '\0';

	ctcstype_name.length = sprintf((char*)ctcstype_name.value, "%s,%s", cstype_name, comptype_name->value);

	if (avd_ctcstype_get(&ctcstype_name) == NULL) {
		if (opdata == NULL) {
			LOG_ER("'%s' does not exist in model", ctcstype_name.value);
			goto free_cstype_name;
		}

		if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &ctcstype_name) == NULL) {
			LOG_ER("'%s' does not exist in existing model or in CCB", ctcstype_name.value);
			goto free_cstype_name;
		}
	}

	free(cstype_name);
	return 1;

free_cstype_name:
	free(cstype_name);
	return 0;
}

static AVD_COMPCS_TYPE *compcstype_create(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes)
{
	int rc = -1;
	AVD_COMPCS_TYPE *compcstype;
	AVD_CTCS_TYPE *ctcstype;
	SaNameT ctcstype_name = {0};
	char *cstype_name = strdup((char*)dn->value);
	char *p;
	SaNameT comp_name;
	AVD_COMP *comp;
	SaUint32T num_max_act_csi, num_max_std_csi;

	TRACE_ENTER2("'%s'", dn->value);

	/*
	** If called at new active at failover, the object is found in the DB
	** but needs to get configuration attributes initialized.
	*/
	if ((NULL == (compcstype = avd_compcstype_get(dn))) &&
	    ((compcstype = avd_compcstype_new(dn)) == NULL))
		goto done;

	avsv_sanamet_init(dn, &comp_name, "safComp=");
	comp = avd_comp_get(&comp_name);

	p = strchr(cstype_name, ',') + 1;
	p = strchr(p, ',');
	*p = '\0';
	ctcstype_name.length = sprintf((char*)ctcstype_name.value,
		"%s,%s", cstype_name, comp->comp_type->name.value);

	ctcstype = avd_ctcstype_get(&ctcstype_name);

	if (immutil_getAttr("saAmfCompNumMaxActiveCSIs", attributes, 0,
				&num_max_act_csi) != SA_AIS_OK) {
		compcstype->saAmfCompNumMaxActiveCSIs = ctcstype->saAmfCtDefNumMaxActiveCSIs;
	} else {
		compcstype->saAmfCompNumMaxActiveCSIs = num_max_act_csi;
		if (compcstype->saAmfCompNumMaxActiveCSIs > ctcstype->saAmfCtDefNumMaxActiveCSIs) {
			compcstype->saAmfCompNumMaxActiveCSIs = ctcstype->saAmfCtDefNumMaxActiveCSIs;
		}
	}

	if (immutil_getAttr("saAmfCompNumMaxStandbyCSIs", attributes, 0,
				&num_max_std_csi) != SA_AIS_OK) {
		compcstype->saAmfCompNumMaxStandbyCSIs = ctcstype->saAmfCtDefNumMaxStandbyCSIs;
	} else {
		compcstype->saAmfCompNumMaxStandbyCSIs = num_max_std_csi;
		if (compcstype->saAmfCompNumMaxStandbyCSIs > ctcstype->saAmfCtDefNumMaxStandbyCSIs) {
			compcstype->saAmfCompNumMaxStandbyCSIs = ctcstype->saAmfCtDefNumMaxStandbyCSIs;
		}
	}

	rc = 0;

done:
	if (rc != 0)
		avd_compcstype_delete(&compcstype);
	free(cstype_name);

	return compcstype;
}

/**
 * Get configuration for all AMF CompCsType objects from IMM and
 * create AVD internal objects.
 * 
 * @param cb
 * @param comp
 * 
 * @return int
 */
SaAisErrorT avd_compcstype_config_get(SaNameT *comp_name, AVD_COMP *comp)
{
	SaAisErrorT error;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfCompCsType";
	AVD_COMPCS_TYPE *compcstype;
	SaImmAttrNameT attributeNames[] = {"saAmfCompNumMaxActiveCSIs", "saAmfCompNumMaxStandbyCSIs", NULL};

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, comp_name, SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR, &searchParam,
		attributeNames, &searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("saImmOmSearchInitialize_2 failed: %u", error);
		goto done1;
	}

	while ((error = immutil_saImmOmSearchNext_2(searchHandle, &dn,
		(SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK) {

		if (!is_config_valid(&dn, NULL)) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}

		if ((compcstype = compcstype_create(&dn, attributes)) == NULL) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}
		compcstype_add_to_model(compcstype);
	}

	error = SA_AIS_OK;

done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

static SaAisErrorT compcstype_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	AVD_COMPCS_TYPE *cst;
	AVD_COMP *comp;
	SaNameT comp_name;
	AVD_SU_SI_REL *curr_susi;
	AVD_COMP_CSI_REL *compcsi;


	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE: {
		if (is_config_valid(&opdata->objectName, opdata))
			rc = SA_AIS_OK;
		break;
	}
	case CCBUTIL_MODIFY:
		LOG_ER("Modification of SaAmfCompCsType not supported");
		break;
	case CCBUTIL_DELETE:
		cst = avd_compcstype_get(&opdata->objectName);
		osafassert(cst);
		avsv_sanamet_init(&opdata->objectName, &comp_name, "safComp=");
		comp = avd_comp_get(&comp_name);
		for (curr_susi = comp->su->list_of_susi; curr_susi != NULL; curr_susi = curr_susi->su_next)
			for (compcsi = curr_susi->list_of_csicomp; compcsi; compcsi = compcsi->susi_csicomp_next) {
				if (compcsi->comp == comp) {
					LOG_ER("Deletion of SaAmfCompCsType requires "
							" comp without any csi assignment");
					goto done;
				}
			}
		rc = SA_AIS_OK;
		break;
	default:
		osafassert(0);
		break;
	}
done:
	return rc;
}

static void compcstype_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_COMPCS_TYPE *cst;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		cst = compcstype_create(&opdata->objectName, opdata->param.create.attrValues);
		osafassert(cst);
		compcstype_add_to_model(cst);
		break;
	case CCBUTIL_DELETE:
		cst = avd_compcstype_get(&opdata->objectName);
		avd_compcstype_delete(&cst);
		break;
	default:
		osafassert(0);
		break;
	}
}

static SaAisErrorT compcstype_rt_attr_callback(SaImmOiHandleT immOiHandle,
	const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	AVD_COMPCS_TYPE *cst = avd_compcstype_get(objectName);
	SaImmAttrNameT attributeName;
	int i = 0;

	TRACE("%s", objectName->value);
	osafassert(cst != NULL);

	while ((attributeName = attributeNames[i++]) != NULL) {
		if (!strcmp("saAmfCompNumCurrActiveCSIs", attributeName)) {
			avd_saImmOiRtObjectUpdate_sync(objectName, attributeName,
				SA_IMM_ATTR_SAUINT32T, &cst->saAmfCompNumCurrActiveCSIs);
		} else if (!strcmp("saAmfCompNumCurrStandbyCSIs", attributeName)) {
			avd_saImmOiRtObjectUpdate(objectName, attributeName,
				SA_IMM_ATTR_SAUINT32T, &cst->saAmfCompNumCurrStandbyCSIs);
		} else if (!strcmp("saAmfCompAssignedCsi", attributeName)) {
			/* TODO */
		} else
			osafassert(0);
	}

	return SA_AIS_OK;
}

void avd_compcstype_constructor(void)
{
	NCS_PATRICIA_PARAMS patricia_params;

	patricia_params.key_size = sizeof(SaNameT);
	osafassert(ncs_patricia_tree_init(&compcstype_db, &patricia_params) == NCSCC_RC_SUCCESS);
	avd_class_impl_set("SaAmfCompCsType", compcstype_rt_attr_callback, NULL,
		compcstype_ccb_completed_cb, compcstype_ccb_apply_cb);
}

