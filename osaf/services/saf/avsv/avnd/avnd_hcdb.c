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
 *
 */

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:
  
  This module deals with the creation, accessing and deletion of the health 
  check database on the AVND.

..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "avnd.h"
#include <saImmOm.h>
#include <immutil.h>

static NCS_PATRICIA_TREE hctypedb;	/* healthcheck type db */

/****************************************************************************
  Name          : avnd_hcdb_init
 
  Description   : This routine initializes the healthcheck database.
 
  Arguments     : cb  - ptr to the AvND control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
void avnd_hcdb_init(AVND_CB *cb)
{
	NCS_PATRICIA_PARAMS params;
	uint32_t rc;

	memset(&params, 0, sizeof(NCS_PATRICIA_PARAMS));
	params.key_size = sizeof(SaNameT);

	rc = ncs_patricia_tree_init(&cb->hcdb, &params);
	osafassert(rc == NCSCC_RC_SUCCESS);
	rc = ncs_patricia_tree_init(&hctypedb, &params);
	osafassert(rc == NCSCC_RC_SUCCESS);
}

/****************************************************************************
  Name          : avnd_hcdb_destroy
 
  Description   : This routine destroys the healthcheck database. It deletes 
                  all the healthcheck records in the database.
 
  Arguments     : cb  - ptr to the AvND control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_hcdb_destroy(AVND_CB *cb)
{
	AVND_HC *hc = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* scan & delete each healthcheck record */
	while (0 != (hc = (AVND_HC *)ncs_patricia_tree_getnext(&cb->hcdb, (uint8_t *)0))) {
		/*AvND is going down, but don't send any async update even for 
		   external components, otherwise external components will be deleted
		   from ACT. */
		rc = avnd_hcdb_rec_del(cb, &hc->key);
		if (NCSCC_RC_SUCCESS != rc)
			goto err;
	}

	/* finally destroy patricia tree */
	rc = ncs_patricia_tree_destroy(&cb->hcdb);
	if (NCSCC_RC_SUCCESS != rc)
		goto err;

	TRACE("HC DB destroy success");
	return rc;

 err:
	LOG_ER("HC DB destroy failed");
	return rc;
}

AVND_HC *avnd_hcdb_rec_get(AVND_CB *cb, AVSV_HLT_KEY *hc_key)
{
	SaNameT dn;

	memset(&dn, 0, sizeof(dn));
	dn.length = snprintf((char *)dn.value, SA_MAX_NAME_LENGTH, "safHealthcheckKey=%s,%s",
		hc_key->name.key, hc_key->comp_name.value);
	return (AVND_HC *)ncs_patricia_tree_get(&cb->hcdb, (uint8_t *)&dn);
}

AVND_HCTYPE *avnd_hctypedb_rec_get(const SaNameT *comp_type_dn, const SaAmfHealthcheckKeyT *key)
{
	SaNameT dn;

	memset(&dn, 0, sizeof(dn));
	dn.length = snprintf((char *)dn.value, SA_MAX_NAME_LENGTH, "safHealthcheckKey=%s,%s",
			     key->key, comp_type_dn->value);
	return (AVND_HCTYPE *)ncs_patricia_tree_get(&hctypedb, (uint8_t *)&dn);
}

/****************************************************************************
  Name          : avnd_hcdb_rec_add
 
  Description   : This routine adds a healthcheck record to the healthcheck 
                  database. If a healthcheck record is already present, 
                  nothing is done.
 
  Arguments     : cb   - ptr to the AvND control block
                  info - ptr to the healthcheck params
                  rc   - ptr to the operation result
 
  Return Values : ptr to the healthcheck record, if success
                  0, otherwise
 
  Notes         : None.
******************************************************************************/
AVND_HC *avnd_hcdb_rec_add(AVND_CB *cb, AVND_HC_PARAM *info, uint32_t *rc)
{
	AVND_HC *hc = 0;

	*rc = NCSCC_RC_SUCCESS;

	/* verify if this healthcheck is already present in the db */
	if (0 != avnd_hcdb_rec_get(cb, &info->name)) {
		*rc = AVND_ERR_DUP_HC;
		goto err;
	}

	/* a fresh healthcheck record... */
	hc = calloc(1, sizeof(AVND_HC));
	if (!hc) {
		*rc = AVND_ERR_NO_MEMORY;
		goto err;
	}

	/* Update the config parameters */
	memcpy(&hc->key, &info->name, sizeof(AVSV_HLT_KEY));
	hc->period = info->period;
	hc->max_dur = info->max_duration;
	hc->is_ext = info->is_ext;

	/* Add to the patricia tree */
	hc->tree_node.bit = 0;
	hc->tree_node.key_info = (uint8_t *)&hc->key;
	*rc = ncs_patricia_tree_add(&cb->hcdb, &hc->tree_node);
	if (NCSCC_RC_SUCCESS != *rc) {
		*rc = AVND_ERR_TREE;
		goto err;
	}

	TRACE("HC DB rec add: %s success",info->name.name.key);
	return hc;

 err:
	if (hc)
		free(hc);

	LOG_CR("HC DB rec add: %s failed",info->name.name.key);
	return 0;
}

/****************************************************************************
  Name          : avnd_hcdb_rec_del
 
  Description   : This routine deletes a healthcheck record from the 
                  healthcheck database. 
 
  Arguments     : cb     - ptr to the AvND control block
                  hc_key - ptr to the healthcheck key
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_hcdb_rec_del(AVND_CB *cb, AVSV_HLT_KEY *hc_key)
{
	AVND_HC *hc = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* get the healthcheck record */
	hc = avnd_hcdb_rec_get(cb, hc_key);
	if (!hc) {
		rc = AVND_ERR_NO_HC;
		goto err;
	}

	/* remove from the patricia tree */
	rc = ncs_patricia_tree_del(&cb->hcdb, &hc->tree_node);
	if (NCSCC_RC_SUCCESS != rc) {
		rc = AVND_ERR_TREE;
		goto err;
	}

	TRACE("HC DB rec:%s delete success",hc_key->name.key);

	/* free the memory */
	free(hc);

	return rc;

 err:
	LOG_CR("HC DB rec:%s delete failed", hc_key->name.key);
	return rc;
}

static AVND_HC *hc_create(AVND_CB *cb, SaNameT *dn, const SaImmAttrValuesT_2 **attributes)
{
	int rc = -1;
	AVND_HC *hc = NULL;
  
	if ((hc = calloc(1, sizeof(AVND_HC))) == NULL)
		goto done;

	if (immutil_getAttr("saAmfHealthcheckPeriod", attributes, 0, &hc->period) != SA_AIS_OK) {
		LOG_ER("Get saAmfHealthcheckPeriod FAILED for '%s'", dn->value);
		goto done;
	}

	if (immutil_getAttr("saAmfHealthcheckMaxDuration", attributes, 0, &hc->max_dur) != SA_AIS_OK) {
		LOG_ER("Get saAmfHealthcheckMaxDuration FAILED for '%s'", dn->value);
		goto done;
	}

	memset(&hc->name, 0, sizeof(hc->name));
	memcpy(hc->name.value, dn->value, dn->length);
	hc->name.length = dn->length;
	hc->tree_node.key_info = (uint8_t *)&hc->name;
	rc = ncs_patricia_tree_add(&cb->hcdb, &hc->tree_node);

 done:
	if (rc != NCSCC_RC_SUCCESS) {
		free(hc);
		hc = NULL;
	}

	return hc;
}

SaAisErrorT avnd_hc_config_get(AVND_COMP *comp)
{
	SaAisErrorT error = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT hc_name;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfHealthcheck";
	SaNameT comp_dn = comp->name;
	comp_dn.length = comp_dn.length;
	SaImmHandleT immOmHandle;
	SaVersionT immVersion = { 'A', 2, 1 };

	immutil_saImmOmInitialize(&immOmHandle, NULL, &immVersion);

	avnd_hctype_config_get(immOmHandle, &comp->saAmfCompType);

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_2(immOmHandle, &comp_dn,
		SA_IMM_SUBTREE, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR,
		&searchParam, NULL, &searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("saImmOmSearchInitialize_2 failed: %u", error);
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &hc_name, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {

		TRACE_1("'%s'", hc_name.value);

		if (hc_create(avnd_cb, &hc_name, attributes) == NULL)
			goto done2;
	}

	error = SA_AIS_OK;

 done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
 done1:
	immutil_saImmOmFinalize(immOmHandle);

	return error;
}

static AVND_HCTYPE *hctype_create(AVND_CB *cb, SaNameT *dn, const SaImmAttrValuesT_2 **attributes)
{
	int rc = -1;
	AVND_HCTYPE *hc;

	if ((hc = calloc(1, sizeof(*hc))) == NULL)
		goto done;

	hc->name = *dn;

	if (immutil_getAttr("saAmfHctDefPeriod", attributes, 0, &hc->saAmfHctDefPeriod) != SA_AIS_OK) {
		LOG_ER("Get saAmfHctDefPeriod FAILED for '%s'", dn->value);
		goto done;
	}

	if (immutil_getAttr("saAmfHctDefMaxDuration", attributes, 0, &hc->saAmfHctDefMaxDuration) != SA_AIS_OK) {
		LOG_ER("Get saAmfHctDefMaxDuration FAILED for '%s'", dn->value);
		goto done;
	}

	hc->tree_node.key_info = (uint8_t *)&hc->name;
	rc = ncs_patricia_tree_add(&hctypedb, &hc->tree_node);

 done:
	if (rc != NCSCC_RC_SUCCESS) {
		free(hc);
		hc = NULL;
	}

	return hc;
}

SaAisErrorT avnd_hctype_config_get(SaImmHandleT immOmHandle, const SaNameT *comptype_dn)
{
	SaAisErrorT error = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT hc_name;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfHealthcheckType";

	TRACE_ENTER2("'%s'", comptype_dn->value);

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_2(immOmHandle, comptype_dn,
		SA_IMM_SUBTREE, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR,
		&searchParam, NULL, &searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("saImmOmSearchInitialize_2 failed: %u", error);
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &hc_name, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {

		TRACE_1("'%s'", hc_name.value);

		if (hctype_create(avnd_cb, &hc_name, attributes) == NULL)
			goto done2;
	}

	error = SA_AIS_OK;

 done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
 done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

uint32_t avnd_hc_oper_req(AVND_CB *cb, AVSV_PARAM_INFO *param)
{
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s'", param->name.value);
	
	AVND_HC *hc = (AVND_HC *)ncs_patricia_tree_get(&cb->hcdb, (uint8_t *)&param->name);

	switch (param->act) {
	case AVSV_OBJ_OPR_MOD: {
		if (!hc) {
			LOG_ER("%s: failed to get %s", __FUNCTION__, param->name.value);
			goto done;
		}

		switch (param->attr_id) {
		case saAmfHealthcheckPeriod_ID:
			osafassert(sizeof(SaTimeT) == param->value_len);
			hc->period = *((SaTimeT *)param->value);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, hc, AVND_CKPT_HC_PERIOD);
			break;

		case saAmfHealthcheckMaxDuration_ID:
			osafassert(sizeof(SaTimeT) == param->value_len);
			hc->max_dur = *((SaTimeT *)param->value);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, hc, AVND_CKPT_HC_MAX_DUR);
			break;

		default:
			LOG_NO("%s: Unsupported attribute %u", __FUNCTION__, param->attr_id);
			goto done;
		}
		break;
	}

	case AVSV_OBJ_OPR_DEL: {
		if (hc != NULL) {
			rc = ncs_patricia_tree_del(&cb->hcdb, &hc->tree_node);
			osafassert(rc == NCSCC_RC_SUCCESS);
			LOG_IN("Deleted '%s'", param->name.value);
		} else {
			/* 
			** Normal case that happens if a parent of this HC was
			** the real delete target for the CCB.
			*/
			TRACE("already deleted!");
		}

		break;
	}
	default:
		LOG_NO("%s: Unsupported action %u", __FUNCTION__, param->act);
		goto done;
	}

	rc = NCSCC_RC_SUCCESS;

done:
	TRACE_LEAVE();
	return rc;
}

