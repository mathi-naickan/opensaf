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

#ifndef IMMA_CB_H
#define IMMA_CB_H

/* Node to store Ccb info for OI client */
struct imma_oi_ccb_record {
	struct imma_oi_ccb_record *next;
	SaImmOiCcbIdT ccbId;  /* High order 32 bits used for PRTO 'pseudo ccbs'.*/
	uns8 isStale;    /* 1 => ccb was terminated by IMMND down. */
	uns8 isCritical; /* 1 => OI has replied OK on completed callback but not */
                         /*      received abort-callback or apply-callback.      */
        SaUint32T opCount; /* Used to ensure PBE has not missed any unacked ops. */
};

typedef struct imma_client_node {
	NCS_PATRICIA_NODE patnode;	/* index for the tree */
	SaImmHandleT handle;
	union {
		SaImmCallbacksT mCallbk;
		SaImmOiCallbacksT_2 iCallbk;
#ifdef IMM_A_01_01
		SaImmOiCallbacksT iCallbk1;
#endif
	} o;
	SaUint32T mImplementerId;	/*Only used for OI.*/
	SaBoolT isOm;		/*If true => then this is an OM client */
	SaImmOiImplementerNameT  mImplementerName; /* needed for active resurrect*/
#ifdef IMM_A_01_01
	SaBoolT isOiA1;
#endif
	uns8 stale;		/*Loss of connection with immnd 
					  will set this to true for the 
					  connection. A resurrect can remove it.*/
	uns8 exposed;    /* Exposed => stale is irreversible */
	uns8 selObjUsable; /* Active resurrect possible for this client */
	uns8 replyPending; /* Syncronous or asyncronous call made towards IMMND */
	uns8 isPbe;  /* True => This is the PBE-OI */
	struct imma_oi_ccb_record *activeOiCcbs; /* For ccb termination on IMMND down.*/
	SYSF_MBX callbk_mbx;	/*Mailbox Queue for clnt messages */
} IMMA_CLIENT_NODE;

/* Node to store adminOwner info */
typedef struct imma_admin_owner_node {
	NCS_PATRICIA_NODE patnode;	/* index for the tree */
	SaImmAdminOwnerHandleT admin_owner_hdl;	/* locally generated handle */
	SaImmHandleT mImmHandle;	/* The immOm handle */
	SaUint32T mAdminOwnerId;
	SaImmAdminOwnerNameT mAdminOwnerName; /* Needed for OM resurrect. */
	uns8 mReleaseOnFinalize; /* Release on finalize set, stale irreversible*/
} IMMA_ADMIN_OWNER_NODE;

/* Node to store Ccb info for OM client */
typedef struct imma_ccb_node {
	NCS_PATRICIA_NODE patnode;	/* index for the tree */
	SaImmCcbHandleT ccb_hdl;	/* locally generated handle */
	SaImmHandleT mImmHandle;	/* The immOm handle */
	SaImmAdminOwnerHandleT mAdminOwnerHdl;
	SaImmCcbFlagsT mCcbFlags;
	SaUint32T mCcbId;   /* Om client uses 32 bit ccbId. */
	uns8 mExclusive;   /* 1 => Ccb-id being created, applied or finalized */
	uns8 mApplying;    /* Critical (apply invoked), IMMND contact lost => 
						  timeout => Ccb-outcome to be recovered. */
	uns8 mApplied;     /* Current mCcbId applied&terminated */
	uns8 mAborted;     /* Current mCcbId aborted */
} IMMA_CCB_NODE;

/* Node to store Search info */
typedef struct imma_search_node {
	NCS_PATRICIA_NODE patnode;	/* index for the tree */
	SaImmSearchHandleT search_hdl;	/* locally generated handle */
	SaImmHandleT mImmHandle;	/* The immOm handle */
	SaUint32T mSearchId;
	void *mLastAttributes;	/* From previous searchNext */
} IMMA_SEARCH_NODE;

typedef struct imma_continuation_record {
	SaUint32T invocation;
	SaInvocationT userInvoc;
	SaImmHandleT immHandle;
	struct imma_continuation_record *next;
} IMMA_CONTINUATION_RECORD;

/*****************************************************************************
 * Data Structure Used to hold IMMA control block
 *****************************************************************************/
typedef struct imma_cb {
	/* Identification Information about the IMMA */
	uns32 imma_mds_hdl;
	MDS_DEST imma_mds_dest;
	NCSMDS_SVC_ID sv_id;
	NCS_LOCK cb_lock;
	uns32 pend_dis;		/* Number of pending dispaches */
	uns32 pend_fin;		/* Number of pending agent destroy */
	EDU_HDL edu_hdl;	/* edu handle obscurely needed by mds */

	/* Information about IMMND */
	MDS_DEST immnd_mds_dest;
	NCS_BOOL is_immnd_up;
	uns16    dispatch_clients_to_resurrect;  /* Nrof clients pending
						    active resurrect.  */

	/* IMMA data */	/* Used for both OM and OI */
	NCS_PATRICIA_TREE client_tree;	/* IMMA_CLIENT_NODE - node */

	/* These trees could theoretically be moved into the client node 
	   But the assumption is that the typical process has few connections
	   and few subhandles. By "few" we mean < 10.
	 */

	NCS_PATRICIA_TREE admin_owner_tree;	/* IMMA_ADMIN_OWNER_NODE  - node */

	NCS_PATRICIA_TREE ccb_tree;	/* IMMA_CCB_NODE  - node */

	NCS_PATRICIA_TREE search_tree;	/* IMMA_CCB_NODE  - node */

	/*Used for matching async reply to saImmOmAdminOperationInvokeAsync */
	IMMA_CONTINUATION_RECORD *imma_continuations;

	/* Sync up with IMMND ( MDS ) see imma_sync_with_immnd() in imma_init.c */
    NCS_LOCK             immnd_sync_lock;
    NCS_BOOL             immnd_sync_awaited;
    NCS_SEL_OBJ          immnd_sync_sel; 

} IMMA_CB;

#define m_IMMSV_SET_SANAMET(name) \
{if(name->length <= SA_MAX_NAME_LENGTH) \
{memset( (uns8 *)&name->value[name->length], 0, (SA_MAX_NAME_LENGTH - name->length) ); }\
}

/* IMMA Function Declerations */
/* function prototypes for client handling*/

EXTERN_C uns32 imma_db_init(IMMA_CB *cb);
EXTERN_C uns32 imma_db_destroy(IMMA_CB *cb);

/*client tree*/
EXTERN_C uns32 imma_client_tree_init(IMMA_CB *cb);
EXTERN_C void imma_client_node_get(NCS_PATRICIA_TREE *client_tree, SaImmHandleT *cl_hdl, IMMA_CLIENT_NODE **cl_node);
EXTERN_C uns32 imma_client_node_add(NCS_PATRICIA_TREE *client_tree, IMMA_CLIENT_NODE *cl_node);
EXTERN_C uns32 imma_client_node_delete(IMMA_CB *cb, IMMA_CLIENT_NODE *cl_node);
EXTERN_C void imma_client_tree_destroy(IMMA_CB *cb);
EXTERN_C void imma_client_tree_cleanup(IMMA_CB *cb);
EXTERN_C void imma_mark_clients_stale(IMMA_CB *cb);
EXTERN_C int  isExposed(IMMA_CB *cb, IMMA_CLIENT_NODE  *clnode);
void imma_oi_ccb_record_add(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId, SaUint32T inv);
int imma_oi_ccb_record_ok_for_critical(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId, SaUint32T inv);
int imma_oi_ccb_record_set_critical(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId, SaUint32T inv);
int imma_oi_ccb_record_terminate(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId);
int imma_oi_ccb_record_exists(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId);


/*admin_owner tree*/
EXTERN_C uns32 imma_admin_owner_tree_init(IMMA_CB *cb);
EXTERN_C uns32 imma_admin_owner_node_get(NCS_PATRICIA_TREE *admin_owner_tree,
					 SaImmAdminOwnerHandleT *adm_hdl, IMMA_ADMIN_OWNER_NODE **adm_node);
EXTERN_C void imma_admin_owner_node_getnext(IMMA_CB *cb,
					    SaImmAdminOwnerHandleT *adm_hdl, IMMA_ADMIN_OWNER_NODE **adm_node);
EXTERN_C uns32 imma_admin_owner_node_add(NCS_PATRICIA_TREE *admin_owner_tree, IMMA_ADMIN_OWNER_NODE *adm_node);
EXTERN_C void imma_admin_owner_node_delete(IMMA_CB *cb, IMMA_ADMIN_OWNER_NODE *adm_node);
EXTERN_C void imma_admin_owner_tree_destroy(IMMA_CB *cb);
EXTERN_C void imma_admin_owner_tree_cleanup(IMMA_CB *cb);

/*ccb tree */
EXTERN_C uns32 imma_ccb_tree_init(IMMA_CB *cb);
EXTERN_C void imma_ccb_node_get(NCS_PATRICIA_TREE *ccb_tree, SaImmCcbHandleT *ccb_hdl, IMMA_CCB_NODE **ccb_node);
EXTERN_C void imma_ccb_node_getnext(IMMA_CB *cb, SaImmCcbHandleT *ccb_hdl, IMMA_CCB_NODE **ccb_node);
EXTERN_C uns32 imma_ccb_node_add(NCS_PATRICIA_TREE *ccb_tree, IMMA_CCB_NODE *ccb_node);
EXTERN_C uns32 imma_ccb_node_delete(IMMA_CB *cb, IMMA_CCB_NODE *ccb_node);
EXTERN_C void imma_ccb_tree_destroy(IMMA_CB *cb);
EXTERN_C void imma_ccb_tree_cleanup(IMMA_CB *cb);

/*search tree */
EXTERN_C uns32 imma_search_tree_init(IMMA_CB *cb);
EXTERN_C uns32 imma_search_node_get(NCS_PATRICIA_TREE *search_tree,
				    SaImmSearchHandleT *search_hdl, IMMA_SEARCH_NODE **search_node);
EXTERN_C void imma_search_node_getnext(IMMA_CB *cb, SaImmSearchHandleT *search_hdl, IMMA_SEARCH_NODE **search_node);
EXTERN_C uns32 imma_search_node_add(NCS_PATRICIA_TREE *search_tree, IMMA_SEARCH_NODE *search_node);
EXTERN_C uns32 imma_search_node_delete(IMMA_CB *cb, IMMA_SEARCH_NODE *search_node);
EXTERN_C void imma_search_tree_destroy(IMMA_CB *cb);
EXTERN_C void imma_search_tree_cleanup(IMMA_CB *cb);

EXTERN_C void imma_process_stale_clients(IMMA_CB *cb);

/*30B Versioning Changes */
#define IMMA_MDS_PVT_SUBPART_VERSION 1
/*IMMA - IMMND communication */
#define IMMA_WRT_IMMND_SUBPART_VER_MIN 1
#define IMMA_WRT_IMMND_SUBPART_VER_MAX 1

#define IMMA_WRT_IMMND_SUBPART_VER_RANGE \
        (IMMA_WRT_IMMND_SUBPART_VER_MAX - \
         IMMA_WRT_IMMND_SUBPART_VER_MIN + 1 )

/*IMMND - IMMD communication */
#define IMMA_WRT_IMMD_SUBPART_VER_MIN 1
#define IMMA_WRT_IMMD_SUBPART_VER_MAX 1

#define IMMA_WRT_IMMD_SUBPART_VER_RANGE \
        (IMMA_WRT_IMMD_SUBPART_VER_MAX - \
         IMMA_WRT_IMMD_SUBPART_VER_MIN + 1 )

#endif
