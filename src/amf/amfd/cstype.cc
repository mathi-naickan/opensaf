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

#include "base/logtrace.h"

#include "amf/amfd/util.h"
#include "amf/amfd/csi.h"
#include "amf/amfd/imm.h"

AmfDb<std::string, AVD_CS_TYPE> *cstype_db = nullptr;

static void cstype_add_to_model(AVD_CS_TYPE *cst)
{
	uint32_t rc = cstype_db->insert(cst->name,cst);
	osafassert(rc == NCSCC_RC_SUCCESS);
}

//
AVD_CS_TYPE::AVD_CS_TYPE(const std::string& dn) :
	name(dn)
{
}

static AVD_CS_TYPE *cstype_create(const std::string& dn, const SaImmAttrValuesT_2 **attributes)
{
	AVD_CS_TYPE *cst;
	SaUint32T values_number;

	TRACE_ENTER2("'%s'", dn.c_str());

	cst = new AVD_CS_TYPE(dn);

	if ((immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("saAmfCSAttrName"), attributes, &values_number) == SA_AIS_OK) &&
	    (values_number > 0)) {
		unsigned int i;

		for (i = 0; i < values_number; i++) {
			cst->saAmfCSAttrName.push_back(immutil_getStringAttr(attributes, "saAmfCSAttrName", i));
		}
	}

	TRACE_LEAVE();
	return cst;
}

/**
 * Delete from DB and return memory
 * @param cst
 */
static void cstype_delete(AVD_CS_TYPE *cst)
{
	cstype_db->erase(cst->name);
	cst->saAmfCSAttrName.clear();
	delete cst;
}

void avd_cstype_add_csi(AVD_CSI *csi)
{
	TRACE_ENTER();
	csi->csi_list_cs_type_next = csi->cstype->list_of_csi;
	csi->cstype->list_of_csi = csi;
	TRACE_LEAVE();
}

void avd_cstype_remove_csi(AVD_CSI *csi)
{
	AVD_CSI *i_csi;
	AVD_CSI *prev_csi = nullptr;

	if (csi->cstype != nullptr) {
		i_csi = csi->cstype->list_of_csi;

		while ((i_csi != nullptr) && (i_csi != csi)) {
			prev_csi = i_csi;
			i_csi = i_csi->csi_list_cs_type_next;
		}

		if (i_csi != csi) {
			/* Log a fatal error */
			osafassert(0);
		} else {
			if (prev_csi == nullptr) {
				csi->cstype->list_of_csi = csi->csi_list_cs_type_next;
			} else {
				prev_csi->csi_list_cs_type_next = csi->csi_list_cs_type_next;
			}
		}

		csi->csi_list_cs_type_next = nullptr;
		csi->cstype = nullptr;
	}
}

static int is_config_valid(const std::string& dn, CcbUtilOperationData_t *opdata)
{
	std::string::size_type parent;

	if ((parent = dn.find(',')) == std::string::npos) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn.c_str());
		return 0;
	}

	/* Should be children to the Comp Base type */
	if (dn.compare(parent + 1, 10, "safCSType=")) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", dn.substr(parent + 1).c_str(), dn.c_str());
		return 0;
	}

	return 1;
}

/**
 * Get configuration for all SaAmfCSType objects from IMM and create internal objects.
 * 
 * 
 * @return int
 */
SaAisErrorT avd_cstype_config_get(void)
{
	SaAisErrorT error = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfCSType";
	AVD_CS_TYPE *cst;

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if (immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, nullptr, SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam, nullptr, &searchHandle) != SA_AIS_OK) {
		LOG_ER("saImmOmSearchInitialize_2 failed: %u", error);
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		if (!is_config_valid(Amf::to_string(&dn), nullptr))
			goto done2;

		if ((cst = cstype_db->find(Amf::to_string(&dn))) == nullptr){
			if ((cst = cstype_create(Amf::to_string(&dn), attributes)) == nullptr)
				goto done2;

			cstype_add_to_model(cst);
		}
	}

	error = SA_AIS_OK;

done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
done1:
	TRACE_LEAVE();
	return error;
}

/**
 * Handle a CCB completed event for SaAmfCSType
 * @param opdata
 * 
 * @return SaAisErrorT
 */
static SaAisErrorT cstype_ccb_completed_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	AVD_CS_TYPE *cst;
	AVD_CSI *csi; 
	bool csi_exist = false;
	CcbUtilOperationData_t *t_opData;
	const std::string object_name(Amf::to_string(&opdata->objectName));

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, object_name.c_str());

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(object_name, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		report_ccb_validation_error(opdata, "Modification of SaAmfCSType not supported");
		break;
	case CCBUTIL_DELETE:
		cst = cstype_db->find(object_name);
		if (cst->list_of_csi != nullptr) {
			/* check whether there exists a delete operation for 
			 * each of the CSI in the cs_type list in the current CCB 
			 */                      
			csi = cst->list_of_csi;
			while (csi != nullptr) {
				const SaNameTWrapper csi_name(csi->name);
				t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, csi_name);
				if ((t_opData == nullptr) || (t_opData->operationType != CCBUTIL_DELETE)) {
					csi_exist = true;   
					break;                  
				}                       
				csi = csi->csi_list_cs_type_next;
			}                       
			if (csi_exist == true) {
				report_ccb_validation_error(opdata, "SaAmfCSType '%s' is in use", cst->name.c_str());
				goto done;
			}
		}
		opdata->userData = cst;	/* Save for later use in apply */
		rc = SA_AIS_OK;
		break;
	default:
		osafassert(0);
		break;
	}

done:
	return rc;
}

static void cstype_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_CS_TYPE *cst;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		cst = cstype_create(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues);
		osafassert(cst);
		cstype_add_to_model(cst);
		break;
	case CCBUTIL_DELETE:
		cstype_delete(static_cast<AVD_CS_TYPE*>(opdata->userData));
		break;
	default:
		osafassert(0);
		break;
	}
}

void avd_cstype_constructor(void)
{
	cstype_db= new AmfDb<std::string, AVD_CS_TYPE>;

	avd_class_impl_set("SaAmfCSType", nullptr, nullptr, cstype_ccb_completed_hdlr,
		cstype_ccb_apply_cb);
	avd_class_impl_set("SaAmfCSBaseType", nullptr, nullptr,
		avd_imm_default_OK_completed_cb, nullptr);
}
