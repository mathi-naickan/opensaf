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

 FILE NAME: cpnd_init.h

..............................................................................

  DESCRIPTION:
  This file consists of constats, enums and data structs used by cpnd_init.c
******************************************************************************/

#ifndef CPND_INIT_H
#define CPND_INIT_H

#include <saClm.h>

/* Macro to get the component name for the component type */
#define m_CPND_TASKNAME "CPND"

/* Macro for MQND task priority */
#define m_CPND_TASK_PRI (5)

/* Macro for CPND task stack size */
#define m_CPND_STACKSIZE NCS_STACKSIZE_HUGE

#define m_CPND_STORE_CB_HDL(hdl) (gl_cpnd_cb_hdl = hdl)
#define m_CPND_GET_CB_HDL (gl_cpnd_cb_hdl)

#define m_CPND_RETRIEVE_CB(cb)                                                 \
{                                                                              \
   uns32 hdl = m_CPND_GET_CB_HDL;                                              \
   if((cb = (CPND_CB *)ncshm_take_hdl(NCS_SERVICE_ID_CPND, hdl)) == NULL)      \
      m_LOG_CPND_CL(CPND_CB_HDL_TAKE_FAILED,CPND_FC_HDLN,NCSFL_SEV_ERROR,__FILE__,__LINE__); \
}
#define m_CPND_GIVEUP_CB    ncshm_give_hdl(m_CPND_GET_CB_HDL)

/* cpnd_ckpt_update_replica error codes  */
#define CKPT_UPDATE_REPLICA_RES_ERR 1
#define CKPT_UPDATE_REPLICA_NO_SECTION 2
/* end of error codes */

/*****************************************************************************
 * structure which holds the create information.
 *****************************************************************************/
typedef struct cpnd_create_info {
	uns8 pool_id;		/* Handle manager Pool ID */
} CPND_CREATE_INFO;

/*****************************************************************************
 * structure which holds the destroy information.
 *****************************************************************************/
typedef struct cpnd_destroy_info {
	uns32 dummy;
} CPND_DESTROY_INFO;

/* internal helping function declaration */

/* file : -  cpnd_proc.c */

uns32 cpnd_ckpt_non_collocated_rplica_close(CPND_CB *cb, CPND_CKPT_NODE *cp_node, SaAisErrorT *error);
uns32 cpnd_ckpt_client_add(CPND_CKPT_NODE *cp_node, CPND_CKPT_CLIENT_NODE *cl_node);
uns32 cpnd_proc_non_colloc_rt_expiry(CPND_CB *cb, SaCkptCheckpointHandleT ckpt_id);
uns32 cpnd_ckpt_client_del(CPND_CKPT_NODE *cp_node, CPND_CKPT_CLIENT_NODE *cl_node);
uns32 cpnd_client_ckpt_info_add(CPND_CKPT_CLIENT_NODE *cl_node, CPND_CKPT_NODE *cp_node);
uns32 cpnd_client_ckpt_info_del(CPND_CKPT_CLIENT_NODE *cl_node, CPND_CKPT_NODE *cp_node);
uns32 cpnd_ckpt_replica_destroy(CPND_CB *cb, CPND_CKPT_NODE *cp_node, SaAisErrorT *error);
uns32 cpnd_ckpt_replica_create(CPND_CB *cb, CPND_CKPT_NODE *cp_node);
uns32 cpnd_ckpt_remote_cpnd_add(CPND_CKPT_NODE *cp_node, MDS_DEST mds_info);
uns32 cpnd_ckpt_remote_cpnd_del(CPND_CKPT_NODE *cp_node, MDS_DEST mds_info);
int32 cpnd_ckpt_get_lck_sec_id(CPND_CKPT_NODE *cp_node);
uns32 cpnd_ckpt_sec_write(CPND_CKPT_NODE *cp_node, CPND_CKPT_SECTION_INFO
			  *sec_info, const void *data, uns32 size, uns32 offset, uns32 type);
uns32 cpnd_ckpt_sec_read(CPND_CKPT_NODE *cp_node, CPND_CKPT_SECTION_INFO
			 *sec_info, void *data, uns32 size, uns32 offset);
void cpnd_proc_cpa_down(CPND_CB *cb, MDS_DEST dest);
uns32 cpnd_ckpt_update_replica(CPND_CB *cb, CPND_CKPT_NODE *cp_node,
			       CPSV_CKPT_ACCESS *write_data, uns32 type, uns32 *err_type, uns32 *errflag);
uns32 cpnd_ckpt_read_replica(CPND_CB *cb, CPND_CKPT_NODE *cp_node, CPSV_CKPT_ACCESS *read_data, CPSV_EVT *evt);
CPSV_CKPT_DATA *cpnd_ckpt_generate_cpsv_ckpt_access_evt(CPND_CKPT_NODE *cp_node);
void cpnd_proc_gen_mapping(CPND_CKPT_NODE *cp_node, CPSV_CKPT_ACCESS *ckpt_read, CPSV_EVT *evt);
uns32 cpnd_proc_update_remote(CPND_CB *cb, CPND_CKPT_NODE *cp_node, CPND_EVT *in_evt,
			      CPSV_EVT *out_evt, CPSV_SEND_INFO *sinfo);
uns32 cpnd_proc_rt_expiry(CPND_CB *cb, SaCkptCheckpointHandleT ckpt_id);
uns32 cpnd_proc_sec_expiry(CPND_CB *cb, CPND_TMR_INFO *tmr_info);
void cpnd_cb_dump(void);
void cpnd_proc_cpd_down(CPND_CB *cb);
void cpnd_proc_free_cpsv_ckpt_data(CPSV_CKPT_DATA *data);
uns32 cpnd_allrepl_write_evt_node_free(CPSV_CPND_ALL_REPL_EVT_NODE *evt_node);
uns32 cpnd_proc_getnext_section(CPND_CKPT_NODE *cp_node,
				CPSV_A2ND_SECT_ITER_GETNEXT *get_next,
				SaCkptSectionDescriptorT *sec_des, uns32 *n_secs_trav);
uns32 cpnd_proc_fill_sec_desc(CPND_CKPT_SECTION_INFO *pTmpSecPtr, SaCkptSectionDescriptorT *sec_des);

uns32 cpnd_proc_ckpt_arrival_info_ntfy(CPND_CB *cb, CPND_CKPT_NODE *cp_node, CPSV_CKPT_ACCESS *in_evt,
				       CPSV_SEND_INFO *sinfo);
uns32 cpnd_proc_ckpt_clm_node_left(CPND_CB *cb);
uns32 cpnd_proc_ckpt_clm_node_joined(CPND_CB *cb);

NCS_BOOL cpnd_is_noncollocated_replica_present_on_payload(CPND_CB *cb, CPND_CKPT_NODE *cp_node);
uns32 cpnd_ckpt_replica_close(CPND_CB *cb, CPND_CKPT_NODE *cp_node, SaAisErrorT *error);
uns32 cpnd_send_ckpt_usr_info_to_cpd(CPND_CB *cb, CPND_CKPT_NODE *cp_node,
				     SaCkptCheckpointOpenFlagsT ckpt_open_flags, CPSV_USR_INFO_CKPT_TYPE usrinfoType);
uns32 cpnd_client_ckpt_info_delete(CPND_CKPT_CLIENT_NODE *cl_node, CPND_CKPT_NODE *cp_node);
void cpnd_proc_cpa_up(CPND_CB *cb, MDS_DEST dest);
void cpnd_proc_app_status(CPND_CB *cb);
uns32 cpnd_ckpt_client_find(CPND_CKPT_NODE *cp_node, CPND_CKPT_CLIENT_NODE *cl_node);
uns32 cpnd_all_repl_rsp_expiry(CPND_CB *cb, CPND_TMR_INFO *tmr_info);
uns32 cpnd_open_active_sync_expiry(CPND_CB *cb, CPND_TMR_INFO *tmr_info);
void cpnd_proc_free_read_data(CPSV_EVT *evt);
/* End cpnd_proc.c */

/* File : ---  cpnd_amf.c */
uns32 cpnd_amf_init(CPND_CB *cpnd_cb);
void cpnd_amf_de_init(CPND_CB *cpnd_cb);
uns32 cpnd_amf_register(CPND_CB *cpnd_cb);
uns32 cpnd_amf_deregister(CPND_CB *cpnd_cb);
void
cpnd_saf_health_chk_callback(SaInvocationT invocation, const SaNameT *compName, const SaAmfHealthcheckKeyT *checkType);
void
cpnd_amf_csi_rmv_callback(SaInvocationT invocation,
			  const SaNameT *compName, const SaNameT *csiName, const SaAmfCSIFlagsT *csiFlags);
void cpnd_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName);

void cpnd_saf_csi_set_cb(SaInvocationT invocation,
			 const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor);

/* End cpnd_amf.c */

/* File cpnd_db.c */
void cpnd_ckpt_node_get(CPND_CB *cb, SaCkptCheckpointHandleT ckpt_hdl, CPND_CKPT_NODE **ckpt_node);
void cpnd_ckpt_node_getnext(CPND_CB *cb, SaCkptCheckpointHandleT ckpt_hdl, CPND_CKPT_NODE **ckpt_node);
uns32 cpnd_ckpt_node_add(CPND_CB *cb, CPND_CKPT_NODE *ckpt_node);
uns32 cpnd_ckpt_node_del(CPND_CB *cb, CPND_CKPT_NODE *ckpt_node);
void cpnd_client_node_get(CPND_CB *cb, SaCkptHandleT ckpt_client_hdl, CPND_CKPT_CLIENT_NODE **ckpt_client_node);
void cpnd_client_node_getnext(CPND_CB *cb, SaCkptHandleT ckpt_client_hdl, CPND_CKPT_CLIENT_NODE **ckpt_client_node);
uns32 cpnd_client_node_add(CPND_CB *cb, CPND_CKPT_CLIENT_NODE *ckpt_node);
uns32 cpnd_client_node_del(CPND_CB *cb, CPND_CKPT_CLIENT_NODE *ckpt_client_node);
void cpnd_evt_node_get(CPND_CB *cb, MDS_DEST dest, CPSV_CPND_ALL_REPL_EVT_NODE **evt_node);
void cpnd_evt_node_getnext(CPND_CB *cb, MDS_DEST dest, CPSV_CPND_ALL_REPL_EVT_NODE **evt_node);
uns32 cpnd_evt_node_add(CPND_CB *cb, CPSV_CPND_ALL_REPL_EVT_NODE *evt_node);
uns32 cpnd_evt_node_del(CPND_CB *cb, CPSV_CPND_ALL_REPL_EVT_NODE *evt_node);
void cpnd_allrepl_write_stale_evt_node_cleanup(CPND_CB *cb, SaCkptCheckpointHandleT ckpt_id);
CPND_CKPT_NODE *cpnd_ckpt_node_find_by_name(CPND_CB *cpnd_cb, SaNameT ckpt_name);
CPND_CKPT_SECTION_INFO *cpnd_ckpt_sec_get(CPND_CKPT_NODE *cp_node, SaCkptSectionIdT *id);
CPND_CKPT_SECTION_INFO *cpnd_ckpt_sec_get_create(CPND_CKPT_NODE *cp_node, SaCkptSectionIdT *id);
uns32 cpnd_ckpt_sec_find(CPND_CKPT_NODE *cp_node, SaCkptSectionIdT *id);
CPND_CKPT_SECTION_INFO *cpnd_ckpt_sec_del(CPND_CKPT_NODE *cp_node, SaCkptSectionIdT *id);
CPND_CKPT_SECTION_INFO *cpnd_ckpt_sec_add(CPND_CKPT_NODE *cp_node, SaCkptSectionIdT *id, SaTimeT exp_time,
					  uns32 gen_flag);
void cpnd_ckpt_delete_all_sect(CPND_CKPT_NODE *cp_node);
void cpnd_evt_backup_queue_add(CPND_CKPT_NODE *cp_node, CPND_EVT *evt);
CPND_CKPT_SECTION_INFO *cpnd_get_sect_with_id(CPND_CKPT_NODE *cp_node, uns32 lcl_sec_id);
uns32 cpnd_ckpt_node_tree_init(CPND_CB *cb);
uns32 cpnd_allrepl_write_evt_node_tree_init(CPND_CB *cb);
uns32 cpnd_client_node_tree_init(CPND_CB *cb);
void cpnd_ckpt_node_tree_cleanup(CPND_CB *cb);
void cpnd_ckpt_node_tree_destroy(CPND_CB *cb);
void cpnd_client_node_tree_cleanup(CPND_CB *cb);
void cpnd_client_node_tree_destroy(CPND_CB *cb);
void cpnd_allrepl_write_evt_node_tree_cleanup(CPND_CB *cb);
void cpnd_allrepl_write_evt_node_tree_destroy(CPND_CB *cb);
uns32 cpnd_sec_hdr_update(CPND_CKPT_SECTION_INFO *pSecPtr, CPND_CKPT_NODE *cp_node);
uns32 cpnd_ckpt_hdr_update(CPND_CKPT_NODE *cp_node);
void cpnd_ckpt_node_destroy(CPND_CB *cb, CPND_CKPT_NODE *cp_node);
uns32 cpnd_get_slot_sub_slot_id_from_mds_dest(MDS_DEST dest);
uns32 cpnd_get_slot_sub_slot_id_from_node_id(NCS_NODE_ID i_node_id);
void cpnd_agent_dest_add(CPND_CKPT_NODE *cp_node, MDS_DEST adest);
void cpnd_agent_dest_del(CPND_CKPT_NODE *cp_node, MDS_DEST adest);
void cpnd_proc_pending_writes(CPND_CB *cb, CPND_CKPT_NODE *cp_node, MDS_DEST adest);
/* End  File : cpnd_db.c */

/* File : --- cpnd_tmr.c */
void cpnd_timer_expiry(NCSCONTEXT uarg);
uns32 cpnd_tmr_start(CPND_TMR *tmr, SaTimeT duration);
void cpnd_tmr_stop(CPND_TMR *tmr);
/* End : --- cpnd_tmr.c */

/* File : --- cpnd_mds.c */
uns32 cpnd_mds_send_rsp(CPND_CB *cb, CPSV_SEND_INFO *s_info, CPSV_EVT *evt);
uns32 cpnd_mds_msg_sync_send(CPND_CB *cb, uns32 to_svc, MDS_DEST to_dest,
			     CPSV_EVT *i_evt, CPSV_EVT **o_evt, uns32 timeout);
uns32 cpnd_mds_msg_send(CPND_CB *cb, uns32 to_svc, MDS_DEST to_dest, CPSV_EVT *evt);
uns32 cpnd_mds_msg_sync_ack_send(CPND_CB *cb, uns32 to_svc, MDS_DEST to_dest, CPSV_EVT *evt, uns32 timeout);
uns32 cpnd_mds_register(CPND_CB *cb);
void cpnd_mds_unregister(CPND_CB *cb);
uns32 cpnd_mds_get_handle(CPND_CB *cb);
uns32 cpnd_mds_bcast_send(CPND_CB *cb, CPSV_EVT *evt, NCSMDS_SVC_ID to_svc);
/* End : --- cpnd_mds.c */

/* File : ----  cpnd_evt.c */

void cpnd_process_evt(CPSV_EVT *evt);
uns32 cpnd_evt_destroy(CPSV_EVT *evt);

/* End : ----  cpnd_evt.c  */

/* File : ----  cpnd_res.c */

uns32 cpnd_restart_client_node_del(CPND_CB *cb, CPND_CKPT_CLIENT_NODE *cl_node);
uns32 cpnd_restart_shm_ckpt_update(CPND_CB *cb, CPND_CKPT_NODE *cp_node, SaCkptHandleT client_hdl);
uns32 cpnd_restart_shm_ckpt_free(CPND_CB *cb, CPND_CKPT_NODE *cp_node);
void cpnd_restart_client_reset(CPND_CB *cb, CPND_CKPT_NODE *cp_node, CPND_CKPT_CLIENT_NODE *cl_node);
void cpnd_restart_ckpt_name_length_reset(CPND_CB *cb, CPND_CKPT_NODE *cp_node);
void cpnd_restart_set_arrcb(CPND_CB *cb, CPND_CKPT_CLIENT_NODE *cl_node);
void cpnd_restart_set_close_flag(CPND_CB *cb, CPND_CKPT_NODE *cp_node);
void cpnd_restart_update_timer(CPND_CB *cb, CPND_CKPT_NODE *cp_node, SaTimeT closetime);

/* End : ----  cpnd_res.c  */

/* File : ----  cpnd_log.c */

void cpnd_flx_log_reg(void);
void cpnd_flx_log_dereg(void);

/* End : ----  cpnd_log.c  */

/* File  :  ---  cpnd_init.c */

void *cpnd_restart_shm_create(NCS_OS_POSIX_SHM_REQ_INFO *cpnd_open_req, CPND_CB *cb, SaClmNodeIdT nodeid);

/* End : -- cpnd_init.c */
#endif
