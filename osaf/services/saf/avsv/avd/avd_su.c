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

#include <avd_util.h>
#include <avd_su.h>
#include <avd_sutype.h>
#include <avd_imm.h>
#include <avd_ntf.h>
#include <avd_proc.h>

static NCS_PATRICIA_TREE su_db;

void avd_su_db_add(AVD_SU *su)
{
	unsigned int rc;

	if (avd_su_get(&su->name) == NULL) {
		rc = ncs_patricia_tree_add(&su_db, &su->tree_node);
		osafassert(rc == NCSCC_RC_SUCCESS);
	}
}

void avd_su_db_remove(AVD_SU *su)
{
	unsigned int rc = ncs_patricia_tree_del(&su_db, &su->tree_node);
	osafassert(rc == NCSCC_RC_SUCCESS);
}

AVD_SU *avd_su_new(const SaNameT *dn)
{
	SaNameT sg_name;
	AVD_SU *su;

	su = calloc(1, sizeof(*su));
	osafassert(su);
	
	memcpy(su->name.value, dn->value, dn->length);
	su->name.length = dn->length;
	su->tree_node.key_info = (uint8_t *)&(su->name);
	avsv_sanamet_init(dn, &sg_name, "safSg");
	su->saAmfSUFailover = false;
	su->term_state = false;
	su->su_switch = AVSV_SI_TOGGLE_STABLE;
	su->saAmfSUPreInstantiable = false;
	/* saAmfSUOperState is set when the SU is added to model depending on
	 * node state. Initialized to invalid due to filtering in avd_su_oper_state_set. */
	su->saAmfSUOperState = 0;
	su->saAmfSUPresenceState = SA_AMF_PRESENCE_UNINSTANTIATED;
	su->saAmfSuReadinessState = SA_AMF_READINESS_OUT_OF_SERVICE;
	su->su_act_state = AVD_SU_NO_STATE;
	su->su_is_external = false;

	return su;
}

/**
 * Delete the SU from the model. Check point with peer. Send delete order
 * to node director. Delete all contained components.
 * 
 * @param i_su
 */
void avd_su_delete(AVD_SU *su)
{
	TRACE_ENTER2("'%s'", su->name.value);
	
	/* All the components under this SU should have been deleted
	 * by now, just do the sanity check to confirm it is done 
	 */
	osafassert(su->list_of_comp == NULL);

	m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, su, AVSV_CKPT_AVD_SU_CONFIG);
	avd_node_remove_su(su);
	avd_sutype_remove_su(su);
	avd_su_db_remove(su);
	avd_sg_remove_su(su);
	free(su);

	TRACE_LEAVE();
}

AVD_SU *avd_su_get(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_SU *)ncs_patricia_tree_get(&su_db, (uint8_t *)&tmp);
}

/**
 * Return an SU object if it exist, otherwise create it and
 * return a reference to the new object.
 * @param dn
 * 
 * @return AVD_SU*
 */
AVD_SU *avd_su_get_or_create(const SaNameT *dn)
{
	AVD_SU *su = avd_su_get(dn);

	if (!su) {
		TRACE("'%s' does not exist, creating it", dn->value);
		su = avd_su_new(dn);
		osafassert(su != NULL);
		avd_su_db_add(su);
	}

	return su;
}

AVD_SU *avd_su_getnext(const SaNameT *dn)
{
	SaNameT tmp = {0};

	tmp.length = dn->length;
	memcpy(tmp.value, dn->value, tmp.length);

	return (AVD_SU *)ncs_patricia_tree_getnext(&su_db, (uint8_t *)&tmp);
}
/**
 * @brief   gets the current no of assignmnents on a SU for a particular state
 *
 * @param[in] su
 * @param[in] ha_state  
 *
 * @return returns current assignment cnt
 */
uint32_t avd_su_get_current_no_of_assignments(AVD_SU *su, SaAmfHAStateT ha_state)
{
	AVD_SU_SI_REL *curr_susi;
	uint32_t curr_assignment_cnt = 0;

	for (curr_susi = su->list_of_susi;curr_susi != NULL;curr_susi = curr_susi->su_next) {
		if (curr_susi->state == ha_state)
			curr_assignment_cnt++;
	}

	return curr_assignment_cnt;
}

void avd_su_remove_comp(AVD_COMP *comp)
{
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

	if (su_ref->list_of_comp == NULL) {
		/* Revert to def val */
		su_ref->saAmfSUPreInstantiable = false;
	}
}

void avd_su_add_comp(AVD_COMP *comp)
{
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
		LOG_ER("No parent to '%s' ", dn->value);
		return 0;
	}

	if (strncmp(++parent, "safSg=", 6) != 0) {
		LOG_ER("Wrong parent '%s' to '%s' ", parent, dn->value);
		return 0;
	}

	rc = immutil_getAttr("saAmfSUType", attributes, 0, &saAmfSUType);
	osafassert(rc == SA_AIS_OK);

	if ((sut = avd_sutype_get(&saAmfSUType)) != NULL) {
		saAmfSutIsExternal = sut->saAmfSutIsExternal;
	} else {
		/* SU type does not exist in current model, check CCB if passed as param */
		if (opdata == NULL) {
			LOG_ER("'%s' does not exist in model", saAmfSUType.value);
			return 0;
		}

		if ((tmp = ccbutil_getCcbOpDataByDN(opdata->ccbId, &saAmfSUType)) == NULL) {
			LOG_ER("'%s' does not exist in existing model or in CCB",
				saAmfSUType.value);
			return 0;
		}

		rc = immutil_getAttr("saAmfSutIsExternal", tmp->param.create.attrValues, 0, &saAmfSutIsExternal);
		osafassert(rc == SA_AIS_OK);
	}

	/* Validate that a configured node or node group exist */
	if (immutil_getAttr("saAmfSUHostNodeOrNodeGroup", attributes, 0, &saAmfSUHostNodeOrNodeGroup) == SA_AIS_OK) {
		if (strncmp((char*)saAmfSUHostNodeOrNodeGroup.value, "safAmfNode=", 11) == 0) {
			if (avd_node_get(&saAmfSUHostNodeOrNodeGroup) == NULL) {
				if (opdata == NULL) {
					LOG_ER("'%s' does not exist in model", saAmfSUHostNodeOrNodeGroup.value);
					return 0;
				}

				if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &saAmfSUHostNodeOrNodeGroup) == NULL) {
					LOG_ER("'%s' does not exist in existing model or in CCB", saAmfSUHostNodeOrNodeGroup.value);
					return 0;
				}
			}
		}
		else {
			if (avd_ng_get(&saAmfSUHostNodeOrNodeGroup) == NULL) {
				if (opdata == NULL) {
					LOG_ER("'%s' does not exist in model", saAmfSUHostNodeOrNodeGroup.value);
					return 0;
				}

				if (ccbutil_getCcbOpDataByDN(opdata->ccbId, &saAmfSUHostNodeOrNodeGroup) == NULL) {
					LOG_ER("'%s' does not exist in existing model or in CCB", saAmfSUHostNodeOrNodeGroup.value);
					return 0;
				}
			}
		}
	}

	/* Get value of saAmfSGSuHostNodeGroup */
	avsv_sanamet_init(dn, &sg_name, "safSg");
	sg = avd_sg_get(&sg_name);
	if (sg) {
		saAmfSGSuHostNodeGroup = sg->saAmfSGSuHostNodeGroup;
	} else {
		if (opdata == NULL) {
			LOG_ER("SG '%s' does not exist in model", sg_name.value);
			return 0;
		}

		if ((tmp = ccbutil_getCcbOpDataByDN(opdata->ccbId, &sg_name)) == NULL) {
			LOG_ER("SG '%s' does not exist in existing model or in CCB", sg_name.value);
			return 0;
		}

		(void) immutil_getAttr("saAmfSGSuHostNodeGroup",
			tmp->param.create.attrValues, 0, &saAmfSGSuHostNodeGroup);
	}

	/* If its a local SU, node or nodegroup must be configured */
	if (!saAmfSutIsExternal && 
	    (strstr((char *)saAmfSUHostNodeOrNodeGroup.value, "safAmfNode=") == NULL) &&
	    (strstr((char *)saAmfSUHostNodeOrNodeGroup.value, "safAmfNodeGroup=") == NULL) &&
	    (strstr((char *)saAmfSGSuHostNodeGroup.value, "safAmfNodeGroup=") == NULL)) {
		LOG_ER("node or node group configuration is missing for '%s'", dn->value);
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
		LOG_ER("node or node group configured for external SU '%s'", dn->value);
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
		SaNameT *ng_node_list_su, *ng_node_list_sg;
		int i, j, found;

		ng_of_su = avd_ng_get(&saAmfSUHostNodeOrNodeGroup);
		if (ng_of_su == NULL) {
			LOG_ER("Invalid saAmfSUHostNodeOrNodeGroup '%s' for '%s'",
				saAmfSUHostNodeOrNodeGroup.value, dn->value);
			return 0;
		}

		ng_of_sg = avd_ng_get(&saAmfSGSuHostNodeGroup);
		if (ng_of_su == NULL) {
			LOG_ER("Invalid saAmfSGSuHostNodeGroup '%s' for '%s'",
				saAmfSGSuHostNodeGroup.value, dn->value);
			return 0;
		}

		if (ng_of_su->number_nodes > ng_of_sg->number_nodes) {
			LOG_ER("SU node group '%s' contains more nodes than the SG node group '%s'",
				saAmfSUHostNodeOrNodeGroup.value, saAmfSGSuHostNodeGroup.value);
			return 0;
		}

		ng_node_list_su = ng_of_su->saAmfNGNodeList;

		for (i = 0; i < ng_of_su->number_nodes; i++) {
			found = 0;
			ng_node_list_sg = ng_of_sg->saAmfNGNodeList;

			for (j = 0; j < ng_of_sg->number_nodes; j++) {
				if (!memcmp(ng_node_list_su, ng_node_list_sg, sizeof(SaNameT)))
					found = 1;

				ng_node_list_sg++;
			}

			if (!found) {
				LOG_ER("SU node group '%s' is not a subset of the SG node group '%s'",
					saAmfSUHostNodeOrNodeGroup.value, saAmfSGSuHostNodeGroup.value);
				return 0;
			}

			ng_node_list_su++;
		}
	}

	// TODO maintenance campaign

	if ((immutil_getAttr("saAmfSUFailover", attributes, 0, &abool) == SA_AIS_OK) &&
	    (abool > SA_TRUE)) {
		LOG_ER("Invalid saAmfSUFailover %u for '%s'", abool, dn->value);
		return 0;
	}

	if ((immutil_getAttr("saAmfSUAdminState", attributes, 0, &admstate) == SA_AIS_OK) &&
	    !avd_admin_state_is_valid(admstate)) {
		LOG_ER("Invalid saAmfSUAdminState %u for '%s'", admstate, dn->value);
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
	if ((su = avd_su_get(dn)) == NULL) {
		if ((su = avd_su_new(dn)) == NULL)
			goto done;
	} else
		TRACE("already created, refreshing config...");

	error = immutil_getAttr("saAmfSUType", attributes, 0, &su->saAmfSUType);
	osafassert(error == SA_AIS_OK);

	if (immutil_getAttr("saAmfSURank", attributes, 0, &su->saAmfSURank) != SA_AIS_OK) {
		/* Empty, assign default value (highest number => lowest rank) */
		su->saAmfSURank = ~0U;
	}

	/* If 0 (zero), treat as lowest possible rank. Should be a positive integer */
	if (su->saAmfSURank == 0) {
		su->saAmfSURank = ~0U;
		TRACE("'%s' saAmfSURank auto-changed to lowest", su->name.value);
	}

	(void) immutil_getAttr("saAmfSUHostedByNode", attributes, 0, &su->saAmfSUHostedByNode);

	(void) immutil_getAttr("saAmfSUHostNodeOrNodeGroup", attributes, 0, &su->saAmfSUHostNodeOrNodeGroup);

	if ((sut = avd_sutype_get(&su->saAmfSUType)) == NULL) {
		LOG_ER("saAmfSUType '%s' does not exist", su->saAmfSUType.value);
		goto done;
	}

	if (immutil_getAttr("saAmfSUFailover", attributes, 0, &su->saAmfSUFailover) != SA_AIS_OK) {
		su->saAmfSUFailover = sut->saAmfSutDefSUFailover;
	}

	(void)immutil_getAttr("saAmfSUMaintenanceCampaign", attributes, 0, &su->saAmfSUMaintenanceCampaign);

	if (immutil_getAttr("saAmfSUAdminState", attributes, 0, &su->saAmfSUAdminState) != SA_AIS_OK)
		su->saAmfSUAdminState = SA_AMF_ADMIN_UNLOCKED;

	rc = 0;

done:
	if (rc != 0) {
		free(su);
		su = NULL;
	}

	TRACE_LEAVE();
	return su;
}

/**
 * Map SU to a node. A node is selected using a static load balancing scheme. Nodes are selected
 * from a node group in the (unordered) order they appear in the node list in the node group.
 * @param su
 * 
 * @return AVD_AVND*
 */
static AVD_AVND *map_su_to_node(AVD_SU *su)
{
	AVD_AMF_NG *ng = NULL;
	int i;          
	AVD_SU *su_temp = NULL;
	AVD_AVND *node = NULL;

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
	for (i = 0; i < ng->number_nodes; i++) {
		node = avd_node_get(&ng->saAmfNGNodeList[i]);
		osafassert(node);

		if (su->sg_of_su->sg_ncs_spec == SA_TRUE) {
			for (su_temp = node->list_of_ncs_su; su_temp != NULL; su_temp = su_temp->avnd_list_su_next) {
				if (su_temp->sg_of_su == su->sg_of_su)
					break;
			}
		}

		if (su->sg_of_su->sg_ncs_spec == SA_FALSE) {
			for (su_temp = node->list_of_su; su_temp != NULL; su_temp = su_temp->avnd_list_su_next) {
				if (su_temp->sg_of_su == su->sg_of_su)
					break;
			}
		}

		if (su_temp == NULL)
			goto done;
	}

	/* All nodes already have an SU mapped for the SG. Return a node in the node group. */
	node = avd_node_get(&ng->saAmfNGNodeList[0]);
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
	int new_su = 0;

	TRACE_ENTER2("%s", su->name.value);

	/* Check parent link to see if it has been added already */
	if (su->sg_of_su != NULL) {
		TRACE("already added");
		goto done;
	}

	/* Determine of the SU is added now, if so msg to amfnd needs to be sent */
	if (avd_su_get(&su->name) == NULL)
		new_su = 1;

	avsv_sanamet_init(&su->name, &dn, "safSg");

	/*
	** Refresh the SG reference, by now it must exist.
	** An SU can be created (on the standby) from the checkpointing logic.
	** In that case the SG reference could now be NULL.
	*/
	su->sg_of_su = avd_sg_get(&dn);
	osafassert(su->sg_of_su);

	avd_su_db_add(su);
	su->su_type = avd_sutype_get(&su->saAmfSUType);
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
			avd_cb->ext_comp_info.ext_comp_hlt_check = malloc(sizeof(AVD_AVND));
			if (NULL == avd_cb->ext_comp_info.ext_comp_hlt_check) {
				avd_sg_remove_su(su);
				LOG_ER("Memory Alloc Failure");
			}
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

	if (new_su) {
		if ((node->node_state == AVD_AVND_STATE_PRESENT) ||
		    (node->node_state == AVD_AVND_STATE_NO_CONFIG) ||
		    (node->node_state == AVD_AVND_STATE_NCS_INIT)) {

			if (avd_snd_su_msg(avd_cb, su) != NCSCC_RC_SUCCESS) {
				avd_node_remove_su(su);
				avd_sg_remove_su(su);

				LOG_ER("%s: avd_snd_su_msg failed %s", __FUNCTION__, su->name.value);
				goto done;
			}

			avd_su_oper_state_set(su, SA_AMF_OPERATIONAL_ENABLED);
		} else
			avd_su_oper_state_set(su, SA_AMF_OPERATIONAL_DISABLED);
	}

done:
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
		"saAmfSUType",
		"saAmfSURank",
		"saAmfSUHostedByNode",
		"saAmfSUHostNodeOrNodeGroup",
		"saAmfSUFailover",
		"saAmfSUMaintenanceCampaign",
		"saAmfSUAdminState",
		NULL
	};

	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
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

void avd_su_pres_state_set(AVD_SU *su, SaAmfPresenceStateT pres_state)
{
	SaAmfPresenceStateT old_state;

	if (su->saAmfSUPresenceState == pres_state)
		return;

	osafassert(pres_state <= SA_AMF_PRESENCE_TERMINATION_FAILED);
	TRACE_ENTER2("'%s' %s => %s",	su->name.value,
				 avd_pres_state_name[su->saAmfSUPresenceState],
				 avd_pres_state_name[pres_state]);

	old_state = su->saAmfSUPresenceState;
	su->saAmfSUPresenceState = pres_state;
	/* only log for certain changes, see notifications in spec */
	if (pres_state == SA_AMF_PRESENCE_UNINSTANTIATED ||
	    pres_state == SA_AMF_PRESENCE_INSTANTIATED ||
	    pres_state == SA_AMF_PRESENCE_RESTARTING) {

		saflog(LOG_NOTICE, amfSvcUsrName, "%s PresenceState %s => %s", su->name.value,
			   avd_pres_state_name[old_state], avd_pres_state_name[su->saAmfSUPresenceState]);

		/* alarm & notifications */
		avd_send_su_pres_state_chg_ntf(&su->name, old_state, su->saAmfSUPresenceState);
	}
	avd_saImmOiRtObjectUpdate(&su->name,
				 "saAmfSUPresenceState", SA_IMM_ATTR_SAUINT32T, &su->saAmfSUPresenceState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, su, AVSV_CKPT_SU_PRES_STATE);
}

void avd_su_oper_state_set(AVD_SU *su, SaAmfOperationalStateT oper_state)
{
	SaAmfOperationalStateT old_state = su->saAmfSUOperState;
	
	if (su->saAmfSUOperState == oper_state)
		return;
	osafassert(oper_state <= SA_AMF_OPERATIONAL_DISABLED);
	TRACE_ENTER2("'%s' %s => %s", su->name.value, avd_oper_state_name[su->saAmfSUOperState], 
			avd_oper_state_name[oper_state]);

	saflog(LOG_NOTICE, amfSvcUsrName, "%s OperState %s => %s", su->name.value,
		   avd_oper_state_name[su->saAmfSUOperState], avd_oper_state_name[oper_state]);

	su->saAmfSUOperState = oper_state;

	/* alarm & notifications */
	avd_send_oper_chg_ntf(&su->name, SA_AMF_NTFID_SU_OP_STATE, old_state, su->saAmfSUOperState);

	avd_saImmOiRtObjectUpdate(&su->name,
		"saAmfSUOperState", SA_IMM_ATTR_SAUINT32T, &su->saAmfSUOperState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, su, AVSV_CKPT_SU_OPER_STATE);
}

/**
 * Set readiness state in SU and calculate readiness state for all contained components
 * @param su
 * @param readiness_state
 */
void avd_su_readiness_state_set(AVD_SU *su, SaAmfReadinessStateT readiness_state)
{
	AVD_COMP *comp = NULL;
	if (su->saAmfSuReadinessState == readiness_state)
		return;
	osafassert(readiness_state <= SA_AMF_READINESS_STOPPING);
	TRACE_ENTER2("'%s' %s", su->name.value, avd_readiness_state_name[readiness_state]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s ReadinessState %s => %s", su->name.value,
		   avd_readiness_state_name[su->saAmfSuReadinessState], avd_readiness_state_name[readiness_state]);
	su->saAmfSuReadinessState = readiness_state;
	avd_saImmOiRtObjectUpdate(&su->name,
		 "saAmfSUReadinessState", SA_IMM_ATTR_SAUINT32T, &su->saAmfSuReadinessState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, su, AVSV_CKPT_SU_READINESS_STATE);

	/* Since Su readiness state has changed, we need to change it for all the component in this SU.*/
	comp = su->list_of_comp;
	while (comp != NULL) {
		SaAmfReadinessStateT saAmfCompReadinessState;
		if ((su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
				(comp->saAmfCompOperState == SA_AMF_OPERATIONAL_ENABLED)) {
			saAmfCompReadinessState = SA_AMF_READINESS_IN_SERVICE;
		} else if((su->saAmfSuReadinessState == SA_AMF_READINESS_STOPPING) &&
				(comp->saAmfCompOperState == SA_AMF_OPERATIONAL_ENABLED)) {
			saAmfCompReadinessState = SA_AMF_READINESS_STOPPING;
		} else
			saAmfCompReadinessState = SA_AMF_READINESS_OUT_OF_SERVICE;

		avd_comp_readiness_state_set(comp, saAmfCompReadinessState);
		comp = comp->su_comp_next;
	}
}

void avd_su_admin_state_set(AVD_SU *su, SaAmfAdminStateT admin_state)
{
	SaAmfAdminStateT old_state = su->saAmfSUAdminState;
	
	osafassert(admin_state <= SA_AMF_ADMIN_SHUTTING_DOWN);
	TRACE_ENTER2("'%s' %s => %s", su->name.value, avd_adm_state_name[old_state], avd_adm_state_name[admin_state]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s AdmState %s => %s", su->name.value,
		   avd_adm_state_name[su->saAmfSUAdminState], avd_adm_state_name[admin_state]);
	su->saAmfSUAdminState = admin_state;
	avd_saImmOiRtObjectUpdate(&su->name,
		"saAmfSUAdminState", SA_IMM_ATTR_SAUINT32T, &su->saAmfSUAdminState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, su, AVSV_CKPT_SU_ADMIN_STATE);
	avd_send_admin_state_chg_ntf(&su->name, SA_AMF_NTFID_SU_ADMIN_STATE, old_state, su->saAmfSUAdminState);
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
	SaBoolT   is_oper_successful = SA_TRUE;
	SaAmfAdminStateT adm_state = SA_AMF_ADMIN_LOCK;
	SaAmfReadinessStateT back_red_state;
	SaAmfAdminStateT back_admin_state;
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER2("%llu, '%s', %llu", invocation, su_name->value, op_id);

	if ( op_id > SA_AMF_ADMIN_SHUTDOWN && op_id != SA_AMF_ADMIN_REPAIRED) {
		rc = SA_AIS_ERR_NOT_SUPPORTED;
		LOG_WA("Unsupported admin op for SU: %llu", op_id);
		goto done;
	}

	if (cb->init_state != AVD_APP_STATE ) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		LOG_WA("AMF (state %u) is not available for admin ops", cb->init_state);
		goto done;
	}

	if (NULL == (su = avd_su_get(su_name))) {
		LOG_CR("SU '%s' not found", su_name->value);
		/* internal error? osafassert instead? */
		goto done;
	}

	if ((su->sg_of_su->sg_ncs_spec == SA_TRUE)
		&& (cb->node_id_avd == su->su_on_node->node_info.nodeId)) {
		rc = SA_AIS_ERR_NOT_SUPPORTED;
		LOG_WA("Admin operation on Active middleware SU is not allowed");
		goto done;
	}

	if (su->pend_cbk.invocation != 0) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		LOG_WA("Admin operation is already going");
		goto done;
	}

	if (su->sg_of_su->sg_fsm_state != AVD_SG_FSM_STABLE) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		LOG_WA("SG state is not stable"); /* whatever that means... */
		goto done;
	}
	/* if Tolerance timer is running for any SI's withing this SG, then return SA_AIS_ERR_TRY_AGAIN */
	if (sg_is_tolerance_timer_running_for_any_si(su->sg_of_su)) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		LOG_WA("Tolerance timer is running for some of the SI's in the SG '%s', so differing admin opr",su->sg_of_su->name.value);
		goto done;
	}

	if ( ((su->saAmfSUAdminState == SA_AMF_ADMIN_UNLOCKED) && (op_id == SA_AMF_ADMIN_UNLOCK)) ||
	     ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED)   && (op_id == SA_AMF_ADMIN_LOCK))   ||
	     ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
	      (op_id == SA_AMF_ADMIN_LOCK_INSTANTIATION))                                     ||
	     ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED)   && (op_id == SA_AMF_ADMIN_UNLOCK_INSTANTIATION))) {

		rc = SA_AIS_ERR_NO_OP;
		LOG_WA("Admin operation (%llu) has no effect on current state (%u)", op_id, su->saAmfSUAdminState);
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

		rc = SA_AIS_ERR_BAD_OPERATION;
		LOG_WA("State transition invalid, state %u, op %llu", su->saAmfSUAdminState, op_id);
		goto done;
	}

	m_AVD_GET_SU_NODE_PTR(cb, su, node);

	/* Validation has passed and admin operation should be done. Proceed with it... */
	switch (op_id) {
	case SA_AMF_ADMIN_UNLOCK:
		avd_su_admin_state_set(su, SA_AMF_ADMIN_UNLOCKED);
		if (((m_AVD_APP_SU_IS_INSVC(su, node)) || (su->sg_of_su->sg_ncs_spec == true)) &&
			((su->saAmfSUPreInstantiable) ?
			 (su->saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATED):true)) {
			/* Pres state check is to prevent assignment to SU in case SU is instantiating in
			 * locked state and somebody issues UNLOCK on SU. Since comp are in instantiating state,
			 * so AMFND will not assign the role to components. Anyway when SU gets instantiated, then
			 * assignment will be given to components/SU.
			 */
			/* Reason for adding "su->sg_of_su->sg_ncs_spec == true" is for Middleware component
			 * node oper state and SU oper state are marked enabled after they gets assignments.
			 * So, we cann't check compatibility with m_AVD_APP_SU_IS_INSVC for them.
			 */
			avd_su_readiness_state_set(su, SA_AMF_READINESS_IN_SERVICE);
			if (su->sg_of_su->su_insvc(cb, su) != NCSCC_RC_SUCCESS)
				is_oper_successful = SA_FALSE;

			avd_sg_app_su_inst_func(cb, su->sg_of_su);
		} else
			LOG_IN("SU is not in service");

		if ( is_oper_successful == SA_TRUE ) {
			if ( su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SG_REALIGN ) {
				/* Store the callback parameters to send operation result later on. */
				su->pend_cbk.admin_oper = op_id;
				su->pend_cbk.invocation = invocation;
				goto done;
			} else {
				avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
				goto done;
			}
		} else {
			avd_su_readiness_state_set(su, SA_AMF_READINESS_OUT_OF_SERVICE);
			avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			rc = SA_AIS_ERR_FAILED_OPERATION;
			LOG_WA("SG redundancy model specific handler failed");
			goto done;
		}

		break;

	case SA_AMF_ADMIN_SHUTDOWN:
		adm_state = SA_AMF_ADMIN_SHUTTING_DOWN;
		/* fall-through */

	case SA_AMF_ADMIN_LOCK:
		if (su->list_of_susi == NULL) {
			avd_su_readiness_state_set(su, SA_AMF_READINESS_OUT_OF_SERVICE);
			avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			avd_sg_app_su_inst_func(cb, su->sg_of_su);
			avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
			goto done;
		}

		back_red_state = su->saAmfSuReadinessState;
		back_admin_state = su->saAmfSUAdminState;
		avd_su_readiness_state_set(su, SA_AMF_READINESS_OUT_OF_SERVICE);
		avd_su_admin_state_set(su, adm_state);

		if (su->sg_of_su->su_admin_down(cb, su, node) != NCSCC_RC_SUCCESS)
			is_oper_successful = SA_FALSE;

		avd_sg_app_su_inst_func(avd_cb, su->sg_of_su);

		if ( is_oper_successful == SA_TRUE ) {
			if ( (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SG_REALIGN) ||
			     (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SU_OPER) ) {
				/* Store the callback parameters to send operation result later on. */
				su->pend_cbk.admin_oper = op_id;
				su->pend_cbk.invocation = invocation;
			} else {
				avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
			}
		} else {
			avd_su_readiness_state_set(su, back_red_state);
			avd_su_admin_state_set(su, back_admin_state);
			rc = SA_AIS_ERR_FAILED_OPERATION;
			LOG_WA("SG redundancy model specific handler failed");
			goto done;
		}

		break;

	case SA_AMF_ADMIN_LOCK_INSTANTIATION:

		/* For non-preinstantiable SU lock-inst is same as lock */
		if ( su->saAmfSUPreInstantiable == false ) {
			avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED_INSTANTIATION);
			avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
			goto done;
		}

		if ( su->list_of_susi != NULL ) {
			rc = SA_AIS_ERR_TRY_AGAIN;
			LOG_WA("SIs still assigned to this SU");
			goto done;
		}

		if ((su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED) ||
				(su->saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATION_FAILED) || 
				(su->saAmfSUPresenceState == SA_AMF_PRESENCE_TERMINATION_FAILED)) {
			/* No need to terminate the SUs in Unins/Inst Failed/Term Failed state */
			avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED_INSTANTIATION);
			avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
			m_AVD_SET_SU_TERM(cb, su, true);
			LOG_NO("'%s' presence state is '%u'", su_name->value, su->saAmfSUPresenceState);
			goto done;
		}

		if ( ( node->node_state == AVD_AVND_STATE_PRESENT )   ||
		     ( node->node_state == AVD_AVND_STATE_NO_CONFIG ) ||
		     ( node->node_state == AVD_AVND_STATE_NCS_INIT ) ) {
			/* When the SU will terminate then prescence state change message will come
			   and so store the callback parameters to send response later on. */
			if (avd_snd_presence_msg(cb, su, true) == NCSCC_RC_SUCCESS) {
				m_AVD_SET_SU_TERM(cb, su, true);
				avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED_INSTANTIATION);

				su->pend_cbk.admin_oper = op_id;
				su->pend_cbk.invocation = invocation;

				goto done;
			}
			rc = SA_AIS_ERR_TRY_AGAIN;
			LOG_ER("Internal error, could not send message to avnd");
			goto done;
		} else {
			avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED_INSTANTIATION);
			avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
			m_AVD_SET_SU_TERM(cb, su, true);
		}

		break;

	case SA_AMF_ADMIN_UNLOCK_INSTANTIATION:

		if (NULL == su->list_of_comp) {
			LOG_WA("There is no component configured for SU '%s'.", su->name.value);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}

		/* For non-preinstantiable SU unlock-inst will not lead to its inst until unlock. */
		if ( su->saAmfSUPreInstantiable == false ) {
			/* Adjusting saAmfSGMaxActiveSIsperSU and saAmfSGMaxStandbySIsperSU
			   is required when SG and SUs/Comps are created in different CCBs. */
			avd_sg_adjust_config(su->sg_of_su);
			avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
			goto done;
		}

		if (su->saAmfSUPresenceState != SA_AMF_PRESENCE_UNINSTANTIATED) {
			LOG_WA("Can't instantiate '%s', whose presense state is '%u'", su_name->value, 
					su->saAmfSUPresenceState);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		} 

		if ((node->node_state == AVD_AVND_STATE_PRESENT) &&
				((node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) && 
				 (su->sg_of_su->saAmfSGAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION)) && 
				(su->sg_of_su->saAmfSGNumPrefInserviceSUs > sg_instantiated_su_count(su->sg_of_su))) {
			/* When the SU will instantiate then prescence state change message will come
			   and so store the callback parameters to send response later on. */
			if (avd_snd_presence_msg(cb, su, false) == NCSCC_RC_SUCCESS) {
				m_AVD_SET_SU_TERM(cb, su, false);
				avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);

				su->pend_cbk.admin_oper = op_id;
				su->pend_cbk.invocation = invocation;

				goto done;
			}
			rc = SA_AIS_ERR_TRY_AGAIN;
			LOG_ER("Internal error, could not send message to avnd");
		} else {
			avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_OK);
			m_AVD_SET_SU_TERM(cb, su, false);
		}

		break;
	case SA_AMF_ADMIN_REPAIRED:
		if (su->saAmfSUOperState == SA_AMF_OPERATIONAL_ENABLED) {
			LOG_NO("Admin repair request for '%s', op state already enabled", su_name->value);
			rc = SA_AIS_ERR_NO_OP;
			goto done;
		}

		/* forward the admin op req to the node director */
		if (avd_admin_op_msg_snd(su_name, AVSV_SA_AMF_SU, op_id,
			su->su_on_node) == NCSCC_RC_SUCCESS) {
			su->pend_cbk.admin_oper = op_id;
			su->pend_cbk.invocation = invocation;
			rc = SA_AIS_OK;
		}
		else {
			LOG_WA("Admin op request send failed '%s'", su_name->value);
			rc = SA_AIS_ERR_TIMEOUT;
		}
		break;
	default:
		LOG_ER("Unsupported admin op");
		rc = SA_AIS_ERR_INVALID_PARAM;
		break;
	}

done:
	if (rc != SA_AIS_OK)
		avd_saImmOiAdminOperationResult(immoi_handle, invocation, rc);

	TRACE_LEAVE2("%u", rc);
}

static SaAisErrorT su_rt_attr_cb(SaImmOiHandleT immOiHandle,
	const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	AVD_SU *su = avd_su_get(objectName);
	SaImmAttrNameT attributeName;
	int i = 0;

	TRACE("%s", objectName->value);

	while ((attributeName = attributeNames[i++]) != NULL) {
		if (!strcmp("saAmfSUAssignedSIs", attributeName)) {
#if 0
			/*  TODO */
			SaUint32T saAmfSUAssignedSIs = su->saAmfSUNumCurrActiveSIs + su->saAmfSUNumCurrStandbySIs;
			avd_saImmOiRtObjectUpdate_sync(immOiHandle, objectName,
				attributeName, SA_IMM_ATTR_SAUINT32T, &saAmfSUAssignedSIs);
#endif
		} else if (!strcmp("saAmfSUNumCurrActiveSIs", attributeName)) {
			avd_saImmOiRtObjectUpdate_sync(objectName, attributeName,
				SA_IMM_ATTR_SAUINT32T, &su->saAmfSUNumCurrActiveSIs);
		} else if (!strcmp("saAmfSUNumCurrStandbySIs", attributeName)) {
			avd_saImmOiRtObjectUpdate_sync(objectName, attributeName,
				SA_IMM_ATTR_SAUINT32T, &su->saAmfSUNumCurrStandbySIs);
		} else if (!strcmp("saAmfSURestartCount", attributeName)) {
			avd_saImmOiRtObjectUpdate_sync(objectName, attributeName,
				SA_IMM_ATTR_SAUINT32T, &su->saAmfSURestartCount);
		} else
			osafassert(0);
	}

	return SA_AIS_OK;
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

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {

		/* Attribute value removed */
		if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || (attr_mod->modAttr.attrValues == NULL))
			continue;

		if (!strcmp(attr_mod->modAttr.attrName, "saAmfSUFailover")) {
			bool su_failover = *((SaTimeT *)attr_mod->modAttr.attrValues[0]);
			if (su_failover > SA_TRUE) {
				LOG_ER("Invalid saAmfSUFailover %u", su_failover);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else if (!strcmp(attr_mod->modAttr.attrName, "saAmfSUMaintenanceCampaign")) {
			AVD_SU *su = avd_su_get(&opdata->objectName);

			if (su->saAmfSUMaintenanceCampaign.length > 0) {
				LOG_ER("saAmfSUMaintenanceCampaign already set for %s", su->name.value);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else if (!strcmp(attr_mod->modAttr.attrName, "saAmfSUType")) {
			AVD_SU *su;
			SaNameT sutype_name = *(SaNameT*) attr_mod->modAttr.attrValues[0];
			su = avd_su_get(&opdata->objectName);
			if(SA_AMF_ADMIN_LOCKED_INSTANTIATION != su->saAmfSUAdminState) {
				LOG_ER("SU is not in locked-inst, present state '%d'", su->saAmfSUAdminState);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
			if (avd_sutype_get(&sutype_name) == NULL) {
				LOG_ER("SU Type not found '%s'", sutype_name.value);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}

		} else {
			LOG_ER("Modification of attribute '%s' not supported", attr_mod->modAttr.attrName);
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

	su = avd_su_get(&opdata->objectName);
	osafassert(su != NULL);

	if (is_app_su && (su->saAmfSUAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION)) {
		LOG_ER("Rejecting deletion of '%s'", su->name.value);
		LOG_ER("Admin state is not locked instantiation required for deletion");
		goto done;
	}

	if (!is_app_su && (su->su_on_node->node_state != AVD_AVND_STATE_ABSENT)) {
		LOG_ER("Rejecting deletion of '%s'", su->name.value);
		LOG_ER("MW SU can only be deleted when its hosting node is down");
		goto done;
	}

	rc = SA_AIS_OK;
	opdata->userData = su;

done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Validation performed when an SU is dynamically created with a CCB.
 * @param dn
 * @param attributes
 * @param opdata
 * 
 * @return int
 */
static int is_ccb_create_config_valid(const SaNameT *dn, const SaImmAttrValuesT_2 **attributes,
	const CcbUtilOperationData_t *opdata)
{
	SaAmfAdminStateT admstate;
	int is_app_su = 1;

	if (strstr((char *)dn->value, "safApp=OpenSAF") != NULL)
		is_app_su = 0;

	if (is_app_su && (immutil_getAttr("saAmfSUAdminState", attributes, 0, &admstate) != SA_AIS_OK)) {
		LOG_ER("saAmfSUAdminState not configured for '%s'", dn->value);
		return 0;
	}

	if (immutil_getAttr("saAmfSUAdminState", attributes, 0, &admstate) == SA_AIS_OK) {
		if (admstate != SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			LOG_ER("Invalid saAmfSUAdminState %u for '%s'", admstate, dn->value);
			LOG_NO("saAmfSUAdminState must be LOCKED_INSTANTIATION(%u) for dynamically created SUs",
				SA_AMF_ADMIN_LOCKED_INSTANTIATION);
			return 0;
		}
	}

	return 1;
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

	su = avd_su_get(&opdata->objectName);

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != NULL) {
		/* Attribute value removed */
		if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || (attr_mod->modAttr.attrValues == NULL))
			value_is_deleted = true;
		else
			value_is_deleted = false;

		if (!strcmp(attr_mod->modAttr.attrName, "saAmfSUFailover")) {
			if (value_is_deleted)
				su->saAmfSUFailover = su->su_type->saAmfSutDefSUFailover;
			else
				su->saAmfSUFailover = *((SaUint32T *)attr_mod->modAttr.attrValues[0]);
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
			sut = avd_sutype_get(&sutype_name);
			avd_sutype_remove_su(su);
			su->saAmfSUType = sutype_name;
			su->su_type = sut;
			avd_sutype_add_su(su);
			su->saAmfSUFailover = sut->saAmfSutDefSUFailover;
			su->su_is_external = sut->saAmfSutIsExternal;
		} else
			osafassert(0);
	}

	TRACE_LEAVE();
}

/**
 * Handle delete operations on SaAmfSU objects.
 * 
 * @param su
 */
static void su_ccb_apply_delete_hdlr(struct CcbUtilOperationData *opdata)
{
	AVD_SU *su = opdata->userData;
	AVD_AVND *su_node_ptr;
	AVSV_PARAM_INFO param;
	AVD_SG *sg = su->sg_of_su;

	TRACE_ENTER2("'%s'", su->name.value);

	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) {
		avd_su_delete(su);
		goto done;
	}

	m_AVD_GET_SU_NODE_PTR(avd_cb, su, su_node_ptr);

	if ((su_node_ptr->node_state == AVD_AVND_STATE_PRESENT) ||
	    (su_node_ptr->node_state == AVD_AVND_STATE_NO_CONFIG) ||
	    (su_node_ptr->node_state == AVD_AVND_STATE_NCS_INIT)) {
		memset(((uint8_t *)&param), '\0', sizeof(AVSV_PARAM_INFO));
		param.class_id = AVSV_SA_AMF_SU;
		param.act = AVSV_OBJ_OPR_DEL;
		param.name = su->name;
		avd_snd_op_req_msg(avd_cb, su_node_ptr, &param);
	}

	avd_su_delete(su);
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

void avd_su_inc_curr_act_si(AVD_SU *su)
{
	su->saAmfSUNumCurrActiveSIs++;
	TRACE("%s saAmfSUNumCurrActiveSIs=%u", su->name.value, su->saAmfSUNumCurrActiveSIs);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, su, AVSV_CKPT_SU_SI_CURR_ACTIVE);
}

void avd_su_dec_curr_act_si(AVD_SU *su)
{
	osafassert(su->saAmfSUNumCurrActiveSIs > 0);
	su->saAmfSUNumCurrActiveSIs--;
	TRACE("%s saAmfSUNumCurrActiveSIs=%u", su->name.value, su->saAmfSUNumCurrActiveSIs);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, su, AVSV_CKPT_SU_SI_CURR_ACTIVE);
}

void avd_su_inc_curr_stdby_si(AVD_SU *su)
{
	su->saAmfSUNumCurrStandbySIs++;
	TRACE("%s saAmfSUNumCurrStandbySIs=%u", su->name.value, su->saAmfSUNumCurrStandbySIs);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, su, AVSV_CKPT_SU_SI_CURR_STBY);
}

void avd_su_dec_curr_stdby_si(AVD_SU *su)
{
	osafassert(su->saAmfSUNumCurrStandbySIs > 0);
	su->saAmfSUNumCurrStandbySIs--;
	TRACE("%s saAmfSUNumCurrStandbySIs=%u", su->name.value, su->saAmfSUNumCurrStandbySIs);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, su, AVSV_CKPT_SU_SI_CURR_STBY);
}

void avd_su_constructor(void)
{
	NCS_PATRICIA_PARAMS patricia_params;

	patricia_params.key_size = sizeof(SaNameT);
	osafassert(ncs_patricia_tree_init(&su_db, &patricia_params) == NCSCC_RC_SUCCESS);

	avd_class_impl_set("SaAmfSU", su_rt_attr_cb, su_admin_op_cb, su_ccb_completed_cb, su_ccb_apply_cb);
}

