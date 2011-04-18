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

#include <avd_util.h>
#include <avd_app.h>
#include <avd_cluster.h>
#include <avd_imm.h>
#include <avd_si.h>
#include <avd_util.h>

static NCS_PATRICIA_TREE app_db;

void avd_app_db_add(AVD_APP *app)
{
	unsigned int rc;

	if (avd_app_get(&app->name) == NULL) {
		rc = ncs_patricia_tree_add(&app_db, &app->tree_node);
		assert(rc == NCSCC_RC_SUCCESS);
	}
}

AVD_APP *avd_app_new(const SaNameT *dn)
{
	AVD_APP *app;

	if ((app = calloc(1, sizeof(AVD_APP))) == NULL) {
		LOG_ER("calloc FAILED");
		return NULL;
	}

	memcpy(app->name.value, dn->value, dn->length);
	app->name.length = dn->length;
	app->tree_node.key_info = (uns8 *)&(app->name);

	return app;
}

void avd_app_delete(AVD_APP *app)
{
	unsigned int rc = ncs_patricia_tree_del(&app_db, &app->tree_node);
	assert(rc == NCSCC_RC_SUCCESS);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, app, AVSV_CKPT_AVD_APP_CONFIG);
	avd_apptype_remove_app(app);
	free(app);
}

AVD_APP *avd_app_get(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_APP *)ncs_patricia_tree_get(&app_db, (uns8 *)&tmp);
}

AVD_APP *avd_app_getnext(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_APP *)ncs_patricia_tree_getnext(&app_db, (uns8 *)&tmp);
}

static void app_add_to_model(AVD_APP *app)
{
	TRACE_ENTER2("%s", app->name.value);

	/* Check type link to see if it has been added already */
	if (app->app_type != NULL) {
		TRACE("already added");
		goto done;
	}

	avd_app_db_add(app);
	/* Find application type and make a link with app type */
	app->app_type = avd_apptype_get(&app->saAmfAppType);
	assert(app->app_type);
	avd_apptype_add_app(app);

	m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(avd_cb, app, AVSV_CKPT_AVD_APP_CONFIG);

done:
	TRACE_LEAVE();
}

void avd_app_add_si(AVD_APP *app, AVD_SI *si)
{
	si->si_list_app_next = si->app->list_of_si;
	si->app->list_of_si = si;
}

void avd_app_remove_si(AVD_APP *app, AVD_SI *si)
{
	AVD_SI *i_si;
	AVD_SI *prev_si = NULL;

	if (!app)
		return;

	i_si = app->list_of_si;

	while ((i_si != NULL) && (i_si != si)) {
		prev_si = i_si;
		i_si = i_si->si_list_app_next;
	}
	
	if (i_si != si) {
		/* Log a fatal error */
		assert(0);
	} else {
		if (prev_si == NULL) {
			app->list_of_si = si->si_list_app_next;
		} else {
			prev_si->si_list_app_next = si->si_list_app_next;
		}
	}
	
	si->si_list_app_next = NULL;
	si->app = NULL;
}

void avd_app_add_sg(AVD_APP *app, AVD_SG *sg)
{
	sg->sg_list_app_next = app->list_of_sg;
	app->list_of_sg = sg;
	app->saAmfApplicationCurrNumSGs++;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, app, AVSV_CKPT_AVD_APP_CONFIG);
}

void avd_app_remove_sg(AVD_APP *app, AVD_SG *sg)
{
	AVD_SG *i_sg;
	AVD_SG *prev_sg = NULL;

	if (!app)
		return;

	i_sg = app->list_of_sg;

	while ((i_sg != NULL) && (i_sg != sg)) {
		prev_sg = i_sg;
		i_sg = i_sg->sg_list_app_next;
	}
	
	if (i_sg != sg) {
		/* Log a fatal error */
		assert(0);
	} else {
		if (prev_sg == NULL) {
			app->list_of_sg = sg->sg_list_app_next;
		} else {
			prev_sg->sg_list_app_next = sg->sg_list_app_next;
		}
	}

	assert(app->saAmfApplicationCurrNumSGs > 0);
	app->saAmfApplicationCurrNumSGs--;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, app, AVSV_CKPT_AVD_APP_CONFIG);
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
		LOG_ER("Parent to '%s' is not root", dn->value);
		return 0;
	}

	rc = immutil_getAttr("saAmfAppType", attributes, 0, &aname);
	assert(rc == SA_AIS_OK);

	if (avd_apptype_get(&aname) == NULL) {
		/* App type does not exist in current model, check CCB */
		if (opdata == NULL) {
			LOG_ER("'%s' does not exist in model", aname.value);
			return 0;
		}

		if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &aname) == NULL) {
			LOG_ER("'%s' does not exist in existing model or in CCB", aname.value);
			return 0;
		}
	}

	if ((immutil_getAttr("saAmfApplicationAdminState", attributes, 0, &admstate) == SA_AIS_OK) &&
	    !avd_admin_state_is_valid(admstate)) {
		LOG_ER("Invalid saAmfApplicationAdminState %u for '%s'", admstate, dn->value);
		return 0;
	}

	return 1;
}

AVD_APP *avd_app_create(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes)
{
	int rc = -1;
	AVD_APP *app;
	SaAisErrorT error;

	TRACE_ENTER2("'%s'", dn->value);

	/*
	** If called at new active at failover, the object is found in the DB
	** but needs to get configuration attributes initialized.
	*/
	if (NULL == (app = avd_app_get(dn))) {
		if ((app = avd_app_new(dn)) == NULL)
			goto done;
	} else
		TRACE("already created, refreshing config...");

	error = immutil_getAttr("saAmfAppType", attributes, 0, &app->saAmfAppType);
	assert(error == SA_AIS_OK);
	
	if (immutil_getAttr("saAmfApplicationAdminState", attributes, 0, &app->saAmfApplicationAdminState) != SA_AIS_OK) {
		/* Empty, assign default value */
		app->saAmfApplicationAdminState = SA_AMF_ADMIN_UNLOCKED;
	}

	rc = 0;

done:
	if (rc != 0) {
		avd_app_delete(app);
		app = NULL;
	}

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
					LOG_ER("saAmfAppType '%s' not found", dn.value);
					goto done;
				}
				rc = SA_AIS_OK;
				break;
			} else
				assert(0);
		}
		break;
	case CCBUTIL_DELETE:
		/* just return OK 
		 * actual validation will be done at SU and SI level objects
		 */
		rc = SA_AIS_OK;
		break;
	default:
		assert(0);
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
		assert(app);
		app_add_to_model(app);
		break;
	case CCBUTIL_MODIFY: {
		const SaImmAttrModificationT_2 *attr_mod;
		app = avd_app_get(&opdata->objectName);

		while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
			const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;

			if (!strcmp(attribute->attrName, "saAmfAppType")) {
				avd_apptype_remove_app(app);
				app->saAmfAppType = *((SaNameT*)attribute->attrValues[0]);
				app->app_type = avd_apptype_get(&app->saAmfAppType);
				avd_apptype_add_app(app);
				LOG_NO("Changed saAmfAppType to '%s' for '%s'",
					app->saAmfAppType.value, app->name.value);
				break;
			}
			else
				assert(0);
		}
		break;
	}
	case CCBUTIL_DELETE:
		app = avd_app_get(&opdata->objectName);
		/* by this time all the SGs and SIs under this 
		 * app object should have been *DELETED* just  
		 * do a sanity check here
		 */
		assert(app->list_of_sg == NULL);
		assert(app->list_of_si == NULL);
		avd_app_delete(app);
		break;
	default:
		assert(0);
	}

	TRACE_LEAVE();
}

static void app_admin_op_cb(SaImmOiHandleT immOiHandle, SaInvocationT invocation,
	const SaNameT *object_name, SaImmAdminOperationIdT op_id,
	const SaImmAdminOperationParamsT_2 **params)
{
	SaAisErrorT rc;
	AVD_APP *app;
	AVD_SG *sg;

	TRACE_ENTER2("%s", object_name->value);

	/* Find the app name. */
	app = avd_app_get(object_name);
	assert(app != NULL);

	if (op_id == SA_AMF_ADMIN_UNLOCK) {
		if (app->saAmfApplicationAdminState == SA_AMF_ADMIN_UNLOCKED) {
			LOG_ER("%s is already unlocked", object_name->value);
			rc = SA_AIS_ERR_NO_OP;
			goto done;
		}

		if (app->saAmfApplicationAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			LOG_ER("%s is locked instantiation", object_name->value);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}
	}

	if (op_id == SA_AMF_ADMIN_LOCK) {
		if (app->saAmfApplicationAdminState == SA_AMF_ADMIN_LOCKED) {
			LOG_ER("%s is already locked", object_name->value);
			rc = SA_AIS_ERR_NO_OP;
			goto done;
		}

		if (app->saAmfApplicationAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			LOG_ER("%s is locked instantiation", object_name->value);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}
	}

	sg = app->list_of_sg;
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
		rc = SA_AIS_ERR_NOT_SUPPORTED;
		break;
	}
 done:
	(void)immutil_saImmOiAdminOperationResult(immOiHandle, invocation, rc);
}

static SaAisErrorT app_rt_attr_cb(SaImmOiHandleT immOiHandle,
	const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	AVD_APP *app = avd_app_get(objectName);
	SaImmAttrNameT attributeName;
	int i = 0;

	TRACE_ENTER2("%s", objectName->value);
	assert(app != NULL);

	while ((attributeName = attributeNames[i++]) != NULL) {
		TRACE("Attribute %s", attributeName);
		if (!strcmp(attributeName, "saAmfApplicationCurrNumSGs")) {
			(void)avd_saImmOiRtObjectUpdate(objectName, attributeName,
				SA_IMM_ATTR_SAUINT32T, &app->saAmfApplicationCurrNumSGs);
		} else
			assert(0);
	}

	return SA_AIS_OK;
}

SaAisErrorT avd_app_config_get(void)
{
	SaAisErrorT error = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfApplication";
	AVD_APP *app;

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if (immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, NULL, SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
		NULL, &searchHandle) != SA_AIS_OK) {

		LOG_ER("No AMF applications found");
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn,
		(SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {

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

	error = SA_AIS_OK;
 done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
 done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

void avd_app_constructor(void)
{
	NCS_PATRICIA_PARAMS patricia_params;

	patricia_params.key_size = sizeof(SaNameT);
	assert(ncs_patricia_tree_init(&app_db, &patricia_params) == NCSCC_RC_SUCCESS);

	avd_class_impl_set("SaAmfApplication", app_rt_attr_cb, app_admin_op_cb,
		app_ccb_completed_cb, app_ccb_apply_cb);
}

