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

AmfDb<std::string, AVD_SU> *su_db = NULL;

void AVD_SU::initialize() {
	
	saAmfSURank = 0;
	saAmfSUHostNodeOrNodeGroup.length = 0;
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
	sg_of_su = NULL;
	su_on_node = NULL;
	list_of_susi = NULL;
	list_of_comp = NULL;
	sg_list_su_next = NULL;
	avnd_list_su_next = NULL;
	su_type = NULL;
	su_list_su_type_next = NULL; 
	name.length = 0;
	saAmfSUType.length = 0;
	saAmfSUMaintenanceCampaign.length = 0;
	saAmfSUHostedByNode.length = 0;
	pend_cbk.invocation = 0;
	pend_cbk.admin_oper = (SaAmfAdminOperationIdT)0;
	surestart = false;
}

AVD_SU::AVD_SU() {
	initialize();
}

AVD_SU::AVD_SU(const SaNameT *dn) {
	initialize();
	memcpy(name.value, dn->value, sizeof(name.value));
	name.length = dn->length;
}

/**
 * Delete the SU from the model. Check point with peer. Send delete order
 * to node director.
 */
void AVD_SU::remove_from_model() {
	TRACE_ENTER2("'%s'", name.value);

	/* All the components under this SU should have been deleted
	 * by now, just do the sanity check to confirm it is done 
	 */
	osafassert(list_of_comp == NULL);
	osafassert(list_of_susi == NULL);

	m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, this, AVSV_CKPT_AVD_SU_CONFIG);
	avd_node_remove_su(this);
	avd_sutype_remove_su(this);
	su_db->erase(Amf::to_string(&name));
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

	for (susi = list_of_susi; susi != NULL; susi = susi->su_next) {
		if (susi->state == ha_state)
			curr_assignment_cnt++;
	}

	return curr_assignment_cnt;
}

void AVD_SU::remove_comp(AVD_COMP *comp) {
	AVD_COMP *i_comp = NULL;
	AVD_COMP *prev_comp = NULL;
	AVD_SU *su_ref = comp->su;

	osafassert(su_ref != NULL);

	if (comp->su != NULL) {
		/* remove COMP from SU */
		i_comp = comp->su->list_of_comp;

		while ((i_comp != NULL) && (i_comp != comp)) {
			prev_comp = i_comp;
			i_comp = i_comp->su_comp_next;
		}

		if (i_comp == comp) {
			if (prev_comp == NULL) {
				comp->su->list_of_comp = comp->su_comp_next;
			} else {
				prev_comp->su_comp_next = comp->su_comp_next;
			}

			comp->su_comp_next = NULL;
			/* Marking SU referance pointer to NULL, please dont use further in the routine */
			comp->su = NULL;
		}
	}

	bool old_preinst_value = saAmfSUPreInstantiable;
	bool curr_preinst_value = saAmfSUPreInstantiable;

	// check if preinst possibly is still true
	if (comp_is_preinstantiable(comp) == true) {
		curr_preinst_value = false;
		i_comp = list_of_comp;
		while (i_comp) {
			if ((comp_is_preinstantiable(i_comp) == true) && (i_comp != comp)) {
				curr_preinst_value = true;
				break;
			}
			i_comp = i_comp->su_comp_next;
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
	AVD_COMP *i_comp = comp->su->list_of_comp;
	AVD_COMP *prev_comp = NULL; 
	bool found_pos= false;

	while ((i_comp == NULL) || (comp->comp_info.inst_level >= i_comp->comp_info.inst_level)) {
		while ((i_comp != NULL) && (comp->comp_info.inst_level == i_comp->comp_info.inst_level)) {

			if (m_CMP_HORDER_SANAMET(comp->comp_info.name, i_comp->comp_info.name) < 0){
				found_pos = true;
				break;
			}
			prev_comp = i_comp;
			i_comp = i_comp->su_comp_next;
			if ((i_comp != NULL) && (i_comp->comp_info.inst_level > comp->comp_info.inst_level)) {
				found_pos = true;
				break;
			}

		}
		if (found_pos || (i_comp == NULL))
			break;
		prev_comp = i_comp;
		i_comp = i_comp->su_comp_next;
	}
	if (prev_comp == NULL) {
		comp->su_comp_next = comp->su->list_of_comp;
		comp->su->list_of_comp = comp;
	} else {
		prev_comp->su_comp_next = comp;
		comp->su_comp_next = i_comp;
	}

	/* Verify if the SUs preinstan value need to be changed */
	if (comp_is_preinstantiable(comp) == true) {
		set_saAmfSUPreInstantiable(true);
	}
}

/**
 * Validate configuration attributes for an AMF SU object
 * @param su
 * 
 * @return int
 */
static int is_config_valid(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes,
	const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc;
	SaNameT saAmfSUType, sg_name;
	SaNameT	saAmfSUHostNodeOrNodeGroup = {0}, saAmfSGSuHostNodeGroup = {0};
	SaBoolT abool;
	SaAmfAdminStateT admstate;
	char *parent;
	SaUint32T saAmfSutIsExternal;
	struct avd_sutype *sut = NULL;
	CcbUtilOperationData_t *tmp;
	AVD_SG *sg;

	if ((parent = strchr((char*)dn->value, ',')) == NULL) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn->value);
		return 0;
	}

	if (strncmp(++parent, "safSg=", 6) != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", parent, dn->value);
		return 0;
	}

	rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUType"), attributes, 0, &saAmfSUType);
	osafassert(rc == SA_AIS_OK);

	if ((sut = sutype_db->find(Amf::to_string(&saAmfSUType))) != NULL) {
		saAmfSutIsExternal = sut->saAmfSutIsExternal;
	} else {
		/* SU type does not exist in current model, check CCB if passed as param */
		if (opdata == NULL) {
			report_ccb_validation_error(opdata, "'%s' does not exist in model", saAmfSUType.value);
			return 0;
		}

		if ((tmp = ccbutil_getCcbOpDataByDN(opdata->ccbId, &saAmfSUType)) == NULL) {
			report_ccb_validation_error(opdata, "'%s' does not exist in existing model or in CCB",
				saAmfSUType.value);
			return 0;
		}

		rc = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSutIsExternal"), tmp->param.create.attrValues, 0, &saAmfSutIsExternal);
		osafassert(rc == SA_AIS_OK);
	}

	/* Validate that a configured node or node group exist */
	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUHostNodeOrNodeGroup"), attributes, 0, 
				&saAmfSUHostNodeOrNodeGroup) == SA_AIS_OK) {
		if (strncmp((char*)saAmfSUHostNodeOrNodeGroup.value, "safAmfNode=", 11) == 0) {
			if (avd_node_get(&saAmfSUHostNodeOrNodeGroup) == NULL) {
				if (opdata == NULL) {
					report_ccb_validation_error(opdata, "'%s' does not exist in model", 
							saAmfSUHostNodeOrNodeGroup.value);
					return 0;
				}

				if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &saAmfSUHostNodeOrNodeGroup) == NULL) {
					report_ccb_validation_error(opdata, 
							"'%s' does not exist in existing model or in CCB",
							saAmfSUHostNodeOrNodeGroup.value);
					return 0;
				}
			}
		}
		else {
			if (avd_ng_get(&saAmfSUHostNodeOrNodeGroup) == NULL) {
				if (opdata == NULL) {
					report_ccb_validation_error(opdata, "'%s' does not exist in model",
							saAmfSUHostNodeOrNodeGroup.value);
					return 0;
				}

				if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &saAmfSUHostNodeOrNodeGroup) == NULL) {
					report_ccb_validation_error(opdata,
							"'%s' does not exist in existing model or in CCB",
							saAmfSUHostNodeOrNodeGroup.value);
					return 0;
				}
			}
		}
	}

	/* Get value of saAmfSGSuHostNodeGroup */
	avsv_sanamet_init(dn, &sg_name, "safSg");
	sg = sg_db->find(Amf::to_string(&sg_name));
	if (sg) {
		saAmfSGSuHostNodeGroup = sg->saAmfSGSuHostNodeGroup;
	} else {
		if (opdata == NULL) {
			report_ccb_validation_error(opdata, "SG '%s' does not exist in model", sg_name.value);
			return 0;
		}

		if ((tmp = ccbutil_getCcbOpDataByDN(opdata->ccbId, &sg_name)) == NULL) {
			report_ccb_validation_error(opdata, "SG '%s' does not exist in existing model or in CCB",
					sg_name.value);
			return 0;
		}

		(void) immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSGSuHostNodeGroup"),
			tmp->param.create.attrValues, 0, &saAmfSGSuHostNodeGroup);
	}

	/* If its a local SU, node or nodegroup must be configured */
	if (!saAmfSutIsExternal && 
	    (strstr((char *)saAmfSUHostNodeOrNodeGroup.value, "safAmfNode=") == NULL) &&
	    (strstr((char *)saAmfSUHostNodeOrNodeGroup.value, "safAmfNodeGroup=") == NULL) &&
	    (strstr((char *)saAmfSGSuHostNodeGroup.value, "safAmfNodeGroup=") == NULL)) {
		report_ccb_validation_error(opdata, "node or node group configuration is missing for '%s'", dn->value);
		return 0;
	}

	/*
	* "It is an error to define the saAmfSUHostNodeOrNodeGroup attribute for an external
	* service unit".
	*/
	if (saAmfSutIsExternal &&
	    ((strstr((char *)saAmfSUHostNodeOrNodeGroup.value, "safAmfNode=") != NULL) ||
	     (strstr((char *)saAmfSUHostNodeOrNodeGroup.value, "safAmfNodeGroup=") != NULL) ||
	     (strstr((char *)saAmfSGSuHostNodeGroup.value, "safAmfNodeGroup=") != NULL))) {
		report_ccb_validation_error(opdata, "node or node group configured for external SU '%s'", dn->value);
		return 0;
	}

	/*
	* "If node groups are configured for both the service units of a service group and the
	* service group, the nodes contained in the node group for the service unit can only be
	* a subset of the nodes contained in the node group for the service group. If a node is
	* configured for a service unit, it must be a member of the node group for the service
	* group, if configured."
        */
	if ((strstr((char *)saAmfSUHostNodeOrNodeGroup.value, "safAmfNodeGroup=") != NULL) &&
	    (strstr((char *)saAmfSGSuHostNodeGroup.value, "safAmfNodeGroup=") != NULL)) {
		AVD_AMF_NG *ng_of_su, *ng_of_sg;

		ng_of_su = avd_ng_get(&saAmfSUHostNodeOrNodeGroup);
		if (ng_of_su == NULL) {
			report_ccb_validation_error(opdata, "Invalid saAmfSUHostNodeOrNodeGroup '%s' for '%s'",
				saAmfSUHostNodeOrNodeGroup.value, dn->value);
			return 0;
		}

		ng_of_sg = avd_ng_get(&saAmfSGSuHostNodeGroup);
		if (ng_of_su == NULL) {
			report_ccb_validation_error(opdata, "Invalid saAmfSGSuHostNodeGroup '%s' for '%s'",
				saAmfSGSuHostNodeGroup.value, dn->value);
			return 0;
		}

		if (ng_of_su->number_nodes() > ng_of_sg->number_nodes()) {
			report_ccb_validation_error(opdata, 
					"SU node group '%s' contains more nodes than the SG node group '%s'",
					saAmfSUHostNodeOrNodeGroup.value, saAmfSGSuHostNodeGroup.value);
			return 0;
		}

		if (std::includes(ng_of_sg->saAmfNGNodeList.begin(), ng_of_sg->saAmfNGNodeList.end(),
			ng_of_su->saAmfNGNodeList.begin(), ng_of_su->saAmfNGNodeList.end()) == false) {
			report_ccb_validation_error(opdata, 
						"SU node group '%s' is not a subset of the SG node group '%s'",
						saAmfSUHostNodeOrNodeGroup.value, saAmfSGSuHostNodeGroup.value);

			return 0;
		}		
	}

	// TODO maintenance campaign

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUFailover"), attributes, 0, &abool) == SA_AIS_OK) &&
	    (abool > SA_TRUE)) {
		report_ccb_validation_error(opdata, "Invalid saAmfSUFailover %u for '%s'", abool, dn->value);
		return 0;
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUAdminState"), attributes, 0, &admstate) == SA_AIS_OK) &&
	    !avd_admin_state_is_valid(admstate, opdata)) {
		report_ccb_validation_error(opdata, "Invalid saAmfSUAdminState %u for '%s'", admstate, dn->value);
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
static AVD_SU *su_create(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes)
{
	int rc = -1;
	AVD_SU *su;
	struct avd_sutype *sut;
	SaAisErrorT error;

	TRACE_ENTER2("'%s'", dn->value);

	/*
	** If called at new active at failover, the object is found in the DB
	** but needs to get configuration attributes initialized.
	*/
	if ((su = su_db->find(Amf::to_string(dn))) == NULL) {
		su = new AVD_SU(dn);
	} else
		TRACE("already created, refreshing config...");

	error = immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUType"), attributes, 0, &su->saAmfSUType);
	osafassert(error == SA_AIS_OK);

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSURank"), attributes, 0, &su->saAmfSURank) != SA_AIS_OK) {
		/* Empty, assign default value (highest number => lowest rank) */
		su->saAmfSURank = ~0U;
	}

	/* If 0 (zero), treat as lowest possible rank. Should be a positive integer */
	if (su->saAmfSURank == 0) {
		su->saAmfSURank = ~0U;
		TRACE("'%s' saAmfSURank auto-changed to lowest", su->name.value);
	}

	(void) immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUHostedByNode"), attributes, 0, &su->saAmfSUHostedByNode);

	(void) immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUHostNodeOrNodeGroup"), attributes, 0, &su->saAmfSUHostNodeOrNodeGroup);

	if ((sut = sutype_db->find(Amf::to_string(&su->saAmfSUType))) == NULL) {
		LOG_ER("saAmfSUType '%s' does not exist", su->saAmfSUType.value);
		goto done;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUFailover"), attributes, 0, &su->saAmfSUFailover) != SA_AIS_OK) {
		su->saAmfSUFailover = static_cast<bool>(sut->saAmfSutDefSUFailover);
		su->saAmfSUFailover_configured = false;
	}
	else 
		su->saAmfSUFailover_configured = true;

	(void)immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUMaintenanceCampaign"), attributes, 0, &su->saAmfSUMaintenanceCampaign);

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfSUAdminState"), attributes, 0, &su->saAmfSUAdminState) != SA_AIS_OK)
		su->saAmfSUAdminState = SA_AMF_ADMIN_UNLOCKED;

	rc = 0;

done:
	if (rc != 0) {
		delete su;
		su = NULL;
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
	AVD_AMF_NG *ng = NULL;
	AVD_SU *su_temp = NULL;
	AVD_AVND *node = NULL;
	std::set<std::string>::const_iterator node_iter;

	TRACE_ENTER2("'%s'", su->name.value);

	/* If node is configured in SU we are done */
	if (strstr((char *)su->saAmfSUHostNodeOrNodeGroup.value, "safAmfNode=") != NULL) {
		node = avd_node_get(&su->saAmfSUHostNodeOrNodeGroup);
		goto done;
	}

	/* A node group configured in the SU is prioritized before the same in SG */
	if (strstr((char *)su->saAmfSUHostNodeOrNodeGroup.value, "safAmfNodeGroup=") != NULL)
		ng = avd_ng_get(&su->saAmfSUHostNodeOrNodeGroup);
	else {
		if (strstr((char *)su->sg_of_su->saAmfSGSuHostNodeGroup.value, "safAmfNodeGroup=") != NULL)
			ng = avd_ng_get(&su->sg_of_su->saAmfSGSuHostNodeGroup);
	}

	osafassert(ng);

	/* Find a node in the group that doesn't have a SU in same SG mapped to it already */
	for (node_iter = ng->saAmfNGNodeList.begin();
		node_iter != ng->saAmfNGNodeList.end();
		node_iter++) {

		node = avd_node_get(*node_iter);
		osafassert(node);

		if (su->sg_of_su->sg_ncs_spec == true) {
			for (su_temp = node->list_of_ncs_su; su_temp != NULL; su_temp = su_temp->avnd_list_su_next) {
				if (su_temp->sg_of_su == su->sg_of_su)
					break;
			}
		}

		if (su->sg_of_su->sg_ncs_spec == false) {
			for (su_temp = node->list_of_su; su_temp != NULL; su_temp = su_temp->avnd_list_su_next) {
				if (su_temp->sg_of_su == su->sg_of_su)
					break;
			}
		}

		if (su_temp == NULL)
			goto done;
	}

	/* All nodes already have an SU mapped for the SG. Return a node in the node group. */
	node = avd_node_get(*ng->saAmfNGNodeList.begin());
done:
	memcpy(&su->saAmfSUHostedByNode, &node->name, sizeof(SaNameT));
	TRACE_LEAVE2("hosted by %s", node->name.value);
	return node;
}

/**
 * Add SU to model
 * @param su
 */
static void su_add_to_model(AVD_SU *su)
{
	SaNameT dn;
	AVD_AVND *node;
	bool new_su = false;
	unsigned int rc;

	TRACE_ENTER2("%s", su->name.value);

	/* Check parent link to see if it has been added already */
	if (su->sg_of_su != NULL) {
		TRACE("already added");
		goto done;
	}

	/* Determine of the SU is added now, if so msg to amfnd needs to be sent */
	if (su_db->find(Amf::to_string(&su->name)) == NULL)
		new_su = true;

	avsv_sanamet_init(&su->name, &dn, "safSg");

	/*
	** Refresh the SG reference, by now it must exist.
	** An SU can be created (on the standby) from the checkpointing logic.
	** In that case the SG reference could now be NULL.
	*/
	su->sg_of_su = sg_db->find(Amf::to_string(&dn));
	osafassert(su->sg_of_su);

	if (su_db->find(Amf::to_string(&su->name)) == NULL) {
		rc = su_db->insert(Amf::to_string(&su->name), su);
		osafassert(rc == NCSCC_RC_SUCCESS);
	}
	su->su_type = sutype_db->find(Amf::to_string(&su->saAmfSUType));
	osafassert(su->su_type);
	avd_sutype_add_su(su);
	avd_sg_add_su(su);

	if (!su->su_is_external) {
		if (su->saAmfSUHostedByNode.length == 0) {
			/* This node has not been mapped yet, do it */
			su->su_on_node = map_su_to_node(su);
		} else {
			/* Already mapped, setup the node link */
			su->su_on_node = avd_node_get(&su->saAmfSUHostedByNode);
		}

		avd_node_add_su(su);
		node = su->su_on_node;
	} else {
		if (NULL == avd_cb->ext_comp_info.ext_comp_hlt_check) {
			/* This is an external SU and we need to create the 
			   supporting info. */
			avd_cb->ext_comp_info.ext_comp_hlt_check = new AVD_AVND;
			memset(avd_cb->ext_comp_info.ext_comp_hlt_check, 0, sizeof(AVD_AVND));
			avd_cb->ext_comp_info.local_avnd_node = avd_node_find_nodeid(avd_cb->node_id_avd);

			if (NULL == avd_cb->ext_comp_info.local_avnd_node) {
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

				LOG_ER("%s: avd_snd_su_msg failed %s", __FUNCTION__, su->name.value);
				goto done;
			}

			su->set_oper_state(SA_AMF_OPERATIONAL_ENABLED);
		} else
			su->set_oper_state(SA_AMF_OPERATIONAL_DISABLED);
	}

done:
	avd_saImmOiRtObjectUpdate(&su->name, "saAmfSUOperState",
			SA_IMM_ATTR_SAUINT32T, &su->saAmfSUOperState);

	avd_saImmOiRtObjectUpdate(&su->name, "saAmfSUPreInstantiable",
		SA_IMM_ATTR_SAUINT32T, &su->saAmfSUPreInstantiable);

	avd_saImmOiRtObjectUpdate(&su->name, "saAmfSUHostedByNode",
		SA_IMM_ATTR_SANAMET, &su->saAmfSUHostedByNode);

	avd_saImmOiRtObjectUpdate(&su->name, "saAmfSUPresenceState",
		SA_IMM_ATTR_SAUINT32T, &su->saAmfSUPresenceState);

	avd_saImmOiRtObjectUpdate(&su->name, "saAmfSUReadinessState",
		SA_IMM_ATTR_SAUINT32T, &su->saAmfSuReadinessState);

	TRACE_LEAVE();
}

SaAisErrorT avd_su_config_get(const SaNameT *sg_name, AVD_SG *sg)
{
	SaAisErrorT error, rc;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT su_name;
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
		NULL
	};

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>("SaImmAttrClassName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &className;

	error = immutil_saImmOmSearchInitialize_2(avd_cb->immOmHandle, sg_name, SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR, &searchParam,
		configAttributes, &searchHandle);

	if (SA_AIS_OK != error) {
		LOG_ER("%s: saImmOmSearchInitialize_2 failed: %u", __FUNCTION__, error);
		goto done1;
	}

	while ((rc = immutil_saImmOmSearchNext_2(searchHandle, &su_name,
					(SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK) {
		if (!is_config_valid(&su_name, attributes, NULL)) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}

		if ((su = su_create(&su_name, attributes)) == NULL) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}

		su_add_to_model(su);

		if (avd_comp_config_get(&su_name, su) != SA_AIS_OK) {
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
	TRACE_ENTER2("'%s' %s => %s", name.value,
				 avd_pres_state_name[saAmfSUPresenceState],
				 avd_pres_state_name[pres_state]);

	SaAmfPresenceStateT old_state = saAmfSUPresenceState;
	saAmfSUPresenceState = pres_state;
	/* only log for certain changes, see notifications in spec */
	if (pres_state == SA_AMF_PRESENCE_UNINSTANTIATED ||
	    pres_state == SA_AMF_PRESENCE_INSTANTIATED ||
	    pres_state == SA_AMF_PRESENCE_RESTARTING) {

		saflog(LOG_NOTICE, amfSvcUsrName, "%s PresenceState %s => %s",
			name.value, avd_pres_state_name[old_state],
			avd_pres_state_name[saAmfSUPresenceState]);

		avd_send_su_pres_state_chg_ntf(&name, old_state,
			saAmfSUPresenceState);
	}
	avd_saImmOiRtObjectUpdate(&name, "saAmfSUPresenceState",
		SA_IMM_ATTR_SAUINT32T, &saAmfSUPresenceState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_PRES_STATE);
	if ((saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED) && (surestart == true)) {
		TRACE("setting surestart flag to false");
		surestart = false;
	}
}

void AVD_SU::set_oper_state(SaAmfOperationalStateT oper_state) {
	SaAmfOperationalStateT old_state = saAmfSUOperState;

	if (saAmfSUOperState == oper_state)
		return;

	osafassert(oper_state <= SA_AMF_OPERATIONAL_DISABLED);
	TRACE_ENTER2("'%s' %s => %s", name.value,
		avd_oper_state_name[saAmfSUOperState],
		avd_oper_state_name[oper_state]);

	saflog(LOG_NOTICE, amfSvcUsrName, "%s OperState %s => %s", name.value,
		avd_oper_state_name[saAmfSUOperState],
		avd_oper_state_name[oper_state]);

	saAmfSUOperState = oper_state;

	avd_send_oper_chg_ntf(&name, SA_AMF_NTFID_SU_OP_STATE, old_state,
		saAmfSUOperState);

	avd_saImmOiRtObjectUpdate(&name, "saAmfSUOperState",
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

	AVD_COMP *comp = NULL;
	osafassert(readiness_state <= SA_AMF_READINESS_STOPPING);
	TRACE_ENTER2("'%s' %s", name.value,
		avd_readiness_state_name[readiness_state]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s ReadinessState %s => %s",
		name.value,
		avd_readiness_state_name[saAmfSuReadinessState],
		avd_readiness_state_name[readiness_state]);
	saAmfSuReadinessState = readiness_state;
	if (surestart == false)
		avd_saImmOiRtObjectUpdate(&name, "saAmfSUReadinessState",
				SA_IMM_ATTR_SAUINT32T, &saAmfSuReadinessState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_READINESS_STATE);

	/* Since Su readiness state has changed, we need to change it for all the
	 * component in this SU.*/
	comp = list_of_comp;
	while (comp != NULL) {
		SaAmfReadinessStateT saAmfCompReadinessState;
		if ((saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
				(comp->saAmfCompOperState == SA_AMF_OPERATIONAL_ENABLED)) {
			saAmfCompReadinessState = SA_AMF_READINESS_IN_SERVICE;
		} else if((saAmfSuReadinessState == SA_AMF_READINESS_STOPPING) &&
				(comp->saAmfCompOperState == SA_AMF_OPERATIONAL_ENABLED)) {
			saAmfCompReadinessState = SA_AMF_READINESS_STOPPING;
		} else
			saAmfCompReadinessState = SA_AMF_READINESS_OUT_OF_SERVICE;

		avd_comp_readiness_state_set(comp, saAmfCompReadinessState);
		comp = comp->su_comp_next;
	}

	TRACE_LEAVE();
}

void AVD_SU::set_admin_state(SaAmfAdminStateT admin_state) {
	SaAmfAdminStateT old_state = saAmfSUAdminState;

	osafassert(admin_state <= SA_AMF_ADMIN_SHUTTING_DOWN);
	TRACE_ENTER2("'%s' %s => %s", name.value,
		avd_adm_state_name[old_state], avd_adm_state_name[admin_state]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s AdmState %s => %s", name.value,
		avd_adm_state_name[saAmfSUAdminState],
		avd_adm_state_name[admin_state]);
	saAmfSUAdminState = admin_state;
	avd_saImmOiRtObjectUpdate(&name, "saAmfSUAdminState",
		SA_IMM_ATTR_SAUINT32T, &saAmfSUAdminState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_ADMIN_STATE);
	avd_send_admin_state_chg_ntf(&name, SA_AMF_NTFID_SU_ADMIN_STATE,
			old_state, saAmfSUAdminState);
}

void AVD_SU::unlock(SaImmOiHandleT immoi_handle, SaInvocationT invocation) {
	bool is_oper_successful = true;

	TRACE_ENTER2("'%s'", name.value);
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
		LOG_IN("SU '%s' is not in service", name.value);
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
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_FAILED_OPERATION, NULL,
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

	TRACE_ENTER2("'%s'", name.value);
	/* Change the admin state to lock and return as cluster timer haven't expired.*/
	if (avd_cb->init_state == AVD_INIT_DONE) {
		set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
		set_admin_state(SA_AMF_ADMIN_LOCKED);
		avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
		goto done;
	}

	if (list_of_susi == NULL) {
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
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_FAILED_OPERATION, NULL,
				"SG redundancy model specific handler failed");
	}

done:
	TRACE_LEAVE();
}

void AVD_SU::shutdown(SaImmOiHandleT immoi_handle, SaInvocationT invocation) {
	TRACE_ENTER2("'%s'", name.value);
	lock(immoi_handle, invocation, SA_AMF_ADMIN_SHUTTING_DOWN);
	TRACE_LEAVE();
}

void AVD_SU::lock_instantiation(SaImmOiHandleT immoi_handle,
                                SaInvocationT invocation) {
	AVD_AVND *node = get_node_ptr();

	TRACE_ENTER2("'%s'", name.value);

	/* For non-preinstantiable SU lock-inst is same as lock */
	if (saAmfSUPreInstantiable == false) {
		set_admin_state(SA_AMF_ADMIN_LOCKED_INSTANTIATION);
		avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
		goto done;
	}

	if (list_of_susi != NULL) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
			"SIs still assigned to this SU '%s'", name.value);
		goto done;
	}

	if ((saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATING) ||
		(saAmfSUPresenceState == SA_AMF_PRESENCE_TERMINATING) ||
			(saAmfSUPresenceState == SA_AMF_PRESENCE_RESTARTING)) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
				"'%s' presence state is '%u'", name.value, saAmfSUPresenceState);
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
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
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

	TRACE_ENTER2("'%s'", name.value);

	if (list_of_comp == NULL) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
				"There is no component configured for SU '%s'.", name.value);
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
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
			"Can't instantiate '%s', whose presence state is '%u'", name.value,
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
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
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
	TRACE_ENTER2("'%s'", name.value);

	if (saAmfSUOperState == SA_AMF_OPERATIONAL_ENABLED) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_NO_OP, NULL,
			"Admin repair request for '%s', op state already enabled", name.value);
		goto done;
	}

	if ((saAmfSUOperState == SA_AMF_OPERATIONAL_DISABLED) &&
			(su_on_node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED)) {
		/* This means that node on which this su is hosted, is absent. */
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
			"Admin repair request for '%s', hosting node'%s' is absent",
			name.value, su_on_node->name.value);
		goto done;
	}

	/* forward the admin op req to the node director */
	if (avd_admin_op_msg_snd(&name, AVSV_SA_AMF_SU, SA_AMF_ADMIN_REPAIRED,
			su_on_node) == NCSCC_RC_SUCCESS) {
		pend_cbk.admin_oper = SA_AMF_ADMIN_REPAIRED;
		pend_cbk.invocation = invocation;
	}
	else {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TIMEOUT, NULL,
			"Admin op request send failed '%s'", name.value);
	}

done:
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
	AVD_SU    *su, *su_ptr;
	AVD_AVND  *node;

	TRACE_ENTER2("%llu, '%s', %llu", invocation, su_name->value, op_id);

	if ( op_id > SA_AMF_ADMIN_SHUTDOWN && op_id != SA_AMF_ADMIN_REPAIRED) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_NOT_SUPPORTED, NULL,
				"Unsupported admin op for SU: %llu", op_id);
		goto done;
	}

	if (NULL == (su = su_db->find(Amf::to_string(su_name)))) {
		LOG_CR("SU '%s' not found", su_name->value);
		/* internal error? osafassert instead? */
		goto done;
	}

	if ((su->sg_of_su->sg_ncs_spec == true)	&&
			(cb->node_id_avd == su->su_on_node->node_info.nodeId)) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_NOT_SUPPORTED, NULL, 
				"Admin operation on Active middleware SU is not allowed");
		goto done;
	}

	/* Avoid multiple admin operations on other SUs belonging to the same SG. */
	for (su_ptr = su->sg_of_su->list_of_su; su_ptr != NULL; su_ptr = su_ptr->sg_list_su_next) {
		/* su's sg_fsm_state is checked below, just check other su. */
		if ((su != su_ptr) && (su_ptr->pend_cbk.invocation != 0)) {
			report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
					"Admin operation is already going on (su'%s')", su_ptr->name.value);
			goto done;
		}
	}

	/* Avoid if any single Csi assignment is undergoing on SG. */
	if (csi_assignment_validate(su->sg_of_su) == true) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
				"Single Csi assignment undergoing on (sg'%s')", su->sg_of_su->name.value);
		goto done;
	}

	if (su->sg_of_su->sg_fsm_state != AVD_SG_FSM_STABLE) {
		if((su->sg_of_su->sg_fsm_state != AVD_SG_FSM_SU_OPER) ||
				(su->saAmfSUAdminState != SA_AMF_ADMIN_SHUTTING_DOWN) || 
				(op_id != SA_AMF_ADMIN_LOCK)) {
			report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
					"SG state is not stable"); /* whatever that means... */
			goto done;
		} else {
			/* This means that shutdown was going on and lock has
			   been issued.  In this case, response to shutdown
			   and then allow lock operation to proceed. */
			report_admin_op_error(immoi_handle, su->pend_cbk.invocation,
					SA_AIS_ERR_INTERRUPT, &su->pend_cbk,
					"SU lock has been issued '%s'", su->name.value);
		}
	}
	/* if Tolerance timer is running for any SI's withing this SG, then return SA_AIS_ERR_TRY_AGAIN */
	if (sg_is_tolerance_timer_running_for_any_si(su->sg_of_su)) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
				"Tolerance timer is running for some of the SI's in the SG '%s', " 
				"so differing admin opr",su->sg_of_su->name.value);
		goto done;
	}

	if ( ((su->saAmfSUAdminState == SA_AMF_ADMIN_UNLOCKED) && (op_id == SA_AMF_ADMIN_UNLOCK)) ||
	     ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED)   && (op_id == SA_AMF_ADMIN_LOCK))   ||
	     ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
	      (op_id == SA_AMF_ADMIN_LOCK_INSTANTIATION))                                     ||
	     ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED)   && (op_id == SA_AMF_ADMIN_UNLOCK_INSTANTIATION))) {

		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_NO_OP, NULL,
				"Admin operation (%llu) has no effect on current state (%u)", op_id,
				su->saAmfSUAdminState);
		goto done;
	}

	if (((su->saAmfSUAdminState == SA_AMF_ADMIN_UNLOCKED) &&
		  (op_id != SA_AMF_ADMIN_LOCK) &&
		  (op_id != SA_AMF_ADMIN_SHUTDOWN) &&
		  (op_id != SA_AMF_ADMIN_REPAIRED)) ||
	     ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED) &&
		  (op_id != SA_AMF_ADMIN_UNLOCK) &&
		  (op_id != SA_AMF_ADMIN_REPAIRED) &&
		  (op_id != SA_AMF_ADMIN_LOCK_INSTANTIATION))  ||
	     ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
		  (op_id != SA_AMF_ADMIN_UNLOCK_INSTANTIATION) &&
		  (op_id != SA_AMF_ADMIN_REPAIRED)) ||
	     ((su->saAmfSUAdminState != SA_AMF_ADMIN_UNLOCKED) &&
		  (op_id == SA_AMF_ADMIN_SHUTDOWN))) {

		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_BAD_OPERATION, NULL,
				"State transition invalid, state %u, op %llu", su->saAmfSUAdminState, op_id);
		goto done;
	}

	node = su->get_node_ptr();
	if (node->admin_node_pend_cbk.admin_oper != 0) {
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_TRY_AGAIN, NULL,
				"Node'%s' hosting SU'%s', undergoing admin operation'%u'", node->name.value,
				su->name.value, node->admin_node_pend_cbk.admin_oper);
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
	default:
		report_admin_op_error(immoi_handle, invocation, SA_AIS_ERR_INVALID_PARAM, NULL,
				"Unsupported admin op");
		break;
	}

done:

	TRACE_LEAVE2();
}

static SaAisErrorT su_rt_attr_cb(SaImmOiHandleT immOiHandle,
	const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	AVD_SU *su = su_db->find(Amf::to_string(objectName));
	SaImmAttrNameT attributeName;
	int i = 0;
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER2("%s", objectName->value);

	while ((attributeName = attributeNames[i++]) != NULL) {
		if (!strcmp("saAmfSUAssignedSIs", attributeName)) {
#if 0
			/*  TODO */
			SaUint32T saAmfSUAssignedSIs = su->saAmfSUNumCurrActiveSIs + su->saAmfSUNumCurrStandbySIs;
			avd_saImmOiRtObjectUpdate_sync(immOiHandle, objectName,
				attributeName, SA_IMM_ATTR_SAUINT32T, &saAmfSUAssignedSIs);
#endif
		} else if (!strcmp("saAmfSUNumCurrActiveSIs", attributeName)) {
			rc = avd_saImmOiRtObjectUpdate_sync(objectName, attributeName,
				SA_IMM_ATTR_SAUINT32T, &su->saAmfSUNumCurrActiveSIs);
		} else if (!strcmp("saAmfSUNumCurrStandbySIs", attributeName)) {
			rc = avd_saImmOiRtObjectUpdate_sync(objectName, attributeName,
				SA_IMM_ATTR_SAUINT32T, &su->saAmfSUNumCurrStandbySIs);
		} else if (!strcmp("saAmfSURestartCount", attributeName)) {
			rc = avd_saImmOiRtObjectUpdate_sync(objectName, attributeName,
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

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {

		if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) ||
				(attr_mod->modAttr.attrValues == NULL)) {
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
				report_ccb_validation_error(opdata, "'%s' is not stable",su->sg_of_su->name.value); 
				goto done;
			}

			if (su_failover > SA_TRUE) {
				report_ccb_validation_error(opdata, "Invalid saAmfSUFailover SU:'%s'", su->name.value);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else if (!strcmp(attr_mod->modAttr.attrName, "saAmfSUMaintenanceCampaign")) {
			if (value_is_deleted == true)
				continue;
			AVD_SU *su = su_db->find(Amf::to_string(&opdata->objectName));

			if (su->saAmfSUMaintenanceCampaign.length > 0) {
				report_ccb_validation_error(opdata, "saAmfSUMaintenanceCampaign already set for %s",
						su->name.value);
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
			if (sutype_db->find(Amf::to_string(&sutype_name)) == NULL) {
				report_ccb_validation_error(opdata, "SU Type not found '%s'", sutype_name.value);
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

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	if (strstr((char *)opdata->objectName.value, "safApp=OpenSAF") != NULL)
		is_app_su = 0;

	su = su_db->find(Amf::to_string(&opdata->objectName));
	osafassert(su != NULL);

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
static bool node_admin_state_is_valid_for_su_create(const SaNameT *su_dn,
        const SaImmAttrValuesT_2 **attributes,
        const CcbUtilOperationData_t *opdata)
{
	TRACE_ENTER2("%s", su_dn->value);
	SaNameT node_name = {0};
	(void) immutil_getAttr("saAmfSUHostNodeOrNodeGroup", attributes, 0, &node_name);
	if (node_name.length == 0) {
		// attribute empty but this is probably not an error, just trace
		TRACE("Create '%s', saAmfSUHostNodeOrNodeGroup not configured",
			su_dn->value);
		TRACE_LEAVE();
		return false;
	}

	if (strncmp((char*)node_name.value, "safAmfNode=", 11) != 0) {
		// attribute non empty but does not contain a node DN, not OK
		amflog(SA_LOG_SEV_NOTICE,
			"Create '%s', saAmfSUHostNodeOrNodeGroup not configured with a node (%s)",
			su_dn->value, node_name.value);
		TRACE_LEAVE();
		return false;
	}

	const AVD_AVND *node = avd_node_get(&node_name);
	if (node == NULL) {
		if (opdata == NULL || ccbutil_getCcbOpDataByDN(opdata->ccbId, &node_name) == NULL) {
			// node must exist in the current model, or created in the same CCB
			amflog(SA_LOG_SEV_WARNING,
				"Create '%s', configured with a non existing node (%s)",
				su_dn->value, node_name.value);
			TRACE_LEAVE();
			return false;
		} else {
			// check admin state of the new node
			SaAmfAdminStateT admin_state;
			const CcbUtilOperationData_t *t_opdata = ccbutil_getCcbOpDataByDN(opdata->ccbId, &node_name);
			immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNodeAdminState"),
				t_opdata->param.create.attrValues, 0, &admin_state);
			if (admin_state != SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
				TRACE("Create '%s', configured node '%s' is not locked instantiation",
					su_dn->value, node_name.value);
				TRACE_LEAVE();
				return false;
			}
		}

	} else {
		// configured with a node DN, accept only locked-in state
		if (node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			TRACE("Create '%s', configured node '%s' is not locked instantiation",
				su_dn->value, node_name.value);
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
static bool sg_admin_state_is_valid_for_su_create(const SaNameT *su_dn,
        const SaImmAttrValuesT_2 **attributes,
        const CcbUtilOperationData_t *opdata)
{
	SaNameT sg_name = {0};
	SaAmfAdminStateT admin_state;

	avsv_sanamet_init(su_dn, &sg_name, "safSg");
	const AVD_SG *sg = sg_db->find(Amf::to_string(&sg_name));
	if (sg != NULL) {
		admin_state = sg->saAmfSGAdminState;
	} else {
		// SG does not exist in current model, check CCB
		const CcbUtilOperationData_t *tmp =
			ccbutil_getCcbOpDataByDN(opdata->ccbId, &sg_name);
		osafassert(tmp != NULL); // already validated

		(void) immutil_getAttr("saAmfSGAdminState",
			tmp->param.create.attrValues, 0, &admin_state);
	}

	if (admin_state != SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
		TRACE("'%s' created UNLOCKED but '%s' is not locked instantiation",
			su_dn->value, sg_name.value);
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
static bool is_ccb_create_config_valid(const SaNameT *dn,
                                       const SaImmAttrValuesT_2 **attributes,
                                       const CcbUtilOperationData_t *opdata)
{
	SaAmfAdminStateT admstate;
	SaAisErrorT rc;

	assert(opdata != NULL);  // must be called in CCB context

	bool is_mw_su = false;
	if (strstr((char *)dn->value, "safApp=OpenSAF") != NULL)
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
			dn->value, admstate);
		return false;
	}

	if (node_admin_state_is_valid_for_su_create(dn, attributes, opdata))
		return true;

	if (sg_admin_state_is_valid_for_su_create(dn, attributes, opdata))
		return true;

	amflog(SA_LOG_SEV_NOTICE, "CCB %d creation of '%s' failed",
		opdata->ccbId, dn->value);
	report_ccb_validation_error(opdata,
		"SG or node not configured properly to allow creation of UNLOCKED SU");

	return false;
}

static SaAisErrorT su_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(&opdata->objectName, opdata->param.create.attrValues, opdata) &&
		    is_ccb_create_config_valid(&opdata->objectName, opdata->param.create.attrValues, opdata))
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

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	su = su_db->find(Amf::to_string(&opdata->objectName));

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
		/* Attribute value removed */
		if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || (attr_mod->modAttr.attrValues == NULL))
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
				su->saAmfSUMaintenanceCampaign.length = 0;
				TRACE("saAmfSUMaintenanceCampaign cleared for '%s'", su->name.value);
			} else {
				osafassert(su->saAmfSUMaintenanceCampaign.length == 0);
				su->saAmfSUMaintenanceCampaign = *((SaNameT *)attr_mod->modAttr.attrValues[0]);
				TRACE("saAmfSUMaintenanceCampaign set to '%s' for '%s'",
					  su->saAmfSUMaintenanceCampaign.value, su->name.value);
			}
		} else if (!strcmp(attr_mod->modAttr.attrName, "saAmfSUType")) {
			struct avd_sutype *sut;
			SaNameT sutype_name = *(SaNameT*) attr_mod->modAttr.attrValues[0];
			TRACE("Modified saAmfSUType from '%s' to '%s'", su->saAmfSUType.value, sutype_name.value);
			sut = sutype_db->find(Amf::to_string(&sutype_name));
			avd_sutype_remove_su(su);
			su->saAmfSUType = sutype_name;
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

	TRACE_ENTER2("'%s'", su->name.value);

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
				compcstype_list.insert(Amf::to_string(&compcstype->name));
			}
			TRACE("Standby Amfd, comp '%s' not deleted", comp->comp_info.name.value);

			for (std::set<std::string>::const_iterator iter1 = compcstype_list.begin();
					iter1 != compcstype_list.end(); ++iter1) {
				AVD_COMPCS_TYPE *compcstype = compcstype_db->find(*iter1);
				if (compcstype->comp == comp) {
					TRACE("Standby Amfd, compcstype '%s' not deleted",
							compcstype->name.value);
					compcstype_db->erase(Amf::to_string(&compcstype->name));
					delete compcstype;
				}
			}
			compcstype_list.clear();
			/* Delete the Comp. */
			struct CcbUtilOperationData opdata;
			memset(&opdata.objectName, 0, sizeof(SaNameT));
			memcpy(opdata.objectName.value, comp->comp_info.name.value,
					comp->comp_info.name.length);
			opdata.objectName.length = comp->comp_info.name.length;
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
		param.name = su->name;
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

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		su = su_create(&opdata->objectName, opdata->param.create.attrValues);
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
	TRACE("%s saAmfSUNumCurrActiveSIs=%u", name.value,
		saAmfSUNumCurrActiveSIs);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_SI_CURR_ACTIVE);
}

void AVD_SU::dec_curr_act_si() {
	osafassert(saAmfSUNumCurrActiveSIs > 0);
	saAmfSUNumCurrActiveSIs--;
	TRACE("%s saAmfSUNumCurrActiveSIs=%u", name.value,
		saAmfSUNumCurrActiveSIs);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_SI_CURR_ACTIVE);
}

void AVD_SU::inc_curr_stdby_si() {
	saAmfSUNumCurrStandbySIs++;
	osafassert(saAmfSUNumCurrStandbySIs <= sg_of_su->saAmfSGMaxStandbySIsperSU);
	TRACE("%s saAmfSUNumCurrStandbySIs=%u", name.value,
		saAmfSUNumCurrStandbySIs);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_SI_CURR_STBY);
}

void AVD_SU::dec_curr_stdby_si() {
	osafassert(saAmfSUNumCurrStandbySIs > 0);
	saAmfSUNumCurrStandbySIs--;
	TRACE("%s saAmfSUNumCurrStandbySIs=%u", name.value,
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
	AVD_AVND *su_node_ptr = NULL;
	AVSV_PARAM_INFO param;
	memset(((uint8_t *)&param), '\0', sizeof(AVSV_PARAM_INFO));

	TRACE_ENTER();

	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) {
		TRACE_LEAVE2("avd is not in active state");
		return;
	}

	param.class_id = AVSV_SA_AMF_SU;
	param.act = AVSV_OBJ_OPR_MOD;
	param.name = name;

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

	/*Update this value on the node hosting this SU*/
	su_node_ptr = get_node_ptr();
	if ((su_node_ptr) && ((su_node_ptr->node_state == AVD_AVND_STATE_PRESENT) ||
			(su_node_ptr->node_state == AVD_AVND_STATE_NO_CONFIG) ||
			(su_node_ptr->node_state == AVD_AVND_STATE_NCS_INIT))) {
		if (avd_snd_op_req_msg(avd_cb, su_node_ptr, &param) != NCSCC_RC_SUCCESS) {
			LOG_ER("%s:failed for %s",__FUNCTION__, su_node_ptr->name.value);
		}
	}

	TRACE_LEAVE();
}

void AVD_SU::set_su_failover(bool value) {
	saAmfSUFailover = value;
	TRACE("Modified saAmfSUFailover to '%u' for '%s'",
		saAmfSUFailover, name.value);
	send_attribute_update(saAmfSUFailOver_ID);
}

/**
 * Delete all SUSIs assigned to the SU.
 *
 */
void AVD_SU::delete_all_susis(void) {
	TRACE_ENTER2("'%s'", name.value);

	while (list_of_susi != NULL) {
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

	TRACE_ENTER2("'%s'", name.value);

	for (; susi != NULL; susi = susi->su_next) {
		if (susi->fsm != AVD_SU_SI_STATE_UNASGN) {
			susi->state = SA_AMF_HA_QUIESCED;
			susi->fsm = AVD_SU_SI_STATE_ASGND;
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

	TRACE_ENTER2("'%s'", name.value);

	for (; susi != NULL; susi = susi->su_next) {
		if (susi->fsm != AVD_SU_SI_STATE_UNASGN) {
			susi->fsm = AVD_SU_SI_STATE_ASGND;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, susi, AVSV_CKPT_AVD_SI_ASS);
			avd_pg_susi_chg_prc(avd_cb, susi);
		}
	}

	TRACE_LEAVE();
}

void AVD_SU::set_term_state(bool state) {
	term_state = state;
	TRACE("%s term_state %u", name.value, term_state);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_TERM_STATE);
}

void AVD_SU::set_su_switch(SaToggleState state) {
	su_switch = state;
	TRACE("%s su_switch %u", name.value, su_switch);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, this, AVSV_CKPT_SU_SWITCH);
}

struct avd_avnd_tag *AVD_SU::get_node_ptr(void) {
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
	struct avd_avnd_tag *node = get_node_ptr();
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


/**
 * Checks if the SU can be made instantiated. 
 * @param su
 * @return true if SU can be made in-service
 */
bool AVD_SU::is_instantiable(void) {
        struct avd_avnd_tag *node = get_node_ptr();
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
	avd_saImmOiRtObjectUpdate(&name, "saAmfSUPreInstantiable",
			SA_IMM_ATTR_SAUINT32T, &saAmfSUPreInstantiable);
	TRACE("%s saAmfSUPreInstantiable %u", name.value, value);
}

/**
 * resets the assign flag for all contained components
 */
void AVD_SU::reset_all_comps_assign_flag() {
	AVD_COMP *t_comp = list_of_comp;
	// TODO(hafe) add and use a comp method
	while (t_comp != NULL) {
		t_comp->assign_flag = false;
		t_comp = t_comp->su_comp_next;
	}
}

/**
 * Finds an unassigned component that provides the specified CSType
 * @param cstype
 * @return
 */
AVD_COMP *AVD_SU::find_unassigned_comp_that_provides_cstype(const SaNameT *cstype) {
	AVD_COMP *l_comp = list_of_comp;
	while (l_comp != NULL) {
		bool npi_is_assigned = false;
		AVD_COMP_TYPE *comptype = comptype_db->find(Amf::to_string(&l_comp->saAmfCompType));
		osafassert(comptype);
		if ((comptype->saAmfCtCompCategory == SA_AMF_COMP_LOCAL) && is_comp_assigned_any_csi(l_comp))
			npi_is_assigned = true;
		
		if ((l_comp->assign_flag == false) && (npi_is_assigned == false)) {
			AVD_COMPCS_TYPE *cst = avd_compcstype_find_match(cstype, l_comp);
			if (cst != NULL)
				break;
		}
		l_comp = l_comp->su_comp_next;
	}

	return l_comp;
}

/**
 * Disables all components since SU is disabled and out of service.
 * Takes care of response to IMM for any admin operation pending on components.
 * @param result
 */
void AVD_SU::disable_comps(SaAisErrorT result)
{
	AVD_COMP *comp;
	for (comp = list_of_comp; comp; comp = comp->su_comp_next) {
		comp->curr_num_csi_actv = 0;
		comp->curr_num_csi_stdby = 0;
		avd_comp_oper_state_set(comp, SA_AMF_OPERATIONAL_DISABLED);
		if (comp->saAmfCompPresenceState != SA_AMF_PRESENCE_TERMINATION_FAILED)
			avd_comp_pres_state_set(comp, SA_AMF_PRESENCE_UNINSTANTIATED);

		/*
		   Mark a term_failed component uninstantiated when node is rebooted.
		   When node goes for reboot then AMFD marks node absent. If node does
		   not go for reboot then term_fail state of comp will be cleared 
		   as part of admin repair operation.
		 */
		if ((comp->saAmfCompPresenceState == SA_AMF_PRESENCE_TERMINATION_FAILED) &&
				(su_on_node->node_state == AVD_AVND_STATE_ABSENT)) {
				avd_comp_pres_state_set(comp, SA_AMF_PRESENCE_UNINSTANTIATED);
		}
		comp->saAmfCompRestartCount = 0;
		comp_complete_admin_op(comp, result);
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, comp, AVSV_CKPT_AVD_COMP_CONFIG);
	}
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
 * @brief    Checks if modification of assignment is sent for any SUSI in SU.
 * @result   true/false  
 */
bool AVD_SU::any_susi_fsm_in_modify()
{
        for (AVD_SU_SI_REL *susi = list_of_susi; susi; susi = susi->su_next) {
                if (susi->fsm == AVD_SU_SI_STATE_MODIFY)
                        return true;
        }
        return false;
}
/**
 * @brief    Checks if deletion of assignment is sent for any SUSI in SU.
 * @result   true/false  
 */
bool AVD_SU::any_susi_fsm_in_unasgn()
{
        for (AVD_SU_SI_REL *susi = list_of_susi; susi; susi = susi->su_next) {
                if (susi->fsm == AVD_SU_SI_STATE_UNASGN)
                        return true;
        }
        return false;
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
		LOG_NO("'%s' undergoing admin operation", name.value);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
	if ((saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATING) ||
			(saAmfSUPresenceState == SA_AMF_PRESENCE_TERMINATING) ||
			(saAmfSUPresenceState == SA_AMF_PRESENCE_RESTARTING)) {
		LOG_NO("'%s' has presence state '%u'", name.value,saAmfSUPresenceState);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}
	if ((saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATION_FAILED) ||
			(saAmfSUPresenceState == SA_AMF_PRESENCE_TERMINATION_FAILED)) {
		LOG_NO("'%s' is in repair pending state", name.value);
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}
	for (AVD_COMP *comp = list_of_comp; comp; comp = comp->su_comp_next) {
		rc = check_comp_stability(comp);
		if (rc != SA_AIS_OK)
			goto done;
        }
done:
	return rc;
}
