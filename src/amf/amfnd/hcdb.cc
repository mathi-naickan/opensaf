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

#include "amf/amfnd/avnd.h"
#include "imm/saf/saImmOm.h"
#include "osaf/immutil/immutil.h"
#include <string>
#include "base/osaf_extended_name.h"
#include "base/osaf_utility.h"

static pthread_mutex_t hcdb_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t hctypedb_mutex = PTHREAD_MUTEX_INITIALIZER;

AVND_HC *avnd_hcdb_rec_get(AVND_CB *cb, AVSV_HLT_KEY *hc_key)
{
	std::string dn = "safHealthcheckKey=";
	dn += reinterpret_cast<char*>(hc_key->name.key);
	dn += ",";
	dn += Amf::to_string(&hc_key->comp_name);
	osaf_mutex_lock_ordie(&hcdb_mutex);
	AVND_HC *hc = cb->hcdb.find(dn);
	osaf_mutex_unlock_ordie(&hcdb_mutex);
	return hc;
}

AVND_HCTYPE *avnd_hctypedb_rec_get(AVND_CB *cb, const std::string& comp_type_dn, const SaAmfHealthcheckKeyT *key)
{
	std::string dn = "safHealthcheckKey=";
	dn += reinterpret_cast<char*>(const_cast<SaUint8T*>(key->key));
	dn += ",";
	dn += comp_type_dn;
	osaf_mutex_lock_ordie(&hctypedb_mutex);
	AVND_HCTYPE *hctype =  cb->hctypedb.find(dn);
	osaf_mutex_unlock_ordie(&hctypedb_mutex);
	return hctype;
}

static AVND_HC *hc_create(AVND_CB *cb, const std::string &dn, const SaImmAttrValuesT_2 **attributes)
{
	int rc = -1;
	AVND_HC *hc = nullptr;
  
	hc = new AVND_HC();

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfHealthcheckPeriod"), attributes, 0, &hc->period) != SA_AIS_OK) {
		LOG_ER("Get saAmfHealthcheckPeriod FAILED for '%s'", dn.c_str());
		goto done;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfHealthcheckMaxDuration"), attributes, 0, &hc->max_dur) != SA_AIS_OK) {
		LOG_ER("Get saAmfHealthcheckMaxDuration FAILED for '%s'", dn.c_str());
		goto done;
	}

	hc->name = dn;

	osaf_mutex_lock_ordie(&hcdb_mutex);
	rc = cb->hcdb.insert(hc->name, hc);
	osaf_mutex_unlock_ordie(&hcdb_mutex);

 done:
	if (rc != NCSCC_RC_SUCCESS) {
		delete hc;
		hc = nullptr;
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
	SaImmHandleT immOmHandle;
	SaVersionT immVersion = { 'A', 2, 15 };

	error = saImmOmInitialize_cond(&immOmHandle, nullptr, &immVersion);
	if (error != SA_AIS_OK) {
		LOG_CR("saImmOmInitialize failed: %u", error);
		goto done;
	}

	avnd_hctype_config_get(immOmHandle, comp->saAmfCompType);

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = amf_saImmOmSearchInitialize_o2(immOmHandle, comp->name,
		SA_IMM_SUBTREE, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR,
		&searchParam, nullptr, searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("saImmOmSearchInitialize_2 failed: %u", error);
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &hc_name, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		const std::string hcn = Amf::to_string(&hc_name);

		TRACE_1("'%s'", hcn.c_str());
		if (avnd_cb->hcdb.find(hcn) == nullptr) {
			if (hc_create(avnd_cb, hcn, attributes) == nullptr)
				goto done2;
		} else
			TRACE_2("Record already exists");
	}

	error = SA_AIS_OK;

 done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
 done1:
	immutil_saImmOmFinalize(immOmHandle);
 done:
	return error;
}

static AVND_HCTYPE *hctype_create(AVND_CB *cb, const std::string &dn, const SaImmAttrValuesT_2 **attributes)
{
	int rc = -1;
	AVND_HCTYPE *hc;

	hc = new AVND_HCTYPE();

	hc->name = dn;

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfHctDefPeriod"), attributes, 0, &hc->saAmfHctDefPeriod) != SA_AIS_OK) {
		LOG_ER("Get saAmfHctDefPeriod FAILED for '%s'", dn.c_str());
		goto done;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfHctDefMaxDuration"), attributes, 0, &hc->saAmfHctDefMaxDuration) != SA_AIS_OK) {
		LOG_ER("Get saAmfHctDefMaxDuration FAILED for '%s'", dn.c_str());
		goto done;
	}
	osaf_mutex_lock_ordie(&hctypedb_mutex);
	rc = cb->hctypedb.insert(hc->name, hc);
	osaf_mutex_unlock_ordie(&hctypedb_mutex);

 done:
	if (rc != NCSCC_RC_SUCCESS) {
		delete hc;
		hc = nullptr;
	}

	return hc;
}

SaAisErrorT avnd_hctype_config_get(SaImmHandleT immOmHandle, const std::string &comptype_dn)
{
	SaAisErrorT error = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT hc_name;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfHealthcheckType";

	TRACE_ENTER2("'%s'", comptype_dn.c_str());

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = amf_saImmOmSearchInitialize_o2(immOmHandle, comptype_dn,
		SA_IMM_SUBTREE, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR,
		&searchParam, nullptr, searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("saImmOmSearchInitialize_2 failed: %u", error);
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &hc_name, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		const std::string hcn = Amf::to_string(&hc_name);

		TRACE_1("'%s'", hcn.c_str());
		//A record may get created in the context of some other component of same comptype.
		osaf_mutex_lock_ordie(&hctypedb_mutex);
		if (avnd_cb->hctypedb.find(hcn) == nullptr) {
			osaf_mutex_unlock_ordie(&hctypedb_mutex);
			if (hctype_create(avnd_cb, hcn, attributes) == nullptr)
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
	std::string comp_type_name;
	AVSV_HLT_KEY hlt_chk;
	AVND_COMP_HC_REC tmp_hc_rec;
	const std::string param_name = Amf::to_string(&param->name);
		
	// 1. find component from componentType, 
	// input example, param->name.value = safHealthcheckKey=AmfDemo,safVersion=1,safCompType=AmfDemo1	
	comp_type_name = strstr(param_name.c_str(), "safVersion");
	TRACE("comp_type_name: %s", comp_type_name.c_str());
	osafassert(comp_type_name.empty() == false);
	
	// 2. search each component for a matching compType
	for (comp = avnd_compdb_rec_get_next(cb->compdb, "");
		 comp != nullptr;
		 comp = avnd_compdb_rec_get_next(cb->compdb, comp->name)) {
		if (comp->saAmfCompType.compare(comp_type_name) == 0) {

			// 3. matching compType found, check that component does not have a SaAmfHealthcheck rec (specialization)
			std::string hlt_chk_key = search_key(param_name, "safHealthcheckKey=");
			if (hlt_chk_key.empty()) {
				LOG_ER("%s: failed to get healthcheckKey from %s", __FUNCTION__, osaf_extended_name_borrow(&param->name));
				return;
			}
			
			memset(&hlt_chk, 0, sizeof(AVSV_HLT_KEY));
			osaf_extended_name_alloc(comp->name.c_str(), &hlt_chk.comp_name);
			hlt_chk.key_len = hlt_chk_key.size();
			memcpy(hlt_chk.name.key, hlt_chk_key.c_str(), hlt_chk_key.size());
			hlt_chk.name.keyLen = hlt_chk.key_len;
			TRACE("comp_name %s key %s keyLen %u", osaf_extended_name_borrow(&hlt_chk.comp_name), hlt_chk.name.key, hlt_chk.name.keyLen);
			if (avnd_hcdb_rec_get(cb, &hlt_chk) == nullptr) {
				TRACE("comp uses healthcheckType rec");
				// 4. found a component that uses the healthcheckType record, update the comp_hc_rec
				tmp_hc_rec.key = hlt_chk.name;
				tmp_hc_rec.req_hdl = comp->reg_hdl;
				TRACE("tmp_hc_rec: key %s req_hdl %llu", tmp_hc_rec.key.key, tmp_hc_rec.req_hdl);
				if ((comp_hc_rec = m_AVND_COMPDB_REC_HC_GET(*comp, tmp_hc_rec)) != nullptr) {
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
			osaf_extended_name_free(&hlt_chk.comp_name);
		}	
	}
}

uint32_t avnd_hc_oper_req(AVND_CB *cb, AVSV_PARAM_INFO *param)
{
	uint32_t rc = NCSCC_RC_FAILURE;
	const std::string param_name = Amf::to_string(&param->name);

	TRACE_ENTER2("'%s'", param_name.c_str());
	
	osaf_mutex_lock_ordie(&hcdb_mutex);
	AVND_HC *hc = cb->hcdb.find(param_name);
	osaf_mutex_unlock_ordie(&hcdb_mutex);

	switch (param->act) {
	case AVSV_OBJ_OPR_MOD: {
		if (!hc) {
			LOG_ER("%s: failed to get %s", __FUNCTION__, param_name.c_str());
			goto done;
		}

		switch (param->attr_id) {
		case saAmfHealthcheckPeriod_ID:
			osafassert(sizeof(SaTimeT) == param->value_len);
			hc->period = *((SaTimeT *)param->value);
			break;

		case saAmfHealthcheckMaxDuration_ID:
			osafassert(sizeof(SaTimeT) == param->value_len);
			hc->max_dur = *((SaTimeT *)param->value);
			break;

		default:
			LOG_NO("%s: Unsupported attribute %u", __FUNCTION__, param->attr_id);
			goto done;
		}
		break;
	}

	case AVSV_OBJ_OPR_DEL: {
		if (hc != nullptr) {
			osaf_mutex_lock_ordie(&hcdb_mutex);
			cb->hcdb.erase(hc->name);
			osaf_mutex_unlock_ordie(&hcdb_mutex);
			delete hc;
			LOG_IN("Deleted '%s'", param_name.c_str());
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
	const std::string param_name = Amf::to_string(&param->name);

	TRACE_ENTER2("'%s'", param_name.c_str());
	osaf_mutex_lock_ordie(&hctypedb_mutex);
	AVND_HCTYPE *hctype = cb->hctypedb.find(param_name);
	osaf_mutex_unlock_ordie(&hctypedb_mutex);
	
	switch (param->act) {
	case AVSV_OBJ_OPR_MOD: {
		if (!hctype) {
			LOG_ER("%s: failed to get %s", __FUNCTION__, param_name.c_str());
			goto done;
		}

		switch (param->attr_id) {
		case saAmfHctDefPeriod_ID:
			osafassert(sizeof(SaTimeT) == param->value_len);
			hctype->saAmfHctDefPeriod = *((SaTimeT *)param->value);
			comp_hctype_update_compdb(cb, param);
			break;

		case saAmfHctDefMaxDuration_ID:
			osafassert(sizeof(SaTimeT) == param->value_len);
			hctype->saAmfHctDefMaxDuration = *((SaTimeT *)param->value);
			comp_hctype_update_compdb(cb, param);
			break;

		default:
			LOG_NO("%s: Unsupported attribute %u", __FUNCTION__, param->attr_id);
			goto done;
		}
		break;
	}

	case AVSV_OBJ_OPR_DEL: {
		if (hctype != nullptr) {
			osaf_mutex_lock_ordie(&hctypedb_mutex);
			cb->hctypedb.erase(hctype->name);
			osaf_mutex_unlock_ordie(&hctypedb_mutex);
			delete hctype;
			LOG_IN("Deleted '%s'", param_name.c_str());
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

