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

#include "base/logtrace.h"

#include "amf/amfd/util.h"
#include "amf/amfd/cluster.h"
#include "amf/amfd/app.h"
#include "amf/amfd/imm.h"
#include "amf/amfd/sutype.h"
#include "amf/amfd/ckpt_msg.h"
#include "amf/amfd/ntf.h"
#include "amf/amfd/sg.h"
#include "amf/amfd/proc.h"
#include "amf/amfd/si_dep.h"
#include "amf/amfd/csi.h"
#include <algorithm>

AmfDb<std::string, AVD_SG> *sg_db = nullptr;

static void avd_verify_equal_ranked_su(AVD_SG *avd_sg);

void avd_sg_db_add(AVD_SG *sg)
{
	unsigned int rc;

	if (sg_db->find(sg->name) == nullptr) {
		rc = sg_db->insert(sg->name,sg);
		osafassert(rc == NCSCC_RC_SUCCESS);
	}
}


/**
 * Add the SG to the model
 * @param sg
 */
static void sg_add_to_model(AVD_SG *sg)
{
	std::string dn;

	TRACE_ENTER2("%s", sg->name.c_str());

	/* Check parent link to see if it has been added already */
	if (sg->app != nullptr) {
		TRACE("already added");
		goto done;
	}

	avsv_sanamet_init(sg->name, dn, "safApp");
	sg->app = app_db->find(dn);
	osafassert(sg->app != nullptr);

	avd_sg_db_add(sg);
	sg->sg_type = sgtype_db->find(sg->saAmfSGType);
	osafassert(sg->sg_type);
	avd_sgtype_add_sg(sg);
	sg->app->add_sg(sg);

	/* SGs belonging to a magic app will be NCS, TODO Better name! */
	if (dn.compare("safApp=OpenSAF") == 0)
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
	sg->app->remove_sg(sg);
	sg_db->erase(sg->name);

	m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, sg, AVSV_CKPT_AVD_SG_CONFIG);
}

AVD_SG::AVD_SG():
		name(""),
		saAmfSGAutoRepair_configured(false),
		saAmfSGType(""),
		saAmfSGSuHostNodeGroup(""),
		saAmfSGAutoRepair(SA_FALSE),
		saAmfSGAutoAdjust(SA_FALSE),
		saAmfSGNumPrefActiveSUs(0),
		saAmfSGNumPrefStandbySUs(0),
		saAmfSGNumPrefInserviceSUs(~0),
		saAmfSGNumPrefAssignedSUs(0),
		saAmfSGMaxActiveSIsperSU(0),
		saAmfSGMaxStandbySIsperSU(0),
		saAmfSGAutoAdjustProb(0),
		saAmfSGCompRestartProb(0),
		saAmfSGCompRestartMax(0),
		saAmfSGSuRestartProb(0),
		saAmfSGSuRestartMax(0),
		saAmfSGAdminState(SA_AMF_ADMIN_UNLOCKED),
		saAmfSGNumCurrAssignedSUs(0),
		saAmfSGNumCurrInstantiatedSpareSUs(0),
		saAmfSGNumCurrNonInstantiatedSpareSUs(0),
		adjust_state(AVSV_SG_STABLE),
		sg_ncs_spec(false),
		sg_fsm_state(AVD_SG_FSM_STABLE),
		admin_si(nullptr),
		sg_redundancy_model(SA_AMF_NO_REDUNDANCY_MODEL),
		sg_type(nullptr),
		sg_list_app_next(nullptr),
		app(nullptr),
		equal_ranked_su(false),
		max_assigned_su(nullptr),
		min_assigned_su(nullptr),
		si_tobe_redistributed(nullptr),
		try_inst_counter(0),
		headless_validation(true)
{
	adminOp = static_cast<SaAmfAdminOperationIdT>(0);
	adminOp_invocationId = 0;
	ng_using_saAmfSGAdminState = false;
}

static AVD_SG *sg_new(const std::string& dn, SaAmfRedundancyModelT redundancy_model)
{
	AVD_SG *sg = nullptr;

	if (redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL)
		sg = new SG_2N();
	else if (redundancy_model == SA_AMF_NPM_REDUNDANCY_MODEL)
		sg = new SG_NPM();
	else if (redundancy_model == SA_AMF_N_WAY_REDUNDANCY_MODEL)
		sg = new SG_NWAY();
	else if (redundancy_model == SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL)
		sg = new SG_NACV();
	else if (redundancy_model == SA_AMF_NO_REDUNDANCY_MODEL)
		sg = new SG_NORED();
	else
		assert(0);

	sg->name = dn;

	return sg;
}

void avd_sg_delete(AVD_SG *sg)
{
	/* by now SU and SI should have been deleted */ 
	osafassert(sg->list_of_su.empty() == true);
	osafassert(sg->list_of_si.empty() == true);
	sg_remove_from_model(sg);
	delete sg;
}

void AVD_SG::add_si(AVD_SI* si)
{
	si->sg_of_si = this;
	list_of_si.push_back(si);
	std::sort(list_of_si.begin(), list_of_si.end(),
		[](const AVD_SI *a, const AVD_SI *b) -> bool {return a->saAmfSIRank < b->saAmfSIRank;});
}

void AVD_SG::remove_si(AVD_SI* si)
{
	auto si_to_remove = std::find(list_of_si.begin(), list_of_si.end(), si);
	
	if (si_to_remove != list_of_si.end()) {
		list_of_si.erase(si_to_remove);
	} else {
		osafassert(false);
	}

	si->sg_of_si = nullptr;
}

/**
 * Validate configuration attributes for an SaAmfSG object
 * 
 * @param dn
 * @param sg
 * 
 * @return int
 */
static int is_config_valid(const std::string& dn,
	const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;
	SaNameT aname;
	SaBoolT abool;
	SaAmfAdminStateT admstate;
	std::string::size_type pos;

	if ((pos = dn.find(',')) == std::string::npos) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn.c_str());
		return 0;
	}

	if (dn.compare(pos + 1, 7, "safApp=") != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", dn.substr(pos +1).c_str(), dn.c_str());
		return 0;
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGType"), attributes, 0, &aname);
	osafassert(rc == SA_AIS_OK);

	if (sgtype_db->find(Amf::to_string(&aname)) == nullptr) {
		if (opdata == nullptr) {
			report_ccb_validation_error(opdata, "'%s' does not exist in model", osaf_extended_name_borrow(&aname));
			return 0;
		}

		/* SG type does not exist in current model, check CCB */
		if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &aname) == nullptr) {
			report_ccb_validation_error(opdata, "'%s' does not exist in existing model or in CCB", osaf_extended_name_borrow(&aname));
			return 0;
		}
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGAutoRepair"), attributes, 0, &abool) == SA_AIS_OK) &&
			(abool > SA_TRUE)) {
		report_ccb_validation_error(opdata, "Invalid saAmfSGAutoRepair %u for '%s'", abool, dn.c_str());
		return 0;
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGAutoAdjust"), attributes, 0, &abool) == SA_AIS_OK) &&
	    (abool > SA_TRUE)) {
		report_ccb_validation_error(opdata, "Invalid saAmfSGAutoAdjust %u for '%s'", abool, dn.c_str());
		return 0;
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGAdminState"), attributes, 0, &admstate) == SA_AIS_OK) &&
	    !avd_admin_state_is_valid(admstate, opdata)) {
		report_ccb_validation_error(opdata, "Invalid saAmfSGAdminState %u for '%s'", admstate, dn.c_str());
		return 0;
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGSuHostNodeGroup"), attributes, 0, &aname) == SA_AIS_OK) &&
	    (avd_ng_get(Amf::to_string(&aname)) == nullptr)) {
		report_ccb_validation_error(opdata, "Invalid saAmfSGSuHostNodeGroup '%s' for '%s'", osaf_extended_name_borrow(&aname), dn.c_str());
		return 0;
	}

	return 1;
}

static AVD_SG *sg_create(const std::string& sg_name, const SaImmAttrValuesT_2 **attributes)
{
	int rc = -1;
	AVD_SG *sg;
	AVD_AMF_SG_TYPE *sgt;
	SaAisErrorT error;
	SaNameT temp_name;

	TRACE_ENTER2("'%s'", sg_name.c_str());

	SaNameT sgtype_dn;
	error = immutil_getAttr("saAmfSGType", attributes, 0, &sgtype_dn);
	osafassert(error == SA_AIS_OK);
	sgt = sgtype_db->find(Amf::to_string(&sgtype_dn));
	osafassert(sgt);
	sg = sg_new(sg_name, sgt->saAmfSgtRedundancyModel);
	sg->saAmfSGType = Amf::to_string(&sgtype_dn);
	sg->sg_type = sgt;

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGSuHostNodeGroup"), attributes, 0, &temp_name) == SA_AIS_OK) {
		sg->saAmfSGSuHostNodeGroup = Amf::to_string(&temp_name);
	}
	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGAutoRepair"), attributes, 0, &sg->saAmfSGAutoRepair) != SA_AIS_OK) {
		sg->saAmfSGAutoRepair = sgt->saAmfSgtDefAutoRepair;
		sg->saAmfSGAutoRepair_configured = false;
	}
	else 
		sg->saAmfSGAutoRepair_configured = true; 

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGAutoAdjust"), attributes, 0, &sg->saAmfSGAutoAdjust) != SA_AIS_OK) {
		sg->saAmfSGAutoAdjust = sgt->saAmfSgtDefAutoAdjust;
	}

	if (sgt->saAmfSgtRedundancyModel == SA_AMF_NPM_REDUNDANCY_MODEL) {
		/* saAmfSGNumPrefActiveSUs and saAmfSGNumPrefStandbySUs are set to 1 by default in imm.xml.
		   So, immutil_getAttr will always return OK even if user doesn't configure them.
		   Later on if these attr are removed from imm.xml, then the below check can be used. */
		if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGNumPrefActiveSUs"),
					attributes, 0, &sg->saAmfSGNumPrefActiveSUs) != SA_AIS_OK) {
			LOG_WA("Mandatory attr 'saAmfSGNumPrefActiveSUs' not configured for NpM");
			sg->saAmfSGNumPrefActiveSUs = 1;
		}
		if ((sg->saAmfSGNumPrefActiveSUs == 0) || (sg->saAmfSGNumPrefActiveSUs == 1))
			LOG_NO("'saAmfSGNumPrefActiveSUs' is set to %u, not useful for NpM",
					sg->saAmfSGNumPrefActiveSUs);

		if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGNumPrefStandbySUs"),
					attributes, 0, &sg->saAmfSGNumPrefStandbySUs) != SA_AIS_OK) {
			LOG_WA("Mandatory attr 'saAmfSGNumPrefStandbySUs' not configured for NpM");
			sg->saAmfSGNumPrefStandbySUs = 1;
		}

		if ((sg->saAmfSGNumPrefStandbySUs == 0) || (sg->saAmfSGNumPrefStandbySUs == 1))
			LOG_NO("'saAmfSGNumPrefStandbySUs' is set to %u, not useful for NpM",
					sg->saAmfSGNumPrefStandbySUs);
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGNumPrefInserviceSUs"), attributes, 0, &sg->saAmfSGNumPrefInserviceSUs) != SA_AIS_OK) {
		/* empty => later assign number of SUs */
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGNumPrefAssignedSUs"), attributes, 0, &sg->saAmfSGNumPrefAssignedSUs) != SA_AIS_OK) {
		/* empty => later assign to saAmfSGNumPrefInserviceSUs */
	}

	sg->saAmfSGMaxActiveSIsperSU = -1; // magic number for no limit

	/* saAmfSGMaxActiveSIsperSU: "This attribute is only applicable to N+M, N-way, and N-way active redundancy models." */
	if ((sgt->saAmfSgtRedundancyModel == SA_AMF_NPM_REDUNDANCY_MODEL) ||
	    (sgt->saAmfSgtRedundancyModel == SA_AMF_N_WAY_REDUNDANCY_MODEL) ||
	    (sgt->saAmfSgtRedundancyModel == SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL)) {
		(void) immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGMaxActiveSIsperSU"), attributes, 0, &sg->saAmfSGMaxActiveSIsperSU);
	} else {
		SaUint32T tmp;
		if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGMaxActiveSIsperSU"), attributes, 0, &tmp) == SA_AIS_OK) {
			LOG_NO("'%s' attribute saAmfSGMaxActiveSIsperSU ignored, not valid for red model",
				sg->name.c_str());
		}
	}

	sg->saAmfSGMaxStandbySIsperSU = -1; // magic number for no limit

	/* saAmfSGMaxStandbySIsperSU: "This attribute is only applicable to N+M and N-way redundancy models." */
	if ((sgt->saAmfSgtRedundancyModel == SA_AMF_NPM_REDUNDANCY_MODEL) ||
	    (sgt->saAmfSgtRedundancyModel == SA_AMF_N_WAY_REDUNDANCY_MODEL)) {
		(void) immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGMaxStandbySIsperSU"), attributes, 0, &sg->saAmfSGMaxStandbySIsperSU);
	} else {
		SaUint32T tmp;
		if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGMaxStandbySIsperSU"), attributes, 0, &tmp) == SA_AIS_OK) {
			LOG_NO("'%s' attribute saAmfSGMaxStandbySIsperSU ignored, not valid for red model",
				sg->name.c_str());
		}
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGAutoAdjustProb"), attributes, 0, &sg->saAmfSGAutoAdjustProb) != SA_AIS_OK) {
		sg->saAmfSGAutoAdjustProb = sgt->saAmfSgtDefAutoAdjustProb;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGCompRestartProb"), attributes, 0, &sg->saAmfSGCompRestartProb) != SA_AIS_OK) {
		sg->saAmfSGCompRestartProb = sgt->saAmfSgtDefCompRestartProb;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGCompRestartMax"), attributes, 0, &sg->saAmfSGCompRestartMax) != SA_AIS_OK) {
		sg->saAmfSGCompRestartMax = sgt->saAmfSgtDefCompRestartMax;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGSuRestartProb"), attributes, 0, &sg->saAmfSGSuRestartProb) != SA_AIS_OK) {
		sg->saAmfSGSuRestartProb = sgt->saAmfSgtDefSuRestartProb;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGSuRestartMax"), attributes, 0, &sg->saAmfSGSuRestartMax) != SA_AIS_OK) {
		sg->saAmfSGSuRestartMax = sgt->saAmfSgtDefSuRestartMax;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGAdminState"), attributes, 0, &sg->saAmfSGAdminState) != SA_AIS_OK) {
		sg->saAmfSGAdminState = SA_AMF_ADMIN_UNLOCKED;
	}
	/*  TODO use value in type instead? */
	sg->sg_redundancy_model = sgt->saAmfSgtRedundancyModel;

	rc = 0;

	if (rc != 0) {
		avd_sg_delete(sg);
		sg = nullptr;
	}

	TRACE_LEAVE();
	return sg;
}

/**
 * Get configuration for all AMF SG objects from IMM and
 * create AVD internal objects.
 * @param cb
 * 
 * @return int
 */
SaAisErrorT avd_sg_config_get(const std::string& app_dn, AVD_APP *app)
{
	AVD_SG *sg;
	SaAisErrorT error, rc;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfSG";
	SaImmAttrNameT configAttributes[] = {
		const_cast<SaImmAttrNameT>("saAmfSGType"),
		const_cast<SaImmAttrNameT>("saAmfSGSuHostNodeGroup"),
		const_cast<SaImmAttrNameT>("saAmfSGAutoRepair"),
		const_cast<SaImmAttrNameT>("saAmfSGAutoAdjust"),
		const_cast<SaImmAttrNameT>("saAmfSGNumPrefActiveSUs"),
		const_cast<SaImmAttrNameT>("saAmfSGNumPrefStandbySUs"),
		const_cast<SaImmAttrNameT>("saAmfSGNumPrefInserviceSUs"),
		const_cast<SaImmAttrNameT>("saAmfSGNumPrefAssignedSUs"),
		const_cast<SaImmAttrNameT>("saAmfSGMaxActiveSIsperSU"),
		const_cast<SaImmAttrNameT>("saAmfSGMaxStandbySIsperSU"),
		const_cast<SaImmAttrNameT>("saAmfSGAutoAdjustProb"),
		const_cast<SaImmAttrNameT>("saAmfSGCompRestartProb"),
		const_cast<SaImmAttrNameT>("saAmfSGCompRestartMax"),
		const_cast<SaImmAttrNameT>("saAmfSGSuRestartProb"),
		const_cast<SaImmAttrNameT>("saAmfSGSuRestartMax"),
		const_cast<SaImmAttrNameT>("saAmfSGAdminState"),
		nullptr
	};

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_o2(avd_cb->immOmHandle, app_dn.c_str(), SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR, &searchParam,
		configAttributes, &searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("%s: saImmOmSearchInitialize_2 failed: %u", __FUNCTION__, error);
		goto done1;
	}

	while ((rc = immutil_saImmOmSearchNext_2(searchHandle, &dn,
		(SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK) {

		if (!is_config_valid(Amf::to_string(&dn), attributes, nullptr)) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}

		if ((sg = sg_create(Amf::to_string(&dn), attributes)) == nullptr) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}

		sg_add_to_model(sg);

		if (avd_su_config_get(Amf::to_string(&dn), sg) != SA_AIS_OK) {
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

/**
 * Check if the node group represented by ngname is a subset of the node group
 * represented by supername
 * @param ngname
 * @param supername
 * @return true/false
 */
static bool ng_is_subset(const std::string& ngname, const AVD_AMF_NG *superng)
{
	const AVD_AMF_NG *ng = avd_ng_get(ngname);

	if (superng->number_nodes() < ng->number_nodes())
		return false;

	if (std::includes(superng->saAmfNGNodeList.begin(), superng->saAmfNGNodeList.end(),
		ng->saAmfNGNodeList.begin(), ng->saAmfNGNodeList.end()) == true) {
		return true;
	}

	return false;
}

/**
 * Validate if the change in node group value is OK, report to IMM otherwise
 * @param sg
 * @param ng_name
 * @param opdata
 * @return true/false
 */
static bool ng_change_is_valid(const AVD_SG *sg, const std::string& ng_name,
		const CcbUtilOperationData_t *opdata)
{
	if (sg->saAmfSGSuHostNodeGroup.length() > 0) {
		// A node group is currently configured for SG. A user wants
		// to change it. Validate that the old node group is subset
		// of the new one.

		const AVD_AMF_NG *ng = avd_ng_get(ng_name);
		if (ng == nullptr) {
			report_ccb_validation_error(opdata,
					"Node Group '%s' not found", ng_name.c_str());
			return false;
		}

		if (ng_is_subset(sg->saAmfSGSuHostNodeGroup, ng) == false) {
			report_ccb_validation_error(opdata,
				"'%s' is not a subset of '%s'",
				sg->saAmfSGSuHostNodeGroup.c_str(), ng_name.c_str());
			return false;
		}
	} else {
		// a node group is currently not configured for this SG
		// don't allow configuring it now, why?
		report_ccb_validation_error(opdata,
			"Attribute saAmfSGSuHostNodeGroup cannot be modified");
		return false;
	}

	return true;
}

static SaAisErrorT ccb_completed_modify_hdlr(const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_OK;
	AVD_SG *sg;
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;
	bool value_is_deleted = false;

	TRACE_ENTER2("'%s'", osaf_extended_name_borrow(&opdata->objectName));

	sg = sg_db->find(Amf::to_string(&opdata->objectName));
	osafassert(sg != nullptr);

	/* Validate whether we can modify it. */

	if ((sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) ||
		(sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED)) {

		i = 0;
		while ((attr_mod = opdata->param.modify.attrMods[i++]) != nullptr) {
			const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;
			void *value = nullptr;

			/* Attribute value removed */
			if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) ||
					(attribute->attrValues == nullptr))
				value_is_deleted = true;
			else {
				value_is_deleted = false;
				value = attribute->attrValues[0];
			}

			if (!strcmp(attribute->attrName, "saAmfSGType")) {
				if (value_is_deleted == true)
					continue;
				SaNameT sg_type_name = *((SaNameT *)value);

				if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
					report_ccb_validation_error(opdata,
						"%s: Attribute saAmfSGType cannot be modified"
						" when SG is not in locked instantion", __FUNCTION__);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}

				if (sgtype_db->find(Amf::to_string(&sg_type_name)) == nullptr) {
					report_ccb_validation_error(opdata,
						"SG Type '%s' not found", osaf_extended_name_borrow(&sg_type_name));
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}

			} else if (!strcmp(attribute->attrName, "saAmfSGSuHostNodeGroup")) {
				if (value_is_deleted == true) {
					report_ccb_validation_error(opdata,
						"Deletion or Modification to NULL value not allowed");
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
				if (ng_change_is_valid(sg, Amf::to_string(static_cast<SaNameT*>(value)), opdata) == false) {
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
			} else if (!strcmp(attribute->attrName, "saAmfSGAutoAdjust")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefActiveSUs")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefStandbySUs")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefInserviceSUs")) {
				if (value_is_deleted == true)
					continue;
				uint32_t pref_inservice_su;
				pref_inservice_su = *((SaUint32T *)value);

				if ((pref_inservice_su == 0) ||
					((sg->sg_redundancy_model < SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL) &&
					 (pref_inservice_su < AVSV_SG_2N_PREF_INSVC_SU_MIN))) {
					report_ccb_validation_error(opdata,
						"%s: Minimum preferred num of su should be 2 in"
						" 2N, N+M and NWay red models", __FUNCTION__);
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
			} else if (!strcmp(attribute->attrName, "saAmfSGAutoRepair")) {
				if (value_is_deleted == true)
					continue;
				uint32_t sg_autorepair = *((SaUint32T *)attribute->attrValues[0]);
				if (sg_autorepair > true ) {
					report_ccb_validation_error(opdata,
						"Invalid saAmfSGAutoRepair SG:'%s'", sg->name.c_str());
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
			} else {
				report_ccb_validation_error(opdata,
					"Unknown attribute '%s'",
					attribute->attrName);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		}		/* while (attr_mod != nullptr) */

	} else {	/* Admin state is UNLOCKED */
		i = 0;
		/* Modifications can be done for the following parameters. */
		while ((attr_mod = opdata->param.modify.attrMods[i++]) != nullptr) {
			const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;
			void *value = nullptr;

			/* Attribute value removed */
			if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) ||
					(attribute->attrValues == nullptr))
				value_is_deleted = true;
			else {
				value_is_deleted = false;
				value = attribute->attrValues[0];
			}
			if (!strcmp(attribute->attrName, "saAmfSGSuRestartProb")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGSuRestartMax")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGCompRestartProb")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGCompRestartMax")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefInserviceSUs")) {
				if (value_is_deleted == true)
					continue;
				uint32_t pref_inservice_su;
				pref_inservice_su = *((SaUint32T *)value);

				if ((pref_inservice_su == 0) ||
					((sg->sg_redundancy_model < SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL) &&
					 (pref_inservice_su < AVSV_SG_2N_PREF_INSVC_SU_MIN))) {
					report_ccb_validation_error(opdata,
						"%s: Minimum preferred num of su should be 2"
						" in 2N, N+M and NWay red models", __FUNCTION__);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
			} else if (!strcmp(attribute->attrName, "saAmfSGAutoRepair")) {
				if (value_is_deleted == true)
					continue;
				uint32_t sg_autorepair = *((SaUint32T *)attribute->attrValues[0]);
				if (sg_autorepair > SA_TRUE) {
					report_ccb_validation_error(opdata,
						"Invalid saAmfSGAutoRepair SG:'%s'", sg->name.c_str());
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
			} else if (!strcmp(attribute->attrName, "saAmfSGSuHostNodeGroup")) {
				if (value_is_deleted == true) {
					report_ccb_validation_error(opdata,
						"Deletion or Modification to NULL value not allowed");
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
				if (ng_change_is_valid(sg, Amf::to_string(static_cast<SaNameT*>(value)), opdata) == false) {
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefActiveSUs")) {
				if (value_is_deleted == true)
					continue;
				uint32_t pref_active_su = *static_cast<SaUint32T *>(value);

				if (sg->sg_redundancy_model != SA_AMF_NPM_REDUNDANCY_MODEL) {
					report_ccb_validation_error(opdata,
						"%s: saAmfSGNumPrefActiveSUs for non-N+M model cannot"
						" be modified when SG is unlocked", __FUNCTION__);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				} else if (pref_active_su < sg->saAmfSGNumPrefActiveSUs) {
					report_ccb_validation_error(opdata,
						"%s: Cannot decrease saAmfSGNumPrefActiveSUs while SG"
						" is unlocked ", __FUNCTION__);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				} else {
					uint32_t increase_amount = pref_active_su - sg->saAmfSGNumPrefActiveSUs;

					if (sg->saAmfSGNumCurrInstantiatedSpareSUs < increase_amount) {
						report_ccb_validation_error(opdata,
							"%s: Not enough instantiated spare SUs to do"
							" in-service increase of saAmfSGNumPrefActiveSUs",
							__FUNCTION__);
						rc = SA_AIS_ERR_BAD_OPERATION;
						goto done;
					}
				}
			} else if (!strcmp(attribute->attrName, "saAmfSGMaxStandbySIsperSU")) {
				if (value_is_deleted == true)
					continue;
				uint32_t max_standby_sis = *static_cast<SaUint32T *>(value);

				if (sg->sg_redundancy_model != SA_AMF_NPM_REDUNDANCY_MODEL) {
					report_ccb_validation_error(opdata,
						"%s: saAmfSGMaxStandbySIsperSU for non-N+M model cannot"
						" be modified when SG is unlocked", __FUNCTION__);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				} else if (max_standby_sis < sg->saAmfSGMaxStandbySIsperSU) {
					report_ccb_validation_error(opdata,
						"%s: Cannot decrease saAmfSGMaxStandbySIsperSU while SG"
						" is unlocked ", __FUNCTION__);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
			} else {
				report_ccb_validation_error(opdata,
					"%s: Attribute '%s' cannot be modified when SG is unlocked",
					__FUNCTION__, attribute->attrName);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		}		/* while (attr_mod != nullptr) */
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
	AVD_AVND *su_node_ptr = nullptr;
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
	for (const auto& su : sg->list_of_su) {
		su_node_ptr = su->get_node_ptr();

		if ((su_node_ptr) && (su_node_ptr->node_state == AVD_AVND_STATE_PRESENT)) {
			SaNameT su_name;
			osaf_extended_name_lend(su->name.c_str(), &su_name);
			param.name = su_name;

			if (avd_snd_op_req_msg(avd_cb, su_node_ptr, &param) != NCSCC_RC_SUCCESS) {
				LOG_ER("%s::failed for %s",__FUNCTION__, su_node_ptr->name.c_str());
			}
		}
	}
	TRACE_LEAVE();
}

static void ccb_apply_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	AVD_SG *sg;
	AVD_AMF_SG_TYPE *sg_type;
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;
	void *value = nullptr;
	bool value_is_deleted;

	TRACE_ENTER2("'%s'", osaf_extended_name_borrow(&opdata->objectName));

	sg = sg_db->find(Amf::to_string(&opdata->objectName));
	assert(sg != nullptr);

	sg_type = sgtype_db->find(sg->saAmfSGType);
	osafassert(nullptr != sg_type);

	if (sg->saAmfSGAdminState != SA_AMF_ADMIN_UNLOCKED) {
		attr_mod = opdata->param.modify.attrMods[i++];
		while (attr_mod != nullptr) {
			const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;

			if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) ||	(attribute->attrValues == nullptr)) {
				/* Attribute value is deleted, revert to default value */
				value_is_deleted = true;
			} else {
				/* Attribute value is modified */
				value_is_deleted = false;
				value = attribute->attrValues[0];
			}

			if (!strcmp(attribute->attrName, "saAmfSGType")) {
				SaNameT sg_type_name = *((SaNameT *)value);
				TRACE("saAmfSGType modified from '%s' to '%s' of Sg'%s'", sg->saAmfSGType.c_str(), 
						osaf_extended_name_borrow(&sg_type_name), sg->name.c_str());

				sg_type = sgtype_db->find(Amf::to_string(&sg_type_name));
				osafassert(nullptr != sg_type);

				/* Remove from old type */
				avd_sgtype_remove_sg(sg);

				sg->saAmfSGType = Amf::to_string(&sg_type_name);

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
				TRACE("Old saAmfSGAutoRepair is '%u'", sg->saAmfSGAutoRepair);
				if (value_is_deleted) {
					sg->saAmfSGAutoRepair = sg_type->saAmfSgtDefAutoRepair;
					sg->saAmfSGAutoRepair_configured = false;
				} else {
					sg->saAmfSGAutoRepair = static_cast<SaBoolT>(*((SaUint32T *)value));
					sg->saAmfSGAutoRepair_configured = true; 
				}
				TRACE("Modified saAmfSGAutoRepair is '%u'", sg->saAmfSGAutoRepair);
				amflog(LOG_NOTICE, "%s saAmfSGAutoRepair changed to %u",
					sg->name.c_str(), sg->saAmfSGAutoRepair);
			} else if (!strcmp(attribute->attrName, "saAmfSGAutoAdjust")) {
				TRACE("Old saAmfSGAutoAdjust is '%u'", sg->saAmfSGAutoAdjust);
				if (value_is_deleted)
					sg->saAmfSGAutoAdjust = sg_type->saAmfSgtDefAutoAdjust;
				else
					sg->saAmfSGAutoAdjust = static_cast<SaBoolT>(*((SaUint32T *)value));
				TRACE("Modified saAmfSGAutoAdjust is '%u'", sg->saAmfSGAutoAdjust);
			} else if (!strcmp(attribute->attrName, "saAmfSGAutoAdjust")) {
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefActiveSUs")) {
				if (value_is_deleted)
					sg->saAmfSGNumPrefActiveSUs = 1;
				else
					sg->saAmfSGNumPrefActiveSUs = *((SaUint32T *)value);
				TRACE("Modified saAmfSGNumPrefActiveSUs is '%u'", sg->saAmfSGNumPrefActiveSUs);
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefStandbySUs")) {
				if (value_is_deleted)
					sg->saAmfSGNumPrefStandbySUs = 1;
				else
					sg->saAmfSGNumPrefStandbySUs = *((SaUint32T *)value);
				TRACE("Modified saAmfSGNumPrefStandbySUs is '%u'", sg->saAmfSGNumPrefStandbySUs);
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefInserviceSUs")) {
				if (value_is_deleted)
					sg->saAmfSGNumPrefInserviceSUs = ~0;
				else
					sg->saAmfSGNumPrefInserviceSUs = *((SaUint32T *)value);
				TRACE("Modified saAmfSGNumPrefInserviceSUs is '%u'", sg->saAmfSGNumPrefInserviceSUs);
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefAssignedSUs")) {
				if (value_is_deleted)
					sg->saAmfSGNumPrefAssignedSUs = sg->saAmfSGNumPrefInserviceSUs;
				else
					sg->saAmfSGNumPrefAssignedSUs = *((SaUint32T *)value);
				TRACE("Modified saAmfSGNumPrefAssignedSUs is '%u'", sg->saAmfSGNumPrefAssignedSUs);
			} else if (!strcmp(attribute->attrName, "saAmfSGMaxActiveSIsperSU")) {
				if (value_is_deleted)
					sg->saAmfSGMaxActiveSIsperSU = -1;
				else {
					if ((sg->sg_redundancy_model == SA_AMF_NPM_REDUNDANCY_MODEL) ||
							(sg->sg_redundancy_model == SA_AMF_N_WAY_REDUNDANCY_MODEL) ||
							(sg->sg_redundancy_model == SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL)) {
						sg->saAmfSGMaxActiveSIsperSU = *((SaUint32T *)value);
						TRACE("Modified saAmfSGMaxActiveSIsperSU is '%u'",
								sg->saAmfSGMaxActiveSIsperSU);
					} else {
						LOG_NO("'%s' attribute saAmfSGMaxActiveSIsperSU not modified,"
								" not valid for Nored/2N Redundancy models",
								sg->name.c_str());
					}
				}
			} else if (!strcmp(attribute->attrName, "saAmfSGMaxStandbySIsperSU")) {
				if (value_is_deleted)
					sg->saAmfSGMaxStandbySIsperSU = -1;
				else {
					if ((sg->sg_redundancy_model == SA_AMF_NPM_REDUNDANCY_MODEL) ||
							(sg->sg_redundancy_model == SA_AMF_N_WAY_REDUNDANCY_MODEL)) {
						sg->saAmfSGMaxStandbySIsperSU = *((SaUint32T *)value);
						TRACE("Modified saAmfSGMaxStandbySIsperSU is '%u'",
								sg->saAmfSGMaxStandbySIsperSU);
					} else {
						LOG_NO("'%s' attribute saAmfSGMaxStandbySIsperSU not modified,"
								" not valid for Nored/2N/NwayAct Redundancy models",
								sg->name.c_str());
					}
				}
			} else if (!strcmp(attribute->attrName, "saAmfSGAutoAdjustProb")) {
				if (value_is_deleted)
					sg->saAmfSGAutoAdjustProb = sg_type->saAmfSgtDefAutoAdjustProb;
				else
					sg->saAmfSGAutoAdjustProb = *((SaTimeT *)value);
				TRACE("Modified saAmfSGAutoAdjustProb is '%llu'", sg->saAmfSGAutoAdjustProb);
			} else if (!strcmp(attribute->attrName, "saAmfSGCompRestartProb")) {
				if (value_is_deleted)
					sg->saAmfSGCompRestartProb = sg_type->saAmfSgtDefCompRestartProb;
				else
					sg->saAmfSGCompRestartProb = *((SaTimeT *)value);
				TRACE("Modified saAmfSGCompRestartProb is '%llu'", sg->saAmfSGCompRestartProb);
				sg_nd_attribute_update(sg, saAmfSGCompRestartProb_ID);
			} else if (!strcmp(attribute->attrName, "saAmfSGCompRestartMax")) {
				if (value_is_deleted)
					sg->saAmfSGCompRestartMax = sg_type->saAmfSgtDefCompRestartMax;
				else
					sg->saAmfSGCompRestartMax = *((SaUint32T *)value);
				TRACE("Modified saAmfSGCompRestartMax is '%u'", sg->saAmfSGCompRestartMax);
				sg_nd_attribute_update(sg, saAmfSGCompRestartMax_ID);
			} else if (!strcmp(attribute->attrName, "saAmfSGSuRestartProb")) {
				if (value_is_deleted)
					sg->saAmfSGSuRestartProb = sg_type->saAmfSgtDefSuRestartProb;
				else
					sg->saAmfSGSuRestartProb = *((SaTimeT *)value);
				TRACE("Modified saAmfSGSuRestartProb is '%llu'", sg->saAmfSGSuRestartProb);
				sg_nd_attribute_update(sg, saAmfSGSuRestartProb_ID);
			} else if (!strcmp(attribute->attrName, "saAmfSGSuRestartMax")) {
				if (value_is_deleted)
					sg->saAmfSGSuRestartMax = sg_type->saAmfSgtDefSuRestartMax;
				else
					sg->saAmfSGSuRestartMax = *((SaUint32T *)value);
				TRACE("Modified saAmfSGSuRestartMax is '%u'", sg->saAmfSGSuRestartMax);
				sg_nd_attribute_update(sg, saAmfSGSuRestartMax_ID);
			} else if (!strcmp(attribute->attrName, "saAmfSGSuHostNodeGroup")) {
				sg->saAmfSGSuHostNodeGroup = Amf::to_string(static_cast<SaNameT *>(value));
			} else {
				osafassert(0);
			}

			attr_mod = opdata->param.modify.attrMods[i++];
		}
	} else {			/* Admin state is UNLOCKED */
		bool realign = false;
		i = 0;
		/* Modifications can be done for the following parameters. */
		attr_mod = opdata->param.modify.attrMods[i++];
		while (attr_mod != nullptr) {
			const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;

			if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) ||	(attribute->attrValues == nullptr)) {
				/* Attribute value is deleted, revert to default value */
				value_is_deleted = true;
			} else {
				/* Attribute value is modified */
				value_is_deleted = false;
				value = attribute->attrValues[0];
			}

			if (!strcmp(attribute->attrName, "saAmfSGNumPrefInserviceSUs")) {
				if (value_is_deleted)
					sg->saAmfSGNumPrefInserviceSUs = ~0;
				else
					sg->saAmfSGNumPrefInserviceSUs = *((SaUint32T *)value);
				TRACE("Modified saAmfSGNumPrefInserviceSUs is '%u'", sg->saAmfSGNumPrefInserviceSUs);

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
				TRACE("Modified saAmfSGCompRestartProb is '%llu'", sg->saAmfSGCompRestartProb);
				sg_nd_attribute_update(sg, saAmfSGCompRestartProb_ID);

			} else if (!strcmp(attribute->attrName, "saAmfSGCompRestartMax")) {
				if (value_is_deleted)
					sg->saAmfSGCompRestartMax = sg_type->saAmfSgtDefCompRestartMax;
				else
					sg->saAmfSGCompRestartMax = *((SaUint32T *)value);
				TRACE("Modified saAmfSGCompRestartMax is '%u'", sg->saAmfSGCompRestartMax);
				sg_nd_attribute_update(sg, saAmfSGCompRestartMax_ID);

			} else if (!strcmp(attribute->attrName, "saAmfSGSuRestartProb")) {
				if (value_is_deleted)
					sg->saAmfSGSuRestartProb = sg_type->saAmfSgtDefSuRestartProb;
				else
					sg->saAmfSGSuRestartProb = *((SaTimeT *)value);
				TRACE("Modified saAmfSGSuRestartProb is '%llu'", sg->saAmfSGSuRestartProb);
				sg_nd_attribute_update(sg, saAmfSGSuRestartProb_ID);

			} else if (!strcmp(attribute->attrName, "saAmfSGSuRestartMax")) {
				if (value_is_deleted)
					sg->saAmfSGSuRestartMax = sg_type->saAmfSgtDefSuRestartMax;
				else
					sg->saAmfSGSuRestartMax = *((SaUint32T *)value);
				TRACE("Modified saAmfSGSuRestartMax is '%u'", sg->saAmfSGSuRestartMax);
				sg_nd_attribute_update(sg, saAmfSGSuRestartMax_ID);
			} else if (!strcmp(attribute->attrName, "saAmfSGAutoRepair")) {
				if (value_is_deleted) {
					sg->saAmfSGAutoRepair = sg_type->saAmfSgtDefAutoRepair;
					sg->saAmfSGAutoRepair_configured = false;
				} else {
					sg->saAmfSGAutoRepair = static_cast<SaBoolT>(*((SaUint32T *)value));
					sg->saAmfSGAutoRepair_configured = true; 
				}
				TRACE("Modified saAmfSGAutoRepair is '%u'", sg->saAmfSGAutoRepair);
				amflog(LOG_NOTICE, "%s saAmfSGAutoRepair changed to %u",
					sg->name.c_str(), sg->saAmfSGAutoRepair);
			} else if (!strcmp(attribute->attrName, "saAmfSGSuHostNodeGroup")) {
				sg->saAmfSGSuHostNodeGroup = Amf::to_string(static_cast<SaNameT*>(value));
			} else if (!strcmp(attribute->attrName, "saAmfSGNumPrefActiveSUs")) {
				if (value_is_deleted) {
					/* TODO: This really should be handled in ccb_completed_modify_hdlr */
					LOG_WA("not deleting saAmfSGNumPrefActiveSUs while in UNLOCKED");
				} else {
					sg->saAmfSGNumPrefActiveSUs = *static_cast<SaUint32T *>(value);

					if (avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE)  {
						/* find an instantiated spare SU */
						realign = true;
					}
				}
			} else if (!strcmp(attribute->attrName, "saAmfSGMaxStandbySIsperSU")) {
				if (value_is_deleted) {
					sg->saAmfSGMaxStandbySIsperSU = -1;
				} else {
					sg->saAmfSGMaxStandbySIsperSU = *static_cast<SaUint32T *>(value);

					if (avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE)  {
						/* any standbys need assignment? */
						realign = true;
					}
				}
			} else {
				osafassert(0);
			}

			attr_mod = opdata->param.modify.attrMods[i++];
		}		/* while (attr_mod != nullptr) */

		if (realign) {
			if (sg->realign(avd_cb, sg) != NCSCC_RC_SUCCESS) {
				osafassert(0);
			}
		}
	}			/* Admin state is UNLOCKED */
}

/**
 * Terminate SU in reverse order  
 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 */
uint32_t AVD_SG::term_su_list_in_reverse()
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVD_SU *su;

	TRACE_ENTER2("sg: %s", this->name.c_str());
	for (auto iter = list_of_su.rbegin(); iter != list_of_su.rend(); ++iter) {
		su = *iter;
		TRACE("terminate su:'%s'", su ? su->name.c_str() : nullptr);

		if ((su->saAmfSUPreInstantiable == true) &&
			(su->saAmfSUPresenceState != SA_AMF_PRESENCE_UNINSTANTIATED) &&
			(su->saAmfSUPresenceState != SA_AMF_PRESENCE_INSTANTIATION_FAILED) &&
			(su->saAmfSUPresenceState != SA_AMF_PRESENCE_TERMINATION_FAILED)) {

			if (avd_snd_presence_msg(avd_cb, su, true) == NCSCC_RC_SUCCESS) {
				su->set_term_state(true);
			} else {
				rc = NCSCC_RC_FAILURE;
				LOG_WA("Failed Termination '%s'", su->name.c_str());
			}
		}
	}

	TRACE_LEAVE();
       
	return rc ;
}
/**
 * perform lock-instantiation on a given SG
 * @param cb
 * @param sg
 */
static uint32_t sg_app_sg_admin_lock_inst(AVD_CL_CB *cb, AVD_SG *sg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%s", sg->name.c_str());

	/* terminate all the SUs on this Node */
	if (sg->list_of_su.empty() == false)
		rc = sg->term_su_list_in_reverse();

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
	uint32_t su_try_inst;

	TRACE_ENTER2("%s", sg->name.c_str());

	/* Instantiate the SUs in this SG */
	su_try_inst = 0;
	
	for (const auto& su : sg->list_of_su) {
		if ((su->saAmfSUAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
				(su->su_on_node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
				(su->saAmfSUOperState == SA_AMF_OPERATIONAL_ENABLED) &&
				(su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED)) {

			if (su->saAmfSUPreInstantiable == true) {
				if (su->su_on_node->node_state == AVD_AVND_STATE_PRESENT) {
					if (su->sg_of_su->saAmfSGNumPrefInserviceSUs > su_try_inst) {
						if (avd_snd_presence_msg(cb, su, false) != NCSCC_RC_SUCCESS) {
							LOG_NO("%s: Failed to send Instantiation order of '%s' to %x",
									__FUNCTION__, su->name.c_str(),
									su->su_on_node->node_info.nodeId);
						} else {
							su_try_inst++;
						}
					}
				}

				/* set uncondionally of msg snd outcome */
				su->set_term_state(false);
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
	for (const auto& si : sg->list_of_si) {
		if (si->si_dep_state == AVD_SI_TOL_TIMER_RUNNING) {
			TRACE("Tolerance timer running for si: %s",si->name.c_str());
			return true;
		}
	}

	return false;
}
static void sg_admin_op_cb(SaImmOiHandleT immOiHandle, SaInvocationT invocation,
	const SaNameT *object_name, SaImmAdminOperationIdT op_id,
	const SaImmAdminOperationParamsT_2 **params)
{
	AVD_SG *sg;
	SaAmfAdminStateT adm_state;
	AVD_AVND *node;

	TRACE_ENTER2("'%s', %llu", osaf_extended_name_borrow(object_name), op_id);
	sg = sg_db->find(Amf::to_string(object_name));

	if (sg->sg_ncs_spec == true) {
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
				"Admin Op on OpenSAF MW SG is not allowed");
		goto done;
	}

	if (sg->any_assignment_absent() == true ) {
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
				"SG has absent assignment (%s)", sg->name.c_str());
		goto done;
	}

	if (sg->sg_fsm_state != AVD_SG_FSM_STABLE) {
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
				"SG not in STABLE state (%s)", sg->name.c_str());
		goto done;
	}

	/* Avoid multiple admin operations on other SUs belonging to the same SG. */
	for (const auto& su : sg->list_of_su) {
		node = su->get_node_ptr();
		if (su->pend_cbk.invocation != 0) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
					"Admin operation'%u' is already going on su'%s' belonging to the same SG",
					su->pend_cbk.admin_oper, su->name.c_str());
			goto done;
		} else if (node->admin_node_pend_cbk.admin_oper != 0) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
					"Node'%s' hosting SU'%s' belonging to the same SG, undergoing admin"
					" operation'%u'", node->name.c_str(), su->name.c_str(),
					node->admin_node_pend_cbk.admin_oper);
			goto done;
		}
	}

	if ((sg->adminOp_invocationId  != 0) || (sg->adminOp != 0))  {
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
				"Admin operation is going on (%s)", sg->name.c_str());
		goto done;
	}
        /* Avoid if any single Csi assignment is undergoing on SG. */
        if (csi_assignment_validate(sg) == true) {
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
				"Single Csi assignment undergoing on (sg'%s')", sg->name.c_str());
                goto done;
        }

	/* if Tolerance timer is running for any SI's withing this SG, then return SA_AIS_ERR_TRY_AGAIN */
	if (sg_is_tolerance_timer_running_for_any_si(sg)) {
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
				"Tolerance timer is running for some of the SI's in the SG '%s',"
				" so differing admin opr", sg->name.c_str());
		goto done;
	}
	switch (op_id) {
	case SA_AMF_ADMIN_UNLOCK:
		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_UNLOCKED) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
					"%s is already unlocked", osaf_extended_name_borrow(object_name));
			goto done;
		}

		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"%s is locked instantiation", osaf_extended_name_borrow(object_name));
			goto done;
		}

		adm_state = sg->saAmfSGAdminState;
		avd_sg_admin_state_set(sg, SA_AMF_ADMIN_UNLOCKED);
		if (avd_cb->init_state == AVD_INIT_DONE) {
			for (const auto& su : sg->list_of_su) {
				if (su->is_in_service() == true) {
					su->set_readiness_state(SA_AMF_READINESS_IN_SERVICE);
				}
			}
			break;
		}
		if (avd_sg_app_sg_admin_func(avd_cb, sg) != NCSCC_RC_SUCCESS) {
			avd_sg_admin_state_set(sg, adm_state);
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					nullptr);
			goto done;
		}
		sg->adminOp = SA_AMF_ADMIN_UNLOCK;
		break;

	case SA_AMF_ADMIN_LOCK:
		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
					"%s is already locked", osaf_extended_name_borrow(object_name));
			goto done;
		}

		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"%s is locked instantiation", osaf_extended_name_borrow(object_name));
			goto done;
		}

		adm_state = sg->saAmfSGAdminState;
		avd_sg_admin_state_set(sg, SA_AMF_ADMIN_LOCKED);

		if (avd_cb->init_state == AVD_INIT_DONE) {
			for (const auto& su : sg->list_of_su) {
				su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
			}
			break;
		}

		if (avd_sg_app_sg_admin_func(avd_cb, sg) != NCSCC_RC_SUCCESS) {
			avd_sg_admin_state_set(sg, adm_state);
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					nullptr);
			goto done;
		}

		sg->adminOp = SA_AMF_ADMIN_LOCK;
		break;
	case SA_AMF_ADMIN_SHUTDOWN:
		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
					"%s is shutting down", osaf_extended_name_borrow(object_name));
			goto done;
		}

		if ((sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) ||
		    (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION)) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"%s is locked (instantiation)", osaf_extended_name_borrow(object_name));
			goto done;
		}

		adm_state = sg->saAmfSGAdminState;
		avd_sg_admin_state_set(sg, SA_AMF_ADMIN_SHUTTING_DOWN);
		if (avd_sg_app_sg_admin_func(avd_cb, sg) != NCSCC_RC_SUCCESS) {
			avd_sg_admin_state_set(sg, adm_state);
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					nullptr);
			goto done;
		}
		sg->adminOp = SA_AMF_ADMIN_SHUTDOWN;
		break;
	case SA_AMF_ADMIN_LOCK_INSTANTIATION:
		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
					"%s is already locked-instantiation", osaf_extended_name_borrow(object_name));
			goto done;
		}

		if (sg->saAmfSGAdminState != SA_AMF_ADMIN_LOCKED) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"%s is not locked", osaf_extended_name_borrow(object_name));
			goto done;
		}

		adm_state = sg->saAmfSGAdminState;
		avd_sg_admin_state_set(sg, SA_AMF_ADMIN_LOCKED_INSTANTIATION);
		if (sg_app_sg_admin_lock_inst(avd_cb, sg) != NCSCC_RC_SUCCESS) {
			avd_sg_admin_state_set(sg, adm_state);
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					nullptr);
			goto done;
		}

		
		sg->adminOp = SA_AMF_ADMIN_LOCK_INSTANTIATION;
		break;
	case SA_AMF_ADMIN_UNLOCK_INSTANTIATION:
		if (sg->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
					"%s is already locked", osaf_extended_name_borrow(object_name));
			goto done;
		}

		if (sg->saAmfSGAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"%s is not in locked instantiation state", osaf_extended_name_borrow(object_name));
			goto done;
		}

		/* If any su is in terminating state, that means lock-in op
		   has not completed. Allow su to move into permanent state
		   i.e. either in uninstanted or term failed state. */
		for (const auto& su : sg->list_of_su) {
			if (su->saAmfSUPresenceState == SA_AMF_PRESENCE_TERMINATING) {
				report_admin_op_error(immOiHandle, invocation,
						SA_AIS_ERR_TRY_AGAIN, nullptr,
						"su'%s' in terminating state",
						su->name.c_str());
				goto done;
			}
		}

		avd_sg_admin_state_set(sg, SA_AMF_ADMIN_LOCKED);

		if ((sg->list_of_su.empty() == false) && (sg->first_su()->saAmfSUPreInstantiable == false)) {
			avd_sg_adjust_config(sg);
			avd_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);
			goto done;
		}
		sg_app_sg_admin_unlock_inst(avd_cb, sg);
		sg->adminOp = SA_AMF_ADMIN_UNLOCK_INSTANTIATION;

		break;
	case SA_AMF_ADMIN_SG_ADJUST:
	default:
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NOT_SUPPORTED, nullptr,
				"Admin Operation '%llu' not supported", op_id);
		goto done;
	}

	if ((op_id != SA_AMF_ADMIN_UNLOCK_INSTANTIATION) && (op_id != SA_AMF_ADMIN_LOCK_INSTANTIATION) 
		&& (sg->sg_fsm_state == AVD_SG_FSM_STABLE)) {
		avd_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);
		sg->adminOp = static_cast<SaAmfAdminOperationIdT>(0);
		goto done;
	}

	if ((op_id == SA_AMF_ADMIN_UNLOCK_INSTANTIATION) || (op_id == SA_AMF_ADMIN_LOCK_INSTANTIATION)) {
		if (sg_stable_after_lock_in_or_unlock_in(sg) == true) {
			avd_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);
			sg->adminOp = static_cast<SaAmfAdminOperationIdT>(0);
			goto done;
		}
	}

	sg->adminOp_invocationId = invocation;
done:
	TRACE_LEAVE();
}

static SaAisErrorT sg_rt_attr_cb(SaImmOiHandleT immOiHandle,
	const SaNameT *objName, const SaImmAttrNameT *attributeNames)
{
	const std::string object_name(Amf::to_string(objName));
	AVD_SG *sg = sg_db->find(object_name);
	SaImmAttrNameT attributeName;
	int i = 0;

	TRACE_ENTER2("'%s'", object_name.c_str());
	osafassert(sg != nullptr);

	while ((attributeName = attributeNames[i++]) != nullptr) {
		if (!strcmp("saAmfSGNumCurrAssignedSUs", attributeName)) {
			sg->saAmfSGNumCurrAssignedSUs = sg->curr_assigned_sus();
			avd_saImmOiRtObjectUpdate_sync(object_name, attributeName,
				SA_IMM_ATTR_SAUINT32T, &sg->saAmfSGNumCurrAssignedSUs);
		} else if (!strcmp("saAmfSGNumCurrNonInstantiatedSpareSUs", attributeName)) {
			sg->saAmfSGNumCurrNonInstantiatedSpareSUs = sg->curr_non_instantiated_spare_sus();
			avd_saImmOiRtObjectUpdate_sync(object_name, attributeName,
				SA_IMM_ATTR_SAUINT32T, &sg->saAmfSGNumCurrNonInstantiatedSpareSUs);
		} else if (!strcmp("saAmfSGNumCurrInstantiatedSpareSUs", attributeName)) {
			sg->saAmfSGNumCurrInstantiatedSpareSUs = sg->curr_instantiated_spare_sus();
			avd_saImmOiRtObjectUpdate_sync(object_name, attributeName,
				SA_IMM_ATTR_SAUINT32T, &sg->saAmfSGNumCurrInstantiatedSpareSUs);
		} else {
			LOG_ER("Ignoring unknown attribute '%s'", attributeName);
		}
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
	bool si_exist = false;
	CcbUtilOperationData_t *t_opData;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		rc = ccb_completed_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		sg = sg_db->find(Amf::to_string(&opdata->objectName));
		if (sg->list_of_si.empty() == false) {
			/* check whether there is parent app delete */
			const SaNameTWrapper app_name(sg->app->name);
			t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, app_name);
			if (t_opData == nullptr || t_opData->operationType != CCBUTIL_DELETE) {
				/* check whether there exists a delete operation for 
				 * each of the SIs in the SG's list in the current CCB
				 */
				for (const auto& si : sg->list_of_si) {
					const SaNameTWrapper si_name(si->name);
					t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, si_name);
					if ((t_opData == nullptr) || (t_opData->operationType != CCBUTIL_DELETE)) {
						si_exist = true;
						break;
					}
				}
			}
			if (si_exist == true) {
				report_ccb_validation_error(opdata, "SIs still exist in SG '%s'", sg->name.c_str());
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

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		sg = sg_create(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues);
		osafassert(sg);
		sg_add_to_model(sg);
		break;
	case CCBUTIL_DELETE:
		avd_sg_delete(static_cast<AVD_SG*>(opdata->userData));
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
	AVD_SG *sg = su->sg_of_su;

	if (su->sg_of_su != nullptr) {
		/* remove SU from SG */
		auto su_to_delete = std::find(sg->list_of_su.begin(),
			sg->list_of_su.end(),
			su);
		
		if (su_to_delete != sg->list_of_su.end()) {
			sg->list_of_su.erase(su_to_delete);
		} else {
			LOG_ER("su cannot be found");
		}

		su->sg_of_su = nullptr;
	} /* if (su->sg_of_su != AVD_SG_NULL) */

	avd_verify_equal_ranked_su(sg);
	return;
}

void avd_sg_add_su(AVD_SU* su)
{
	if ((su == nullptr) || (su->sg_of_su == nullptr))
		return;

	su->sg_of_su->list_of_su.push_back(su);

	// in descending order of SU rank (lowest to
	// highest integer). Note: the lower the integer
	// value, the higher the rank.
	std::sort(su->sg_of_su->list_of_su.begin(), su->sg_of_su->list_of_su.end(),
		[](const AVD_SU *a, const AVD_SU *b) -> bool {return a->saAmfSURank < b->saAmfSURank;});

	avd_verify_equal_ranked_su(su->sg_of_su);
}

void avd_sg_constructor(void)
{
	sg_db = new AmfDb<std::string, AVD_SG>;
	avd_class_impl_set("SaAmfSG", sg_rt_attr_cb, sg_admin_op_cb,
		sg_ccb_completed_cb, sg_ccb_apply_cb);
}

void avd_sg_admin_state_set(AVD_SG* sg, SaAmfAdminStateT state)
{
	SaAmfAdminStateT old_state = sg->saAmfSGAdminState;
	
	osafassert(state <= SA_AMF_ADMIN_SHUTTING_DOWN);
	if (sg->saAmfSGAdminState == state)
		return;
	TRACE_ENTER2("%s AdmState %s => %s", sg->name.c_str(),
			avd_adm_state_name[old_state], avd_adm_state_name[state]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s AdmState %s => %s", sg->name.c_str(),
                  avd_adm_state_name[old_state], avd_adm_state_name[state]);      
	sg->saAmfSGAdminState = state;
	avd_saImmOiRtObjectUpdate(sg->name,
		const_cast<SaImmAttrNameT>("saAmfSGAdminState"), SA_IMM_ATTR_SAUINT32T, &sg->saAmfSGAdminState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, sg, AVSV_CKPT_SG_ADMIN_STATE);

	/* Notification */ 
	avd_send_admin_state_chg_ntf(sg->name,
					SA_AMF_NTFID_SG_ADMIN_STATE,
					old_state,
					sg->saAmfSGAdminState);
	TRACE_LEAVE();
}

void AVD_SG::set_admin_state(SaAmfAdminStateT state) {
	SaAmfAdminStateT old_state = saAmfSGAdminState;

	osafassert(state <= SA_AMF_ADMIN_SHUTTING_DOWN);
	if (saAmfSGAdminState == state)
		return;
	TRACE_ENTER2("%s AdmState %s => %s", name.c_str(),
			avd_adm_state_name[old_state], avd_adm_state_name[state]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s AdmState %s => %s", name.c_str(),
                  avd_adm_state_name[old_state], avd_adm_state_name[state]);
	saAmfSGAdminState = state;
	avd_saImmOiRtObjectUpdate(name,
		  const_cast<SaImmAttrNameT>("saAmfSGAdminState"), SA_IMM_ATTR_SAUINT32T,
		  &saAmfSGAdminState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SG_ADMIN_STATE);

	avd_send_admin_state_chg_ntf(name, SA_AMF_NTFID_SG_ADMIN_STATE, old_state,
		saAmfSGAdminState);
	TRACE_LEAVE();
}

void AVD_SG::set_fsm_state(AVD_SG_FSM_STATE state, bool wrt_to_imm) {
	TRACE_ENTER();

	if (sg_fsm_state != state) {
		TRACE("%s sg_fsm_state %u => %u", name.c_str(), sg_fsm_state, state);
		sg_fsm_state = state;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SG_FSM_STATE);
		if (avd_cb->scs_absence_max_duration > 0 && wrt_to_imm) {
			avd_saImmOiRtObjectUpdate_sync(name,
						const_cast<SaImmAttrNameT>("osafAmfSGFsmState"),
						SA_IMM_ATTR_SAUINT32T, &sg_fsm_state);
		}
	}

	if (state == AVD_SG_FSM_STABLE) {
		osafassert(su_oper_list.empty() == true);
		if (adminOp_invocationId != 0) {
			TRACE("Admin operation finishes on SG:'%s'",name.c_str());
			avd_saImmOiAdminOperationResult(avd_cb->immOiHandle, adminOp_invocationId, SA_AIS_OK);
			adminOp_invocationId = 0;
			adminOp = static_cast<SaAmfAdminOperationIdT>(0);
		}
		for (const auto& si : list_of_si) {
			if (si->invocation != 0) {
				TRACE("Admin operation finishes on SI:'%s'",si->name.c_str());
				avd_saImmOiAdminOperationResult(avd_cb->immOiHandle,
						si->invocation, SA_AIS_OK);
				si->invocation = 0;
			}
		}
	}

	TRACE_LEAVE();
}

void AVD_SG::set_adjust_state(SaAdjustState state) {
	TRACE("%s adjust_state %u => %u", name.c_str(), adjust_state, state);
	adjust_state = state;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SG_ADJUST_STATE);
}

void AVD_SG::set_admin_si(AVD_SI *si) {
	TRACE("%s admin_si set to %s", name.c_str(), si->name.c_str());
	admin_si = si;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(avd_cb, this, AVSV_CKPT_AVD_SG_ADMIN_SI);
}

void AVD_SG::clear_admin_si() {
	if (admin_si != nullptr) {
		TRACE("%s admin_si cleared", name.c_str());
		m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, this, AVSV_CKPT_AVD_SG_ADMIN_SI);
		admin_si = nullptr;
	}
}

void AVD_SG::for_all_su_set_readiness_state(SaAmfReadinessStateT state) {
	for (const auto& su : list_of_su) {
		su->set_readiness_state(state);
	}
}

bool AVD_SG::in_su_oper_list(const AVD_SU *i_su) {
	if (std::find(su_oper_list.begin(), su_oper_list.end(), i_su) !=
		su_oper_list.end()) {
		TRACE("%s found in %s", i_su->name.c_str(), name.c_str());
		return true;
	}

	TRACE("%s not found in %s", i_su->name.c_str(), name.c_str());
	return false;
}

uint32_t AVD_SG::su_oper_list_add(AVD_SU *su) {
	// TODO(hafe) move implementation here later when all uses this method
	osafassert(this == su->sg_of_su);
	return avd_sg_su_oper_list_add(avd_cb, su, false);
}

uint32_t AVD_SG::su_oper_list_del(AVD_SU *su) {
	// TODO(hafe) move implementation here later when all uses this method
	osafassert(this == su->sg_of_su);
	return avd_sg_su_oper_list_del(avd_cb, su, false);
}

void AVD_SG::su_oper_list_clear() {
	TRACE_ENTER2("%s", name.c_str());
	for (auto it = su_oper_list.begin(); it != su_oper_list.end();) {
		osafassert(this == (*it)->sg_of_su);
		su_oper_list_del(*it++);
	}
	TRACE_LEAVE();
}

const AVD_SU* AVD_SG::su_oper_list_front() {
	if (su_oper_list.empty() == false) {
		return su_oper_list.front();
	} else {
		return nullptr;
	}
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
	uint32_t rank = 0;
	bool valid_rank = false;

	TRACE_ENTER();
	/* Check ranks are still equal or not. Mark true in the beginning*/
	avd_sg->equal_ranked_su = true;
	
	for (const auto& su : avd_sg->list_of_su) {
		if (valid_rank && rank != su->saAmfSURank) {
			avd_sg->equal_ranked_su = false;
			break;
		}
		
		// store rank for comparison
		rank = su->saAmfSURank;
		valid_rank = true;
	}

	TRACE_LEAVE2("avd_sg->equal_ranked_su '%u'", avd_sg->equal_ranked_su);

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
	if (sg->list_of_su.empty() == false && !(sg->first_su()->saAmfSUPreInstantiable)) {
		// NPI SUs can only be assigned one SI, see 3.2.1.1
		if ((sg->saAmfSGMaxActiveSIsperSU != static_cast<SaUint32T>(-1)) && (sg->saAmfSGMaxActiveSIsperSU > 1)) {
			LOG_WA("invalid saAmfSGMaxActiveSIsperSU (%u) for NPI SU '%s', adjusting...",
					sg->saAmfSGMaxActiveSIsperSU, sg->name.c_str());
		}
		sg->saAmfSGMaxActiveSIsperSU = 1;

		if ((sg->saAmfSGMaxStandbySIsperSU != static_cast<SaUint32T>(-1)) && (sg->saAmfSGMaxStandbySIsperSU > 1)) {
			LOG_WA("invalid saAmfSGMaxStandbySIsperSU (%u) for NPI SU '%s', adjusting...",
					sg->saAmfSGMaxStandbySIsperSU, sg->name.c_str());
		}
		sg->saAmfSGMaxStandbySIsperSU = 1;
		/* saAmfSUFailover must be true for a NPI SU sec 3.11.1.3.2 AMF-B.04.01 spec */
		for (const auto& su : sg->list_of_su) {
			if (!su->saAmfSUFailover) {
				su->set_su_failover(true);
			}
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
				sg->name.c_str(), sg->saAmfSGNumPrefAssignedSUs);
	}
}


/**
 * @brief Counts number of instantiated su in the sg.
 * @param Service Group
 *
 * @return Number of instantiated su in the sg.
 */
uint32_t sg_instantiated_su_count(const AVD_SG *sg)
{
	uint32_t inst_su_count = 0;

	TRACE_ENTER();
	for (const auto& su : sg->list_of_su) {
		TRACE_1("su'%s', pres state'%u', in_serv'%u', PrefIn'%u'", su->name.c_str(),
				su->saAmfSUPresenceState, su->saAmfSuReadinessState, sg->saAmfSGNumPrefInserviceSUs);
		if (((su->saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATED) ||
					(su->saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATING) ||
					(su->saAmfSUPresenceState == SA_AMF_PRESENCE_RESTARTING))) {
			inst_su_count ++;
		}
	}

	TRACE_LEAVE2("%u", inst_su_count);
	return inst_su_count;
}

// default implementation
SaAisErrorT AVD_SG::si_swap(AVD_SI *si, SaInvocationT invocation) {
	saflog(LOG_NOTICE, amfSvcUsrName,
		"SI SWAP is not supported for redundancy model %u", sg_redundancy_model);

	return SA_AIS_ERR_NOT_SUPPORTED;
}

// default implementation
void AVD_SG::ng_admin(AVD_SU* su, AVD_AMF_NG* ng) {
	return;
}
/**
 * @brief  Checks if SG is stable with respect to lock-in or unlock-on operation.  
 *
 * @param[in]  sg
 *
 * @return  true/false
 **/
bool sg_stable_after_lock_in_or_unlock_in(AVD_SG *sg)
{
	uint32_t instantiated_sus = 0, to_be_instantiated_sus = 0;
	SaAmfAdminStateT node_admin_state;

	switch (sg->adminOp) {
	case SA_AMF_ADMIN_LOCK_INSTANTIATION :
		for (const auto& su : sg->list_of_su) {
			if ((su->saAmfSUPresenceState != SA_AMF_PRESENCE_UNINSTANTIATED) &&
				(su->saAmfSUPresenceState != SA_AMF_PRESENCE_INSTANTIATION_FAILED) &&
				(su->saAmfSUPresenceState != SA_AMF_PRESENCE_TERMINATION_FAILED)) 
				return false;
		}
		break;
	case SA_AMF_ADMIN_UNLOCK_INSTANTIATION :
		/* Unlock-in of SG will not instantiate any component in NPI SU.*/
		if ((sg->list_of_su.empty() == false) && (sg->first_su()->saAmfSUPreInstantiable == false))
			return true;

		for (const auto& su : sg->list_of_su) {
			node_admin_state = su->su_on_node->saAmfNodeAdminState;
			
			if ((su->saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATION_FAILED) ||
					(su->saAmfSUPresenceState == SA_AMF_PRESENCE_TERMINATION_FAILED))
				continue;

			if (su->saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATED)  {
				instantiated_sus++;
				continue;
			}

			if (node_admin_state == SA_AMF_ADMIN_LOCKED_INSTANTIATION)
				continue;
			
			if ((node_admin_state != SA_AMF_ADMIN_LOCKED_INSTANTIATION) && 
					(su->saAmfSUAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
					(su->saAmfSUOperState == SA_AMF_OPERATIONAL_ENABLED) &&
					(su->su_on_node->node_state == AVD_AVND_STATE_PRESENT)) { 

				if (su->saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATING)
					return false;
				if (su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED) {
					to_be_instantiated_sus++;
					continue;
				}
					
			}
			
		}

		if (instantiated_sus >= sg->saAmfSGNumPrefInserviceSUs)
			return true;
		else  {
			if (to_be_instantiated_sus == 0)
				return true;
			else
				return false;
		}

		break;
	default:
		TRACE("Called for wrong admin operation");
		break;
	}
		
	return true;
}
/*
 * @brief      Checks if SG has assignments only in the nodes of nodegroup.
 * @param[in]  ptr to Nodegroup (AVD_AMF_NG).   
 * @return     true/false.
 */
bool AVD_SG::is_sg_assigned_only_in_ng(const AVD_AMF_NG *ng)
{
	for (const auto& su : list_of_su) {
		if (su->list_of_susi == nullptr)
			continue;
		//Return if this assigned su is not in this ng.
		if (node_in_nodegroup(su->su_on_node->name, ng) == false)
			return false;
	}
	return true;
}
/*
 * @brief      Checks if there exists atleast one instantiable or inservice
 *             SU outside the nodegroup. 
 * @param[in]  ptr to Nodegroup (AVD_AMF_NG).
 * @return     true/false.
 */
bool AVD_SG::is_sg_serviceable_outside_ng(const AVD_AMF_NG *ng)
{
	for (const auto& su : list_of_su) {
		//Check if any su exists outside nodegroup for this sg. 
		if (node_in_nodegroup(su->su_on_node->name, ng) == false) {
			/* 
			   During lock or shutdown operation on Node or SU, AMF
			   instantiates new SUs or it will switch-over existing 
			   assignments of locked Node or SU to other SUs.
			   To avoid service outage of complete SG check if 
			   there exists atleast one instantiable or atleast one 
			   in service SU outside the nodegroup.
			 */
			//TODO_NG:Evaluate if more parameters can be checked.
			if ((su->is_instantiable() == true) || 
					 (su->is_in_service() == true))
				return false;
		}
	}
	return true;
}
/**
 * @brief  Verify if SG is stable for admin operation on entity 
           Node, App and Nodegroup etc.
 * @param  ptr to SG(AVD_SG).
 * @Return SA_AIS_OK/SA_AIS_ERR_TRY_AGAIN/SA_AIS_ERR_BAD_OPERATION.
*/
SaAisErrorT AVD_SG::check_sg_stability()
{
	SaAisErrorT rc = SA_AIS_OK;
	if (sg_fsm_state != AVD_SG_FSM_STABLE) {
		LOG_NO("'%s' is in unstable/transition state", name.c_str());
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
	if ((adminOp_invocationId != 0) || (adminOp != 0))  {
		LOG_NO("Admin operation is already going on '%s' ", name.c_str());
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
        }
	if (sg_is_tolerance_timer_running_for_any_si(this)) {
		LOG_NO("Tolerance timer is running for some of the SI's in the SG '%s',"
				" so differing admin opr", name.c_str());
		rc = SA_AIS_ERR_TRY_AGAIN;
                goto done;
        }
	if (csi_assignment_validate(this) == true) {
                LOG_NO("Single Csi assignment undergoing in '%s'", name.c_str());
		rc = SA_AIS_ERR_TRY_AGAIN;
                goto done;
        }
	if (any_assignment_absent() == true ) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
done:
	return rc;

}

AVD_SU* AVD_SG::first_su()
{
	if (!list_of_su.empty()) {
		return *list_of_su.begin();
	} else {
		return nullptr;
	}
}

AVD_SU* AVD_SG::get_su_by_name(SaNameT su_name)
{
	for (const auto& su : list_of_su) {
		if (Amf::to_string(const_cast<SaNameT*>(&su_name)) == su->name) {
			return su;
		}
	}
	return nullptr;
}

uint32_t AVD_SG::curr_assigned_sus() const
{
	return (std::count_if (list_of_su.cbegin(), list_of_su.cend(),
                [](AVD_SU *su) -> bool { return (su->list_of_susi != nullptr);}));
}
uint32_t AVD_SG::curr_instantiated_spare_sus() const
{
	return (std::count_if (list_of_su.cbegin(), list_of_su.cend(),
                [](AVD_SU *su) -> bool { return ((su->list_of_susi == nullptr) &&
                        (su->saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATED));}));
}
uint32_t AVD_SG::curr_non_instantiated_spare_sus() const
{
	return (std::count_if (list_of_su.cbegin(), list_of_su.cend(),
		[](AVD_SU *su) -> bool { return ((su->list_of_susi == nullptr) &&
			(su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED));}));	
}

void avd_sg_read_headless_fsm_state_cached_rta(AVD_CL_CB *cb)
{

	SaAisErrorT rc;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;

	SaNameT sg_dn;
	AVD_SG *sg;
	const SaImmAttrValuesT_2 **attributes;
	AVD_SG_FSM_STATE imm_sg_fsm_state;
	const char *className = "SaAmfSG";
	const SaImmAttrNameT searchAttributes[] = {
		const_cast<SaImmAttrNameT>("osafAmfSGFsmState"),
		NULL
	};

	TRACE_ENTER();

	osafassert(cb->scs_absence_max_duration > 0);

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if ((rc = immutil_saImmOmSearchInitialize_2(cb->immOmHandle, NULL, SA_IMM_SUBTREE,
	      SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR, &searchParam,
		  searchAttributes, &searchHandle)) != SA_AIS_OK) {

		LOG_ER("%s: saImmOmSearchInitialize_2 failed: %u", __FUNCTION__, rc);
		goto done;
	}

	while ((rc = immutil_saImmOmSearchNext_2(searchHandle, &sg_dn,
					(SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK) {
		sg = sg_db->find(Amf::to_string(&sg_dn));
		if (sg && sg->sg_ncs_spec == false) {
			if (sg->headless_validation == false) {
				continue;
			}
			if (sg->any_assignment_in_progress() == false &&
				sg->any_assignment_assigned() == false) {
				continue;
			}
			// Read sg fsm state
			rc = immutil_getAttr(const_cast<SaImmAttrNameT>("osafAmfSGFsmState"),
					attributes, 0, &imm_sg_fsm_state);
			osafassert(rc == SA_AIS_OK);
			sg->set_fsm_state(imm_sg_fsm_state, false);
		}
	}

	(void)immutil_saImmOmSearchFinalize(searchHandle);

done:
	TRACE_LEAVE();

}

void avd_sg_read_headless_su_oper_list_cached_rta(AVD_CL_CB *cb)
{

	SaAisErrorT rc;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;

	SaNameT sg_dn;
	AVD_SG *sg;
	unsigned int num_of_values = 0;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfSG";
	const SaImmAttrNameT searchAttributes[] = {
		const_cast<SaImmAttrNameT>("osafAmfSGSuOperationList"),
		NULL
	};

	TRACE_ENTER();

	osafassert(cb->scs_absence_max_duration > 0);

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if ((rc = immutil_saImmOmSearchInitialize_2(cb->immOmHandle, NULL, SA_IMM_SUBTREE,
	      SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR, &searchParam,
		  searchAttributes, &searchHandle)) != SA_AIS_OK) {

		LOG_ER("%s: saImmOmSearchInitialize_2 failed: %u", __FUNCTION__, rc);
		goto done;
	}

	while ((rc = immutil_saImmOmSearchNext_2(searchHandle, &sg_dn,
					(SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK) {
		sg = sg_db->find(Amf::to_string(&sg_dn));
		if (sg && sg->sg_ncs_spec == false) {
			// Read sg operation list
			if (immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("osafAmfSGSuOperationList"), attributes, &num_of_values) == SA_AIS_OK) {
				unsigned int i;
				for (i = 0; i < num_of_values; i++) {
					const SaNameT *su_name = immutil_getNameAttr(attributes, "osafAmfSGSuOperationList", i);
					AVD_SU* op_su = sg->get_su_by_name(*su_name);
					if (op_su) {
						if (op_su->sg_of_su->any_assignment_in_progress()) {
							avd_sg_su_oper_list_add(avd_cb, op_su, false, false);
						}
					}
				}
			}

			// Move fsm state of absent SUSI to AVD_SU_SI_STATE_ABSENT
			for (const auto& su : sg->list_of_su) {
				for (AVD_SU_SI_REL *susi = su->list_of_susi; susi;
						susi = susi->su_next) {
					TRACE("SUSI:'%s,%s', fsm:'%d'", susi->su->name.c_str(),
							susi->si->name.c_str(), susi->fsm);
					if (susi->absent == true) {
						avd_susi_update_fsm(susi, AVD_SU_SI_STATE_ABSENT);
						//Checkpoint to add this SUSI only after absent SUSI fsm
						//moves to AVD_SU_SI_STATE_ABSENT, so var @absent is safe
						//at standby side
						m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(cb, susi, AVSV_CKPT_AVD_SI_ASS);
						susi->absent = false;
					}
				}
			}
		}
	}

	(void)immutil_saImmOmSearchFinalize(searchHandle);

done:
	TRACE_LEAVE();

}

/**
 * @brief  Validate all cached RTAs read from IMM after headless.
           This validation is necessary. If AMFD doesn't have this
           validation routine and the cached RTAs are invalid,
           that would lead into *unpredictably* wrong states, which
           is hard to debug (harder if no trace)
 * @param  Control block (AVD_CL_CB).
 * @Return true if valid, false otherwise.
*/
bool avd_sg_validate_headless_cached_rta(AVD_CL_CB *cb) {
	TRACE_ENTER();
	bool valid = true;
	for (std::map<std::string, AVD_SG*>::const_iterator it = sg_db->begin();
			it != sg_db->end(); it++) {
		AVD_SG *i_sg = it->second;
		if (i_sg->sg_ncs_spec == true) {
			continue;
		}

		if (i_sg->headless_validation == false) {
			//TODO: AMFD should make all SUs of this SG faulty to remove
			//all assignments, clean up IMM headless cached RTA.
			//Just assert for now
			//osafassert(false);
			valid = false;
		}
	}
	TRACE_LEAVE2("%u", valid);
	return valid;
}

void AVD_SG::failover_absent_assignment() {

	TRACE_ENTER2("SG:'%s'", name.c_str());
	for (const auto& su : list_of_su) {
		if (su->any_susi_fsm_in(AVD_SU_SI_STATE_ABSENT)){
			node_fail(avd_cb, su);
			if (su->is_in_service())
				su->set_readiness_state(SA_AMF_READINESS_IN_SERVICE);
			if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE)
				su->sg_of_su->realign(avd_cb, this);
			break;
		}
	}
	TRACE_LEAVE();
}

bool AVD_SG::any_assignment_absent() {
	bool pending = false;
	TRACE_ENTER2("SG:'%s'", name.c_str());
	for (const auto& su : list_of_su) {
		if (su->any_susi_fsm_in(AVD_SU_SI_STATE_ABSENT)){
			pending = true;
			break;
		}
	}
	TRACE_LEAVE();
	return pending;
}

bool AVD_SG::any_assignment_in_progress() {
	bool pending = false;
	TRACE_ENTER2("SG:'%s'", name.c_str());
	for (const auto& su : list_of_su) {
		if (su->any_susi_fsm_in(AVD_SU_SI_STATE_ASGN) ||
			su->any_susi_fsm_in(AVD_SU_SI_STATE_UNASGN) ||
			su->any_susi_fsm_in(AVD_SU_SI_STATE_MODIFY)){
			pending = true;
			break;
		}
	}
	TRACE_LEAVE();
	return pending;
}

bool AVD_SG::any_assignment_assigned() {
	bool pending = false;
	TRACE_ENTER2("SG:'%s'", name.c_str());
	for (const auto& su : list_of_su) {
		if (su->any_susi_fsm_in(AVD_SU_SI_STATE_ASGND)){
			pending = true;
			break;
		}
	}
	TRACE_LEAVE();
	return pending;
}


