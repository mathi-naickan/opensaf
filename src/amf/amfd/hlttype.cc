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
 *
 */

#include "amf/amfd/util.h"
#include <set>
#include "amf/amfd/node.h"
#include "base/logtrace.h"
#include "osaf/immutil/immutil.h"
#include "base/ncsgl_defs.h"
#include "amf/amfd/imm.h"
#include "amf/amfd/comp.h"

/**
 * Validates proposed change in comptype
 */
static SaAisErrorT ccb_completed_modify_hdlr(const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_OK;
	const SaImmAttrModificationT_2 *mod;
	int i = 0;
	const std::string dn(Amf::to_string(&opdata->objectName));

	while ((mod = opdata->param.modify.attrMods[i++]) != nullptr) {
		if (strcmp(mod->modAttr.attrName, "saAmfHctDefPeriod") == 0) {
			SaTimeT value = *((SaTimeT *)mod->modAttr.attrValues[0]);
			if (value < SA_TIME_ONE_SECOND) {
				report_ccb_validation_error(opdata,
					"Invalid saAmfHctDefPeriod for '%s'", dn.c_str());
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else if (strcmp(mod->modAttr.attrName, "saAmfHctDefMaxDuration") == 0) {
			SaTimeT value = *((SaTimeT *)mod->modAttr.attrValues[0]);
			if (value < 100 * SA_TIME_ONE_MILLISECOND) {
				report_ccb_validation_error(opdata,
					"Invalid saAmfHctDefMaxDuration for '%s'", dn.c_str());
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		}
	}

done:
	return rc;
}

static void ccb_apply_modify_hdlr(const CcbUtilOperationData_t *opdata)
{
	const SaImmAttrModificationT_2 *attr_mod;
	int i;
	const AVD_COMP_TYPE *comp_type;
	std::string comp_type_name;
	const std::string object_name(Amf::to_string(&opdata->objectName));

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, object_name.c_str());

	// input example: opdata.objectName.value, safHealthcheckKey=AmfDemo,safVersion=1,safCompType=AmfDemo1
	avsv_sanamet_init(object_name, comp_type_name, "safVersion=");

	if ((comp_type = comptype_db->find(comp_type_name)) == 0) {
		LOG_ER("Internal error: %s not found", comp_type_name.c_str());
		return;
	}

	// Create a set of nodes where components "may" be using the given SaAmfHealthcheckType. 
	// A msg will be sent to the related node regarding this change. If a component has an 
	// SaAmfHealthcheck record that overrides this SaAmfHealthcheckType it will be handled by the amfnd.
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
			const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;
			SaTimeT *param_val = (SaTimeT *)attribute->attrValues[0];

			memset(&param, 0, sizeof(param));
			param.class_id = AVSV_SA_AMF_HEALTH_CHECK_TYPE;
			param.act = AVSV_OBJ_OPR_MOD;
			param.name = opdata->objectName;
			param.value_len = sizeof(*param_val);
			memcpy(param.value, param_val, param.value_len);

			if (!strcmp(attribute->attrName, "saAmfHctDefPeriod")) {
				TRACE("saAmfHctDefPeriod modified to '%llu' for CompType '%s' on node '%s'", *param_val, 
				      object_name.c_str(), (*it)->name.c_str());
				param.attr_id = saAmfHctDefPeriod_ID;
			} else if (!strcmp(attribute->attrName, "saAmfHctDefMaxDuration")) {
				TRACE("saAmfHctDefMaxDuration modified to '%llu' for CompType '%s' on node '%s", *param_val, 
				      object_name.c_str(), (*it)->name.c_str());
				param.attr_id = saAmfHctDefMaxDuration_ID;
			} else
				LOG_WA("Unexpected attribute name: %s", attribute->attrName);

			avd_snd_op_req_msg(avd_cb, *it, &param);
		}
	}	
}

static SaAisErrorT hct_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
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

	return rc;
}

static void hct_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		break;
	case CCBUTIL_DELETE:
		break;
	case CCBUTIL_MODIFY:
		ccb_apply_modify_hdlr(opdata);
		break;
	default:
		osafassert(0);
		break;
	}
}

void avd_hctype_constructor(void)
{
	avd_class_impl_set("SaAmfHealthcheckType", nullptr, nullptr,
		hct_ccb_completed_cb, hct_ccb_apply_cb);
}

