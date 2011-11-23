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

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:

  This file contains the Enum and datastructure definations for events 
  exchanged between IMMSv components 

******************************************************************************/

#ifndef IMMSV_EVT_H
#define IMMSV_EVT_H

#include <rda_papi.h>
#include "immsv_evt_model.h"

#define IMMSV_LOADERNAME "IMMLOADER"

/*****************************************************************************
 * Event Type of IMMSV
 *****************************************************************************/
typedef enum immsv_evt_type {
	IMMSV_EVT_TYPE_IMMA = 1,
	IMMSV_EVT_TYPE_IMMND = 2,
	IMMSV_EVT_TYPE_IMMD = 3,
	IMMSV_EVT_TYPE_MAX
} IMMSV_EVT_TYPE;

/*****************************************************************************
 * Event Type of IMMA 
 *****************************************************************************/
typedef enum imma_evt_type {
	/* Locally generated events */
	IMMA_EVT_MDS_INFO = 1,	/* IMMND UP/DOWN Info */
	IMMA_EVT_TIME_OUT = 2,	/* Time out events at IMMA */

	/* Events from IMMND */
	IMMA_EVT_ND2A_IMM_INIT_RSP = 3,
	IMMA_EVT_ND2A_IMM_FINALIZE_RSP = 4,
	IMMA_EVT_ND2A_IMM_ADMINIT_RSP = 5,
	IMMA_EVT_ND2A_IMM_ADMOP = 6,	/*Admin-op OI callback */
	IMMA_EVT_ND2A_IMM_ERROR = 7,	/*Generic error reply */
	IMMA_EVT_ND2A_ADMOP_RSP = 8,	/*Response from AdminOp to OM client, normal call */
	IMMA_EVT_ND2A_CCBINIT_RSP = 9,	/*Response from for CcbInit */
	IMMA_EVT_ND2A_SEARCHINIT_RSP = 10,	/*Response from for SearchInit */
	IMMA_EVT_ND2A_SEARCHNEXT_RSP = 11,	/*Response from for SearchNext */
	IMMA_EVT_ND2A_SEARCH_REMOTE = 12,	/*Fetch pure runtime attributes. */
	IMMA_EVT_ND2A_CLASS_DESCR_GET_RSP = 13,	/*Response for SaImmOmClassDescriptionGet */
	IMMA_EVT_ND2A_IMPLSET_RSP = 14,	/*Response for saImmOiImplementerSet */
	IMMA_EVT_ND2A_OI_OBJ_CREATE_UC = 15,	/*OBJ CREATE UP-CALL. */
	IMMA_EVT_ND2A_OI_OBJ_DELETE_UC = 16,	/*OBJ DELETE UP-CALL. */
	IMMA_EVT_ND2A_OI_OBJ_MODIFY_UC = 17,	/*OBJ MODIFY UP-CALL. */
	IMMA_EVT_ND2A_OI_CCB_COMPLETED_UC = 18,	/*CCB COMPLETED UP-CALL. */
	IMMA_EVT_ND2A_OI_CCB_APPLY_UC = 19,	/*CCB APPLY UP-CALL. */
	IMMA_EVT_ND2A_OI_CCB_ABORT_UC = 20,	/*CCB ABORT UP-CALL. */

	IMMA_EVT_ND2A_IMM_SYNC_RSP = 21,

	IMMA_EVT_CB_DUMP = 22,
	IMMA_EVT_ND2A_IMM_RESURRECT_RSP = 23,
	IMMA_EVT_ND2A_PROC_STALE_CLIENTS = 24,
	IMMA_EVT_ND2A_IMM_PBE_ADMOP = 25,       /*Special PBE admop callback */
	IMMA_EVT_MAX
} IMMA_EVT_TYPE;

/*****************************************************************************
 * Event Type of IMMND 
 *****************************************************************************/
typedef enum immnd_evt_type {
	/* events from IMMA to IMMND */

	/* Locally generated events */
	IMMND_EVT_MDS_INFO = 1,	/* IMMA/IMMND/IMMD UP/DOWN Info */
	IMMND_EVT_TIME_OUT = 2,	/* Time out event */

	/* Events from IMMA */
	IMMND_EVT_A2ND_IMM_INIT = 3,	/* ImmOm Initialization */
	IMMND_EVT_A2ND_IMM_OI_INIT = 4,	/* ImmOi Initialization */
	IMMND_EVT_A2ND_IMM_FINALIZE = 5,	/* ImmOm finalization */
	IMMND_EVT_A2ND_IMM_OI_FINALIZE = 6,	/* ImmOi finalization */
	IMMND_EVT_A2ND_IMM_ADMINIT = 7,	/* AdminOwnerInitialize */
	IMMND_EVT_A2ND_ADMO_FINALIZE = 8,	/* AdminOwnerFinalize */
	IMMND_EVT_A2ND_ADMO_SET = 9,	/* AdminOwnerSet */
	IMMND_EVT_A2ND_ADMO_RELEASE = 10,	/* AdminOwnerRelease */
	IMMND_EVT_A2ND_ADMO_CLEAR = 11,	/* AdminOwnerClear */
	IMMND_EVT_A2ND_IMM_ADMOP = 12,	/* Syncronous AdminOp */
	IMMND_EVT_A2ND_IMM_ADMOP_ASYNC = 13,	/* Asyncronous AdminOp */
	IMMND_EVT_A2ND_IMM_FEVS = 14,	/* Fake EVS msg from Agent (forward) */
	IMMND_EVT_A2ND_CCBINIT = 15,	/* CcbInitialize */
	IMMND_EVT_A2ND_SEARCHINIT = 16,	/* SearchInitialize */
	IMMND_EVT_A2ND_SEARCHNEXT = 17,	/* SearchNext */
	IMMND_EVT_A2ND_SEARCHFINALIZE = 18,	/* SearchFinalize */
	IMMND_EVT_A2ND_SEARCH_REMOTE = 19,	/* forward fetch of pure rt attr vals */
	IMMND_EVT_A2ND_RT_ATT_UPPD_RSP = 20,	/* reply for fetch of pure rt attr vals */
	IMMND_EVT_A2ND_ADMOP_RSP = 21,	/* AdminOperation sync local Reply */
	IMMND_EVT_A2ND_ASYNC_ADMOP_RSP = 22,	/* AdminOperation async local Reply */
	IMMND_EVT_A2ND_CCB_COMPLETED_RSP = 23,	/* CcbCompleted local Reply */
	IMMND_EVT_A2ND_CCB_OBJ_CREATE_RSP = 24,	/*CcbObjCreate local Reply */
	IMMND_EVT_A2ND_CCB_OBJ_MODIFY_RSP = 25,	/*CcbObjModify local Reply */
	IMMND_EVT_A2ND_CCB_OBJ_DELETE_RSP = 26,	/*CcbObjDelete local Reply */
	IMMND_EVT_A2ND_CLASS_CREATE = 27,	/* saImmOmClassCreate */
	IMMND_EVT_A2ND_CLASS_DESCR_GET = 28,	/* saImmOmClassDescriptionGet */
	IMMND_EVT_A2ND_CLASS_DELETE = 29,	/* saImmOmClassDelete */
	IMMND_EVT_A2ND_OBJ_CREATE = 30,	/* saImmOmCcbObjectCreate */
	IMMND_EVT_A2ND_OBJ_MODIFY = 31,	/* saImmOmCcbObjectModify */
	IMMND_EVT_A2ND_OBJ_DELETE = 32,	/* saImmOmCcbObjectDelete */
	IMMND_EVT_A2ND_CCB_APPLY = 33,	/* saImmOmCcbApply */
	IMMND_EVT_A2ND_CCB_FINALIZE = 34,	/* saImmOmCcbFinalize */
	IMMND_EVT_A2ND_OBJ_SYNC = 35,	/* immsv_sync */
	IMMND_EVT_A2ND_SYNC_FINALIZE = 36,	/* immsv_finalize_sync */

	IMMND_EVT_A2ND_OI_OBJ_CREATE = 37,	/* saImmOiRtObjectCreate */
	IMMND_EVT_A2ND_OI_OBJ_MODIFY = 38,	/* saImmOiRtObjectUpdate */
	IMMND_EVT_A2ND_OI_OBJ_DELETE = 39,	/* saImmOiRtObjectDelete */
	IMMND_EVT_A2ND_OI_IMPL_SET = 40,	/* saImmOiImplementerSet */
	IMMND_EVT_A2ND_OI_IMPL_CLR = 41,	/* saImmOiImplementerClear */
	IMMND_EVT_A2ND_OI_CL_IMPL_SET = 42,	/* saImmOiClassImplementerSet */
	IMMND_EVT_A2ND_OI_CL_IMPL_REL = 43,	/* saImmOiClassImplementerRelease */
	IMMND_EVT_A2ND_OI_OBJ_IMPL_SET = 44,	/* saImmOiObjectImplementerSet */
	IMMND_EVT_A2ND_OI_OBJ_IMPL_REL = 45,	/* saImmOiObjectImplementerRelease */

	IMMND_EVT_ND2ND_ADMOP_RSP = 46,	/* AdminOperation sync fevs Reply */
	IMMND_EVT_ND2ND_ASYNC_ADMOP_RSP = 47,	/* AdminOperation async fevs Reply */
	IMMND_EVT_ND2ND_SYNC_FINALIZE = 48,	/* Sync finalize from coord over fevs */
	IMMND_EVT_ND2ND_SEARCH_REMOTE = 49,	/* Forward of search request ro impl ND */
	IMMND_EVT_ND2ND_SEARCH_REMOTE_RSP = 50,	/* Forward of search request ro impl ND */

	/* Events from IMMD to IMMND */
	IMMND_EVT_D2ND_INTRO_RSP = 51,
	IMMND_EVT_D2ND_SYNC_REQ = 52,
	IMMND_EVT_D2ND_LOADING_OK = 53,
	IMMND_EVT_D2ND_SYNC_START = 54,
	IMMND_EVT_D2ND_SYNC_ABORT = 55,
	IMMND_EVT_D2ND_DUMP_OK = 56,
	IMMND_EVT_D2ND_GLOB_FEVS_REQ = 57,	/* Fake EVS msg from director (consume) */
	IMMND_EVT_D2ND_ADMINIT = 58,	/* Admin Owner init reply */
	IMMND_EVT_D2ND_CCBINIT = 59,	/* Ccb init reply */
	IMMND_EVT_D2ND_IMPLSET_RSP = 60,	/* Implementer set reply from D with impl id */
	IMMND_EVT_D2ND_DISCARD_IMPL = 61,	/* Discard implementer broadcast to NDs */
	IMMND_EVT_D2ND_ABORT_CCB = 62,	/* Abort CCB broadcast to NDs */
	IMMND_EVT_D2ND_ADMO_HARD_FINALIZE = 63,	/* Admo hard finalize broadcast to NDs */
	IMMND_EVT_D2ND_DISCARD_NODE = 64,	/* Crashed IMMND or node, broadcast to NDs */
	IMMND_EVT_D2ND_RESET = 65,	/* Force all IMMNDs to start from scratch. */

	IMMND_EVT_CB_DUMP = 66,

	IMMND_EVT_A2ND_IMM_OM_RESURRECT = 67,	/* Request resurrect of OM handle */
	IMMND_EVT_A2ND_IMM_OI_RESURRECT = 68,	/* Request resurrect of OI handle */
	IMMND_EVT_A2ND_IMM_CLIENTHIGH = 69,  /* Highest client id IMMA knows */
	IMMND_EVT_ND2ND_SYNC_FINALIZE_2 = 70,	/* Sync finalize from coord over fevs version 2*/
	IMMND_EVT_A2ND_RECOVER_CCB_OUTCOME = 71,/* Fetch ccb outcome OK/FAILED_OP given ccb-id. */
	IMMND_EVT_A2ND_PBE_PRT_OBJ_CREATE_RSP = 72,/* Response on PBE OI PRT OBJ CREATE. */
	IMMND_EVT_D2ND_PBE_PRTO_PURGE_MUTATIONS = 73,/* Purge all unack'ed PRT OBJ changes */
	IMMND_EVT_A2ND_PBE_PRTO_DELETES_COMPLETED_RSP = 74,/*Resp on PBE OI PRTO deletes */
	IMMND_EVT_A2ND_PBE_PRT_ATTR_UPDATE_RSP = 75,/* Response on PBE OI PRT OBJ CREATE. */
	IMMND_EVT_A2ND_IMM_OM_CLIENTHIGH = 76,  /* Highest client id IMMA knows */
	IMMND_EVT_A2ND_IMM_OI_CLIENTHIGH = 77,  /* Highest client id IMMA knows */
	IMMND_EVT_A2ND_PBE_ADMOP_RSP = 78,      /* PBE AdminOperation result FEVS reply */
	IMMND_EVT_MAX
} IMMND_EVT_TYPE;
/* Make sure the string array in immsv_evt.c matches the IMMND_EVT_TYPE enum. */

/*****************************************************************************
 * Event Types of IMMD 
 *****************************************************************************/
typedef enum immd_evt_type {
	/* Locally generated Events */
	IMMD_EVT_MDS_INFO = 1,

	/* Events from IMMND */
	IMMD_EVT_ND2D_INTRO = 2,	/*Identification message from IMMND to IMMD. */
	IMMD_EVT_ND2D_ANNOUNCE_LOADING = 3,	/*Coordinator attempts start of load. */
	IMMD_EVT_ND2D_REQ_SYNC = 4,	/*Straggler requests sync.  */
	IMMD_EVT_ND2D_ANNOUNCE_DUMP = 5,	/*Dump/backup invoked */
	IMMD_EVT_ND2D_SYNC_START = 6,	/*Coordinator wants to start sync. */
	IMMD_EVT_ND2D_SYNC_ABORT = 7,	/*Coordinator wants to start sync. */
	IMMD_EVT_ND2D_ADMINIT_REQ = 8,	/* AdminOwnerInitialize */
	IMMD_EVT_ND2D_ACTIVE_SET = 9,	/*ABT may need this or something similar */
	IMMD_EVT_ND2D_IMM_SYNC_INFO = 10,	/*May need this for Director<->standby sync */
	IMMD_EVT_ND2D_FEVS_REQ = 11,	/*Fake EVS over Director. */
	IMMD_EVT_ND2D_CCBINIT_REQ = 12,	/* CcbInitialize */
	IMMD_EVT_ND2D_IMPLSET_REQ = 13,	/*OiImplementerSet */
	IMMD_EVT_ND2D_OI_OBJ_MODIFY = 14,	/*saImmOiRtObjectUpdate */
	IMMD_EVT_ND2D_DISCARD_IMPL = 15,	/*Internal discard implemener message */
	IMMD_EVT_ND2D_ABORT_CCB = 16,	/*Broadcast attempt to abort ccb. */
	IMMD_EVT_ND2D_ADMO_HARD_FINALIZE = 17,	/* Broadcast hard admo finalize */

	IMMD_EVT_CB_DUMP = 18,
	IMMD_EVT_MDS_QUIESCED_ACK_RSP = 19,
	IMMD_EVT_LGA_CB = 20,

	IMMD_EVT_ND2D_PBE_PRTO_PURGE_MUTATIONS = 21, /* Broadcast for cleanup*/

	IMMD_EVT_ND2D_LOADING_FAILED = 22, /* Loading failed. */

	IMMD_EVT_MAX
} IMMD_EVT_TYPE;
/* Make sure the string array in immsv_evt.c matches the IMMD_EVT_TYPE enum. */

/*****************************************************************************
 Common Structs used in events 
 *****************************************************************************/

/* Struct used for passing only error response message */
typedef struct immsv_saerr_info {
	SaAisErrorT error;
} IMMSV_SAERR_INFO;

/* Structure for passing MDS info to components */
typedef struct immsv_mds_info {
	NCSMDS_CHG change;	/* GONE, UP, DOWN, CHG ROLE  */
	MDS_DEST dest;
	MDS_SVC_ID svc_id;
	NODE_ID node_id;
	V_DEST_RL role;
} IMMSV_MDS_INFO;

typedef struct immsv_send_info {
	MDS_SVC_ID to_svc;	/* The service at the destination */
	MDS_DEST dest;		/* Who to send */
	MDS_SENDTYPES stype;	/* Send type */
	MDS_SYNC_SND_CTXT ctxt;	/* MDS Opaque context */
	uint8_t mSynReqCount;
} IMMSV_SEND_INFO;

typedef struct immsv_fevs {
	SaUint64T sender_count;
	MDS_DEST reply_dest;	//The dest of the ND, which NDs should
	/* reply to (implementer&client  */
	/* on different nodes). */
	/* If zeroed, then reply to all or none. */
	SaImmHandleT client_hdl;	//Needed for aborting callbacks
	/* Holds nodeId and connection */
	IMMSV_OCTET_STRING msg;
} IMMSV_FEVS;

/****************************************************************************
 Requests IMMA --> IMMND 
 ****************************************************************************/
typedef struct immsv_init_req {
	SaVersionT version;
	SaUint32T client_pid;
} IMMSV_INIT_REQ;

typedef struct immsv_finalize_req {
	SaImmHandleT client_hdl;
} IMMSV_FINALIZE_REQ;

typedef struct immsv_a2nd_adminit_req {
	SaImmHandleT client_hdl;	//ND needs for callbacks
	IMMSV_OM_ADMIN_OWNER_INITIALIZE i;
	/* 
	   SaInvocationT                          invocation;       
	   SaTimeT                                timeout; 
	 */
} IMMSV_A2ND_ADMINIT_REQ;

typedef struct immsv_a2nd_admown_finalize {
	SaUint32T adm_owner_id;
} IMMSV_A2ND_ADMOWN_FINALIZE;

typedef struct immsv_a2nd_admown_set {
	/*SaImmAdminOwnerHandleT             adm_owner_id; */
	SaUint32T adm_owner_id;
	SaUint32T scope;
	IMMSV_OBJ_NAME_LIST *objectNames;
} IMMSV_A2ND_ADMOWN_SET;

typedef struct immsv_a2nd_search_op {
	SaImmHandleT client_hdl;
	SaUint32T searchId;
} IMMSV_A2ND_SEARCH_OP;

typedef struct immsv_oi_admin_op_rsp {
	SaImmOiHandleT oi_client_hdl;
	SaInvocationT invocation;	//Negative => async invocation
	SaAisErrorT result;
	SaAisErrorT error;
} IMMSV_OI_ADMIN_OP_RSP;

typedef struct immsv_oi_search_remote_rsp {
	IMMSV_OM_SEARCH_REMOTE sr;
	SaAisErrorT result;
} IMMSV_OI_SEARCH_REMOTE_RSP;

/****************************************************************************
 Resp to Requests IMMND --> IMMA
 ****************************************************************************/

/* OM Init Response */
typedef struct immsv_nd2a_init_rsp {
	SaImmHandleT immHandle;
	SaAisErrorT error;
} IMMSV_ND2A_INIT_RSP;

/* AdminOwnerInit Response */
typedef struct immsv_nd2a_adminit_rsp {
	SaAisErrorT error;
	SaUint32T ownerId;
} IMMSV_ND2A_ADMINIT_RSP;

/* CcbInit Response */
typedef struct immsv_nd2a_ccbinit_rsp {
	SaAisErrorT error;
	SaUint32T ccbId;
} IMMSV_ND2A_CCBINIT_RSP;

/* SearchInit Response */
typedef struct immsv_nd2a_searchinit_rsp {
	SaAisErrorT error;
	SaUint32T searchId;
} IMMSV_ND2A_SEARCHINIT_RSP;

/* ImplementerSet Response */
typedef struct immsv_nd2a_impl_set_rsp {
	SaAisErrorT error;
	SaUint32T implId;
} IMMSV_ND2A_IMPLSET_RSP;

/****************************************************************************
 IMMD --> IMMND
 ****************************************************************************/

/*Reply from central director to node director on admin owner initialize ABT */
typedef struct immsv_d2nd_adminit {
	SaUint32T globalOwnerId;
	IMMSV_OM_ADMIN_OWNER_INITIALIZE i;
} IMMSV_D2ND_ADMINIT;

typedef struct immsv_d2nd_ccbinit {
	SaUint32T globalCcbId;
	IMMSV_OM_CCB_INITIALIZE i;
} IMMSV_D2ND_CCBINIT;

typedef struct immsv_d2nd_control {
	SaUint32T nodeId;
	SaUint32T rulingEpoch;
	SaUint64T fevsMsgStart;
	SaUint32T ndExecPid;
	uns8 canBeCoord;
	uns8 isCoord;
	uns8 syncStarted;
	SaUint32T nodeEpoch;
	uns8 pbeEnabled;
} IMMSV_D2ND_CONTROL;

/****************************************************************************
 IMMND --> IMMD
 ****************************************************************************/

typedef struct immsv_nd2d_control {
	SaUint32T ndExecPid;
	SaUint32T epoch;
	uns8 refresh;		//TRUE=> This is a refresh of epoch.
	uns8 pbeEnabled;
} IMMSV_ND2D_CONTROL;

typedef struct immsv_nd2d_adminit_req {
	SaImmHandleT client_hdl;
	IMMSV_OM_ADMIN_OWNER_INITIALIZE i;
} IMMSV_ND2D_ADMINIT_REQ;

typedef struct immsv_nd2d_implset_req {
	IMMSV_OI_IMPLSET_REQ r;
	MDS_DEST reply_dest;	/*The dest of the originating ND */
} IMMSV_ND2D_IMPLSET_REQ;

/* IMMA Local Events */
typedef struct imma_tmr_info {
	uns32 type;
	SaImmAdminOwnerHandleT adm_owner_hdl;
	SaImmHandleT client_hdl;
	SaInvocationT invocation;
} IMMA_TMR_INFO;

/******************************************************************************
 IMMA Event Data Struct 
 ******************************************************************************/
/* BUGBUG ABT: All commented out members are my doing. */
typedef struct imma_evt {
	IMMA_EVT_TYPE type;
	union {
		IMMSV_ND2A_INIT_RSP initRsp;
		IMMSV_SAERR_INFO errRsp;
		IMMSV_ND2A_ADMINIT_RSP admInitRsp;
		IMMSV_ND2A_CCBINIT_RSP ccbInitRsp;
		IMMSV_ND2A_SEARCHINIT_RSP searchInitRsp;
		IMMSV_OM_RSP_SEARCH_NEXT *searchNextRsp;
		IMMSV_OM_SEARCH_REMOTE searchRemote;
		IMMSV_OM_ADMIN_OP_INVOKE admOpReq;	//For the OI callback.
		IMMSV_OI_ADMIN_OP_RSP admOpRsp;
		IMMSV_OM_CCB_OBJECT_CREATE objCreate;	//callback
		IMMSV_OM_CCB_OBJECT_DELETE objDelete;	//callback
		IMMSV_OM_CCB_OBJECT_MODIFY objModify;	//callback
		IMMSV_OM_CCB_COMPLETED ccbCompl;
		IMMSV_OM_CLASS_DESCR classDescr;
		IMMSV_ND2A_IMPLSET_RSP implSetRsp;
		/* IMMSV_IMM_STATUS       status; */

		/* IMMSV_ND2A_SYNC_RSP         sync_rsp; */
		/* IMMSV_SAERR_INFO            readackRsp; */
		IMMA_TMR_INFO tmr_info;
	} info;

} IMMA_EVT;

/******************************************************************************
 IMMND Event Data Structures
 ******************************************************************************/
typedef struct immnd_evt {
	NCS_BOOL dont_free_me;
	SaAisErrorT error;
	IMMND_EVT_TYPE type;
	union {
		/* IMMA --> IMMND */
		IMMSV_INIT_REQ initReq;
		IMMSV_FINALIZE_REQ finReq;
		IMMSV_A2ND_ADMINIT_REQ adminitReq;
		IMMSV_OM_CCB_INITIALIZE ccbinitReq;
		IMMSV_OI_IMPLSET_REQ implSet;

		IMMSV_A2ND_ADMOWN_FINALIZE admFinReq;
		IMMSV_A2ND_ADMOWN_SET admReq;

		IMMSV_OM_ADMIN_OP_INVOKE admOpReq;
		IMMSV_FEVS fevsReq;	//Prepacked message "virtual EVS"

		IMMSV_OI_ADMIN_OP_RSP admOpRsp;
		IMMSV_OI_CCB_UPCALL_RSP ccbUpcallRsp;

		IMMSV_OM_CLASS_DESCR classDescr;

		IMMSV_OM_CCB_OBJECT_CREATE objCreate;
		IMMSV_OM_CCB_OBJECT_MODIFY objModify;
		IMMSV_OM_CCB_OBJECT_DELETE objDelete;
		IMMSV_OM_OBJECT_SYNC obj_sync;
		IMMSV_OM_FINALIZE_SYNC finSync;

		SaUint32T ccbId;	//CcbApply, CcbFinalize, CCbAbort
		IMMSV_A2ND_SEARCH_OP searchOp;	//SearchNext, SearchFinalize
		IMMSV_OM_SEARCH_INIT searchInit;
		IMMSV_OI_SEARCH_REMOTE_RSP rtAttUpdRpl;
		IMMSV_OM_SEARCH_REMOTE searchRemote;
		IMMSV_OM_RSP_SEARCH_REMOTE rspSrchRmte;

		/* IMMD --> IMMND */
		IMMSV_D2ND_CONTROL ctrl;
		IMMSV_D2ND_ADMINIT adminitGlobal;
		IMMSV_D2ND_CCBINIT ccbinitGlobal;

		/* Locally generated events */
		IMMSV_MDS_INFO mds_info;
	} info;
} IMMND_EVT;

/* IMMD Local Events */
typedef struct immd_tmr_info {
	uns32 type;
	union {
		MDS_DEST immnd_dest;
	} info;
} IMMD_TMR_INFO;

typedef struct {
	PCS_RDA_ROLE io_role;
} IMMSV_RDA_INFO;

/******************************************************************************
 IMMD Event Data Structures
 ******************************************************************************/
typedef struct immd_evt {
	IMMD_EVT_TYPE type;
	union {
		/* IMMND --> IMMD */
		IMMSV_ND2D_CONTROL ctrl_msg;
		IMMSV_ND2D_ADMINIT_REQ admown_init;
		IMMSV_OM_CCB_INITIALIZE ccb_init;
		IMMSV_ND2D_IMPLSET_REQ impl_set;
		IMMSV_OM_CCB_OBJECT_MODIFY objModify;	/* Actually OI runtime obj update */
		SaUint32T ccbId;	/*For CcbAbort */
		SaUint32T admoId;	/*For admo ahrd finalize */

		IMMSV_FEVS fevsReq;	//Prepacked message "virtual EVS"

		IMMD_TMR_INFO tmr_info;
		IMMSV_MDS_INFO mds_info;
		IMMSV_RDA_INFO rda_info;
	} info;
} IMMD_EVT;

/******************************************************************************
 IMMSV Event Data Struct 
 ******************************************************************************/
typedef struct immsv_evt {
	struct immsv_evt *next;
	IMMSV_EVT_TYPE type;
	union {
		IMMA_EVT imma;
		IMMND_EVT immnd;
		IMMD_EVT immd;
	} info;
	IMMSV_SEND_INFO sinfo;	/* MDS Sender information */
} IMMSV_EVT;

/* Event Declerations */

EXTERN_C uns32 immsv_evt_enc_flat( /*EDU_HDL *edu_hdl, */ IMMSV_EVT *i_evt,
				  NCS_UBAID *o_ub);
EXTERN_C uns32 immsv_evt_dec_flat( /*EDU_HDL *edu_hdl, */ NCS_UBAID *i_ub,
				  IMMSV_EVT *o_evt);
EXTERN_C uns32 immsv_evt_enc( /*EDU_HDL *edu_hdl, */ IMMSV_EVT *i_evt,
			     NCS_UBAID *o_ub);
EXTERN_C uns32 immsv_evt_dec( /*EDU_HDL *edu_hdl, */ NCS_UBAID *i_ub,
			     IMMSV_EVT *o_evt);

EXTERN_C void immsv_evt_enc_inline_string(NCS_UBAID *o_ub, IMMSV_OCTET_STRING *os);

EXTERN_C void immsv_evt_dec_inline_string(NCS_UBAID *i_ub, IMMSV_OCTET_STRING *os);

EXTERN_C void immsv_evt_free_att_val(IMMSV_EDU_ATTR_VAL *v, SaImmValueTypeT t);

EXTERN_C void immsv_msg_trace_send(MDS_DEST to, IMMSV_EVT *evt);
EXTERN_C void immsv_msg_trace_rec(MDS_DEST from, IMMSV_EVT *evt);

EXTERN_C void immsv_free_attrmods(IMMSV_ATTR_MODS_LIST *p);
EXTERN_C void immsv_evt_free_admo(IMMSV_ADMO_LIST *p);
EXTERN_C void immsv_evt_free_impl(IMMSV_IMPL_LIST *p);
EXTERN_C void immsv_evt_free_classList(IMMSV_CLASS_LIST *p);
EXTERN_C void immsv_evt_free_attrNames(IMMSV_ATTR_NAME_LIST *p);
EXTERN_C void immsv_free_attrvalues_list(IMMSV_ATTR_VALUES_LIST *avl);
EXTERN_C void immsv_free_attrdefs_list(IMMSV_ATTR_DEF_LIST *adp);
EXTERN_C void immsv_evt_free_name_list(IMMSV_OBJ_NAME_LIST *p);
EXTERN_C void immsv_evt_free_ccbOutcomeList(IMMSV_CCB_OUTCOME_LIST *o);

/* Macros to pack and unpack imm handles */
#define m_IMMSV_PACK_HANDLE(high, low) ((((SaUint64T) high) << 32) | ((SaUint32T) low))
#define  m_IMMSV_UNPACK_HANDLE_HIGH(imm_handle) (SaUint32T) ((imm_handle) >> 32)
#define  m_IMMSV_UNPACK_HANDLE_LOW(imm_handle) (SaUint32T) ((imm_handle) & 0x00000000ffffffff)

#endif
