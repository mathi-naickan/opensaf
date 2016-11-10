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
#include <saflog.h>

#include <util.h>
#include <su.h>
#include <sutype.h>
#include <imm.h>
#include <ntf.h>
#include <proc.h>
#include <csi.h>
#include <cluster.h>
#include <algorithm>

AmfDb<std::string, AVD_SU> *su_db = nullptr;

void AVD_SU::initialize() {
	
	saAmfSURank = 0;
	saAmfSUHostNodeOrNodeGroup = "";
	saAmfSUFailover = false;
	saAmfSUFailover_configured = false;
	saAmfSUPreInstantiable = SA_FALSE;
	saAmfSUOperState = SA_AMF_OPERATIONAL_DISABLED;
	saAmfSUAdminState = SA_AMF_ADMIN_UNLOCKED;
	saAmfSuReadinessState = SA_AMF_READINESS_OUT_OF_SERVICE;
	saAmfSUPresenceState = SA_AMF_PRESENCE_UNINSTANTIATED;
	saAmfSUNumCurrActiveSIs = 0;
	saAmfSUNumCurrStandbySIs = 0;
	saAmfSURestartCount = 0;
	term_state = false;
	su_switch = AVSV_SI_TOGGLE_STABLE;
	su_is_external = false;
	su_act_state = 0;
	sg_of_su = nullptr;
	su_on_node = nullptr;
	list_of_susi = nullptr;
	list_of_comp = {};
	su_type = nullptr;
	su_list_su_type_next = nullptr; 
	saAmfSUType = "";
	saAmfSUMaintenanceCampaign = "";
	saAmfSUHostedByNode = "";
	pend_cbk.invocation = 0;
	pend_cbk.admin_oper = (SaAmfAdminOperationIdT)0;
	surestart = false;
}

AVD_SU::AVD_SU() {
	initialize();
}

AVD_SU::AVD_SU(const std::string& dn) :
	name(dn)
{
	initialize();
}

/**
 * Delete the SU from the model. Check point with peer. Send delete order
 * to node director.
 */
void AVD_SU::remove_from_model() {
	TRACE_ENTER2("'%s'", name.c_str());

	/* All the components under this SU should have been deleted
	 * by now, just do the sanity check to confirm it is done 
	 */
	osafassert(list_of_comp.empty() == true);
	osafassert(list_of_susi == nullptr);

	m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, this, AVSV_CKPT_AVD_SU_CONFIG);
	avd_node_remove_su(this);
	avd_sutype_remove_su(this);
	su_db->erase(name);
	avd_sg_remove_su(this);

	TRACE_LEAVE();
}

/**
 * @brief   gets the current no of assignmnents on a SU for a particular state
 *
 * @param[in] ha_state  
 *
 * @return returns current assignment cnt
 */
int AVD_SU::hastate_assignments_count(SaAmfHAStateT ha_state) {
	const AVD_SU_SI_REL *susi;
	int curr_assignment_cnt = 0;

	for (susi = list_of_susi; susi != nullptr; susi = susi->su_next) {
		if (susi->state == ha_state)
			curr_assignment_cnt++;
	}

	return curr_assignment_cnt;
}

void AVD_SU::remove_comp(AVD_COMP *comp) {
	AVD_SU *su_ref = comp->su;

	osafassert(su_ref != nullptr);

	if (comp->su != nullptr) {
		su_ref->list_of_comp.erase(std::remove(su_ref->list_of_comp.begin(),
                                           su_ref->list_of_comp.end(), comp), su_ref->list_of_comp.end());

		/* Marking SU referance pointer to nullptr, please dont use further in the routine */
		comp->su = nullptr;
	}

	bool old_preinst_value = saAmfSUPreInstantiable;
	bool curr_preinst_value = saAmfSUPreInstantiable;

	// check if preinst possibly is still true
	if (comp->is_preinstantiable() == true) {
		curr_preinst_value = false;
		for (const auto& i_comp : list_of_comp) {
			if ((i_comp->is_preinstantiable() == true) && (i_comp != comp)) {
				curr_preinst_value = true;
				break;
			}
		}
	}

	// if preinst has changed, update IMM and recalculate saAmfSUFailover
	if (curr_preinst_value != old_preinst_value) {
		set_saAmfSUPreInstantiable(curr_preinst_value);

		/* If SU becomes NPI then set saAmfSUFailover flag
		 * Sec 3.11.1.3.2 AMF-B.04.01 spec */
		if (saAmfSUPreInstantiable == false) {
			set_su_failover(true);
		}
	}
}

void AVD_SU::add_comp(AVD_COMP *comp) {
	list_of_comp.push_back(comp);
	std::sort(list_of_comp.begin(), list_of_comp.end(), [] (const AVD_COMP *c1, const AVD_COMP *c2) -> bool {
			const std::string c1_name(Amf::to_string(&c1->comp_info.name));
			const std::string c2_name(Amf::to_string(&c2->comp_info.name));
			if (c1->comp_info.inst_level < c2->comp_info.inst_level) return true;
			if (c1->comp_info.inst_level == c2->comp_info.inst_level) {
				if (compare_sanamet(c1_name, c2_name) < 0 ){
					return true;
				}
			}	
			return false;
		});

	/* Verify if the SUs preinstan value need to be changed */
	if (comp->is_preinstantiable() == true) {
		set_saAmfSUPreInstantiable(true);
	}
}

/**
 * Validate configuration attributes for an AMF SU object
 * @param su
 * 
 * @return int
 */
static int is_config_valid(const std::string& dn, const SaImmAttrValuesT_2 **attributes,
	const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;
	std::string sg_name;
	std::string su_host_node_or_nodegroup;
	std::string sg_su_host_nodegroup;
	SaNameT saAmfSUHostNodeOrNodeGroup = {0};
	SaNameT saAmfSUType = {0};
	SaBoolT abool;
	SaAmfAdminStateT admstate;
	std::string::size_type pos;
	SaUint32T saAmfSutIsExternal;
	AVD_SUTYPE *sut = nullptr;
	CcbUtilOperationData_t *tmp;
	AVD_SG *sg;

	if ((pos = dn.find(',')) == std::string::npos) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn.c_str());
		return 0;
	}

	if (dn.compare(pos + 1, 6, "safSg=") != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", dn.substr(pos + 1).c_str(), dn.c_str());
		return 0;
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUType"), attributes, 0, &saAmfSUType);
	osafassert(rc == SA_AIS_OK);

	if ((sut = sutype_db->find(Amf::to_string(&saAmfSUType))) != nullptr) {
		saAmfSutIsExternal = sut->saAmfSutIsExternal;
	} else {
		/* SU type does not exist in current model, check CCB if passed as param */
		if (opdata == nullptr) {
			report_ccb_validation_error(opdata, "'%s' does not exist in model", osaf_extended_name_borrow(&saAmfSUType));
			return 0;
		}

		if ((tmp = ccbutil_getCcbOpDataByDN(opdata->ccbId, &saAmfSUType)) == nullptr) {
			report_ccb_validation_error(opdata, "'%s' does not exist in existing model or in CCB",
				osaf_extended_name_borrow(&saAmfSUType));
			return 0;
		}

		rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSutIsExternal"), tmp->param.create.attrValues, 0, &saAmfSutIsExternal);
		osafassert(rc == SA_AIS_OK);
	}

	/* Validate that a configured node or node group exist */
	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUHostNodeOrNodeGroup"), attributes, 0, 
				&saAmfSUHostNodeOrNodeGroup) == SA_AIS_OK) {
		su_host_node_or_nodegroup = Amf::to_string(&saAmfSUHostNodeOrNodeGroup);
		if (su_host_node_or_nodegroup.compare(0, 11, "safAmfNode=") == 0) {
			if (avd_node_get(su_host_node_or_nodegroup) == nullptr) {
				if (opdata == nullptr) {
					report_ccb_validation_error(opdata, "'%s' does not exist in model", 
							su_host_node_or_nodegroup.c_str());
					return 0;
				}

				if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &saAmfSUHostNodeOrNodeGroup) == nullptr) {
					report_ccb_validation_error(opdata, 
							"'%s' does not exist in existing model or in CCB",
							su_host_node_or_nodegroup.c_str());
					return 0;
				}
			}
		}
		else {
			if (avd_ng_get(su_host_node_or_nodegroup) == nullptr) {
				if (opdata == nullptr) {
					report_ccb_validation_error(opdata, "'%s' does not exist in model",
							su_host_node_or_nodegroup.c_str());
					return 0;
				}

				if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &saAmfSUHostNodeOrNodeGroup) == nullptr) {
					report_ccb_validation_error(opdata,
							"'%s' does not exist in existing model or in CCB",
							su_host_node_or_nodegroup.c_str());
					return 0;
				}
			}
		}
	}

	/* Get value of saAmfSGSuHostNodeGroup */
	avsv_sanamet_init(dn, sg_name, "safSg");
	sg = sg_db->find(sg_name);
	if (sg) {
		sg_su_host_nodegroup = sg->saAmfSGSuHostNodeGroup;
	} else {
		if (opdata == nullptr) {
			report_ccb_validation_error(opdata, "SG '%s' does not exist in model", sg_name.c_str());
			return 0;
		}

		const SaNameTWrapper sg_namet(sg_name);
		if ((tmp = ccbutil_getCcbOpDataByDN(opdata->ccbId, sg_namet)) == nullptr) {
			report_ccb_validation_error(opdata, "SG '%s' does not exist in existing model or in CCB",
					sg_name.c_str());
			return 0;
		}

		SaNameT saAmfSGSuHostNodeGroup = {0};
		(void) immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGSuHostNodeGroup"),
			tmp->param.create.attrValues, 0, &saAmfSGSuHostNodeGroup);
		sg_su_host_nodegroup = Amf::to_string(&saAmfSGSuHostNodeGroup);
	}

	/* If its a local SU, node or nodegroup must be configured */
	if (!saAmfSutIsExternal &&
		su_host_node_or_nodegroup.find("safAmfNode=") == std::string::npos &&
		su_host_node_or_nodegroup.find("safAmfNodeGroup=") == std::string::npos &&
		sg_su_host_nodegroup.find("safAmfNodeGroup=") == std::string::npos ) {
		
		report_ccb_validation_error(opdata, "node or node group configuration is missing for '%s'", dn.c_str());
		return 0;
	}

	/*
	* "It is an error to define the saAmfSUHostNodeOrNodeGroup attribute for an external
	* service unit".
	*/
	if (saAmfSutIsExternal &&
		(su_host_node_or_nodegroup.find("safAmfNode=") != std::string::npos ||
		su_host_node_or_nodegroup.find("safAmfNodeGroup=") != std::string::npos ||
		sg_su_host_nodegroup.find("safAmfNodeGroup=") != std::string::npos)) {

		report_ccb_validation_error(opdata, "node or node group configured for external SU '%s'", dn.c_str());
		return 0;
	}

	/*
	* "If node groups are configured for both the service units of a service group and the
	* service group, the nodes contained in the node group for the service unit can only be
	* a subset of the nodes contained in the node group for the service group. If a node is
	* configured for a service unit, it must be a member of the node group for the service
	* group, if configured."
        */
	if (su_host_node_or_nodegroup.find("safAmfNodeGroup=") != std::string::npos &&
		sg_su_host_nodegroup.find("safAmfNodeGroup=") != std::string::npos) {
		AVD_AMF_NG *ng_of_su, *ng_of_sg;

		ng_of_su = avd_ng_get(su_host_node_or_nodegroup);
		if (ng_of_su == nullptr) {
			report_ccb_validation_error(opdata, "Invalid saAmfSUHostNodeOrNodeGroup '%s' for '%s'",
				su_host_node_or_nodegroup.c_str(), dn.c_str());
			return 0;
		}

		ng_of_sg = avd_ng_get(sg_su_host_nodegroup);
		if (ng_of_su == nullptr) {
			report_ccb_validation_error(opdata, "Invalid saAmfSGSuHostNodeGroup '%s' for '%s'",
				sg_su_host_nodegroup.c_str(), dn.c_str());
			return 0;
		}

		if (ng_of_su->number_nodes() > ng_of_sg->number_nodes()) {
			report_ccb_validation_error(opdata, 
					"SU node group '%s' contains more nodes than the SG node group '%s'",
					su_host_node_or_nodegroup.c_str(), sg_su_host_nodegroup.c_str());
			return 0;
		}

		if (std::includes(ng_of_sg->saAmfNGNodeList.begin(), ng_of_sg->saAmfNGNodeList.end(),
			ng_of_su->saAmfNGNodeList.begin(), ng_of_su->saAmfNGNodeList.end()) == false) {
			report_ccb_validation_error(opdata, 
						"SU node group '%s' is not a subset of the SG node group '%s'",
						su_host_node_or_nodegroup.c_str(), sg_su_host_nodegroup.c_str());

			return 0;
		}		
	}

	// TODO maintenance campaign

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUFailover"), attributes, 0, &abool) == SA_AIS_OK) &&
	    (abool > SA_TRUE)) {
		report_ccb_validation_error(opdata, "Invalid saAmfSUFailover %u for '%s'", abool, dn.c_str());
		return 0;
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUAdminState"), attributes, 0, &admstate) == SA_AIS_OK) &&
	    !avd_admin_state_is_valid(admstate, opdata)) {
		report_ccb_validation_error(opdata, "Invalid saAmfSUAdminState %u for '%s'", admstate, dn.c_str());
		return 0;
	}

	return 1;
}

/**
 * Create a new SU object and initialize its attributes from the attribute list.
 * 
 * @param su_name
 * @param attributes
 * 
 * @return AVD_SU*
 */
static AVD_SU *su_create(const std::string& dn, const SaImmAttrValuesT_2 **attributes)
{
	int rc = -1;
	AVD_SU *su;
	AVD_SUTYPE *sut;
	SaAisErrorT error;
	SaNameT temp_name;

	TRACE_ENTER2("'%s'", dn.c_str());

	/*
	** If called at new active at failover, the object is found in the DB
	** but needs to get configuration attributes initialized.
	*/
	if ((su = su_db->find(dn)) == nullptr) {
		su = new AVD_SU(dn);
	} else
		TRACE("already created, refreshing config...");

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUType"), attributes, 0, &temp_name);
	osafassert(error == SA_AIS_OK);
	su->saAmfSUType = Amf::to_string(&temp_name);

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSURank"), attributes, 0, &su->saAmfSURank) != SA_AIS_OK) {
		/* Empty, assign default value (highest number => lowest rank) */
		su->saAmfSURank = ~0U;
	}

	/* If 0 (zero), treat as lowest possible rank. Should be a positive integer */
	if (su->saAmfSURank == 0) {
		su->saAmfSURank = ~0U;
		TRACE("'%s' saAmfSURank auto-changed to lowest", su->name.c_str());
	}

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUHostedByNode"), attributes, 0, &temp_name);
	if (error == SA_AIS_OK) {
		su->saAmfSUHostedByNode = Amf::to_string(&temp_name);
		TRACE("saAmfSUHostedByNode set to %s", su->saAmfSUHostedByNode.c_str());
	} else {
		su->saAmfSUHostedByNode = "";
		TRACE("saAmfSUHostedByNode set to blank");
	}

		
	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUHostNodeOrNodeGroup"), attributes, 0, &temp_name);
	if (error == SA_AIS_OK) {
		su->saAmfSUHostNodeOrNodeGroup = Amf::to_string(&temp_name);
		TRACE("saAmfSUHostNodeOrNodeGroup set to %s", su->saAmfSUHostNodeOrNodeGroup.c_str());
	} else {
		su->saAmfSUHostNodeOrNodeGroup = "";
		TRACE("saAmfSUHostNodeOrNodeGroup set to blank");
	}

	
	if ((sut = sutype_db->find(su->saAmfSUType)) == nullptr) {
		LOG_ER("saAmfSUType '%s' does not exist", su->saAmfSUType.c_str());
		goto done;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUFailover"), attributes, 0, &su->saAmfSUFailover) != SA_AIS_OK) {
		su->saAmfSUFailover = static_cast<bool>(sut->saAmfSutDefSUFailover);
		su->saAmfSUFailover_configured = false;
	}
	else 
		su->saAmfSUFailover_configured = true;

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUMaintenanceCampaign"), attributes, 0, &temp_name);
	if (error == SA_AIS_OK) {
		su->saAmfSUMaintenanceCampaign = Amf::to_string(&temp_name);
		TRACE("saAmfSUMaintenanceCampaign set to %s", su->saAmfSUMaintenanceCampaign.c_str());
	} else {
		su->saAmfSUMaintenanceCampaign = "";
		TRACE("saAmfSUMaintenanceCampaign set to blank");
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUAdminState"), attributes, 0, &su->saAmfSUAdminState) != SA_AIS_OK)
		su->saAmfSUAdminState = SA_AMF_ADMIN_UNLOCKED;

	rc = 0;

done:
	if (rc != 0) {
		delete su;
		su = nullptr;
	}

	TRACE_LEAVE();
	return su;
}

/**
 * Map SU to a node. A node is selected using a static load balancing scheme. Nodes are selected
 * from a node group in alphabetical order.
 * @param su
 * 
 * @return AVD_AVND*
 */
static AVD_AVND *map_su_to_node(AVD_SU *su)
{
	AVD_AMF_NG *ng = nullptr;
	AVD_SU *su_temp = nullptr;
	AVD_AVND *node = nullptr;
	std::set<std::string>::const_iterator node_iter;
	std::vector<AVD_SU*>::const_iterator su_iter;
	std::vector<AVD_SU*> *su_list = nullptr;

	TRACE_ENTER2("'%s'", su->name.c_str());

	/* If node is configured in SU we are done */
	if (su->saAmfSUHostNodeOrNodeGroup.find("safAmfNode=") != std::string::npos) {
		node = avd_node_get(su->saAmfSUHostNodeOrNodeGroup);
		if (node == nullptr) {
			LOG_WA("Could not find: %s in su: %s", su->saAmfSUHostNodeOrNodeGroup.c_str(), su->name.c_str());
			TRACE_LEAVE();
			return node;
		}
		goto done;
	}

	/* A node group configured in the SU is prioritized before the same in SG */
	if (su->saAmfSUHostNodeOrNodeGroup.find("safAmfNodeGroup=") != std::string::npos) {
		ng = avd_ng_get(su->saAmfSUHostNodeOrNodeGroup);
	} else if (su->sg_of_su->saAmfSGSuHostNodeGroup.find("safAmfNodeGroup=") != std::string::npos) {
		ng = avd_ng_get(su->sg_of_su->saAmfSGSuHostNodeGroup);
	}

	osafassert(ng);

	/* Find a node in the group that doesn't have a SU in same SG mapped to it already */
	for (node_iter = ng->saAmfNGNodeList.begin();
		node_iter != ng->saAmfNGNodeList.end();
		node_iter++) {

		node = avd_node_get(*node_iter);
		osafassert(node);

		if (su->sg_of_su->sg_ncs_spec == true) {
			su_list = &node->list_of_ncs_su;
		} else {
			su_list = &node->list_of_su;
		}

		for (su_iter = su_list->begin(); su_iter != su_list->end(); ++su_iter) {
			su_temp = *su_iter;
			if (su_temp->sg_of_su == su->sg_of_su)
				break;
		}

		if (su_iter == su_list->end())
			goto done;
	}

	/* All nodes already have an SU mapped for the SG. Return a node in the node group. */
        node_iter = ng->saAmfNGNodeList.begin();
        if (node_iter != ng->saAmfNGNodeList.end()) {
		node = avd_node_get(*ng->saAmfNGNodeList.begin());
        } else {
		LOG_WA("%s is empty", su->saAmfSUHostNodeOrNodeGroup.c_str());
		TRACE_LEAVE();
		return nullptr;
	}
done:
	su->saAmfSUHostedByNode = node->name;
	TRACE_LEAVE2("hosted by %s", node->name.c_str());
	return node;
}

/**
 * Add SU to model
 * @param su
 */
static void su_add_to_model(AVD_SU *su)
{
	std::string dn;
	AVD_AVND *node;
	bool new_su = false;
	unsigned int rc;

	TRACE_ENTER2("%s", su->name.c_str());

	/* Check parent link to see if it has been added already */
	if (su->sg_of_su != nullptr) {
		TRACE("already added");
		goto done;
	}

	/* Determine of the SU is added now, if so msg to amfnd needs to be sent */
	if (su_db->find(su->name) == nullptr)
		new_su = true;

	avsv_sanamet_init(su->name, dn, "safSg");

	/*
	** Refresh the SG reference, by now it must exist.
	** An SU can be created (on the standby) from the checkpointing logic.
	** In that case the SG reference could now be nullptr.
	*/
	su->sg_of_su = sg_db->find(dn);
	osafassert(su->sg_of_su);

	if (su_db->find(su->name) == nullptr) {
		TRACE("su_db inserted %s", su->name.c_str());
		rc = su_db->insert(su->name, su);
		osafassert(rc == NCSCC_RC_SUCCESS);
	}
	su->su_type = sutype_db->find(su->saAmfSUType);
	osafassert(su->su_type);
	avd_sutype_add_su(su);
	avd_sg_add_su(su);

	if (!su->su_is_external) {
		if (su->saAmfSUHostedByNode.empty() == true) {
			/* This node has not been mapped yet, do it */
			su->su_on_node = map_su_to_node(su);
		} else {
			/* Already mapped, setup the node link */
			su->su_on_node = avd_node_get(su->saAmfSUHostedByNode);
		}

		avd_node_add_su(su);
		node = su->su_on_node;
	} else {
		if (nullptr == avd_cb->ext_comp_info.ext_comp_hlt_check) {
			/* This is an external SU and we need to create the 
			   supporting info. */
			avd_cb->ext_comp_info.ext_comp_hlt_check = new AVD_AVND;
			avd_cb->ext_comp_info.local_avnd_node = avd_node_find_nodeid(avd_cb->node_id_avd);

			if (nullptr == avd_cb->ext_comp_info.local_avnd_node) {
				LOG_ER("%s: avd_node_find_nodeid failed %x", __FUNCTION__, avd_cb->node_id_avd);
				avd_sg_remove_su(su);
				LOG_ER("Avnd Lookup failure, node id %u", avd_cb->node_id_avd);
			}
		}

		node = avd_cb->ext_comp_info.local_avnd_node;
	}

	m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(avd_cb, su, AVSV_CKPT_AVD_SU_CONFIG);

	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) 
		goto done;

	if (new_su == true) {
		if ((node->node_state == AVD_AVND_STATE_PRESENT) ||
		    (node->node_state == AVD_AVND_STATE_NO_CONFIG) ||
		    (node->node_state == AVD_AVND_STATE_NCS_INIT)) {

			if (avd_snd_su_msg(avd_cb, su) != NCSCC_RC_SUCCESS) {
				avd_node_remove_su(su);
				avd_sg_remove_su(su);

				LOG_ER("%s: avd_snd_su_msg failed %s", __FUNCTION__, su->name.c_str());
				goto done;
			}

			su->set_oper_state(SA_AMF_OPERATIONAL_ENABLED);
		} else
			su->set_oper_state(SA_AMF_OPERATIONAL_DISABLED);
	}

done:
	avd_saImmOiRtObjectUpdate(su->name, "saAmfSUOperState",
			SA_IMM_ATTR_SAUINT32T, &su->saAmfSUOperState);

	avd_saImmOiRtObjectUpdate(su->name, "saAmfSUPreInstantiable",
		SA_IMM_ATTR_SAUINT32T, &su->saAmfSUPreInstantiable);

	const SaNameTWrapper hosted_by_node(su->saAmfSUHostedByNode);
	avd_saImmOiRtObjectUpdate(su->name, "saAmfSUHostedByNode",
		SA_IMM_ATTR_SANAMET, (void*)static_cast<const SaNameT*>(hosted_by_node));

	avd_saImmOiRtObjectUpdate(su->name, "saAmfSUPresenceState",
		SA_IMM_ATTR_SAUINT32T, &su->saAmfSUPresenceState);

	avd_saImmOiRtObjectUpdate(su->name, "saAmfSUReadinessState",
		SA_IMM_ATTR_SAUINT32T, &su->saAmfSuReadinessState);

	TRACE_LEAVE();
}

SaAisErrorT avd_su_config_get(const std::string& sg_name, AVD_SG *sg)
{
	SaAisErrorT error, rc;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT tmp_su_name;
	std::string su_name;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfSU";
	AVD_SU *su;
	SaImmAttrNameT configAttributes[] = {
		const_cast<SaImmAttrNameT>("saAmfSUType"),
		const_cast<SaImmAttrNameT>("saAmfSURank"),
		const_cast<SaImmAttrNameT>("saAmfSUHostedByNode"),
		const_cast<SaImmAttrNameT>("saAmfSUHostNodeOrNodeGroup"),
		const_cast<SaImmAttrNameT>("saAmfSUFailover"),
		const_cast<SaImmAttrNameT>("saAmfSUMaintenanceCampaign"),
		const_cast<SaImmAttrNameT>("saAmfSUAdminState"),
		nullptr
	};

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_o2(avd_cb->immOmHandle, sg_name.c_str(), SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR, &searchParam,
		configAttributes, &searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("%s: saImmOmSearchInitialize_2 failed: %u", __FUNCTION__, error);
		goto done1;
	}

	while ((rc = immutil_saImmOmSearchNext_2(searchHandle, &tmp_su_name,
					(SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK) {
		su_name = Amf::to_string(&tmp_su_name);
		if (!is_config_valid(su_name, attributes, nullptr)) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}

		if ((su = su_create(su_name, attributes)) == nullptr) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}

		su_add_to_model(su);

		if (avd_comp_config_get(su_name, su) != SA_AIS_OK) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}
	}

	osafassert(rc == SA_AIS_ERR_NOT_EXIST);
	error = SA_AIS_OK;

 done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
 done1:
	TRACE_LEAVE2("%u", error);
	return error;
}

void AVD_SU::set_pres_state(SaAmfPresenceStateT pres_state) {
	if (saAmfSUPresenceState == pres_state)
		return;
	else if ((pres_state == SA_AMF_PRESENCE_TERMINATION_FAILED) &&
			(su_on_node->saAmfNodeFailfastOnTerminationFailure == true) &&
			(sg_of_su->saAmfSGAutoRepair == true) &&
			(su_on_node->saAmfNodeAutoRepair == true)) 
		/* According to AMF B.04.01 Section 4.8 Page 214 if user configures
		   saAmfNodeFailfastOnTerminationFailure = true, AMF has to perform
		   node failfast recovery action. So mark SU to SA_AMF_PRESENCE_TERMINATION_FAILED 
		   only if nodefailfast is not possible.
		 */
		return;
	else if ((pres_state == SA_AMF_PRESENCE_INSTANTIATION_FAILED) &&
			(su_on_node->saAmfNodeFailfastOnInstantiationFailure == true) &&
			(sg_of_su->saAmfSGAutoRepair == true) &&
			(su_on_node->saAmfNodeAutoRepair == true)) 
		/* According to AMF B.04.01 Section 4.6 Page 212 if user configures
		   saAmfNodeFailfastOnInstantiationFailure = true, AMF has to perform
		   node failfast recovery action. So mark SU to SA_AMF_PRESENCE_INSTANTIATION_FAILED
		   only if nodefailfast is not possible.
		 */
		return;

	osafassert(pres_state <= SA_AMF_PRESENCE_TERMINATION_FAILED);
	TRACE_ENTER2("'%s' %s => %s", name.c_str(),
				 avd_pres_state_name[saAmfSUPresenceState],
				 avd_pres_state_name[pres_state]);

	SaAmfPresenceStateT old_state = saAmfSUPresenceState;
	saAmfSUPresenceState = pres_state;
	/* only log for certain changes, see notifications in spec */
	if (pres_state == SA_AMF_PRESENCE_UNINSTANTIATED ||
	    pres_state == SA_AMF_PRESENCE_INSTANTIATED ||
	    pres_state == SA_AMF_PRESENCE_RESTARTING) {

		saflog(LOG_NOTICE, amfSvcUsrName, "%s PresenceState %s => %s",
			name.c_str(), avd_pres_state_name[old_state],
			avd_pres_state_name[saAmfSUPresenceState]);

		avd_send_su_pres_state_chg_ntf(name, old_state,
			saAmfSUPresenceState);
	}
	avd_saImmOiRtObjectUpdate(name, "saAmfSUPresenceState",
		SA_IMM_ATTR_SAUINT32T, &saAmfSUPresenceState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_PRES_STATE);
	if ((saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATED) &&
			(get_surestart() == true))
		set_surestart(false);
	//Section 3.2.1.4 Readiness State: presence state affects readiness state of  a PI SU.
	if (saAmfSUPreInstantiable == true) {
		if (((saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATED) ||
				(saAmfSUPresenceState == SA_AMF_PRESENCE_RESTARTING)) &&
				(is_in_service() == true))
			set_readiness_state(SA_AMF_READINESS_IN_SERVICE);	
		else {
			set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);	
		} 
	}	
	TRACE_LEAVE();
}

void AVD_SU::set_oper_state(SaAmfOperationalStateT oper_state) {
	SaAmfOperationalStateT old_state = saAmfSUOperState;

	if (saAmfSUOperState == oper_state)
		return;

	osafassert(oper_state <= SA_AMF_OPERATIONAL_DISABLED);
	TRACE_ENTER2("'%s' %s => %s", name.c_str(),
		avd_oper_state_name[saAmfSUOperState],
		avd_oper_state_name[oper_state]);

	saflog(LOG_NOTICE, amfSvcUsrName, "%s OperState %s => %s", name.c_str(),
		avd_oper_state_name[saAmfSUOperState],
		avd_oper_state_name[oper_state]);

	saAmfSUOperState = oper_state;

	avd_send_oper_chg_ntf(name, SA_AMF_NTFID_SU_OP_STATE, old_state,
		saAmfSUOperState);

	avd_saImmOiRtObjectUpdate(name, "saAmfSUOperState",
		SA_IMM_ATTR_SAUINT32T, &saAmfSUOperState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_OPER_STATE);

	TRACE_LEAVE();
}

/**
 * Set readiness state in SU and calculate readiness state for all contained components
 * @param su
 * @param readiness_state
 */
void AVD_SU::set_readiness_state(SaAmfReadinessStateT readiness_state) {
	if (saAmfSuReadinessState == readiness_state)
		return;

	osafassert(readiness_state <= SA_AMF_READINESS_STOPPING);
	TRACE_ENTER2("'%s' %s", name.c_str(),
		avd_readiness_state_name[readiness_state]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s ReadinessState %s => %s",
		name.c_str(),
		avd_readiness_state_name[saAmfSuReadinessState],
		avd_readiness_state_name[readiness_state]);
	saAmfSuReadinessState = readiness_state;
	if (get_surestart() == false)
		avd_saImmOiRtObjectUpdate(name, "saAmfSUReadinessState",
				SA_IMM_ATTR_SAUINT32T, &saAmfSuReadinessState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_READINESS_STATE);

	/* Since Su readiness state has changed, we need to change it for all the
	 * component in this SU.*/

	for (const auto& comp : list_of_comp) {
		SaAmfReadinessStateT saAmfCompReadinessState;
		if ((saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
				(comp->saAmfCompOperState == SA_AMF_OPERATIONAL_ENABLED)) {
			saAmfCompReadinessState = SA_AMF_READINESS_IN_SERVICE;
		} else if((saAmfSuReadinessState == SA_AMF_READINESS_STOPPING) &&
				(comp->saAmfCompOperState == SA_AMF_OPERATIONAL_ENABLED)) {
			saAmfCompReadinessState = SA_AMF_READINESS_STOPPING;
		} else
			saAmfCompReadinessState = SA_AMF_READINESS_OUT_OF_SERVICE;

		comp->avd_comp_readiness_state_set(saAmfCompReadinessState);
	}

	TRACE_LEAVE();
}

void AVD_SU::set_admin_state(SaAmfAdminStateT admin_state) {
	SaAmfAdminStateT old_state = saAmfSUAdminState;

	osafassert(admin_state <= SA_AMF_ADMIN_SHUTTING_DOWN);
	TRACE_ENTER2("'%s' %s => %s", name.c_str(),
		avd_adm_state_name[old_state], avd_adm_state_name[admin_state]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s AdmState %s => %s", name.c_str(),
		avd_adm_state_name[saAmfSUAdminState],
		avd_adm_state_name[admin_state]);
	saAmfSUAdminState = admin_state;
	avd_saImmOiRtObjectUpdate(name, "saAmfSUAdminState",
		SA_IMM_ATTR_SAUINT32T, &saAmfSUAdminState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_ADMIN_STATE);
	avd_send_admin_state_chg_ntf(name, SA_AMF_NTFID_SU_ADMIN_STATE,
			old_state, saAmfSUAdminState);
	TRACE_LEAVE();
}

void AVD_SU::unlock(SaImmOiHandleT immoi_handle, SaInvocationT invocation) {
	bool is_oper_successful = true;

	TRACE_ENTER2("'%s'", name.c_str());
	set_admin_state(SA_AMF_ADMIN_UNLOCKED);

	/* Return as cluster timer haven't expired.*/
	if (avd_cb->init_state == AVD_INIT_DONE) {
		if (is_in_service() == true)
			set_readiness_state(SA_AMF_READINESS_IN_SERVICE);
		avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
		goto done;
	}

	if ((is_in_service() == true) || (sg_of_su->sg_ncs_spec == true)) {
		/* Reason for checking for MW component is that node oper state and
		 * SU oper state are marked enabled after they gets assignments.
		 * So, we can't check compatibility with is_in_service() for them.
		 */
		set_readiness_state(SA_AMF_READINESS_IN_SERVICE);
		if (sg_of_su->su_insvc(avd_cb, this) != NCSCC_RC_SUCCESS)
			is_oper_successful = false;

		avd_sg_app_su_inst_func(avd_cb, sg_of_su);
	} else
		LOG_IN("SU '%s' is not in service", name.c_str());
	avd_sg_app_su_inst_func(avd_cb, sg_of_su);

	if (is_oper_successful == true) {
		if (sg_of_su->sg_fsm_state == AVD_SG_FSM_SG_REALIGN ) {
			pend_cbk.admin_oper = SA_AMF_ADMIN_UNLOCK;
			pend_cbk.invocation = invocation;
		} else {
			avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
		}
	} else {
		set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
		set_admin_state(SA_AMF_ADMIN_LOCKED);
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_FAILED_OPERATION, nullptr,
				"SG redundancy model specific handler failed");
	}
done:
	TRACE_LEAVE();
}

void AVD_SU::lock(SaImmOiHandleT immoi_handle, SaInvocationT invocation,
                  SaAmfAdminStateT adm_state = SA_AMF_ADMIN_LOCKED) {
	AVD_AVND *node = get_node_ptr();
	SaAmfReadinessStateT back_red_state;
	SaAmfAdminStateT back_admin_state;
	bool is_oper_successful = true;

	TRACE_ENTER2("'%s'", name.c_str());

	if (sg_of_su->sg_ncs_spec == true &&
	    sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL &&
	    sg_of_su->list_of_su.size() > 2) {
		report_admin_op_error(immoi_handle, invocation,
				      SA_AIS_ERR_NOT_SUPPORTED, nullptr,
				      "Locking OpenSAF 2N SU is currently not "
				      "supported when more than two SUs are "
				      "configured");
		goto done;
	}

        /* Change the admin state to lock and return as cluster timer haven't expired.*/
	if (avd_cb->init_state == AVD_INIT_DONE) {
		set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
		set_admin_state(SA_AMF_ADMIN_LOCKED);
		avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
		goto done;
	}

	if (list_of_susi == nullptr) {
		set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
		set_admin_state(SA_AMF_ADMIN_LOCKED);
		avd_sg_app_su_inst_func(avd_cb, sg_of_su);
		avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
		goto done;
	}

	back_red_state = saAmfSuReadinessState;
	back_admin_state = saAmfSUAdminState;
	set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
	set_admin_state(adm_state);

	if (sg_of_su->su_admin_down(avd_cb, this, node) != NCSCC_RC_SUCCESS)
		is_oper_successful = false;

	avd_sg_app_su_inst_func(avd_cb, sg_of_su);

	if (is_oper_successful == true) {
		if ((sg_of_su->sg_fsm_state == AVD_SG_FSM_SG_REALIGN) ||
				(sg_of_su->sg_fsm_state == AVD_SG_FSM_SU_OPER)) {
			pend_cbk.admin_oper = (adm_state == SA_AMF_ADMIN_SHUTTING_DOWN) ?
				SA_AMF_ADMIN_SHUTDOWN : SA_AMF_ADMIN_LOCK;
			pend_cbk.invocation = invocation;
		} else {
			avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
		}
	} else {
		set_readiness_state(back_red_state);
		set_admin_state(back_admin_state);
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_FAILED_OPERATION, nullptr,
				"SG redundancy model specific handler failed");
	}

done:
	TRACE_LEAVE();
}

void AVD_SU::shutdown(SaImmOiHandleT immoi_handle, SaInvocationT invocation) {
	TRACE_ENTER2("'%s'", name.c_str());
	lock(immoi_handle, invocation, SA_AMF_ADMIN_SHUTTING_DOWN);
	TRACE_LEAVE();
}

void AVD_SU::lock_instantiation(SaImmOiHandleT immoi_handle,
                                SaInvocationT invocation) {
	AVD_AVND *node = get_node_ptr();

	TRACE_ENTER2("'%s'", name.c_str());

	/* For non-preinstantiable SU lock-inst is same as lock */
	if (saAmfSUPreInstantiable == false) {
		set_admin_state(SA_AMF_ADMIN_LOCKED_INSTANTIATION);
		avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
		goto done;
	}

	if (list_of_susi != nullptr) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
			"SIs still assigned to this SU '%s'", name.c_str());
		goto done;
	}

	if ((saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATING) ||
		(saAmfSUPresenceState == SA_AMF_PRESENCE_TERMINATING) ||
			(saAmfSUPresenceState == SA_AMF_PRESENCE_RESTARTING)) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
				"'%s' presence state is '%u'", name.c_str(), saAmfSUPresenceState);
		goto done;
	}

	if ((saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED) ||
		(saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATION_FAILED) ||
			(saAmfSUPresenceState == SA_AMF_PRESENCE_TERMINATION_FAILED)) {
		/* No need to terminate the SUs in Unins/Inst Failed/Term Failed state */
		set_admin_state(SA_AMF_ADMIN_LOCKED_INSTANTIATION);
		avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
		set_term_state(true);
		goto done;
	}

	if ( ( node->node_state == AVD_AVND_STATE_PRESENT )   ||
	     ( node->node_state == AVD_AVND_STATE_NO_CONFIG ) ||
	     ( node->node_state == AVD_AVND_STATE_NCS_INIT ) ) {
		/* When the SU will terminate then presence state change message will come
		   and so store the callback parameters to send response later on. */
		if (avd_snd_presence_msg(avd_cb, this, true) == NCSCC_RC_SUCCESS) {
			set_term_state(true);
			set_admin_state(SA_AMF_ADMIN_LOCKED_INSTANTIATION);
			pend_cbk.admin_oper = SA_AMF_ADMIN_LOCK_INSTANTIATION;
			pend_cbk.invocation = invocation;
			goto done;
		}
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
				"Internal error, could not send message to avnd");
		goto done;
	} else {
		set_admin_state(SA_AMF_ADMIN_LOCKED_INSTANTIATION);
		avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
		set_term_state(true);
	}
done:
	TRACE_LEAVE();
}

void AVD_SU::unlock_instantiation(SaImmOiHandleT immoi_handle,
                                  SaInvocationT invocation) {
	AVD_AVND *node = get_node_ptr();

	TRACE_ENTER2("'%s'", name.c_str());

	if (list_of_comp.empty() == true) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
				"There is no component configured for SU '%s'.", name.c_str());
		goto done;
	}

	/* For non-preinstantiable SU unlock-inst will not lead to its inst until unlock. */
	if (saAmfSUPreInstantiable == false) {
		/* Adjusting saAmfSGMaxActiveSIsperSU and saAmfSGMaxStandbySIsperSU
		   is required when SG and SUs/Comps are created in different CCBs. */
		avd_sg_adjust_config(sg_of_su);
		set_admin_state(SA_AMF_ADMIN_LOCKED);
		avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
		goto done;
	}

	if (saAmfSUPresenceState != SA_AMF_PRESENCE_UNINSTANTIATED) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
			"Can't instantiate '%s', whose presence state is '%u'", name.c_str(),
			saAmfSUPresenceState);
		goto done;
	}

	/* Middleware sus are not enabled until node joins. During
	   starting of opensaf, if mw su is locked-in and unlock-in
	   command is issued, su should get instantiated. */
	if (((node->node_state == AVD_AVND_STATE_PRESENT) ||
				(node->node_state == AVD_AVND_STATE_NO_CONFIG) ||
				(node->node_state == AVD_AVND_STATE_NCS_INIT)) &&
			((node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
			 (sg_of_su->saAmfSGAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION)) &&
			((saAmfSUOperState == SA_AMF_OPERATIONAL_ENABLED) ||
			 (sg_of_su->sg_ncs_spec == true)) &&
			(sg_of_su->saAmfSGNumPrefInserviceSUs > sg_instantiated_su_count(sg_of_su))) {
		/* When the SU will instantiate then prescence state change message will come
		   and so store the callback parameters to send response later on. */
		if (avd_snd_presence_msg(avd_cb, this, false) == NCSCC_RC_SUCCESS) {
			set_term_state(false);
			set_admin_state(SA_AMF_ADMIN_LOCKED);
			pend_cbk.admin_oper = SA_AMF_ADMIN_UNLOCK_INSTANTIATION;
			pend_cbk.invocation = invocation;
			goto done;
		}
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
				"Internal error, could not send message to avnd");
	} else {
		set_admin_state(SA_AMF_ADMIN_LOCKED);
		avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
		set_term_state(false);
	}

done:
		TRACE_LEAVE();
}

void AVD_SU::repaired(SaImmOiHandleT immoi_handle,
                      SaInvocationT invocation) {
	TRACE_ENTER2("'%s'", name.c_str());

	if (saAmfSUOperState == SA_AMF_OPERATIONAL_ENABLED) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_NO_OP, nullptr,
			"Admin repair request for '%s', op state already enabled", name.c_str());
		goto done;
	}

	if ((saAmfSUOperState == SA_AMF_OPERATIONAL_DISABLED) &&
			(su_on_node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED)) {
		/* This means that node on which this su is hosted, is absent. */
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
			"Admin repair request for '%s', hosting node'%s' is absent",
			name.c_str(), su_on_node->name.c_str());
		goto done;
	}

	/* forward the admin op req to the node director */
	if (avd_admin_op_msg_snd(name, AVSV_SA_AMF_SU, SA_AMF_ADMIN_REPAIRED,
			su_on_node) == NCSCC_RC_SUCCESS) {
		pend_cbk.admin_oper = SA_AMF_ADMIN_REPAIRED;
		pend_cbk.invocation = invocation;
	}
	else {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TIMEOUT, nullptr,
			"Admin op request send failed '%s'", name.c_str());
	}

done:
	TRACE_LEAVE();
}
/**
 * @brief       Handles RESTART admin operation on SU.
	        For a assigned non-restartable SU, it will only initiate
 *		switchover/removal of assignments only.  For a restartable SU or 
 *		a non-restartable unassigned SU, it will send RESTART request to AMFND.
 * @param[in]   immoi_handle. 
 * @param[in]   invocation.
 */
void AVD_SU::restart(SaImmOiHandleT immoi_handle,
                      SaInvocationT invocation) {
	TRACE_ENTER2("'%s'", name.c_str());

	pend_cbk.admin_oper = SA_AMF_ADMIN_RESTART;
	pend_cbk.invocation = invocation;
	if ((su_all_comps_restartable() == true) ||
			(is_any_non_restartable_comp_assigned() == false)) {
		if (avd_admin_op_msg_snd(name, AVSV_SA_AMF_SU, SA_AMF_ADMIN_RESTART,
					su_on_node) != NCSCC_RC_SUCCESS) {
			report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TIMEOUT, nullptr,
					"Admin op request send failed '%s'", name.c_str());
			pend_cbk.invocation = 0;
			pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(0);
		}
	} else {
		/* Atleast one non-restartable (saAmfCompDisableRestart or
		   saAmfCtDefDisableRestart is true) comp is assigned.
		   First gracefully switch-over SU's assignments to other
		   At present assignment of whole SU will be gracefully
		   reassigned.
		   Thus for PI applications modeled on NWay and Nway Active model
		   this is spec deviation.
		 */
		set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
		sg_of_su->su_fault(avd_cb, this);
	}
	TRACE_LEAVE();
}
/**
 * Handle admin operations on SaAmfSU objects.
 * 
 * @param immoi_handle
 * @param invocation
 * @param su_name
 * @param op_id
 * @param params
 */
static void su_admin_op_cb(SaImmOiHandleT immoi_handle,	SaInvocationT invocation,
	const SaNameT *su_name, SaImmAdminOperationIdT op_id,
	const SaImmAdminOperationParamsT_2 **params)
{
	AVD_CL_CB *cb = (AVD_CL_CB*) avd_cb;
	AVD_SU    *su;
	AVD_AVND  *node;

	TRACE_ENTER2("%llu, '%s', %llu", invocation, osaf_extended_name_borrow(su_name), op_id);

	if ( op_id > SA_AMF_ADMIN_RESTART && op_id != SA_AMF_ADMIN_REPAIRED) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_NOT_SUPPORTED, nullptr,
				"Unsupported admin op for SU: %llu", op_id);
		goto done;
	}

	if (nullptr == (su = su_db->find(Amf::to_string(su_name)))) {
		LOG_CR("SU '%s' not found", osaf_extended_name_borrow(su_name));
		/* internal error? osafassert instead? */
		goto done;
	}

	if ((su->sg_of_su->sg_ncs_spec == true)	&&
			(cb->node_id_avd == su->su_on_node->node_info.nodeId)) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_NOT_SUPPORTED, nullptr, 
				"Admin operation on Active middleware SU is not allowed");
		goto done;
	}

	/* Avoid multiple admin operations on other SUs belonging to the same SG. */
	for (auto const& su_ptr : su->sg_of_su->list_of_su) {
		/* su's sg_fsm_state is checked below, just check other su. */
		if ((su != su_ptr) && (su_ptr->pend_cbk.invocation != 0)) {
			report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
					"Admin operation is already going on (su'%s')", su_ptr->name.c_str());
			goto done;
		}
	}

	/* Avoid if any single Csi assignment is undergoing on SG. */
	if (csi_assignment_validate(su->sg_of_su) == true) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
				"Single Csi assignment undergoing on (sg'%s')", su->sg_of_su->name.c_str());
		goto done;
	}

	if (su->sg_of_su->sg_fsm_state != AVD_SG_FSM_STABLE) {
		if((su->sg_of_su->sg_fsm_state != AVD_SG_FSM_SU_OPER) ||
				(su->saAmfSUAdminState != SA_AMF_ADMIN_SHUTTING_DOWN) || 
				(op_id != SA_AMF_ADMIN_LOCK)) {
			report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
					"SG state is not stable"); /* whatever that means... */
			goto done;
		} else {
			/* This means that shutdown was going on and lock has
			   been issued.  In this case, response to shutdown
			   and then allow lock operation to proceed. */
			SaInvocationT invoc = su->pend_cbk.invocation;
			if (invoc == 0)
				invoc = invocation;

			report_admin_op_error(immoi_handle, invoc,
					SA_AIS_ERR_INTERRUPT, &su->pend_cbk,
					"SU lock has been issued '%s'", su->name.c_str());
		}
	}
	/* if Tolerance timer is running for any SI's withing this SG, then return SA_AIS_ERR_TRY_AGAIN */
	if (sg_is_tolerance_timer_running_for_any_si(su->sg_of_su)) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
				"Tolerance timer is running for some of the SI's in the SG '%s', " 
				"so differing admin opr",su->sg_of_su->name.c_str());
		goto done;
	}

	if ( ((su->saAmfSUAdminState == SA_AMF_ADMIN_UNLOCKED) && (op_id == SA_AMF_ADMIN_UNLOCK)) ||
	     ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED)   && (op_id == SA_AMF_ADMIN_LOCK))   ||
	     ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
	      (op_id == SA_AMF_ADMIN_LOCK_INSTANTIATION))                                     ||
	     ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED)   && (op_id == SA_AMF_ADMIN_UNLOCK_INSTANTIATION))) {

		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_NO_OP, nullptr,
				"Admin operation (%llu) has no effect on current state (%u)", op_id,
				su->saAmfSUAdminState);
		goto done;
	}

	if (((su->saAmfSUAdminState == SA_AMF_ADMIN_UNLOCKED) &&
		  (op_id != SA_AMF_ADMIN_LOCK) &&
		  (op_id != SA_AMF_ADMIN_SHUTDOWN) &&
		  (op_id != SA_AMF_ADMIN_RESTART) &&
		  (op_id != SA_AMF_ADMIN_REPAIRED)) ||
	     ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED) &&
		  (op_id != SA_AMF_ADMIN_UNLOCK) &&
		  (op_id != SA_AMF_ADMIN_REPAIRED) &&
		  (op_id != SA_AMF_ADMIN_RESTART) &&
		  (op_id != SA_AMF_ADMIN_LOCK_INSTANTIATION))  ||
	     ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
		  (op_id != SA_AMF_ADMIN_UNLOCK_INSTANTIATION) &&
		  (op_id != SA_AMF_ADMIN_REPAIRED)) ||
	     ((su->saAmfSUAdminState != SA_AMF_ADMIN_UNLOCKED) &&
		  (op_id == SA_AMF_ADMIN_SHUTDOWN))) {

		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
				"State transition invalid, state %u, op %llu", su->saAmfSUAdminState, op_id);
		goto done;
	}
	if  (op_id == SA_AMF_ADMIN_RESTART) { 
		if (su->sg_of_su->sg_ncs_spec == true) {
			report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"Not allowed on middleware SU: %s, op_id: %llu",
					su->name.c_str(), op_id);
			goto done;
		}
		if (su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED) {
			report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"Prescence state of SU is uninstantiated, it is: %u, op_id: %llu",
					su->saAmfSUPresenceState, op_id);
			goto done;
		}
		if (su->saAmfSUOperState == SA_AMF_OPERATIONAL_DISABLED) {
			report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"SU is disabled (%u), repair it or check node status, op_id: %llu",
					su->saAmfSUOperState, op_id);
			goto done;
		}
		SaAisErrorT rc = SA_AIS_OK;
		rc = su->check_su_stability();
                if (rc != SA_AIS_OK) {
                        report_admin_op_error(immoi_handle, invocation,
                                              SA_AIS_ERR_TRY_AGAIN, nullptr,
                                              "Some entity is unstable, Operation cannot "
                                              "be performed on '%s'"
                                              "Check syslog for entity details", su->name.c_str());
                        goto done;
                }
	}
	node = su->get_node_ptr();
	if (node->admin_node_pend_cbk.admin_oper != 0) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
				"Node'%s' hosting SU'%s', undergoing admin operation'%u'", node->name.c_str(),
				su->name.c_str(), node->admin_node_pend_cbk.admin_oper);
		goto done;
	}

	/* Validation has passed and admin operation should be done. Proceed with it... */
	switch (op_id) {
	case SA_AMF_ADMIN_UNLOCK:
		su->unlock(immoi_handle, invocation);
		break;
	case SA_AMF_ADMIN_SHUTDOWN:
		su->shutdown(immoi_handle, invocation);
		break;
	case SA_AMF_ADMIN_LOCK:
		su->lock(immoi_handle, invocation);
		break;
	case SA_AMF_ADMIN_LOCK_INSTANTIATION:
		su->lock_instantiation(immoi_handle, invocation);
		break;
	case SA_AMF_ADMIN_UNLOCK_INSTANTIATION:
		su->unlock_instantiation(immoi_handle, invocation);
		break;
	case SA_AMF_ADMIN_REPAIRED:
		su->repaired(immoi_handle, invocation);
		break;
	case SA_AMF_ADMIN_RESTART:
		su->restart(immoi_handle, invocation);
		break;
	default:
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_INVALID_PARAM, nullptr,
				"Unsupported admin op");
		break;
	}

done:

	TRACE_LEAVE2();
}

static SaAisErrorT su_rt_attr_cb(SaImmOiHandleT immOiHandle,
	const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	const std::string obj_name(Amf::to_string(objectName));
	AVD_SU *su = su_db->find(obj_name);
	SaImmAttrNameT attributeName;
	int i = 0;
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER2("%s", osaf_extended_name_borrow(objectName));

	while ((attributeName = attributeNames[i++]) != nullptr) {
		if (!strcmp("saAmfSUAssignedSIs", attributeName)) {
#if 0
			/*  TODO */
			SaUint32T saAmfSUAssignedSIs = su->saAmfSUNumCurrActiveSIs + su->saAmfSUNumCurrStandbySIs;
			avd_saImmOiRtObjectUpdate_sync(immOiHandle, objectName,
				attributeName, SA_IMM_ATTR_SAUINT32T, &saAmfSUAssignedSIs);
#endif
		} else if (!strcmp("saAmfSUNumCurrActiveSIs", attributeName)) {
			rc = avd_saImmOiRtObjectUpdate_sync(obj_name, attributeName,
				SA_IMM_ATTR_SAUINT32T, &su->saAmfSUNumCurrActiveSIs);
		} else if (!strcmp("saAmfSUNumCurrStandbySIs", attributeName)) {
			rc = avd_saImmOiRtObjectUpdate_sync(obj_name, attributeName,
				SA_IMM_ATTR_SAUINT32T, &su->saAmfSUNumCurrStandbySIs);
		} else if (!strcmp("saAmfSURestartCount", attributeName)) {
			rc = avd_saImmOiRtObjectUpdate_sync(obj_name, attributeName,
				SA_IMM_ATTR_SAUINT32T, &su->saAmfSURestartCount);
		} else {
			LOG_ER("Ignoring unknown attribute '%s'", attributeName);
		}
		if (rc != SA_AIS_OK) {
			/* For any failures of update, return FAILED_OP. */
			rc = SA_AIS_ERR_FAILED_OPERATION;
			break;
		}
	}
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_su_ccb_completed_modify_hdlr
 * 
 * Purpose: This routine validates modify CCB operations on SaAmfSU objects.
 * 
 *
 * Input  : Ccb Util Oper Data
 *  
 * Returns: None.
 *  
 * NOTES  : None.
 *
 *
 **************************************************************************/
static SaAisErrorT su_ccb_completed_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_OK;
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;
	bool value_is_deleted = false;

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != nullptr) {

		if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) ||
				(attr_mod->modAttr.attrValues == nullptr)) {
			/* Attribute value is deleted, revert to default value if applicable*/
			value_is_deleted = true;
		} else {
			/* Attribute value is modified */
			value_is_deleted = false;
		}
		if (!strcmp(attr_mod->modAttr.attrName, "saAmfSUFailover")) {
			if (value_is_deleted == true)
				continue;
			AVD_SU *su = su_db->find(Amf::to_string(&opdata->objectName));
			uint32_t su_failover = *((SaUint32T *)attr_mod->modAttr.attrValues[0]);

			/* If SG is not in stable state and amfnd is already busy in the handling of some fault,
			   modification of this attribute in an unstable SG can affects the recovery at amfd though 
			   amfnd part of recovery had been done without the modified value of this attribute.
			   So modification should be allowed only in the stable state of SG.
			 */
			if (su->sg_of_su->sg_fsm_state != AVD_SG_FSM_STABLE) {
				rc = SA_AIS_ERR_TRY_AGAIN;
				report_ccb_validation_error(opdata, "'%s' is not stable",su->sg_of_su->name.c_str()); 
				goto done;
			}

			if (su_failover > SA_TRUE) {
				report_ccb_validation_error(opdata, "Invalid saAmfSUFailover SU:'%s'", su->name.c_str());
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else if (!strcmp(attr_mod->modAttr.attrName, "saAmfSUMaintenanceCampaign")) {
			if (value_is_deleted == true)
				continue;
			AVD_SU *su = su_db->find(Amf::to_string(&opdata->objectName));

			if (su->saAmfSUMaintenanceCampaign.length() > 0) {
				report_ccb_validation_error(opdata, "saAmfSUMaintenanceCampaign already set for %s",
						su->name.c_str());
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else if (!strcmp(attr_mod->modAttr.attrName, "saAmfSUType")) {
			if (value_is_deleted == true)
				continue;
			AVD_SU *su;
			SaNameT sutype_name = *(SaNameT*) attr_mod->modAttr.attrValues[0];
			su = su_db->find(Amf::to_string(&opdata->objectName));
			if(SA_AMF_ADMIN_LOCKED_INSTANTIATION != su->saAmfSUAdminState) {
				report_ccb_validation_error(opdata, "SU is not in locked-inst, present state '%d'",
						su->saAmfSUAdminState);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
			if (sutype_db->find(Amf::to_string(&sutype_name)) == nullptr) {
				report_ccb_validation_error(opdata, "SU Type not found '%s'", osaf_extended_name_borrow(&sutype_name));
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}

		} else {
			report_ccb_validation_error(opdata, "Modification of attribute '%s' not supported",
					attr_mod->modAttr.attrName);
			rc = SA_AIS_ERR_NOT_SUPPORTED;
			goto done;
		}
	}

done:
	return rc;
}

/*****************************************************************************
 * Function: avd_su_ccb_completed_delete_hdlr
 * 
 * Purpose: This routine validates delete CCB operations on SaAmfSU objects.
 * 
 *
 * Input  : Ccb Util Oper Data
 *  
 * Returns: None.
 *  
 * NOTES  : None.
 *
 *
 **************************************************************************/
static SaAisErrorT su_ccb_completed_delete_hdlr(CcbUtilOperationData_t *opdata)
{
	AVD_SU *su;
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	int is_app_su = 1;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	if (strstr((char *)osaf_extended_name_borrow(&opdata->objectName), "safApp=OpenSAF") != nullptr)
		is_app_su = 0;

	su = su_db->find(Amf::to_string(&opdata->objectName));
	osafassert(su != nullptr);

	if (is_app_su) {
		if (su->su_on_node->node_state == AVD_AVND_STATE_ABSENT)
			goto done_ok;

		if (su->su_on_node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION)
			goto done_ok;

		if (su->sg_of_su->saAmfSGAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION)
			goto done_ok;

		// skip this check if this is the standby director
		if (su->saAmfSUAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION &&
			avd_cb->avail_state_avd != SA_AMF_HA_STANDBY) {
			report_ccb_validation_error(opdata,
				"Admin state is not locked instantiation required for deletion");
			goto done;
		}
	}

	if (!is_app_su && (su->su_on_node->node_state != AVD_AVND_STATE_ABSENT)) {
		report_ccb_validation_error(opdata, "MW SU can only be deleted when its hosting node is down");
		goto done;
	}

done_ok:
	rc = SA_AIS_OK;

done:
	// store su in all cases. saAmfSUAdminState may change
	// between ccb complete and ccb apply, if this is the standby
	opdata->userData = su;
	TRACE_LEAVE();
	return rc;
}

/**
 * Validates if a node's admin state is valid for creating an SU
 * Should only be called if the SU admin state is UNLOCKED, otherwise the
 * logged errors could be misleading.
 * @param su_dn
 * @param attributes
 * @param opdata
 * @return true if so
 */
static bool node_admin_state_is_valid_for_su_create(const std::string& su_dn,
        const SaImmAttrValuesT_2 **attributes,
        const CcbUtilOperationData_t *opdata)
{
	TRACE_ENTER2("%s", su_dn.c_str());
	SaNameT tmp_node_name = {0};
	(void) immutil_getAttr("saAmfSUHostNodeOrNodeGroup", attributes, 0, &tmp_node_name);
	const std::string node_name(Amf::to_string(&tmp_node_name));

	if (node_name.empty() == true) {
		// attribute empty but this is probably not an error, just trace
		TRACE("Create '%s', saAmfSUHostNodeOrNodeGroup not configured",
			su_dn.c_str());
		TRACE_LEAVE();
		return false;
	}

	if (node_name.compare(0, 11, "safAmfNode=") != 0) {
		// attribute non empty but does not contain a node DN, not OK
		amflog(SA_LOG_SEV_NOTICE,
			"Create '%s', saAmfSUHostNodeOrNodeGroup not configured with a node (%s)",
			su_dn.c_str(), node_name.c_str());
		TRACE_LEAVE();
		return false;
	}

	const AVD_AVND *node = avd_node_get(node_name);
	if (node == nullptr) {
		if (opdata == nullptr || ccbutil_getCcbOpDataByDN(opdata->ccbId, &tmp_node_name) == nullptr) {
			// node must exist in the current model, or created in the same CCB
			amflog(SA_LOG_SEV_WARNING,
				"Create '%s', configured with a non existing node (%s)",
				su_dn.c_str(), node_name.c_str());
			TRACE_LEAVE();
			return false;
		} else {
			// check admin state of the new node
			SaAmfAdminStateT admin_state;
			const CcbUtilOperationData_t *t_opdata = ccbutil_getCcbOpDataByDN(opdata->ccbId, &tmp_node_name);
			immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNodeAdminState"),
				t_opdata->param.create.attrValues, 0, &admin_state);
			if (admin_state != SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
				TRACE("Create '%s', configured node '%s' is not locked instantiation",
					su_dn.c_str(), node_name.c_str());
				TRACE_LEAVE();
				return false;
			}
		}

	} else {
		// configured with a node DN, accept only locked-in state
		if (node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			TRACE("Create '%s', configured node '%s' is not locked instantiation",
				su_dn.c_str(), node_name.c_str());
			TRACE_LEAVE();
			return false;
		}
	}

	TRACE_LEAVE();
	return true;
}

/**
 * Validates if an SG's admin state is valid for creating an SU
 * Should only be called if the SU admin state is UNLOCKED, otherwise the
 * logged errors could be misleading.
 * @param su_dn
 * @param attributes
 * @param opdata
 * @return true if so
 */
static bool sg_admin_state_is_valid_for_su_create(const std::string& su_dn,
        const SaImmAttrValuesT_2 **attributes,
        const CcbUtilOperationData_t *opdata)
{
	std::string sg_name;
	SaAmfAdminStateT admin_state;

	avsv_sanamet_init(su_dn, sg_name, "safSg");
	const AVD_SG *sg = sg_db->find(sg_name);
	if (sg != nullptr) {
		admin_state = sg->saAmfSGAdminState;
	} else {
		const SaNameTWrapper tmp_sg_name(sg_name);
		// SG does not exist in current model, check CCB
		const CcbUtilOperationData_t *tmp =
			ccbutil_getCcbOpDataByDN(opdata->ccbId, tmp_sg_name);
		osafassert(tmp != nullptr); // already validated

		(void) immutil_getAttr("saAmfSGAdminState",
			tmp->param.create.attrValues, 0, &admin_state);
	}

	if (admin_state != SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
		TRACE("'%s' created UNLOCKED but '%s' is not locked instantiation",
			su_dn.c_str(), sg_name.c_str());
		return false;
	}

	return true;
}

/**
 * Validation performed when an SU is dynamically created with a CCB.
 * @param dn
 * @param attributes
 * @param opdata
 * 
 * @return bool
 */
static bool is_ccb_create_config_valid(const std::string& dn,
                                       const SaImmAttrValuesT_2 **attributes,
                                       const CcbUtilOperationData_t *opdata)
{
	SaAmfAdminStateT admstate;
	SaAisErrorT rc;

	assert(opdata != nullptr);  // must be called in CCB context

	bool is_mw_su = false;
	if (strstr((char *)dn.c_str(), "safApp=OpenSAF") != nullptr)
		is_mw_su = true;

	rc = immutil_getAttr("saAmfSUAdminState", attributes, 0, &admstate);

	if (is_mw_su == true) {
		// Allow MW SUs to be created without an admin state
		if (rc != SA_AIS_OK)
			return true;

		// value exist, it must be unlocked
		if (admstate == SA_AMF_ADMIN_UNLOCKED)
			return true;
		else {
			report_ccb_validation_error(opdata,
				"admin state must be UNLOCKED for dynamically created MW SUs");
			return false;
		}
	}

	// A non MW SU (application SU), check admin state
	// Default value is UNLOCKED if created without a value
	if (rc != SA_AIS_OK)
		admstate = SA_AMF_ADMIN_UNLOCKED;

	// locked-in state is fine
	if (admstate == SA_AMF_ADMIN_LOCKED_INSTANTIATION)
		return true;

	if (admstate != SA_AMF_ADMIN_UNLOCKED) {
		report_ccb_validation_error(opdata,
			"'%s' created with invalid saAmfSUAdminState (%u)",
			dn.c_str(), admstate);
		return false;
	}

	if (node_admin_state_is_valid_for_su_create(dn, attributes, opdata))
		return true;

	if (sg_admin_state_is_valid_for_su_create(dn, attributes, opdata))
		return true;

	amflog(SA_LOG_SEV_NOTICE, "CCB %d creation of '%s' failed",
		opdata->ccbId, dn.c_str());
	report_ccb_validation_error(opdata,
		"SG or node not configured properly to allow creation of UNLOCKED SU");

	return false;
}

static SaAisErrorT su_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	const std::string obj_name(Amf::to_string(&opdata->objectName));
	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, obj_name.c_str());

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(obj_name, opdata->param.create.attrValues, opdata) &&
		    is_ccb_create_config_valid(obj_name, opdata->param.create.attrValues, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		rc = su_ccb_completed_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		rc = su_ccb_completed_delete_hdlr(opdata);
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_su_ccb_apply_modify_hdlr
 * 
 * Purpose: This routine handles modify operations on SaAmfSU objects.
 * 
 *
 * Input  : Ccb Util Oper Data. 
 *  
 * Returns: None.
 *  
 * NOTES  : None.
 *
 *
 **************************************************************************/
static void su_ccb_apply_modify_hdlr(struct CcbUtilOperationData *opdata)
{
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;
	AVD_SU *su;
	bool value_is_deleted;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	su = su_db->find(Amf::to_string(&opdata->objectName));
	osafassert(su != nullptr);

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != nullptr) {
		/* Attribute value removed */
		if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || (attr_mod->modAttr.attrValues == nullptr))
			value_is_deleted = true;
		else
			value_is_deleted = false;

		if (!strcmp(attr_mod->modAttr.attrName, "saAmfSUFailover")) {
			if (value_is_deleted) {
				su->set_su_failover(su->su_type->saAmfSutDefSUFailover);
				su->saAmfSUFailover_configured = false;
			}
			else {
				bool value =
					static_cast<bool>(*((SaUint32T *)attr_mod->modAttr.attrValues[0]));
				su->set_su_failover(value);
				su->saAmfSUFailover_configured = true;
			}
			if (!su->saAmfSUPreInstantiable) {
				su->set_su_failover(true);
				su->saAmfSUFailover_configured = true;
			}
		} else if (!strcmp(attr_mod->modAttr.attrName, "saAmfSUMaintenanceCampaign")) {
			if (value_is_deleted) {
				su->saAmfSUMaintenanceCampaign = "";
				TRACE("saAmfSUMaintenanceCampaign cleared for '%s'", su->name.c_str());
			} else {
				osafassert(su->saAmfSUMaintenanceCampaign.empty() == true);
				su->saAmfSUMaintenanceCampaign = Amf::to_string(
					reinterpret_cast<SaNameT*>(attr_mod->modAttr.attrValues[0]));
				TRACE("saAmfSUMaintenanceCampaign set to '%s' for '%s'",
					  su->saAmfSUMaintenanceCampaign.c_str(), su->name.c_str());
			}
		} else if (!strcmp(attr_mod->modAttr.attrName, "saAmfSUType")) {
			AVD_SUTYPE *sut;
			SaNameT sutype_name = *(SaNameT*) attr_mod->modAttr.attrValues[0];
			TRACE("Modified saAmfSUType from '%s' to '%s'", su->saAmfSUType.c_str(), osaf_extended_name_borrow(&sutype_name));
			sut = sutype_db->find(Amf::to_string(&sutype_name));
			avd_sutype_remove_su(su);
			su->saAmfSUType = Amf::to_string(&sutype_name);
			su->su_type = sut;
			avd_sutype_add_su(su);
			if (su->saAmfSUPreInstantiable) {
				su->set_su_failover(static_cast<bool>(sut->saAmfSutDefSUFailover));
				su->saAmfSUFailover_configured = false;
			} else {
				su->set_su_failover(true);
				su->saAmfSUFailover_configured = true;
			}
			su->su_is_external = sut->saAmfSutIsExternal;
		} else {
			osafassert(0);
		}
	}

	TRACE_LEAVE();
}

/**
 * Handle delete operations on SaAmfSU objects.
 * 
 * @param su
 */
void su_ccb_apply_delete_hdlr(struct CcbUtilOperationData *opdata)
{
	AVD_SU *su = static_cast<AVD_SU*>(opdata->userData);
	AVD_AVND *su_node_ptr;
	AVSV_PARAM_INFO param;
	AVD_SG *sg = su->sg_of_su;

	TRACE_ENTER2("'%s'", su->name.c_str());

	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) {
		/* Check if the comp and related objects are still left. This can
		   happen on Standby Amfd when it has just read the configuration
		   and before it becomes applier, Act Amfd deletes Comps. Those Comps
		   will be left out at Standby Amfd. Though this could be rare.*/
		std::set<std::string> comp_list;
		for (const auto& comp : su->list_of_comp)
			comp_list.insert(Amf::to_string(&comp->comp_info.name));
		for (std::set<std::string>::const_iterator iter1 = comp_list.begin();
				iter1 != comp_list.end(); ++iter1) {
			AVD_COMP *comp = comp_db->find(*iter1);

			//Create a tmp database of compcstype.
			std::set<std::string> compcstype_list;
			for (std::map<std::string, AVD_COMPCS_TYPE*>::const_iterator it =
					compcstype_db->begin();
					it != compcstype_db->end(); it++) {
				AVD_COMPCS_TYPE *compcstype = it->second;
				compcstype_list.insert(compcstype->name);
			}
			TRACE("Standby Amfd, comp '%s' not deleted", osaf_extended_name_borrow(&comp->comp_info.name));

			for (std::set<std::string>::const_iterator iter1 = compcstype_list.begin();
					iter1 != compcstype_list.end(); ++iter1) {
				AVD_COMPCS_TYPE *compcstype = compcstype_db->find(*iter1);
				if (compcstype->comp == comp) {
					TRACE("Standby Amfd, compcstype '%s' not deleted",
							compcstype->name.c_str());
					compcstype_db->erase(compcstype->name);
					delete compcstype;
				}
			}
			compcstype_list.clear();
			/* Delete the Comp. */
			struct CcbUtilOperationData opdata;
			osaf_extended_name_alloc(osaf_extended_name_borrow(&comp->comp_info.name),
									 &opdata.objectName);
			comp_ccb_apply_delete_hdlr(&opdata);
		}
		comp_list.clear();

		su->remove_from_model();
		delete su;
		goto done;
	}

	su_node_ptr = su->get_node_ptr();

	if ((su_node_ptr->node_state == AVD_AVND_STATE_PRESENT) ||
	    (su_node_ptr->node_state == AVD_AVND_STATE_NO_CONFIG) ||
	    (su_node_ptr->node_state == AVD_AVND_STATE_NCS_INIT)) {
		memset(((uint8_t *)&param), '\0', sizeof(AVSV_PARAM_INFO));
		param.class_id = AVSV_SA_AMF_SU;
		param.act = AVSV_OBJ_OPR_DEL;

		// this needs to be freed later
		SaNameT su_name;
		osaf_extended_name_lend(su->name.c_str(), &su_name);
		param.name = su_name;
		avd_snd_op_req_msg(avd_cb, su_node_ptr, &param);
	}

	su->remove_from_model();
	delete su;

	if (AVD_SG_FSM_STABLE == sg->sg_fsm_state) {
		/*if su of uneqal rank has been delete and all SUs are of same rank then do screening
		  for SI Distribution. */
		if (true == sg->equal_ranked_su) {
			switch (sg->sg_redundancy_model) {
				case SA_AMF_NPM_REDUNDANCY_MODEL:
				break;

				case SA_AMF_N_WAY_REDUNDANCY_MODEL:
					avd_sg_nway_screen_si_distr_equal(sg);
					break;

				case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
					avd_sg_nwayact_screening_for_si_distr(sg);
					break;
				default:

					break;
			} /* switch */
		} /*	if (true == sg->equal_ranked_su) */ 
	} /*if (AVD_SG_FSM_STABLE == sg->sg_fsm_state) */

done:
	TRACE_LEAVE();
}

static void su_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_SU *su;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		su = su_create(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues);
		osafassert(su);
		su_add_to_model(su);
		break;
	case CCBUTIL_MODIFY:
		su_ccb_apply_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		su_ccb_apply_delete_hdlr(opdata);
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE();
}

void AVD_SU::inc_curr_act_si() {
	saAmfSUNumCurrActiveSIs++;
	osafassert(saAmfSUNumCurrActiveSIs <= sg_of_su->saAmfSGMaxActiveSIsperSU);
	TRACE("%s saAmfSUNumCurrActiveSIs=%u", name.c_str(),
		saAmfSUNumCurrActiveSIs);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_SI_CURR_ACTIVE);
}

void AVD_SU::dec_curr_act_si() {
	osafassert(saAmfSUNumCurrActiveSIs > 0);
	saAmfSUNumCurrActiveSIs--;
	TRACE("%s saAmfSUNumCurrActiveSIs=%u", name.c_str(),
		saAmfSUNumCurrActiveSIs);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_SI_CURR_ACTIVE);
}

void AVD_SU::inc_curr_stdby_si() {
	saAmfSUNumCurrStandbySIs++;
	osafassert(saAmfSUNumCurrStandbySIs <= sg_of_su->saAmfSGMaxStandbySIsperSU);
	TRACE("%s saAmfSUNumCurrStandbySIs=%u", name.c_str(),
		saAmfSUNumCurrStandbySIs);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_SI_CURR_STBY);
}

void AVD_SU::dec_curr_stdby_si() {
	osafassert(saAmfSUNumCurrStandbySIs > 0);
	saAmfSUNumCurrStandbySIs--;
	TRACE("%s saAmfSUNumCurrStandbySIs=%u", name.c_str(),
		saAmfSUNumCurrStandbySIs);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_SI_CURR_STBY);
}

void avd_su_constructor(void)
{
	su_db = new AmfDb<std::string, AVD_SU>;
	avd_class_impl_set("SaAmfSU", su_rt_attr_cb, su_admin_op_cb,
		su_ccb_completed_cb, su_ccb_apply_cb);
}

/**
 * send update of SU attribute to node director 
 * @param su
 * @param attrib_id
 */
void AVD_SU::send_attribute_update(AVSV_AMF_SU_ATTR_ID attrib_id) {
	AVD_AVND *su_node_ptr = nullptr;
	AVSV_PARAM_INFO param;
	memset(((uint8_t *)&param), '\0', sizeof(AVSV_PARAM_INFO));

	TRACE_ENTER();

	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) {
		TRACE_LEAVE2("avd is not in active state");
		return;
	}

	/*Update this value on the node hosting this SU*/
	su_node_ptr = get_node_ptr();
	if ((su_node_ptr) && ((su_node_ptr->node_state == AVD_AVND_STATE_PRESENT) ||
			(su_node_ptr->node_state == AVD_AVND_STATE_NO_CONFIG) ||
			(su_node_ptr->node_state == AVD_AVND_STATE_NCS_INIT))) {
		param.class_id = AVSV_SA_AMF_SU;
		param.act = AVSV_OBJ_OPR_MOD;

		SaNameT su_name;
		osaf_extended_name_lend(name.c_str(), &su_name);
		param.name = su_name;

		switch (attrib_id) {
		case saAmfSUFailOver_ID:
		{
			uint32_t sufailover; 
			param.attr_id = saAmfSUFailOver_ID;
			param.value_len = sizeof(uint32_t);
			sufailover = htonl(saAmfSUFailover);
			memcpy(&param.value[0], &sufailover, param.value_len);
			break;
		}
		default:
			osafassert(0);
		}

		if (avd_snd_op_req_msg(avd_cb, su_node_ptr, &param) != NCSCC_RC_SUCCESS) {
			LOG_ER("%s:failed for %s",__FUNCTION__, su_node_ptr->name.c_str());
		}
	}

	TRACE_LEAVE();
}

void AVD_SU::set_su_failover(bool value) {
	saAmfSUFailover = value;
	TRACE("Modified saAmfSUFailover to '%u' for '%s'",
		saAmfSUFailover, name.c_str());
	send_attribute_update(saAmfSUFailOver_ID);
}

/**
 * Delete all SUSIs assigned to the SU.
 *
 */
void AVD_SU::delete_all_susis(void) {
	TRACE_ENTER2("'%s'", name.c_str());

	while (list_of_susi != nullptr) {
		avd_compcsi_delete(avd_cb, list_of_susi, false);
		m_AVD_SU_SI_TRG_DEL(avd_cb, list_of_susi);
	}

	saAmfSUNumCurrStandbySIs = 0;
	saAmfSUNumCurrActiveSIs = 0;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_AVD_SU_CONFIG);

	TRACE_LEAVE();
}

void AVD_SU::set_all_susis_assigned_quiesced(void) {
	AVD_SU_SI_REL *susi = list_of_susi;

	TRACE_ENTER2("'%s'", name.c_str());

	for (; susi != nullptr; susi = susi->su_next) {
		if (susi->fsm != AVD_SU_SI_STATE_UNASGN) {
			susi->state = SA_AMF_HA_QUIESCED;
			avd_susi_update_fsm(susi, AVD_SU_SI_STATE_ASGND);
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, susi, AVSV_CKPT_AVD_SI_ASS);
			avd_gen_su_ha_state_changed_ntf(avd_cb, susi);
			avd_susi_update_assignment_counters(susi, AVSV_SUSI_ACT_MOD,
					SA_AMF_HA_QUIESCING, SA_AMF_HA_QUIESCED);
			avd_pg_susi_chg_prc(avd_cb, susi);
		}
	}

	TRACE_LEAVE();
}

void AVD_SU::set_all_susis_assigned(void) {
	AVD_SU_SI_REL *susi = list_of_susi;

	TRACE_ENTER2("'%s'", name.c_str());

	for (; susi != nullptr; susi = susi->su_next) {
		if (susi->fsm != AVD_SU_SI_STATE_UNASGN) {
			avd_susi_update_fsm(susi, AVD_SU_SI_STATE_ASGND);
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, susi, AVSV_CKPT_AVD_SI_ASS);
			avd_pg_susi_chg_prc(avd_cb, susi);
		}
	}

	TRACE_LEAVE();
}

void AVD_SU::set_term_state(bool state) {
	term_state = state;
	TRACE("%s term_state %u", name.c_str(), term_state);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_TERM_STATE);
}

void AVD_SU::set_su_switch(SaToggleState state, bool wrt_to_imm) {
	su_switch = state;
	TRACE("%s su_switch %u", name.c_str(), su_switch);
	if (avd_cb->scs_absence_max_duration > 0 && wrt_to_imm) {
		avd_saImmOiRtObjectUpdate_sync(name,
				const_cast<SaImmAttrNameT>("osafAmfSUSwitch"),
				SA_IMM_ATTR_SAUINT32T, &su_switch);
	}
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_SWITCH);
}

AVD_AVND *AVD_SU::get_node_ptr(void) {
	 if (su_is_external == true)
		 return avd_cb->ext_comp_info.local_avnd_node;
	 else
		 return su_on_node;
}

/**
 * Checks if the SU can be made in-service
 * For reference see 3.2.1.4 and for pre-instantiable SUs Table 4
 *
 * @param su
 * @return true if SU can be made in-service
 */
bool AVD_SU::is_in_service(void) {
	AVD_AVND *node = get_node_ptr();
	const AVD_SG *sg = sg_of_su;
	const AVD_APP *app = sg->app;

    if (saAmfSUPreInstantiable == true) {
    	return (avd_cluster->saAmfClusterAdminState == SA_AMF_ADMIN_UNLOCKED) &&
    			(app->saAmfApplicationAdminState == SA_AMF_ADMIN_UNLOCKED) &&
    			(saAmfSUAdminState == SA_AMF_ADMIN_UNLOCKED) &&
    			(sg->saAmfSGAdminState == SA_AMF_ADMIN_UNLOCKED) &&
    			(node->saAmfNodeAdminState == SA_AMF_ADMIN_UNLOCKED) &&
    			(node->saAmfNodeOperState == SA_AMF_OPERATIONAL_ENABLED) &&
    			(saAmfSUOperState == SA_AMF_OPERATIONAL_ENABLED) &&
    			((saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATED ||
    					saAmfSUPresenceState == SA_AMF_PRESENCE_RESTARTING)) &&
			(are_all_ngs_in_unlocked_state(node));
    } else {
            return (avd_cluster->saAmfClusterAdminState == SA_AMF_ADMIN_UNLOCKED) &&
            		(app->saAmfApplicationAdminState == SA_AMF_ADMIN_UNLOCKED) &&
            		(saAmfSUAdminState == SA_AMF_ADMIN_UNLOCKED) &&
            		(sg->saAmfSGAdminState == SA_AMF_ADMIN_UNLOCKED) &&
            		(node->saAmfNodeAdminState == SA_AMF_ADMIN_UNLOCKED) &&
            		(node->saAmfNodeOperState == SA_AMF_OPERATIONAL_ENABLED) &&
            		(saAmfSUOperState == SA_AMF_OPERATIONAL_ENABLED) &&
			(are_all_ngs_in_unlocked_state(node));
    }
}

void avd_su_read_headless_cached_rta(AVD_CL_CB *cb)
{
	SaAisErrorT rc;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;

	SaNameT su_dn;
	AVD_SU *su;
	const SaImmAttrValuesT_2 **attributes;
	SaToggleState su_toggle;
	const char *className = "SaAmfSU";
	const SaImmAttrNameT searchAttributes[] = {
		const_cast<SaImmAttrNameT>("osafAmfSUSwitch"),
		NULL
	};

	TRACE_ENTER();

	osafassert(cb->scs_absence_max_duration > 0);

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	if ((rc = immutil_saImmOmSearchInitialize_2(cb->immOmHandle, NULL, SA_IMM_SUBTREE,
	      SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR, &searchParam,
		  searchAttributes, &searchHandle)) != SA_AIS_OK) {

		LOG_ER("%s: saImmOmSearchInitialize_2 failed: %u", __FUNCTION__, rc);
		goto done;
	}

	while ((rc = immutil_saImmOmSearchNext_2(searchHandle, &su_dn,
					(SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK) {
		su = su_db->find(Amf::to_string(&su_dn));
		if (su && su->sg_of_su->sg_ncs_spec == false) {
			// Read sg fsm state
			rc = immutil_getAttr(const_cast<SaImmAttrNameT>("osafAmfSUSwitch"),
					attributes, 0, &su_toggle);
			osafassert(rc == SA_AIS_OK);
			su->set_su_switch(su_toggle, false);
		}
	}

	(void)immutil_saImmOmSearchFinalize(searchHandle);

done:
    TRACE_LEAVE();
}


/**
 * Checks if the SU can be made instantiated. 
 * @param su
 * @return true if SU can be made in-service
 */
bool AVD_SU::is_instantiable(void) {
        AVD_AVND *node = get_node_ptr();
        const AVD_SG *sg = sg_of_su;
        const AVD_APP *app = sg->app;

        return (avd_cluster->saAmfClusterAdminState == SA_AMF_ADMIN_UNLOCKED) &&
                        (app->saAmfApplicationAdminState == SA_AMF_ADMIN_UNLOCKED) &&
                        (saAmfSUAdminState == SA_AMF_ADMIN_UNLOCKED) &&
                        (sg->saAmfSGAdminState == SA_AMF_ADMIN_UNLOCKED) &&
                        (node->saAmfNodeAdminState == SA_AMF_ADMIN_UNLOCKED) &&
                        (node->saAmfNodeOperState == SA_AMF_OPERATIONAL_ENABLED) &&
                        (saAmfSUOperState == SA_AMF_OPERATIONAL_ENABLED) &&
			(saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED) &&
			(are_all_ngs_in_unlocked_state(node));
}

void AVD_SU::set_saAmfSUPreInstantiable(bool value) {
	saAmfSUPreInstantiable = static_cast<SaBoolT>(value);
	avd_saImmOiRtObjectUpdate(name, "saAmfSUPreInstantiable",
			SA_IMM_ATTR_SAUINT32T, &saAmfSUPreInstantiable);
	TRACE("%s saAmfSUPreInstantiable %u", name.c_str(), value);
}

/**
 * resets the assign flag for all contained components
 */
void AVD_SU::reset_all_comps_assign_flag() {
	std::for_each(list_of_comp.begin(), list_of_comp.end(), [](AVD_COMP *comp ) {comp->set_assigned(false);});
}

/**
 * Finds an unassigned component that provides the specified CSType
 * @param cstype
 * @return
 */
AVD_COMP *AVD_SU::find_unassigned_comp_that_provides_cstype(const std::string& cstype) {
	AVD_COMP *l_comp = nullptr;
	auto iter = list_of_comp.begin();
	for (; iter != list_of_comp.end(); ++iter) {
		l_comp = *iter;
		bool npi_is_assigned = false;
		AVD_COMP_TYPE *comptype = comptype_db->find(l_comp->saAmfCompType);
		osafassert(comptype);
		if ((comptype->saAmfCtCompCategory == SA_AMF_COMP_LOCAL) && l_comp->is_comp_assigned_any_csi())
			npi_is_assigned = true;
		
		if ((l_comp->assigned() == false) && (npi_is_assigned == false)) {
			AVD_COMPCS_TYPE *cst = avd_compcstype_find_match(cstype, l_comp);
			if (cst != nullptr)
				break;
		}
	}

	if (iter == list_of_comp.end()) {
		return nullptr;
	} else {
		return l_comp;
	}
}

/**
 * Disables all components since SU is disabled and out of service.
 * Takes care of response to IMM for any admin operation pending on components.
 * @param result
 */
void AVD_SU::disable_comps(SaAisErrorT result)
{
	for (const auto& comp : list_of_comp) {
		comp->curr_num_csi_actv = 0;
		comp->curr_num_csi_stdby = 0;
		comp->avd_comp_oper_state_set(SA_AMF_OPERATIONAL_DISABLED);
		if (comp->saAmfCompPresenceState != SA_AMF_PRESENCE_TERMINATION_FAILED)
			comp->avd_comp_pres_state_set(SA_AMF_PRESENCE_UNINSTANTIATED);

		/*
		   Mark a term_failed component uninstantiated when node is rebooted.
		   When node goes for reboot then AMFD marks node absent. If node does
		   not go for reboot then term_fail state of comp will be cleared 
		   as part of admin repair operation.
		 */
		if ((comp->saAmfCompPresenceState == SA_AMF_PRESENCE_TERMINATION_FAILED) &&
				(su_on_node->node_state == AVD_AVND_STATE_ABSENT)) {
				comp->avd_comp_pres_state_set(SA_AMF_PRESENCE_UNINSTANTIATED);
		}
		comp->saAmfCompRestartCount = 0;
		comp_complete_admin_op(comp, result);
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, comp, AVSV_CKPT_AVD_COMP_CONFIG);
	}
}
/**
 * @brief    Calculates number of standby assignments in a SU even
 *         when SG is unstable.
 * @return   count
 */
uint32_t AVD_SU::curr_num_standby_sis()
{
	uint32_t count = 0;
	for (AVD_SU_SI_REL *susi = list_of_susi; susi != nullptr; susi = susi->su_next)
		if ((susi->state == SA_AMF_HA_STANDBY) &&
				((susi->fsm == AVD_SU_SI_STATE_ASGN) ||
				 (susi->fsm == AVD_SU_SI_STATE_ASGND) ||
				 (susi->fsm == AVD_SU_SI_STATE_MODIFY)))
			count++;
	return count;
}
/**
 * @brief    Calculates number of standby assignments in a SU even
 *           when SG is unstable.
 * @return   count
 */
uint32_t AVD_SU::curr_num_active_sis()
{
	uint32_t count = 0;
	for (AVD_SU_SI_REL *susi = list_of_susi; susi != nullptr; susi = susi->su_next)
		if ((susi->state == SA_AMF_HA_ACTIVE) &&
				((susi->fsm == AVD_SU_SI_STATE_ASGN) ||
				 (susi->fsm == AVD_SU_SI_STATE_ASGND) ||
				 (susi->fsm == AVD_SU_SI_STATE_MODIFY)))
			count++;
	return count;
}

/**
 * @brief	This function completes admin operation on SU.
 *              It responds IMM with the result of admin operation on SU.
 * @param 	ptr to su
 * @param 	result
 *
 */
void AVD_SU::complete_admin_op(SaAisErrorT result)
{
	if (pend_cbk.invocation != 0) {
		avd_saImmOiAdminOperationResult(avd_cb->immOiHandle,
			pend_cbk.invocation, result);
		pend_cbk.invocation = 0;
		pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(0);
	}
}
/**
 * @brief    Checks if assignment is under specific fsm state
 *           . If any assignment's fsm is AVD_SU_SI_STATE_UNASGN,
 *           deletion of assignment has sent to amfnd.
 *           . If any assignment's fsm is AVD_SU_SI_STATE_MODIFY,
 *           modification of assignment has sent to amfnd.
 *           . If any assignment's fsm is AVD_SU_SI_STATE_ASGN,
 *           creation of assignment has sent to amfnd.
 * @param    @check_fsm: to-be-check fsm state
 * @result   true/false
 */
bool AVD_SU::any_susi_fsm_in(uint32_t check_fsm)
{
	TRACE_ENTER2("SU:'%s'", name.c_str());
	bool rc = false;
	for (AVD_SU_SI_REL *susi = list_of_susi; susi && rc == false;
			susi = susi->su_next) {
		if (susi->fsm == check_fsm) {
			rc = true;
			TRACE("SUSI:'%s,%s', fsm:'%d'", susi->su->name.c_str(),
					susi->si->name.c_str(), susi->fsm);
		}
	}
	TRACE_LEAVE();
	return rc;
}
/**
 * @brief  Verify if SU is stable for admin operation on any higher 
           level enity like SG, Node and Nodegroup etc.  
 * @param  ptr to su(AVD_SU).
 * @Return SA_AIS_OK/SA_AIS_ERR_TRY_AGAIN/SA_AIS_ERR_BAD_OPERATION.
 *
 * TODO: Such checks are present in node.cc and sg.cc also.
   Replace such checks by this function call as a part of refactoring.
*/
SaAisErrorT AVD_SU::check_su_stability()
{
	SaAisErrorT rc = SA_AIS_OK;

	if (pend_cbk.admin_oper != 0) {
		LOG_NO("'%s' undergoing admin operation", name.c_str());
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
	if ((saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATING) ||
			(saAmfSUPresenceState == SA_AMF_PRESENCE_TERMINATING) ||
			(saAmfSUPresenceState == SA_AMF_PRESENCE_RESTARTING)) {
		LOG_NO("'%s' has presence state '%u'", name.c_str(),saAmfSUPresenceState);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
	if ((saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATION_FAILED) ||
			(saAmfSUPresenceState == SA_AMF_PRESENCE_TERMINATION_FAILED)) {
		LOG_NO("'%s' is in repair pending state", name.c_str());
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}
	for (const auto& comp : list_of_comp) {
		rc = comp->check_comp_stability();
		if (rc != SA_AIS_OK)
			goto done;
        }
done:
	return rc;
}

/**
 * @brief Checks any component is undergoing RESTART admin operation.
 * @return true/false.
 */
bool AVD_SU::su_any_comp_undergoing_restart_admin_op()
{
	for (const auto& comp : list_of_comp) {
		if (comp->admin_pend_cbk.admin_oper == SA_AMF_ADMIN_RESTART)
			return true;
        }
	return false;
}

/**
 * @brief Returns the comp which is undergoing RESTART admin operation. 
 * @return AVD_COMP/nullptr.
 */
AVD_COMP *AVD_SU::su_get_comp_undergoing_restart_admin_op()
{ 
	for (const auto& comp : list_of_comp) {
		if (comp->admin_pend_cbk.admin_oper == SA_AMF_ADMIN_RESTART)
			return comp;
	}
	return nullptr;
}

/**
 * @brief Checks if SU consists of only restartable components.
 *        For a restartable component saAmfCompDisableRestart=0.
 * @return true/false.
 */
bool AVD_SU::su_all_comps_restartable()
{
	for (const auto& comp : list_of_comp) {
		if (comp->comp_info.comp_restart == true)
			return false;
	}
	return true;
}
/**
 * @brief Checks if any non-restartable comp in SU is assigned..
 *        For a non restartable comp saAmfCompDisableRestart=1.
 * @return true/false.
 */
bool AVD_SU::is_any_non_restartable_comp_assigned()
{
	for (const auto& comp : list_of_comp) {
		if ((comp->comp_info.comp_restart == true) && 
				(comp->is_comp_assigned_any_csi() == true))
			return true;
	}
	return false;
}

/**
 * @brief Checks if all PI comps of the SU are restartable.
 *        For a restartable component saAmfCompDisableRestart=0.
 * @return true/false.
 */
bool AVD_SU::all_pi_comps_restartable()
{
	for (const auto& comp : list_of_comp) {
		AVD_COMP_TYPE *comptype = comptype_db->find(comp->saAmfCompType);
		if ((comp->comp_info.comp_restart == true) &&
				((comptype->saAmfCtCompCategory == SA_AMF_COMP_SA_AWARE) ||
				 (IS_COMP_PROXIED_PI(comptype->saAmfCtCompCategory))))
			return false;
	}
	return true;
}

/**
 * @brief Checks if all PI comps of the SU are non-restartable.
 *        For a restartable component saAmfCompDisableRestart=1.
 * @return true/false.
 */
bool AVD_SU::all_pi_comps_nonrestartable()
{
	for (const auto& comp : list_of_comp) {
		AVD_COMP_TYPE *comptype = comptype_db->find(comp->saAmfCompType);
		if ((comp->comp_info.comp_restart == false) &&
				((comptype->saAmfCtCompCategory == SA_AMF_COMP_SA_AWARE) ||
				 (IS_COMP_PROXIED_PI(comptype->saAmfCtCompCategory))))
			return false;
	}
	return true;
}
/**
 * @brief Returns Operation Id of the admin operation on SU.
 */
SaAmfAdminOperationIdT AVD_SU::get_admin_op_id() const
{ return pend_cbk.admin_oper; }

/**
 * @brief  Checks if all comps of SU are in a given presnece state. 
 * @return true/false
 */
bool AVD_SU::all_comps_in_presence_state(SaAmfPresenceStateT pres) const
{
	if (std::all_of(list_of_comp.begin(), list_of_comp.end(), 
		[&](AVD_COMP *comp) -> bool {return comp->saAmfCompPresenceState == pres;})) {
		return true;
	} else {
		return false;
	}
}

void AVD_SU::set_surestart(bool value)
{
	surestart = value;
	TRACE("surestart flag set to '%u' for '%s'",surestart, name.c_str());
}

bool AVD_SU::get_surestart() const
{ return surestart; }

