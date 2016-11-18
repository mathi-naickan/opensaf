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
#include "amf/amfd/sgtype.h"
#include "amf/amfd/proc.h"
#include <algorithm>

AmfDb<std::string, AVD_AMF_SG_TYPE> *sgtype_db = nullptr;

void avd_sgtype_add_sg(AVD_SG *sg)
{
	sg->sg_type->list_of_sg.push_back(sg);
}

void avd_sgtype_remove_sg(AVD_SG *sg) {
  AVD_AMF_SG_TYPE *sg_type = sg->sg_type;

  if (sg_type != nullptr) {
    auto pos = std::find(sg_type->list_of_sg.begin(), sg_type->list_of_sg.end(), sg);
    if(pos != sg_type->list_of_sg.end()) {
      sg_type->list_of_sg.erase(pos);
    } else {
      /* Log a fatal error */
      osafassert(0);
    }
  }
}

static void sgtype_delete(AVD_AMF_SG_TYPE *sg_type)
{
	sgtype_db->erase(sg_type->name);
	delete sg_type;
}

/**
 * Add the SG type to the DB
 * @param sgt
 */
static void sgtype_add_to_model(AVD_AMF_SG_TYPE *sgt)
{
	unsigned int rc = sgtype_db->insert(sgt->name,sgt);
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
static int is_config_valid(const std::string& dn, const SaImmAttrValuesT_2 **attributes,
	const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;
	int i = 0;
	unsigned j = 0;
	std::string::size_type pos;
	SaUint32T value;
	SaBoolT abool;
	AVD_SUTYPE *sut;
	const SaImmAttrValuesT_2 *attr;

	if ((pos = dn.find(',')) == std::string::npos) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn.c_str());
		return 0;
	}

	/* SGType should be children to SGBaseType */
	if (dn.compare(pos + 1, 10, "safSgType=") != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", dn.substr(pos +1).c_str(), dn.c_str());
		return 0;
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSgtRedundancyModel"), attributes, 0, &value);
	osafassert(rc == SA_AIS_OK);

	if ((value < SA_AMF_2N_REDUNDANCY_MODEL) || (value > SA_AMF_NO_REDUNDANCY_MODEL)) {
		report_ccb_validation_error(opdata, "Invalid saAmfSgtRedundancyModel %u for '%s'",
				value, dn.c_str());
		return 0;
	}

	while ((attr = attributes[i++]) != nullptr)
		if (!strcmp(attr->attrName, "saAmfSgtValidSuTypes"))
			break;

	osafassert(attr);
	osafassert(attr->attrValuesNumber > 0);

	for (j = 0; j < attr->attrValuesNumber; j++) {
		SaNameT *name = (SaNameT *)attr->attrValues[j];
		sut = sutype_db->find(Amf::to_string(name));
		if (sut == nullptr) {
			if (opdata == nullptr) {
				report_ccb_validation_error(opdata, "'%s' does not exist in model", osaf_extended_name_borrow(name));
				return 0;
			}
			
			/* SG type does not exist in current model, check CCB */
			if (ccbutil_getCcbOpDataByDN(opdata->ccbId, name) == nullptr) {
				report_ccb_validation_error(opdata, "'%s' does not exist either in model or CCB",
						osaf_extended_name_borrow(name));
				return 0;
			}
		}
	}

	if (j == 0) {
		report_ccb_validation_error(opdata, "No SU types configured for '%s'", dn.c_str());
		return 0;
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSgtDefAutoRepair"), attributes, 0, &abool) == SA_AIS_OK) &&
	    (abool > SA_TRUE)) {
		report_ccb_validation_error(opdata, "Invalid saAmfSgtDefAutoRepair %u for '%s'", abool, dn.c_str());
		return 0;
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSgtDefAutoAdjust"), attributes, 0, &abool) == SA_AIS_OK) &&
	    (abool > SA_TRUE)) {
		report_ccb_validation_error(opdata, "Invalid saAmfSgtDefAutoAdjust %u for '%s'", abool, dn.c_str());
		return 0;
	}

	return 1;
}

//
AVD_AMF_SG_TYPE::AVD_AMF_SG_TYPE(const std::string& dn) :
	name(dn)
{
}

/**
 * Create a new SG type object and initialize its attributes from the attribute list.
 * @param dn
 * @param attributes
 * 
 * @return AVD_AMF_SG_TYPE*
 */
static AVD_AMF_SG_TYPE *sgtype_create(const std::string& dn, const SaImmAttrValuesT_2 **attributes)
{
	int i = 0, rc = -1;
	unsigned j = 0;
	AVD_AMF_SG_TYPE *sgt;
	const SaImmAttrValuesT_2 *attr;
	SaAisErrorT error;

	TRACE_ENTER2("'%s'", dn.c_str());

	sgt = new AVD_AMF_SG_TYPE(dn);

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSgtRedundancyModel"), attributes, 0, &sgt->saAmfSgtRedundancyModel);
	osafassert(error == SA_AIS_OK);

	while ((attr = attributes[i++]) != nullptr)
		if (!strcmp(attr->attrName, "saAmfSgtValidSuTypes"))
			break;

	osafassert(attr);
	osafassert(attr->attrValuesNumber > 0);

	sgt->number_su_type = attr->attrValuesNumber;
	for (j = 0; j < attr->attrValuesNumber; j++) {
		sgt->saAmfSGtValidSuTypes.push_back(
			Amf::to_string(reinterpret_cast<SaNameT*>(attr->attrValues[j]))
		);
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSgtDefAutoRepair"), attributes, 0, &sgt->saAmfSgtDefAutoRepair) != SA_AIS_OK) {
		sgt->saAmfSgtDefAutoRepair = SA_TRUE;
		sgt->saAmfSgtDefAutoRepair_configured = false;
	}
	else
		sgt->saAmfSgtDefAutoRepair_configured = true;

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSgtDefAutoAdjust"), attributes, 0, &sgt->saAmfSgtDefAutoAdjust) != SA_AIS_OK) {
		sgt->saAmfSgtDefAutoAdjust = SA_FALSE;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSgtDefAutoAdjustProb"), attributes, 0, &sgt->saAmfSgtDefAutoAdjustProb) != SA_AIS_OK) {
		LOG_ER("Get saAmfSgtDefAutoAdjustProb FAILED for '%s'", dn.c_str());
		goto done;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSgtDefCompRestartProb"), attributes, 0, &sgt->saAmfSgtDefCompRestartProb) != SA_AIS_OK) {
		LOG_ER("Get saAmfSgtDefCompRestartProb FAILED for '%s'", dn.c_str());
		goto done;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSgtDefCompRestartMax"), attributes, 0, &sgt->saAmfSgtDefCompRestartMax) != SA_AIS_OK) {
		LOG_ER("Get saAmfSgtDefCompRestartMax FAILED for '%s'", dn.c_str());
		goto done;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSgtDefSuRestartProb"), attributes, 0, &sgt->saAmfSgtDefSuRestartProb) != SA_AIS_OK) {
		LOG_ER("Get saAmfSgtDefSuRestartProb FAILED for '%s'", dn.c_str());
		goto done;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSgtDefSuRestartMax"), attributes, 0, &sgt->saAmfSgtDefSuRestartMax) != SA_AIS_OK) {
		LOG_ER("Get saAmfSgtDefSuRestartMax FAILED for '%s'", dn.c_str());
		goto done;
	}

	rc = 0;

 done:
	if (rc != 0) {
		delete sgt;
		sgt = nullptr;
	}

	TRACE_LEAVE();
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

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, nullptr, SA_IMM_SUBTREE,
						  SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
						  nullptr, &searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("saImmOmSearchInitialize_2 failed: %u", error);
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		if (!is_config_valid(Amf::to_string(&dn), attributes, nullptr))
			goto done2;
		if (( sgt = sgtype_db->find(Amf::to_string(&dn))) == nullptr) {
			if ((sgt = sgtype_create(Amf::to_string(&dn), attributes)) == nullptr)
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
 * This routine validates modify CCB operations on SaAmfSGType objects.
 * @param   Ccb Util Oper Data
 * @return 
 */

static SaAisErrorT sgtype_ccb_completed_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_OK;
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;
	AVD_AMF_SG_TYPE *sgt = sgtype_db->find(Amf::to_string(&opdata->objectName));

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));
	while ((attr_mod = opdata->param.modify.attrMods[i++]) != nullptr) {
		
		if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || (attr_mod->modAttr.attrValues == nullptr))
			continue;

		if (!strcmp(attr_mod->modAttr.attrName, "saAmfSgtDefAutoRepair")) {
			uint32_t sgt_autorepair = *((SaUint32T *)attr_mod->modAttr.attrValues[0]);
			if (sgt_autorepair > SA_TRUE) {
				report_ccb_validation_error(opdata, "invalid saAmfSgtDefAutoRepair in:'%s'",
						sgt->name.c_str());
				rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
			}	
		} else {
			report_ccb_validation_error(opdata, "Modification of attribute is Not Supported:'%s' ",
					attr_mod->modAttr.attrName);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}
	}

done:
	return rc;
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

	bool sg_exist = false;
	CcbUtilOperationData_t *t_opData;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		rc = sgtype_ccb_completed_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		sgt = sgtype_db->find(Amf::to_string(&opdata->objectName));

		for (const auto& sg : sgt->list_of_sg) {
			SaNameTWrapper sg_name(sg->name);
			t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, sg_name);
			if ((t_opData == nullptr) || (t_opData->operationType != CCBUTIL_DELETE)) {
				sg_exist = true;
				break;
			}
		}

		if (sg_exist == true) {
			report_ccb_validation_error(opdata, "SGs exist of this SG type '%s'",sgt->name.c_str());
				goto done;
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
 * This function handles modify operations on SaAmfSGType objects. 
 * @param ptr to opdata 
 */
static void sgtype_ccb_apply_modify_hdlr(struct CcbUtilOperationData *opdata)
{
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	AVD_AMF_SG_TYPE *sgt = sgtype_db->find(Amf::to_string(&opdata->objectName));

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != nullptr) {
		bool value_is_deleted; 
		if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) ||	(attr_mod->modAttr.attrValues == nullptr))
			/* Attribute value is deleted, revert to default value */
			value_is_deleted = true;
		else 
			/* Attribute value is modified */
			value_is_deleted = false;
		
		if (!strcmp(attr_mod->modAttr.attrName, "saAmfSgtDefAutoRepair")) {
			SaBoolT old_value = sgt->saAmfSgtDefAutoRepair;
			if (value_is_deleted) {
				sgt->saAmfSgtDefAutoRepair = static_cast<SaBoolT>(true);
				sgt->saAmfSgtDefAutoRepair_configured = false;
			} else {
				sgt->saAmfSgtDefAutoRepair = static_cast<SaBoolT>(*((SaUint32T *)attr_mod->modAttr.attrValues[0]));
				sgt->saAmfSgtDefAutoRepair_configured = true; 
			}
			TRACE("Modified saAmfSgtDefAutoRepair is '%u'", sgt->saAmfSgtDefAutoRepair);
			amflog(LOG_NOTICE, "%s saAmfSgtDefAutoRepair changed to %u",
				sgt->name.c_str(), sgt->saAmfSgtDefAutoRepair);

			/* Modify saAmfSGAutoRepair for SGs which had inherited saAmfSgtDefAutoRepair.*/
			if (old_value != sgt->saAmfSgtDefAutoRepair) {

				for (const auto& sg : sgt->list_of_sg) {
					if (!sg->saAmfSGAutoRepair_configured) {
						sg->saAmfSGAutoRepair = static_cast<SaBoolT>(sgt->saAmfSgtDefAutoRepair);
						TRACE("Modified saAmfSGAutoRepair is '%u'", sg->saAmfSGAutoRepair);
						amflog(LOG_NOTICE, "%s inherited saAmfSGAutoRepair value changed to %u",
							sg->name.c_str(), sg->saAmfSGAutoRepair);
					}
				}
			}
		} 
	}

	TRACE_LEAVE();
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

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		sgt = sgtype_create(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues);
		osafassert(sgt);
		sgtype_add_to_model(sgt);
		break;
	case CCBUTIL_MODIFY:
		sgtype_ccb_apply_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		sgtype_delete(static_cast<AVD_AMF_SG_TYPE*>(opdata->userData));
		break;
	default:
		osafassert(0);
	}

	TRACE_LEAVE();
}

void avd_sgtype_constructor(void)
{

	sgtype_db = new AmfDb<std::string, AVD_AMF_SG_TYPE>;
	avd_class_impl_set("SaAmfSGType", nullptr, nullptr, sgtype_ccb_completed_cb,
		sgtype_ccb_apply_cb);
	avd_class_impl_set("SaAmfSGBaseType", nullptr, nullptr,
		avd_imm_default_OK_completed_cb, nullptr);
}
