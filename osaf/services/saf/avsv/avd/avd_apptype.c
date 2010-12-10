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
#include <avd_app.h>
#include <avd_cluster.h>
#include <avd_imm.h>
#include <avd_si.h>
#include <avd_util.h>

static NCS_PATRICIA_TREE apptype_db;

AVD_APP_TYPE *avd_apptype_get(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_APP_TYPE *)ncs_patricia_tree_get(&apptype_db, (uns8 *)&tmp);
}

static void apptype_delete(AVD_APP_TYPE **apptype)
{
	unsigned int rc = ncs_patricia_tree_del(&apptype_db, &(*apptype)->tree_node);
	assert(rc == NCSCC_RC_SUCCESS);
	free((*apptype)->sgAmfApptSGTypes);
	free(*apptype);
	*apptype = NULL;
}

static void apptype_add_to_model(AVD_APP_TYPE *app_type)
{
	unsigned int rc;

	assert(app_type != NULL);
	TRACE("'%s'", app_type->name.value);
	rc = ncs_patricia_tree_add(&apptype_db, &app_type->tree_node);
	assert(rc == NCSCC_RC_SUCCESS);
}

static int is_config_valid(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes,
	const CcbUtilOperationData_t *opdata)
{
	int i = 0, j;
	char *parent;
	AVD_AMF_SG_TYPE *sg_type;
	const SaImmAttrValuesT_2 *attr;

	if ((parent = strchr((char*)dn->value, ',')) == NULL) {
		LOG_ER("No parent to '%s' ", dn->value);
		return 0;
	}

	/* AppType should be children to AppBaseType */
	if (strncmp(++parent, "safAppType=", 11) != 0) {
		LOG_ER("Wrong parent '%s' to '%s' ", parent, dn->value);
		return 0;
	}

	while ((attr = attributes[i++]) != NULL)
		if (!strcmp(attr->attrName, "saAmfApptSGTypes"))
			break;

	assert(attr);
	assert(attr->attrValuesNumber > 0);

	for (j = 0; j < attr->attrValuesNumber; j++) {
		SaNameT *name = (SaNameT *)attr->attrValues[j];
		sg_type = avd_sgtype_get(name);
		if (sg_type == NULL) {
			if (opdata == NULL) {
				LOG_ER("'%s' does not exist in model", name->value);
				return 0;
			}
			
			/* SG type does not exist in current model, check CCB */
			if (ccbutil_getCcbOpDataByDN(opdata->ccbId, name) == NULL) {
				LOG_ER("'%s' does not exist either in model or CCB", name->value);
				return 0;
			}
		}
	}

	return 1;
}

static AVD_APP_TYPE *apptype_create(SaNameT *dn, const SaImmAttrValuesT_2 **attributes)
{
	int i = 0, j;
	int rc = -1;
	AVD_APP_TYPE *app_type;
	const SaImmAttrValuesT_2 *attr;

	TRACE_ENTER2("'%s'", dn->value);

	if ((app_type = calloc(1, sizeof(AVD_APP_TYPE))) == NULL) {
		LOG_ER("calloc FAILED");
		goto done;
	}

	memcpy(app_type->name.value, dn->value, dn->length);
	app_type->name.length = dn->length;
	app_type->tree_node.key_info = (uns8 *)&(app_type->name);

	while ((attr = attributes[i++]) != NULL)
		if (!strcmp(attr->attrName, "saAmfApptSGTypes"))
			break;

	assert(attr);
	assert(attr->attrValuesNumber > 0);

	app_type->no_sg_types = attr->attrValuesNumber;
	app_type->sgAmfApptSGTypes = malloc(attr->attrValuesNumber * sizeof(SaNameT));
	for (j = 0; j < attr->attrValuesNumber; j++) {
		app_type->sgAmfApptSGTypes[j] = *((SaNameT *)attr->attrValues[j]);
	}

	rc = 0;

 done:
	if (rc != 0)
		apptype_delete(&app_type);

	return app_type;
}

static SaAisErrorT apptype_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	AVD_APP_TYPE *app_type;
	AVD_APP *app;
	SaBoolT app_exist = SA_FALSE;
	CcbUtilOperationData_t *t_opData;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(&opdata->objectName, opdata->param.create.attrValues, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		LOG_ER("Modification of SaAmfAppType not supported");
		break;
	case CCBUTIL_DELETE:
		app_type = avd_apptype_get(&opdata->objectName);
		if (NULL != app_type->list_of_app) {
			/* check whether there exists a delete operation for 
			 * each of the App in the app_type list in the current CCB
			 */
			app = app_type->list_of_app;
			while (app != NULL) {
				t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, &app->name);
				if ((t_opData == NULL) || (t_opData->operationType != CCBUTIL_DELETE)) {
					app_exist = SA_TRUE;
					break;
				}
				app = app->app_type_list_app_next;
			}
			if (app_exist == SA_TRUE) {
				LOG_ER("SaAmfAppType '%s' is in use", app_type->name.value);
				goto done;
			}
		}
		opdata->userData = app_type;	/* Save for later use in apply */
		rc = SA_AIS_OK;
		break;
	default:
		assert(0);
		break;
	}
done:
	return rc;
}

static void apptype_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_APP_TYPE *app_type;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		app_type = apptype_create(&opdata->objectName, opdata->param.create.attrValues);
		assert(app_type);
		apptype_add_to_model(app_type);
		break;
	case CCBUTIL_DELETE:
		app_type = opdata->userData;
		apptype_delete(&app_type);
		break;
	default:
		assert(0);
			break;
	}
}

SaAisErrorT avd_apptype_config_get(void)
{
	SaAisErrorT rc = SA_AIS_ERR_INVALID_PARAM, error;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfAppType";
	AVD_APP_TYPE *app_type;

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, NULL, SA_IMM_SUBTREE,
			SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
			NULL, &searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("No AMF app types found");
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		if (!is_config_valid(&dn, attributes, NULL))
			goto done2;

		if ((app_type = avd_apptype_get(&dn)) == NULL ) {
			if ((app_type = apptype_create(&dn, attributes)) == NULL)
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
	AVD_APP *i_app = NULL;
	AVD_APP *prev_app = NULL;

	if (app->app_type != NULL) {
		i_app = app->app_type->list_of_app;

		while ((i_app != NULL) && (i_app != app)) {
			prev_app = i_app;
			i_app = i_app->app_type_list_app_next;
		}

		if (i_app != app) {
			/* Log a fatal error */
		} else {
			if (prev_app == NULL) {
				app->app_type->list_of_app = app->app_type_list_app_next;
			} else {
				prev_app->app_type_list_app_next = app->app_type_list_app_next;
			}
		}

		app->app_type_list_app_next = NULL;
		app->app_type = NULL;
	}
}

void avd_apptype_constructor(void)
{
	NCS_PATRICIA_PARAMS patricia_params;

	patricia_params.key_size = sizeof(SaNameT);
	assert(ncs_patricia_tree_init(&apptype_db, &patricia_params) == NCSCC_RC_SUCCESS);

	avd_class_impl_set("SaAmfAppBaseType", NULL, NULL, avd_imm_default_OK_completed_cb, NULL);
	avd_class_impl_set("SaAmfAppType", NULL, NULL, apptype_ccb_completed_cb, apptype_ccb_apply_cb);
}

