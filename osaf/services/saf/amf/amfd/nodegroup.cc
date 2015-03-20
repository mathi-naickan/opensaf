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
#include <set>

AmfDb<std::string, AVD_AMF_NG> *nodegroup_db = 0;
static AVD_AMF_NG *ng_create(SaNameT *dn, const SaImmAttrValuesT_2 **attributes);

/**
 * Lookup object in db using dn
 * @param dn
 * 
 * @return AVD_AMF_NG*
 */
AVD_AMF_NG *avd_ng_get(const SaNameT *dn)
{
	return nodegroup_db->find(Amf::to_string(dn));
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

	/* Check for duplicate entries in nodelist of this nodegroup at the time of 
	   creation of nodegroup. This check is applicable:
	   -when AMFD is reading the configuration from IMM at OpenSAF start or
	   -nodegroup creation using CCB operation.
	 */
	
	AVD_AMF_NG *tmp_ng = ng_create((SaNameT *)dn, attributes);
	if (tmp_ng == NULL)
		return 0;
	if (attr->attrValuesNumber != tmp_ng->number_nodes()) {
		LOG_ER("Duplicate nodes in saAmfNGNodeList of '%s'",tmp_ng->name.value);
		delete tmp_ng;
		return 0;
	}
	delete tmp_ng;
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

	memcpy(ng->name.value, dn->value, dn->length);
	ng->name.length = dn->length;

	if ((immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("saAmfNGNodeList"), attributes,
		&values_number) == SA_AIS_OK) && (values_number > 0)) {

		for (i = 0; i < values_number; i++) {
			if ((node_name = immutil_getNameAttr(attributes, "saAmfNGNodeList", i)) != NULL) {
				ng->saAmfNGNodeList.insert(Amf::to_string(node_name));
			}
		}
	}
	else {
		LOG_ER("Node groups must contian at least one node");
		goto done;
	}
	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNGAdminState"),
				attributes, 0, &ng->saAmfNGAdminState) != SA_AIS_OK) {
                ng->saAmfNGAdminState = SA_AMF_ADMIN_UNLOCKED;
		LOG_NO("Setting saAmfNGAdminState to :'%u'",ng->saAmfNGAdminState);
	}
	//TODO_NG: Add protection against shutting down state and lock-in state.
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
	nodegroup_db->erase(Amf::to_string(&ng->name));
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
	for (std::map<std::string, AVD_AMF_NG*>::const_iterator it = nodegroup_db->begin();
			it != nodegroup_db->end(); it++) {
		AVD_AMF_NG *ng = it->second;
		ng_delete(ng);
	}

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

		nodegroup_db->insert(Amf::to_string(&ng->name), ng);
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
	if ((memcmp(&ng->name, &su->saAmfSUHostNodeOrNodeGroup, sizeof(SaNameT)) == 0) ||
	    (memcmp(&ng->name, &su->sg_of_su->saAmfSGSuHostNodeGroup, sizeof(SaNameT)) == 0)) {
		
		TRACE("SU '%s' mapped using '%s'", su->name.value, ng->name.value);
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
bool node_in_nodegroup(const std::string& node, const AVD_AMF_NG *ng)
{
	std::set<std::string>::const_iterator iter;

	iter = ng->saAmfNGNodeList.find(node);
	if (iter != ng->saAmfNGNodeList.end())
		return true;

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

				if (node_in_nodegroup(Amf::to_string((SaNameT *)mod->modAttr.attrValues[j]),
						ng) == false) {
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
								node->name.value, ng->name.value);
						goto done;
						
					}
				}

				/* for all application SUs hosted by this node */
				for (su = node->list_of_su; su; su = su->avnd_list_su_next) {
					if (su_is_mapped_to_node_via_nodegroup(su, ng)) {
						report_ccb_validation_error(opdata, "Cannot delete '%s' from '%s'."
								" An SU is mapped using node group",
								node->name.value, ng->name.value);
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

			for (j = 0; j < mod->modAttr.attrValuesNumber; j++) {
				if (node_in_nodegroup(Amf::to_string((SaNameT *)mod->modAttr.attrValues[j])
							, ng) == true) {
					report_ccb_validation_error(opdata, "'%s' already exists in"
							" the nodegroup",
							((SaNameT *)mod->modAttr.attrValues[j])->value);
					goto done;
				}
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

	TRACE_ENTER2("%u", ng->number_nodes());
	std::set<std::string>::const_iterator iter;
	if ((ng->saAmfNGAdminState != SA_AMF_ADMIN_LOCKED) &&
			(ng->saAmfNGAdminState != SA_AMF_ADMIN_UNLOCKED)) {
		report_ccb_validation_error(opdata, "'%s' can be deleted in locked or unlocked admin state",
					ng->name.value);
		goto done;
	}
	for (iter = ng->saAmfNGNodeList.begin();
		iter != ng->saAmfNGNodeList.end();
		++iter) {

		node = avd_node_get(*iter);
		
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
					ng->name.value, su->name.value);
				goto done;
			}
		}

		for (su = node->list_of_su; su; su = su->avnd_list_su_next) {
			if (su_is_mapped_to_node_via_nodegroup(su, ng) &&
				is_deleted_in_ccb(opdata->ccbId, &su->name) == false) {
				report_ccb_validation_error(opdata, "Cannot delete '%s' because '%s' is mapped using it",
					ng->name.value, su->name.value);
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
			for (j = 0; j < mod->modAttr.attrValuesNumber; j++) {
				ng->saAmfNGNodeList.insert(Amf::to_string((SaNameT*)mod->modAttr.attrValues[j]));
			}

			TRACE("number_nodes %u", ng->number_nodes());
			break;
		}
		case SA_IMM_ATTR_VALUES_DELETE: {
			/* find node to delete */
			for (j = 0; j < mod->modAttr.attrValuesNumber; j++) {
				ng->saAmfNGNodeList.erase(Amf::to_string((SaNameT*)mod->modAttr.attrValues[j]));
			}

			TRACE("number_nodes %u", ng->number_nodes());
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
		nodegroup_db->insert(Amf::to_string(&ng->name), ng);
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
 * @brief  sets admin state of ng. Update in IMM, saflogging and notification for it.
 * @param  ptr to Nodegroup (AVD_AMF_NG).
 * @param  state(SaAmfAdminStateT).
 */
void avd_ng_admin_state_set(AVD_AMF_NG* ng, SaAmfAdminStateT state)
{
	SaAmfAdminStateT old_state = ng->saAmfNGAdminState;
	
	osafassert(state <= SA_AMF_ADMIN_SHUTTING_DOWN);
	TRACE_ENTER2("%s AdmState %s => %s", ng->name.value,
			avd_adm_state_name[old_state], avd_adm_state_name[state]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s AdmState %s => %s", ng->name.value,
                  avd_adm_state_name[old_state], avd_adm_state_name[state]);      
	ng->saAmfNGAdminState = state;
	avd_saImmOiRtObjectUpdate(&ng->name,
			const_cast<SaImmAttrNameT>("saAmfNGAdminState"), 
			SA_IMM_ATTR_SAUINT32T, &ng->saAmfNGAdminState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, ng, AVSV_CKPT_NG_ADMIN_STATE);
	avd_send_admin_state_chg_ntf(&ng->name,
			(SaAmfNotificationMinorIdT)SA_AMF_NTFID_NG_ADMIN_STATE,
			old_state, ng->saAmfNGAdminState);
}
/**
 * @brief  Verify if Node is stable for admin operation on Nodegroup etc.
 * @param  ptr to node(AVD_AVND).
 * @param  ptr to nodegroup(AVD_AMF_NG).
 * @Return SA_AIS_OK/SA_AIS_ERR_TRY_AGAIN/SA_AIS_ERR_BAD_OPERATION.
*/
static SaAisErrorT check_node_stability(const AVD_AVND *node, const AVD_AMF_NG *ng)
{
	SaAisErrorT rc = SA_AIS_OK;

	if (node->admin_node_pend_cbk.admin_oper != 0) {
		LOG_NO("'%s' undergoing admin operation", node->name.value);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
	if (node->clm_pend_inv != 0) {
		LOG_NO("'%s' Clm operation going on", node->name.value);
		rc = SA_AIS_ERR_TRY_AGAIN;
                goto done;
        }
	for (AVD_SU *su = node->list_of_su; su != NULL;  
			su = su->avnd_list_su_next) {
		rc = su->sg_of_su->check_sg_stability();	
		if (rc != SA_AIS_OK)
			goto done;
		rc = su->check_su_stability();
		if (rc != SA_AIS_OK)
			goto done;
	}
done:
	return rc;
}
/**
 * @brief  Ccheck for unsupported red model and warning for service outage in any SG.
 * @param  ptr to node(AVD_AVND).
 * @param  ptr to nodegroup(AVD_AMF_NG).
 * @Return SA_AIS_OK/SA_AIS_ERR_BAD_OPERATION.
*/
static SaAisErrorT check_red_model_service_outage(const AVD_AMF_NG *ng)
{
	SaAisErrorT rc = SA_AIS_OK;
	std::set<std::string> tmp_sg_list;
	for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
			iter != ng->saAmfNGNodeList.end(); ++iter) {
		AVD_AVND *node = avd_node_get(*iter);
		for (AVD_SU *su = node->list_of_su; su != NULL;  
				su = su->avnd_list_su_next) {
			//Make a temorary list_of_SG for later verification of service outage.
			tmp_sg_list.insert(Amf::to_string(&su->sg_of_su->name));
		}
	}
	for (std::set<std::string>::const_iterator iter =  tmp_sg_list.begin();
                                iter != tmp_sg_list.end(); ++iter) {
		AVD_SG *sg = sg_db->find(*iter);
		/*
		   To avoid service outage of complete SG check if there exists atleast
		   one instantiable or atleast one in service SU outside the nodegroup.
		 */
		//As of now, AMF will throw only warning, but operation will continue.	
		if (sg->is_sg_serviceable_outside_ng(ng) == true)
			LOG_NO("service outage for '%s' because of shutdown/lock "
					"on '%s'",sg->name.value,ng->name.value);

		if ((sg->sg_redundancy_model == SA_AMF_N_WAY_REDUNDANCY_MODEL) ||
				(sg->sg_redundancy_model == SA_AMF_NPM_REDUNDANCY_MODEL)) { 
			LOG_NO("Admin op on '%s'  hosting SUs of '%s' with redundancy '%u' "
					"is not supported",ng->name.value, sg->name.value,
					sg->sg_redundancy_model);
			rc = SA_AIS_ERR_NOT_SUPPORTED;
			tmp_sg_list.clear();
			goto done;
		}
	}
	tmp_sg_list.clear();
done:
	return rc;
}
/**
 * @brief  Verify if all the AMF entities are stable in NodeGroup.
 *         If all the entities are stable then AMF can accept
 *         admin operation on nodegroup.
 * @param  ptr to nodegroup(AVD_AMF_NG).
 * @Return SA_AIS_OK/SA_AIS_ERR_TRY_AGAIN/SA_AIS_ERR_BAD_OPERATION.
*/
static SaAisErrorT check_ng_stability(const AVD_AMF_NG *ng)
{
	SaAisErrorT rc = SA_AIS_OK;
		
	if (ng->admin_ng_pend_cbk.admin_oper != 0) {
		LOG_NO("'%s' is already undergoing admin operation", ng->name.value);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
	//Check for other AMF entities which are part of nodegroup.	
	for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
                                iter != ng->saAmfNGNodeList.end(); ++iter) {
                        AVD_AVND *node = avd_node_get(*iter);
			rc = check_node_stability(node, ng);
			if (rc != SA_AIS_OK)
				goto done;
	}
done:
	return rc;
}
/**
 * @brief       This function completes admin operation on Nodegroup.
 *              It responds IMM with the result of admin operation on Nodegroup.
 *              It also unsets all the admin op related parameters.
 * @param       ptr to Nodegroup (AVD_AMF_NG). 
 */
void ng_complete_admin_op(AVD_AMF_NG *ng, SaAisErrorT result)
{
	if (ng->admin_ng_pend_cbk.invocation != 0) {
		TRACE("Replying to IMM for admin op on '%s'", ng->name.value);
		avd_saImmOiAdminOperationResult(avd_cb->immOiHandle,
				ng->admin_ng_pend_cbk.invocation, result);
		ng->admin_ng_pend_cbk.invocation = 0;
		ng->admin_ng_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(0);
	}
	for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
			iter != ng->saAmfNGNodeList.end(); ++iter) {
		AVD_AVND *node = avd_node_get(*iter);
		node->admin_ng = NULL;
	}
}
/**
 * @brief       Handles node level funtionality during nodegroup lock
 *              or shutdown operation. For each SU hosted on this node, 
 *              it calls redundancy model specific handler (sg->ng_admin()),
 *              defined in the sg_*_fsm.cc, to handle assignments at SU level. 
 *              Also it updates ng->node_oper_list for tracking operation on 
 *              this node.
 * @param       ptr to Node (AVD_AVND).
 */
void ng_node_lock_and_shutdown(AVD_AVND *node)
{
	TRACE_ENTER2("'%s'",node->name.value);
	if (node->node_info.member == false) {
		node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
		LOG_NO("%s' LOCK: CLM node is not member", node->name.value);
		return;
	}
	if (avd_cb->init_state == AVD_INIT_DONE) {
		node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
		for(AVD_SU *su = node->list_of_su; su != NULL; su = su->avnd_list_su_next) {
			su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
		}
		return;
	}
	if (node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED)
		return;
	for (AVD_SU *su = node->list_of_su; su != NULL;  su = su->avnd_list_su_next) {
		su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
		su->sg_of_su->ng_admin(su, node->admin_ng);
	}
	if (node->su_cnt_admin_oper > 0)
		node->admin_ng->node_oper_list.insert(Amf::to_string(&node->name));
	TRACE_LEAVE2("node_oper_list size:%u",node->admin_ng->oper_list_size());
}
/*
 * @brief       Handles unlock of nodegroup. For each SU hosted on this node,
 *              it calls redundancy model specific assignment handler i.e 
 *	        (sg->su_insvc()), defined in the sg_*_fsm.cc, to assign SUs 
 *              based on ranks. Also SG of each SU is looked for instantiation of 
 *		new SUs. Also it updates ng->node_oper_list for tracking operation
 * 		on this node.
 * @param       ptr to Nodegroup (AVD_AMF_NG).
 */
void ng_unlock(AVD_AMF_NG *ng)
{
	TRACE_ENTER2("'%s'",ng->name.value);
	for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
			iter != ng->saAmfNGNodeList.end(); ++iter) {
		AVD_AVND *node = avd_node_get(*iter);
		node_admin_state_set(node, SA_AMF_ADMIN_UNLOCKED);
		if (node->node_info.member == false) {
			LOG_NO("'%s' UNLOCK: CLM node is not member", node->name.value);
			continue;
		}
		node->su_cnt_admin_oper = 0;
		node->admin_ng = ng;
	}
	for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
			iter != ng->saAmfNGNodeList.end(); ++iter) {
		AVD_AVND *node = avd_node_get(*iter);
		if ((node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED) ||
				(node->node_info.member == false))
			continue;
		for (AVD_SU *su = node->list_of_su; su != NULL;  su = su->avnd_list_su_next) {
			if (su->is_in_service() == true) {
				su->set_readiness_state(SA_AMF_READINESS_IN_SERVICE);
			}
		}
	}
	for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
			iter != ng->saAmfNGNodeList.end(); ++iter) {
		AVD_AVND *node = avd_node_get(*iter);
		if ((node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED) ||
				(node->node_info.member == false) ||
				(avd_cb->init_state == AVD_INIT_DONE))
			continue;
		/*
		   By this time Nodes of node group are in unlocked state.Let the
		   SG semantics decide which su to chose for assignment and instantiation. 
		 */
		for (AVD_SU *su = node->list_of_su; su != NULL;  su = su->avnd_list_su_next) {
			su->sg_of_su->su_insvc(avd_cb, su);
			avd_sg_app_su_inst_func(avd_cb, su->sg_of_su);
		}
		if (node->su_cnt_admin_oper > 0)
			node->admin_ng->node_oper_list.insert(Amf::to_string(&node->name));
	}
	TRACE_LEAVE2("node_oper_list size:%u",ng->oper_list_size());
}
/**
 * Handle admin operations on SaAmfNodeGroup objects.
 *
 * @param immoi_handle
 * @param invocation
 * @param ng_name
 * @param op_id
 * @param params
 */
static void ng_admin_op_cb(SaImmOiHandleT immoi_handle, SaInvocationT invocation,
		const SaNameT *ng_name, SaImmAdminOperationIdT op_id,
		const SaImmAdminOperationParamsT_2 **params)
{
	SaAisErrorT rc = SA_AIS_OK;
	AVD_AMF_NG *ng = avd_ng_get(ng_name);	
	TRACE_ENTER2("'%s', inv:'%llu', op:'%llu'",ng_name->value,invocation,op_id);

	switch(op_id) {
	case SA_AMF_ADMIN_LOCK:
		rc = check_ng_stability(ng);	
		if (rc != SA_AIS_OK) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, 
					SA_AIS_ERR_TRY_AGAIN, NULL,
					"Some entity is unstable, Operation cannot "
					"be performed on '%s'"
					"Check syslog for entity details", ng_name->value);
			goto done;
		}
		if (ng->saAmfNGAdminState == SA_AMF_ADMIN_LOCKED) {
                        report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_NO_OP, NULL,
                                        "'%s' Already in LOCKED state", ng->name.value);
                        goto done;
                }
		if (ng->saAmfNGAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
					"'%s' Invalid Admin Operation LOCK in state %s",
					ng->name.value, avd_adm_state_name[ng->saAmfNGAdminState]);
			goto done;
		}
		rc = check_red_model_service_outage(ng);
		if (rc != SA_AIS_OK) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, 
					SA_AIS_ERR_NOT_SUPPORTED, NULL,
					"SUs of unsupported red models hosted on '%s'"
					"Check syslog for entity details", ng_name->value);
			goto done;
		}
		ng->admin_ng_pend_cbk.invocation = invocation;
		ng->admin_ng_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(op_id);
		ng->node_oper_list.clear();

		avd_ng_admin_state_set(ng, SA_AMF_ADMIN_LOCKED);
		for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
				iter != ng->saAmfNGNodeList.end(); ++iter) {
			AVD_AVND *node = avd_node_get(*iter);
			node->su_cnt_admin_oper = 0;
			node->admin_ng = ng;
			node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
		}
		for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
				iter != ng->saAmfNGNodeList.end(); ++iter) {
			AVD_AVND *node = avd_node_get(*iter);
			ng_node_lock_and_shutdown(node);
		}
		if (ng->node_oper_list.empty())
			ng_complete_admin_op(ng, SA_AIS_OK);
		break;
	case SA_AMF_ADMIN_SHUTDOWN:
		rc = check_ng_stability(ng);	
		if (rc != SA_AIS_OK) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, 
					SA_AIS_ERR_TRY_AGAIN, NULL,
					"Some entity is unstable, Operation cannot "
					"be performed on '%s'"
					"Check syslog for entity details", ng_name->value);
			goto done;
		}
		if (ng->saAmfNGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_NO_OP, NULL,
					"'%s' Already in SHUTTING DOWN state", ng->name.value);
			goto done;
		}
		if (ng->saAmfNGAdminState != SA_AMF_ADMIN_UNLOCKED) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
					"'%s' Invalid Admin Operation SHUTDOWN in state %s",
					ng->name.value, avd_adm_state_name[ng->saAmfNGAdminState]);
			goto done;
		}
		rc = check_red_model_service_outage(ng);
		if (rc != SA_AIS_OK) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, 
					SA_AIS_ERR_NOT_SUPPORTED, NULL,
					"SUs of unsupported red models hosted on '%s'"
					"Check syslog for entity details", ng_name->value);
			goto done;
		}
		ng->admin_ng_pend_cbk.invocation = invocation;
		ng->admin_ng_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(op_id);
		ng->node_oper_list.clear();

		avd_ng_admin_state_set(ng, SA_AMF_ADMIN_SHUTTING_DOWN);
		for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
				iter != ng->saAmfNGNodeList.end(); ++iter) {
			AVD_AVND *node = avd_node_get(*iter);
			node->su_cnt_admin_oper = 0;
			node->admin_ng = ng;
			node_admin_state_set(node, SA_AMF_ADMIN_SHUTTING_DOWN);
		}
		for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
				iter != ng->saAmfNGNodeList.end(); ++iter) {
			AVD_AVND *node = avd_node_get(*iter);
			ng_node_lock_and_shutdown(node);
			if (node->su_cnt_admin_oper == 0)
				node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
		}
		if (ng->node_oper_list.empty())  {
			avd_ng_admin_state_set(ng, SA_AMF_ADMIN_LOCKED);
			for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
					iter != ng->saAmfNGNodeList.end(); ++iter) {
				AVD_AVND *node = avd_node_get(*iter);
				node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
			}
			ng_complete_admin_op(ng, SA_AIS_OK);
		}
		break;
	case SA_AMF_ADMIN_UNLOCK:
		rc = check_ng_stability(ng);
		if (rc != SA_AIS_OK) {
			report_admin_op_error(avd_cb->immOiHandle, invocation,
					SA_AIS_ERR_TRY_AGAIN, NULL,
					"Some entity is unstable, Operation cannot "
					"be performed on '%s'"
					"Check syslog for entity details", ng_name->value);
			goto done;
		}
		if (ng->saAmfNGAdminState == SA_AMF_ADMIN_UNLOCKED) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_NO_OP, NULL,
					"'%s' Already in UNLOCKED state", ng->name.value);
                        goto done;
		}
		if (ng->saAmfNGAdminState != SA_AMF_ADMIN_LOCKED) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
					"'%s' Invalid Admin Operation UNLOCK in state %s",
                                        ng->name.value, avd_adm_state_name[ng->saAmfNGAdminState]);
                        goto done;
                }
		ng->admin_ng_pend_cbk.invocation = invocation;
		ng->admin_ng_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(op_id);
		ng->node_oper_list.clear();

		avd_ng_admin_state_set(ng, SA_AMF_ADMIN_UNLOCKED);
		ng_unlock(ng);
		if (ng->node_oper_list.empty())
			ng_complete_admin_op(ng, SA_AIS_OK);
		break;
	default:
		report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_NOT_SUPPORTED, NULL,
				"Operation is not supported (%llu)", op_id);
		break;
	}
done:
	TRACE_LEAVE();
}

/**
 * Constructor for node group class. Should be called first of all.
 */
void avd_ng_constructor(void)
{
	nodegroup_db = new AmfDb<std::string, AVD_AMF_NG>;
	avd_class_impl_set("SaAmfNodeGroup", NULL, ng_admin_op_cb, ng_ccb_completed_cb,
			ng_ccb_apply_cb);
}

