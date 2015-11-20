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

#include <logtrace.h>

#include <util.h>
#include <app.h>
#include <cluster.h>
#include <imm.h>

AmfDb<std::string, AVD_APP> *app_db = 0;

AVD_APP::AVD_APP() :
	saAmfApplicationAdminState(SA_AMF_ADMIN_UNLOCKED),
	saAmfApplicationCurrNumSGs(0),
	list_of_sg(NULL),
	list_of_si(NULL),
	app_type_list_app_next(NULL),
	app_type(NULL)
{
	memset(&name, 0, sizeof(SaNameT));
	memset(&saAmfAppType, 0, sizeof(SaNameT));
}

AVD_APP::AVD_APP(const SaNameT* dn) :
	saAmfApplicationAdminState(SA_AMF_ADMIN_UNLOCKED),
	saAmfApplicationCurrNumSGs(0),
	list_of_sg(NULL),
	list_of_si(NULL),
	app_type_list_app_next(NULL),
	app_type(NULL)
{
	memset(&name, 0, sizeof(SaNameT));
	memcpy(name.value, dn->value, dn->length);
	name.length = dn->length;
	memset(&saAmfAppType, 0, sizeof(SaNameT));
}

AVD_APP::~AVD_APP()
{
}
		
// TODO(hafe) change this to a destructor
static void avd_app_delete(AVD_APP *app)
{
	app_db->erase(Amf::to_string(&app->name));
	m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, app, AVSV_CKPT_AVD_APP_CONFIG);
	avd_apptype_remove_app(app);
	delete app;
}

static void app_add_to_model(AVD_APP *app)
{
	TRACE_ENTER2("%s", app->name.value);

	/* Check type link to see if it has been added already */
	if (app->app_type != NULL) {
		TRACE("already added");
		goto done;
	}

	app_db->insert(Amf::to_string(&app->name), app);

	/* Find application type and make a link with app type */
	app->app_type = avd_apptype_get(&app->saAmfAppType);
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
	AVD_SI *prev_si = NULL;

	i_si = list_of_si;

	while ((i_si != NULL) && (i_si != si)) {
		prev_si = i_si;
		i_si = i_si->si_list_app_next;
	}
	
	if (i_si != si) {
		/* Log a fatal error */
		osafassert(0);
	} else {
		if (prev_si == NULL) {
			list_of_si = si->si_list_app_next;
		} else {
			prev_si->si_list_app_next = si->si_list_app_next;
		}
	}
	
	si->si_list_app_next = NULL;
	si->app = NULL;
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
	AVD_SG *prev_sg = NULL;

	i_sg = list_of_sg;

	while ((i_sg != NULL) && (i_sg != sg)) {
		prev_sg = i_sg;
		i_sg = i_sg->sg_list_app_next;
	}
	
	if (i_sg != sg) {
		/* Log a fatal error */
		osafassert(0);
	} else {
		if (prev_sg == NULL) {
			list_of_sg = sg->sg_list_app_next;
		} else {
			prev_sg->sg_list_app_next = sg->sg_list_app_next;
		}
	}

	osafassert(saAmfApplicationCurrNumSGs > 0);
	saAmfApplicationCurrNumSGs--;
	if (avd_cb->avd_peer_ver < AVD_MBCSV_SUB_PART_VERSION_4)
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_AVD_APP_CONFIG);
	sg->sg_list_app_next = NULL;
	sg->app = NULL;
}

static int is_config_valid(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes,
	const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;
	SaNameT aname;
	SaAmfAdminStateT admstate;

	/* Applications should be root objects */
	if (strchr((char *)dn->value, ',') != NULL) {
		report_ccb_validation_error(opdata, "Parent to '%s' is not root", dn->value);
		return 0;
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfAppType"), attributes, 0, &aname);
	osafassert(rc == SA_AIS_OK);

	if (avd_apptype_get(&aname) == NULL) {
		/* App type does not exist in current model, check CCB */
		if (opdata == NULL) {
			report_ccb_validation_error(opdata, "'%s' does not exist in model", aname.value);
			return 0;
		}

		if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &aname) == NULL) {
			report_ccb_validation_error(opdata, "'%s' does not exist in existing model or in CCB", aname.value);
			return 0;
		}
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfApplicationAdminState"), attributes, 0, &admstate) == SA_AIS_OK) &&
	    !avd_admin_state_is_valid(admstate, opdata)) {
		report_ccb_validation_error(opdata, "Invalid saAmfApplicationAdminState %u for '%s'", admstate, dn->value);
		return 0;
	}

	return 1;
}

AVD_APP *avd_app_create(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes)
{
	AVD_APP *app;
	SaAisErrorT error;

	TRACE_ENTER2("'%s'", dn->value);

	/*
	** If called at new active at failover, the object is found in the DB
	** but needs to get configuration attributes initialized.
	*/
	app = app_db->find(Amf::to_string(dn));
	if (app == NULL) {
		app = new AVD_APP(dn);
	} else
		TRACE("already created, refreshing config...");

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfAppType"), attributes, 0, &app->saAmfAppType);
	osafassert(error == SA_AIS_OK);
	
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

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(&opdata->objectName, opdata->param.create.attrValues, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
			const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;

			if (!strcmp(attribute->attrName, "saAmfAppType")) {
				SaNameT dn = *((SaNameT*)attribute->attrValues[0]);
				if (NULL == avd_apptype_get(&dn)) {
					report_ccb_validation_error(opdata, "saAmfAppType '%s' not found", dn.value);
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

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		app = avd_app_create(&opdata->objectName, opdata->param.create.attrValues);
		osafassert(app);
		app_add_to_model(app);
		break;
	case CCBUTIL_MODIFY: {
		const SaImmAttrModificationT_2 *attr_mod;
		app = app_db->find(Amf::to_string(&opdata->objectName));

		while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
			const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;

			if (!strcmp(attribute->attrName, "saAmfAppType")) {
				LOG_NO("Modified saAmfAppType from '%s' to '%s' for '%s'", app->saAmfAppType.value,
						(*((SaNameT*)attribute->attrValues[0])).value, app->name.value);
				avd_apptype_remove_app(app);
				app->saAmfAppType = *((SaNameT*)attribute->attrValues[0]);
				app->app_type = avd_apptype_get(&app->saAmfAppType);
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
		osafassert(app->list_of_sg == NULL);
		osafassert(app->list_of_si == NULL);
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

	TRACE_ENTER2("%s", object_name->value);

	/* Find the app name. */
	app = app_db->find(Amf::to_string(object_name));
	osafassert(app != NULL);

	if (op_id == SA_AMF_ADMIN_UNLOCK) {
		if (app->saAmfApplicationAdminState == SA_AMF_ADMIN_UNLOCKED) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, NULL,
					"%s is already unlocked", object_name->value);
			goto done;
		}

		if (app->saAmfApplicationAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
					"%s is locked instantiation", object_name->value);
			goto done;
		}
	}

	if (op_id == SA_AMF_ADMIN_LOCK) {
		if (app->saAmfApplicationAdminState == SA_AMF_ADMIN_LOCKED) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, NULL,
					"%s is already locked", object_name->value);
			goto done;
		}

		if (app->saAmfApplicationAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
					"%s is locked instantiation", object_name->value);
			goto done;
		}
	}

	/* recursively perform admin op */
#if 0
	while (sg != NULL) {
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
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NOT_SUPPORTED, NULL,
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

	TRACE_ENTER2("%s", objectName->value);
	osafassert(app != NULL);

	while ((attributeName = attributeNames[i++]) != NULL) {
		TRACE("Attribute %s", attributeName);
		if (!strcmp(attributeName, "saAmfApplicationCurrNumSGs")) {
			avd_saImmOiRtObjectUpdate_sync(objectName, attributeName,
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
		NULL
	};

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if (immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, NULL, SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR, &searchParam,
		configAttributes, &searchHandle) != SA_AIS_OK) {

		LOG_ER("%s: saImmOmSearchInitialize_2 failed: %u", __FUNCTION__, error);
		goto done1;
	}

	while ((rc = immutil_saImmOmSearchNext_2(searchHandle, &dn,
		(SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK) {

		if (!is_config_valid(&dn, attributes, NULL))
			goto done2;

		if ((app = avd_app_create(&dn, (const SaImmAttrValuesT_2 **)attributes)) == NULL)
			goto done2;

		app_add_to_model(app);

		if (avd_sg_config_get(&dn, app) != SA_AIS_OK)
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

