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

#include "imm/saf/saImmOm.h"
#include "osaf/immutil/immutil.h"
#include "base/logtrace.h"
#include "osaf/saflog/saflog.h"

#include "amf/amfd/amfd.h"
#include "amf/amfd/cluster.h"
#include "amf/amfd/imm.h"
#include <algorithm>

AmfDb<std::string, AVD_AVND> *node_name_db = 0;	/* SaNameT index */
AmfDb<uint32_t, AVD_AVND> *node_id_db = 0;	/* SaClmNodeIdT index */

//Remember MDS install version of AMFNDs. It can be used to send msg to AMFNDs based on their versions.
std::map<SaClmNodeIdT, MDS_SVC_PVT_SUB_PART_VER> nds_mds_ver_db;

bool operator<(const AVD_AVND &lhs, const AVD_AVND &rhs) {
	if (lhs.name.compare(rhs.name) < 0) {
		return true;
	} else {
		return false;
	}
}

bool NodeNameCompare::operator() (const AVD_AVND* lhs, const AVD_AVND* rhs) {
  return *lhs < *rhs;
}

uint32_t avd_node_add_nodeid(AVD_AVND *node)
{
	unsigned int rc;

	if ((node_id_db->find(node->node_info.nodeId) == nullptr) &&
			(node->node_info.nodeId != 0)) {
		TRACE("added node %d", node->node_info.nodeId);
		rc = node_id_db->insert(node->node_info.nodeId, node);
		osafassert(rc == NCSCC_RC_SUCCESS);
	}

	return NCSCC_RC_SUCCESS;
}

void avd_node_delete_nodeid(AVD_AVND *node)
{
	node_id_db->erase(node->node_info.nodeId);
}

void avd_node_db_add(AVD_AVND *node)
{
	unsigned int rc;
	TRACE_ENTER();

	if (node_name_db->find(node->name) == nullptr) {
		TRACE("add %s", node->name.c_str());
		rc = node_name_db->insert(node->name, node);
		osafassert(rc == NCSCC_RC_SUCCESS);
	}
	TRACE_LEAVE();
}

//
bool AVD_AVND::is_node_lock() {
  AVD_SU_SI_REL *curr_susi;
  for (const auto& su : list_of_su) {
    if ((su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SU_OPER) ||
        (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SG_REALIGN)) {
      for (curr_susi = su->list_of_susi;
           (curr_susi) && ((SA_AMF_HA_QUIESCING != curr_susi->state) ||
                           ((AVD_SU_SI_STATE_UNASGN == curr_susi->fsm)));
           curr_susi = curr_susi->su_next);
      if (curr_susi)
        return false;
    }
  }
  return true;
}

//
void AVD_AVND::initialize() {
  node_name = {};
  node_info = {};
  node_info.member = SA_FALSE;
  adest = {};
  saAmfNodeClmNode = {};
  saAmfNodeCapacity = {};
  saAmfNodeSuFailOverProb = {};
  saAmfNodeSuFailoverMax = {};
  saAmfNodeAutoRepair = {};
  saAmfNodeFailfastOnTerminationFailure = {};
  saAmfNodeFailfastOnInstantiationFailure = {};
  saAmfNodeAdminState = SA_AMF_ADMIN_UNLOCKED;
  saAmfNodeOperState = SA_AMF_OPERATIONAL_DISABLED;
  admin_node_pend_cbk = {};
  su_cnt_admin_oper = {};
  node_state = AVD_AVND_STATE_ABSENT;
  list_of_ncs_su = {};
  list_of_su = {};
  pg_csi_list = {};
  pg_csi_list.order = NCS_DBLIST_ANY_ORDER;
  pg_csi_list.cmp_cookie = avsv_dblist_uns32_cmp;
  rcv_msg_id = {};
  snd_msg_id = {};
  cluster_list_node_next = {};
  cluster = {};
  clm_pend_inv = {};
  clm_change_start_preceded = {};
  recvr_fail_sw = {};
  admin_ng = {};
}

//
AVD_AVND::AVD_AVND() {
  initialize();
}

//
AVD_AVND::AVD_AVND(const std::string& dn) :
	name(dn)
{
	std::string::size_type eq_pos;
	std::string::size_type comma_pos;

	initialize();
	// DN looks like safAmfNode=SC-1,safAmfCluster=myAmfCluster
	eq_pos = dn.find('=');
	comma_pos = dn.find(',');
	node_name = dn.substr(eq_pos + 1, comma_pos - eq_pos - 1);
}

//
AVD_AVND::~AVD_AVND() {
}

//
AVD_AVND *avd_node_new(const std::string& dn)
{
	AVD_AVND *node;
	node = new AVD_AVND(dn);
	node->node_up_msg_count = 0;
	node->reboot = false;
	return node;
}

void avd_node_delete(AVD_AVND *node)
{
	TRACE_ENTER();
	osafassert(node->pg_csi_list.n_nodes == 0);
	if (node->node_info.nodeId)
		avd_node_delete_nodeid(node);
	/* Check if the SUs and related objects are still left. This can
	   happen on Standby Amfd when it has just read the configuration
	   and before it becomes applier, Act Amfd deletes SUs. Those SUs
	   will be left out at Standby Amfd. Though this could be rare.*/
	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) {
		if (node->list_of_su.empty() != true) {
			std::set<std::string> su_list;
			std::set<std::string> comp_list;
			for (const auto& su : node->list_of_su)
				su_list.insert(su->name);
			for (std::set<std::string>::const_iterator iter = su_list.begin();
					iter != su_list.end(); ++iter) {
				AVD_SU *su = su_db->find(*iter);
				TRACE("Standby Amfd, su '%s' not deleted", su->name.c_str());
				for (const auto& comp : su->list_of_comp)
					comp_list.insert(Amf::to_string(&comp->comp_info.name));
				for (std::set<std::string>::const_iterator iter1 = comp_list.begin();
						iter1 != comp_list.end(); ++iter1) {
					AVD_COMP *comp = comp_db->find(*iter1);
					TRACE("Standby Amfd, comp '%s' not deleted", osaf_extended_name_borrow(&comp->comp_info.name));

					std::map<std::string, AVD_COMPCS_TYPE*>::iterator it =
						compcstype_db->begin();
					while (it != compcstype_db->end()) {
						AVD_COMPCS_TYPE *compcstype = it->second;
						if (compcstype->comp == comp) {
							TRACE("Standby Amfd, compcstype '%s' not deleted",
									compcstype->name.c_str());
							it = compcstype_db->erase(it);
							delete compcstype;
						}
						else
							it++;
					}


					/* Delete the Comp. */
					struct CcbUtilOperationData opdata;
					osaf_extended_name_alloc(osaf_extended_name_borrow(&comp->comp_info.name), &opdata.objectName);
					comp_ccb_apply_delete_hdlr(&opdata);
				}
				comp_list.clear();
				/* Delete the SU. */
				struct CcbUtilOperationData opdata;
				opdata.userData = su;
				su_ccb_apply_delete_hdlr(&opdata);
			}
			su_list.clear();
		}
	}
	m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, node, AVSV_CKPT_AVD_NODE_CONFIG);
	node_name_db->erase(node->name);
	delete node;
}

static void node_add_to_model(AVD_AVND *node)
{
	TRACE_ENTER2("%s", osaf_extended_name_borrow(&node->node_info.nodeName));

	/* Check parent link to see if it has been added already */
	if (node->cluster != nullptr) {
		TRACE("already added");
		goto done;
	}

	node->cluster = avd_cluster;
	node->admin_ng = nullptr;

	avd_node_db_add(node);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(avd_cb, node, AVSV_CKPT_AVD_NODE_CONFIG);

done:
	TRACE_LEAVE();
}

AVD_AVND *avd_node_get(const std::string& dn)
{
	TRACE_ENTER2("%s", dn.c_str());
	return node_name_db->find(dn);
	TRACE_LEAVE();
}

AVD_AVND *avd_node_find_nodeid(SaClmNodeIdT node_id)
{
	return node_id_db->find(node_id);
}

/**
 * Validate configuration attributes for an AMF node object
 * @param node
 * @param opdata
 * 
 * @return int
 */
static int is_config_valid(const std::string& dn, const SaImmAttrValuesT_2 **attributes, CcbUtilOperationData_t * opdata)
{
	SaBoolT abool;
	SaAmfAdminStateT admstate;
	std::string::size_type pos;
	SaNameT saAmfNodeClmNode;

	if ((pos = dn.find(',')) == std::string::npos) {
		report_ccb_validation_error(opdata, "No parent to '%s' ", dn.c_str());
		return 0;
	}

	if (dn.compare(pos + 1, 14, "safAmfCluster=") != 0) {
		report_ccb_validation_error(opdata, "Wrong parent '%s' to '%s' ", dn.substr(pos + 1).c_str(), dn.c_str());
		return 0;
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNodeAutoRepair"), attributes, 0, &abool) == SA_AIS_OK) && (abool > SA_TRUE)) {
		report_ccb_validation_error(opdata, "Invalid saAmfNodeAutoRepair %u for '%s'", abool, dn.c_str());
		return 0;
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNodeFailfastOnTerminationFailure"), attributes, 0, &abool) == SA_AIS_OK) &&
	    (abool > SA_TRUE)) {
		report_ccb_validation_error(opdata, "Invalid saAmfNodeFailfastOnTerminationFailure %u for '%s'",
				abool, dn.c_str());
		return 0;
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNodeFailfastOnInstantiationFailure"), attributes, 0, &abool) == SA_AIS_OK) &&
	    (abool > SA_TRUE)) {
		report_ccb_validation_error(opdata, "Invalid saAmfNodeFailfastOnInstantiationFailure %u for '%s'",
				abool, dn.c_str());
		return 0;
	}

	if ((immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNodeAdminState"), attributes, 0, &admstate) == SA_AIS_OK) &&
	    !avd_admin_state_is_valid(admstate, opdata)) {
		report_ccb_validation_error(opdata, "Invalid saAmfNodeAdminState %u for '%s'", admstate, dn.c_str());
		return 0;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNodeClmNode"), attributes, 0, &saAmfNodeClmNode) != SA_AIS_OK) { 
		report_ccb_validation_error(opdata, "saAmfNodeClmNode not configured for '%s'", dn.c_str());
		return 0;
	}

	return 1;
}

/**
 * Create a new Node object
 * @param dn
 * @param attributes
 * 
 * @return AVD_AVND*
 */
static AVD_AVND *node_create(const std::string& dn, const SaImmAttrValuesT_2 **attributes)
{
	int rc = -1;
	AVD_AVND *node;

	TRACE_ENTER2("'%s'", dn.c_str());

	/*
	 ** If called at new active at failover, the object is found in the DB
	 ** but needs to get configuration attributes initialized.
	 */
	if (nullptr == (node = avd_node_get(dn))) {
		if ((node = avd_node_new(dn)) == nullptr)
			goto done;
	} else
		TRACE("already created, refreshing config...");

	SaNameT safAmfNodeClmNode;
	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNodeClmNode"), attributes, 0, &safAmfNodeClmNode) != SA_AIS_OK) { 
		LOG_ER("saAmfNodeClmNode not configured for '%s'", osaf_extended_name_borrow(&safAmfNodeClmNode));
		goto done;
	}

	node->saAmfNodeClmNode = Amf::to_string(&safAmfNodeClmNode);

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNodeSuFailOverProb"), attributes, 0, &node->saAmfNodeSuFailOverProb) != SA_AIS_OK) {
		LOG_ER("Get saAmfNodeSuFailOverProb FAILED for '%s'", dn.c_str());
		goto done;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNodeSuFailoverMax"), attributes, 0, &node->saAmfNodeSuFailoverMax) != SA_AIS_OK) {
		LOG_ER("Get saAmfNodeSuFailoverMax FAILED for '%s'", dn.c_str());
		goto done;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNodeAutoRepair"), attributes, 0, &node->saAmfNodeAutoRepair) != SA_AIS_OK) {
		node->saAmfNodeAutoRepair = SA_TRUE;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNodeFailfastOnTerminationFailure"), attributes, 0,
			    &node->saAmfNodeFailfastOnTerminationFailure) != SA_AIS_OK) {
		node->saAmfNodeFailfastOnTerminationFailure = SA_FALSE;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNodeFailfastOnInstantiationFailure"), attributes, 0,
			    &node->saAmfNodeFailfastOnInstantiationFailure) != SA_AIS_OK) {
		node->saAmfNodeFailfastOnInstantiationFailure = SA_FALSE;
	}

	if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfNodeAdminState"), attributes, 0, &node->saAmfNodeAdminState) != SA_AIS_OK) {
		/* Empty, assign default value */
		node->saAmfNodeAdminState = SA_AMF_ADMIN_UNLOCKED;
	}

	rc = 0;
	osafassert(node->name.empty() == false);

done:
	if (rc != 0) {
		avd_node_delete(node);
		node = nullptr;
	}

	TRACE_LEAVE();
	return node;
}

/**
 * Get configuration for all AMF node objects from IMM and
 * create AVD internal objects.
 * 
 * @return int
 */
SaAisErrorT avd_node_config_get(void)
{
	SaAisErrorT error, rc = SA_AIS_ERR_FAILED_OPERATION;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT dn;
	const SaImmAttrValuesT_2 **attributes;
	const char *className = "SaAmfNode";
	AVD_AVND *node;

	TRACE_ENTER();

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
		if (!is_config_valid(Amf::to_string(&dn), attributes, nullptr)) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}

		if ((node = node_create(Amf::to_string(&dn), attributes)) == nullptr) {
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto done2;
		}

		node_add_to_model(node);
	}

	rc = SA_AIS_OK;

 done2:
	(void)immutil_saImmOmSearchFinalize(searchHandle);
 done1:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

static const char *node_state_name[] = {
	"ABSENT",
	"NO_CONFIG",
	"PRESENT",
	"GO_DOWN",
	"SHUTTING_DOWN",
	"NCS_INIT"
};

void avd_node_state_set(AVD_AVND *node, AVD_AVND_STATE node_state)
{
	osafassert(node_state <= AVD_AVND_STATE_NCS_INIT);
	TRACE_ENTER2("'%s' %s => %s",	node->name.c_str(), node_state_name[node->node_state],
		node_state_name[node_state]);
	if (node->node_state != node_state) {
		node->node_state = node_state;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, node, AVSV_CKPT_AVND_NODE_STATE);
	}
	TRACE_LEAVE();
}

/**
 * Set AMF operational state of a node. Log & update IMM, checkpoint with standby
 * @param node
 * @param oper_state
 */
void avd_node_oper_state_set(AVD_AVND *node, SaAmfOperationalStateT oper_state)
{
	if (node->saAmfNodeOperState == oper_state) {
		/* In the case of node failover, oper_state is disabled in the avnd_down
		 * event. Since we dont update oper_state in avnd_down because the role is
		 * not set to Active(there is no implementer), so updating now.
		 */
		avd_saImmOiRtObjectUpdate(node->name, "saAmfNodeOperState",
			SA_IMM_ATTR_SAUINT32T, &node->saAmfNodeOperState);

		/* Send notification for node oper state down. It is set to 
		   DISABLE in avd_mds_avnd_down_evh and again
		   avd_node_oper_state_set is called from avd_node_mark_absent.
		   Since the oper state is the same when called from
		   avd_node_mark_absent, we need to send notification. */
		if ((node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED) &&
				(node->node_state == AVD_AVND_STATE_ABSENT))
			avd_send_oper_chg_ntf(node->name,
					SA_AMF_NTFID_NODE_OP_STATE,
					SA_AMF_OPERATIONAL_ENABLED,
					node->saAmfNodeOperState);

		return;
	}
	
	SaAmfOperationalStateT old_state = node->saAmfNodeOperState;

	osafassert(oper_state <= SA_AMF_OPERATIONAL_DISABLED);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s OperState %s => %s", node->name.c_str(),
		   avd_oper_state_name[node->saAmfNodeOperState], avd_oper_state_name[oper_state]);
	node->saAmfNodeOperState = oper_state;
        avd_saImmOiRtObjectUpdate(node->name, "saAmfNodeOperState",
			SA_IMM_ATTR_SAUINT32T, &node->saAmfNodeOperState);
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, node, AVSV_CKPT_AVND_OPER_STATE);
	
	/* notification */
	avd_send_oper_chg_ntf(node->name,
				SA_AMF_NTFID_NODE_OP_STATE,
				old_state,
				node->saAmfNodeOperState);
}

/*****************************************************************************
 * Function: node_ccb_completed_delete_hdlr
 *
 * Purpose: This routine handles delete operations on SaAmfCluster objects.
 *
 * Input  : Ccb Util Oper Data.
 *
 * Returns: None.
 *
 ****************************************************************************/
static SaAisErrorT node_ccb_completed_delete_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_OK;
	AVD_AVND *node = avd_node_get(Amf::to_string(&opdata->objectName));
	bool su_exist = false;
	CcbUtilOperationData_t *t_opData;

	TRACE_ENTER2("'%s'", osaf_extended_name_borrow(&opdata->objectName));

	if (node->node_info.member) {
		report_ccb_validation_error(opdata, "Node '%s' is still cluster member", osaf_extended_name_borrow(&opdata->objectName));
		return SA_AIS_ERR_BAD_OPERATION;
	}

 	for (const auto& ncs_su : node->list_of_ncs_su) {
		if (ncs_su->sg_of_su->sg_redundancy_model ==
		    SA_AMF_2N_REDUNDANCY_MODEL) {
			if (ncs_su->sg_of_su->list_of_su.size() <=
			    avd_cb->minimum_cluster_size) {
				report_ccb_validation_error(
					opdata, "Configured minimum cluster "
					"size is %u (see "
					"OSAF_AMF_MIN_CLUSTER_SIZE in "
					"amfd.conf)",
					avd_cb->minimum_cluster_size);
				return SA_AIS_ERR_BAD_OPERATION;
			}
			break;
		}
	}

	/* Check to see that the node is in admin locked state before delete */
	if (node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
		report_ccb_validation_error(opdata, "Node '%s' is not locked instantiation", osaf_extended_name_borrow(&opdata->objectName));
		return SA_AIS_ERR_BAD_OPERATION;
	}

	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE) {
		goto done_ok;
	}

	/* Check to see that no SUs exists on this node */
	if (node->list_of_su.empty() != true) {
		/* check whether there exists a delete operation for 
		 * each of the SU in the node list in the current CCB 
		 */                      
		for (const auto& su : node->list_of_su) {
			const SaNameTWrapper su_name(su->name);
			t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, su_name);
			if ((t_opData == nullptr) || (t_opData->operationType != CCBUTIL_DELETE)) {
				su_exist = true;   
				break;                  
			}                       
		}                       
		if (su_exist == true) {
			report_ccb_validation_error(opdata, "Node '%s' still has SUs", osaf_extended_name_borrow(&opdata->objectName));
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}
	}

	/* This node shouldn't be part of any nodegroup. First this node has to be deleted from
	   node group. */
	for (std::map<std::string, AVD_AMF_NG*>::const_iterator it = nodegroup_db->begin();
			it != nodegroup_db->end(); it++) {
		AVD_AMF_NG *ng = it->second;

		if (node_in_nodegroup(Amf::to_string(&(opdata->objectName)), ng) == true) {
			// if the node is being removed from nodegroup too, then it's OK
			TRACE("check if node is being deleted from nodegroup '%s'", ng->name.c_str());
			const SaNameTWrapper ng_name(ng->name);
			t_opData = ccbutil_getCcbOpDataByDN(opdata->ccbId, ng_name);
			
			if (t_opData == nullptr) {
				TRACE("t_opData is nullptr");
				report_ccb_validation_error(opdata, "'%s' exists in"
						" the nodegroup '%s'",
						osaf_extended_name_borrow(&opdata->objectName), ng->name.c_str());
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}

			const SaImmAttrModificationT_2 *mod = nullptr;
			int i = 0;
			bool node_being_removed = false;

			switch (t_opData->operationType) {
			case CCBUTIL_DELETE:
				// the nodegroup is being removed too
				node_being_removed = true;
				break;
			case CCBUTIL_MODIFY:				
				while ((mod = t_opData->param.modify.attrMods[i++]) != nullptr &&
					node_being_removed == false) {
					if (mod->modType == SA_IMM_ATTR_VALUES_DELETE) {
						for (unsigned j = 0; j < mod->modAttr.attrValuesNumber; j++) {
							if (node->name.compare(Amf::to_string(static_cast<SaNameT *>(mod->modAttr.attrValues[j]))) == 0) {
								// node is being removed from nodegroup
								TRACE("node %s is being removed from %s", node->name.c_str(), ng->name.c_str());
								node_being_removed = true;
								break;
							}
						}
					}
				}
				break;
			default:
				break;
			}
			
			if (node_being_removed == false) {
				report_ccb_validation_error(opdata, "'%s' exists in"
						" the nodegroup '%s'",
						osaf_extended_name_borrow(&opdata->objectName), ng->name.c_str());
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		}
	}

done_ok:
	rc = SA_AIS_OK;
	opdata->userData = node;
done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT node_ccb_completed_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_OK;
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;

	TRACE_ENTER2("'%s'", osaf_extended_name_borrow(&opdata->objectName));

	while ((attr_mod = opdata->param.modify.attrMods[i++]) != nullptr) {
		const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;

		if (!strcmp(attribute->attrName, "saAmfNodeSuFailOverProb")) {
			SaTimeT su_failover_prob = *((SaTimeT *)attribute->attrValues[0]);
			if (su_failover_prob == 0) {
				report_ccb_validation_error(opdata,
						"Modification of '%s' failed - invalid saAmfNodeSuFailOverProb (0)",
						osaf_extended_name_borrow(&opdata->objectName));
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfNodeSuFailoverMax")) {
			/*  No validation needed, avoiding fall-through to Unknown Attribute error-case */
		} else if (!strcmp(attribute->attrName, "saAmfNodeAutoRepair")) {
			if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || (attribute->attrValues == NULL)) {
				report_ccb_validation_error(opdata,
						"Invalid saAmfNodeAutoRepair value for '%s'",
						osaf_extended_name_borrow(&opdata->objectName));
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
			uint32_t value = *((SaUint32T *)attribute->attrValues[0]);
			if (value > SA_TRUE) {
				report_ccb_validation_error(opdata,
					"Invalid saAmfNodeAutoRepair '%s'",
					osaf_extended_name_borrow(&opdata->objectName));
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfNodeFailfastOnTerminationFailure")) {
			if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || (attribute->attrValues == NULL)) {
				report_ccb_validation_error(opdata,
						"Invalid saAmfNodeFailfastOnTerminationFailure value for '%s'",
						osaf_extended_name_borrow(&opdata->objectName));
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
			uint32_t value = *((SaUint32T *)attribute->attrValues[0]);
			if (value > SA_TRUE) {
				report_ccb_validation_error(opdata,
					"Invalid saAmfNodeFailfastOnTerminationFailure value for %s'",
					osaf_extended_name_borrow(&opdata->objectName));
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else if (!strcmp(attribute->attrName, "saAmfNodeFailfastOnInstantiationFailure")) {
			if ((attr_mod->modType == SA_IMM_ATTR_VALUES_DELETE) || (attribute->attrValues == nullptr)) {
				report_ccb_validation_error(opdata,
					"Invalid saAmfNodeFailfastOnInstantiationFailure '%s'",
					osaf_extended_name_borrow(&opdata->objectName));
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
			uint32_t value = *((SaUint32T *)attribute->attrValues[0]);
			if (value > SA_TRUE) {
				report_ccb_validation_error(opdata,
					"Invalid saAmfNodeFailfastOnInstantiationFailure value for '%s'",
					osaf_extended_name_borrow(&opdata->objectName));
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else {
			report_ccb_validation_error(opdata, "Modification of '%s' failed-attribute '%s' cannot be modified",
					osaf_extended_name_borrow(&opdata->objectName), attribute->attrName);
			rc = SA_AIS_ERR_BAD_OPERATION;
			goto done;
		}
	}

done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT node_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		if (is_config_valid(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues, opdata))
			rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		rc = node_ccb_completed_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		rc = node_ccb_completed_delete_hdlr(opdata);
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE();
	return rc;
}

static void node_ccb_apply_delete_hdlr(AVD_AVND *node)
{
	TRACE_ENTER2("'%s'", node->name.c_str());
	avd_node_delete_nodeid(node);
	avd_node_delete(node);
	TRACE_LEAVE();
}

static void node_ccb_apply_modify_hdlr(CcbUtilOperationData_t *opdata)
{
	AVD_AVND *node;
	const SaImmAttrModificationT_2 *attr_mod;
	int i = 0;

	TRACE_ENTER2("'%s'", osaf_extended_name_borrow(&opdata->objectName));

	node = avd_node_get(Amf::to_string(&opdata->objectName));
	osafassert(node != nullptr);

	const SaNameTWrapper node_name(node->name);

	i = 0;
	/* Modifications can be done for the following parameters. */
	while (((attr_mod = opdata->param.modify.attrMods[i++])) != nullptr) {
		void *value;
		const SaImmAttrValuesT_2 *attribute = &attr_mod->modAttr;
		AVSV_PARAM_INFO param;

		value = attribute->attrValues[0];

		if (!strcmp(attribute->attrName, "saAmfNodeSuFailOverProb")) {
			SaTimeT su_failover_prob;
			su_failover_prob = *((SaTimeT *)value);
			SaTimeT temp_su_prob;

			m_NCS_OS_HTONLL_P(&temp_su_prob, su_failover_prob);
			su_failover_prob = temp_su_prob;

			memset(((uint8_t *)&param), '\0', sizeof(AVSV_PARAM_INFO));
			param.class_id = AVSV_SA_AMF_NODE;
			param.attr_id = saAmfNodeSuFailoverProb_ID;
			param.act = AVSV_OBJ_OPR_MOD;
			param.name = node_name;
			TRACE("Old saAmfNodeSuFailOverProb is '%llu'", node->saAmfNodeSuFailOverProb);
			if (node->node_state != AVD_AVND_STATE_ABSENT) {
				param.value_len = sizeof(SaTimeT);
				memcpy((char *)&param.value[0], (char *)&su_failover_prob, param.value_len);

				node->saAmfNodeSuFailOverProb = m_NCS_OS_NTOHLL_P(&su_failover_prob);

				avd_snd_op_req_msg(avd_cb, node, &param);
			} else {
				node->saAmfNodeSuFailOverProb = m_NCS_OS_NTOHLL_P(&su_failover_prob);
			}
			TRACE("Modified saAmfNodeSuFailOverProb is '%llu'", node->saAmfNodeSuFailOverProb);


		} else if (!strcmp(attribute->attrName, "saAmfNodeSuFailoverMax")) {
			uint32_t failover_val;

			failover_val = *((SaUint32T *)value);

			memset(((uint8_t *)&param), '\0', sizeof(AVSV_PARAM_INFO));
			param.class_id = AVSV_SA_AMF_NODE;
			param.attr_id = saAmfNodeSuFailoverMax_ID;
			param.act = AVSV_OBJ_OPR_MOD;
			param.name = node_name;
			TRACE("Old saAmfNodeSuFailoverMax is '%u'", node->saAmfNodeSuFailoverMax);

			if (node->node_state != AVD_AVND_STATE_ABSENT) {
				param.value_len = sizeof(uint32_t);
				m_NCS_OS_HTONL_P(&param.value[0], failover_val);
				node->saAmfNodeSuFailoverMax = failover_val;

				avd_snd_op_req_msg(avd_cb, node, &param);
			} else {
				node->saAmfNodeSuFailoverMax = failover_val;
			}
			TRACE("Modified saAmfNodeSuFailoverMax is '%u'", node->saAmfNodeSuFailoverMax);

		} else if (!strcmp(attribute->attrName, "saAmfNodeAutoRepair")) {
			node->saAmfNodeAutoRepair = *((SaBoolT *)attribute->attrValues[0]);
			amflog(LOG_NOTICE, "%s saAmfNodeAutoRepair changed to %u",
				node->name.c_str(), node->saAmfNodeAutoRepair);
		} else if (!strcmp(attribute->attrName, "saAmfNodeFailfastOnTerminationFailure")) {
			node->saAmfNodeFailfastOnTerminationFailure =
					*((SaBoolT *)attribute->attrValues[0]);
			amflog(LOG_NOTICE, "%s saAmfNodeFailfastOnTerminationFailure changed to %u",
				node->name.c_str(), node->saAmfNodeFailfastOnTerminationFailure);
		} else if (!strcmp(attribute->attrName, "saAmfNodeFailfastOnInstantiationFailure")) {
			node->saAmfNodeFailfastOnInstantiationFailure =
					*((SaBoolT *)attribute->attrValues[0]);
			amflog(LOG_NOTICE, "%s saAmfNodeFailfastOnInstantiationFailure changed to %u",
				node->name.c_str(), node->saAmfNodeFailfastOnInstantiationFailure);
			LOG_NO( "%s saAmfNodeFailfastOnInstantiationFailure changed to %u",
				node->name.c_str(), node->saAmfNodeFailfastOnInstantiationFailure);
		} else {
			osafassert(0);
		}
	}			/* while (attr_mod != nullptr) */

	TRACE_LEAVE();
}

static void node_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	AVD_AVND *node;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, osaf_extended_name_borrow(&opdata->objectName));

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		node = node_create(Amf::to_string(&opdata->objectName), opdata->param.create.attrValues);
		osafassert(node);
		node_add_to_model(node);
		break;
	case CCBUTIL_MODIFY:
		node_ccb_apply_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		node_ccb_apply_delete_hdlr(static_cast<AVD_AVND*>(opdata->userData));
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE();
}

/**
 * sets the admin state of a node
 *
 * @param node
 * @param admin_state
 */
void node_admin_state_set(AVD_AVND *node, SaAmfAdminStateT admin_state)
{
	SaAmfAdminStateT old_state  = node->saAmfNodeAdminState;
	
	if (old_state == admin_state)
		return;
	osafassert(admin_state <= SA_AMF_ADMIN_SHUTTING_DOWN);
	if (0 != node->clm_pend_inv) {
		/* Clm operations going on, skip notifications and rt updates for node change. We are using node state
		   as LOCKED for making CLM Admin operation going through. */
		node->saAmfNodeAdminState = admin_state;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, node, AVSV_CKPT_AVND_ADMIN_STATE);
	} else {
		TRACE_ENTER2("%s AdmState %s => %s", node->name.c_str(),
				avd_adm_state_name[node->saAmfNodeAdminState], avd_adm_state_name[admin_state]);

		saflog(LOG_NOTICE, amfSvcUsrName, "%s AdmState %s => %s", node->name.c_str(),
				avd_adm_state_name[node->saAmfNodeAdminState], avd_adm_state_name[admin_state]);
		node->saAmfNodeAdminState = admin_state;
		avd_saImmOiRtObjectUpdate(node->name, "saAmfNodeAdminState",
			SA_IMM_ATTR_SAUINT32T, &node->saAmfNodeAdminState);
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, node, AVSV_CKPT_AVND_ADMIN_STATE);
		avd_send_admin_state_chg_ntf(node->name, SA_AMF_NTFID_NODE_ADMIN_STATE, old_state, node->saAmfNodeAdminState);
	}
	TRACE_LEAVE();
}

/**
 * Sends msg to terminate all SUs on the node as part of lock instantiation
 *
 * @param node
 */
uint32_t avd_node_admin_lock_instantiation(AVD_AVND *node)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%s", node->name.c_str());

	/* terminate all the SUs on this Node */
	for (const auto& su : node->list_of_su) {
		if ((su->saAmfSUPreInstantiable == true) &&
		    (su->saAmfSUPresenceState != SA_AMF_PRESENCE_UNINSTANTIATED) &&
		    (su->saAmfSUPresenceState != SA_AMF_PRESENCE_INSTANTIATION_FAILED) &&
		    (su->saAmfSUPresenceState != SA_AMF_PRESENCE_TERMINATION_FAILED)) {

			if (avd_snd_presence_msg(avd_cb, su, true) == NCSCC_RC_SUCCESS) {
				node->su_cnt_admin_oper++;
			} else {
				rc = NCSCC_RC_FAILURE;
				LOG_WA("Failed Termination '%s'", su->name.c_str());
			}
		}
	}

	TRACE_LEAVE2("%u, %u", rc, node->su_cnt_admin_oper);
	return rc;
}

/**
 * Sends msg to instantiate SUs on the node as part of unlock instantiation
 *
 * @param node
 */
uint32_t node_admin_unlock_instantiation(AVD_AVND *node)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%s", node->name.c_str());

	/* instantiate the SUs on this Node */
	for (const auto& su : node->list_of_su) {
		if ((su->saAmfSUAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
		    (su->sg_of_su->saAmfSGAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
		    (su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED) && 
		    (su->saAmfSUOperState == SA_AMF_OPERATIONAL_ENABLED)) {

			if (su->saAmfSUPreInstantiable == true) {
				if (su->sg_of_su->saAmfSGNumPrefInserviceSUs >
						(sg_instantiated_su_count(su->sg_of_su) +
						 su->sg_of_su->try_inst_counter)) {
					if (avd_snd_presence_msg(avd_cb, su, false) == NCSCC_RC_SUCCESS) {
						node->su_cnt_admin_oper++;
						su->sg_of_su->try_inst_counter++;
					} else {
						rc = NCSCC_RC_FAILURE;
						LOG_WA("Failed Instantiation '%s'", su->name.c_str());
					}
				}
			} else
				avd_sg_adjust_config(su->sg_of_su);
		}
	}

	node_reset_su_try_inst_counter(node);

	TRACE_LEAVE2("%u, %u", rc, node->su_cnt_admin_oper);
	return rc;
}

/**
 * processes  lock, unlock and shutdown admin operations on a node
 *
 * @param node
 * @param invocation
 * @param operationId
 */
void avd_node_admin_lock_unlock_shutdown(AVD_AVND *node,
				    SaInvocationT invocation, SaAmfAdminOperationIdT operationId)
{
	AVD_CL_CB *cb = (AVD_CL_CB *)avd_cb;
	bool su_admin = false;
	AVD_SU_SI_REL *curr_susi;
	AVD_AVND *su_node_ptr = nullptr;
	AVD_AVND *su_sg_node_ptr = nullptr;
	SaAmfAdminStateT new_admin_state;
	bool is_assignments_done = false;

	TRACE_ENTER2("%s", node->name.c_str());

        /* determine the new_admin_state from operation ID */
	if (operationId == SA_AMF_ADMIN_SHUTDOWN)
		new_admin_state = SA_AMF_ADMIN_SHUTTING_DOWN;
	else if (operationId == SA_AMF_ADMIN_UNLOCK)
		new_admin_state = SA_AMF_ADMIN_UNLOCKED;
	else
		new_admin_state = SA_AMF_ADMIN_LOCKED;

	/* If the node is not yet operational just modify the admin state field
	 * incase of shutdown to lock and return success as this will only cause
	 * state filed change and no semantics need to be followed.
	 */
	if (node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED && 
	    invocation != 0) {
		if (new_admin_state == SA_AMF_ADMIN_SHUTTING_DOWN) {
			node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
		} else {
			node_admin_state_set(node, new_admin_state);
		}
		avd_saImmOiAdminOperationResult(cb->immOiHandle, invocation, SA_AIS_OK);
		goto end;
	}

	/* Based on the admin operation that is been done call the corresponding.
	 * Redundancy model specific functionality for each of the SUs on 
	 * the node.
	 */

	switch (new_admin_state) {
	case SA_AMF_ADMIN_UNLOCKED:

		for (const auto& su : node->list_of_su) {

			/* if SG to which this SU belongs and has SI assignments is undergoing 
			 * su semantics return error.
			 */
			if ((su->list_of_susi != AVD_SU_SI_REL_NULL) &&
			    (su->sg_of_su->sg_fsm_state != AVD_SG_FSM_STABLE)) {

				/* Dont go ahead as a SG that is undergoing transition is
				 * there related to this node.
				 */
				LOG_WA("invalid sg state %u for unlock", su->sg_of_su->sg_fsm_state);
				
				if (invocation != 0)
					report_admin_op_error(cb->immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
							"invalid sg state %u for unlock", su->sg_of_su->sg_fsm_state);
				goto end;
			}
		}		/* while(su != AVD_SU_NULL) */

		/* For each of the SUs calculate the readiness state. This routine is called
		 * only when AvD is in AVD_APP_STATE. call the SG FSM with the new readiness
		 * state.
		 */

		node_admin_state_set(node, new_admin_state);

		/* store invocation early since insvc logic depends on it */
		node->admin_node_pend_cbk.invocation = invocation;
		node->admin_node_pend_cbk.admin_oper = operationId;
		node->su_cnt_admin_oper = 0;

		for (const auto& su : node->list_of_su) {
			if (su->is_in_service() == true) {
				su->set_readiness_state(SA_AMF_READINESS_IN_SERVICE);
				su->sg_of_su->su_insvc(cb, su);

				/* Since an SU has come in-service re look at the SG to see if other
				 * instantiations or terminations need to be done.
				 */
			}
			avd_sg_app_su_inst_func(cb, su->sg_of_su);
		}
		if (node->su_cnt_admin_oper == 0 && invocation != 0) {
			avd_saImmOiAdminOperationResult(cb->immOiHandle, invocation, SA_AIS_OK);
			/* reset state, invocation consumed */
			node->admin_node_pend_cbk.invocation = 0;
			node->admin_node_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(0);
		}
		break;		/* case NCS_ADMIN_STATE_UNLOCK: */

	case SA_AMF_ADMIN_LOCKED:
	case SA_AMF_ADMIN_SHUTTING_DOWN:

		for (const auto& su : node->list_of_su) {
			if (su->list_of_susi != AVD_SU_SI_REL_NULL) {
				is_assignments_done = true;
				/* verify that two assigned SUs belonging to the same SG are not
				 * on this node 
				 */
				for (const auto& su_sg : su->sg_of_su->list_of_su) {
					su_node_ptr = su->get_node_ptr();
					su_sg_node_ptr = su_sg->get_node_ptr();

					if ((su != su_sg) &&
					    (su_node_ptr == su_sg_node_ptr) &&
					    (su_sg->list_of_susi != AVD_SU_SI_REL_NULL) &&
					    (!((su->sg_of_su->sg_redundancy_model ==
					    		SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL) &&
					    		(new_admin_state == SA_AMF_ADMIN_LOCKED)))) {
						LOG_WA("Node lock/shutdown not allowed with two SUs on same node");
						if (invocation != 0)
							report_admin_op_error(cb->immOiHandle, invocation,
									SA_AIS_ERR_NOT_SUPPORTED, nullptr,
									"Node lock/shutdown not allowed with two SUs"
									" on same node");
						else {
							saClmResponse_4(cb->clmHandle, node->clm_pend_inv, SA_CLM_CALLBACK_RESPONSE_ERROR);
							node->clm_pend_inv = 0;
						} 
						goto end;
					}
				}

				/* if SG to which this SU belongs and has SI assignments is undergoing 
				 * any semantics other than for this SU return error.
				 */
				if (su->sg_of_su->sg_fsm_state != AVD_SG_FSM_STABLE) {
					if ((su->sg_of_su->sg_fsm_state != AVD_SG_FSM_SU_OPER) ||
					    (node->saAmfNodeAdminState != SA_AMF_ADMIN_SHUTTING_DOWN) ||
					    (new_admin_state != SA_AMF_ADMIN_LOCKED)) {
						LOG_WA("invalid sg state %u for lock/shutdown",
						       su->sg_of_su->sg_fsm_state);
						if (invocation != 0)
							report_admin_op_error(cb->immOiHandle, invocation,
									SA_AIS_ERR_TRY_AGAIN, nullptr,
									"invalid sg state %u for lock/shutdown",
									su->sg_of_su->sg_fsm_state);
						else {
							saClmResponse_4(cb->clmHandle, node->clm_pend_inv, SA_CLM_CALLBACK_RESPONSE_ERROR);
							node->clm_pend_inv = 0;
						} 

						goto end;
					}
				}
				/*if (su->sg_of_su->sg_fsm_state != AVD_SG_FSM_STABLE) */
				if (su->list_of_susi->state == SA_AMF_HA_ACTIVE) {
					su_admin = true;
				} else if ((node->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) &&
					   (su_admin == false) &&
					   (su->sg_of_su->sg_redundancy_model == SA_AMF_N_WAY_REDUNDANCY_MODEL)) {
					for (curr_susi = su->list_of_susi;
					     (curr_susi) && ((SA_AMF_HA_ACTIVE != curr_susi->state) ||
							     ((AVD_SU_SI_STATE_UNASGN == curr_susi->fsm)));
					     curr_susi = curr_susi->su_next) ;
					if (curr_susi)
						su_admin = true;
				}

			}
		}		/* for (const auto& su : node->list_of_su) */

		if(invocation != 0) {
			node_admin_state_set(node, new_admin_state);
		} else {
			if (is_assignments_done == true) {
				/* In clm node adm operation, node admin_state is temporarily set to new_admin_state
				 * as node->admin_state is used in the SI failover processing, It will be reset in 
				 * the susi delete response processing
				 */ 
				node_admin_state_set(node, new_admin_state);
			}
		}

		/* Now call the SG FSM for each of the SUs that have SI assignment. */
		for (const auto& su : node->list_of_su) {
			is_assignments_done = false;
			su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
			if (su->list_of_susi != AVD_SU_SI_REL_NULL) {
				is_assignments_done = true;
				su->sg_of_su->su_admin_down(cb, su, node);
			}

			/* since an SU is now OOS we need to take a relook at the SG. */
			avd_sg_app_su_inst_func(cb, su->sg_of_su);

			if ((is_assignments_done) && ((su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SG_REALIGN) ||
						(su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SU_OPER))) {
				node->su_cnt_admin_oper++;
				TRACE("su_cnt_admin_oper:%u", node->su_cnt_admin_oper);
			}
		}

		if ((node->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) && (su_admin == false)) {
			node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
		}

		if (node->su_cnt_admin_oper == 0 && invocation != 0)
			avd_saImmOiAdminOperationResult(cb->immOiHandle, invocation, SA_AIS_OK);
		else if (invocation != 0){
			node->admin_node_pend_cbk.invocation = invocation;
			node->admin_node_pend_cbk.admin_oper = operationId;
		}
		break;		/* case NCS_ADMIN_STATE_LOCK: case NCS_ADMIN_STATE_SHUTDOWN: */

	default:
		osafassert(0);
		break;
	}
 end:
	TRACE_LEAVE();
}

/**
 * Set term_state for all pre-inst SUs hosted on the specified node
 * 
 * @param node
 */
void AVD_AVND::node_sus_termstate_set(bool term_state) const
{
	for (const auto& su : list_of_su) {
		if (su->saAmfSUPreInstantiable == true)
			su->set_term_state(term_state);
	}
}

/**
 * Handle admin operations on SaAmfNode objects.
 *      
 * @param immOiHandle             
 * @param invocation
 * @param objectName
 * @param operationId
 * @param params
 */
static void node_admin_op_cb(SaImmOiHandleT immOiHandle, SaInvocationT invocation,
			     const SaNameT *objectName, SaImmAdminOperationIdT operationId,
			     const SaImmAdminOperationParamsT_2 **params)
{
	AVD_AVND *node;
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER2("%llu, '%s', %llu", invocation, osaf_extended_name_borrow(objectName), operationId);

	node = avd_node_get(Amf::to_string(objectName));
	osafassert(node != AVD_AVND_NULL);

	if (node->admin_node_pend_cbk.admin_oper != 0) {
		/* Donot pass node->admin_node_pend_cbk here as previous counters will get reset in
		   report_admin_op_error. */
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
				"Node undergoing admin operation");
		goto done;
	}

	/* Check for any conflicting admin operations */

	for (const auto& su : node->list_of_su) {
		if (su->pend_cbk.admin_oper != 0) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
					"SU on this node is undergoing admin op (%s)", su->name.c_str());
			goto done;
		}

		if ((su->saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATING) ||
				(su->saAmfSUPresenceState == SA_AMF_PRESENCE_TERMINATING) || 
				(su->saAmfSUPresenceState == SA_AMF_PRESENCE_RESTARTING)) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
					"'%s' presence state is '%u'", su->name.c_str(), su->saAmfSUPresenceState);
			goto done;
		}

		if (su->sg_of_su->sg_fsm_state != AVD_SG_FSM_STABLE) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
					"SG'%s' of SU'%s' on this node not in STABLE state",
					su->sg_of_su->name.c_str(), su->name.c_str());
			goto done;
		}
		if (su->sg_of_su->any_assignment_absent() == true ) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
					"SG has absent assignment (%s)", su->sg_of_su->name.c_str());
			goto done;
		}
	}

	if (node->clm_pend_inv != 0) {
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TRY_AGAIN, nullptr,
				"Clm lock operation going on");
		goto done;
	}

	switch (operationId) {
	case SA_AMF_ADMIN_SHUTDOWN:
		if (node->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
					"'%s' Already in SHUTTING DOWN state", node->name.c_str());
			goto done;
		}

		if (node->saAmfNodeAdminState != SA_AMF_ADMIN_UNLOCKED) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"'%s' Invalid Admin Operation SHUTDOWN in state %s",
					node->name.c_str(), avd_adm_state_name[node->saAmfNodeAdminState]);
			goto done;
		}

		if (node->node_info.member == false) {
			node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
			LOG_NO("'%s' SHUTDOWN: CLM node is not member", node->name.c_str());
			avd_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);
			goto done;
		}

		avd_node_admin_lock_unlock_shutdown(node, invocation, static_cast<SaAmfAdminOperationIdT>(operationId));
		break;

	case SA_AMF_ADMIN_UNLOCK:
		if (node->saAmfNodeAdminState == SA_AMF_ADMIN_UNLOCKED) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
					"'%s' Already in UNLOCKED state", node->name.c_str());
			goto done;
		}

		if (node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"'%s' Invalid Admin Operation UNLOCK in state %s",
					node->name.c_str(), avd_adm_state_name[node->saAmfNodeAdminState]);
			goto done;
		}

		if (node->node_info.member == false) {
			LOG_NO("'%s' UNLOCK: CLM node is not member", node->name.c_str());
			node_admin_state_set(node, SA_AMF_ADMIN_UNLOCKED);
			avd_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);
			goto done;
		}

		if (avd_cb->init_state == AVD_INIT_DONE) {
			node_admin_state_set(node, SA_AMF_ADMIN_UNLOCKED);
			for (const auto& su : node->list_of_su) {
				if (su->is_in_service() == true) {
					su->set_readiness_state(SA_AMF_READINESS_IN_SERVICE);
				}
			}
			avd_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);
			break;
		}

		avd_node_admin_lock_unlock_shutdown(node, invocation, static_cast<SaAmfAdminOperationIdT>(operationId));
		break;

	case SA_AMF_ADMIN_LOCK:
		if (node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
					"'%s' Already in LOCKED state", node->name.c_str());
			goto done;
		}

		if (node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"'%s' Invalid Admin Operation LOCK in state %s",
					node->name.c_str(), avd_adm_state_name[node->saAmfNodeAdminState]);
			goto done;
		}

		if (node->node_info.member == false) {
			node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
			LOG_NO("%s' LOCK: CLM node is not member", node->name.c_str());
			avd_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);
			goto done;
		}

		if (avd_cb->init_state == AVD_INIT_DONE) {
			node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);
			for (const auto& su : node->list_of_su) {
				su->set_readiness_state(SA_AMF_READINESS_OUT_OF_SERVICE);
			}
			avd_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);
			break;
		}

		avd_node_admin_lock_unlock_shutdown(node, invocation, static_cast<SaAmfAdminOperationIdT>(operationId));
		break;

	case SA_AMF_ADMIN_LOCK_INSTANTIATION:
		if (node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
					"'%s' Already in LOCKED INSTANTIATION state", node->name.c_str());
			goto done;
		}

		if (node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"'%s' Invalid Admin Operation LOCK_INSTANTIATION in state %s",
					node->name.c_str(), avd_adm_state_name[node->saAmfNodeAdminState]);
			goto done;
		}

		node->node_sus_termstate_set(true);
		node_admin_state_set(node, SA_AMF_ADMIN_LOCKED_INSTANTIATION);

		if (node->node_info.member == false) {
			LOG_NO("'%s' LOCK_INSTANTIATION: CLM node is not member", node->name.c_str());
			avd_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);
			goto done;
		}

		if (node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED) {
			LOG_NO("'%s' LOCK_INSTANTIATION: AMF node oper state disabled", node->name.c_str());
			avd_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);
			goto done;
		}

		/* now do the lock_instantiation of the node */
		if (NCSCC_RC_SUCCESS == avd_node_admin_lock_instantiation(node)) {
			/* Check if we have pending admin operations
			 * Otherwise reply to IMM immediately*/
			if( node->su_cnt_admin_oper != 0)
			{
				node->admin_node_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(operationId);
				node->admin_node_pend_cbk.invocation = invocation;
			}
			else
				avd_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);
		} else {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_REPAIR_PENDING, nullptr,
					"LOCK_INSTANTIATION FAILED");
			avd_node_oper_state_set(node, SA_AMF_OPERATIONAL_DISABLED);
			goto done;
		}
		break;

	case SA_AMF_ADMIN_UNLOCK_INSTANTIATION:
		if (node->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NO_OP, nullptr,
					"'%s' Already in LOCKED state", node->name.c_str());
			goto done;
		}

		if (node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_BAD_OPERATION, nullptr,
					"'%s' Invalid Admin Operation UNLOCK_INSTANTIATION in state %s",
					node->name.c_str(), avd_adm_state_name[node->saAmfNodeAdminState]);
			goto done;
		}

		node->node_sus_termstate_set(false);
		node_admin_state_set(node, SA_AMF_ADMIN_LOCKED);

		if (node->node_info.member == false) {
			LOG_NO("'%s' UNLOCK_INSTANTIATION: CLM node is not member", node->name.c_str());
			avd_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);
			goto done;
		}

		if (node->saAmfNodeOperState == SA_AMF_OPERATIONAL_DISABLED) {
			LOG_NO("'%s' UNLOCK_INSTANTIATION: AMF node oper state disabled", node->name.c_str());
			avd_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);
			goto done;
		}
		//If any Nodegroup of node is still in lock-in state then do not instantiate the node.
		if (any_ng_in_locked_in_state(node) == true) {
			TRACE_1("Atleast one Node group of node is in lock-in state");
			avd_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);
			node->admin_node_pend_cbk.invocation = 0;
			node->admin_node_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(0);
			goto done;
		}

		/* now do the unlock_instantiation of the node */
		if (NCSCC_RC_SUCCESS == node_admin_unlock_instantiation(node)) {
			/* Check if we have pending admin operations.
			 * Otherwise reply to IMM immediately*/
			if( node->su_cnt_admin_oper != 0)
			{
				node->admin_node_pend_cbk.admin_oper = static_cast<SaAmfAdminOperationIdT>(operationId);
				node->admin_node_pend_cbk.invocation = invocation;
			}
			else
				avd_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_OK);
		} else {
			rc = SA_AIS_ERR_TIMEOUT;
			report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_TIMEOUT, nullptr,
					"UNLOCK_INSTANTIATION FAILED");
			goto done;
		}
		break;

	default:
		report_admin_op_error(immOiHandle, invocation, SA_AIS_ERR_NOT_SUPPORTED, nullptr,
				"UNSUPPORTED ADMIN OPERATION (%llu)", operationId);
		break;
	}

 done:
	TRACE_LEAVE2("%u", rc);
}

void avd_node_add_su(AVD_SU *su)
{
	if (su->name.find("safApp=OpenSAF") != std::string::npos) {
		su->su_on_node->list_of_ncs_su.push_back(su);
		std::sort(su->su_on_node->list_of_ncs_su.begin(), su->su_on_node->list_of_ncs_su.end(),
			[](const AVD_SU *a, const AVD_SU *b) -> bool {return a->saAmfSURank < b->saAmfSURank;});
	} else {
		su->su_on_node->list_of_su.push_back(su);
		std::sort(su->su_on_node->list_of_su.begin(), su->su_on_node->list_of_su.end(),
			[](const AVD_SU *a, const AVD_SU *b) -> bool {return a->saAmfSURank < b->saAmfSURank;});
	}
}

void avd_node_remove_su(AVD_SU *su) {
  std::vector<AVD_SU*> *su_list;

  if ((su->sg_of_su) && (su->sg_of_su->sg_ncs_spec == true)) {
    su_list = &su->su_on_node->list_of_ncs_su;
  }
  else {
    su_list = &su->su_on_node->list_of_su;
  }

  auto pos = std::find(su_list->begin(), su_list->end(), su);
  if(pos != su_list->end()) {
    su_list->erase(pos);
  } else {
    /* Log a fatal error */
    osafassert(0);
  }
}

/**
 * Reset try_inst_counter of SG of all SU of the node given.
 *
 * @param node pointer
 * @return None
 */
void node_reset_su_try_inst_counter(const AVD_AVND *node)
{
	/* Reset the counters.*/
	for (const auto& su : node->list_of_su) {
		su->sg_of_su->try_inst_counter = 0;
	}
}
/**
 * @brief  Checks all  nodegroup of nodes are in UNLOCKED state.
 * @param  ptr to Node (AVD_AVND).
 * @return true/false
 */
bool are_all_ngs_in_unlocked_state(const AVD_AVND *node)
{
        for (std::map<std::string, AVD_AMF_NG*>::const_iterator it = nodegroup_db->begin();
                        it != nodegroup_db->end(); it++) {
                AVD_AMF_NG *ng = it->second;
                if ((node_in_nodegroup(node->name, ng) == true) &&
                                (ng->saAmfNGAdminState != SA_AMF_ADMIN_UNLOCKED))
                        return false;
        }
        return true;
}
/**
 * @brief  Checks if any nodegroup of node are is in LOCKED_IN state.
 * @param  ptr to Node (AVD_AVND).
 * @return true/false
 */
bool any_ng_in_locked_in_state(const AVD_AVND *node)
{
	for (std::map<std::string, AVD_AMF_NG*>::const_iterator it = nodegroup_db->begin();
			it != nodegroup_db->end(); it++) {
		AVD_AMF_NG *ng = it->second;
		if ((node_in_nodegroup(node->name, ng) == true) &&
				(ng->saAmfNGAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION)) {
			TRACE("Nodegroup '%s' is in locked-in", ng->name.c_str());
			return true;
		}
	}
	return false;
}

void avd_node_constructor(void)
{
	node_name_db = new AmfDb<std::string, AVD_AVND>;
	node_id_db = new AmfDb<uint32_t, AVD_AVND>;

	avd_class_impl_set("SaAmfNode", nullptr, node_admin_op_cb,
			node_ccb_completed_cb, node_ccb_apply_cb);
}

