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


#include "osaf/immutil/immutil.h"
#include "base/logtrace.h"

#include "amf/amfd/util.h"
#include "amf/amfd/sutcomptype.h"
#include "amf/amfd/imm.h"

AmfDb<std::string, AVD_SUTCOMP_TYPE> *sutcomptype_db = nullptr;

static void sutcomptype_db_add(AVD_SUTCOMP_TYPE *sutcomptype)
{
	unsigned int rc = sutcomptype_db->insert(sutcomptype->name,sutcomptype);
	osafassert(rc == NCSCC_RC_SUCCESS);
}

static AVD_SUTCOMP_TYPE *sutcomptype_create(const std::string& dn, const SaImmAttrValuesT_2 **attributes)
{
	AVD_SUTCOMP_TYPE *sutcomptype;

	TRACE_ENTER2("'%s'", dn.c_str());

	sutcomptype = new AVD_SUTCOMP_TYPE();

	sutcomptype->name = dn;

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSutMaxNumComponents"), attributes, 0, &sutcomptype->saAmfSutMaxNumComponents) != SA_AIS_OK)
		sutcomptype->saAmfSutMaxNumComponents = -1; /* no limit */

	    if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSutMinNumComponents"), attributes, 0, &sutcomptype->saAmfSutMinNumComponents) != SA_AIS_OK)
		sutcomptype->saAmfSutMinNumComponents = 1;

	TRACE_LEAVE();
	return sutcomptype;
}

static void sutcomptype_delete(AVD_SUTCOMP_TYPE *sutcomptype)
{
	sutcomptype_db->erase(sutcomptype->name);
	delete sutcomptype;
}

static int is_config_valid(const std::string& dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
	// TODO
	return 1;
}

SaAisErrorT avd_sutcomptype_config_get(const std::string& sutype_name, AVD_SUTYPE *sut)
{
	AVD_SUTCOMP_TYPE *sutcomptype;
	SaAisErrorT error;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfSutCompType";

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_o2(avd_cb->immOmHandle, sutype_name.c_str(), SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
		nullptr, &searchHandle);
	
	if (SA_AIS_OK != error) {
		LOG_ER("saImmOmSearchInitialize_2 failed: %u", error);
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		if (!is_config_valid(Amf::to_string(&dn), attributes, nullptr))
			goto done2;
		if ((sutcomptype = sutcomptype_db->find(Amf::to_string(&dn))) == nullptr) {
			if ((sutcomptype = sutcomptype_create(Amf::to_string(&dn), attributes)) == nullptr) {
				error = SA_AIS_ERR_FAILED_OPERATION;
				goto done2;
			}

			sutcomptype_db_add(sutcomptype);
		}
	}

	error = SA_AIS_OK;

 done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
 done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

static SaAisErrorT sutcomptype_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	AVD_SUTCOMP_TYPE *sutcomptype = nullptr;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues, opdata))
		    rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		report_ccb_validation_error(opdata, "Modification of SaAmfSUType not supported");
		break;
	case CCBUTIL_DELETE:
		sutcomptype = sutcomptype_db->find(Amf::to_string(&opdata->objectName));
		if (sutcomptype->curr_num_components == 0) {
			rc = SA_AIS_OK;
			opdata->userData = sutcomptype;	/* Save for later use in apply */
		}
		break;
	default:
		osafassert(0);
		break;
	}

	return rc;
}

static void sutcomptype_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_SUTCOMP_TYPE *sutcomptype = nullptr;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		sutcomptype = sutcomptype_create(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues);
		osafassert(sutcomptype);
		sutcomptype_db_add(sutcomptype);
		break;
	case CCBUTIL_DELETE:
		sutcomptype_delete(static_cast<AVD_SUTCOMP_TYPE*>(opdata->userData));
		break;
	default:
		osafassert(0);
		break;
	}
}

void avd_sutcomptype_constructor(void)
{

	sutcomptype_db = new AmfDb<std::string, AVD_SUTCOMP_TYPE>;
	avd_class_impl_set("SaAmfSutCompType", nullptr, nullptr,
		sutcomptype_ccb_completed_cb, sutcomptype_ccb_apply_cb);
}

