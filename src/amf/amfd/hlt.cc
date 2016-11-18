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

#include <stddef.h>
#include "amf/amfd/util.h"

#include "base/logtrace.h"
#include "osaf/immutil/immutil.h"

#include "amf/amf_util.h"
#include "amf/amfd/imm.h"
#include "amf/amfd/csi.h"
#include "amf/amfd/hlt.h"
#include "amf/amfd/comp.h"
#include "amf/amfd/util.h"

static SaAisErrorT ccb_completed_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != nullptr) {
		const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;
		SaTimeT *value = (SaTimeT *)attribute->attrValues[0];

		if (!strcmp(attribute->attrName, "saAmfHealthcheckPeriod")) {
			if (*value < (100 * SA_TIME_ONE_MILLISECOND)) {
				report_ccb_validation_error(opdata, "Unreasonable saAmfHealthcheckPeriod %llu for '%s' ",
						*value, osaf_extended_name_borrow(&opdata->objectName));
				return SA_AIS_ERR_BAD_OPERATION;
			}
		} else if (!strcmp(attribute->attrName, "saAmfHealthcheckMaxDuration")) {
			if (*value < (100 * SA_TIME_ONE_MILLISECOND)) {
				report_ccb_validation_error(opdata, "Unreasonable saAmfHealthcheckMaxDuration %llu for '%s'",
						*value, osaf_extended_name_borrow(&opdata->objectName));
				return SA_AIS_ERR_BAD_OPERATION;
			}
		} else {
			report_ccb_validation_error(opdata,
				"Unknown attribute '%s'",
				attribute->attrName);
			return SA_AIS_ERR_BAD_OPERATION;
		}
	}

	return SA_AIS_OK;
}

static SaAisErrorT ccb_completed_delete_hdlr(CcbUtilOperationData_t *opdata)
{
	AVD_COMP *comp;
	std::string comp_name;
	AVD_SU_SI_REL *curr_susi;
	AVD_COMP_CSI_REL *compcsi;
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER();
	avsv_sanamet_init(Amf::to_string(&opdata->objectName), comp_name, "safComp=");

	comp = comp_db->find(comp_name);
	for (curr_susi = comp->su->list_of_susi; curr_susi != nullptr; curr_susi = curr_susi->su_next)
		for (compcsi = curr_susi->list_of_csicomp; compcsi; compcsi = compcsi->susi_csicomp_next) {
			if (compcsi->comp == comp) {
				report_ccb_validation_error(opdata, "Deletion of SaAmfHealthcheck requires"
						" comp without any csi assignment; Ideally su should be locked-in");
				goto done;
			}
		}

	opdata->userData = comp->su->su_on_node;
	rc = SA_AIS_OK;

done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT hc_ccb_completed_cb(CcbUtilOperationData_t *opdata)
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
		rc = ccb_completed_delete_hdlr(opdata);
		break;
	default:
		osafassert(0);
		break;
	}

	return rc;
}

static void ccb_apply_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;
	const AVD_COMP *comp;
	std::string comp_name;

	const std::string object_name(Amf::to_string(&opdata->objectName));
	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, object_name.c_str());

	comp_name = object_name.substr(object_name.find("safComp"));
	osafassert(comp_name.empty() == false);
	comp = comp_db->find(comp_name);
	osafassert(comp);

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != nullptr) {
		AVSV_PARAM_INFO param;
		const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;
		SaTimeT *param_val = (SaTimeT *)attribute->attrValues[0];

		memset(&param, 0, sizeof(param));
		param.class_id = AVSV_SA_AMF_HEALTH_CHECK;
		param.act = AVSV_OBJ_OPR_MOD;
		param.name = opdata->objectName;
		param.value_len = sizeof(*param_val);
		memcpy(param.value, param_val, param.value_len);

		if (!strcmp(attribute->attrName, "saAmfHealthcheckPeriod")) {
			TRACE("saAmfHealthcheckPeriod modified to '%llu' for Comp'%s'", *param_val, 
					osaf_extended_name_borrow(&comp->comp_info.name));
			param.attr_id = saAmfHealthcheckPeriod_ID;
		} else if (!strcmp(attribute->attrName, "saAmfHealthcheckMaxDuration")) {
			TRACE("saAmfHealthcheckMaxDuration modified to '%llu' for Comp'%s'", *param_val, 
					osaf_extended_name_borrow(&comp->comp_info.name));
			param.attr_id = saAmfHealthcheckMaxDuration_ID;
		} else {
			osafassert(0);
		}

		avd_snd_op_req_msg(avd_cb, comp->su->su_on_node, &param);
	}
}

static void ccb_apply_delete_hdlr(CcbUtilOperationData_t *opdata)
{
	AVSV_PARAM_INFO param = {0};
	AVD_AVND *node = static_cast<AVD_AVND*>(opdata->userData);

	param.class_id = AVSV_SA_AMF_HEALTH_CHECK;
	param.act = AVSV_OBJ_OPR_DEL;
	param.name = opdata->objectName;

	avd_snd_op_req_msg(avd_cb, node, &param);
}

static void hc_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		break;
	case CCBUTIL_DELETE:
		ccb_apply_delete_hdlr(opdata);
		break;
	case CCBUTIL_MODIFY:
		ccb_apply_modify_hdlr(opdata);
		break;
	default:
		osafassert(0);
		break;
	}
}

void avd_hc_constructor(void)
{
	avd_class_impl_set("SaAmfHealthcheck", nullptr, nullptr, hc_ccb_completed_cb,
		hc_ccb_apply_cb);
}

