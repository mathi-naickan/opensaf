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

#include <stdbool.h>
#include <logtrace.h>
#include <avd_util.h>
#include <avsv_util.h>
#include <avd_csi.h>
#include <avd_imm.h>

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
static SaAisErrorT csiattr_dn_to_csiattr_name(const SaNameT *csiattr_obj_dn, SaNameT *csiattr_name)
{
	SaAisErrorT rc = SA_AIS_OK;
	char *p1, *p2;
	SaNameT dn = *csiattr_obj_dn;

	TRACE_ENTER();

	p2 = strchr((char*)dn.value, ',');
	*p2 = '\0';
	
	p1 = strchr((char*)dn.value, '=');
	p1++;
	
	if ((p2 - p1) > SA_MAX_NAME_LENGTH) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		LOG_ER("CSI attr name too long('%lu') (impl limit is '%u')", (long)(p2 - p1), SA_MAX_NAME_LENGTH);
		goto done;
	}

	strcpy((char *)&csiattr_name->value, p1);
	csiattr_name->length = p2 - p1;
done:
	TRACE_LEAVE2("obj name '%s', csi attr name '%s'", csiattr_obj_dn->value, csiattr_name->value);
	return rc;
}

static AVD_CSI_ATTR *csiattr_create(const SaNameT *csiattr_obj_name, const SaImmAttrValuesT_2 **attributes)
{
	int rc = 0;
	AVD_CSI_ATTR *csiattr = NULL, *tmp;
	unsigned int values_number;
	SaNameT dn;
	int i;
	const char *value;
	
	if (SA_AIS_OK != csiattr_dn_to_csiattr_name(csiattr_obj_name, &dn)) {
		rc = -1;
		goto done;
	}

	/* Handle multi value attributes */
	if ((immutil_getAttrValuesNumber("saAmfCSIAttriValue", attributes,
		&values_number) == SA_AIS_OK) && (values_number > 0)) {
		
		for (i = 0; i < values_number; i++) {
			tmp = calloc(1, sizeof(AVD_CSI_ATTR));
			osafassert(tmp);
			tmp->attr_next = csiattr;
			csiattr = tmp;

			csiattr->name_value.name.length = dn.length;
			memcpy(csiattr->name_value.name.value, dn.value, csiattr->name_value.name.length);

			if ((value = immutil_getStringAttr(attributes, "saAmfCSIAttriValue", i)) != NULL) {
				csiattr->name_value.string_ptr = calloc(1,strlen(value)+1);
				osafassert(csiattr->name_value.string_ptr);
				memcpy(csiattr->name_value.string_ptr, value, strlen(value)+1);
				csiattr->name_value.value.length
					= snprintf((char *)csiattr->name_value.value.value,
							SA_MAX_NAME_LENGTH, "%s", value);
			} else {
				csiattr->name_value.string_ptr = NULL;
				csiattr->name_value.value.length = 0;
			}
			tmp = NULL; 
		}
	} else {
		/* No values found, create value empty attribute */
		csiattr = calloc(1, sizeof(AVD_CSI_ATTR));
		osafassert(csiattr);
		csiattr->name_value.name.length = dn.length;
		memcpy(csiattr->name_value.name.value, dn.value, csiattr->name_value.name.length);
	}

done:
	if (rc != 0) {
		while (csiattr != NULL) {
			tmp = csiattr;
			csiattr = csiattr->attr_next;
			free(tmp);
		}
		csiattr = NULL;
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
static AVD_CSI_ATTR *csi_name_value_pair_find_last_entry(AVD_CSI *csi, SaNameT *attr_name)
{
	AVD_CSI_ATTR *i_attr = csi->list_attributes;
	AVD_CSI_ATTR *attr = NULL;
	SaUint32T    found_once = false;

	TRACE_ENTER();

	while (i_attr != NULL) {
		if (strncmp((char *)&i_attr->name_value.name.value, (char *)&attr_name->value,
				attr_name->length) == 0) {
			if (found_once == true) {
				/* More then one entry exist */
				return NULL;
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
 * Returns: Pointer to name value pair if found else NULL.
 *  
 * NOTES  : None.
 *
 **************************************************************************/
static AVD_CSI_ATTR * csi_name_value_pair_find(AVD_CSI *csi, SaNameT *csiattr_name, char *value)
{
        AVD_CSI_ATTR *i_attr = csi->list_attributes;
	TRACE_ENTER();

        while (i_attr != NULL) {
		if ((strncmp((char *)&i_attr->name_value.name.value, (char *)&csiattr_name->value, 
						csiattr_name->length) == 0) &&
				(strncmp((char *)i_attr->name_value.string_ptr, value, strlen(value)) == 0)) {
			return i_attr;
		}
                i_attr = i_attr->attr_next;
        }

	TRACE_LEAVE();
        return NULL;
}

static AVD_CSI_ATTR * is_csiattr_exists(AVD_CSI *csi, SaNameT *csiattr_obj_name)
{
	AVD_CSI_ATTR *i_attr = csi->list_attributes;
	SaNameT dn;
	
	TRACE_ENTER();

	if (SA_AIS_OK != csiattr_dn_to_csiattr_name(csiattr_obj_name, &dn)) goto done;

	while (i_attr != NULL) {
		if (strncmp((char *)&i_attr->name_value.name.value, (char *)&dn.value, dn.length) == 0)
			return i_attr;
		i_attr = i_attr->attr_next;
	}

done:
	TRACE_LEAVE();
	return NULL;
}

static int is_config_valid(const SaNameT *dn)
{
	char *parent;

	if ((parent = strchr((char*)dn->value, ',')) == NULL) {
		LOG_ER("No parent to '%s' ", dn->value);
		return 0;
	}

	if (strncmp(++parent, "safCsi=", 7) != 0) {
		LOG_ER("Wrong parent '%s' to '%s' ", parent, dn->value);
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
SaAisErrorT avd_csiattr_config_get(const SaNameT *csi_name, AVD_CSI *csi)
{
	SaAisErrorT error;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT csiattr_name;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfCSIAttribute";
	AVD_CSI_ATTR *csiattr;

	TRACE_ENTER();
	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if ((error = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, csi_name,
		SA_IMM_SUBTREE, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR,
		&searchParam, NULL, &searchHandle)) != SA_AIS_OK) {

		LOG_ER("saImmOmSearchInitialize failed: %u", error);
		goto done1;
	}

	while ((error = immutil_saImmOmSearchNext_2(searchHandle, &csiattr_name,
		(SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK) {

		if ((csiattr = csiattr_create(&csiattr_name, attributes)) != NULL)
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
	SaNameT csi_dn;
	AVD_CSI *csi;

	TRACE_ENTER();

	if (!is_config_valid(&opdata->objectName))
		goto done;

	/* extract the parent csi dn */
	strncpy((char *)&csi_dn.value, 
			strstr((char *)&opdata->objectName.value, "safCsi="),
			SA_MAX_NAME_LENGTH); 
	csi_dn.length = strlen((char *)&csi_dn.value);

	if (NULL == (csi = avd_csi_get(&csi_dn))) {
		/* if CSI is NULL, that means the CSI is added in the same CCB
		 * so allow the csi attributes also to be added in any state of the parent SI
		 */
		rc = SA_AIS_OK;
		goto done;
	}

	/* check whether an attribute with this name already exists in csi
	   if exists, only then the modification of attr values allowed */
	if (is_csiattr_exists(csi, &opdata->objectName)) {
		LOG_ER("csi attr already '%s' exists", opdata->objectName.value);
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
	SaNameT csi_dn, csi_attr_name;
	AVD_CSI *csi;
	int attr_counter = 0, add_found = 0, delete_found = 0, i = 0;

	TRACE_ENTER();

	if (!is_config_valid(&opdata->objectName)) goto done;

	/* extract the parent csi dn */
	strncpy((char *)&csi_dn.value,
			strstr((char *)&opdata->objectName.value, "safCsi="),
			SA_MAX_NAME_LENGTH);
	csi_dn.length = strlen((char *)&csi_dn.value);

	if (NULL == (csi = avd_csi_get(&csi_dn))) {
		LOG_ER("csi '%s' doesn't exists", csi_dn.value);
		goto done;
	}

	/* check whether an attribute with this name already exists in csi
	   if exists, only then the modification of attr values allowed */
	if (!is_csiattr_exists(csi, &opdata->objectName)) {
		LOG_ER("csi attr '%s' doesn't exists", opdata->objectName.value);
		rc = SA_AIS_ERR_NOT_EXIST;
		goto done;
	}

	if (SA_AIS_OK != (rc = csiattr_dn_to_csiattr_name(&opdata->objectName, &csi_attr_name))) goto done;

	while ((attr_mod = opdata->param.modify.attrMods[attr_counter++]) != NULL) {

		if ((SA_IMM_ATTR_VALUES_ADD == attr_mod->modType) || (SA_IMM_ATTR_VALUES_DELETE == attr_mod->modType)) {
			if (0 == attr_mod->modAttr.attrValuesNumber) {
				LOG_ER("CSI Attr %s Add/Del with attrValuesNumber zero", opdata->objectName.value);
				goto done;
			}
		}
		attribute = &attr_mod->modAttr;
		if (SA_IMM_ATTR_VALUES_ADD == attr_mod->modType) {
			if (delete_found) {
                                LOG_ER("csi attr '%s' modify: no support for mixed ops", opdata->objectName.value);
                                goto done;
                        }

			add_found = 1;
			/* We need to varify that csi attr name value pair already exists. */
			for (i = 0; i < attribute->attrValuesNumber; i++) {
				char *value = *(char **)attribute->attrValues[i++];

				if (csi_name_value_pair_find(csi, &csi_attr_name, value)) {
					LOG_ER("csi attr name '%s' and value '%s' exists", csi_attr_name.value, value);
					rc = SA_AIS_ERR_EXIST;
					goto done;
				}
                        } /* for  */
		} else if (SA_IMM_ATTR_VALUES_DELETE == attr_mod->modType) {
			if (add_found) {
				LOG_ER("csi attr '%s' modify: no support for mixed ops", opdata->objectName.value);
				goto done;
			}
			add_found = 1;
			/* We need to varify that csi attr name value pair already exists. */
			for (i = 0; i < attribute->attrValuesNumber; i++) {
				char *value = *(char **)attribute->attrValues[i++];

				if (NULL == value) goto done;

				if (NULL == csi_name_value_pair_find(csi, &csi_attr_name, value)) {
					LOG_ER("csi attr name '%s' and value '%s' doesn't exist", 
							csi_attr_name.value, value);
					rc = SA_AIS_ERR_EXIST;
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
        SaNameT csi_dn;
        AVD_CSI *csi;

        TRACE_ENTER();

        /* extract the parent csi dn */
        strncpy((char *)&csi_dn.value,
                        strstr((char *)&opdata->objectName.value, "safCsi="),
                        SA_MAX_NAME_LENGTH);
        csi_dn.length = strlen((char *)&csi_dn.value);

        if (NULL == (csi = avd_csi_get(&csi_dn))) {
                LOG_ER("csi '%s' doesn't exists", csi_dn.value);
                goto done;
        }

        /* check whether an attribute with this name already exists in csi
           if exists, only then the modification of attr values allowed */
        if (!is_csiattr_exists(csi, &opdata->objectName)) {
                LOG_ER("csi attr '%s' doesn't exists", opdata->objectName.value);
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

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

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

	csiattr = csiattr_create(&opdata->objectName, opdata->param.create.attrValues);
	csi = avd_csi_get(opdata->param.create.parentName);
	avd_csi_add_csiattr(csi, csiattr);
}

static void csiattr_modify_apply(CcbUtilOperationData_t *opdata)
{
	const SaImmAttrModificationT_2 *attr_mod;
	const SaImmAttrValuesT_2 *attribute;
	AVD_CSI_ATTR *csiattr = NULL, *i_attr, *tmp_csi_attr = NULL;
        SaNameT csi_attr_name, csi_dn;
	int i, counter = 0;
	AVD_CSI *csi;

        /* extract the parent csi dn */
        strncpy((char *)&csi_dn.value,
                        strstr((char *)&opdata->objectName.value, "safCsi="),
                        SA_MAX_NAME_LENGTH);
        csi_dn.length = strlen((char *)&csi_dn.value);

        csi = avd_csi_get(&csi_dn);

        csiattr_dn_to_csiattr_name(&opdata->objectName, &csi_attr_name);
	/* create new name-value pairs for the modified csi attribute */
	while ((attr_mod = opdata->param.modify.attrMods[counter++]) != NULL) {
		attribute = &attr_mod->modAttr;
		if (SA_IMM_ATTR_VALUES_ADD == attr_mod->modType) {
			tmp_csi_attr = csi_name_value_pair_find_last_entry(csi, &csi_attr_name);
			if((NULL != tmp_csi_attr)&&  (!tmp_csi_attr->name_value.value.length)) {
				/* dummy csi_attr_name+value node is present in the csi->list_attributes
				* use this node for adding the first value
				*/
				char *value = *(char **)attribute->attrValues[0];
				tmp_csi_attr->name_value.string_ptr = calloc(1,strlen(value)+1);
				osafassert(tmp_csi_attr->name_value.string_ptr);
				memcpy(tmp_csi_attr->name_value.string_ptr, value, strlen(value)+1);
				tmp_csi_attr->name_value.value.length
					= snprintf((char *)tmp_csi_attr->name_value.value.value,
							SA_MAX_NAME_LENGTH, "%s", value);
				i = 1;
			} else {
				for (i = 0; i < attribute->attrValuesNumber; i++) {
					char *value = *(char **)attribute->attrValues[i++];
					if ((i_attr = calloc(1, sizeof(AVD_CSI_ATTR))) == NULL) {
						LOG_ER("%s:calloc FAILED", __FUNCTION__);
						return;
					}

					i_attr->attr_next = csiattr;
					csiattr = i_attr;

					csiattr->name_value.name.length = csi_attr_name.length;
					memcpy(csiattr->name_value.name.value, csi_attr_name.value, csiattr->name_value.name.length);
					csiattr->name_value.string_ptr = calloc(1,strlen(value)+1);
					osafassert(csiattr->name_value.string_ptr);
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

				if ((csiattr = csi_name_value_pair_find(csi, &csi_attr_name, value))) {
					tmp_csi_attr = csi_name_value_pair_find_last_entry(csi,
								&csiattr->name_value.name);
					if(tmp_csi_attr) {
						/* Only one entry with this csi_attr_name is present, so set NULL value.
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
			while ((csiattr = is_csiattr_exists(csi, &opdata->objectName))) {
				avd_csi_remove_csiattr(csi, csiattr);
			}

			/* Add New CSI attr. */
			for (i = 0; i < attribute->attrValuesNumber; i++) {
				char *value = *(char **)attribute->attrValues[i++];
				i_attr = calloc(1, sizeof(AVD_CSI_ATTR));
				osafassert(i_attr);

				i_attr->attr_next = csiattr;
				csiattr = i_attr;
				csiattr->name_value.name.length = csi_attr_name.length;
				memcpy(csiattr->name_value.name.value, csi_attr_name.value, csiattr->name_value.name.length);
				csiattr->name_value.string_ptr = calloc(1, strlen(value)+1);
				osafassert(csiattr->name_value.string_ptr);
				memcpy(csiattr->name_value.string_ptr, value, strlen(value)+1);
			}
			/* add the modified csiattr values to parent csi */
			avd_csi_add_csiattr(csi, csiattr);
		}
	} /* while */

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
        AVD_CSI_ATTR *csiattr = NULL;
        SaNameT csi_attr_name, csi_dn;
        AVD_CSI *csi;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

        /* extract the parent csi dn */
        strncpy((char *)&csi_dn.value,
                        strstr((char *)&opdata->objectName.value, "safCsi="),
                        SA_MAX_NAME_LENGTH);
        csi_dn.length = strlen((char *)&csi_dn.value);

        csi = avd_csi_get(&csi_dn);

	if (NULL == csi) {
		/* This is the case when csi might have been deleted before csi attr. 
		   This will happen when csi delete is done before csi attr. In csi delete case, csi attr
		   also gets deleted along with csi. */
		goto done;
	}
        csiattr_dn_to_csiattr_name(&opdata->objectName, &csi_attr_name);

	/* Delete existing CSI attributes */
	while ((csiattr = is_csiattr_exists(csi, &opdata->objectName))) {
		avd_csi_remove_csiattr(csi, csiattr);
	}
done:
	TRACE_LEAVE();
}

static void csiattr_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

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
	avd_class_impl_set("SaAmfCSIAttribute", NULL, NULL, csiattr_ccb_completed_cb, csiattr_ccb_apply_cb);
}

