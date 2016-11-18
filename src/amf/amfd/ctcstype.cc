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

#include "imm/saf/saImmOm.h"
#include "osaf/immutil/immutil.h"
#include "base/logtrace.h"

#include "amf/amfd/util.h"
#include "amf/amfd/comp.h"
#include "amf/amfd/imm.h"
#include "amf/amfd/csi.h"

AmfDb<std::string, AVD_CTCS_TYPE> *ctcstype_db = nullptr;

static void ctcstype_db_add(AVD_CTCS_TYPE *ctcstype)
{
	unsigned int rc = ctcstype_db->insert(ctcstype->name,ctcstype);
	osafassert(rc == NCSCC_RC_SUCCESS);
}

static int is_config_valid(const std::string& dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
	SaUint32T uint32;
	std::string::size_type parent;
	std::string cstype_dn;

	if (get_child_dn_from_ass_dn(dn, cstype_dn) != 0) {
		report_ccb_validation_error(opdata, "malformed DN '%s'", dn.c_str());
		return 0;
	}

	if (cstype_db->find(cstype_dn) == nullptr) {
		if (opdata == nullptr) {
			report_ccb_validation_error(opdata,
				"SaAmfCSType object '%s' does not exist in model",
				cstype_dn.c_str());
			return 0;
		}

		const SaNameTWrapper cstype(cstype_dn);
		if (ccbutil_getCcbOpDataByDN(opdata->ccbId, cstype) == nullptr) {
			report_ccb_validation_error(opdata,
				"SaAmfCSType object '%s' does not exist in model or CCB",
				cstype_dn.c_str());
			return 0;
		}
	}

	/* Second comma should be parent */
	if ((parent = dn.find(',')) == std::string::npos) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn.c_str());
		return 0;
	}

	if ((parent = dn.find(',', parent + 1)) == std::string::npos) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn.c_str());
		return 0;
	}

	/* Should be children to SaAmfCompType */
	if (dn.compare(parent + 1, 11, "safVersion=") != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s'", dn.c_str());
		return 0;
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtCompCapability"), attributes, 0, &uint32) == SA_AIS_OK) &&
	    (uint32 > SA_AMF_COMP_NON_PRE_INSTANTIABLE)) {
		report_ccb_validation_error(opdata, "Invalid saAmfCtCompCapability %u for '%s'", uint32, dn.c_str());
		return 0;
	}

	return 1;
}

//
AVD_CTCS_TYPE::AVD_CTCS_TYPE(const std::string& dn) :
	name(dn)
{
}

//
static AVD_CTCS_TYPE *ctcstype_create(const std::string& dn, const SaImmAttrValuesT_2 **attributes)
{
	AVD_CTCS_TYPE *ctcstype;
	int rc = -1;
	SaAisErrorT error;

	TRACE_ENTER2("'%s'", dn.c_str());

	ctcstype = new AVD_CTCS_TYPE(dn);

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtCompCapability"), attributes, 0, &ctcstype->saAmfCtCompCapability);
	osafassert(error == SA_AIS_OK);

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefNumMaxActiveCSIs"), attributes, 0, &ctcstype->saAmfCtDefNumMaxActiveCSIs) !=
	    SA_AIS_OK)
		ctcstype->saAmfCtDefNumMaxActiveCSIs = -1;	/* no limit */

	    if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCtDefNumMaxStandbyCSIs"), attributes, 0, &ctcstype->saAmfCtDefNumMaxStandbyCSIs) !=
	    SA_AIS_OK)
		ctcstype->saAmfCtDefNumMaxStandbyCSIs = -1;	/* no limit */

	rc = 0;

	if (rc != 0) {
		delete ctcstype;
		ctcstype = nullptr;
	}
	TRACE_LEAVE();
	return ctcstype;
}

SaAisErrorT avd_ctcstype_config_get(const std::string& comp_type_dn, AVD_COMP_TYPE *comp_type)
{
	SaAisErrorT error = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfCtCsType";
	AVD_CTCS_TYPE *ctcstype;

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if (immutil_saImmOmSearchInitialize_o2(avd_cb->immOmHandle, comp_type_dn.c_str(),
		SA_IMM_SUBTREE, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR,
		&searchParam, nullptr, &searchHandle) != SA_AIS_OK) {

		LOG_ER("saImmOmSearchInitialize_2 failed: %u", error);
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		if (!is_config_valid(Amf::to_string(&dn), attributes, nullptr))
			goto done2;

		if ((ctcstype = ctcstype_db->find(Amf::to_string(&dn))) == nullptr ) {
			if ((ctcstype = ctcstype_create(Amf::to_string(&dn), attributes)) == nullptr)
				goto done2;

			ctcstype_db_add(ctcstype);
		}
	}

	error = SA_AIS_OK;

done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

static SaAisErrorT ctcstype_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		report_ccb_validation_error(opdata, "Modification of SaAmfCtCsType not supported");
		break;
	case CCBUTIL_DELETE:
		rc = SA_AIS_OK;
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

static void ctcstype_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_CTCS_TYPE *ctcstype;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		ctcstype = ctcstype_create(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues);
		osafassert(ctcstype);
		ctcstype_db_add(ctcstype);
		break;
	case CCBUTIL_DELETE:
		ctcstype = ctcstype_db->find(Amf::to_string(&opdata->objectName));
		if (ctcstype != nullptr) {
			ctcstype_db->erase(ctcstype->name);
			delete ctcstype;
		}
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE();
}

AVD_CTCS_TYPE *get_ctcstype(const std::string& comptype_name, const std::string& cstype_name)
{
	SaNameT dn;
	const SaNameTWrapper comptype(comptype_name);
	const SaNameTWrapper cstype(cstype_name);
	AVD_CTCS_TYPE *ctcs_type = nullptr;
	avsv_create_association_class_dn(cstype, comptype,
			"safSupportedCsType", &dn);
	ctcs_type = ctcstype_db->find(Amf::to_string(&dn));
	osaf_extended_name_free(&dn);
	return ctcs_type;
}

void avd_ctcstype_constructor(void)
{
	ctcstype_db = new AmfDb<std::string, AVD_CTCS_TYPE>;

	avd_class_impl_set("SaAmfCtCsType", nullptr, nullptr,
		ctcstype_ccb_completed_cb, ctcstype_ccb_apply_cb);
}

