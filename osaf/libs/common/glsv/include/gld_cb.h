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

#ifndef GLD_CB_H
#define GLD_CB_H

#include "gld_tmr.h"
#include "saImmOi.h"

/* typedef enums */
typedef enum {
	GLD_RESOURCE_NOT_INITIALISED = 0,
	GLD_RESOURCE_ACTIVE_MASTER,
	GLD_RESOURCE_ACTIVE_NON_MASTER,
	GLD_RESOURCE_ELECTION_IN_PROGESS,
} GLD_RESOURCE_STATUS;

typedef struct glsv_node_list_tag {
	MDS_DEST dest_id;
	uint32_t node_id;
	struct glsv_node_list_tag *next;
} GLSV_NODE_LIST;

typedef struct glsv_gld_rsc_info_tag {
	NCS_PATRICIA_NODE pat_node;
	SaLckResourceIdT rsc_id;	/* unique resource id - Index for            */
	SaNameT lck_name;	/* Cluster-wide unique lock name             */
	GLSV_NODE_LIST *node_list;	/* Nodes on which this resource is reffered  */
	bool can_orphan;	/* is this resource allocated in orphan mode */
	SaLckLockModeT orphan_lck_mode;	/* related to orphan mode                    */
	GLD_TMR reelection_timer;
	uint32_t status;
	struct glsv_gld_rsc_info_tag *next;	/* List of resources                         */
	struct glsv_gld_rsc_info_tag *prev;

	SaTimeT saf_rsc_creation_time;
	uint32_t saf_rsc_no_of_users;
	uint32_t saf_rsc_stripped_cnt;
} GLSV_GLD_RSC_INFO;

typedef struct glsv_glnd_rsc_ref_tag {
	NCS_PATRICIA_NODE pat_node;
	SaLckResourceIdT rsc_id;	/* unique resource id - Index for  */
	GLSV_GLD_RSC_INFO *rsc_info;
} GLSV_GLD_GLND_RSC_REF;

typedef struct glsv_gld_glnd_details_tag {
	NCS_PATRICIA_NODE pat_node;
	MDS_DEST dest_id;	/* Vcard id as given by MDS                  */
	uint32_t node_id;		/* Node id of the Node Director              */
	uint32_t status;
	GLD_TMR restart_timer;
	NCS_PATRICIA_TREE rsc_info_tree;	/* List of resources ref by this node        */
} GLSV_GLD_GLND_DETAILS;

typedef struct glsv_gld_rsc_map_info {
	NCS_PATRICIA_NODE pat_node;
	SaLckResourceIdT rsc_id;	/* unique resource id - Index for            */
	SaNameT rsc_name;
} GLSV_GLD_RSC_MAP_INFO;

typedef struct glsv_gld_cb_tag {
	SYSF_MBX mbx;		/* GLDs mailbox                              */
	SaNameT comp_name;	/* Component name - "GLD"                    */
	MDS_HDL mds_handle;	/* PWE handle used for interacting with NDs  */
	uint8_t hm_poolid;		/* For use with handle manager               */
	NCSCONTEXT task_hdl;
	uint32_t my_hdl;		/* Handle manager handle                     */
	uint32_t clm_hdl;		/* Handle manager handle                     */
	NCS_MBCSV_HDL mbcsv_handle;
	NCS_MBCSV_CKPT_HDL o_ckpt_hdl;
	SaSelectionObjectT mbcsv_sel_obj;
	uint32_t gld_async_cnt;
	V_DEST_QA my_anc;
	MDS_VDEST_ID my_dest_id;	/* My identification in MDS                  */

	bool glnd_details_tree_up;
	NCS_PATRICIA_TREE glnd_details;	/* Details of nodes on which NDs are up      */
	bool rsc_info_id_tree_up;
	NCS_PATRICIA_TREE rsc_info_id;	/* Resource info indexed by rsc_id           */
	bool rsc_map_info_tree_up;
	NCS_PATRICIA_TREE rsc_map_info;	/* Resource info indexed by rsc_name           */
	GLSV_GLD_RSC_INFO *rsc_info;	/* List of resources referred to             */
	SaLckResourceIdT nxt_rsc_id;	/* Next rsc id to be generated               */
	SaLckResourceIdT prev_rsc_id;	/* Prev res_id to be used for next cold sync rsp */

	SaAmfHandleT amf_hdl;	/* AMF handle, obtained thru AMF init        */
	SaAmfHAStateT ha_state;	/* present AMF HA state of the component     */

	EDU_HDL edu_hdl;	/* EDU Handle for encode decodes             */

	SaInvocationT invocation;
	uint32_t is_quiasced;

	SaImmOiHandleT immOiHandle;	/* IMM OI Handle */
	SaSelectionObjectT imm_sel_obj;	/*Selection object to wait for 
					   IMM events */
	bool is_impl_set;
} GLSV_GLD_CB;

#define GLD_CB_NULL  ((GLSV_GLD_CB *)0)

/* Function Declarations */

uint32_t gld_mds_quiesced_process(GLSV_GLD_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *rcv_info);
void gld_snd_master_status(GLSV_GLD_CB *gld_cb, GLSV_GLD_RSC_INFO *rsc_info, uint32_t status);
GLSV_GLD_RSC_INFO *gld_find_rsc_by_id(GLSV_GLD_CB *gld_cb, SaLckResourceIdT rsc_id);
uint32_t glsv_gld_mbcsv_register(GLSV_GLD_CB *gld_cb);
uint32_t glsv_gld_mbcsv_unregister(GLSV_GLD_CB *gld_cb);
void gld_dump_cb(void);

GLSV_GLD_RSC_INFO *gld_find_add_rsc_name(GLSV_GLD_CB *gld_cb,
					 SaNameT *rsc_name,
					 SaLckResourceIdT rsc_id, SaLckResourceOpenFlagsT flag, SaAisErrorT *error);

void gld_free_rsc_info(GLSV_GLD_CB *gld_cb, GLSV_GLD_RSC_INFO *rsc_info);

void gld_rsc_rmv_node_ref(GLSV_GLD_CB *gld_cb,
			  GLSV_GLD_RSC_INFO *rsc_info,
			  GLSV_GLD_GLND_RSC_REF *glnd_rsc, GLSV_GLD_GLND_DETAILS *node_details, bool orphan_flag);

void gld_rsc_add_node_ref(GLSV_GLD_CB *gld_cb, GLSV_GLD_GLND_DETAILS *node_details, GLSV_GLD_RSC_INFO *rsc_info);

#define m_GLSV_GLD_RETRIEVE_GLD_CB  ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl)
#define m_GLSV_GLD_GIVEUP_GLD_CB    ncshm_give_hdl(gl_gld_hdl)

#define GLD_RESOURCE_INFO_NODE_NULL ((GLSV_GLD_RSC_INFO *)0)
uint32_t gld_rsc_ref_set_orphan(GLSV_GLD_GLND_DETAILS *node_details, SaLckResourceIdT rsc_id, bool orphan,
				      SaLckLockModeT lck_mode);
GLSV_GLD_RSC_INFO *gld_add_rsc_info(GLSV_GLD_CB *gld_cb, SaNameT *rsc_name, SaLckResourceIdT rsc_id,
					     SaAisErrorT *error);
uint32_t gld_process_node_down_evts(GLSV_GLD_CB *gld_cb);

#endif
