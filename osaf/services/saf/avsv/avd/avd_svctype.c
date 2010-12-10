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

#include <avd_si.h>
#include <saImmOm.h>
#include <immutil.h>
#include <avd_app.h>
#include <avd_cluster.h>
#include <avd_imm.h>
#include <avd_susi.h>
#include <avd_csi.h>
#include <avd_proc.h>

static NCS_PATRICIA_TREE svctype_db;

static void svctype_db_add(AVD_SVC_TYPE *svct)
{
	unsigned int rc = ncs_patricia_tree_add(&svctype_db, &svct->tree_node);
	assert (rc == NCSCC_RC_SUCCESS);
}

AVD_SVC_TYPE *avd_svctype_get(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_SVC_TYPE *)ncs_patricia_tree_get(&svctype_db, (uns8 *)&tmp);
}

static void svctype_delete(AVD_SVC_TYPE *svc_type)
{
	unsigned int rc = ncs_patricia_tree_del(&svctype_db, &svc_type->tree_node);
	assert(rc == NCSCC_RC_SUCCESS);
	free(svc_type);
}

static AVD_SVC_TYPE *svctype_create(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes)
{
	int i;
	AVD_SVC_TYPE *svct;
	SaUint32T attrValuesNumber;

	TRACE_ENTER2("'%s'", dn->value);

	if ((svct = calloc(1, sizeof(AVD_SVC_TYPE))) == NULL) {
		LOG_ER("calloc FAILED");
		return NULL;
	}

	memcpy(svct->name.value, dn->value, dn->length);
	svct->name.length = dn->length;
	svct->tree_node.key_info = (uns8 *)&svct->name;

	/* Optional, [0..*] */
	if (immutil_getAttrValuesNumber("saAmfSvcDefActiveWeight", attributes, &attrValuesNumber) == SA_AIS_OK) {
		svct->saAmfSvcDefActiveWeight = malloc((attrValuesNumber + 1) * sizeof(char *));
		for (i = 0; i < attrValuesNumber; i++) {
			svct->saAmfSvcDefActiveWeight[i] =
			    strdup(immutil_getStringAttr(attributes, "saAmfSvcDefActiveWeight", i));
		}
		svct->saAmfSvcDefActiveWeight[i] = NULL;
	}

	/* Optional, [0..*] */
	if (immutil_getAttrValuesNumber("saAmfSvcDefStandbyWeight", attributes, &attrValuesNumber) == SA_AIS_OK) {
		svct->saAmfSvcDefStandbyWeight = malloc((attrValuesNumber + 1) * sizeof(char *));
		for (i = 0; i < attrValuesNumber; i++) {
			svct->saAmfSvcDefStandbyWeight[i] =
			    strdup(immutil_getStringAttr(attributes, "saAmfSvcDefStandbyWeight", i));
		}
		svct->saAmfSvcDefStandbyWeight[i] = NULL;
	}

	TRACE_LEAVE();
	return svct;
}

static int is_config_valid(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
	char *parent;

	if ((parent = strchr((char*)dn->value, ',')) == NULL) {
		LOG_ER("No parent to '%s' ", dn->value);
		return 0;
	}

	/* Should be children to the SvcBasetype */
	if (strncmp(++parent, "safSvcType=", 11) != 0) {
		LOG_ER("Wrong parent '%s' to '%s' ", parent, dn->value);
		return 0;
	}

	return 1;
}

static SaAisErrorT svctype_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	AVD_SVC_TYPE *svc_type;
	AVD_SI *si;
	SaBoolT si_exist = SA_FALSE;
	CcbUtilOperationData_t *t_opData;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(&opdata->objectName, opdata->param.create.attrValues, opdata))
		    rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		LOG_ER("Modification of SaAmfSvcType not supported");
		break;
	case CCBUTIL_DELETE:
		svc_type = avd_svctype_get(&opdata->objectName);
		if (NULL != svc_type->list_of_si) {
			/* check whether there exists a delete operation for
			 * each of the SI in the svc_type list in the current CCB
			 */
			si = svc_type->list_of_si;
			while (si != NULL) {
				t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, &si->name);
				if ((t_opData == NULL) || (t_opData->operationType != CCBUTIL_DELETE)) {
					si_exist = SA_TRUE;
					break;
				}
				si = si->si_list_svc_type_next;
			}
			if (si_exist == SA_TRUE) {
				LOG_ER("SaAmfSvcType '%s' is in use",svc_type->name.value);
				goto done;
			}
		}
		opdata->userData = svc_type;
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

static void svctype_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_SVC_TYPE *svc_type;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		svc_type = svctype_create(&opdata->objectName, opdata->param.create.attrValues);
		assert(svc_type);
		svctype_db_add(svc_type);
		break;
	case CCBUTIL_DELETE:
		svctype_delete(opdata->userData);
		break;
	default:
		assert(0);
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

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if (immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, NULL, SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
		NULL, &searchHandle) != SA_AIS_OK) {

		LOG_ER("No objects found (1)");
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		if (!is_config_valid(&dn, attributes, NULL))
			goto done2;

		if ((svc_type = avd_svctype_get(&dn))==NULL) {
			if ((svc_type = svctype_create(&dn, attributes)) == NULL)
				goto done2;

			svctype_db_add(svc_type);
		}

		if (avd_svctypecstypes_config_get(&dn) != SA_AIS_OK)
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
	si->si_list_svc_type_next = si->svc_type->list_of_si;
	si->svc_type->list_of_si = si;
}

void avd_svctype_remove_si(AVD_SI *si)
{
	AVD_SI *i_si = NULL;
	AVD_SI *prev_si = NULL;

	if (!si)
		return;

	if (si->svc_type != NULL) {
		i_si = si->svc_type->list_of_si;

		while ((i_si != NULL) && (i_si != si)) {
			prev_si = i_si;
			i_si = i_si->si_list_svc_type_next;
		}

		if (i_si != si) {
			assert(0);
		} else {
			if (prev_si == NULL) {
				si->svc_type->list_of_si = si->si_list_svc_type_next;
			} else {
				prev_si->si_list_svc_type_next = si->si_list_svc_type_next;
			}
		}

		si->si_list_svc_type_next = NULL;
		si->svc_type = NULL;
	}
}

void avd_svctype_constructor(void)
{
	NCS_PATRICIA_PARAMS patricia_params;

	patricia_params.key_size = sizeof(SaNameT);

	assert(ncs_patricia_tree_init(&svctype_db, &patricia_params) == NCSCC_RC_SUCCESS);

	avd_class_impl_set("SaAmfSvcType", NULL, NULL, svctype_ccb_completed_cb, svctype_ccb_apply_cb);
	avd_class_impl_set("SaAmfSvcBaseType", NULL, NULL, avd_imm_default_OK_completed_cb, NULL);
}

