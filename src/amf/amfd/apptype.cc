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
#include "amf/amfd/app.h"
#include "amf/amfd/cluster.h"
#include "amf/amfd/imm.h"
#include "amf/amfd/si.h"
#include "amf/amfd/util.h"
#include "amf/amfd/sgtype.h"

AmfDb<std::string, AVD_APP_TYPE> *app_type_db = 0;

//
AVD_APP_TYPE::AVD_APP_TYPE(const std::string& dn) :
	name(dn)
{
}

//
AVD_APP_TYPE::~AVD_APP_TYPE() {
}

AVD_APP_TYPE *avd_apptype_get(const std::string& dn)
{
	return app_type_db->find(dn);
}

static void apptype_delete(AVD_APP_TYPE **apptype)
{
	app_type_db->erase((*apptype)->name);

	(*apptype)->sgAmfApptSGTypes.clear();
	delete *apptype;
	*apptype = nullptr;
}

static void apptype_add_to_model(AVD_APP_TYPE *app_type)
{
	unsigned int rc;
	osafassert(app_type != nullptr);
	TRACE("'%s'", app_type->name.c_str());

	rc = app_type_db->insert(app_type->name, app_type);
	osafassert(rc == NCSCC_RC_SUCCESS);
}

static int is_config_valid(const std::string& dn, const SaImmAttrValuesT_2 **attributes,
	const CcbUtilOperationData_t *opdata)
{
	int i = 0;
	unsigned j;
	std::string::size_type parent;
	AVD_AMF_SG_TYPE *sg_type;
	const SaImmAttrValuesT_2 *attr;

	parent = dn.find(',');
	if (parent == std::string::npos) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn.c_str());
		return 0;
	}

	/* AppType should be children to AppBaseType */
	if (dn.compare(parent + 1, 11, "safAppType=") != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", dn.substr(0, parent).c_str(), dn.c_str());
		return 0;
	}

	while ((attr = attributes[i++]) != nullptr)
		if (!strcmp(attr->attrName, "saAmfApptSGTypes"))
			break;

	osafassert(attr);
	osafassert(attr->attrValuesNumber > 0);

	for (j = 0; j < attr->attrValuesNumber; j++) {
		SaNameT *name = (SaNameT *)attr->attrValues[j];
		sg_type = sgtype_db->find(Amf::to_string(name));
		if (sg_type == nullptr) {
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

	return 1;
}

static AVD_APP_TYPE *apptype_create(const std::string& dn, const SaImmAttrValuesT_2 **attributes)
{
	int i = 0;
	unsigned int j;
	int rc = -1;
	AVD_APP_TYPE *app_type;
	const SaImmAttrValuesT_2 *attr;

	TRACE_ENTER2("'%s'", dn.c_str());

	app_type = new AVD_APP_TYPE(dn);

	while ((attr = attributes[i++]) != nullptr)
		if (!strcmp(attr->attrName, "saAmfApptSGTypes"))
			break;

	osafassert(attr);
	osafassert(attr->attrValuesNumber > 0);

	for (j = 0; j < attr->attrValuesNumber; j++) {
		app_type->sgAmfApptSGTypes.push_back(Amf::to_string(static_cast<SaNameT*>(attr->attrValues[j])));
	}

	rc = 0;

	if (rc != 0)
		apptype_delete(&app_type);

	TRACE_LEAVE();

	return app_type;
}

static SaAisErrorT apptype_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	AVD_APP_TYPE *app_type;
	AVD_APP *app;
	bool app_exist = false;
	CcbUtilOperationData_t *t_opData;
	const std::string object_name(Amf::to_string(&opdata->objectName));

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, object_name.c_str());

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(object_name, opdata->param.create.attrValues, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		report_ccb_validation_error(opdata, "Modification of SaAmfAppType not supported");
		break;
	case CCBUTIL_DELETE:
		app_type = avd_apptype_get(object_name);
		if (nullptr != app_type->list_of_app) {
			/* check whether there exists a delete operation for 
			 * each of the App in the app_type list in the current CCB
			 */
			app = app_type->list_of_app;
			while (app != nullptr) {
				const SaNameTWrapper app_name(app->name);
				t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, app_name);
				if ((t_opData == nullptr) || (t_opData->operationType != CCBUTIL_DELETE)) {
					app_exist = true;
					break;
				}
				app = app->app_type_list_app_next;
			}
			if (app_exist == true) {
				report_ccb_validation_error(opdata, "SaAmfAppType '%s' is in use", app_type->name.c_str());
				goto done;
			}
		}
		opdata->userData = app_type;	/* Save for later use in apply */
		rc = SA_AIS_OK;
		break;
	default:
		osafassert(0);
		break;
	}
done:
	return rc;
}

static void apptype_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_APP_TYPE *app_type;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		app_type = apptype_create(Amf::to_string(&opdata->objectName),
			opdata->param.create.attrValues);
		osafassert(app_type);
		apptype_add_to_model(app_type);
		break;
	case CCBUTIL_DELETE:
		app_type = static_cast<AVD_APP_TYPE*>(opdata->userData);
		apptype_delete(&app_type);
		break;
	default:
		osafassert(0);
			break;
	}
}

SaAisErrorT avd_apptype_config_get(void)
{
	SaAisErrorT rc = SA_AIS_ERR_INVALID_PARAM, error;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn_name;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfAppType";
	AVD_APP_TYPE *app_type;

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, nullptr, SA_IMM_SUBTREE,
			SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
			nullptr, &searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("No AMF app types found");
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn_name, const_cast<SaImmAttrValuesT_2 ***>(&attributes)) == SA_AIS_OK) {
		std::string dn(Amf::to_string(&dn_name));
		if (!is_config_valid(dn, attributes, nullptr))
			goto done2;

		if ((app_type = avd_apptype_get(dn)) == nullptr ) {
			if ((app_type = apptype_create(dn, attributes)) == nullptr)
				goto done2;

			apptype_add_to_model(app_type);
		}
	}

	rc = SA_AIS_OK;
done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
done1:
	TRACE_LEAVE2("%u", error);
	return rc;
}

void avd_apptype_add_app(AVD_APP *app)
{
	app->app_type_list_app_next = app->app_type->list_of_app;
	app->app_type->list_of_app = app;
}

void avd_apptype_remove_app(AVD_APP *app)
{
	AVD_APP *i_app = nullptr;
	AVD_APP *prev_app = nullptr;

	if (app->app_type != nullptr) {
		i_app = app->app_type->list_of_app;

		while ((i_app != nullptr) && (i_app != app)) {
			prev_app = i_app;
			i_app = i_app->app_type_list_app_next;
		}

		if (i_app != app) {
			/* Log a fatal error */
		} else {
			if (prev_app == nullptr) {
				app->app_type->list_of_app = app->app_type_list_app_next;
			} else {
				prev_app->app_type_list_app_next = app->app_type_list_app_next;
			}
		}

		app->app_type_list_app_next = nullptr;
		app->app_type = nullptr;
	}
}

void avd_apptype_constructor(void)
{
	app_type_db = new AmfDb<std::string, AVD_APP_TYPE>;

	avd_class_impl_set("SaAmfAppBaseType", nullptr, nullptr,
			avd_imm_default_OK_completed_cb, nullptr);
	avd_class_impl_set("SaAmfAppType", nullptr, nullptr, apptype_ccb_completed_cb,
			apptype_ccb_apply_cb);
}

