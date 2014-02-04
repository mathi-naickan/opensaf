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

#include <si.h>
#include <saImmOm.h>
#include <immutil.h>
#include <app.h>
#include <cluster.h>
#include <imm.h>
#include <susi.h>
#include <csi.h>
#include <proc.h>

static NCS_PATRICIA_TREE svctype_db;

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
	unsigned int rc = ncs_patricia_tree_add(&svctype_db, &svct->tree_node);
	osafassert (rc == NCSCC_RC_SUCCESS);
}

AVD_SVC_TYPE *avd_svctype_get(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_SVC_TYPE *)ncs_patricia_tree_get(&svctype_db, (uint8_t *)&tmp);
}

static void svctype_delete(AVD_SVC_TYPE *svc_type)
{
	unsigned int rc = ncs_patricia_tree_del(&svctype_db, &svc_type->tree_node);
	osafassert(rc == NCSCC_RC_SUCCESS);

	if (svc_type->saAmfSvcDefActiveWeight != NULL) {
		unsigned int i = 0;
		while (svc_type->saAmfSvcDefActiveWeight[i] != NULL) {
			delete [] svc_type->saAmfSvcDefActiveWeight[i];
			++i;
		}
		delete [] svc_type->saAmfSvcDefActiveWeight;
	}

	if (svc_type->saAmfSvcDefStandbyWeight != NULL) {
		unsigned int i = 0;
		while (svc_type->saAmfSvcDefStandbyWeight[i] != NULL) {
			delete [] svc_type->saAmfSvcDefStandbyWeight[i];
			++i;
		}
		delete [] svc_type->saAmfSvcDefStandbyWeight;
	}
	delete svc_type;
}

static AVD_SVC_TYPE *svctype_create(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes)
{
	unsigned int i;
	AVD_SVC_TYPE *svct;
	SaUint32T attrValuesNumber;

	TRACE_ENTER2("'%s'", dn->value);

	svct = new AVD_SVC_TYPE();

	memcpy(svct->name.value, dn->value, dn->length);
	svct->name.length = dn->length;
	svct->tree_node.key_info = (uint8_t *)&svct->name;
	svct->saAmfSvcDefActiveWeight = NULL;
	svct->saAmfSvcDefStandbyWeight = NULL;

	/* Optional, [0..*] */
	if (immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("saAmfSvcDefActiveWeight"), attributes, &attrValuesNumber) == SA_AIS_OK) {
		svct->saAmfSvcDefActiveWeight = new char*[attrValuesNumber + 1];
		for (i = 0; i < attrValuesNumber; i++) {
			svct->saAmfSvcDefActiveWeight[i] =
			    StrDup(immutil_getStringAttr(attributes, "saAmfSvcDefActiveWeight", i));
		}
		svct->saAmfSvcDefActiveWeight[i] = NULL;
	}

	/* Optional, [0..*] */
	if (immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("saAmfSvcDefStandbyWeight"), attributes, &attrValuesNumber) == SA_AIS_OK) {
		svct->saAmfSvcDefStandbyWeight = new char*[attrValuesNumber + 1];
		for (i = 0; i < attrValuesNumber; i++) {
			svct->saAmfSvcDefStandbyWeight[i] =
			    StrDup(immutil_getStringAttr(attributes, "saAmfSvcDefStandbyWeight", i));
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
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn->value);
		return 0;
	}

	/* Should be children to the SvcBasetype */
	if (strncmp(++parent, "safSvcType=", 11) != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", parent, dn->value);
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
		report_ccb_validation_error(opdata, "Modification of SaAmfSvcType not supported");
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
				report_ccb_validation_error(opdata, "SaAmfSvcType '%s' is in use",svc_type->name.value);
				goto done;
			}
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

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		svc_type = svctype_create(&opdata->objectName, opdata->param.create.attrValues);
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
			osafassert(0);
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

	osafassert(ncs_patricia_tree_init(&svctype_db, &patricia_params) == NCSCC_RC_SUCCESS);

	avd_class_impl_set(const_cast<SaImmClassNameT>("SaAmfSvcType"), NULL, NULL, svctype_ccb_completed_cb, svctype_ccb_apply_cb);
	avd_class_impl_set(const_cast<SaImmClassNameT>("SaAmfSvcBaseType"), NULL, NULL, avd_imm_default_OK_completed_cb, NULL);
}

