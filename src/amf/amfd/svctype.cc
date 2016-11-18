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

#include "amf/amfd/si.h"
#include "imm/saf/saImmOm.h"
#include "osaf/immutil/immutil.h"
#include "amf/amfd/app.h"
#include "amf/amfd/cluster.h"
#include "amf/amfd/imm.h"
#include "amf/amfd/susi.h"
#include "amf/amfd/csi.h"
#include "amf/amfd/proc.h"
#include <algorithm>

AmfDb<std::string, AVD_SVC_TYPE> *svctype_db = nullptr;

//
// TODO(HANO) Temporary use this function instead of strdup which uses malloc.
// Later on remove this function and use std::string instead
#include <cstring>
static char *StrDup(const char *s)
{
	char *c = new char[strlen(s) + 1];
	std::strcpy(c,s);
	return c;
}

static void svctype_db_add(AVD_SVC_TYPE *svct)
{
	unsigned int rc = svctype_db->insert(svct->name,svct);
	osafassert (rc == NCSCC_RC_SUCCESS);
}


static void svctype_delete(AVD_SVC_TYPE *svc_type)
{
	svctype_db->erase(svc_type->name);

	if (svc_type->saAmfSvcDefActiveWeight != nullptr) {
		unsigned int i = 0;
		while (svc_type->saAmfSvcDefActiveWeight[i] != nullptr) {
			delete [] svc_type->saAmfSvcDefActiveWeight[i];
			++i;
		}
		delete [] svc_type->saAmfSvcDefActiveWeight;
	}

	if (svc_type->saAmfSvcDefStandbyWeight != nullptr) {
		unsigned int i = 0;
		while (svc_type->saAmfSvcDefStandbyWeight[i] != nullptr) {
			delete [] svc_type->saAmfSvcDefStandbyWeight[i];
			++i;
		}
		delete [] svc_type->saAmfSvcDefStandbyWeight;
	}
	delete svc_type;
}

//
AVD_SVC_TYPE::AVD_SVC_TYPE(const std::string& dn) :
	name(dn)
{
}

static AVD_SVC_TYPE *svctype_create(const std::string& dn, const SaImmAttrValuesT_2 **attributes)
{
	unsigned int i;
	AVD_SVC_TYPE *svct;
	SaUint32T attrValuesNumber;

	TRACE_ENTER2("'%s'", dn.c_str());

	svct = new AVD_SVC_TYPE(dn);

	svct->saAmfSvcDefActiveWeight = nullptr;
	svct->saAmfSvcDefStandbyWeight = nullptr;

	/* Optional, [0..*] */
	if (immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("saAmfSvcDefActiveWeight"), attributes, &attrValuesNumber) == SA_AIS_OK) {
		svct->saAmfSvcDefActiveWeight = new char*[attrValuesNumber + 1];
		for (i = 0; i < attrValuesNumber; i++) {
			svct->saAmfSvcDefActiveWeight[i] =
			    StrDup(immutil_getStringAttr(attributes, "saAmfSvcDefActiveWeight", i));
		}
		svct->saAmfSvcDefActiveWeight[i] = nullptr;
	}

	/* Optional, [0..*] */
	if (immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("saAmfSvcDefStandbyWeight"), attributes, &attrValuesNumber) == SA_AIS_OK) {
		svct->saAmfSvcDefStandbyWeight = new char*[attrValuesNumber + 1];
		for (i = 0; i < attrValuesNumber; i++) {
			svct->saAmfSvcDefStandbyWeight[i] =
			    StrDup(immutil_getStringAttr(attributes, "saAmfSvcDefStandbyWeight", i));
		}
		svct->saAmfSvcDefStandbyWeight[i] = nullptr;
	}

	TRACE_LEAVE();
	return svct;
}

static int is_config_valid(const std::string& dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
	std::string::size_type pos;

	if ((pos = dn.find(',')) == std::string::npos) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn.c_str());
		return 0;
	}

	/* Should be children to the SvcBasetype */
	if (dn.compare(pos + 1, 11, "safSvcType=") != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", dn.substr(pos +1).c_str(), dn.c_str());
		return 0;
	}

	return 1;
}

static SaAisErrorT svctype_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	AVD_SVC_TYPE *svc_type;

	bool si_exist = false;
	CcbUtilOperationData_t *t_opData;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues, opdata))
		    rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		report_ccb_validation_error(opdata, "Modification of SaAmfSvcType not supported");
		break;
	case CCBUTIL_DELETE:
		svc_type = svctype_db->find(Amf::to_string(&opdata->objectName));

		/* check whether there exists a delete operation for
		 * each of the SI in the svc_type list in the current CCB
		 */
		for (const auto& si : svc_type->list_of_si) {
			const SaNameTWrapper si_name(si->name);
			t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, si_name);
			if ((t_opData == nullptr) || (t_opData->operationType != CCBUTIL_DELETE)) {
				si_exist = true;
				break;
			}
		}

		if (si_exist == true) {
			report_ccb_validation_error(opdata, "SaAmfSvcType '%s' is in use",svc_type->name.c_str());
			goto done;
		}
		opdata->userData = svc_type;
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

static void svctype_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_SVC_TYPE *svc_type;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		svc_type = svctype_create(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues);
		osafassert(svc_type);
		svctype_db_add(svc_type);
		break;
	case CCBUTIL_DELETE:
		svctype_delete(static_cast<AVD_SVC_TYPE*>(opdata->userData));
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE();
}

/**
 * Get configuration for all SaAmfSvcType objects from IMM and 
 * create AVD internal objects. 
 * 
 * @param cb
 * @param app
 * 
 * @return int
 */
SaAisErrorT avd_svctype_config_get(void)
{
	SaAisErrorT error = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfSvcType";
	AVD_SVC_TYPE *svc_type;

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if (immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, nullptr, SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
		nullptr, &searchHandle) != SA_AIS_OK) {

		LOG_ER("No objects found (1)");
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		const std::string temp_dn(Amf::to_string(&dn));
		if (!is_config_valid(temp_dn, attributes, nullptr))
			goto done2;

		if ((svc_type = svctype_db->find(temp_dn)) == nullptr) {
			if ((svc_type = svctype_create(temp_dn, attributes)) == nullptr)
				goto done2;

			svctype_db_add(svc_type);
		}

		if (avd_svctypecstypes_config_get(temp_dn) != SA_AIS_OK)
			goto done2;
	}

	error = SA_AIS_OK;
 done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
 done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

void avd_svctype_add_si(AVD_SI *si)
{
	si->svc_type->list_of_si.push_back(si);
}

void avd_svctype_remove_si(AVD_SI *si)
{
  if (!si)
    return;

  AVD_SVC_TYPE *svc_type = si->svc_type;

  if (svc_type != nullptr) {
    auto pos = std::find(svc_type->list_of_si.begin(), svc_type->list_of_si.end(), si);
    if(pos != svc_type->list_of_si.end()) {
      svc_type->list_of_si.erase(pos);
    } else {
      /* Log a fatal error */
      osafassert(0);
    }
  }
}

void avd_svctype_constructor(void)
{
	svctype_db = new AmfDb<std::string, AVD_SVC_TYPE>;

	avd_class_impl_set("SaAmfSvcType", nullptr, nullptr, svctype_ccb_completed_cb,
			svctype_ccb_apply_cb);
	avd_class_impl_set("SaAmfSvcBaseType", nullptr, nullptr,
			avd_imm_default_OK_completed_cb, nullptr);
}

