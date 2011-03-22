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

#ifndef IMMND_CB_H
#define IMMND_CB_H

#include <saClm.h>

typedef enum immnd_server_state {
	IMM_SERVER_UNKNOWN = 0,
	IMM_SERVER_ANONYMOUS = 1,
	IMM_SERVER_CLUSTER_WAITING = 2,
	IMM_SERVER_LOADING_PENDING = 3,
	IMM_SERVER_LOADING_SERVER = 4,
	IMM_SERVER_LOADING_CLIENT = 5,
	IMM_SERVER_SYNC_PENDING = 6,
	IMM_SERVER_SYNC_CLIENT = 7,
	IMM_SERVER_SYNC_SERVER = 8,
	IMM_SERVER_DUMP = 9,	//Possibly needed to prevent conflicting ops?
	IMM_SERVER_READY = 10
} IMMND_SERVER_STATE;

/*****************************************************************************
 Client  information 
*****************************************************************************/
typedef struct immnd_om_search_node {
	SaUint32T searchId;
	void *searchOp;
	struct immnd_om_search_node *next;
} IMMND_OM_SEARCH_NODE;

typedef struct immnd_immom_client_node {
	NCS_PATRICIA_NODE patnode;
	SaImmHandleT imm_app_hdl;	/* index for the client tree */
	MDS_DEST agent_mds_dest;	/* mds dest of the agent */
	SaVersionT version;
	SaUint32T client_pid;	/*Used to recognize loader */
	IMMSV_SEND_INFO tmpSinfo;	/*needed for replying to 
					   syncronousrequests */

	NCSMDS_SVC_ID sv_id;	/* OM or OI */
	IMMND_OM_SEARCH_NODE *searchOpList;
	uns8 mIsSync;		/* Client is special sync client */
	uns8 mIsPbe;            /* Client is persistent back end */
	uns8 mSyncBlocked;	/* Sync client expects reply */
	uns8 mIsStale;		/* Client disconnected when IMMD
				   is unavailable => postpone 
				   discardClient. */
	uns8 mIsResurrect;      /* The client is a temprary place holder
                                  for handle resurrect. We use it to send
				  the PROC_STALE_CLIENT internal upcall to
				  the IMMA clients that are dispatching.
				  This is done when immnd sync has completed.
				  The tmp client is then removed, anticipating
				  a resurrect request by the IMMA.
                               */
} IMMND_IMM_CLIENT_NODE;

/******************************************************************************
 Used to maintain the linked list of fevs messages received out of order
*****************************************************************************/

typedef struct immnd_fevs_msg_node {
	SaUint64T msgNo;
	SaImmHandleT clnt_hdl;
	MDS_DEST reply_dest;
	IMMSV_OCTET_STRING msg;
	struct immnd_fevs_msg_node *next;
} IMMND_FEVS_MSG_NODE;

/*****************************************************************************
 * Data Structure used to hold IMMND control block
 *****************************************************************************/
typedef struct immnd_cb_tag {
	SYSF_MBX immnd_mbx;	/* mailbox */
	SaNameT comp_name;
	uns32 immnd_mds_hdl;
	MDS_DEST immnd_mdest_id;
	NCS_NODE_ID node_id;

	/*Nr of FEVS messages sent, but not received back at origin.*/
	uns8 fevs_replies_pending; 

	SaUint32T cli_id_gen;	/* for generating client_id */

	SaUint64T highestProcessed;	//highest fevs msg processed so far.
	SaUint64T highestReceived;	//highest fevs msg received so far 
	SaUint64T syncFevsBase;	        //Last fevsMessage before sync iterator.
	IMMND_FEVS_MSG_NODE *fevs_in_list;  //incomming queue
	IMMND_FEVS_MSG_NODE *fevs_out_list; //outgoing queue
	IMMND_FEVS_MSG_NODE *fevs_out_list_end; //end outgoing queue
	SaUint32T           fevs_out_count; //Nrof elements in outgoing queue

	void *immModel;
	SaUint32T mMyEpoch;	//Epoch counter, used in synch of immnds
	SaUint32T mMyPid;	//Is this needed ??
	SaUint32T mRulingEpoch;
	uns8 mAccepted;		//Should all fevs messages be processed?
	uns8 mIntroduced;	//Ack received on introduce message
	uns8 mSyncRequested;	//true=> I am coord, other req sync
	uns8 mPendSync;		//1=>sync announced but not received.
	uns8 mSyncFinalizing;   //1=>finalizeSync sent but not received.
	uns8 mSync;		//true => this node is being synced (client).
	uns8 mCanBeCoord;
	uns8 mIsCoord;
	uns8 mLostNodes;       //Detached & not syncreq => delay sync start
	uns8 mBlockPbeEnable;  //Current PBE has not completed shutdown yet.

	/* Information about the IMMD */
	MDS_DEST immd_mdest_id;
	NCS_BOOL is_immd_up;

	/* IMMND data */
	NCS_PATRICIA_TREE client_info_db;	/* IMMND_IMM_CLIENT_NODE - node */

	SaClmHandleT clm_hdl;
	SaClmNodeIdT clm_node_id;
	SaAmfHandleT amf_hdl;	// AMF handle, obtained thru AMF init

	int32 loaderPid;
	int32 syncPid;
	int32 pbePid;   //Persistent back end (PBE) is running if pbePid > 0
	IMMND_SERVER_STATE mState;
	uns32 mStep;		//Measures progress in immnd_proc_server
	time_t mJobStart;       //Start time for major server tasks like start, load, sync.
	char *mProgName;	//The full path name of the immnd executable.
	const char *mDir;	//The directory where imm.xml & pbe files reside
	const char *mFile;	//The imm.xml file to start from
	const char *mPbeFile;   //Pbe feature is configured (IMMSV_PBE_FILE).
	SaImmRepositoryInitModeT mRim; 

	uns8 mExpectedNodes;
	uns8 mWaitSecs;
	uns8 mNumNodes;
	uns8 mPbeVeteran;       //false => generate. true => re-attach db-file

	SaAmfHAStateT ha_state;	// present AMF HA state of the component
	EDU_HDL immnd_edu_hdl;	// edu handle, obscurely needed by mds.
	NCSCONTEXT task_hdl;

	NCS_LOCK immnd_immd_up_lock;

	NCS_SEL_OBJ usr1_sel_obj;	/* Selection object for USR1 signal events */
	SaSelectionObjectT amf_sel_obj;	/* Selection Object for AMF events */
	int nid_started;	/* true if started by NID */
} IMMND_CB;

/* CB prototypes */
EXTERN_C IMMND_CB *immnd_cb_create(uns32 pool_id);
EXTERN_C NCS_BOOL immnd_cleanup_mbx(NCSCONTEXT arg, NCSCONTEXT msg);
EXTERN_C uns32 immnd_cb_destroy(IMMND_CB *immnd_cb);
EXTERN_C void immnd_dump_cb(IMMND_CB *immnd_cb);

EXTERN_C uns32 immnd_client_extract_bits(uns32 bitmap_value, uns32 *bit_position);

EXTERN_C void immnd_client_node_get(IMMND_CB *cb, SaImmHandleT imm_client_hdl, IMMND_IMM_CLIENT_NODE **imm_client_node);
EXTERN_C void immnd_client_node_getnext(IMMND_CB *cb,
					SaImmHandleT imm_client_hdl, IMMND_IMM_CLIENT_NODE **imm_client_node);
EXTERN_C uns32 immnd_client_node_add(IMMND_CB *cb, IMMND_IMM_CLIENT_NODE *cl_node);
EXTERN_C uns32 immnd_client_node_del(IMMND_CB *cb, IMMND_IMM_CLIENT_NODE *cl_node);
EXTERN_C uns32 immnd_client_node_tree_init(IMMND_CB *cb);
EXTERN_C void immnd_client_node_tree_cleanup(IMMND_CB *cb);
EXTERN_C void immnd_client_node_tree_destroy(IMMND_CB *cb);

/*
  #define m_IMMSV_CONVERT_EXPTIME_TEN_MILLI_SEC(t) \
  SaTimeT now; \
  t = (( (t) - (m_GET_TIME_STAMP(now)*(1000000000)))/(10000000));
*/

/*30B Versioning Changes */
#define IMMND_MDS_PVT_SUBPART_VERSION 1

/*IMMND - IMMA communication */
#define IMMND_WRT_IMMA_SUBPART_VER_MIN 1
#define IMMND_WRT_IMMA_SUBPART_VER_MAX 1

#define IMMND_WRT_IMMA_SUBPART_VER_RANGE \
        (IMMND_WRT_IMMA_SUBPART_VER_MAX - \
         IMMND_WRT_IMMA_SUBPART_VER_MIN + 1 )

/*IMMND - IMMND communication */
#define IMMND_WRT_IMMND_SUBPART_VER_MIN 1
#define IMMND_WRT_IMMND_SUBPART_VER_MAX 1

#define IMMND_WRT_IMMND_SUBPART_VER_RANGE \
        (IMMND_WRT_IMMND_SUBPART_VER_MAX - \
         IMMND_WRT_IMMND_SUBPART_VER_MIN + 1 )

/*IMMND - IMMD communication */
#define IMMND_WRT_IMMD_SUBPART_VER_MIN 1
#define IMMND_WRT_IMMD_SUBPART_VER_MAX 1

#define IMMND_WRT_IMMD_SUBPART_VER_RANGE \
        (IMMND_WRT_IMMD_SUBPART_VER_MAX - \
         IMMND_WRT_IMMD_SUBPART_VER_MIN + 1 )

#endif
