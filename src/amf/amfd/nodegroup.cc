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

#include "osaf/immutil/immutil.h"
#include "base/logtrace.h"

#include "amf/amfd/amfd.h"
#include "amf/amfd/cluster.h"
#include "amf/amfd/imm.h"
#include <set>

AmfDb<std::string, AVD_AMF_NG> *nodegroup_db = 0;
static AVD_AMF_NG *ng_create(const std::string& dn, const SaImmAttrValuesT_2 **attributes);
//TODO: Make  below function members.
static void ng_admin_unlock_inst(AVD_AMF_NG *ng);
static void ng_unlock(AVD_AMF_NG *ng);

/**
 * Lookup object in db using dn
 * @param dn
 * 
 * @return AVD_AMF_NG*
 */
AVD_AMF_NG *avd_ng_get(const std::string& dn)
{
	return nodegroup_db->find(dn);
}

/**
 * Validate configuration attributes for an SaAmfNodeGroup object
 * @param ng
 * @param opdata
 * 
 * @return int
 */
static int is_config_valid(const std::string& dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t *opdata)
{
	int i = 0;
	unsigned j = 0;
	std::string::size_type pos;
	const SaImmAttrValuesT_2 *attr;

	TRACE_ENTER2("%s", dn.c_str());

	if ((pos = dn.find(',')) == std::string::npos) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn.c_str());
		TRACE_LEAVE();
		return 0;
	}

	if (dn.compare(pos + 1, 14, "safAmfCluster=") != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", dn.substr(pos + 1).c_str(), dn.c_str());
		TRACE_LEAVE();
		return 0;
	}

	while ((attr = attributes[i++]) != nullptr)
		if (!strcmp(attr->attrName, "saAmfNGNodeList"))
			break;

	osafassert(attr);
	osafassert(attr->attrValuesNumber > 0);

	for (j = 0; j < attr->attrValuesNumber; j++) {
		SaNameT *name = static_cast<SaNameT *>(attr->attrValues[j]);
		AVD_AVND *node = avd_node_get(Amf::to_string(name));
		if (node == nullptr) {
			if (opdata == nullptr) {
				report_ccb_validation_error(opdata, "'%s' does not exist in model", osaf_extended_name_borrow(name));
				TRACE_LEAVE();
				return 0;
			}

			/* Node does not exist in current model, check CCB */
			if (ccbutil_getCcbOpDataByDN(opdata->ccbId, name) == nullptr) {
				report_ccb_validation_error(opdata, "'%s' does not exist either in model or CCB",
						osaf_extended_name_borrow(name));
				TRACE_LEAVE();
				return 0;
			}
		}
	}

	/* Check for duplicate entries in nodelist of this nodegroup at the time of 
	   creation of nodegroup. This check is applicable:
	   -when AMFD is reading the configuration from IMM at OpenSAF start or
	   -nodegroup creation using CCB operation.
	 */

	AVD_AMF_NG *tmp_ng = ng_create(dn, attributes);
	if (tmp_ng == nullptr) {
		TRACE_LEAVE();
		return 0;
	}

	if (attr->attrValuesNumber != tmp_ng->number_nodes()) {
		LOG_ER("Duplicate nodes in saAmfNGNodeList of '%s'",tmp_ng->name.c_str());
		delete tmp_ng;
		TRACE_LEAVE();
		return 0;
	}
	//Check if admin state is valid or not.
	if (!avd_admin_state_is_valid(tmp_ng->saAmfNGAdminState, opdata)) {
		LOG_ER("Incorrect saAmfNGAdminState:'%u' for '%s'",tmp_ng->saAmfNGAdminState,
				tmp_ng->name.c_str());
		delete tmp_ng;
		TRACE_LEAVE();
		return 0;
	}
	delete tmp_ng;
	TRACE_LEAVE();
	return 1;
}

/**
 *
 */
AVD_AMF_NG::AVD_AMF_NG() :
	cluster_list_ng_next(0),
	ng_on_cluster(0),
	saAmfNGAdminState(SA_AMF_ADMIN_UNLOCKED)
{
	admin_ng_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(0);
	admin_ng_pend_cbk.invocation = 0;
}

/**
 * Create a new SaAmfNodeGroup object
 * @param dn
 * @param attributes
 * 
 * @return AVD_AVND*
 */
static AVD_AMF_NG *ng_create(const std::string& dn, const SaImmAttrValuesT_2 **attributes)
{
	int rc = -1;
	unsigned int i, values_number;
	AVD_AMF_NG *ng;
	const SaNameT *node_name;

	TRACE_ENTER2("'%s'", dn.c_str());

	ng = new AVD_AMF_NG();

	ng->name = dn;

	if ((immutil_getAttrValuesNumber(const_cast<SaImmAttrNameT>("saAmfNGNodeList"), attributes,
		&values_number) == SA_AIS_OK) && (values_number > 0)) {

		for (i = 0; i < values_number; i++) {
			if ((node_name = immutil_getNameAttr(attributes, "saAmfNGNodeList", i)) != nullptr) {
				ng->saAmfNGNodeList.insert(Amf::to_string(node_name));
			}
		}
	}
	else {
		LOG_ER("Node groups must contain at least one node");
		goto done;
	}
	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNGAdminState"),
				attributes, 0, &ng->saAmfNGAdminState) != SA_AIS_OK) {
                ng->saAmfNGAdminState = SA_AMF_ADMIN_UNLOCKED;
		TRACE("Setting saAmfNGAdminState to :'%u'",ng->saAmfNGAdminState);
	}
	rc = 0;
done:
	if (rc != 0) {
		delete ng;
		ng = nullptr;
	}

	osafassert(ng != nullptr);
	TRACE_LEAVE2("%u", rc);
	return ng;
}

/**
 * Delete a SaAmfNodeGroup object
 * 
 * @param ng
 */
static void ng_delete(AVD_AMF_NG *ng)
{
	nodegroup_db->erase(ng->name);
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
	for (std::map<std::string, AVD_AMF_NG*>::const_iterator it = nodegroup_db->begin();
			it != nodegroup_db->end(); it++) {
		AVD_AMF_NG *ng = it->second;
		ng_delete(ng);
	}

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, nullptr, SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
		nullptr, &searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("No objects found");
		goto done1;
	}

	while (immutil_saImmOmSearchNext_2(searchHandle, &dn, (SaImmAttrValuesT_2 ***)&attributes) == SA_AIS_OK) {

		if (is_config_valid(Amf::to_string(&dn), attributes, nullptr) == false) {
			TRACE("%s config is invalid", osaf_extended_name_borrow(&dn));
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}

		if ((ng = ng_create(Amf::to_string(&dn), (const SaImmAttrValuesT_2 **)attributes)) == nullptr) {
			goto done2;
		}

		nodegroup_db->insert(ng->name, ng);
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
	if (ng->name.compare(su->saAmfSUHostNodeOrNodeGroup) == 0 ||
		ng->name.compare(su->sg_of_su->saAmfSGSuHostNodeGroup) == 0) {
		TRACE("SU '%s' mapped using '%s'", su->name.c_str(), ng->name.c_str());
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
	int delete_found = 0;
	int add_found = 0;
	int nodes_deleted = 0;
	AVD_AMF_NG *ng;
	CcbUtilOperationData_t *t_opData;

	TRACE_ENTER();

	ng = avd_ng_get(Amf::to_string(&opdata->objectName));
	if (ng == nullptr) {
		report_ccb_validation_error(opdata, "ng modify: nodegroup cannot be found");
		goto done;
	}

	while ((mod = opdata->param.modify.attrMods[i++]) != nullptr) {
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
				const std::string node_name(Amf::to_string(static_cast<SaNameT *>(mod->modAttr.attrValues[j])));
				node = avd_node_get(node_name);
				if (node == nullptr) {
					report_ccb_validation_error(opdata, "Node '%s' does not exist",
							node_name.c_str());
					goto done;
				}

				TRACE("DEL %s", node_name.c_str());

				if (node_in_nodegroup(node_name, ng) == false) {
					report_ccb_validation_error(opdata, "ng modify: node '%s' does not exist in node group",
						node_name.c_str());
					goto done;
				}
				
				/* Ensure no SU is mapped to this node via the node group */

				/* for all OpenSAF SUs hosted by this node */
				for (const auto& su : node->list_of_ncs_su) {
					const SaNameTWrapper su_name(su->name);
					if (su_is_mapped_to_node_via_nodegroup(su, ng)) {
						t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, su_name);
						if (t_opData == nullptr || t_opData->operationType != CCBUTIL_DELETE) {
							report_ccb_validation_error(opdata, "Cannot delete '%s' from '%s'."
								" An SU is mapped using node group",
								node->name.c_str(), ng->name.c_str());
								goto done;
						}
					}
				}

				/* for all application SUs hosted by this node */
				for (const auto& su : node->list_of_su) {
					const SaNameTWrapper su_name(su->name);
					if (su_is_mapped_to_node_via_nodegroup(su, ng)) {
						t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, su_name);
						if (t_opData == nullptr || t_opData->operationType != CCBUTIL_DELETE) {
							report_ccb_validation_error(opdata, "Cannot delete '%s' from '%s'."
							" An SU is mapped using node group",
							node->name.c_str(), ng->name.c_str());
							goto done;
						}
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
				node = avd_node_get(Amf::to_string(static_cast<SaNameT *>(mod->modAttr.attrValues[j])));
				if ((node == nullptr) &&
					(ccbutil_getCcbOpDataByDN(opdata->ccbId, (SaNameT *)mod->modAttr.attrValues[j]) == nullptr)) {

					report_ccb_validation_error(opdata, "'%s' does not exist in model or CCB",
							osaf_extended_name_borrow((SaNameT *)mod->modAttr.attrValues[j]));
					goto done;
				}

				TRACE("ADD %s", osaf_extended_name_borrow(static_cast<SaNameT *>(mod->modAttr.attrValues[j])));
			}

			for (j = 0; j < mod->modAttr.attrValuesNumber; j++) {
				if (node_in_nodegroup(Amf::to_string(static_cast<SaNameT *>(mod->modAttr.attrValues[j]))
							, ng) == true) {
					report_ccb_validation_error(opdata, "'%s' already exists in"
							" the nodegroup",
							osaf_extended_name_borrow(static_cast<SaNameT *>(mod->modAttr.attrValues[j])));
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
static bool is_deleted_in_ccb(SaImmOiCcbIdT ccbId, const std::string& dn)
{
	const SaNameTWrapper dn_name(dn);
	CcbUtilOperationData_t *opdata = ccbutil_getCcbOpDataByDN(ccbId, dn_name);

	if ((opdata != nullptr) && (opdata->operationType == CCBUTIL_DELETE))
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
	AVD_AVND *node;
	AVD_AMF_NG *ng = avd_ng_get(Amf::to_string(&opdata->objectName));

        if (ng == nullptr) {
		LOG_WA("Could not find %s in nodegroup_db", osaf_extended_name_borrow(&opdata->objectName));
		TRACE_LEAVE();
		return SA_AIS_OK;
        }

	TRACE_ENTER2("%u", ng->number_nodes());
	std::set<std::string>::const_iterator iter;
	if ((ng->saAmfNGAdminState != SA_AMF_ADMIN_LOCKED) &&
			(ng->saAmfNGAdminState != SA_AMF_ADMIN_UNLOCKED) && 
			(ng->saAmfNGAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION)) {
		report_ccb_validation_error(opdata, "'%s' can be deleted in locked," 
				"lock-in or unlocked admin state", ng->name.c_str());
		goto done;
	}
	for (iter = ng->saAmfNGNodeList.begin();
		iter != ng->saAmfNGNodeList.end();
		++iter) {

		node = avd_node_get(*iter);
		if (node == nullptr)
			continue;
		
		TRACE("%s", node->name.c_str());

		/*
		** for all SUs hosted by this node, if any SU is mapped using
		** the node group that is to be deleted AND
		** the SU is not deleted in the same CCB (special case of
		** application removal), reject the deletion.
		** If no SU is mapped, deletion is OK.
		*/
		for (const auto& su : node->list_of_ncs_su) {
			if (su_is_mapped_to_node_via_nodegroup(su, ng) &&
				is_deleted_in_ccb(opdata->ccbId, su->name) == false) {
				report_ccb_validation_error(opdata, "Cannot delete '%s' because '%s' is mapped using it",
					ng->name.c_str(), su->name.c_str());
				goto done;
			}
		}

		for (const auto& su : node->list_of_su) {
			if (su_is_mapped_to_node_via_nodegroup(su, ng) &&
				is_deleted_in_ccb(opdata->ccbId, su->name) == false) {
				report_ccb_validation_error(opdata, "Cannot delete '%s' because '%s' is mapped using it",
					ng->name.c_str(), su->name.c_str());
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

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues, opdata))
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
	SaAisErrorT rc;
	const SaImmAttrValuesT_2 **attributes;
	SaImmAccessorHandleT accessorHandle;
	TRACE_ENTER();

	ng = avd_ng_get(Amf::to_string(&opdata->objectName));
	if (ng == nullptr) {
		TRACE("%p", ng);
		/* During Standby cold sync, there may be chance that
		   node group creation was missed in-between config read
		   and being applier, so re-read config again.
		   Once it is read, then below ADD/DEL logic may not be
		   needed because config will be the latest, but just keeping
		   because they are not causing any harm.*/
		immutil_saImmOmAccessorInitialize(avd_cb->immOmHandle, &accessorHandle);
		rc = immutil_saImmOmAccessorGet_2(accessorHandle, &opdata->objectName,
				nullptr, (SaImmAttrValuesT_2 ***)&attributes);
		if (rc != SA_AIS_OK) {
			LOG_ER("saImmOmAccessorGet_2 FAILED %u for %s", rc, osaf_extended_name_borrow(&opdata->objectName));
			goto done;
		}
		TRACE("'%s'", osaf_extended_name_borrow(&opdata->objectName));
		if ((ng = ng_create(Amf::to_string(&opdata->objectName), attributes)) == nullptr) {
			LOG_ER("ng_create failed for %s", osaf_extended_name_borrow(&opdata->objectName));
			goto done;
		}
		nodegroup_db->insert(ng->name, ng);
		TRACE("ng '%s' created with number_nodes '%u'", ng->name.c_str(),
				ng->number_nodes());
		immutil_saImmOmAccessorFinalize(accessorHandle);
	}

	while ((mod = opdata->param.modify.attrMods[i++]) != nullptr) {
		switch (mod->modType) {
		case SA_IMM_ATTR_VALUES_ADD: {
			for (j = 0; j < mod->modAttr.attrValuesNumber; j++) {
				ng->saAmfNGNodeList.insert(Amf::to_string((SaNameT*)mod->modAttr.attrValues[j]));
			}

			TRACE("number_nodes after addition %u", ng->number_nodes());
			break;
		}
		case SA_IMM_ATTR_VALUES_DELETE: {
			/* find node to delete */
			for (j = 0; j < mod->modAttr.attrValuesNumber; j++) {
				ng->saAmfNGNodeList.erase(Amf::to_string((SaNameT*)mod->modAttr.attrValues[j]));
			}

			TRACE("number_nodes left after deletion %u", ng->number_nodes());
			break;
		}
		default:
			osafassert(0);
		}
	}
done:
	TRACE_LEAVE();
}

static void ng_ccb_apply_delete_hdlr(CcbUtilOperationData_t *opdata)
{
	TRACE_ENTER();
	AVD_AMF_NG *ng = avd_ng_get(Amf::to_string(&opdata->objectName));
	if (ng == nullptr) {
		LOG_WA("Could not find %s in nodegroup_db", osaf_extended_name_borrow(&opdata->objectName));
		TRACE_LEAVE();
		return;
	}
	SaAmfAdminStateT old_admin_state = ng->saAmfNGAdminState;
	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) {
		//Since AMF will delete NG, clear its pointers in node.
		for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
				iter != ng->saAmfNGNodeList.end(); ++iter) {
			AVD_AVND *node = avd_node_get(*iter);
			if (node == nullptr)
				continue;
			node->admin_ng = nullptr;
			node->su_cnt_admin_oper = 0;
		}
		ng_delete(ng);
		goto done;
        }
	//Temporarily keep NG in UNLOCKED state to instantiate and assign SUs.
	ng->saAmfNGAdminState = SA_AMF_ADMIN_UNLOCKED;	
	if (old_admin_state == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
		for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
				iter != ng->saAmfNGNodeList.end(); ++iter) {
			AVD_AVND *node = avd_node_get(*iter);
			if ((node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) ||
					(any_ng_in_locked_in_state(node) == true))
				continue;
			node->node_sus_termstate_set(false);
		}
		//Instantiate SUs on nodes of NG. AMFD takes care of assignment after instantiation.
		ng_admin_unlock_inst(ng);
	} else if (old_admin_state == SA_AMF_ADMIN_LOCKED){
		/*Now try to assign SUs on the eligible nodes. Internally it takes care 
		  of instantiation of new SUs also.*/
		ng->assign_unlocked_nodes();
	}
	//Since AMF will delete NG, clear its pointers in node.
	for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
			iter != ng->saAmfNGNodeList.end(); ++iter) {
		AVD_AVND *node = avd_node_get(*iter);
		node->admin_ng = nullptr;
		node->su_cnt_admin_oper = 0;
	}
	ng->node_oper_list.clear();
	ng_delete(ng);
done:
	TRACE_LEAVE2("deleted %s", osaf_extended_name_borrow(&opdata->objectName));
}
/**
 * Callback for CCB apply
 * @param opdata
 */
static void ng_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_AMF_NG *ng;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		ng = ng_create(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues);
		osafassert(ng);
		nodegroup_db->insert(ng->name, ng);
		break;
	case CCBUTIL_MODIFY:
		ng_ccb_apply_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		ng_ccb_apply_delete_hdlr(opdata);
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
	TRACE_ENTER2("%s AdmState %s => %s", ng->name.c_str(),
			avd_adm_state_name[old_state], avd_adm_state_name[state]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s AdmState %s => %s", ng->name.c_str(),
                  avd_adm_state_name[old_state], avd_adm_state_name[state]);      
	ng->saAmfNGAdminState = state;
	avd_saImmOiRtObjectUpdate(ng->name,
			const_cast<SaImmAttrNameT>("saAmfNGAdminState"), 
			SA_IMM_ATTR_SAUINT32T, &ng->saAmfNGAdminState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, ng, AVSV_CKPT_NG_ADMIN_STATE);
	avd_send_admin_state_chg_ntf(ng->name,
			(SaAmfNotificationMinorIdT)SA_AMF_NTFID_NG_ADMIN_STATE,
			old_state, ng->saAmfNGAdminState);
	TRACE_LEAVE();
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
		LOG_NO("'%s' undergoing admin operation", node->name.c_str());
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
	if (node->clm_pend_inv != 0) {
		LOG_NO("'%s' Clm operation going on", node->name.c_str());
		rc = SA_AIS_ERR_TRY_AGAIN;
                goto done;
        }
	for (const auto& su : node->list_of_su) {
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
		for (const auto& su : node->list_of_su) {
			//Make a temorary list_of_SG for later verification of service outage.
			tmp_sg_list.insert(su->sg_of_su->name);
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
					"on '%s'",sg->name.c_str(),ng->name.c_str());

	}
	tmp_sg_list.clear();
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
		LOG_NO("'%s' is already undergoing admin operation", ng->name.c_str());
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
		TRACE("Replying to IMM for admin op on '%s'", ng->name.c_str());
		avd_saImmOiAdminOperationResult(avd_cb->immOiHandle,
				ng->admin_ng_pend_cbk.invocation, result);
		ng->admin_ng_pend_cbk.invocation = 0;
		ng->admin_ng_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(0);
	}
	for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
			iter != ng->saAmfNGNodeList.end(); ++iter) {
		AVD_AVND *node = avd_node_get(*iter);
		node->admin_ng = nullptr;
	}
}

/**
 *  Assign SUs on nodes of NG after evaluating 
 *  assignment criteria.	 
 */
void AVD_AMF_NG::assign_unlocked_nodes() const
{
	TRACE_ENTER();
	for (std::set<std::string>::const_iterator iter = saAmfNGNodeList.begin();
			iter != saAmfNGNodeList.end(); ++iter) {
		AVD_AVND *node = avd_node_get(*iter);
		if ((node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED) ||
				(node->saAmfNodeAdminState != SA_AMF_ADMIN_UNLOCKED) ||
				(node->node_info.member == false) ||
				(are_all_ngs_in_unlocked_state(node) == false))
			continue;
		for (const auto& su : node->list_of_su) {
			if (su->is_in_service() == true) {
				su->set_readiness_state(SA_AMF_READINESS_IN_SERVICE);
			}
		}
	}
	for (std::set<std::string>::const_iterator iter = saAmfNGNodeList.begin();
			iter != saAmfNGNodeList.end(); ++iter) {
		AVD_AVND *node = avd_node_get(*iter);
		if ((node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED) ||
				(node->node_info.member == false) ||
				(node->saAmfNodeAdminState != SA_AMF_ADMIN_UNLOCKED) ||
				(avd_cb->init_state == AVD_INIT_DONE) ||
				(are_all_ngs_in_unlocked_state(node) == false))
			continue;
		/* This node is capable of assignment. Let the SG semantics decide which
		   su to choose for assignment.
		 */
		for (const auto& su : node->list_of_su)
			su->sg_of_su->su_insvc(avd_cb, su);
	}
	TRACE_LEAVE();
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
	TRACE_ENTER2("'%s'",node->name.c_str());
	if (node->node_info.member == false) {
		node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
		LOG_NO("%s' LOCK: CLM node is not member", node->name.c_str());
		return;
	}
	if (avd_cb->init_state == AVD_INIT_DONE) {
		node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
		for (const auto& su : node->list_of_su) {
			su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
		}
		return;
	}
	if (node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED)
		return;
	for (const auto& su : node->list_of_su) {
		su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
		su->sg_of_su->ng_admin(su, node->admin_ng);
	}
	if (node->su_cnt_admin_oper > 0)
		node->admin_ng->node_oper_list.insert(node->name);
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
static void ng_unlock(AVD_AMF_NG *ng)
{
	TRACE_ENTER2("'%s'",ng->name.c_str());
	for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
			iter != ng->saAmfNGNodeList.end(); ++iter) {
		AVD_AVND *node = avd_node_get(*iter);
		node->su_cnt_admin_oper = 0;
		if (node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION)
			continue;
		node_admin_state_set(node, SA_AMF_ADMIN_UNLOCKED);
		if (node->node_info.member == false) {
			LOG_NO("'%s' UNLOCK: CLM node is not member", node->name.c_str());
			continue;
		}
		node->admin_ng = ng;
	}
	for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
			iter != ng->saAmfNGNodeList.end(); ++iter) {
		AVD_AVND *node = avd_node_get(*iter);
		if ((node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED) ||
				(node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) ||
				(node->node_info.member == false)) 
			continue;
		for (const auto& su : node->list_of_su) {
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
				(node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) ||
				(avd_cb->init_state == AVD_INIT_DONE))
			continue;
		/*
		   By this time Nodes of node group are in unlocked state.Let the
		   SG semantics decide which su to chose for assignment and instantiation. 
		 */
		for (const auto& su : node->list_of_su) {
			su->sg_of_su->su_insvc(avd_cb, su);
			avd_sg_app_su_inst_func(avd_cb, su->sg_of_su);
		}
		if (node->su_cnt_admin_oper > 0)
			node->admin_ng->node_oper_list.insert(node->name);
	}
	TRACE_LEAVE2("node_oper_list size:%u",ng->oper_list_size());
}

/**
 * perform unlock-instantiation on NG with honoring saAmfSURank. 
 * 
 * @param cb
 * @param ng
 */
static void ng_admin_unlock_inst(AVD_AMF_NG *ng)
{
	uint32_t su_try_inst;
	std::set<std::string> tmp_sg_list;
	TRACE_ENTER2("%s", ng->name.c_str());

	for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
			iter != ng->saAmfNGNodeList.end(); ++iter) {
		AVD_AVND *node = avd_node_get(*iter);
		if (node->node_info.member == false) {
			LOG_NO("'%s' UNLOCK_INSTANTIATION: CLM node is not member", node->name.c_str());
			continue;
		}
		if (node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED) {
			LOG_NO("'%s' UNLOCK_INSTANTIATION: AMF node oper state disabled", node->name.c_str());
			continue;
		}
		if (any_ng_in_locked_in_state(node) == true) {
			LOG_NO("One of the node group of '%s' is in locked-in", node->name.c_str());
			continue;
		}
		for (const auto& node_su : node->list_of_su) {
			/*Instantiate only those SUs in this SG which are hosted on the Nodes of NG.
			   Also honor saAmfSURank while instantating.
			 */
			AVD_SG *sg = node_su->sg_of_su;

			std::set<std::string>::const_iterator iter1 ;
			iter1 = tmp_sg_list.find(sg->name);
			if (iter1 != tmp_sg_list.end())
				continue;

			su_try_inst = 0;
			avd_sg_adjust_config(sg);
			for (const auto& su : sg->list_of_su) {
				//Continue if this SU is not hosted on the Node of NG.
				if (node_in_nodegroup(su->su_on_node->name, ng) == false)
					continue;

				if ((su->saAmfSUAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
						(su->su_on_node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
						(su->saAmfSUOperState == SA_AMF_OPERATIONAL_ENABLED) &&
						(su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED) &&
						(su->su_on_node->saAmfNodeOperState == SA_AMF_OPERATIONAL_ENABLED)) {
					if ((su->saAmfSUPreInstantiable == false) ||
							(su->su_on_node->node_state != AVD_AVND_STATE_PRESENT))
						continue;

					if (sg->saAmfSGNumPrefInserviceSUs > su_try_inst) {
						if (avd_snd_presence_msg(avd_cb, su, false) != NCSCC_RC_SUCCESS) {
							LOG_NO("Failed to send Instantiation of '%s'", su->name.c_str());
						} else {
							su->su_on_node->su_cnt_admin_oper++;
							su_try_inst++;
						}
					}
				}
			}
			tmp_sg_list.insert(sg->name);
		}
		TRACE("node:'%s', su_cnt_admin_oper:%u",
				node->name.c_str(), node->su_cnt_admin_oper);
		if (node->su_cnt_admin_oper > 0)
			ng->node_oper_list.insert(node->name);
	}
	TRACE("node_oper_list size:%u",ng->oper_list_size());
	TRACE_LEAVE();
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
	AVD_AMF_NG *ng = avd_ng_get(Amf::to_string(ng_name));	
	TRACE_ENTER2("'%s', inv:'%llu', op:'%llu'",osaf_extended_name_borrow(ng_name),invocation,op_id);

	switch(op_id) {
	case SA_AMF_ADMIN_LOCK_INSTANTIATION:
		rc = check_ng_stability(ng);
		if (rc != SA_AIS_OK) {
			report_admin_op_error(avd_cb->immOiHandle, invocation,
					      rc, nullptr,
					      "Some entity is unstable, Operation cannot "
					      "be performed on '%s'"
					      "Check syslog for entity details", osaf_extended_name_borrow(ng_name));
			goto done;
		}
		if (ng->saAmfNGAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
					      "'%s' Invalid Admin Operation LOCK INSTANTIATION in state %s",
					      ng->name.c_str(), avd_adm_state_name[ng->saAmfNGAdminState]);
			goto done;
		}
		if (ng->saAmfNGAdminState != SA_AMF_ADMIN_LOCKED) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					      "'%s' Invalid Admin Operation LOCK_INSTANTIATION in state %s",
					      ng->name.c_str(), avd_adm_state_name[ng->saAmfNGAdminState]);
			goto done;
		}
		rc = check_red_model_service_outage(ng);
		if (rc != SA_AIS_OK) {
			report_admin_op_error(avd_cb->immOiHandle, invocation,
					      SA_AIS_ERR_NOT_SUPPORTED, nullptr,
					      "SUs of unsupported red models hosted on '%s'"
					      "Check syslog for entity details", osaf_extended_name_borrow(ng_name));
			goto done;
		}
		ng->admin_ng_pend_cbk.invocation = invocation;
		ng->admin_ng_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(op_id);
		ng->node_oper_list.clear();

		avd_ng_admin_state_set(ng, SA_AMF_ADMIN_LOCKED_INSTANTIATION);
		for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
		     iter != ng->saAmfNGNodeList.end(); ++iter) {
			AVD_AVND *node = avd_node_get(*iter);
			node->su_cnt_admin_oper = 0;
			node->admin_ng = ng;
			if (node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED)
				continue;
			node->node_sus_termstate_set(true);
			node_admin_state_set(node, SA_AMF_ADMIN_LOCKED_INSTANTIATION);
		}
		for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
		     iter != ng->saAmfNGNodeList.end(); ++iter) {
			AVD_AVND *node = avd_node_get(*iter);
			if (node->node_info.member == false) {
				LOG_NO("'%s' LOCK_INSTANTIATION: CLM node is not member", node->name.c_str());
				continue;
			}
			if (node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED) {
				LOG_NO("'%s' LOCK_INSTANTIATION: AMF node oper state disabled", node->name.c_str());
				continue;
			}
			if (node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
				LOG_NO("'%s' LOCK_INSTANTIATION: AMF node admin state is not LOCKED", node->name.c_str());
				continue;
			}
			avd_node_admin_lock_instantiation(node);
			TRACE("node:'%s', su_cnt_admin_oper:%u",
					node->name.c_str(), node->su_cnt_admin_oper);
			if (node->su_cnt_admin_oper > 0)
				ng->node_oper_list.insert(node->name);
		}
		TRACE("node_oper_list size:%u",ng->oper_list_size());
		if (ng->node_oper_list.empty())
			ng_complete_admin_op(ng, SA_AIS_OK);
		break;
	case SA_AMF_ADMIN_UNLOCK_INSTANTIATION:
		rc = check_ng_stability(ng);
		if (rc != SA_AIS_OK) {
			report_admin_op_error(avd_cb->immOiHandle, invocation,
					      rc, nullptr,
					      "Some entity is unstable, Operation cannot "
					      "be performed on '%s'"
					      "Check syslog for entity details", osaf_extended_name_borrow(ng_name));
			goto done;
		}
		if (ng->saAmfNGAdminState == SA_AMF_ADMIN_LOCKED) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
					      "'%s' Already in LOCKED state", ng->name.c_str());
			goto done;
		}

		if (ng->saAmfNGAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					      "'%s' Invalid Admin Operation UNLOCK_INSTANTIATION in state %s",
					      ng->name.c_str(), avd_adm_state_name[ng->saAmfNGAdminState]);
			goto done;
		}
		rc = check_red_model_service_outage(ng);
		if (rc != SA_AIS_OK) {
			report_admin_op_error(avd_cb->immOiHandle, invocation,
					      SA_AIS_ERR_NOT_SUPPORTED, nullptr,
					      "SUs of unsupported red models hosted on '%s'"
					      "Check syslog for entity details", osaf_extended_name_borrow(ng_name));
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
			if (node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION)
				continue;
			node->node_sus_termstate_set(false);  
			node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
		}
		ng_admin_unlock_inst(ng);
		if (ng->node_oper_list.empty())
			ng_complete_admin_op(ng, SA_AIS_OK);
		break;
	case SA_AMF_ADMIN_LOCK:
		rc = check_ng_stability(ng);	
		if (rc != SA_AIS_OK) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, 
					rc, nullptr,
					"Some entity is unstable, Operation cannot "
					"be performed on '%s'"
					"Check syslog for entity details", osaf_extended_name_borrow(ng_name));
			goto done;
		}
		if (ng->saAmfNGAdminState == SA_AMF_ADMIN_LOCKED) {
                        report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
                                        "'%s' Already in LOCKED state", ng->name.c_str());
                        goto done;
                }
		if (ng->saAmfNGAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"'%s' Invalid Admin Operation LOCK in state %s",
					ng->name.c_str(), avd_adm_state_name[ng->saAmfNGAdminState]);
			goto done;
		}
		rc = check_red_model_service_outage(ng);
		if (rc != SA_AIS_OK) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, 
					SA_AIS_ERR_NOT_SUPPORTED, nullptr,
					"SUs of unsupported red models hosted on '%s'"
					"Check syslog for entity details", osaf_extended_name_borrow(ng_name));
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
			if (node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION)
				continue;
			node->admin_ng = ng;
			node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
		}
		for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
				iter != ng->saAmfNGNodeList.end(); ++iter) {
			AVD_AVND *node = avd_node_get(*iter);
			if (node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION)
				continue;
			ng_node_lock_and_shutdown(node);
		}
		if (ng->node_oper_list.empty())
			ng_complete_admin_op(ng, SA_AIS_OK);
		break;
	case SA_AMF_ADMIN_SHUTDOWN:
		rc = check_ng_stability(ng);	
		if (rc != SA_AIS_OK) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, 
					rc, nullptr,
					"Some entity is unstable, Operation cannot "
					"be performed on '%s'"
					"Check syslog for entity details", osaf_extended_name_borrow(ng_name));
			goto done;
		}
		if (ng->saAmfNGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
					"'%s' Already in SHUTTING DOWN state", ng->name.c_str());
			goto done;
		}
		if (ng->saAmfNGAdminState != SA_AMF_ADMIN_UNLOCKED) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"'%s' Invalid Admin Operation SHUTDOWN in state %s",
					ng->name.c_str(), avd_adm_state_name[ng->saAmfNGAdminState]);
			goto done;
		}
		rc = check_red_model_service_outage(ng);
		if (rc != SA_AIS_OK) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, 
					SA_AIS_ERR_NOT_SUPPORTED, nullptr,
					"SUs of unsupported red models hosted on '%s'"
					"Check syslog for entity details", osaf_extended_name_borrow(ng_name));
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
			if (node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION)
				continue;
			node->admin_ng = ng;
			node_admin_state_set(node, SA_AMF_ADMIN_SHUTTING_DOWN);
		}
		for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
				iter != ng->saAmfNGNodeList.end(); ++iter) {
			AVD_AVND *node = avd_node_get(*iter);
			if (node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION)
				continue;
			ng_node_lock_and_shutdown(node);
			if (node->su_cnt_admin_oper == 0)
				node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
		}
		if (ng->node_oper_list.empty())  {
			avd_ng_admin_state_set(ng, SA_AMF_ADMIN_LOCKED);
			for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
					iter != ng->saAmfNGNodeList.end(); ++iter) {
				AVD_AVND *node = avd_node_get(*iter);
				if (node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION)
					continue;
				node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
			}
			ng_complete_admin_op(ng, SA_AIS_OK);
		}
		break;
	case SA_AMF_ADMIN_UNLOCK:
		rc = check_ng_stability(ng);
		if (rc != SA_AIS_OK) {
			report_admin_op_error(avd_cb->immOiHandle, invocation,
					rc, nullptr,
					"Some entity is unstable, Operation cannot "
					"be performed on '%s'"
					"Check syslog for entity details", osaf_extended_name_borrow(ng_name));
			goto done;
		}
		if (ng->saAmfNGAdminState == SA_AMF_ADMIN_UNLOCKED) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
					"'%s' Already in UNLOCKED state", ng->name.c_str());
                        goto done;
		}
		if (ng->saAmfNGAdminState != SA_AMF_ADMIN_LOCKED) {
			report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"'%s' Invalid Admin Operation UNLOCK in state %s",
                                        ng->name.c_str(), avd_adm_state_name[ng->saAmfNGAdminState]);
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
		report_admin_op_error(avd_cb->immOiHandle, invocation, SA_AIS_ERR_NOT_SUPPORTED, nullptr,
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
	avd_class_impl_set("SaAmfNodeGroup", nullptr, ng_admin_op_cb, ng_ccb_completed_cb,
			ng_ccb_apply_cb);
}

/*
 * @brief: Restore nodegroup related variables which are used in nodegroup
 *         operations before headless. The main variables are @admin_ng,
 *         @ng_using_saAmfSGAdminState, and the saAmfSGAdminState which
 *         is borrowed by nodegroup operation on 2N SG
 * @param: control block, and susi is being read after headless
 * @return: void
 */

void avd_ng_restore_headless_states(AVD_CL_CB *cb, struct avd_su_si_rel_tag* susi) {

	TRACE_ENTER();
	osafassert(cb->scs_absence_max_duration > 0);
	// In order to restore nodegroup operation, AMF needs to know exactly
	// whether nodegroup operation was running during headless up on @susi
	// If susi is in QUIESCED/QUIESCING or being removed while its related
	// entities su, si, sg are not in LOCKED and SHUTTING_DOWN, that means
	// either node or nodegroup MUST be in LOCKED or SHUTTING_DOWN.
	// In case of SHUTTING_DOWN saAmfNGAdminState, that's enough to know
	// a nodegroup operation was running. However, if saAmfNGAdminState is
	// in LOCKED, this case is an ambiguity of locking a node.
	// The reason of differentiation of locking a node or nodegroup is because
	// 2N SG uses both AVD_SG_FSM_SG_ADMIN and AVD_SG_FSM_SU_OPER for nodegroup
	// operation while AVD_SG_FSM_SU_OPER is only used for node operation.
	// When 2N SG uses AVD_SG_FSM_SG_ADMIN for nodegroup, the saAmfSGAdminState
	// is borrowed (but not updated to IMM) to run the admin operation sequence.
	// Therefore, after headless if AVD_SG_FSM_SG_ADMIN was being used for nodegroup
	// then saAmfSGAdminState also needs to be set.

	// Make sure su, si, sg are not in LOCKED, SHUTTING_DOWN
	if (susi->su->saAmfSUAdminState != SA_AMF_ADMIN_LOCKED &&
			susi->su->saAmfSUAdminState != SA_AMF_ADMIN_SHUTTING_DOWN &&
			susi->si->saAmfSIAdminState != SA_AMF_ADMIN_LOCKED &&
			susi->si->saAmfSIAdminState != SA_AMF_ADMIN_SHUTTING_DOWN &&
			susi->su->sg_of_su->saAmfSGAdminState != SA_AMF_ADMIN_LOCKED &&
			susi->su->sg_of_su->saAmfSGAdminState != SA_AMF_ADMIN_SHUTTING_DOWN) {

		// susi are in state transition
		if (susi->state == SA_AMF_HA_QUIESCED || susi->state == SA_AMF_HA_QUIESCING ||
				susi->fsm == AVD_SU_SI_STATE_UNASGN) {

	    	for (std::map<std::string, AVD_AMF_NG*>::const_iterator it = nodegroup_db->begin();
	    		it != nodegroup_db->end(); it++) {
	    		AVD_AMF_NG *ng = it->second;
	    		for (std::set<std::string>::const_iterator iter = ng->saAmfNGNodeList.begin();
	    				iter != ng->saAmfNGNodeList.end(); ++iter) {
	    			AVD_AVND *node = avd_node_get(*iter);

	    			if (susi->su->su_on_node == node) {
	    				// Nodegroup was shutting down, also needs to check whether nodegroup
	    				// operation was borrowing SG ADMIN FSM code (for 2N only)
	    				if (ng->saAmfNGAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
	    					node->admin_ng = ng;
	    					if (susi->su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SG_ADMIN) {
	    						osafassert(susi->su->sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL);
								susi->su->sg_of_su->ng_using_saAmfSGAdminState = true;
								susi->su->sg_of_su->saAmfSGAdminState = ng->saAmfNGAdminState;
								m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi->su->sg_of_su, AVSV_CKPT_SG_ADMIN_STATE);
	    					}
	    				}
	    				// Node group was LOCKED, AMFD treat nodegroup and node operation in the same view,
	    				// except nodegroup was borrowing SG ADMIN FSM. This is because the other case that
	    				// nodegroup operation was only SG SU_OPER FSM, which is the same as of node operation
	    				if (ng->saAmfNGAdminState == SA_AMF_ADMIN_LOCKED) {
	    					if(susi->su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SG_ADMIN) {
	    						osafassert(susi->su->sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL);
								node->admin_ng = ng;
								susi->su->sg_of_su->ng_using_saAmfSGAdminState = true;
								susi->su->sg_of_su->saAmfSGAdminState = ng->saAmfNGAdminState;
								m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi->su->sg_of_su, AVSV_CKPT_SG_ADMIN_STATE);
	    					}
	    				}
						TRACE("Restore nodegroup operation, SG(%s), ng_using_saAmfSGAdminState: %u, saAmfSGAdminState:%u",
								susi->su->sg_of_su->name.c_str(), susi->su->sg_of_su->ng_using_saAmfSGAdminState,
								susi->su->sg_of_su->saAmfSGAdminState);

	    			}
	    		}
	    	}
		}
	}
	TRACE_LEAVE();
}
