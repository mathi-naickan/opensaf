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

#include <logtrace.h>

#include <avd_util.h>
#include <avd_cluster.h>
#include <avd_app.h>
#include <avd_imm.h>
#include <avd_sutype.h>
#include <avd_ckpt_msg.h>
#include <avd_ntf.h>
#include <avd_sg.h>
#include <avd_proc.h>
#include <avd_si_dep.h>

static NCS_PATRICIA_TREE sg_db;
static void avd_verify_equal_ranked_su(AVD_SG *avd_sg);

void avd_sg_db_add(AVD_SG *sg)
{
	unsigned int rc;

	if (avd_sg_get(&sg->name) == NULL) {
		rc = ncs_patricia_tree_add(&sg_db, &sg->tree_node);
		osafassert(rc == NCSCC_RC_SUCCESS);
	}
}

void avd_sg_db_remove(AVD_SG *sg)
{
	unsigned int rc = ncs_patricia_tree_del(&sg_db, &sg->tree_node);
	osafassert(rc == NCSCC_RC_SUCCESS);
}

/**
 * Add the SG to the model
 * @param sg
 */
static void sg_add_to_model(AVD_SG *sg)
{
	SaNameT dn;

	TRACE_ENTER2("%s", sg->name.value);

	/* Check parent link to see if it has been added already */
	if (sg->app != NULL) {
		TRACE("already added");
		goto done;
	}

	avsv_sanamet_init(&sg->name, &dn, "safApp");
	sg->app = avd_app_get(&dn);

	avd_sg_db_add(sg);
	sg->sg_type = avd_sgtype_get(&sg->saAmfSGType);
	osafassert(sg->sg_type);
	avd_sgtype_add_sg(sg);
	avd_app_add_sg(sg->app, sg);

	/* SGs belonging to a magic app will be NCS, TODO Better name! */
	if (!strcmp((char *)dn.value, "safApp=OpenSAF"))
		sg->sg_ncs_spec = true;

	m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(avd_cb, sg, AVSV_CKPT_AVD_SG_CONFIG);
done:
	TRACE_LEAVE();
}

/**
 * Remove the SG to the model
 * @param sg
 */
static void sg_remove_from_model(AVD_SG *sg)
{
	avd_sgtype_remove_sg(sg);
	avd_app_remove_sg(sg->app, sg);
	avd_sg_db_remove(sg);

	m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, sg, AVSV_CKPT_AVD_SG_CONFIG);
}

AVD_SG *avd_sg_new(const SaNameT *dn)
{
	AVD_SG *sg;

	if ((sg = calloc(1, sizeof(AVD_SG))) == NULL) {
		LOG_ER("calloc FAILED");
		return NULL;
	}

	memcpy(sg->name.value, dn->value, dn->length);
	sg->name.length = dn->length;
	sg->tree_node.key_info = (uint8_t *)&(sg->name);
	sg->sg_ncs_spec = SA_FALSE;
	sg->sg_fsm_state = AVD_SG_FSM_STABLE;
	sg->adjust_state = AVSV_SG_STABLE;

	return sg;
}

void avd_sg_delete(AVD_SG *sg)
{
	/* by now SU and SI should have been deleted */ 
	osafassert(sg->list_of_su == NULL);
	osafassert(sg->list_of_si == NULL);
	sg_remove_from_model(sg);
	free(sg);
}

void avd_sg_add_si(AVD_SG *sg, AVD_SI* si)
{
	AVD_SI *i_si = AVD_SI_NULL;
	AVD_SI *prev_si = AVD_SI_NULL;

	i_si = sg->list_of_si;

	while ((i_si != AVD_SI_NULL) && (i_si->saAmfSIRank < si->saAmfSIRank)) {
		prev_si = i_si;
		i_si = i_si->sg_list_of_si_next;
	}

	if (prev_si == AVD_SI_NULL) {
		si->sg_list_of_si_next = si->sg_of_si->list_of_si;
		sg->list_of_si = si;
	} else {
		prev_si->sg_list_of_si_next = si;
		si->sg_list_of_si_next = i_si;
	}
}

void avd_sg_remove_si(AVD_SG *sg, AVD_SI* si)
{
	AVD_SI *i_si = NULL;
	AVD_SI *prev_si = NULL;

	if (!sg)
		return;

	if (sg->list_of_si != NULL) {
		i_si = sg->list_of_si;

		while ((i_si != NULL) && (i_si != si)) {
			prev_si = i_si;
			i_si = i_si->sg_list_of_si_next;
		}

		if (i_si == si) {
			if (prev_si == NULL) {
				sg->list_of_si = si->sg_list_of_si_next;
			} else {
				prev_si->sg_list_of_si_next = si->sg_list_of_si_next;
			}

			si->sg_list_of_si_next = NULL;
			si->sg_of_si = NULL;
		}
	}
}

AVD_SG *avd_sg_get(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_SG *)ncs_patricia_tree_get(&sg_db, (uint8_t *)&tmp);
}

AVD_SG *avd_sg_getnext(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_SG *)ncs_patricia_tree_getnext(&sg_db, (uint8_t *)&tmp);
}

/**
 * Validate configuration attributes for an SaAmfSG object
 * 
 * @param dn
 * @param sg
 * 
 * @return int
 */
static int is_config_valid(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;
	SaNameT aname;
	SaBoolT abool;
	SaAmfAdminStateT admstate;
	char *parent;

	if ((parent = strchr((char*)dn->value, ',')) == NULL) {
		LOG_ER("No parent to '%s' ", dn->value);
		return 0;
	}

	if (strncmp(++parent, "safApp=", 7) != 0) {
		LOG_ER("Wrong parent '%s' to '%s' ", parent, dn->value);
		return 0;
	}

	rc = immutil_getAttr("saAmfSGType", attributes, 0, &aname);
	osafassert(rc == SA_AIS_OK);

	if (avd_sgtype_get(&aname) == NULL) {
		if (opdata == NULL) {
			LOG_ER("'%s' does not exist in model", aname.value);
			return 0;
		}

		/* SG type does not exist in current model, check CCB */
		if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &aname) == NULL) {
			LOG_ER("'%s' does not exist in existing model or in CCB", aname.value);
			return 0;
		}
	}

	if ((immutil_getAttr("saAmfSGAutoRepair", attributes, 0, &abool) == SA_AIS_OK) &&
	    (abool > SA_TRUE)) {
		LOG_ER("Invalid saAmfSGAutoRepair %u for '%s'", abool, dn->value);
		return 0;
	}

	if ((immutil_getAttr("saAmfSGAutoAdjust", attributes, 0, &abool) == SA_AIS_OK) &&
	    (abool > SA_TRUE)) {
		LOG_ER("Invalid saAmfSGAutoAdjust %u for '%s'", abool, dn->value);
		return 0;
	}

	if ((immutil_getAttr("saAmfSGAdminState", attributes, 0, &admstate) == SA_AIS_OK) &&
	    !avd_admin_state_is_valid(admstate)) {
		LOG_ER("Invalid saAmfSGAdminState %u for '%s'", admstate, dn->value);
		return 0;
	}

	if ((immutil_getAttr("saAmfSGSuHostNodeGroup", attributes, 0, &aname) == SA_AIS_OK) &&
	    (avd_ng_get(&aname) == NULL)) {
		LOG_ER("Invalid saAmfSGSuHostNodeGroup '%s' for '%s'", aname.value, dn->value);
		return 0;
	}

	return 1;
}

static AVD_SG *sg_create(const SaNameT *sg_name, const SaImmAttrValuesT_2 **attributes)
{
	int rc = -1;
	AVD_SG *sg;
	AVD_AMF_SG_TYPE *sgt;
	SaAisErrorT error;

	TRACE_ENTER2("'%s'", sg_name->value);

	/*
	** If called at new active at failover, the object is found in the DB
	** but needs to get configuration attributes initialized.
	*/
	if (NULL == (sg = avd_sg_get(sg_name))) {
		if ((sg = avd_sg_new(sg_name)) == NULL)
			goto done;
	} else
		TRACE("already created, refreshing config...");


	error = immutil_getAttr("saAmfSGType", attributes, 0, &sg->saAmfSGType);
	osafassert(error == SA_AIS_OK);

	sgt = avd_sgtype_get(&sg->saAmfSGType);
	osafassert(sgt);
	sg->sg_type = sgt;

	(void)immutil_getAttr("saAmfSGSuHostNodeGroup", attributes, 0, &sg->saAmfSGSuHostNodeGroup);

	if (immutil_getAttr("saAmfSGAutoRepair", attributes, 0, &sg->saAmfSGAutoRepair) != SA_AIS_OK) {
		sg->saAmfSGAutoRepair = sgt->saAmfSgtDefAutoRepair;
	}

	if (immutil_getAttr("saAmfSGAutoAdjust", attributes, 0, &sg->saAmfSGAutoAdjust) != SA_AIS_OK) {
		sg->saAmfSGAutoAdjust = sgt->saAmfSgtDefAutoAdjust;
	}

	if (immutil_getAttr("saAmfSGNumPrefActiveSUs", attributes, 0, &sg->saAmfSGNumPrefActiveSUs) != SA_AIS_OK) {
		sg->saAmfSGNumPrefActiveSUs = 1;
	}

	if (immutil_getAttr("saAmfSGNumPrefStandbySUs", attributes, 0, &sg->saAmfSGNumPrefStandbySUs) != SA_AIS_OK) {
		sg->saAmfSGNumPrefStandbySUs = 1;
	}

	if (immutil_getAttr("saAmfSGNumPrefInserviceSUs", attributes, 0, &sg->saAmfSGNumPrefInserviceSUs) != SA_AIS_OK) {
		/* empty => later assign number of SUs */
	}

	if (immutil_getAttr("saAmfSGNumPrefAssignedSUs", attributes, 0, &sg->saAmfSGNumPrefAssignedSUs) != SA_AIS_OK) {
		/* empty => later assign to saAmfSGNumPrefInserviceSUs */
	}

	sg->saAmfSGMaxActiveSIsperSU = -1; // magic number for no limit

	/* saAmfSGMaxActiveSIsperSU: "This attribute is only applicable to N+M, N-way, and N-way active redundancy models." */
	if ((sgt->saAmfSgtRedundancyModel == SA_AMF_NPM_REDUNDANCY_MODEL) ||
	    (sgt->saAmfSgtRedundancyModel == SA_AMF_N_WAY_REDUNDANCY_MODEL) ||
	    (sgt->saAmfSgtRedundancyModel == SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL)) {
		(void) immutil_getAttr("saAmfSGMaxActiveSIsperSU", attributes, 0, &sg->saAmfSGMaxActiveSIsperSU);
	} else {
		SaUint32T tmp;
		if (immutil_getAttr("saAmfSGMaxActiveSIsperSU", attributes, 0, &tmp) == SA_AIS_OK) {
			LOG_NO("'%s' attribute saAmfSGMaxActiveSIsperSU ignored, not valid for red model",
				sg->name.value);
		}
	}

	sg->saAmfSGMaxStandbySIsperSU = -1; // magic number for no limit

	/* saAmfSGMaxStandbySIsperSU: "This attribute is only applicable to N+M and N-way redundancy models." */
	if ((sgt->saAmfSgtRedundancyModel == SA_AMF_NPM_REDUNDANCY_MODEL) ||
	    (sgt->saAmfSgtRedundancyModel == SA_AMF_N_WAY_REDUNDANCY_MODEL)) {
		(void) immutil_getAttr("saAmfSGMaxStandbySIsperSU", attributes, 0, &sg->saAmfSGMaxStandbySIsperSU);
	} else {
		SaUint32T tmp;
		if (immutil_getAttr("saAmfSGMaxStandbySIsperSU", attributes, 0, &tmp) == SA_AIS_OK) {
			LOG_NO("'%s' attribute saAmfSGMaxStandbySIsperSU ignored, not valid for red model",
				sg->name.value);
		}
	}

	if (immutil_getAttr("saAmfSGAutoAdjustProb", attributes, 0, &sg->saAmfSGAutoAdjustProb) != SA_AIS_OK) {
		sg->saAmfSGAutoAdjustProb = sgt->saAmfSgtDefAutoAdjustProb;
	}

	if (immutil_getAttr("saAmfSGCompRestartProb", attributes, 0, &sg->saAmfSGCompRestartProb) != SA_AIS_OK) {
		sg->saAmfSGCompRestartProb = sgt->saAmfSgtDefCompRestartProb;
	}

	if (immutil_getAttr("saAmfSGCompRestartMax", attributes, 0, &sg->saAmfSGCompRestartMax) != SA_AIS_OK) {
		sg->saAmfSGCompRestartMax = sgt->saAmfSgtDefCompRestartMax;
	}

	if (immutil_getAttr("saAmfSGSuRestartProb", attributes, 0, &sg->saAmfSGSuRestartProb) != SA_AIS_OK) {
		sg->saAmfSGSuRestartProb = sgt->saAmfSgtDefSuRestartProb;
	}

	if (immutil_getAttr("saAmfSGSuRestartMax", attributes, 0, &sg->saAmfSGSuRestartMax) != SA_AIS_OK) {
		sg->saAmfSGSuRestartMax = sgt->saAmfSgtDefSuRestartMax;
	}

	if (immutil_getAttr("saAmfSGAdminState", attributes, 0, &sg->saAmfSGAdminState) != SA_AIS_OK) {
		sg->saAmfSGAdminState = SA_AMF_ADMIN_UNLOCKED;
	}

	/*  TODO use value in type instead? */
	sg->sg_redundancy_model = sgt->saAmfSgtRedundancyModel;

	rc = 0;

done:
	if (rc != 0) {
		avd_sg_delete(sg);
		sg = NULL;
	}

	return sg;
}

/**
 * Get configuration for all AMF SG objects from IMM and
 * create AVD internal objects.
 * @param cb
 * 
 * @return int
 */
SaAisErrorT avd_sg_config_get(const SaNameT *app_dn, AVD_APP *app)
{
	AVD_SG *sg;
	SaAisErrorT error, rc;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfSG";
	SaImmAttrNameT configAttributes[] = {
		"saAmfSGType",
		"saAmfSGSuHostNodeGroup",
		"saAmfSGAutoRepair",
		"saAmfSGAutoAdjust",
		"saAmfSGNumPrefActiveSUs",
		"saAmfSGNumPrefStandbySUs",
		"saAmfSGNumPrefInserviceSUs",
		"saAmfSGNumPrefAssignedSUs",
		"saAmfSGMaxActiveSIsperSU",
		"saAmfSGMaxStandbySIsperSU",
		"saAmfSGAutoAdjustProb",
		"saAmfSGCompRestartProb",
		"saAmfSGCompRestartMax",
		"saAmfSGSuRestartProb",
		"saAmfSGSuRestartMax",
		"saAmfSGAdminState",
		NULL
	};

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;


	error = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, app_dn, SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR, &searchParam,
		configAttributes, &searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("%s: saImmOmSearchInitialize_2 failed: %u", __FUNCTION__, error);
		goto done1;
	}

	while ((rc = immutil_saImmOmSearchNext_2(searchHandle, &dn,
		(SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK) {

		if (!is_config_valid(&dn, attributes, NULL)) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}

		if ((sg = sg_create(&dn, attributes)) == NULL) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}

		sg_add_to_model(sg);

		if (avd_su_config_get(&dn, sg) != SA_AIS_OK) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}
	}

	osafassert(rc == SA_AIS_ERR_NOT_EXIST);
	error = SA_AIS_OK;

done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

static SaAisErrorT ccb_completed_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_OK;
	AVD_SG *avd_sg = NULL;
	AVD_AMF_SG_TYPE *avd_sg_type = NULL;
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;

	TRACE_ENTER2("'%s'", opdata->objectName.value);

	avd_sg = avd_sg_get(&opdata->objectName);
	osafassert(avd_sg != NULL);

	/* Validate whether we can modify it. */

	if ((avd_sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) ||
		(avd_sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED)) {

		i = 0;
		while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
			const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;
			void *value = NULL;

			/* Attribute value removed */
			if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || (attribute->attrValues == NULL))
				continue;

			value = attribute->attrValues[0];

			if (!strcmp(attribute->attrName, "saAmfSGType")) {
				SaNameT sg_type_name = *((SaNameT *)value);

				if (avd_sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
					LOG_ER("%s: Attribute saAmfSGType cannot be modified when SG is not in locked instantion", __FUNCTION__);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}

				avd_sg_type = avd_sgtype_get(&sg_type_name);
				if (NULL == avd_sg_type) {
					LOG_ER("SG Type '%s' not found", sg_type_name.value);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}

			} else if (!strcmp(attribute->attrName, "saAmfSGSuHostNodeGroup")) {
				LOG_ER("%s: Attribute saAmfSGSuHostNodeGroup cannot be modified", __FUNCTION__);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			} else if (!strcmp(attribute->attrName, "saAmfSGAutoRepair")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGAutoAdjust")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefActiveSUs")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefStandbySUs")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefInserviceSUs")) {
				uint32_t pref_inservice_su;
				pref_inservice_su = *((SaUint32T *)value);

				if ((pref_inservice_su == 0) ||
					((avd_sg->sg_redundancy_model < SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL) &&
					 (pref_inservice_su < AVSV_SG_2N_PREF_INSVC_SU_MIN))) {
					LOG_ER("%s: Minimum preferred num of su should be 2 in 2N, N+M and NWay red models", __FUNCTION__);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefAssignedSUs")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGMaxActiveSIsperSU")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGMaxStandbySIsperSU")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGAutoAdjustProb")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGCompRestartProb")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGCompRestartMax")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGSuRestartProb")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGSuRestartMax")) {
			} else {
				osafassert(0);
			}
		}		/* while (attr_mod != NULL) */

	} else {	/* Admin state is UNLOCKED */
		i = 0;
		/* Modifications can be done for the following parameters. */
		while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
			const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;
			void *value = NULL;

			/* Attribute value removed */
			if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || (attribute->attrValues == NULL))
				continue;

			value = attribute->attrValues[0];

			if (!strcmp(attribute->attrName, "saAmfSGSuRestartProb")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGSuRestartMax")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGCompRestartProb")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGCompRestartMax")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefInserviceSUs")) {
				uint32_t pref_inservice_su;
				pref_inservice_su = *((SaUint32T *)value);

				if ((pref_inservice_su == 0) ||
					((avd_sg->sg_redundancy_model < SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL) &&
					 (pref_inservice_su < AVSV_SG_2N_PREF_INSVC_SU_MIN))) {
					LOG_ER("%s: Minimum preferred num of su should be 2 in 2N, N+M and NWay red models", __FUNCTION__);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
			} else {
				LOG_ER("%s: Attribute '%s' cannot be modified when SG is unlocked", __FUNCTION__, attribute->attrName);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		}		/* while (attr_mod != NULL) */
	}			/* Admin state is UNLOCKED */

done:
	TRACE_LEAVE();
	return rc;
}

/**
 * update these attributes on the nd
 * @param sg
 * @param attrib_id
 * @param value
 */
static void sg_nd_attribute_update(AVD_SG *sg, uint32_t attrib_id)
{
	AVD_SU *su = NULL;
	AVD_AVND *su_node_ptr = NULL;
	AVSV_PARAM_INFO param;
	memset(((uint8_t *)&param), '\0', sizeof(AVSV_PARAM_INFO));

	TRACE_ENTER();

	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) {
		TRACE_LEAVE2("avd is not in active state");
		return;
	}
	param.class_id = AVSV_SA_AMF_SG;
	param.act = AVSV_OBJ_OPR_MOD;

	switch (attrib_id) {
	case saAmfSGSuRestartProb_ID:
	{
		SaTimeT temp_su_restart_prob;
		param.attr_id = saAmfSGSuRestartProb_ID;
		param.value_len = sizeof(SaTimeT);
		m_NCS_OS_HTONLL_P(&temp_su_restart_prob, sg->saAmfSGSuRestartProb);
		memcpy((char *)&param.value[0], (char *)&temp_su_restart_prob, param.value_len);
		break;
	}
	case saAmfSGSuRestartMax_ID:
	{
		param.attr_id = saAmfSGSuRestartMax_ID;
		param.value_len = sizeof(SaUint32T);
		m_NCS_OS_HTONL_P(&param.value[0], sg->saAmfSGSuRestartMax);
		break;
	}
	case saAmfSGCompRestartProb_ID:
	{
		SaTimeT temp_comp_restart_prob;
		param.attr_id = saAmfSGCompRestartProb_ID;
		param.value_len = sizeof(SaTimeT);
		m_NCS_OS_HTONLL_P(&temp_comp_restart_prob, sg->saAmfSGCompRestartProb);
		memcpy((char *)&param.value[0], (char *)&temp_comp_restart_prob, param.value_len);
		break;
	}
	case saAmfSGCompRestartMax_ID:
	{
		param.attr_id = saAmfSGCompRestartMax_ID;
		param.value_len = sizeof(SaUint32T);
		m_NCS_OS_HTONL_P(&param.value[0], sg->saAmfSGCompRestartMax);	
		break;
	}
	default:
		osafassert(0);
	}

	/* This value has to be updated on each SU on this SG */
	su = sg->list_of_su;
	while (su) {
		m_AVD_GET_SU_NODE_PTR(avd_cb, su, su_node_ptr);

		if ((su_node_ptr) && (su_node_ptr->node_state == AVD_AVND_STATE_PRESENT)) {
			param.name = su->name;

			if (avd_snd_op_req_msg(avd_cb, su_node_ptr, &param) != NCSCC_RC_SUCCESS) {
				LOG_ER("%s::failed for %s",__FUNCTION__, su_node_ptr->name.value);
			}
		}
		su = su->sg_list_su_next;
	}
	TRACE_LEAVE();
}

static unsigned int sg_su_cnt(AVD_SG *sg)
{
	AVD_SU *su;
	int cnt;

	for (su = sg->list_of_su, cnt = 0; su; su = su->sg_list_su_next)
		cnt++;

	return cnt;
}

static void ccb_apply_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	AVD_SG *sg;
	AVD_AMF_SG_TYPE *sg_type;
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;
	void *value = NULL;
	bool value_is_deleted;

	TRACE_ENTER2("'%s'", opdata->objectName.value);

	sg = avd_sg_get(&opdata->objectName);
	assert(sg != NULL);

	sg_type = avd_sgtype_get(&sg->saAmfSGType);
	osafassert(NULL != sg_type);

	if (sg->saAmfSGAdminState != SA_AMF_ADMIN_UNLOCKED) {
		attr_mod = opdata->param.modify.attrMods[i++];
		while (attr_mod != NULL) {
			const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;

			if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) ||	(attribute->attrValues == NULL)) {
				/* Attribute value is deleted, revert to default value */
				value_is_deleted = true;
			} else {
				/* Attribute value is modified */
				value_is_deleted = false;
				value = attribute->attrValues[0];
			}

			if (!strcmp(attribute->attrName, "saAmfSGType")) {
				SaNameT sg_type_name = *((SaNameT *)value);

				sg_type = avd_sgtype_get(&sg_type_name);
				osafassert(NULL != sg_type);

				/* Remove from old type */
				avd_sgtype_remove_sg(sg);

				sg->saAmfSGType = sg_type_name;

				/* Fill the relevant values from sg_type. */
				sg->saAmfSGAutoRepair = sg_type->saAmfSgtDefAutoRepair;
				sg->saAmfSGAutoAdjust = sg_type->saAmfSgtDefAutoAdjust;
				sg->saAmfSGAutoAdjustProb = sg_type->saAmfSgtDefAutoAdjustProb;
				sg->saAmfSGCompRestartProb = sg_type->saAmfSgtDefCompRestartProb;
				sg->saAmfSGCompRestartMax = sg_type->saAmfSgtDefCompRestartMax;
				sg->saAmfSGSuRestartProb = sg_type->saAmfSgtDefSuRestartProb;
				sg->saAmfSGSuRestartMax = sg_type->saAmfSgtDefSuRestartMax;
				sg->sg_redundancy_model = sg_type->saAmfSgtRedundancyModel;

				/* Add to new type */
				sg->sg_type = sg_type;
				avd_sgtype_add_sg(sg);

				/* update all the nodes with the modified values */
				sg_nd_attribute_update(sg, saAmfSGSuRestartProb_ID);
				sg_nd_attribute_update(sg, saAmfSGSuRestartMax_ID);
				sg_nd_attribute_update(sg, saAmfSGCompRestartProb_ID);
				sg_nd_attribute_update(sg, saAmfSGCompRestartMax_ID);
			} else if (!strcmp(attribute->attrName, "saAmfSGAutoRepair")) {
				if (value_is_deleted)
					sg->saAmfSGAutoRepair = sg_type->saAmfSgtDefAutoRepair;
				else
					sg->saAmfSGAutoRepair = *((SaUint32T *)value);
			} else if (!strcmp(attribute->attrName, "saAmfSGAutoAdjust")) {
				if (value_is_deleted)
					sg->saAmfSGAutoAdjust = sg_type->saAmfSgtDefAutoAdjust;
				else
					sg->saAmfSGAutoAdjust = *((SaUint32T *)value);
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefActiveSUs")) {
				if (value_is_deleted)
					sg->saAmfSGNumPrefActiveSUs = 1;
				else
					sg->saAmfSGNumPrefActiveSUs = *((SaUint32T *)value);
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefStandbySUs")) {
				if (value_is_deleted)
					sg->saAmfSGNumPrefStandbySUs = 1;
				else
					sg->saAmfSGNumPrefStandbySUs = *((SaUint32T *)value);
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefInserviceSUs")) {
				if (value_is_deleted)
					sg->saAmfSGNumPrefInserviceSUs = sg_su_cnt(sg);
				else
					sg->saAmfSGNumPrefInserviceSUs = *((SaUint32T *)value);
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefAssignedSUs")) {
				if (value_is_deleted)
					sg->saAmfSGNumPrefAssignedSUs = sg->saAmfSGNumPrefInserviceSUs;
				else
					sg->saAmfSGNumPrefAssignedSUs = *((SaUint32T *)value);
			} else if (!strcmp(attribute->attrName, "saAmfSGMaxActiveSIsperSU")) {
				if (value_is_deleted)
					sg->saAmfSGMaxActiveSIsperSU = -1;
				else
					sg->saAmfSGMaxActiveSIsperSU = *((SaUint32T *)value);
			} else if (!strcmp(attribute->attrName, "saAmfSGMaxStandbySIsperSU")) {
				if (value_is_deleted)
					sg->saAmfSGMaxStandbySIsperSU = -1;
				else
					sg->saAmfSGMaxStandbySIsperSU = *((SaUint32T *)value);
			} else if (!strcmp(attribute->attrName, "saAmfSGAutoAdjustProb")) {
				if (value_is_deleted)
					sg->saAmfSGAutoAdjustProb = sg_type->saAmfSgtDefAutoAdjustProb;
				else
					sg->saAmfSGAutoAdjustProb = *((SaTimeT *)value);
			} else if (!strcmp(attribute->attrName, "saAmfSGCompRestartProb")) {
				if (value_is_deleted)
					sg->saAmfSGCompRestartProb = sg_type->saAmfSgtDefCompRestartProb;
				else
					sg->saAmfSGCompRestartProb = *((SaTimeT *)value);
				sg_nd_attribute_update(sg, saAmfSGCompRestartProb_ID);
			} else if (!strcmp(attribute->attrName, "saAmfSGCompRestartMax")) {
				if (value_is_deleted)
					sg->saAmfSGCompRestartMax = sg_type->saAmfSgtDefCompRestartMax;
				else
					sg->saAmfSGCompRestartMax = *((SaUint32T *)value);
				sg_nd_attribute_update(sg, saAmfSGCompRestartMax_ID);
			} else if (!strcmp(attribute->attrName, "saAmfSGSuRestartProb")) {
				if (value_is_deleted)
					sg->saAmfSGSuRestartProb = sg_type->saAmfSgtDefSuRestartProb;
				else
					sg->saAmfSGSuRestartProb = *((SaTimeT *)value);
				sg_nd_attribute_update(sg, saAmfSGSuRestartProb_ID);
			} else if (!strcmp(attribute->attrName, "saAmfSGSuRestartMax")) {
				if (value_is_deleted)
					sg->saAmfSGSuRestartMax = sg_type->saAmfSgtDefSuRestartMax;
				else
					sg->saAmfSGSuRestartMax = *((SaUint32T *)value);
				sg_nd_attribute_update(sg, saAmfSGSuRestartMax_ID);
			} else {
				osafassert(0);
			}

			attr_mod = opdata->param.modify.attrMods[i++];
		}
	} else {			/* Admin state is UNLOCKED */
		i = 0;
		/* Modifications can be done for the following parameters. */
		attr_mod = opdata->param.modify.attrMods[i++];
		while (attr_mod != NULL) {
			const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;

			if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) ||	(attribute->attrValues == NULL)) {
				/* Attribute value is deleted, revert to default value */
				value_is_deleted = true;
			} else {
				/* Attribute value is modified */
				value_is_deleted = false;
				value = attribute->attrValues[0];
			}

			if (!strcmp(attribute->attrName, "saAmfSGNumPrefInserviceSUs")) {
				if (value_is_deleted)
					sg->saAmfSGNumPrefInserviceSUs = sg_su_cnt(sg);
				else
					sg->saAmfSGNumPrefInserviceSUs = *((SaUint32T *)value);

				if (avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE)  {
					if (avd_sg_app_su_inst_func(avd_cb, sg) != NCSCC_RC_SUCCESS) {
						osafassert(0);
					}
				}
			} else if (!strcmp(attribute->attrName, "saAmfSGCompRestartProb")) {
				if (value_is_deleted)
					sg->saAmfSGCompRestartProb = sg_type->saAmfSgtDefCompRestartProb;
				else
					sg->saAmfSGCompRestartProb = *((SaTimeT *)value);
				sg_nd_attribute_update(sg, saAmfSGCompRestartProb_ID);

			} else if (!strcmp(attribute->attrName, "saAmfSGCompRestartMax")) {
				if (value_is_deleted)
					sg->saAmfSGCompRestartMax = sg_type->saAmfSgtDefCompRestartMax;
				else
					sg->saAmfSGCompRestartMax = *((SaUint32T *)value);
				sg_nd_attribute_update(sg, saAmfSGCompRestartMax_ID);

			} else if (!strcmp(attribute->attrName, "saAmfSGSuRestartProb")) {
				if (value_is_deleted)
					sg->saAmfSGSuRestartProb = sg_type->saAmfSgtDefSuRestartProb;
				else
					sg->saAmfSGSuRestartProb = *((SaTimeT *)value);
				sg_nd_attribute_update(sg, saAmfSGSuRestartProb_ID);

			} else if (!strcmp(attribute->attrName, "saAmfSGSuRestartMax")) {
				if (value_is_deleted)
					sg->saAmfSGSuRestartMax = sg_type->saAmfSgtDefSuRestartMax;
				else
					sg->saAmfSGSuRestartMax = *((SaUint32T *)value);
				sg_nd_attribute_update(sg, saAmfSGSuRestartMax_ID);
			} else {
				osafassert(0);
			}

			attr_mod = opdata->param.modify.attrMods[i++];
		}		/* while (attr_mod != NULL) */

	}			/* Admin state is UNLOCKED */
}

/**
 * perform lock-instantiation on a given SG
 * @param cb
 * @param sg
 */
static uint32_t sg_app_sg_admin_lock_inst(AVD_CL_CB *cb, AVD_SG *sg)
{
	AVD_SU *su;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%s", sg->name.value);

	/* terminate all the SUs on this Node */
	su = sg->list_of_su;
	while (su != NULL) {
		if ((su->saAmfSUPreInstantiable == true) &&
			(su->saAmfSUPresenceState != SA_AMF_PRESENCE_UNINSTANTIATED) &&
			(su->saAmfSUPresenceState != SA_AMF_PRESENCE_INSTANTIATION_FAILED) &&
			(su->saAmfSUPresenceState != SA_AMF_PRESENCE_TERMINATION_FAILED)) {

			if (avd_snd_presence_msg(cb, su, true) == NCSCC_RC_SUCCESS) {
				m_AVD_SET_SU_TERM(cb, su, true);
			} else {
				rc = NCSCC_RC_FAILURE;
				LOG_WA("Failed Termination '%s'", su->name.value);
			}
		}
		su = su->sg_list_su_next;
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/**
 * perform unlock-instantiation on a given SG
 * 
 * @param cb
 * @param sg
 */
static void sg_app_sg_admin_unlock_inst(AVD_CL_CB *cb, AVD_SG *sg)
{
	AVD_SU *su;

	TRACE_ENTER2("%s", sg->name.value);

	/* Instantiate the SUs in this SG */
	for (su = sg->list_of_su; su != NULL; su = su->sg_list_su_next) {
		if ((su->saAmfSUAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
			(su->su_on_node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION)
			&& (su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED)) {

			if (su->saAmfSUPreInstantiable == true) {
				if (su->su_on_node->node_state == AVD_AVND_STATE_PRESENT) {
					if (avd_snd_presence_msg(cb, su, false) != NCSCC_RC_SUCCESS) {
						LOG_NO("%s: Failed to send Instantiation order of '%s' to %x",
							   __FUNCTION__, su->name.value, su->su_on_node->node_info.nodeId);
					}
				}

				/* set uncondionally of msg snd outcome */
				m_AVD_SET_SU_TERM(cb, su, false);

			} else {
				avd_su_oper_state_set(su, SA_AMF_OPERATIONAL_ENABLED);
			}
		}
	}

	TRACE_LEAVE();
}

/**
 * @brief       Checks if Tolerance timer is running for any of the SI's of the SG  
 *
 * @param[in]  sg
 *
 * @return  true/false
 **/
bool sg_is_tolerance_timer_running_for_any_si(AVD_SG *sg)
{
	AVD_SI  *si;

	for (si = sg->list_of_si; si != NULL; si = si->sg_list_of_si_next) {
		if (si->si_dep_state == AVD_SI_TOL_TIMER_RUNNING) {
			TRACE("Tolerance timer running for si: %s",si->name.value);
			return true;
		}
	}

	return false;
}
static void sg_admin_op_cb(SaImmOiHandleT immOiHandle, SaInvocationT invocation,
	const SaNameT *object_name, SaImmAdminOperationIdT op_id,
	const SaImmAdminOperationParamsT_2 **params)
{
	SaAisErrorT rc = SA_AIS_OK;
	AVD_SG *sg;
	SaAmfAdminStateT adm_state;

	TRACE_ENTER2("'%s', %llu", object_name->value, op_id);
	sg = avd_sg_get(object_name);

	if (sg->sg_ncs_spec == SA_TRUE) {
		LOG_ER("Admin Op on OpenSAF MW SG is not allowed");
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

	if (sg->sg_fsm_state != AVD_SG_FSM_STABLE) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		LOG_WA("SG not in STABLE state (%s)", sg->name.value);
		goto done;
	}

	/* if Tolerance timer is running for any SI's withing this SG, then return SA_AIS_ERR_TRY_AGAIN */
	if (sg_is_tolerance_timer_running_for_any_si(sg)) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		LOG_WA("Tolerance timer is running for some of the SI's in the SG '%s', so differing admin opr",sg->name.value);
		goto done;
	}
	switch (op_id) {
	case SA_AMF_ADMIN_UNLOCK:
		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_UNLOCKED) {
			LOG_ER("%s is already unlocked", object_name->value);
			rc = SA_AIS_ERR_NO_OP;
			goto done;
		}

		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			LOG_ER("%s is locked instantiation", object_name->value);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}

		adm_state = sg->saAmfSGAdminState;
		avd_sg_admin_state_set(sg, SA_AMF_ADMIN_UNLOCKED);
		if (avd_sg_app_sg_admin_func(avd_cb, sg) != NCSCC_RC_SUCCESS) {
			avd_sg_admin_state_set(sg, adm_state);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}
		break;

	case SA_AMF_ADMIN_LOCK:
		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			LOG_ER("%s is already locked", object_name->value);
			rc = SA_AIS_ERR_NO_OP;
			goto done;
		}

		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			LOG_ER("%s is locked instantiation", object_name->value);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}

		adm_state = sg->saAmfSGAdminState;
		avd_sg_admin_state_set(sg, SA_AMF_ADMIN_LOCKED);
		if (avd_sg_app_sg_admin_func(avd_cb, sg) != NCSCC_RC_SUCCESS) {
			avd_sg_admin_state_set(sg, adm_state);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}

		break;
	case SA_AMF_ADMIN_SHUTDOWN:
		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			LOG_ER("%s is shutting down", object_name->value);
			rc = SA_AIS_ERR_NO_OP;
			goto done;
		}

		if ((sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) ||
		    (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION)) {
			LOG_ER("%s is locked (instantiation)", object_name->value);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}

		adm_state = sg->saAmfSGAdminState;
		avd_sg_admin_state_set(sg, SA_AMF_ADMIN_SHUTTING_DOWN);
		if (avd_sg_app_sg_admin_func(avd_cb, sg) != NCSCC_RC_SUCCESS) {
			avd_sg_admin_state_set(sg, adm_state);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}
		break;
	case SA_AMF_ADMIN_LOCK_INSTANTIATION:
		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			LOG_ER("%s is already locked-instantiation", object_name->value);
			rc = SA_AIS_ERR_NO_OP;
			goto done;
		}

		if (sg->saAmfSGAdminState != SA_AMF_ADMIN_LOCKED) {
			LOG_ER("%s is not locked", object_name->value);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}

		adm_state = sg->saAmfSGAdminState;
		avd_sg_admin_state_set(sg, SA_AMF_ADMIN_LOCKED_INSTANTIATION);
		if (sg_app_sg_admin_lock_inst(avd_cb, sg) != NCSCC_RC_SUCCESS) {
			avd_sg_admin_state_set(sg, adm_state);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}

		break;
	case SA_AMF_ADMIN_UNLOCK_INSTANTIATION:
		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			LOG_ER("%s is already locked", object_name->value);
			rc = SA_AIS_ERR_NO_OP;
			goto done;
		}

		if (sg->saAmfSGAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			LOG_ER("%s is not in locked instantiation state", object_name->value);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}

		avd_sg_admin_state_set(sg, SA_AMF_ADMIN_LOCKED);
		sg_app_sg_admin_unlock_inst(avd_cb, sg);

		break;
	case SA_AMF_ADMIN_SG_ADJUST:
	default:
		rc = SA_AIS_ERR_NOT_SUPPORTED;
		break;
	}
 done:
	avd_saImmOiAdminOperationResult(immOiHandle, invocation, rc);
}

static SaAisErrorT sg_rt_attr_cb(SaImmOiHandleT immOiHandle,
	const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	AVD_SG *sg = avd_sg_get(objectName);
	SaImmAttrNameT attributeName;
	int i = 0;

	TRACE_ENTER2("'%s'", objectName->value);
	osafassert(sg != NULL);

	while ((attributeName = attributeNames[i++]) != NULL) {
		if (!strcmp("saAmfSGNumCurrAssignedSUs", attributeName)) {
			avd_saImmOiRtObjectUpdate_sync(objectName, attributeName,
				SA_IMM_ATTR_SAUINT32T, &sg->saAmfSGNumCurrAssignedSUs);
		} else if (!strcmp("saAmfSGNumCurrNonInstantiatedSpareSUs", attributeName)) {
			avd_saImmOiRtObjectUpdate_sync(objectName, attributeName,
				SA_IMM_ATTR_SAUINT32T, &sg->saAmfSGNumCurrNonInstantiatedSpareSUs);
		} else if (!strcmp("saAmfSGNumCurrInstantiatedSpareSUs", attributeName)) {
			avd_saImmOiRtObjectUpdate_sync(objectName, attributeName,
				SA_IMM_ATTR_SAUINT32T, &sg->saAmfSGNumCurrInstantiatedSpareSUs);
		} else
			osafassert(0);
	}

	return SA_AIS_OK;
}

/**
 * Validate SaAmfSG CCB completed
 * @param opdata
 * 
 * @return SaAisErrorT
 */
static SaAisErrorT sg_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	AVD_SG *sg;
	AVD_SI *si;
	SaBoolT si_exist = SA_FALSE;
	CcbUtilOperationData_t *t_opData;

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
		sg = avd_sg_get(&opdata->objectName);
		if (sg->list_of_si != NULL) {
			/* check whether there is parent app delete */
			t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, &sg->app->name);
			if (t_opData == NULL || t_opData->operationType != CCBUTIL_DELETE) {
				/* check whether there exists a delete operation for 
				 * each of the SIs in the SG's list in the current CCB
				 */
				si = sg->list_of_si;
				while (si != NULL) {
					t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, &si->name);
					if ((t_opData == NULL) || (t_opData->operationType != CCBUTIL_DELETE)) {
						si_exist = SA_TRUE;
						break;
					}
					si = si->sg_list_of_si_next;
				}
			}
			if (si_exist == SA_TRUE) {
				LOG_ER("SIs still exist in SG '%s'", sg->name.value);
				goto done;
			}
		}
		rc = SA_AIS_OK;
		opdata->userData = sg;	/* Save for later use in apply */
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
 * Apply SaAmfSG CCB changes
 * @param opdata
 * 
 * @return SaAisErrorT
 */
static void sg_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_SG *sg;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		sg = sg_create(&opdata->objectName, opdata->param.create.attrValues);
		osafassert(sg);
		sg_add_to_model(sg);
		break;
	case CCBUTIL_DELETE:
		avd_sg_delete(opdata->userData);
		break;
	case CCBUTIL_MODIFY:
		ccb_apply_modify_hdlr(opdata);
		break;
	default:
		osafassert(0);
	}
}

void avd_sg_remove_su(AVD_SU* su)
{
	AVD_SU *i_su = NULL;
	AVD_SU *prev_su = NULL;
	AVD_SG *sg = su->sg_of_su;

	if (su->sg_of_su != NULL) {
		/* remove SU from SG */
		i_su = su->sg_of_su->list_of_su;

		while ((i_su != NULL) && (i_su != su)) {
			prev_su = i_su;
			i_su = i_su->sg_list_su_next;
		}

		if (i_su != su) {
			/* Log a fatal error */
		} else {
			if (prev_su == NULL) {
				su->sg_of_su->list_of_su = su->sg_list_su_next;
			} else {
				prev_su->sg_list_su_next = su->sg_list_su_next;
			}
		}

		su->sg_list_su_next = NULL;
		su->sg_of_su = NULL;
	} /* if (su->sg_of_su != AVD_SG_NULL) */

	avd_verify_equal_ranked_su(sg);
	return;
}

void avd_sg_add_su(AVD_SU* su)
{
	AVD_SU *i_su = NULL;
	AVD_SU *prev_su = NULL;

	if ((su == NULL) || (su->sg_of_su == NULL))
		return;

	i_su = su->sg_of_su->list_of_su;

	while ((i_su != NULL) && (i_su->saAmfSURank < su->saAmfSURank)) {
		prev_su = i_su;
		i_su = i_su->sg_list_su_next;
	}

	if (prev_su == NULL) {
		su->sg_list_su_next = su->sg_of_su->list_of_su;
		su->sg_of_su->list_of_su = su;
	} else {
		prev_su->sg_list_su_next = su;
		su->sg_list_su_next = i_su;
	}
	avd_verify_equal_ranked_su(su->sg_of_su);
}

void avd_sg_constructor(void)
{
	NCS_PATRICIA_PARAMS patricia_params;

	patricia_params.key_size = sizeof(SaNameT);
	osafassert(ncs_patricia_tree_init(&sg_db, &patricia_params) == NCSCC_RC_SUCCESS);

	avd_class_impl_set("SaAmfSG", sg_rt_attr_cb, sg_admin_op_cb, sg_ccb_completed_cb, sg_ccb_apply_cb);
}

void avd_sg_admin_state_set(AVD_SG* sg, SaAmfAdminStateT state)
{
	SaAmfAdminStateT old_state = sg->saAmfSGAdminState;
	
	osafassert(state <= SA_AMF_ADMIN_SHUTTING_DOWN);
	TRACE_ENTER2("%s AdmState %s => %s", sg->name.value,
			avd_adm_state_name[old_state], avd_adm_state_name[state]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s AdmState %s => %s", sg->name.value,
                  avd_adm_state_name[old_state], avd_adm_state_name[state]);      
	sg->saAmfSGAdminState = state;
	avd_saImmOiRtObjectUpdate(&sg->name,
			"saAmfSGAdminState", SA_IMM_ATTR_SAUINT32T, &sg->saAmfSGAdminState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, sg, AVSV_CKPT_SG_ADMIN_STATE);

	/* Notification */ 
	avd_send_admin_state_chg_ntf(&sg->name,
					SA_AMF_NTFID_SG_ADMIN_STATE,
					old_state,
					sg->saAmfSGAdminState);
}
/**
 *
 * @brief  This function verifies whether SU ranks are equal or not in an SG.
 *
 * @param[in]  avd_sg - pointer to service group
 *
 * @return     none
 *
 */
static void avd_verify_equal_ranked_su(AVD_SG *avd_sg)
{
	AVD_SU *pre_temp_su = NULL, *temp_su;

	TRACE_ENTER();
	/* Check ranks are still equal or not. Mark true in the begining*/
	avd_sg->equal_ranked_su = true;
	temp_su = avd_sg->list_of_su;
	while(temp_su) {
		if (pre_temp_su && temp_su->saAmfSURank != pre_temp_su->saAmfSURank) {
			avd_sg->equal_ranked_su = false;
			break;
		}
		pre_temp_su = temp_su;
		temp_su = temp_su->sg_list_su_next;
	}
	TRACE_LEAVE2("avd_sg->equal_ranked_su '%u'", avd_sg->equal_ranked_su);

	return; 
}

/**
 * @brief        This routine checks whether the SIs can be redistributed among SUs
 *               to achieve equal  distribution, if so finds  the  max assigned SU,
 *               min assigned SU and the SI that needs to be transferred
 *
 * @param[in]    avd_sg - pointer to service group  
 *
 * @return       Returns nothing.
 */
void avd_sg_nwayact_screening_for_si_distr(AVD_SG *avd_sg) 
{
	AVD_SU *i_su;
	AVD_SI *i_si = NULL;
	AVD_SU_SI_REL *su_si, *i_susi = NULL;

	TRACE_ENTER();
	osafassert(true == avd_sg->equal_ranked_su);
	osafassert(AVD_SG_FSM_STABLE == avd_sg->sg_fsm_state);
	/* Reset Max and Min ptrs. */
	avd_sg->max_assigned_su = avd_sg->min_assigned_su = NULL;
	avd_sg->si_tobe_redistributed = NULL;

        i_su = avd_sg->list_of_su;
	while (i_su != NULL) {
		if (i_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) {
			i_su = i_su->sg_list_su_next;
			continue;
		}
		if (NULL == avd_sg->max_assigned_su) {
			/* First SU. Assign it to max_assigned_su and min_assigned_su. */
			avd_sg->max_assigned_su = avd_sg->min_assigned_su = i_su;
		} else {
			if (i_su->saAmfSUNumCurrActiveSIs > avd_sg->max_assigned_su->saAmfSUNumCurrActiveSIs)
				avd_sg->max_assigned_su = i_su;

			if (i_su->saAmfSUNumCurrActiveSIs < avd_sg->min_assigned_su->saAmfSUNumCurrActiveSIs)
				avd_sg->min_assigned_su = i_su;
		}
		i_su = i_su->sg_list_su_next;
	} /*  while (i_su != NULL)  */

	if ((NULL == avd_sg->max_assigned_su) || (NULL == avd_sg->min_assigned_su)) {
		TRACE("No need for reassignments");
		goto done; /* No need to change assignments. */
	}

	if ((avd_sg->max_assigned_su->saAmfSUNumCurrActiveSIs - avd_sg->min_assigned_su->saAmfSUNumCurrActiveSIs) <= 1) {
		TRACE("max_assigned_su '%s' no of assignments '%u', min_assigned_su '%s' no of assignments '%u'", 
				avd_sg->max_assigned_su->name.value, avd_sg->max_assigned_su->saAmfSUNumCurrActiveSIs, 
				avd_sg->min_assigned_su->name.value, avd_sg->min_assigned_su->saAmfSUNumCurrActiveSIs);
		avd_sg->max_assigned_su = avd_sg->min_assigned_su = NULL;
		TRACE("No need for reassignments");
		goto done; /* No need to change assignments. */
	}
	else {
		/* Need to change assignments. Find the SI whose assignment has to be changed, figure out less 
		   ranked SI if SU has more than one assignments. Do check that max_assigned_su and min_assigned_su 
		   don't share the same SI assignment(Say SU1 has SI1, SI2, SI3, SI4 and SU2 has SI4, so don't select
		   SI4 at least.)*/
		su_si = avd_sg->max_assigned_su->list_of_susi;
		while (NULL != su_si) {
			/* Find if this is assigned to avd_sg->min_assigned_su. */
			if (avd_su_susi_find(avd_cb, avd_sg->min_assigned_su, &su_si->si->name) != AVD_SU_SI_REL_NULL) {
				/* This SU has already a assignment for this SI go to the
				 * next SI.
				 */
				su_si = su_si->su_next;
				continue;
			}
			if (NULL == i_si) { /* first si. */
				i_si = su_si->si;
				i_susi = su_si;
			}
			else {
				if (i_si->saAmfSIRank < su_si->si->saAmfSIRank) {
					i_si = su_si->si;
					i_susi = su_si;
				}
			}
			su_si = su_si->su_next;
		}/* while (NULL != su_si) */
		osafassert(i_si);
		osafassert(i_susi);
		avd_sg->si_tobe_redistributed = i_si;
	}
	/* check point the SI transfer parameters with STANDBY*/
	m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(avd_cb, avd_sg, AVSV_CKPT_AVD_SI_TRANS);

	TRACE("max_assigned_su '%s' no of assignments '%u', min_assigned_su '%s' no of assignments '%u'," 
			"si_tobe_redistributed '%s'", avd_sg->max_assigned_su->name.value, avd_sg->max_assigned_su->
			saAmfSUNumCurrActiveSIs, avd_sg->min_assigned_su->name.value, avd_sg->min_assigned_su->
			saAmfSUNumCurrActiveSIs, avd_sg->si_tobe_redistributed->name.value);
	/* Trigger SI redistribution/transfer here. Only one SI getting shifted from one SU to another SU */

	if (avd_susi_mod_send(i_susi, SA_AMF_HA_QUIESCED) != NCSCC_RC_SUCCESS) {
		avd_sg->max_assigned_su = NULL;
		avd_sg->min_assigned_su = NULL;
		avd_sg->si_tobe_redistributed = NULL;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, avd_sg, AVSV_CKPT_AVD_SI_TRANS);
		goto done;
	}
	/* add the SU to the operation list and change the SG FSM to SU operation. */
	avd_sg_su_oper_list_add(avd_cb, i_susi->su, false);
	m_AVD_SET_SG_FSM(avd_cb, (i_susi->su->sg_of_su), AVD_SG_FSM_SU_OPER);

done:
	TRACE_LEAVE();

	return;
}

/**
 * Adjust @a sg configuration attributes after it has been added to the model
 *
 * This function should only be called at the end of a configuration change
 * when all instances has been added to the model.
 * For more info see sai-im-xmi-a.04.02.xml
 */
void avd_sg_adjust_config(AVD_SG *sg)
{
	// SUs in an SG are equal, only need to look at the first one
	if ((sg->list_of_su != NULL) && !sg->list_of_su->saAmfSUPreInstantiable) {
		// NPI SUs can only be assigned one SI, see 3.2.1.1
		if ((sg->saAmfSGMaxActiveSIsperSU != -1) && (sg->saAmfSGMaxActiveSIsperSU > 1)) {
			LOG_WA("invalid saAmfSGMaxActiveSIsperSU (%u) for NPI SU '%s', adjusting...",
					sg->saAmfSGMaxActiveSIsperSU, sg->name.value);
		}
		sg->saAmfSGMaxActiveSIsperSU = 1;

		if ((sg->saAmfSGMaxStandbySIsperSU != -1) && (sg->saAmfSGMaxStandbySIsperSU > 1)) {
			LOG_WA("invalid saAmfSGMaxStandbySIsperSU (%u) for NPI SU '%s', adjusting...",
					sg->saAmfSGMaxStandbySIsperSU, sg->name.value);
		}
		sg->saAmfSGMaxStandbySIsperSU = 1;
	}

	/* adjust saAmfSGNumPrefInserviceSUs if not configured */
	if (sg->saAmfSGNumPrefInserviceSUs == 0) {
		sg->saAmfSGNumPrefInserviceSUs = sg_su_cnt(sg);
		if ((sg->sg_type->saAmfSgtRedundancyModel == SA_AMF_2N_REDUNDANCY_MODEL) &&
				(sg->saAmfSGNumPrefInserviceSUs < 2)) {
			sg->saAmfSGNumPrefInserviceSUs = 2;
			LOG_NO("'%s' saAmfSGNumPrefInserviceSUs adjusted to 2", sg->name.value);
		}
	}

	/* adjust saAmfSGNumPrefAssignedSUs if not configured, only applicable for
	 * the N-way and N-way active redundancy models
	 */
	if ((sg->saAmfSGNumPrefAssignedSUs == 0) &&
			((sg->sg_type->saAmfSgtRedundancyModel == SA_AMF_N_WAY_REDUNDANCY_MODEL) ||
				(sg->sg_type->saAmfSgtRedundancyModel == SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL))) {
		sg->saAmfSGNumPrefAssignedSUs = sg->saAmfSGNumPrefInserviceSUs;
		LOG_NO("'%s' saAmfSGNumPrefAssignedSUs adjusted to %u",
				sg->name.value, sg->saAmfSGNumPrefAssignedSUs);
	}
}
