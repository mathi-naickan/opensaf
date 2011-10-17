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
 * Author(s): Ericsson AB
 *
 */

#ifndef IMMD_CB_H
#define IMMD_CB_H

#include <saClm.h>

#define IMMD_EVT_TIME_OUT 100
#define IMMSV_WAIT_TIME  100

#define m_IMMND_IS_ON_SCXB(m,n) ((m==n)?1:0)
#define m_IMMD_IS_LOCAL_NODE(m,n)   (m == n) ? 1 : 0
#define m_IMMND_NODE_ID_CMP(m,n) (m==n) ? 1 : 0

/* 30B Versioning Changes */
#define IMMD_MDS_PVT_SUBPART_VERSION 4
/* IMMD -IMMND communication */
#define IMMD_WRT_IMMND_SUBPART_VER_MIN 4
#define IMMD_WRT_IMMND_SUBPART_VER_MAX 4

#define IMMD_WRT_IMMND_SUBPART_VER_RANGE \
        (IMMD_WRT_IMMND_SUBPART_VER_MAX - \
         IMMD_WRT_IMMND_SUBPART_VER_MIN + 1 )

#define IMMSV_IMMD_MBCSV_VERSION_MIN 4
#define IMMSV_IMMD_MBCSV_VERSION 4

typedef struct immd_saved_fevs_msg {
	IMMSV_FEVS fevsMsg;
	NCS_BOOL re_sent;
	struct immd_saved_fevs_msg *next;
} IMMD_SAVED_FEVS_MSG;

typedef struct immd_immnd_info_node {
	NCS_PATRICIA_NODE patnode;
	MDS_DEST immnd_dest;
	NODE_ID immnd_key;

	/*ABT below corresponds to old ImmEvs::NodeInfo */
	int immnd_execPid;
	int epoch;
	NCS_BOOL syncRequested;
	NCS_BOOL isOnController;
	NCS_BOOL isCoord;
	NCS_BOOL syncStarted;
} IMMD_IMMND_INFO_NODE;

typedef struct immd_cb_tag {
	SYSF_MBX mbx;
	SaNameT comp_name;
	uns32 mds_handle;
	V_DEST_QA immd_anc;
	V_DEST_RL mds_role;	/* Current MDS role - ACTIVE/STANDBY */
	MDS_DEST immd_dest_id;

	NCS_MBCSV_HDL mbcsv_handle;	/* Needed for MBCKPT */
	SaSelectionObjectT mbcsv_sel_obj;	/* Needed for MBCKPT */
	NCS_MBCSV_CKPT_HDL o_ckpt_hdl;	/* Needed for MBCKPT */

	uns32 immd_sync_cnt;	//ABT 32 bit => wrapparround!!

	uns32 immd_self_id;
	uns32 immd_remote_id;
	NCS_BOOL immd_remote_up; //Ticket #1819

	NCS_NODE_ID node_id;

	NCS_BOOL is_loc_immnd_up;
	NCS_BOOL is_rem_immnd_up;
	NCS_BOOL is_quiesced_set;	/* ABT new csi_set */
	MDS_DEST loc_immnd_dest;
	MDS_DEST rem_immnd_dest;	/*ABT used if local immnd crashes ? */

	NCS_PATRICIA_TREE immnd_tree;	/*ABT <- message count in each node? */
	NCS_BOOL is_immnd_tree_up;	/* if TRUE immnd_tree is UP */

	SaAmfHandleT amf_hdl;	/* AMF handle, obtained thru AMF init */
	SaClmHandleT clm_hdl;

	SaAmfHAStateT ha_state;	/* present AMF HA state of the component */
	EDU_HDL edu_hdl;	/* EDU Handle obscurely needed by mds */

	SaInvocationT amf_invocation;

	/* IMM specific stuff. */
	SaUint32T admo_id_count;	//Global counter for AdminOwner ID 
	SaUint32T ccb_id_count;	//Global counter for CCB ID
	SaUint32T impl_count;	//Global counter for implementer id's
	SaUint64T fevsSendCount;	//Global counter for FEVS messages 
	/* Also maintain a quarantine of the latest N messages sent. */
	/* This to be able to re-send the messages on request. */
	/* Periodic Acks on the max-no received from each ND. */

	SaUint32T mRulingEpoch;
	uns8 mExpectedNodes;	//Hint on number of nodes in cluster
	uns8 mWaitSecs;		//Max time to wait for mExpectedNodes
	/* to join in cluster start. */
	NCS_NODE_ID immnd_coord;	//The nodeid of the current IMMND Coord
	NCS_SEL_OBJ usr1_sel_obj;	/* Selection object for USR1 signal events */
	SaSelectionObjectT amf_sel_obj;	/* Selection Object for AMF events */

	IMMD_SAVED_FEVS_MSG *saved_msgs;

	SaImmRepositoryInitModeT mRim; /* Should be the rim obtained from coord. */
} IMMD_CB;

EXTERN_C uns32 immd_immnd_info_tree_init(IMMD_CB *cb);

EXTERN_C uns32 immd_immnd_info_node_get(NCS_PATRICIA_TREE *immnd_tree,
					MDS_DEST *dest, IMMD_IMMND_INFO_NODE **immnd_info_node);

EXTERN_C void immd_immnd_info_node_getnext(NCS_PATRICIA_TREE *immnd_tree,
					   MDS_DEST *dest, IMMD_IMMND_INFO_NODE **immnd_info_node);

/*EXTERN_C uns32 immd_immnd_info_node_add(NCS_PATRICIA_TREE *immnd_tree, IMMD_IMMND_INFO_NODE *immnd_info_node);*/

EXTERN_C uns32 immd_immnd_info_node_delete(IMMD_CB *cb, IMMD_IMMND_INFO_NODE *immnd_info_node);

EXTERN_C void immd_immnd_info_tree_cleanup(IMMD_CB *cb);

EXTERN_C void immd_immnd_info_tree_destroy(IMMD_CB *cb);

EXTERN_C uns32 immd_immnd_info_node_find_add(NCS_PATRICIA_TREE *immnd_tree,
					     MDS_DEST *dest, IMMD_IMMND_INFO_NODE **immnd_info_node,
					     NCS_BOOL *add_flag);

EXTERN_C uns32 immd_cb_db_init(IMMD_CB *cb);

EXTERN_C uns32 immd_cb_db_destroy(IMMD_CB *cb);

EXTERN_C void immd_clm_cluster_track_cb(const SaClmClusterNotificationBufferT *notificationBuffer,
					SaUint32T numberOfMembers, SaAisErrorT error);

EXTERN_C uns32 immd_mds_change_role(IMMD_CB *cb);

EXTERN_C void immd_proc_immd_reset(IMMD_CB *cb, NCS_BOOL active);

#endif
