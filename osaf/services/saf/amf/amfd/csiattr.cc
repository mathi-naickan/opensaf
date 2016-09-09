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
 *            Ericsson
 *
 */

#include <logtrace.h>
#include <util.h>
#include <amf_util.h>
#include <csi.h>
#include <imm.h>

/*****************************************************************************
 * Function: csiattr_dn_to_csiattr_name
 * 
 * Purpose: This routine extracts csi attr name(Name1) from csi attr obj name
 *          (safCsiAttr=Name1,safCsi=csi1,safSi=si1,safApp=app1).
 * 
 *
 * Input   : Pointer to csi attr obj name.
 * Output  : csi attr name filled in csiattr_name.
 *  
 * Returns: OK/ERROR.
 *  
 **************************************************************************/
static SaAisErrorT csiattr_dn_to_csiattr_name(const std::string& csiattr_obj_dn, std::string& csiattr_name)
{
	SaAisErrorT rc = SA_AIS_OK;
	std::string::size_type equal_pos;
	std::string::size_type comma_pos;

	TRACE_ENTER();

	equal_pos = csiattr_obj_dn.find('=');
	comma_pos = csiattr_obj_dn.find(',');

	csiattr_name = csiattr_obj_dn.substr(equal_pos + 1, comma_pos - equal_pos - 1);

	TRACE_LEAVE2("obj name '%s', csi attr name '%s'", csiattr_obj_dn.c_str(), csiattr_name.c_str());
	return rc;
}

static AVD_CSI_ATTR *csiattr_create(const std::string& csiattr_obj_name, const SaImmAttrValuesT_2 **attributes)
{
	int rc = 0;
	AVD_CSI_ATTR *csiattr = nullptr, *tmp;
	unsigned int values_number;
	std::string dn;
	unsigned int i;
	const char *value;
	
	if (SA_AIS_OK != csiattr_dn_to_csiattr_name(csiattr_obj_name, dn)) {
		rc = -1;
		goto done;
	}

	/* Handle multi value attributes */
	if ((immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("saAmfCSIAttriValue"), attributes,
		&values_number) == SA_AIS_OK) && (values_number > 0)) {
		
		for (i = 0; i < values_number; i++) {
			tmp = new AVD_CSI_ATTR();

			tmp->attr_next = csiattr;
			csiattr = tmp;

			osaf_extended_name_alloc(dn.c_str(), &csiattr->name_value.name);

			if ((value = immutil_getStringAttr(attributes, "saAmfCSIAttriValue", i)) != nullptr) {
				csiattr->name_value.string_ptr = new char[strlen(value)+1];
				memcpy(csiattr->name_value.string_ptr, value, strlen(value)+1);
				osaf_extended_name_alloc(value, &csiattr->name_value.value);
			} else {
				csiattr->name_value.string_ptr = nullptr;
				osaf_extended_name_clear(&csiattr->name_value.value);
			}
			tmp = nullptr; 
		}
	} else {
		/* No values found, create value empty attribute */
		csiattr = new AVD_CSI_ATTR();
		osaf_extended_name_alloc(dn.c_str(), &csiattr->name_value.name);
		csiattr->name_value.string_ptr = nullptr;
		osaf_extended_name_clear(&csiattr->name_value.value);
	}

done:
	if (rc != 0) {
		while (csiattr != nullptr) {
			tmp = csiattr;
			csiattr = csiattr->attr_next;
			osaf_extended_name_free(&tmp->name_value.name);
			osaf_extended_name_free(&tmp->name_value.value);
			delete [] tmp->name_value.string_ptr;
			delete tmp;
		}
		csiattr = nullptr;
	}

	return csiattr;
}
/*
 * @brief This routine searches csi attr matching att_name + value, if it finds checks if only one
 * 	  entry or more then one are present.
 *
 * @param[in] Csi pointer
 *
 * @retuen AVD_CSI_ATTR
 */
static AVD_CSI_ATTR *csi_name_value_pair_find_last_entry(const AVD_CSI *csi, const std::string& attr_name)
{
	AVD_CSI_ATTR *i_attr = csi->list_attributes;
	AVD_CSI_ATTR *attr = nullptr;
	SaUint32T    found_once = false;

	TRACE_ENTER();

	while (i_attr != nullptr) {
		if (attr_name.compare(Amf::to_string(&i_attr->name_value.name)) == 0) {
			if (found_once == true) {
				/* More then one entry exist */
				return nullptr;
			}
			found_once = true;
			attr = i_attr;
		}
		i_attr = i_attr->attr_next;
	}
	TRACE_LEAVE();
	return attr;
}
/*****************************************************************************
 * Function: csi_name_value_pair_find
 * 
 * Purpose: This routine searches csi attr matching name and value.
 * 
 *
 * Input  : Csi pointer, pointer to csi attr obj name, pointer to value of name-value pair.
 *  
 * Returns: Pointer to name value pair if found else nullptr.
 *  
 * NOTES  : None.
 *
 **************************************************************************/
static AVD_CSI_ATTR * csi_name_value_pair_find(const AVD_CSI *csi, const std::string& csiattr_name, const char *value)
{
	AVD_CSI_ATTR *i_attr = csi->list_attributes;
	TRACE_ENTER();

        while (i_attr != nullptr) {
		if ((csiattr_name.compare(Amf::to_string(&i_attr->name_value.name)) == 0) &&
				(strncmp((char *)i_attr->name_value.string_ptr, value, strlen(value)) == 0)) {
			TRACE_LEAVE();
			return i_attr;
		}
                i_attr = i_attr->attr_next;
        }

	TRACE_LEAVE();
        return nullptr;
}

static AVD_CSI_ATTR * is_csiattr_exists(const AVD_CSI *csi, const std::string& csiattr_obj_name)
{
	AVD_CSI_ATTR *i_attr = csi->list_attributes;
	std::string dn;
	
	TRACE_ENTER();

	if (SA_AIS_OK != csiattr_dn_to_csiattr_name(csiattr_obj_name, dn)) goto done;

	while (i_attr != nullptr) {
		if (dn.compare(Amf::to_string(&i_attr->name_value.name)) == 0) {
			TRACE_LEAVE();
			return i_attr;
		}
		i_attr = i_attr->attr_next;
	}

done:
	TRACE_LEAVE();
	return nullptr;
}

static int is_config_valid(const std::string& dn, CcbUtilOperationData_t *opdata)
{
	std::string::size_type pos;

	if ((pos = dn.find(',')) == std::string::npos) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn.c_str());
		return 0;
	}

	if (dn.compare(pos + 1, 7, "safCsi=") != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", dn.substr(pos +1).c_str(), dn.c_str());
		return 0;
	}

	return 1;
}

/**
 * Get configuration for the AMF CSI Attribute objects related
 * to this CSI from IMM and create AVD internal objects.
 * @param cb
 * 
 * @return int
 */
SaAisErrorT avd_csiattr_config_get(const std::string& csi_name, AVD_CSI *csi)
{
	SaAisErrorT error;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT csiattr_name;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfCSIAttribute";
	AVD_CSI_ATTR *csiattr;

	TRACE_ENTER();
	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if ((error = immutil_saImmOmSearchInitialize_o2(avd_cb->immOmHandle, csi_name.c_str(),
		SA_IMM_SUBTREE, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR,
		&searchParam, nullptr, &searchHandle)) != SA_AIS_OK) {

		LOG_ER("saImmOmSearchInitialize failed: %u", error);
		goto done1;
	}

	while ((error = immutil_saImmOmSearchNext_2(searchHandle, &csiattr_name,
		(SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK) {

		if ((csiattr = csiattr_create(Amf::to_string(&csiattr_name), attributes)) != nullptr)
			avd_csi_add_csiattr(csi, csiattr);
	}

	error = SA_AIS_OK;

	(void)immutil_saImmOmSearchFinalize(searchHandle);

done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

/*****************************************************************************
 * Function: csiattr_ccb_completed_create_hdlr
 * 
 * Purpose: This routine validates create CCB operations on SaAmfCSIAttribute objects.
 * 
 *
 * Input  : Ccb Util Oper Data
 *  
 * Returns: OK/ERROR.
 *  
 * NOTES  : None.
 *
 **************************************************************************/
static SaAisErrorT csiattr_ccb_completed_create_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	std::string csi_dn;
	AVD_CSI *csi;

	TRACE_ENTER();
	const std::string object_name(Amf::to_string(&opdata->objectName));

	if (!is_config_valid(object_name, opdata))
		goto done;

	/* extract the parent csi dn */
	csi_dn = object_name.substr(object_name.find("safCsi="));

	if (nullptr == (csi = csi_db->find(csi_dn))) {
		/* if CSI is nullptr, that means the CSI is added in the same CCB
		 * so allow the csi attributes also to be added in any state of the parent SI
		 */
		rc = SA_AIS_OK;
		goto done;
	}

	/* check whether an attribute with this name already exists in csi
	   if exists, only then the modification of attr values allowed */
	if (is_csiattr_exists(csi, object_name)) {
		report_ccb_validation_error(opdata, "csi attr already '%s' exists", object_name.c_str());
		rc = SA_AIS_ERR_EXIST;
		goto done;
	}

	rc = SA_AIS_OK;

done:

	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
 * Function: csiattr_ccb_completed_modify_hdlr
 * 
 * Purpose: This routine validates modify CCB operations on SaAmfCSIAttribute objects.
 * 
 *
 * Input  : Ccb Util Oper Data
 *  
 * Returns: OK/ERROR.
 *  
 * NOTES  : None.
 *
 **************************************************************************/
static SaAisErrorT csiattr_ccb_completed_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	const SaImmAttrModificationT_2 *attr_mod;
	const SaImmAttrValuesT_2 *attribute;
	std::string csi_dn;
	std::string csi_attr_name;
	AVD_CSI *csi;
	int attr_counter = 0, add_found = 0, delete_found = 0;
	unsigned int i = 0;

	TRACE_ENTER();
	const std::string object_name(Amf::to_string(&opdata->objectName));

	if (!is_config_valid(object_name, opdata)) goto done;

	/* extract the parent csi dn */
	csi_dn = object_name.substr(object_name.find("safCsi="));

	if (nullptr == (csi = csi_db->find(csi_dn))) {
		report_ccb_validation_error(opdata, "csi '%s' doesn't exists", csi_dn.c_str());
		goto done;
	}

	/* check whether an attribute with this name already exists in csi
	   if exists, only then the modification of attr values allowed */
	if (!is_csiattr_exists(csi, object_name)) {
		report_ccb_validation_error(opdata, "csi attr '%s' doesn't exists", object_name.c_str());
		rc = SA_AIS_ERR_NOT_EXIST;
		goto done;
	}

	if (SA_AIS_OK != (rc = csiattr_dn_to_csiattr_name(object_name, csi_attr_name))) goto done;

	while ((attr_mod = opdata->param.modify.attrMods[attr_counter++]) != nullptr) {

		if ((SA_IMM_ATTR_VALUES_ADD == attr_mod->modType) || (SA_IMM_ATTR_VALUES_DELETE == attr_mod->modType)) {
			if (0 == attr_mod->modAttr.attrValuesNumber) {
				report_ccb_validation_error(opdata, "CSI Attr %s Add/Del with attrValuesNumber zero",
						object_name.c_str());
				goto done;
			}
		}
		attribute = &attr_mod->modAttr;
		if (SA_IMM_ATTR_VALUES_ADD == attr_mod->modType) {
			if (delete_found) {
				report_ccb_validation_error(opdata, "csi attr '%s' modify: no support for mixed ops",
						object_name.c_str());
                                goto done;
                        }

			add_found = 1;
			/* We need to varify that csi attr name value pair already exists. */
			for (i = 0; i < attribute->attrValuesNumber; i++) {
				char *value = *(char **)attribute->attrValues[i++];

				if (csi_name_value_pair_find(csi, csi_attr_name, value)) {
					report_ccb_validation_error(opdata, "csi attr name '%s' and value '%s' exists", csi_attr_name.c_str(), value);
					rc = SA_AIS_ERR_EXIST;
					goto done;
				}
                        } /* for  */
		} else if (SA_IMM_ATTR_VALUES_DELETE == attr_mod->modType) {
			if (add_found) {
				report_ccb_validation_error(opdata, "csi attr '%s' modify: no support for mixed ops", object_name.c_str());
				goto done;
			}
			add_found = 1;
			/* We need to varify that csi attr name value pair already exists. */
			for (i = 0; i < attribute->attrValuesNumber; i++) {
				char *value = *(char **)attribute->attrValues[i++];

				if (nullptr == value) goto done;

				if (nullptr == csi_name_value_pair_find(csi, csi_attr_name, value)) {
					report_ccb_validation_error(opdata, "csi attr name '%s' and value '%s' doesn't exist", 
							csi_attr_name.c_str(), value);
					rc = SA_AIS_ERR_NOT_EXIST;
					goto done;
				}
                        } /* for  */
		}

	}/* while */


	rc = SA_AIS_OK;
done:

	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
 * Function: csiattr_ccb_completed_delete_hdlr
 * 
 * Purpose: This routine validates delete CCB operations on SaAmfCSIAttribute objects.
 * 
 *
 * Input  : Ccb Util Oper Data
 *  
 * Returns: OK/ERROR.
 *  
 * NOTES  : None.
 *
 **************************************************************************/
static SaAisErrorT csiattr_ccb_completed_delete_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	std::string csi_dn;
	AVD_CSI *csi;

        TRACE_ENTER();
	const std::string object_name(Amf::to_string(&opdata->objectName));

	/* extract the parent csi dn */
	csi_dn = object_name.substr(object_name.find("safCsi="));

	if (nullptr == (csi = csi_db->find(csi_dn))) {
		report_ccb_validation_error(opdata, "csi '%s' doesn't exists", csi_dn.c_str());
		goto done;
	}

	/* check whether an attribute with this name already exists in csi
	   if exists, only then the modification of attr values allowed */
	if (!is_csiattr_exists(csi, object_name)) {
		report_ccb_validation_error(opdata, "csi attr '%s' doesn't exists", object_name.c_str());
		rc = SA_AIS_ERR_NOT_EXIST;
		goto done;
	}

	rc = SA_AIS_OK;
done:

	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT csiattr_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		rc = csiattr_ccb_completed_create_hdlr(opdata);
		break;
	case CCBUTIL_MODIFY:
		rc = csiattr_ccb_completed_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		rc = csiattr_ccb_completed_delete_hdlr(opdata);
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE();
	return rc;
}

static void csiattr_create_apply(CcbUtilOperationData_t *opdata)
{
	AVD_CSI_ATTR *csiattr;
	AVD_CSI *csi;

	csiattr = csiattr_create(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues);
	csi = csi_db->find(Amf::to_string(opdata->param.create.parentName));
	avd_csi_add_csiattr(csi, csiattr);
}

/**
 * @brief       Sends CSI attribute list to AMFNDs where this CSI is assigned.
 *	        On receving this message AMFND passes this changed values to comps.
		For PI comp AMFND will issue csi attribute change callback to comp.
 *
 * @param[in]   csi - Pointer to CSI.
 */
static void csiattr_change_send_msg(AVD_CSI *csi) {
//  std::map<SaClmNodeIdT, MDS_SVC_PVT_SUB_PART_VER>::iterator it;

  for (AVD_COMP_CSI_REL *compcsi = csi->list_compcsi; compcsi != nullptr;
    compcsi = compcsi->csi_csicomp_next) {
    auto it = nds_mds_ver_db.find(compcsi->comp->su->su_on_node->node_info.nodeId);

    if (it == nds_mds_ver_db.end()) 
      continue; 
    //Check if this amfnd is capable of this functionality.
    if (it->second < AVSV_AVD_AVND_MSG_FMT_VER_7) {
      TRACE_3("not sending to '%s' on :%x with mds version:'%u'",
        osaf_extended_name_borrow(&compcsi->comp->comp_info.name),	
        compcsi->comp->su->su_on_node->node_info.nodeId,
        it->second);
      continue;
    }
    //For a NPI component check osafAmfCSICommunicateCsiAttributeChange is enabled or not.
    if (((compcsi->csi->osafAmfCSICommunicateCsiAttributeChange == true) &&
      (compcsi->comp->saaware() == false)) || 
      (compcsi->comp->saaware()== true) || 
      (compcsi->comp->proxied_pi() == true) ||
      (compcsi->comp->proxied_npi() == true)) {
      TRACE_3("sending to '%s' on :%x with mds version:'%u'",
        osaf_extended_name_borrow(&compcsi->comp->comp_info.name),	
        compcsi->comp->su->su_on_node->node_info.nodeId,
        it->second);
      avd_snd_compcsi_msg(compcsi->comp, compcsi->csi, compcsi, 
         AVSV_COMPCSI_ATTR_CHANGE_AND_NO_ACK);
    }
  }
}

static void csiattr_modify_apply(CcbUtilOperationData_t *opdata)
{
	const SaImmAttrModificationT_2 *attr_mod;
	const SaImmAttrValuesT_2 *attribute;
	AVD_CSI_ATTR *csiattr = nullptr, *i_attr, *tmp_csi_attr = nullptr;
	std::string csi_attr_name;
	std::string csi_dn;
	int counter = 0;
	unsigned int i = 0;
	AVD_CSI *csi;

	const std::string object_name(Amf::to_string(&opdata->objectName));

        /* extract the parent csi dn */
	csi_dn = object_name.substr(object_name.find("safCsi="));

	csi = csi_db->find(csi_dn);

	csiattr_dn_to_csiattr_name(object_name, csi_attr_name);
	/* create new name-value pairs for the modified csi attribute */
	while ((attr_mod = opdata->param.modify.attrMods[counter++]) != nullptr) {
		attribute = &attr_mod->modAttr;
		if (SA_IMM_ATTR_VALUES_ADD == attr_mod->modType) {
			tmp_csi_attr = csi_name_value_pair_find_last_entry(csi, csi_attr_name);
			if((nullptr != tmp_csi_attr)&&  (osaf_extended_name_length(&tmp_csi_attr->name_value.value) == 0)) {
				/* dummy csi_attr_name+value node is present in the csi->list_attributes
				* use this node for adding the first value
				*/
				char *value = *(char **)attribute->attrValues[0];
				tmp_csi_attr->name_value.string_ptr = new char[strlen(value)+1]();
				memcpy(tmp_csi_attr->name_value.string_ptr, value, strlen(value)+1);
				osaf_extended_name_alloc(tmp_csi_attr->name_value.string_ptr, &tmp_csi_attr->name_value.value);
				i = 1;
			} else{
				for (i = 0; i < attribute->attrValuesNumber; i++) {
					char *value = *(char **)attribute->attrValues[i++];
					i_attr = new AVD_CSI_ATTR();

					i_attr->attr_next = csiattr;
					csiattr = i_attr;

					osaf_extended_name_alloc(csi_attr_name.c_str(), &csiattr->name_value.name);
					csiattr->name_value.string_ptr = new char[strlen(value)+1]();
					memcpy(csiattr->name_value.string_ptr, value, strlen(value)+1 );
				} /* for  */
			}
			/* add the modified csiattr values to parent csi */
			if(csiattr) {
				avd_csi_add_csiattr(csi, csiattr);
			}
		} else if (SA_IMM_ATTR_VALUES_DELETE == attr_mod->modType) {
			for (i = 0; i < attribute->attrValuesNumber; i++) {
				char *value = *(char **)attribute->attrValues[i++];

				if ((csiattr = csi_name_value_pair_find(csi, csi_attr_name, value)) != nullptr) {
					tmp_csi_attr = csi_name_value_pair_find_last_entry(csi,
								Amf::to_string(&csiattr->name_value.name));
					if(tmp_csi_attr) {
						/* Only one entry with this csi_attr_name is present, so set nullptr value.
						 * This is to make  sure that csi_attr node in the csi->list_attributes
						 * wont be deleted when there is only  one name+value pair is found
						 */
						memset(tmp_csi_attr->name_value.string_ptr, 0, 
							strlen(tmp_csi_attr->name_value.string_ptr));
						memset(&tmp_csi_attr->name_value.value, 0, 
							sizeof(tmp_csi_attr->name_value.value));
					} else {
							avd_csi_remove_csiattr(csi, csiattr);
					}
				}

			}
		} else if (SA_IMM_ATTR_VALUES_REPLACE == attr_mod->modType) {
			/* Delete existing CSI attributes */
			while ((csiattr = is_csiattr_exists(csi, object_name)) != nullptr) {
				avd_csi_remove_csiattr(csi, csiattr);
			}

			/* Add New CSI attr. */
			for (i = 0; i < attribute->attrValuesNumber; i++) {
				char *value = *(char **)attribute->attrValues[i++];
				i_attr =  new AVD_CSI_ATTR();

				i_attr->attr_next = csiattr;
				csiattr = i_attr;
				osaf_extended_name_alloc(csi_attr_name.c_str(), &csiattr->name_value.name);
				csiattr->name_value.string_ptr = new char[strlen(value)+1]();

				memcpy(csiattr->name_value.string_ptr, value, strlen(value)+1);
			}
			/* add the modified csiattr values to parent csi */
			avd_csi_add_csiattr(csi, csiattr);
		}
	} /* while */

	if (avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE) {
		csiattr_change_send_msg(csi);
	}
}

/*****************************************************************************
 * Function: csiattr_delete_apply 
 * 
 * Purpose: This routine handles delete operations on SaAmfCSIAttribute objects.
 * 
 *
 * Input  : Ccb Util Oper Data. 
 *  
 * Returns: None.
 **************************************************************************/
static void csiattr_delete_apply(CcbUtilOperationData_t *opdata)
{
	AVD_CSI_ATTR *csiattr = nullptr;
	std::string csi_attr_name;
	std::string csi_dn;
	AVD_CSI *csi;
	const std::string object_name(Amf::to_string(&opdata->objectName));

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, object_name.c_str());

	/* extract the parent csi dn */
	csi_dn = object_name.substr(object_name.find("safCsi="));

        csi = csi_db->find(csi_dn);

	if (nullptr == csi) {
		/* This is the case when csi might have been deleted before csi attr. 
		   This will happen when csi delete is done before csi attr. In csi delete case, csi attr
		   also gets deleted along with csi. */
		goto done;
	}
        csiattr_dn_to_csiattr_name(object_name, csi_attr_name);

	/* Delete existing CSI attributes */
	while ((csiattr = is_csiattr_exists(csi, object_name))) {
		avd_csi_remove_csiattr(csi, csiattr);
	}
done:
	TRACE_LEAVE();
}

static void csiattr_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		csiattr_create_apply(opdata);
		break;
	case CCBUTIL_DELETE:
		csiattr_delete_apply(opdata);
		break;
	case CCBUTIL_MODIFY:
		csiattr_modify_apply(opdata);
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE();
}

void avd_csiattr_constructor(void)
{
	avd_class_impl_set("SaAmfCSIAttribute", nullptr, nullptr,
		csiattr_ccb_completed_cb, csiattr_ccb_apply_cb);
}

