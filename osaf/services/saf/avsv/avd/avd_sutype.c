/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2009 The OpenSAF Foundation
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

#include <saImmOm.h>
#include <immutil.h>
#include <logtrace.h>

#include <avd_util.h>
#include <avd_sutype.h>
#include <avd_imm.h>
#include <avd_cluster.h>
#include <avd_ntf.h>
#include <avd_proc.h>

static NCS_PATRICIA_TREE sutype_db;

static struct avd_sutype *sutype_new(const SaNameT *dn)
{
	struct avd_sutype *sutype = calloc(1, sizeof(struct avd_sutype));

	if (sutype == NULL) {
		LOG_ER("sutype_new: calloc FAILED");
		return NULL;
	}

	memcpy(sutype->name.value, dn->value, dn->length);
	sutype->name.length = dn->length;
	sutype->tree_node.key_info = (uns8 *)&(sutype->name);

	return sutype;
}

static void sutype_delete(struct avd_sutype **sutype)
{
	assert(NULL == (*sutype)->list_of_su);
	free((*sutype)->saAmfSutProvidesSvcTypes);
	free(*sutype);
	*sutype = NULL;
}

static void sutype_db_add(struct avd_sutype *sutype)
{
	unsigned int rc = ncs_patricia_tree_add(&sutype_db, &sutype->tree_node);
	assert(rc == NCSCC_RC_SUCCESS);
}

static void sutype_db_delete(struct avd_sutype *sutype)
{
	unsigned int rc = ncs_patricia_tree_del(&sutype_db, &sutype->tree_node);
	assert(rc == NCSCC_RC_SUCCESS);
}

struct avd_sutype *avd_sutype_get(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (struct avd_sutype *)ncs_patricia_tree_get(&sutype_db, (uns8 *)&tmp);
}

static struct avd_sutype *sutype_create(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes)
{
	const SaImmAttrValuesT_2 *attr;
	struct avd_sutype *sutype;
	int rc, i = 0;
	SaAisErrorT error;

	TRACE_ENTER2("'%s'", dn->value);

	if ((sutype = sutype_new(dn)) == NULL) {
		LOG_ER("avd_sutype_new failed");
		return NULL;
	}

	error = immutil_getAttr("saAmfSutIsExternal", attributes, 0, &sutype->saAmfSutIsExternal);
	assert(error == SA_AIS_OK);

	error = immutil_getAttr("saAmfSutDefSUFailover", attributes, 0, &sutype->saAmfSutDefSUFailover);
	assert(error == SA_AIS_OK);

	while ((attr = attributes[i++]) != NULL)
		if (!strcmp(attr->attrName, "saAmfSutProvidesSvcTypes"))
			break;

	assert(attr);
	assert(attr->attrValuesNumber > 0);

	sutype->number_svc_types = attr->attrValuesNumber;
	sutype->saAmfSutProvidesSvcTypes = malloc(sutype->number_svc_types * sizeof(SaNameT));

	for (i = 0; i < sutype->number_svc_types; i++) {
		if (immutil_getAttr("saAmfSutProvidesSvcTypes", attributes, i, &sutype->saAmfSutProvidesSvcTypes[i]) != SA_AIS_OK) {
			LOG_ER("Get saAmfSutProvidesSvcTypes FAILED for '%s'", dn->value);
			assert(0);
		}
		TRACE("%s", sutype->saAmfSutProvidesSvcTypes[i].value);
	}

	rc = 0;

	if (rc != 0)
		sutype_delete(&sutype);

	return sutype;
}

static int is_config_valid(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;
	SaBoolT abool;
	/* int i = 0; */
	char *parent;

	if ((parent = strchr((char*)dn->value, ',')) == NULL) {
		LOG_ER("No parent to '%s' ", dn->value);
		return 0;
	}

	parent++;

	/* Should be children to the SU Base type */
	if (strncmp(parent, "safSuType=", 10) != 0) {
		LOG_ER("Wrong parent '%s' to '%s' ", parent, dn->value);
		return 0;
	}
#if 0
	/*  TODO we need proper Svc Type handling for this, revisit */

	/* Make sure all Svc types exist */
	for (i = 0; i < su_type->number_svc_types; i++) {
		AVD_AMF_SVC_TYPE *svc_type =
		    avd_svctype_find(avd_cb, su_type->saAmfSutProvidesSvcTypes[i], TRUE);
		if (svc_type == NULL) {
			/* Svc type does not exist in current model, check CCB */
			if ((opdata != NULL) &&
			    (ccbutil_getCcbOpDataByDN(opdata->ccbId, &su_type->saAmfSutProvidesSvcTypes[i]) == NULL)) {
				LOG_ER("Svc type '%s' does not exist either in model or CCB",
					su_type->saAmfSutProvidesSvcTypes[i]);
				return SA_AIS_ERR_BAD_OPERATION;
			}
			LOG_ER("AMF Svc type '%s' does not exist",
				su_type->saAmfSutProvidesSvcTypes[i].value);
			return -1;
		}
	}
#endif
	rc = immutil_getAttr("saAmfSutDefSUFailover", attributes, 0, &abool);
	assert(rc == SA_AIS_OK);

	if ((immutil_getAttr("saAmfSutDefSUFailover", attributes, 0, &abool) == SA_AIS_OK) &&
	    (abool > SA_TRUE)) {
		LOG_ER("Invalid saAmfSutDefSUFailover %u for '%s'", abool, dn->value);
		return 0;
	}

	rc = immutil_getAttr("saAmfSutIsExternal", attributes, 0, &abool);
	assert(rc == SA_AIS_OK);

	if ((immutil_getAttr("saAmfSutIsExternal", attributes, 0, &abool) == SA_AIS_OK) &&
	    (abool > SA_TRUE)) {
		LOG_ER("Invalid saAmfSutIsExternal %u for '%s'", abool, dn->value);
		return 0;
	}

	return 1;
}

SaAisErrorT avd_sutype_config_get(void)
{
	struct avd_sutype *sut;
	SaAisErrorT error;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfSUType";

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, NULL, SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
		NULL, &searchHandle);
	
	if (SA_AIS_OK != error) {
		LOG_ER("saImmOmSearchInitialize_2 failed: %u", error);
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		if (!is_config_valid(&dn, attributes, NULL))
		    goto done2;

		if (( sut = avd_sutype_get(&dn)) == NULL) {

			if ((sut = sutype_create(&dn, attributes)) == NULL) {
				error = SA_AIS_ERR_FAILED_OPERATION;
				goto done2;
			}

			sutype_db_add(sut);
		}

		if (avd_sutcomptype_config_get(&dn, sut) != SA_AIS_OK) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}
	}

	error = SA_AIS_OK;

done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

static void sutype_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	struct avd_sutype *sut;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		sut = sutype_create(&opdata->objectName, opdata->param.create.attrValues);
		assert(sut);
		sutype_db_add(sut);
		break;
	case CCBUTIL_DELETE:
		sut = avd_sutype_get(&opdata->objectName);
		sutype_db_delete(sut);
		sutype_delete(&sut);
		break;
	default:
		assert(0);
		break;
	}

	TRACE_LEAVE();
}

static SaAisErrorT sutype_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	struct avd_sutype *sut;
	AVD_SU *su; 
	SaBoolT su_exist = SA_FALSE;
	CcbUtilOperationData_t *t_opData;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(&opdata->objectName, opdata->param.create.attrValues, opdata))
		    rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		LOG_ER("Modification of SaAmfSUType not supported");
		break;
	case CCBUTIL_DELETE:
		sut = avd_sutype_get(&opdata->objectName);
		if (NULL != sut->list_of_su) {
			/* check whether there exists a delete operation for 
			 * each of the SU in the su_type list in the current CCB 
			 */                      
			su = sut->list_of_su;
			while (su != NULL) {  
				t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, &su->name);
				if ((t_opData == NULL) || (t_opData->operationType != CCBUTIL_DELETE)) {
					su_exist = SA_TRUE;   
					break;                  
				}                       
				su = su->su_list_su_type_next;
			}                       
			if (su_exist == SA_TRUE) {
				LOG_ER("SaAmfSUType '%s'is in use",sut->name.value);
				goto done;
			}
		}
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

	/* Add SU to list in SU Type */
void avd_sutype_add_su(AVD_SU* su)
{
	struct avd_sutype *sut = su->su_type;
	su->su_list_su_type_next = sut->list_of_su;
	sut->list_of_su = su;
}

void avd_sutype_remove_su(AVD_SU* su)
{
	AVD_SU *i_su = NULL;
	AVD_SU *prev_su = NULL;

	if (su->su_type != NULL) {
		i_su = su->su_type->list_of_su;

		while ((i_su != NULL) && (i_su != su)) {
			prev_su = i_su;
			i_su = i_su->su_list_su_type_next;
		}

		if (i_su == su) {
			if (prev_su == NULL) {
				su->su_type->list_of_su = su->su_list_su_type_next;
			} else {
				prev_su->su_list_su_type_next = su->su_list_su_type_next;
			}
			
			su->su_list_su_type_next = NULL;
			su->su_type = NULL;
		}
	}
}

void avd_sutype_constructor(void)
{
	NCS_PATRICIA_PARAMS patricia_params;

	patricia_params.key_size = sizeof(SaNameT);
	assert(ncs_patricia_tree_init(&sutype_db, &patricia_params) == NCSCC_RC_SUCCESS);

	avd_class_impl_set("SaAmfSUBaseType", NULL, NULL, avd_imm_default_OK_completed_cb, NULL);
	avd_class_impl_set("SaAmfSUType", NULL, NULL, sutype_ccb_completed_cb, sutype_ccb_apply_cb);
}
