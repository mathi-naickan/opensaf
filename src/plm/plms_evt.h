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
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************
..............................................................................


..............................................................................

  DESCRIPTION:


******************************************************************************
*/
#ifndef PLM_PLMS_EVT_H_
#define PLM_PLMS_EVT_H_

#include <SaHpi.h>
#include "plms_hpi.h"


typedef enum {
	PLMS_IMM_ADM_OP_EVT_T,
	PLMS_HPI_EVT_T,
	PLMS_AGENT_LIB_REQ_EVT_T,
	PLMS_AGENT_GRP_OP_EVT_T,
	PLMS_AGENT_TRACK_EVT_T,
	PLMS_PLMC_EVT_T,
	PLMS_TMR_EVT_T,
	PLMS_MDS_INFO_EVT_T,
	PLMS_DUMP_CB_EVT_T
}PLMS_EVT_TYPE;

typedef enum {
	PLMS_AGENT_INIT_EVT,
	PLMS_AGENT_FINALIZE_EVT
}PLMS_LIB_REQ_EVT_TYPE;

typedef enum {
	PLMS_AGENT_GRP_CREATE_EVT,
	PLMS_AGENT_GRP_REMOVE_EVT,
	PLMS_AGENT_GRP_ADD_EVT,
	PLMS_AGENT_GRP_DEL_EVT
}PLMS_GRP_EVT_TYPE;

typedef enum {
	/* PLMC to PLMD */
	PLMS_AGENT_TRACK_START_EVT,
	PLMS_AGENT_TRACK_STOP_EVT,
	PLMS_AGENT_TRACK_READINESS_IMPACT_EVT,
	PLMS_AGENT_TRACK_RESPONSE_EVT,
	/* PLMD to PLMC */
	PLMS_AGENT_TRACK_CBK_EVT
}PLMS_TRACK_EVT_TYPE;

typedef enum {
	/* PLMC to PLMD */
	PLMS_PLMC_EE_INSTING,
	PLMS_PLMC_EE_INSTED,
	PLMS_PLMC_EE_TRMTING,
	PLMS_PLMC_EE_TRMTED,
	PLMS_PLMC_EE_TCP_CONCTED,
	PLMS_PLMC_EE_TCP_DISCONCTED,
	PLMS_PLMC_EE_VER_RESP,
	PLMS_PLMC_EE_OS_RESP,
	PLMS_PLMC_EE_LOCK_RESP,
	PLMS_PLMC_EE_UNLOCK_RESP,
	PLMS_PLMC_EE_LOCK_INST_RESP,
	PLMS_PLMC_SVC_START_RESP,
	PLMS_PLMC_SVC_STOP_RESP,
	PLMS_PLMC_OSAF_START_RESP,
	PLMS_PLMC_OSAF_STOP_RESP,
	PLMS_PLMC_PLMD_RESTART_RESP,
	PLMS_PLMC_EE_ID_RESP,
	PLMS_PLMC_EE_RESTART_RESP,
	PLMS_PLMC_ERR_CBK
}PLMS_PLMC_EVT_TYPE;
 
typedef enum {
	PLMS_PLMC_SUCCESS,
	PLMS_PLMC_FAILURE,
	PLMS_PLMC_TIMEOUT
}PLMS_PLMC_RESP_RES;

typedef enum {
	PLMS_TMR_NONE,
	PLMS_TMR_EE_INSTANTIATING,
	PLMS_TMR_EE_TERMINATING
}PLMS_TMR_EVT_TYPE;

typedef struct plms_imm_admin_op
{
	SaPlmAdminOperationIdT operation_id;
	SaInvocationT inv_id;
	SaNameT dn_name;
	/* Needed for Lock and Restart */
	SaInt8T option[64];
}PLMS_IMM_ADMIN_OP;


typedef struct plms_hpi_evt
{
	SaHpiEventT      sa_hpi_evt;
	SaInt8T	 *entity_path;
	SaHpiEntityPathT epath_key; 
	SaUint32T	 state_model;
	PLMS_INV_DATA    inv_data;
}PLMS_HPI_EVT;

typedef struct plms_agent_lib_req
{
	PLMS_LIB_REQ_EVT_TYPE lib_req_type;
	SaPlmHandleT plm_handle;
	MDS_DEST agent_mdest_id;
}PLMS_AGENT_LIB_REQ;

typedef struct plms_agent_grp_op
{
	PLMS_GRP_EVT_TYPE grp_evt_type;
	SaPlmHandleT plm_handle;
	SaPlmEntityGroupHandleT grp_handle;
	SaNameT *entity_names;
	SaUint32T entity_names_number;
	SaPlmGroupOptionsT grp_add_option;
}PLMS_AGENT_GRP_OP;

typedef struct plms_agent_track_start
{
	SaUint8T track_flags;
	SaUint64T track_cookie;
	SaUint32T no_of_entities;
}PLMS_AGENT_TRACK_START;


/* Defing SaNtfCorrelationIdsT  for compilation purpose
need to check with NTF guys */


typedef struct plms_agent_readinees_impact
{
	SaNameT *impacted_entity;
	SaPlmReadinessImpactT impact;
	SaNtfCorrelationIdsT *correlation_ids;
}PLMS_AGENT_READINESS_IMPACT;

typedef struct plms_track_cbk_res{
	SaInvocationT invocation_id;
	SaPlmReadinessTrackResponseT response;
}PLMS_TRACK_CBK_RES;

typedef struct plms_agent_track_cbk{
	SaUint64T track_cookie;
	SaInvocationT invocation_id;
	SaPlmTrackCauseT track_cause;
	SaNameT root_cause_entity;
	SaNtfIdentifierT root_correlation_id;
	SaPlmReadinessTrackedEntitiesT tracked_entities;
	SaPlmChangeStepT change_step;
	SaAisErrorT error;
}PLMS_AGENT_TRACK_CBK;

typedef struct plms_agent_track_op
{
	PLMS_TRACK_EVT_TYPE evt_type;
	/* TODO: If not needed, remove.*/
	SaPlmHandleT agent_handle;
	SaPlmEntityGroupHandleT grp_handle;
	union {
		PLMS_AGENT_TRACK_START track_start;
		PLMS_AGENT_READINESS_IMPACT readiness_impact;
		PLMS_TRACK_CBK_RES track_cbk_res;
		PLMS_AGENT_TRACK_CBK track_cbk;
	};
}PLMS_AGENT_TRACK_OP;

  
typedef struct plms_plmc_EE_os_info{
	SaInt8T *version;
	SaInt8T *product;
	SaInt8T *vendor;
	SaInt8T *release;
}PLMS_PLMC_EE_OS_INFO;
typedef struct plms_plmc_err{
	int acode;
	int ecode;
	int cmd;
}PLMS_PLMC_ERR;

typedef struct plms_plmc_evt{
	PLMS_PLMC_EVT_TYPE plmc_evt_type;
	SaNameT ee_id;
	union {
		PLMS_PLMC_EE_OS_INFO ee_os_info;
		SaUint8T *ee_ver;
		PLMS_PLMC_RESP_RES resp;
		PLMS_PLMC_ERR err_info;
	};
}PLMS_PLMC_EVT;

typedef struct plms_tmr_evt{
	timer_t timer_id;
	PLMS_TMR_EVT_TYPE tmr_type;
	void *context_info; 	
}PLMS_TMR_EVT;


typedef struct plms_send_info
{  
	MDS_SVC_ID to_svc;
	MDS_DEST dest;
	MDS_SENDTYPES stype;  
	MDS_SYNC_SND_CTXT ctxt;
}PLMS_SEND_INFO;

/* Structure for passing MDS info to components */
typedef struct plms_mds_info {
	NCSMDS_CHG change;      /* GONE, UP, DOWN, CHG ROLE  */
	MDS_DEST dest;
	MDS_SVC_ID svc_id;
	NODE_ID node_id;
	V_DEST_RL role;
} PLMS_MDS_INFO;

typedef struct plms_evt_req
{
	PLMS_EVT_TYPE req_type;
	union{
		PLMS_IMM_ADMIN_OP admin_op;
		PLMS_HPI_EVT hpi_evt;
		PLMS_AGENT_LIB_REQ agent_lib_req;
		PLMS_AGENT_GRP_OP agent_grp_op;
		PLMS_AGENT_TRACK_OP agent_track;
		PLMS_PLMC_EVT plms_plmc_evt;
		PLMS_TMR_EVT plm_tmr;
		PLMS_MDS_INFO mds_info;
		
	};
}PLMS_EVT_REQ;

typedef enum {
	PLMS_AGENT_INIT_RES,
	PLMS_AGENT_FIN_RES,
	PLMS_AGENT_GRP_CREATE_RES,
	PLMS_AGENT_GRP_REMOVE_RES,
	PLMS_AGENT_GRP_ADD_RES,
	PLMS_AGENT_GRP_DEL_RES,
	PLMS_AGENT_TRACK_START_RES,
	PLMS_AGENT_TRACK_STOP_RES,
	PLMS_AGENT_TRACK_READINESS_IMPACT_RES
	
}PLMS_EVT_RES_TYPE;

typedef struct plms_evt_res
{
	PLMS_EVT_RES_TYPE res_type;
	SaAisErrorT error;
	SaNtfIdentifierT ntf_id;
	union{
		SaUint64T hdl;
		SaPlmReadinessTrackedEntitiesT *entities;
	};
}PLMS_EVT_RES;

typedef enum{
	PLMS_REQ,
	PLMS_RES
}PLMS_EVT_REQ_RES;

typedef struct plms_evt
{
	struct plms_evt *next;
	PLMS_EVT_REQ_RES req_res;
	PLMS_SEND_INFO sinfo;
	union {
		PLMS_EVT_REQ req_evt;
		PLMS_EVT_RES res_evt;
	};
}PLMS_EVT;


typedef enum 
{
	PLMS_HPI_CMD_BASE,
	PLMS_HPI_CMD_STANDBY_SET,
	PLMS_HPI_CMD_ACTION_REQUEST,
	PLMS_HPI_CMD_RESOURCE_ACTIVE_SET,
	PLMS_HPI_CMD_RESOURCE_INACTIVE_SET,
	PLMS_HPI_CMD_RESOURCE_POWER_ON,
	PLMS_HPI_CMD_RESOURCE_POWER_OFF,
	PLMS_HPI_CMD_RESOURCE_RESET,
	PLMS_HPI_CMD_RESOURCE_IDR_GET,
	PLMS_HPI_CMD_MAX
}PLMS_HPI_CMD;

typedef struct plms_hpi_req
{
	struct plms_hpi_req *next;
	PLMS_HPI_CMD         cmd;               /* HPI command */
	SaUint32T	     arg;              /* argument to command */
	SaUint32T	     entity_path_len;  /* length of input data */
	SaInt8T           *entity_path;     /* type, length and value of data */
	MDS_SYNC_SND_CTXT    mds_context;
	MDS_DEST             from_dest;
} PLMS_HPI_REQ;

/*Structure information passed by PLM-HPI to PLM  through sync response */
typedef struct plms_hpi_rsp
{
	SaUint32T	ret_val;    /* return value, success/failure */
	SaUint32T	data_len;   /* length of output data */
	SaUint8T        *data;       /* generic return data */
} PLMS_HPI_RSP;

SaUint32T plms_process_event();
#endif  // PLM_PLMS_EVT_H_
