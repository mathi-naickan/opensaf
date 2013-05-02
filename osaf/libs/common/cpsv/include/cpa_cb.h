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
#include "ncs_main_papi.h"
#include "ncssysf_def.h"

/* Node to store checkpoint client information */
typedef struct cpa_client_node {
	NCS_PATRICIA_NODE patnode;
	SaCkptHandleT cl_hdl;	/* index for the tree */
	uint8_t stale;		/*Loss of connection with cpnd because of clm node left
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
	uint32_t sect_iter_cnt;
} CPA_LOCAL_CKPT_NODE;

typedef struct cpa_glbl_ckpt_node {
	NCS_PATRICIA_NODE patnode;
	SaCkptCheckpointHandleT gbl_ckpt_hdl;	/* globally aware handle */
	/*SaCkptCheckpointHandleT    lcl_ckpt_hdl; */
	NCS_OS_POSIX_SHM_REQ_INFO open;
	SaCkptCheckpointCreationAttributesT ckpt_creat_attri;
	uint32_t ref_cnt;		/* Client count */
	MDS_DEST active_mds_dest;
	bool is_active_exists;
	bool is_restart;
	/* Active replica sync with cpd flags */
	NCS_LOCK cpd_active_sync_lock;
	NCS_SEL_OBJ cpd_active_sync_sel;
	bool cpd_active_sync_awaited;
	bool is_active_bcast_came;
} CPA_GLOBAL_CKPT_NODE;

/* Section Iteration Info */
typedef struct cpa_sect_iter_node {
	NCS_PATRICIA_NODE patnode;
	SaCkptSectionIterationHandleT iter_id;	/* index of the list */
	SaCkptCheckpointHandleT lcl_ckpt_hdl;
	SaCkptCheckpointHandleT gbl_ckpt_hdl;
	SaTimeT exp_time;	/* absolute time */
	SaCkptSectionsChosenT filter;
	uint32_t n_secs_trav;
	CPSV_EVT *out_evt;

	/* current section the iterator pointing to */
	SaCkptSectionIdT section_id;
} CPA_SECT_ITER_NODE;

/*****************************************************************************
 * Data Structure Used to hold CPA control block
 *****************************************************************************/
typedef struct cpa_cb {
	/* Identification Information about the CPA */
	uint32_t process_id;
	uint8_t *process_name;
	uint32_t agent_handle_id;
	uint8_t pool_id;
	uint32_t cpa_mds_hdl;
	MDS_DEST cpa_mds_dest;
	NCS_LOCK cb_lock;
	EDU_HDL edu_hdl;	/* edu handle used for encode/decode */

	/* Information about CPND */
	MDS_DEST cpnd_mds_dest;
	bool is_cpnd_up;
	bool is_cpnd_joined_clm;
	CPA_TMR cpnd_down_tmr;

	/* CPA data */
	NCS_PATRICIA_TREE client_tree;	/* CPA_CLIENT_NODE - node */
	bool is_client_tree_up;

	NCS_PATRICIA_TREE lcl_ckpt_tree;	/* CPA_LOCAL_CKPT_NODE  - node */
	bool is_lcl_ckpt_tree_up;

	NCS_PATRICIA_TREE gbl_ckpt_tree;	/* CPA_GLOBAL_CKPT_NODE - node */
	bool is_gbl_ckpt_tree_up;

	NCS_PATRICIA_TREE sect_iter_tree;	/* CPA_SECT_ITER_NODE - node */
	bool is_sect_iter_tree_up;
        NCS_QUEUE cpa_evt_process_queue;

	/* Sync up with CPND ( MDS ) */
	NCS_LOCK cpnd_sync_lock;
	bool cpnd_sync_awaited;
	NCS_SEL_OBJ cpnd_sync_sel;

} CPA_CB;

uint32_t gl_cpa_hdl;

typedef struct cpa_prcess_evt_sync {
	NCS_QELEM qelem;
	CPSV_EVT error_code;
} CPA_PROCESS_EVT_SYNC;



#define m_CPA_RETRIEVE_CB(cb)                                                  \
{                                                                              \
   cb = (CPA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_CPA, gl_cpa_hdl);              \
   if(!cb)									\
	LOG_ER("CPND CB TAKE HDL FAILED ");\
}
#define m_CPA_GIVEUP_CB    ncshm_give_hdl(gl_cpa_hdl)

#define m_CPSV_SET_SANAMET(name) \
{\
   memset( (uint8_t *)&name->value[name->length], 0, (SA_MAX_NAME_LENGTH - name->length) ); \
}

#define CPSV_MAX_DATA_SIZE 40000000

#define m_CPA_IS_ALL_REPLICA_ATTR_SET(attr)   \
            (((attr & SA_CKPT_WR_ALL_REPLICAS) != 0)?true:false)
#define m_CPA_IS_COLLOCATED_ATTR_SET(attr)   \
            (((attr & SA_CKPT_CHECKPOINT_COLLOCATED) != 0)?true:false)

/*30B Versioning Changes */
#define CPA_MDS_PVT_SUBPART_VERSION 4
/*CPA - CPND communication */
#define CPA_WRT_CPND_SUBPART_VER_MIN 1
#define CPA_WRT_CPND_SUBPART_VER_MAX 4

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

uint32_t cpa_db_init(CPA_CB *cb);
uint32_t cpa_db_destroy(CPA_CB *cb);

uint32_t cpa_client_tree_init(CPA_CB *cb);
uint32_t cpa_client_node_get(NCS_PATRICIA_TREE *client_tree, SaCkptHandleT *cl_hdl, CPA_CLIENT_NODE **cl_node);
uint32_t cpa_client_node_add(NCS_PATRICIA_TREE *client_tree, CPA_CLIENT_NODE *cl_node);
uint32_t cpa_client_node_delete(CPA_CB *cb, CPA_CLIENT_NODE *cl_node);
void cpa_client_tree_destroy(CPA_CB *cb);
void cpa_client_tree_cleanup(CPA_CB *cb);
uint32_t cpa_lcl_ckpt_tree_init(CPA_CB *cb);
uint32_t cpa_lcl_ckpt_node_get(NCS_PATRICIA_TREE *lcl_ckpt_tree,
				     SaCkptCheckpointHandleT *lc_hdl, CPA_LOCAL_CKPT_NODE **lc_node);
void cpa_lcl_ckpt_node_getnext(CPA_CB *cb, SaCkptCheckpointHandleT *lc_hdl, CPA_LOCAL_CKPT_NODE **lc_node);
uint32_t cpa_lcl_ckpt_node_add(NCS_PATRICIA_TREE *lcl_ckpt_tree, CPA_LOCAL_CKPT_NODE *lc_node);
uint32_t cpa_lcl_ckpt_node_delete(CPA_CB *cb, CPA_LOCAL_CKPT_NODE *lc_node);
void cpa_lcl_ckpt_tree_destroy(CPA_CB *cb);
void cpa_lcl_ckpt_tree_cleanup(CPA_CB *cb);
uint32_t cpa_gbl_ckpt_tree_init(CPA_CB *cb);
void cpa_gbl_ckpt_tree_destroy(CPA_CB *cb);
uint32_t cpa_gbl_ckpt_node_find_add(NCS_PATRICIA_TREE *gbl_ckpt_tree,
					  SaCkptCheckpointHandleT *gc_hdl,
					  CPA_GLOBAL_CKPT_NODE **gc_node, bool *add_flag);
uint32_t cpa_gbl_ckpt_node_add(NCS_PATRICIA_TREE *gbl_ckpt_tree, CPA_GLOBAL_CKPT_NODE *gc_node);
uint32_t cpa_gbl_ckpt_node_delete(CPA_CB *cb, CPA_GLOBAL_CKPT_NODE *gc_node);

uint32_t cpa_sect_iter_tree_init(CPA_CB *cb);
uint32_t cpa_sect_iter_node_get(NCS_PATRICIA_TREE *sect_iter_tree,
				      SaCkptSectionIterationHandleT *sect_iter_hdl,
				      CPA_SECT_ITER_NODE **sect_iter_node);
uint32_t cpa_sect_iter_node_add(NCS_PATRICIA_TREE *sect_iter_tree, CPA_SECT_ITER_NODE *sect_iter_node);
uint32_t cpa_sect_iter_node_delete(CPA_CB *cb, CPA_SECT_ITER_NODE *sect_iter_node);
void cpa_sect_iter_tree_destroy(CPA_CB *cb);
void cpa_sect_iter_node_getnext(NCS_PATRICIA_TREE *sect_iter_tree,
					 SaCkptSectionIterationHandleT *sect_iter_hdl,
					 CPA_SECT_ITER_NODE **sect_iter_node);

void cpa_client_tree_mark_stale(CPA_CB *cb);
#endif
