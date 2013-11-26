/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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
 * Author(s):  Emerson Network Power
 */

#ifndef CLMS_CB_H
#define CLMS_CB_H

#define IMPLEMENTER_NAME	"safClmService"
#define CLMS_HA_INIT_STATE	0

typedef enum clms_tmr_type_t {
	CLMS_TMR_BASE,
	CLMS_CLIENT_RESP_TMR = CLMS_TMR_BASE,
	CLMS_NODE_LOCK_TMR,
	CLMS_TMR_MAX
} CLMS_TMR_TYPE;

typedef enum {
	PLM = 1,
	IMM_LOCK = 2,
	IMM_SHUTDOWN = 3,
	IMM_UNLOCK = 4,
	IMM_RECONFIGURED = 5
} ADMIN_OP;

/* Cluster Properties */
typedef struct cluster_db_t {
	SaNameT name;
	SaUint32T num_nodes;
	SaTimeT init_time;
	SaBoolT rtu_pending; /* Flag to indicate whether an RTU failed and is pending */
	/*struct cluster_db_t *next; */	/* Multiple cluster is not supported as of now */
} CLMS_CLUSTER_INFO;

/* A CLM node properties record */
typedef struct cluster_node_t {
	NCS_PATRICIA_NODE pat_node_eename;
	NCS_PATRICIA_NODE pat_node_name;
	NCS_PATRICIA_NODE pat_node_id;
	SaClmNodeIdT node_id;
	SaClmNodeAddressT node_addr;
	SaNameT node_name;
	SaNameT ee_name;
	SaBoolT member;
	SaTimeT boot_time;
	SaUint64T init_view;
	SaBoolT disable_reboot;
	SaTimeT lck_cbk_timeout;
	SaClmAdminStateT admin_state;
	SaClmClusterChangesT change;
#ifdef ENABLE_AIS_PLM
	SaPlmReadinessStateT ee_red_state;
#endif
	SaBoolT nodeup;		/*Check for the connectivity */
	SaInvocationT curr_admin_inv;
	NCS_PATRICIA_TREE trackresp;
	SaBoolT stat_change;	/*Required to check for the number of nodes on which change has occured */
	ADMIN_OP admin_op;	/*plm or clm operation */
	timer_t lock_timerid;	/*Timer id for admin lock operation */
	SaInvocationT plm_invid;	/*plmtrack callback invocation id */
	SaBoolT rtu_pending; /* Flag to mark whether an IMM RunTime attribute Update is pending and to be retried */
	SaBoolT admin_rtu_pending; /* Flag to mark whether an IMM RunTime attribute Update is pending and to be retried */
	struct cluster_node_t *dep_node_list;	/*Dependent nodes list - in case of plm operation */
	struct cluster_node_t *next;
} CLMS_CLUSTER_NODE;

typedef struct clms_rem_reboot_t {
	char * str;
	CLMS_CLUSTER_NODE *node;
}CLMS_REM_REBOOT_INFO;

/* List to track the responses from CLMAs.
 * A node is added to this list when a track callback
 * is sent to a CLMA.
 * A node is deleted from this list when a saClmResponse()
 * is received from the CLMA. 
 */
typedef struct clms_track_info_t {
	NCS_PATRICIA_NODE pat_node;
	MDS_DEST client_dest;
	uint32_t client_id;
	uint32_t client_id_net;
	SaInvocationT inv_id;
	/* How about Invocation id!? Check the current CLMA and accordingly add */
} CLMS_TRACK_INFO;

/* CLM client information record */
typedef struct client_info_t {
	NCS_PATRICIA_NODE pat_node;
	uint32_t client_id;	/* Changes to 32 -bit from 64-bit */
	uint32_t client_id_net;	/* dude client_id_net it was 32 bit now got changed to 64 bit, see counter effects */
	MDS_DEST mds_dest;
	SaUint8T track_flags;
	SaInvocationT inv_id;
} CLMS_CLIENT_INFO;

/* Confirmation to be data to be checkpointed*/
/* CHECKPOINT status */
typedef enum checkpoint_status {
	COLD_SYNC_IDLE = 0,
	REG_REC_SENT,
	SUBSCRIPTION_REC_SENT,
	COLD_SYNC_COMPLETE,
} CHECKPOINT_STATE;

typedef struct clma_down_list_tag {
	MDS_DEST mds_dest;
	struct clma_down_list_tag *next;
} CLMA_DOWN_LIST;

typedef struct node_down_list_tag {
	SaClmNodeIdT node_id;
	struct node_down_list_tag *next;
} NODE_DOWN_LIST;

/* CLM Server control block */
typedef struct clms_cb_t {
	/* MDS, MBX & thread related defs */
	SYSF_MBX mbx;		/* CLM Server's Mail Box */
	MDS_HDL mds_hdl;	/* handle to interact with MDS */
	MDS_HDL mds_vdest_hdl;	/* Not Needed. can be removed */
	V_DEST_RL mds_role;	/* Current MDS VDEST role */
	MDS_DEST vaddr;		/* CLMS' MDS address */
	uint32_t my_hdl;		/* Handle Manager Handle */
	PCS_RDA_ROLE my_role;	/* RDA provided role */
	SaVersionT clm_ver;	/*Currently Supported CLM Version */
	/* AMF related defs */
	SaNameT comp_name;	/* My AMF name */
	SaAmfHandleT amf_hdl;	/* Handle obtained from AMF */
	SaInvocationT amf_inv;	/* AMF Invocation Id */
	bool is_quiesced_set;	/* */
	SaSelectionObjectT amf_sel_obj;	/* AMF provided selection object */
	NCS_SEL_OBJ sighdlr_sel_obj;	/* Selection object to handle SIGUSR1 */
	SaAmfHAStateT ha_state;	/* My current AMF HA state */
	bool csi_assigned;
	NCS_MBCSV_HDL mbcsv_hdl;
	SaSelectionObjectT mbcsv_sel_obj;	/* MBCSv Selection Object to maintain a HotStandBy CLMS */
	NCS_MBCSV_CKPT_HDL mbcsv_ckpt_hdl;
	CHECKPOINT_STATE ckpt_state;
	SaImmOiHandleT immOiHandle;	/* IMM OI Handle */
	SaSelectionObjectT imm_sel_obj;	/* Selection object to wait for IMM events */
	NODE_ID node_id;	/* My CLM Node Id */
	SaUint64T cluster_view_num;	/* the current cluster membership view number */

	SaUint32T async_upd_cnt;	/* Async Update Count for Warmsync */
	NCS_PATRICIA_TREE nodes_db;	/* CLMS_NODE_DB storing information of Cluster Nodes & Clients
					 * and the client's cluster track subscription information.
					 * Indexed by Node Name.
					 */
	NCS_PATRICIA_TREE ee_lookup;	/* To lookup the CLMS_NODE_DB with ee_name as key,
					 * for PLM track callback.
					 */
	NCS_PATRICIA_TREE id_lookup;	/* To lookup the CLMS_NODE_DB with node_id as key,
					 * for saClmNodeGet() API.
					 */
	NCS_PATRICIA_TREE client_db;	/* Client DataBase */
	uint32_t curr_invid;
	uint32_t last_client_id;	/* Value of last client_id assigned */
#ifdef ENABLE_AIS_PLM
	SaPlmHandleT plm_hdl;	/* Handle obtained from PLM */
	SaPlmEntityGroupHandleT ent_group_hdl;	/* PLM Entity Group Handle */
#endif
	SaSelectionObjectT plm_sel_obj;	/* PLMSv selection object */
	SaNtfHandleT ntf_hdl;	/* Handled obtained from NTFSv */
	SaBoolT reg_with_plm;	/*plm present in system */
	SaBoolT rtu_pending; /* Global flag to determine a pending RTU update and the poll timeout */
	CLMA_DOWN_LIST *clma_down_list_head;	/* CLMA down reccords - Fix for Failover missed 
						   down events Processing */
	CLMA_DOWN_LIST *clma_down_list_tail;
	NODE_DOWN_LIST *node_down_list_head;	/*NODE_DOWN record - Fix when active node goes down */
	NODE_DOWN_LIST *node_down_list_tail;
	bool is_impl_set;
} CLMS_CB;

typedef struct clms_lock_tmr_t {
	SaNameT node_name;
} CLMS_LOCK_TMR;

uint32_t clm_snd_track_changes(CLMS_CB * cb, CLMS_CLUSTER_NODE * node, CLMS_CLIENT_INFO * client,
				     SaImmAdminOperationIdT opId, SaClmChangeStepT step);
void clms_track_send_node_down(CLMS_CLUSTER_NODE * node);
void clms_reboot_remote_node(CLMS_CLUSTER_NODE * op_node, char *str);
void * clms_rem_reboot(void * _rem_reboot); 
#define m_CLMSV_PACK_INV(inv, nodeid) ((((SaUint64T) inv) << 32) | nodeid)
#define  m_CLMSV_INV_UNPACK_INVID(inv) ((inv) >> 32)
#define  m_CLMSV_INV_UNPACK_NODEID(inv) ((inv) & 0x00000000ffffffff)

#endif
