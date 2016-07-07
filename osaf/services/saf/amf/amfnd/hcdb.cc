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
#include <string>
#include "osaf_utility.h"

static NCS_PATRICIA_TREE hctypedb;	/* healthcheck type db */
static pthread_mutex_t hcdb_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t hctypedb_mutex = PTHREAD_MUTEX_INITIALIZER;

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

AVND_HC *avnd_hcdb_rec_get(AVND_CB *cb, AVSV_HLT_KEY *hc_key)
{
	SaNameT dn;

	memset(&dn, 0, sizeof(dn));
	dn.length = snprintf((char *)dn.value, SA_MAX_NAME_LENGTH, "safHealthcheckKey=%s,%s",
		hc_key->name.key, hc_key->comp_name.value);
	osaf_mutex_lock_ordie(&hcdb_mutex);
	AVND_HC *hc = (AVND_HC *)ncs_patricia_tree_get(&cb->hcdb, (uint8_t *)&dn);
	osaf_mutex_unlock_ordie(&hcdb_mutex);
	return hc;
}

AVND_HCTYPE *avnd_hctypedb_rec_get(const SaNameT *comp_type_dn, const SaAmfHealthcheckKeyT *key)
{
	SaNameT dn;

	memset(&dn, 0, sizeof(dn));
	dn.length = snprintf((char *)dn.value, SA_MAX_NAME_LENGTH, "safHealthcheckKey=%s,%s",
			     key->key, comp_type_dn->value);
	osaf_mutex_lock_ordie(&hctypedb_mutex);
	AVND_HCTYPE *hctype = (AVND_HCTYPE *)ncs_patricia_tree_get(&hctypedb, (uint8_t *)&dn);
	osaf_mutex_unlock_ordie(&hctypedb_mutex);
	return hctype;
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
	TRACE_ENTER();

	*rc = NCSCC_RC_SUCCESS;

	/* verify if this healthcheck is already present in the db */
	if (0 != avnd_hcdb_rec_get(cb, &info->name)) {
		*rc = AVND_ERR_DUP_HC;
		goto err;
	}

	/* a fresh healthcheck record... */
	hc = new AVND_HC();

	/* Update the config parameters */
	memcpy(&hc->key, &info->name, sizeof(AVSV_HLT_KEY));
	hc->period = info->period;
	hc->max_dur = info->max_duration;
	hc->is_ext = info->is_ext;

	/* Add to the patricia tree */
	hc->tree_node.bit = 0;
	hc->tree_node.key_info = (uint8_t *)&hc->key;
	osaf_mutex_lock_ordie(&hcdb_mutex);
	*rc = ncs_patricia_tree_add(&cb->hcdb, &hc->tree_node);
	osaf_mutex_unlock_ordie(&hcdb_mutex);
	if (NCSCC_RC_SUCCESS != *rc) {
		*rc = AVND_ERR_TREE;
		goto err;
	}

	TRACE("HC DB rec add: %s success",info->name.name.key);
	return hc;

 err:
	if (hc)
		delete hc;

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
	TRACE_ENTER2();

	/* get the healthcheck record */
	hc = avnd_hcdb_rec_get(cb, hc_key);
	if (!hc) {
		rc = AVND_ERR_NO_HC;
		goto err;
	}

	/* remove from the patricia tree */
	osaf_mutex_lock_ordie(&hcdb_mutex);
	rc = ncs_patricia_tree_del(&cb->hcdb, &hc->tree_node);
	osaf_mutex_unlock_ordie(&hcdb_mutex);
	if (NCSCC_RC_SUCCESS != rc) {
		rc = AVND_ERR_TREE;
		goto err;
	}

	TRACE("HC DB rec:%s delete success",hc_key->name.key);

	/* free the memory */
	delete hc;

	return rc;

 err:
	LOG_CR("HC DB rec:%s delete failed", hc_key->name.key);
	return rc;
}

static AVND_HC *hc_create(AVND_CB *cb, SaNameT *dn, const SaImmAttrValuesT_2 **attributes)
{
	int rc = -1;
	AVND_HC *hc = NULL;
  
	hc = new AVND_HC();

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfHealthcheckPeriod"), attributes, 0, &hc->period) != SA_AIS_OK) {
		LOG_ER("Get saAmfHealthcheckPeriod FAILED for '%s'", dn->value);
		goto done;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfHealthcheckMaxDuration"), attributes, 0, &hc->max_dur) != SA_AIS_OK) {
		LOG_ER("Get saAmfHealthcheckMaxDuration FAILED for '%s'", dn->value);
		goto done;
	}

	memset(&hc->name, 0, sizeof(hc->name));
	memcpy(hc->name.value, dn->value, dn->length);
	hc->name.length = dn->length;
	hc->tree_node.key_info = (uint8_t *)&hc->name;
	osaf_mutex_lock_ordie(&hcdb_mutex);
	rc = ncs_patricia_tree_add(&cb->hcdb, &hc->tree_node);
	osaf_mutex_unlock_ordie(&hcdb_mutex);

 done:
	if (rc != NCSCC_RC_SUCCESS) {
		delete hc;
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

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
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

	hc = new AVND_HCTYPE();

	hc->name = *dn;

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfHctDefPeriod"), attributes, 0, &hc->saAmfHctDefPeriod) != SA_AIS_OK) {
		LOG_ER("Get saAmfHctDefPeriod FAILED for '%s'", dn->value);
		goto done;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfHctDefMaxDuration"), attributes, 0, &hc->saAmfHctDefMaxDuration) != SA_AIS_OK) {
		LOG_ER("Get saAmfHctDefMaxDuration FAILED for '%s'", dn->value);
		goto done;
	}

	hc->tree_node.key_info = (uint8_t *)&hc->name;
	osaf_mutex_lock_ordie(&hctypedb_mutex);
	rc = ncs_patricia_tree_add(&hctypedb, &hc->tree_node);
	osaf_mutex_unlock_ordie(&hctypedb_mutex);

 done:
	if (rc != NCSCC_RC_SUCCESS) {
		delete hc;
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

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
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
		//A record may get created in the context of some other component of same comptype.
		AVND_HCTYPE *hctype = NULL;
		osaf_mutex_lock_ordie(&hctypedb_mutex);
		if ((hctype = (AVND_HCTYPE *)ncs_patricia_tree_get(&hctypedb,
						(uint8_t *)&hc_name)) == NULL) {
			osaf_mutex_unlock_ordie(&hctypedb_mutex);
			if (hctype_create(avnd_cb, &hc_name, attributes) == NULL)
				goto done2;
		}
		else {
			TRACE_2("Record already exists");
			osaf_mutex_unlock_ordie(&hctypedb_mutex);
		}
	}

	error = SA_AIS_OK;

 done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
 done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

// Search for a given key and return its value  or empty string if not found. 
static std::string search_key(const std::string& str, const std::string& key)
{
	std::string value;

	std::string::size_type idx1;

	idx1 = str.find(key);

	// if key was found
	if (idx1 != std::string::npos) {
		// get value 
		idx1 += key.length();
		std::string part2 = str.substr(idx1);

		idx1 = part2.find(",");

		// get value
		value = part2.substr(0, idx1);
	}

	return value;
}

static void comp_hctype_update_compdb(AVND_CB *cb, AVSV_PARAM_INFO *param)
{
	AVND_COMP_HC_REC *comp_hc_rec;
	AVND_COMP * comp;
	const char *comp_type_name;
	AVSV_HLT_KEY hlt_chk;
	AVND_COMP_HC_REC tmp_hc_rec;
		
	// 1. find component from componentType, 
	// input example, param->name.value = safHealthcheckKey=AmfDemo,safVersion=1,safCompType=AmfDemo1	
	comp_type_name = strstr((char *)param->name.value, "safVersion");
	TRACE("comp_type_name: %s", comp_type_name);
	osafassert(comp_type_name);
	
	// 2. search each component for a matching compType
	comp = (AVND_COMP *)compdb_rec_get_next(&cb->compdb, (uint8_t *)0);
	while (comp != 0) {
		if (strncmp((const char*) comp->saAmfCompType.value, comp_type_name, comp->saAmfCompType.length) == 0) {

			// 3. matching compType found, check that component does not have a SaAmfHealthcheck rec (specialization)
			std::string hlt_chk_key = search_key((const char*) param->name.value, "safHealthcheckKey=");
			if (hlt_chk_key.empty()) {
				LOG_ER("%s: failed to get healthcheckKey from %s", __FUNCTION__, param->name.value);
				return;
			}
			
			memset(&hlt_chk, 0, sizeof(AVSV_HLT_KEY));
			hlt_chk.comp_name.length = comp->name.length;
			memcpy(hlt_chk.comp_name.value, comp->name.value, hlt_chk.comp_name.length);
			hlt_chk.key_len = hlt_chk_key.size();
			memcpy(hlt_chk.name.key, hlt_chk_key.c_str(), hlt_chk_key.size());
			hlt_chk.name.keyLen = hlt_chk.key_len;
			TRACE("comp_name %s key %s keyLen %u", hlt_chk.comp_name.value, hlt_chk.name.key, hlt_chk.name.keyLen);
			if (avnd_hcdb_rec_get(cb, &hlt_chk) == NULL) {
				TRACE("comp uses healthcheckType rec");
				// 4. found a component that uses the healthcheckType record, update the comp_hc_rec
				memset(&tmp_hc_rec, '\0', sizeof(AVND_COMP_HC_REC));
				tmp_hc_rec.key = hlt_chk.name;
				tmp_hc_rec.req_hdl = comp->reg_hdl;
				TRACE("tmp_hc_rec: key %s req_hdl %llu", tmp_hc_rec.key.key, tmp_hc_rec.req_hdl);
				if ((comp_hc_rec = m_AVND_COMPDB_REC_HC_GET(*comp, tmp_hc_rec)) != NULL) {
					TRACE("comp_hc_rec: period %llu max_dur %llu", comp_hc_rec->period, comp_hc_rec->max_dur);
					switch (param->attr_id) {
					case saAmfHctDefPeriod_ID:
						comp_hc_rec->period = *((SaTimeT *)param->value);
						break;
					case saAmfHctDefMaxDuration_ID:
						comp_hc_rec->max_dur = *((SaTimeT *)param->value);
						break;
					default:
						osafassert(0);
						break;
					}
				}
			}
		}	
		comp = (AVND_COMP *) compdb_rec_get_next(&cb->compdb, (uint8_t *)&comp->name);
	}
}

uint32_t avnd_hc_oper_req(AVND_CB *cb, AVSV_PARAM_INFO *param)
{
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s'", param->name.value);
	
	osaf_mutex_lock_ordie(&hcdb_mutex);
	AVND_HC *hc = (AVND_HC *)ncs_patricia_tree_get(&cb->hcdb, (uint8_t *)&param->name);
	osaf_mutex_unlock_ordie(&hcdb_mutex);

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
			osaf_mutex_lock_ordie(&hcdb_mutex);
			rc = ncs_patricia_tree_del(&cb->hcdb, &hc->tree_node);
			osaf_mutex_unlock_ordie(&hcdb_mutex);
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

uint32_t avnd_hctype_oper_req(AVND_CB *cb, AVSV_PARAM_INFO *param)
{
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s'", param->name.value);
	osaf_mutex_lock_ordie(&hctypedb_mutex);
	AVND_HCTYPE *hctype = (AVND_HCTYPE *)ncs_patricia_tree_get(&hctypedb, (uint8_t *)&param->name);
	osaf_mutex_unlock_ordie(&hctypedb_mutex);
	
	switch (param->act) {
	case AVSV_OBJ_OPR_MOD: {
		if (!hctype) {
			LOG_ER("%s: failed to get %s", __FUNCTION__, param->name.value);
			goto done;
		}

		switch (param->attr_id) {
		case saAmfHctDefPeriod_ID:
			osafassert(sizeof(SaTimeT) == param->value_len);
			hctype->saAmfHctDefPeriod = *((SaTimeT *)param->value);
			comp_hctype_update_compdb(cb, param);
			//m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, hc, AVND_CKPT_HC_PERIOD);
			break;

		case saAmfHctDefMaxDuration_ID:
			osafassert(sizeof(SaTimeT) == param->value_len);
			hctype->saAmfHctDefMaxDuration = *((SaTimeT *)param->value);
			comp_hctype_update_compdb(cb, param);
			//m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, hc, AVND_CKPT_HC_MAX_DUR);
			break;

		default:
			LOG_NO("%s: Unsupported attribute %u", __FUNCTION__, param->attr_id);
			goto done;
		}
		break;
	}

	case AVSV_OBJ_OPR_DEL: {
		if (hctype != NULL) {
			osaf_mutex_lock_ordie(&hctypedb_mutex);
			rc = ncs_patricia_tree_del(&hctypedb, &hctype->tree_node);
			osaf_mutex_unlock_ordie(&hctypedb_mutex);
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
	rc = NCSCC_RC_SUCCESS;
	
	TRACE_LEAVE();
	return rc;
}

