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

#include <immutil.h>
#include <logtrace.h>

#include <amfd.h>
#include <cluster.h>
#include <imm.h>

static NCS_PATRICIA_TREE nodegroup_db;

/**
 * Add a node group object to the db.
 * @param ng
 */
static void ng_db_add(AVD_AMF_NG *ng)
{
	unsigned int rc = ncs_patricia_tree_add(&nodegroup_db, &ng->tree_node);
	osafassert(rc == NCSCC_RC_SUCCESS);
}

/**
 * Lookup object in db using dn
 * @param dn
 * 
 * @return AVD_AMF_NG*
 */
AVD_AMF_NG *avd_ng_get(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_AMF_NG *)ncs_patricia_tree_get(&nodegroup_db, (uint8_t *)&tmp);
}

/**
 * Get next object from db using dn
 * @param dn
 * 
 * @return AVD_AMF_NG*
 */
static AVD_AMF_NG *ng_getnext(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_AMF_NG *)ncs_patricia_tree_getnext(&nodegroup_db, (uint8_t *)&tmp);
}

/**
 * Validate configuration attributes for an SaAmfNodeGroup object
 * @param ng
 * @param opdata
 * 
 * @return int
 */
static int is_config_valid(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
	int i = 0;
	unsigned j = 0;
	char *p;
	const SaImmAttrValuesT_2 *attr;

	p = strchr((char *)dn->value, ',');
	if (p == NULL) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn->value);
		return 0;
	}

	if (strncmp(++p, "safAmfCluster=", 14) != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", p, dn->value);
		return 0;
	}

	while ((attr = attributes[i++]) != NULL)
		if (!strcmp(attr->attrName, "saAmfNGNodeList"))
			break;

	osafassert(attr);
	osafassert(attr->attrValuesNumber > 0);

	for (j = 0; j < attr->attrValuesNumber; j++) {
		SaNameT *name = (SaNameT *)attr->attrValues[j];
		AVD_AVND *node = avd_node_get(name);
		if (node == NULL) {
			if (opdata == NULL) {
				report_ccb_validation_error(opdata, "'%s' does not exist in model", name->value);
				return 0;
			}

			/* Node does not exist in current model, check CCB */
			if (ccbutil_getCcbOpDataByDN(opdata->ccbId, name) == NULL) {
				report_ccb_validation_error(opdata, "'%s' does not exist either in model or CCB",
						name->value);
				return 0;
			}
		}
	}

	return 1;
}

/**
 * Create a new SaAmfNodeGroup object
 * @param dn
 * @param attributes
 * 
 * @return AVD_AVND*
 */
static AVD_AMF_NG *ng_create(SaNameT *dn, const SaImmAttrValuesT_2 **attributes)
{
	int rc = -1;
	unsigned int i, values_number;
	AVD_AMF_NG *ng;
	const SaNameT *node_name;

	TRACE_ENTER2("'%s'", dn->value);

	ng = new AVD_AMF_NG();

	memcpy(ng->ng_name.value, dn->value, dn->length);
	ng->ng_name.length = dn->length;
	ng->tree_node.key_info = (uint8_t *)&(ng->ng_name);

	if ((immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("saAmfNGNodeList"), attributes,
		&values_number) == SA_AIS_OK) && (values_number > 0)) {

		ng->number_nodes = values_number;
		ng->saAmfNGNodeList = static_cast<SaNameT*>(malloc(values_number * sizeof(SaNameT)));
		for (i = 0; i < values_number; i++) {
			if ((node_name = immutil_getNameAttr(attributes, "saAmfNGNodeList", i)) != NULL)
				ng->saAmfNGNodeList[i] = *node_name;
		}
	}
	else {
		LOG_ER("Node groups must contian at least one node");
		goto done;
	}

	rc = 0;
done:
	if (rc != 0) {
		delete ng;
		ng = NULL;
	}

	return ng;
}

/**
 * Delete a SaAmfNodeGroup object
 * 
 * @param ng
 */
static void ng_delete(AVD_AMF_NG *ng)
{
	unsigned int rc = ncs_patricia_tree_del(&nodegroup_db, &ng->tree_node);
	osafassert(rc == NCSCC_RC_SUCCESS);

	osafassert(ng);
	free(ng->saAmfNGNodeList);
	delete ng;
}

/**
 * Get configuration for all AMF node group objects from IMM and
 * create AVD internal objects.
 * 
 * @return int
 */
SaAisErrorT avd_ng_config_get(void)
{
	SaAisErrorT error, rc = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfNodeGroup";
	AVD_AMF_NG *ng;

	TRACE_ENTER();

	/* Could be here as a consequence of a fail/switch-over. Delete the DB
	** since it is anyway not synced and needs to be rebuilt. */
	dn.length = 0;
	for (ng = ng_getnext(&dn); ng != NULL; ng = ng_getnext(&dn))
		ng_delete(ng);

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, NULL, SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
		NULL, &searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("No objects found");
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {
		if (!is_config_valid(&dn, attributes, NULL)) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}

		if ((ng = ng_create(&dn, (const SaImmAttrValuesT_2 **)attributes)) == NULL)
			goto done2;

		ng_db_add(ng);
	}

	rc = SA_AIS_OK;

done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
done1:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/**
 * Determine if SU is mapped using node group
 * @param su
 * @param ng
 * 
 * @return bool
 */
static bool su_is_mapped_to_node_via_nodegroup(const AVD_SU *su, const AVD_AMF_NG *ng)
{
	if ((memcmp(&ng->ng_name, &su->saAmfSUHostNodeOrNodeGroup, sizeof(SaNameT)) == 0) ||
	    (memcmp(&ng->ng_name, &su->sg_of_su->saAmfSGSuHostNodeGroup, sizeof(SaNameT)) == 0)) {
		
		TRACE("SU '%s' mapped using '%s'", su->name.value, ng->ng_name.value);
		return true;
	}

	return false;
}

/**
 * Determine if a node is in a node group
 * @param node node
 * @param ng nodegroup
 * 
 * @return true if found, otherwise false
 */
bool node_in_nodegroup(const SaNameT *node, const AVD_AMF_NG *ng)
{
	for (unsigned int i = 0; i < ng->number_nodes; i++) {
		if ((ng->saAmfNGNodeList[i].length == node->length) &&
			memcmp(&ng->saAmfNGNodeList[i].value, node->value, node->length) == 0)
			return true;
	}
	
	return false;
}

/**
 * Validate modification of node group
 * @param opdata
 * 
 * @return SaAisErrorT
 */
static SaAisErrorT ng_ccb_completed_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	int i = 0;
	unsigned j = 0;
	const SaImmAttrModificationT_2 *mod;
	AVD_AVND *node;
	AVD_SU *su;
	int delete_found = 0;
	int add_found = 0;
	int nodes_deleted = 0;
	AVD_AMF_NG *ng;

	TRACE_ENTER();

	ng = avd_ng_get(&opdata->objectName);

	while ((mod = opdata->param.modify.attrMods[i++]) != NULL) {
		if (mod->modType == SA_IMM_ATTR_VALUES_REPLACE) {
			TRACE("replace");
			goto done;
		}

		if (mod->modType == SA_IMM_ATTR_VALUES_DELETE) {
			if (add_found) {
				report_ccb_validation_error(opdata, "ng modify: no support for mixed ops");
				goto done;
			}

			delete_found = 1;

			for (j = 0; j < mod->modAttr.attrValuesNumber; j++) {
				node = avd_node_get((SaNameT *)mod->modAttr.attrValues[j]);
				if (node == NULL) {
					report_ccb_validation_error(opdata, "Node '%s' does not exist",
							((SaNameT *)mod->modAttr.attrValues[j])->value);
					goto done;
				}

				TRACE("DEL %s", ((SaNameT *)mod->modAttr.attrValues[j])->value);

				if (node_in_nodegroup((SaNameT *)mod->modAttr.attrValues[j], ng) == false) {
					report_ccb_validation_error(opdata, "ng modify: node '%s' does not exist in node group",
						((SaNameT *)mod->modAttr.attrValues[j])->value);
					goto done;
				}
				
				/* Ensure no SU is mapped to this node via the node group */

				/* for all OpenSAF SUs hosted by this node */
				for (su = node->list_of_ncs_su; su; su = su->avnd_list_su_next) {
					if (su_is_mapped_to_node_via_nodegroup(su, ng)) {
						report_ccb_validation_error(opdata, "Cannot delete '%s' from '%s'."
								" An SU is mapped using node group",
								node->name.value, ng->ng_name.value);
						goto done;
						
					}
				}

				/* for all application SUs hosted by this node */
				for (su = node->list_of_su; su; su = su->avnd_list_su_next) {
					if (su_is_mapped_to_node_via_nodegroup(su, ng)) {
						report_ccb_validation_error(opdata, "Cannot delete '%s' from '%s'."
								" An SU is mapped using node group",
								node->name.value, ng->ng_name.value);
						goto done;
					}
				}
				
				++nodes_deleted;
				/* currently, we don't support multiple node deletions from a group in a CCB */
				if (nodes_deleted > 1) {
					report_ccb_validation_error(opdata,
						"ng modify: cannot delete more than one node from a node group in a CCB");
					goto done;
				}
			}
		}

		if (mod->modType == SA_IMM_ATTR_VALUES_ADD) {
			if (delete_found) {
				report_ccb_validation_error(opdata, "ng modify: no support for mixed ops");
				goto done;
			}

			add_found = 1;

			for (j = 0; j < mod->modAttr.attrValuesNumber; j++) {
				node = avd_node_get((SaNameT *)mod->modAttr.attrValues[j]);
				if ((node == NULL) &&
					(ccbutil_getCcbOpDataByDN(opdata->ccbId, (SaNameT *)mod->modAttr.attrValues[j]) == NULL)) {

					report_ccb_validation_error(opdata, "'%s' does not exist in model or CCB",
							((SaNameT *)mod->modAttr.attrValues[j])->value);
					goto done;
				}

				TRACE("ADD %s", ((SaNameT *)mod->modAttr.attrValues[j])->value);
			}
		}
	}

	rc = SA_AIS_OK;

done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Determine if object specified with DN is deleted in the CCB
 * @param ccbId
 * @param dn
 * 
 * @return bool
 */
static bool is_deleted_in_ccb(SaImmOiCcbIdT ccbId, const SaNameT *dn)
{
	CcbUtilOperationData_t *opdata = ccbutil_getCcbOpDataByDN(ccbId, dn);

	if ((opdata != NULL) && (opdata->operationType == CCBUTIL_DELETE))
		return true;
	else
		return false;
}

/**
 * Validate deletion of node group
 * @param opdata
 * 
 * @return SaAisErrorT
 */
static SaAisErrorT ng_ccb_completed_delete_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	AVD_SU *su;
	AVD_AVND *node;
	AVD_AMF_NG *ng = avd_ng_get(&opdata->objectName);
	unsigned int i;

	TRACE_ENTER2("%u", ng->number_nodes);

	/* for all nodes in node group */
	for (i = 0; i < ng->number_nodes; i++) {
		node = avd_node_get(&ng->saAmfNGNodeList[i]);

		TRACE("%s", node->name.value);

		/*
		** for all SUs hosted by this node, if any SU is mapped using
		** the node group that is to be deleted AND
		** the SU is not deleted in the same CCB (special case of
		** application removal), reject the deletion.
		** If no SU is mapped, deletion is OK.
		*/

		for (su = node->list_of_ncs_su; su; su = su->avnd_list_su_next) {
			if (su_is_mapped_to_node_via_nodegroup(su, ng) &&
			    is_deleted_in_ccb(opdata->ccbId, &su->name) == false) {
				report_ccb_validation_error(opdata, "Cannot delete '%s' because '%s' is mapped using it",
					ng->ng_name.value, su->name.value);
				goto done;
			}
		}

		for (su = node->list_of_su; su; su = su->avnd_list_su_next) {
			if (su_is_mapped_to_node_via_nodegroup(su, ng) &&
			    is_deleted_in_ccb(opdata->ccbId, &su->name) == false) {
				report_ccb_validation_error(opdata, "Cannot delete '%s' because '%s' is mapped using it",
					ng->ng_name.value, su->name.value);
				goto done;
			}
		}
	}

	rc = SA_AIS_OK;

done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Callback for CCB completed
 * @param opdata
 */
static SaAisErrorT ng_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(&opdata->objectName, opdata->param.create.attrValues, opdata))
		    rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		rc = ng_ccb_completed_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		rc = ng_ccb_completed_delete_hdlr(opdata);
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/**
 * Apply modify handler
 * @param opdata
 */
static void ng_ccb_apply_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	int i = 0;
	unsigned j = 0;
	const SaImmAttrModificationT_2 *mod;
	AVD_AMF_NG *ng;

	TRACE_ENTER();

	ng = avd_ng_get(&opdata->objectName);

	while ((mod = opdata->param.modify.attrMods[i++]) != NULL) {
		switch (mod->modType) {
		case SA_IMM_ATTR_VALUES_ADD: {
			ng->saAmfNGNodeList = static_cast<SaNameT*>(realloc(ng->saAmfNGNodeList,
								    (ng->number_nodes + mod->modAttr.attrValuesNumber) * sizeof(SaNameT)));

			if (ng->saAmfNGNodeList == NULL) {
				LOG_EM("%s: realloc FAILED", __FUNCTION__);
				exit(1);
			}

			for (j = 0; j < mod->modAttr.attrValuesNumber; j++) {
				ng->saAmfNGNodeList[ng->number_nodes + j] = *((SaNameT *)mod->modAttr.attrValues[j]);
				TRACE("ADD %s", ng->saAmfNGNodeList[ng->number_nodes + j].value);
			}

			ng->number_nodes += mod->modAttr.attrValuesNumber;
			TRACE("number_nodes %u", ng->number_nodes);
			break;
		}
		case SA_IMM_ATTR_VALUES_DELETE: {
			/* find node to delete */
			for (j = 0; j < ng->number_nodes; j++) {
				if (memcmp(&ng->saAmfNGNodeList[j], mod->modAttr.attrValues[0], sizeof(SaNameT)) == 0)
					break;
			}

			osafassert(j < ng->number_nodes);

			TRACE("found node %s", ng->saAmfNGNodeList[j].value);

			for (; j < (ng->number_nodes - 1); j++)
				ng->saAmfNGNodeList[j] = ng->saAmfNGNodeList[j + 1];

			ng->number_nodes -= mod->modAttr.attrValuesNumber;
			TRACE("number_nodes %u", ng->number_nodes);
			break;
		}
		default:
			osafassert(0);
		}
	}

	TRACE_LEAVE();
}

/**
 * Callback for CCB apply
 * @param opdata
 */
static void ng_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_AMF_NG *ng;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		ng = ng_create(&opdata->objectName, opdata->param.create.attrValues);
		osafassert(ng);
		ng_db_add(ng);
		break;
	case CCBUTIL_MODIFY:
		ng_ccb_apply_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		ng = avd_ng_get(&opdata->objectName);
		ng_delete(ng);
		TRACE("deleted %s", opdata->objectName.value);
		break;
	default:
		osafassert(0);
	}

	TRACE_LEAVE();
}

/**
 * Constructor for node group class. Should be called first of all.
 */
void avd_ng_constructor(void)
{
	NCS_PATRICIA_PARAMS patricia_params;

	patricia_params.key_size = sizeof(SaNameT);
	osafassert(ncs_patricia_tree_init(&nodegroup_db, &patricia_params) == NCSCC_RC_SUCCESS);

	avd_class_impl_set(const_cast<SaImmClassNameT>("SaAmfNodeGroup"), NULL, NULL, ng_ccb_completed_cb, ng_ccb_apply_cb);
}

