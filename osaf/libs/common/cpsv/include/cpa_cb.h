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

#ifndef CPA_CB_H
#define CPA_CB_H

#include "ncs_queue.h"

/* Node to store checkpoint client information */
typedef struct cpa_client_node {
	NCS_PATRICIA_NODE patnode;
	SaCkptHandleT cl_hdl;	/* index for the tree */
	uns8 stale;		/*Loss of connection with cpnd because of clm node left
				  will set this to true for the connection. */
	SaCkptCallbacksT ckpt_callbk;
	SYSF_MBX callbk_mbx;	/* Mailbox Queue for client messages */
	ncsCkptCkptArrivalCallbackT ckptArrivalCallback;	/* NCS callback extention */
	SaVersionT version;
} CPA_CLIENT_NODE;

/* Node to store the mapping info between local & global check point handles */
typedef struct cpa_local_ckpt_node {
	NCS_PATRICIA_NODE patnode;
	SaCkptCheckpointHandleT lcl_ckpt_hdl;	/* locally generated handle */
	SaCkptHandleT cl_hdl;	/* client handle */
	SaCkptCheckpointHandleT gbl_ckpt_hdl;	/* globally aware handle */
	SaCkptCheckpointOpenFlagsT open_flags;
	SaNameT ckpt_name;
	CPA_TMR async_req_tmr;	/* Timer used for async requests */
	uns32 sect_iter_cnt;
} CPA_LOCAL_CKPT_NODE;

typedef struct cpa_glbl_ckpt_node {
	NCS_PATRICIA_NODE patnode;
	SaCkptCheckpointHandleT gbl_ckpt_hdl;	/* globally aware handle */
	/*SaCkptCheckpointHandleT    lcl_ckpt_hdl; */
	NCS_OS_POSIX_SHM_REQ_INFO open;
	SaCkptCheckpointCreationAttributesT ckpt_creat_attri;
	uns32 ref_cnt;		/* Client count */
	MDS_DEST active_mds_dest;
	NCS_BOOL is_active_exists;
	NCS_BOOL is_restart;
	/* Active replica sync with cpd flags */
	NCS_LOCK cpd_active_sync_lock;
	NCS_SEL_OBJ cpd_active_sync_sel;
	NCS_BOOL cpd_active_sync_awaited;
	NCS_BOOL is_active_bcast_came;
} CPA_GLOBAL_CKPT_NODE;

/* Section Iteration Info */
typedef struct cpa_sect_iter_node {
	NCS_PATRICIA_NODE patnode;
	SaCkptSectionIterationHandleT iter_id;	/* index of the list */
	SaCkptCheckpointHandleT lcl_ckpt_hdl;
	SaCkptCheckpointHandleT gbl_ckpt_hdl;
	SaTimeT exp_time;	/* absolute time */
	SaCkptSectionsChosenT filter;
	uns32 n_secs_trav;
	CPSV_EVT *out_evt;

	/* current section the iterator pointing to */
	SaCkptSectionIdT section_id;
} CPA_SECT_ITER_NODE;

/*****************************************************************************
 * Data Structure Used to hold CPA control block
 *****************************************************************************/
typedef struct cpa_cb {
	/* Identification Information about the CPA */
	uns32 process_id;
	uns8 *process_name;
	uns32 agent_handle_id;
	uns8 pool_id;
	uns32 cpa_mds_hdl;
	MDS_DEST cpa_mds_dest;
	NCS_LOCK cb_lock;
	EDU_HDL edu_hdl;	/* edu handle used for encode/decode */

	/* Information about CPND */
	MDS_DEST cpnd_mds_dest;
	NCS_BOOL is_cpnd_up;
	NCS_BOOL is_cpnd_joined_clm;
	CPA_TMR cpnd_down_tmr;

	/* CPA data */
	NCS_PATRICIA_TREE client_tree;	/* CPA_CLIENT_NODE - node */
	NCS_BOOL is_client_tree_up;

	NCS_PATRICIA_TREE lcl_ckpt_tree;	/* CPA_LOCAL_CKPT_NODE  - node */
	NCS_BOOL is_lcl_ckpt_tree_up;

	NCS_PATRICIA_TREE gbl_ckpt_tree;	/* CPA_GLOBAL_CKPT_NODE - node */
	NCS_BOOL is_gbl_ckpt_tree_up;

	NCS_PATRICIA_TREE sect_iter_tree;	/* CPA_SECT_ITER_NODE - node */
	NCS_BOOL is_sect_iter_tree_up;
        NCS_QUEUE cpa_evt_process_queue;

	/* Sync up with CPND ( MDS ) */
	NCS_LOCK cpnd_sync_lock;
	NCS_BOOL cpnd_sync_awaited;
	NCS_SEL_OBJ cpnd_sync_sel;

} CPA_CB;

EXTERN_C uns32 gl_cpa_hdl;

typedef struct cpa_prcess_evt_sync {
	NCS_QELEM qelem;
	CPSV_EVT error_code;
} CPA_PROCESS_EVT_SYNC;



#define m_CPA_RETRIEVE_CB(cb)                                                  \
{                                                                              \
   cb = (CPA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_CPA, gl_cpa_hdl);              \
   if(!cb)                                                                     \
      m_LOG_CPA_HEADLINE(CPA_CB_RETRIEVAL_FAILED ,NCSFL_SEV_ERROR);            \
}
#define m_CPA_GIVEUP_CB    ncshm_give_hdl(gl_cpa_hdl)

#define m_CPSV_SET_SANAMET(name) \
{\
   memset( (uns8 *)&name->value[name->length], 0, (SA_MAX_NAME_LENGTH - name->length) ); \
}

#define CPSV_MAX_DATA_SIZE 40000000

#define m_CPA_IS_ALL_REPLICA_ATTR_SET(attr)   \
            (((attr & SA_CKPT_WR_ALL_REPLICAS) != 0)?TRUE:FALSE)
#define m_CPA_IS_COLLOCATED_ATTR_SET(attr)   \
            (((attr & SA_CKPT_CHECKPOINT_COLLOCATED) != 0)?TRUE:FALSE)

/*30B Versioning Changes */
#define CPA_MDS_PVT_SUBPART_VERSION 2
/*CPA - CPND communication */
#define CPA_WRT_CPND_SUBPART_VER_MIN 1
#define CPA_WRT_CPND_SUBPART_VER_MAX 2

#define CPA_WRT_CPND_SUBPART_VER_RANGE \
        (CPA_WRT_CPND_SUBPART_VER_MAX - \
         CPA_WRT_CPND_SUBPART_VER_MIN + 1 )

/*CPND - CPD communication */
#define CPA_WRT_CPD_SUBPART_VER_MIN 1
#define CPA_WRT_CPD_SUBPART_VER_MAX 2

#define CPA_WRT_CPD_SUBPART_VER_RANGE \
        (CPA_WRT_CPD_SUBPART_VER_MAX - \
         CPA_WRT_CPD_SUBPART_VER_MIN + 1 )

/* CPA Function Declerations */
/* function prototypes for client handling*/

EXTERN_C uns32 cpa_db_init(CPA_CB *cb);
EXTERN_C uns32 cpa_db_destroy(CPA_CB *cb);

EXTERN_C uns32 cpa_client_tree_init(CPA_CB *cb);
EXTERN_C uns32 cpa_client_node_get(NCS_PATRICIA_TREE *client_tree, SaCkptHandleT *cl_hdl, CPA_CLIENT_NODE **cl_node);
EXTERN_C uns32 cpa_client_node_add(NCS_PATRICIA_TREE *client_tree, CPA_CLIENT_NODE *cl_node);
EXTERN_C uns32 cpa_client_node_delete(CPA_CB *cb, CPA_CLIENT_NODE *cl_node);
EXTERN_C void cpa_client_tree_destroy(CPA_CB *cb);
EXTERN_C void cpa_client_tree_cleanup(CPA_CB *cb);
EXTERN_C uns32 cpa_lcl_ckpt_tree_init(CPA_CB *cb);
EXTERN_C uns32 cpa_lcl_ckpt_node_get(NCS_PATRICIA_TREE *lcl_ckpt_tree,
				     SaCkptCheckpointHandleT *lc_hdl, CPA_LOCAL_CKPT_NODE **lc_node);
EXTERN_C void cpa_lcl_ckpt_node_getnext(CPA_CB *cb, SaCkptCheckpointHandleT *lc_hdl, CPA_LOCAL_CKPT_NODE **lc_node);
EXTERN_C uns32 cpa_lcl_ckpt_node_add(NCS_PATRICIA_TREE *lcl_ckpt_tree, CPA_LOCAL_CKPT_NODE *lc_node);
EXTERN_C uns32 cpa_lcl_ckpt_node_delete(CPA_CB *cb, CPA_LOCAL_CKPT_NODE *lc_node);
EXTERN_C void cpa_lcl_ckpt_tree_destroy(CPA_CB *cb);
EXTERN_C void cpa_lcl_ckpt_tree_cleanup(CPA_CB *cb);
EXTERN_C uns32 cpa_gbl_ckpt_tree_init(CPA_CB *cb);
EXTERN_C void cpa_gbl_ckpt_tree_destroy(CPA_CB *cb);
EXTERN_C uns32 cpa_gbl_ckpt_node_find_add(NCS_PATRICIA_TREE *gbl_ckpt_tree,
					  SaCkptCheckpointHandleT *gc_hdl,
					  CPA_GLOBAL_CKPT_NODE **gc_node, NCS_BOOL *add_flag);
EXTERN_C uns32 cpa_gbl_ckpt_node_add(NCS_PATRICIA_TREE *gbl_ckpt_tree, CPA_GLOBAL_CKPT_NODE *gc_node);
EXTERN_C uns32 cpa_gbl_ckpt_node_delete(CPA_CB *cb, CPA_GLOBAL_CKPT_NODE *gc_node);

EXTERN_C uns32 cpa_sect_iter_tree_init(CPA_CB *cb);
EXTERN_C uns32 cpa_sect_iter_node_get(NCS_PATRICIA_TREE *sect_iter_tree,
				      SaCkptSectionIterationHandleT *sect_iter_hdl,
				      CPA_SECT_ITER_NODE **sect_iter_node);
EXTERN_C uns32 cpa_sect_iter_node_add(NCS_PATRICIA_TREE *sect_iter_tree, CPA_SECT_ITER_NODE *sect_iter_node);
EXTERN_C uns32 cpa_sect_iter_node_delete(CPA_CB *cb, CPA_SECT_ITER_NODE *sect_iter_node);
EXTERN_C void cpa_sect_iter_tree_destroy(CPA_CB *cb);
EXTERN_C void cpa_sect_iter_node_getnext(NCS_PATRICIA_TREE *sect_iter_tree,
					 SaCkptSectionIterationHandleT *sect_iter_hdl,
					 CPA_SECT_ITER_NODE **sect_iter_node);

EXTERN_C void cpa_client_tree_mark_stale(CPA_CB *cb);
#endif
