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

  This module is the include file for handling Availability Directors 
  Node Director structure.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVD_AVND_H
#define AVD_AVND_H

#include <saAmf.h>
#include <saImm.h>
#include <ncsdlib.h>
#include <mds_papi.h>
#include <su.h>
#include <amf_d2nmsg.h>
#include <timer.h>
#include <amf_db_template.h>
#include <set>
#include <vector>
#include <string>
#include <susi.h>

class AVD_SU;
struct avd_cluster_tag;
struct avd_node_sw_bundle_tag;
struct CcbUtilOperationData;

typedef enum {

	AVD_AVND_STATE_ABSENT,
	AVD_AVND_STATE_NO_CONFIG,
	AVD_AVND_STATE_PRESENT,
	AVD_AVND_STATE_GO_DOWN,
	AVD_AVND_STATE_SHUTTING_DOWN,
	AVD_AVND_STATE_NCS_INIT
} AVD_AVND_STATE;

/* Fail-over Node List */
typedef struct avd_fail_over_node {

	SaClmNodeIdT node_id;
} AVD_FAIL_OVER_NODE;
class AVD_AMF_NG;
/* Availability directors Node Director structure(AVD_AVND): 
 * This data structure lives in the AvD and reflects data points
 * associated with the AvD's remote, distributed AvNDs. In addition
 * this data structure is also a home for data-points required to
 * support the cluster membership service.
 */

class AVD_AVND {
 public:
  AVD_AVND();
  explicit AVD_AVND(const std::string& dn);
  ~AVD_AVND();

  bool is_node_lock();
  std::string name; /* DN */ 
  std::string node_name;    /* RDN value, normally the short host name */
  SaClmClusterNodeT_4 node_info;	/* the node information of the node on
					 * which this AvND exists. The length
					 * field of nodeName structure is in
					 * network order. The nodename is
					 * used as the index.
					 * Checkpointing - Sent as a one time update.
					 */

  MDS_DEST adest;		/* the Adest address of this
				 * nodes AvND.
				 * Checkpointing - Sent on node up.
				 */
  /************ AMF B.04 **************************************************/
  std::string saAmfNodeClmNode;
  char *saAmfNodeCapacity;
  SaTimeT saAmfNodeSuFailOverProb;
  SaUint32T saAmfNodeSuFailoverMax;
  SaBoolT saAmfNodeAutoRepair;
  SaBoolT saAmfNodeFailfastOnTerminationFailure;
  SaBoolT saAmfNodeFailfastOnInstantiationFailure;
  SaAmfAdminStateT saAmfNodeAdminState;
  SaAmfOperationalStateT saAmfNodeOperState;

  AVD_ADMIN_OPER_CBK admin_node_pend_cbk;	/*to store any pending admin op
                                                  callbacks on this node */
  uint32_t su_cnt_admin_oper;	/* count to keep track SUs on this node 
                                   undergoing node admin op */

  /************ AMF B.04 **************************************************/

  AVD_AVND_STATE node_state;	/* F.S.M state of the AVND 
                                 * Checkpointing - Sent independent update 
                                 */

  std::vector<AVD_SU*> list_of_ncs_su;	/* the list of NCS service units on
                                 * this node.
                                 */
  std::vector<AVD_SU*> list_of_su;	/* the list of service units on this
                         * node that are application specific.
                         */
  NCS_DB_LINK_LIST pg_csi_list;	/* list of csis for which pg is tracked 
					 * from this node */

  uint32_t rcv_msg_id;	/* The receive message id counter 
                         * Checkpointing - Sent independent update 
                         */

  uint32_t snd_msg_id;	/* The send message id counter 
                         * Checkpointing - Sent independent update 
                         */

  AVD_AVND *cluster_list_node_next;
  struct avd_cluster_tag *cluster;
  SaInvocationT clm_pend_inv; /* pending response for any clm track cb */
  bool clm_change_start_preceded; /* to indicate there was CLM start cbk before CLM completed cb. */
  bool recvr_fail_sw; /* to indicate there was node reboot because of node failover/switchover.*/
  AVD_AMF_NG *admin_ng; /* points to the nodegroup on which admin operation is going on.*/
  uint16_t node_up_msg_count; /* to count of node_up msg that director had received from this node */
  bool reboot;

  //Member functions.
  void node_sus_termstate_set(bool term_state) const;
 private:
  void initialize();
  // disallow copy and assign
  AVD_AVND(const AVD_AVND&);
  void operator=(const AVD_AVND&);
};

bool operator<(const AVD_AVND& lhs, const AVD_AVND& rhs);

struct NodeNameCompare: public std::binary_function<AVD_AVND*, AVD_AVND*, bool> {
  bool operator() (const AVD_AVND* lhs, const AVD_AVND* rhs);
};

extern AmfDb<std::string, AVD_AVND> *node_name_db;
extern AmfDb<uint32_t, AVD_AVND> *node_id_db;
extern AmfDb<uint32_t, AVD_FAIL_OVER_NODE> *node_list_db;
extern std::set<uint32_t> *amfnd_svc_db; 
extern std::map<SaClmNodeIdT, MDS_SVC_PVT_SUB_PART_VER> nds_mds_ver_db;
class AVD_AMF_NG {
public:
	AVD_AMF_NG();

	std::string name;
	std::set<std::string> saAmfNGNodeList;
	
	/* number of element in saAmfNGNodeList */
	uint32_t number_nodes() const {
		return saAmfNGNodeList.size();
	}

	struct avd_ng_tag *cluster_list_ng_next;
	struct avd_cluster_tag *ng_on_cluster;
	SaAmfAdminStateT  saAmfNGAdminState;
	AVD_ADMIN_OPER_CBK admin_ng_pend_cbk;   /*to store any pending admin op
						  callbacks on this node group*/
	std::set<std::string> node_oper_list; /* list of nodes impacted because of 
						 node group admin op */
	uint32_t oper_list_size() const {
		return node_oper_list.size();
	}
	void assign_unlocked_nodes() const;
 private:
	// disallow copy and assign
	AVD_AMF_NG(const AVD_AMF_NG&);
	void operator=(const AVD_AMF_NG&);
};
extern AmfDb<std::string, AVD_AMF_NG> *nodegroup_db;
#define AVD_AVND_NULL     ((AVD_AVND *)0)

#define m_AVD_SET_AVND_RCV_ID(cb,node,rcvid) {\
node->rcv_msg_id = rcvid;\
m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, node, AVSV_CKPT_AVND_RCV_MSG_ID);\
}

/* AMF Node */
extern AVD_AVND *avd_node_new(const std::string& dn);
extern void avd_node_delete(AVD_AVND *avnd);
extern void avd_node_db_add(AVD_AVND *node);
extern AVD_AVND *avd_node_get(const std::string& node_name);
extern AVD_AVND *avd_node_getnext(const std::string& node_name);
extern uint32_t avd_node_add_nodeid(AVD_AVND *avnd);
extern void avd_node_delete_nodeid(AVD_AVND *node);
extern AVD_AVND *avd_node_find_nodeid(SaClmNodeIdT node_id);
extern SaAisErrorT avd_node_config_get(void);
extern void avd_node_state_set(AVD_AVND *node, AVD_AVND_STATE node_state);
extern void avd_node_oper_state_set(AVD_AVND *node, SaAmfOperationalStateT oper_state);
extern void node_admin_state_set(AVD_AVND *node, SaAmfAdminStateT admin_state);
extern void avd_node_constructor(void);
extern void avd_node_add_su(AVD_SU *su);
extern void avd_node_remove_su(AVD_SU *su);
extern uint32_t avd_node_admin_lock_instantiation(AVD_AVND *node);
extern uint32_t node_admin_unlock_instantiation(AVD_AVND *node);
extern void avd_node_admin_lock_unlock_shutdown(AVD_AVND *node,
			    SaInvocationT invocation, SaAmfAdminOperationIdT operationId);
extern void node_reset_su_try_inst_counter(const AVD_AVND *node);
/* AMF Node group */
extern SaAisErrorT avd_ng_config_get(void);
extern AVD_AMF_NG *avd_ng_get(const std::string& dn);
extern void avd_ng_constructor(void);
extern bool node_in_nodegroup(const std::string& node, const AVD_AMF_NG *ng);

/* AMF Node SW Bundle */
extern void avd_nodeswbundle_constructor(void);

extern void ng_complete_admin_op(AVD_AMF_NG *ng, SaAisErrorT result);
extern void avd_ng_admin_state_set(AVD_AMF_NG* ng, SaAmfAdminStateT state);
extern bool are_all_ngs_in_unlocked_state(const AVD_AVND *node);
extern bool any_ng_in_locked_in_state(const AVD_AVND *node);
void avd_ng_restore_headless_states(AVD_CL_CB *cb, struct avd_su_si_rel_tag* susi);

#endif
