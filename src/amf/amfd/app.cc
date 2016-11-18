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
#include "amf/amfd/app.h"
#include "amf/amfd/cluster.h"
#include "amf/amfd/imm.h"

AmfDb<std::string, AVD_APP> *app_db = 0;

AVD_APP::AVD_APP() :
	name{},
	saAmfAppType{},
	saAmfApplicationAdminState(SA_AMF_ADMIN_UNLOCKED),
	saAmfApplicationCurrNumSGs(0),
	list_of_sg(nullptr),
	list_of_si(nullptr),
	app_type_list_app_next(nullptr),
	app_type(nullptr)
{
}

AVD_APP::AVD_APP(const std::string& dn) :
	name{dn},
	saAmfAppType{},
	saAmfApplicationAdminState(SA_AMF_ADMIN_UNLOCKED),
	saAmfApplicationCurrNumSGs(0),
	list_of_sg(nullptr),
	list_of_si(nullptr),
	app_type_list_app_next(nullptr),
	app_type(nullptr)
{
}

AVD_APP::~AVD_APP()
{
}
		
// TODO(hafe) change this to a destructor
static void avd_app_delete(AVD_APP *app)
{
	app_db->erase(app->name);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, app, AVSV_CKPT_AVD_APP_CONFIG);
	avd_apptype_remove_app(app);
	delete app;
}

static void app_add_to_model(AVD_APP *app)
{
	TRACE_ENTER2("%s", app->name.c_str());

	/* Check type link to see if it has been added already */
	if (app->app_type != nullptr) {
		TRACE("already added");
		goto done;
	}

	app_db->insert(app->name, app);

	/* Find application type and make a link with app type */
	app->app_type = avd_apptype_get(app->saAmfAppType);
	osafassert(app->app_type);
	avd_apptype_add_app(app);

	m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(avd_cb, app, AVSV_CKPT_AVD_APP_CONFIG);

done:
	TRACE_LEAVE();
}

void AVD_APP::add_si(AVD_SI *si)
{
	si->si_list_app_next = list_of_si;
	list_of_si = si;
}

void AVD_APP::remove_si(AVD_SI *si)
{
	AVD_SI *i_si;
	AVD_SI *prev_si = nullptr;

	i_si = list_of_si;

	while ((i_si != nullptr) && (i_si != si)) {
		prev_si = i_si;
		i_si = i_si->si_list_app_next;
	}
	
	if (i_si != si) {
		/* Log a fatal error */
		osafassert(0);
	} else {
		if (prev_si == nullptr) {
			list_of_si = si->si_list_app_next;
		} else {
			prev_si->si_list_app_next = si->si_list_app_next;
		}
	}
	
	si->si_list_app_next = nullptr;
	si->app = nullptr;
}

void AVD_APP::add_sg(AVD_SG *sg)
{
	sg->sg_list_app_next = list_of_sg;
	list_of_sg = sg;
	saAmfApplicationCurrNumSGs++;
	if (avd_cb->avd_peer_ver < AVD_MBCSV_SUB_PART_VERSION_4)
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_AVD_APP_CONFIG);
}

void AVD_APP::remove_sg(AVD_SG *sg)
{
	AVD_SG *i_sg;
	AVD_SG *prev_sg = nullptr;

	i_sg = list_of_sg;

	while ((i_sg != nullptr) && (i_sg != sg)) {
		prev_sg = i_sg;
		i_sg = i_sg->sg_list_app_next;
	}
	
	if (i_sg != sg) {
		/* Log a fatal error */
		osafassert(0);
	} else {
		if (prev_sg == nullptr) {
			list_of_sg = sg->sg_list_app_next;
		} else {
			prev_sg->sg_list_app_next = sg->sg_list_app_next;
		}
	}

	osafassert(saAmfApplicationCurrNumSGs > 0);
	saAmfApplicationCurrNumSGs--;
	if (avd_cb->avd_peer_ver < AVD_MBCSV_SUB_PART_VERSION_4)
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_AVD_APP_CONFIG);
	sg->sg_list_app_next = nullptr;
	sg->app = nullptr;
}

static int is_config_valid(const std::string& dn, const SaImmAttrValuesT_2 **attributes,
	const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;
	SaNameT aname;
	SaAmfAdminStateT admstate;

	/* Applications should be root objects */
	if (dn.find(',') != std::string::npos) {
		report_ccb_validation_error(opdata, "Parent to '%s' is not root", dn.c_str());
		return 0;
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfAppType"), attributes, 0, &aname);
	osafassert(rc == SA_AIS_OK);

	if (avd_apptype_get(Amf::to_string(&aname)) == nullptr) {
		/* App type does not exist in current model, check CCB */
		if (opdata == nullptr) {
			report_ccb_validation_error(opdata, "'%s' does not exist in model", osaf_extended_name_borrow(&aname));
			return 0;
		}

		if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &aname) == nullptr) {
			report_ccb_validation_error(opdata, "'%s' does not exist in existing model or in CCB", osaf_extended_name_borrow(&aname));
			return 0;
		}
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfApplicationAdminState"), attributes, 0, &admstate) == SA_AIS_OK) &&
	    !avd_admin_state_is_valid(admstate, opdata)) {
		report_ccb_validation_error(opdata, "Invalid saAmfApplicationAdminState %u for '%s'", admstate, dn.c_str());
		return 0;
	}

	return 1;
}

AVD_APP *avd_app_create(const std::string& dn, const SaImmAttrValuesT_2 **attributes)
{
	AVD_APP *app;
	SaAisErrorT error;
	SaNameT app_type;

	TRACE_ENTER2("'%s'", dn.c_str());

	/*
	** If called at new active at failover, the object is found in the DB
	** but needs to get configuration attributes initialized.
	*/
	app = app_db->find(dn);
	if (app == nullptr) {
		app = new AVD_APP(dn);
	} else {
		TRACE("already created, refreshing config...");
	}

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfAppType"), attributes, 0, &app_type);
	osafassert(error == SA_AIS_OK);
	app->saAmfAppType = Amf::to_string(&app_type);
	
	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfApplicationAdminState"), attributes, 0, &app->saAmfApplicationAdminState) != SA_AIS_OK) {
		/* Empty, assign default value */
		app->saAmfApplicationAdminState = SA_AMF_ADMIN_UNLOCKED;
	}

	TRACE_LEAVE();
	return app;
}

static SaAisErrorT app_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	const SaImmAttrModificationT_2 *attr_mod;
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	int i = 0;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		while ((attr_mod = opdata->param.modify.attrMods[i++]) != nullptr) {
			const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;

			if (!strcmp(attribute->attrName, "saAmfAppType")) {
				SaNameT dn = *((SaNameT*)attribute->attrValues[0]);
				if (nullptr == avd_apptype_get(Amf::to_string(&dn))) {
					report_ccb_validation_error(opdata, "saAmfAppType '%s' not found", osaf_extended_name_borrow(&dn));
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
				rc = SA_AIS_OK;
				break;
			} else {
				report_ccb_validation_error(opdata,
					"Unknown attribute '%s'",
					attribute->attrName);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		}
		break;
	case CCBUTIL_DELETE:
		/* just return OK 
		 * actual validation will be done at SU and SI level objects
		 */
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

static void app_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_APP *app;
	int i = 0;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		app = avd_app_create(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues);
		osafassert(app);
		app_add_to_model(app);
		break;
	case CCBUTIL_MODIFY: {
		const SaImmAttrModificationT_2 *attr_mod;
		app = app_db->find(Amf::to_string(&opdata->objectName));

		while ((attr_mod = opdata->param.modify.attrMods[i++]) != nullptr) {
			const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;

			if (!strcmp(attribute->attrName, "saAmfAppType")) {
				LOG_NO("Modified saAmfAppType from '%s' to '%s' for '%s'", app->saAmfAppType.c_str(),
						osaf_extended_name_borrow(static_cast<SaNameT*>(attribute->attrValues[0])), app->name.c_str());
				avd_apptype_remove_app(app);
				app->saAmfAppType = Amf::to_string(static_cast<SaNameT*>(attribute->attrValues[0]));
				app->app_type = avd_apptype_get(app->saAmfAppType);
				avd_apptype_add_app(app);
			}
			else {
				osafassert(0);
			}
		}
		break;
	}
	case CCBUTIL_DELETE:
		app = app_db->find(Amf::to_string(&opdata->objectName));
		/* by this time all the SGs and SIs under this 
		 * app object should have been *DELETED* just  
		 * do a sanity check here
		 */
		osafassert(app->list_of_sg == nullptr);
		osafassert(app->list_of_si == nullptr);
		avd_app_delete(app);
		break;
	default:
		osafassert(0);
	}

	TRACE_LEAVE();
}

static void app_admin_op_cb(SaImmOiHandleT immOiHandle, SaInvocationT invocation,
	const SaNameT *object_name, SaImmAdminOperationIdT op_id,
	const SaImmAdminOperationParamsT_2 **params)
{
	AVD_APP *app;

	TRACE_ENTER2("%s", osaf_extended_name_borrow(object_name));

	/* Find the app name. */
	app = app_db->find(Amf::to_string(object_name));
	osafassert(app != nullptr);

	if (op_id == SA_AMF_ADMIN_UNLOCK) {
		if (app->saAmfApplicationAdminState == SA_AMF_ADMIN_UNLOCKED) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
					"%s is already unlocked", osaf_extended_name_borrow(object_name));
			goto done;
		}

		if (app->saAmfApplicationAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"%s is locked instantiation", osaf_extended_name_borrow(object_name));
			goto done;
		}
	}

	if (op_id == SA_AMF_ADMIN_LOCK) {
		if (app->saAmfApplicationAdminState == SA_AMF_ADMIN_LOCKED) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
					"%s is already locked", osaf_extended_name_borrow(object_name));
			goto done;
		}

		if (app->saAmfApplicationAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"%s is locked instantiation", osaf_extended_name_borrow(object_name));
			goto done;
		}
	}

	/* recursively perform admin op */
#if 0
	while (sg != nullptr) {
		avd_sa_avd_sg_object_admin_hdlr(avd_cb, object_name, op_id, params);
		sg = sg->sg_list_app_next;
	}
#endif
	switch (op_id) {
	case SA_AMF_ADMIN_SHUTDOWN:
	case SA_AMF_ADMIN_UNLOCK:
	case SA_AMF_ADMIN_LOCK:
	case SA_AMF_ADMIN_LOCK_INSTANTIATION:
	case SA_AMF_ADMIN_UNLOCK_INSTANTIATION:
	case SA_AMF_ADMIN_RESTART:
	default:
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NOT_SUPPORTED, nullptr,
				"Not supported");
		break;
	}
done:
	TRACE_LEAVE();
}

static SaAisErrorT app_rt_attr_cb(SaImmOiHandleT immOiHandle,
	const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	AVD_APP *app = app_db->find(Amf::to_string(objectName));
	SaImmAttrNameT attributeName;
	int i = 0;
	const std::string obj(Amf::to_string(objectName));

	TRACE_ENTER2("%s", obj.c_str());
	osafassert(app != nullptr);

	while ((attributeName = attributeNames[i++]) != nullptr) {
		TRACE("Attribute %s", attributeName);
		if (!strcmp(attributeName, "saAmfApplicationCurrNumSGs")) {
			avd_saImmOiRtObjectUpdate_sync(obj, attributeName,
				SA_IMM_ATTR_SAUINT32T, &app->saAmfApplicationCurrNumSGs);
		} else {
			LOG_ER("Ignoring unknown attribute '%s'", attributeName);
		}
	}

	return SA_AIS_OK;
}

SaAisErrorT avd_app_config_get(void)
{
	SaAisErrorT error = SA_AIS_ERR_FAILED_OPERATION, rc;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfApplication";
	AVD_APP *app;
	SaImmAttrNameT configAttributes[] = {
	   const_cast<SaImmAttrNameT>("saAmfAppType"),
	   const_cast<SaImmAttrNameT>("saAmfApplicationAdminState"),
		nullptr
	};

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if (immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, nullptr, SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR, &searchParam,
		configAttributes, &searchHandle) != SA_AIS_OK) {

		LOG_ER("%s: saImmOmSearchInitialize_2 failed: %u", __FUNCTION__, error);
		goto done1;
	}

	while ((rc = immutil_saImmOmSearchNext_2(searchHandle, &dn,
		const_cast<SaImmAttrValuesT_2 ***>(&attributes))) == SA_AIS_OK) {

		if (!is_config_valid(Amf::to_string(&dn), attributes, nullptr))
			goto done2;

		if ((app = avd_app_create(Amf::to_string(&dn), static_cast<const SaImmAttrValuesT_2 **>(attributes))) == nullptr)
			goto done2;

		app_add_to_model(app);

		if (avd_sg_config_get(Amf::to_string(&dn), app) != SA_AIS_OK)
			goto done2;

		if (avd_si_config_get(app) != SA_AIS_OK)
			goto done2;
	}

	osafassert(rc == SA_AIS_ERR_NOT_EXIST);
	error = SA_AIS_OK;
 done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
 done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

void avd_app_constructor(void)
{
	app_db = new AmfDb<std::string, AVD_APP>;

	avd_class_impl_set(const_cast<SaImmClassNameT>("SaAmfApplication"),
			   app_rt_attr_cb,
			   app_admin_op_cb,
			   app_ccb_completed_cb,
			   app_ccb_apply_cb);
}

