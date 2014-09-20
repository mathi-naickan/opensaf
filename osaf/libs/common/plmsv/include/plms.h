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

  DESCRIPTION:
  This include file contains PLMS Control block and entity related data structures 

******************************************************************************/
#ifndef PLMS_H
#define PLMS_H

#include <stdbool.h>
#include "ncsgl_defs.h"

#include "ncs_saf.h"
#include "ncs_lib.h"
#include "ncs_util.h"
#include "mds_papi.h"
#include "ncs_edu_pub.h"
#include "ncs_main_papi.h"
#include "ncs_mda_pvt.h"
#include "ncssysf_lck.h"

#include <SaHpi.h>
#include <saAis.h>
#include <saImmOm.h>
#include <saImmOi.h>
#include <saImm.h>
#include <saNtf.h>
#include <saPlm.h>
#include "logtrace.h"
#include "mbcsv_papi.h"
#include "plms_hpi.h"
#include "plms_evt.h"
#include "plmc.h"
#include "plmc_lib_internal.h"
#include "plmc_lib.h"


/* Macros to see if track flags are set */
#define m_PLM_IS_SA_TRACK_CURRENT_SET(attr) \
                  (attr & SA_TRACK_CURRENT)
#define m_PLM_IS_SA_TRACK_CHANGES_SET(attr) \
                  (attr & SA_TRACK_CHANGES)
#define m_PLM_IS_SA_TRACK_CHANGES_ONLY_SET(attr) \
                  (attr & SA_TRACK_CHANGES_ONLY)
#define m_PLM_IS_SA_TRACK_START_STEP_SET(attr) \
                  (attr & SA_TRACK_START_STEP)
#define m_PLM_IS_SA_TRACK_VALIDATE_STEP_SET(attr) \
                  (attr & SA_TRACK_VALIDATE_STEP)


/* Macros to see what entity group options are set */
#define m_PLM_IS_SA_PLM_GROUP_SINGLE_ENTITY_SET(attr) \
                  (attr & SA_PLM_GROUP_SINGLE_ENTITY)
#define m_PLM_IS_SA_PLM_GROUP_SUBTREE_SET(attr) \
                  (attr & SA_PLM_GROUP_SUBTREE)
#define m_PLM_IS_SA_PLM_GROUP_SUBTREE_HES_ONLY_SET(attr) \
                  (attr & SA_PLM_GROUP_SUBTREE_HES_ONLY)
#define m_PLM_IS_SA_PLM_GROUP_SUBTREE_EES_ONLY_SET(attr) \
                  (attr & SA_PLM_GROUP_SUBTREE_EES_ONLY)


#define SA_PLM_HE_ADMIN_STATE_MAX SA_PLM_HE_ADMIN_SHUTTING_DOWN+1
#define SA_PLM_EE_ADMIN_STATE_MAX SA_PLM_EE_ADMIN_SHUTTING_DOWN+1
#define SA_PLM_HE_PRES_STATE_MAX SA_PLM_HE_PRESENCE_DEACTIVATING+1
#define SA_PLM_HPI_HE_PRES_STATE_MAX SAHPI_HS_STATE_NOT_PRESENT+1
#define SA_PLM_ADMIN_OP_MAX SA_PLM_ADMIN_REMOVED+1

#define SA_PLM_ADMIN_LOCK_OPTION_DEFAULT "default"
#define PLMS_READINESS_FLAG_RESET 0
#define HPI_DN_NAME "safApp=safHpiService" 
#define PLMS_HRB_MDS_TIMEOUT 1500 /* In 10 msec.*/

/* 3OB Versioning */
#define PLMS_MDS_PVT_SUBPART_VERSION 1
#define PLMS_WRT_PLMA_SUBPART_VER_MIN 1
#define PLMS_WRT_PLMA_SUBPART_VER_MAX 1
#define PLMS_WRT_PLMA_SUBPART_VER_RANGE        \
        (PLMS_WRT_PLMA_SUBPART_VER_MIN  - \
	PLMS_WRT_PLMA_SUBPART_VER_MAX + 1)

/* MBCSV Version for PLMS */
#define PLMS_MBCSV_VERSION_MIN 1
#define PLMS_MBCSV_VERSION 1

/* FIXME : see if this is the right place for this */
#define m_PLMS_GET_AMF_VER(amf_ver) amf_ver.releaseCode='B'; amf_ver.majorVersion=0x01; amf_ver.minorVersion=0x01;


typedef struct
{
	SaStringT safDomain; 
	SaPlmHEDeactivationPolicyT  saPlmHEDeactivationPolicy;
}SaPlmDomainT;

/* data structure to hold dependency list of an entity */
typedef struct
{
	SaStringT safDependency; /* dnName */
	SaNameT   *saPlmDepNames;
	SaUint32T saPlmDepMinNumber;
}SaPlmDependencyT;


typedef struct
{
	SaStringT  safVersion;
	SaStringT  saPlmHetIdr;
}SaPlmHETypeT;

typedef struct
{
	SaStringT  safHEType;    /*rdn */
	SaStringT  saPlmHetHpiEntityType;
}SaPlmHEBaseTypeT;

typedef struct 
{
	SaStringT  safVersion;
	SaTimeT    saPlmEeDefaultInstantiateTimeout;
	SaTimeT    saPlmEeDefTerminateTimeout;
} SaPlmEETypeT;



typedef struct 
{
	SaStringT  safEEType;
	SaStringT  saPlmEetProduct;
	SaStringT  saPlmEetVendor;
	SaStringT  saPlmEetRelease;
}SaPlmEEBaseTypeT;


typedef struct
{
	SaStringT     safHE;  /*  rdName */
	SaNameT       saPlmHEBaseHEType;
	SaStringT     saPlmHEEntityPaths[SAHPI_MAX_ENTITY_PATH];
	SaNameT       saPlmHECurrHEType;
	SaStringT     saPlmHECurrEntityPath;
	SaPlmHEAdminStateT	saPlmHEAdminState;
	SaPlmReadinessStateT    saPlmHEReadinessState; 
	SaPlmReadinessFlagsT	saPlmHEReadinessFlags; 
	SaPlmHEPresenceStateT   saPlmHEPresenceState;
	SaPlmOperationalStateT  saPlmHEOperationalState;
} SaPlmHeT;


typedef struct 
{
	SaStringT      safEE;   /*rdn*/
	SaNameT        saPlmEEType;
	SaTimeT        saPlmEEInstantiateTimeout;
	SaTimeT        saPlmEETerminateTimeout;
	 
	SaPlmEEAdminStateT    saPlmEEAdminState;
	SaPlmReadinessStateT  saPlmEEReadinessState;
	SaPlmReadinessFlagsT  saPlmEEReadinessFlags; 
	SaPlmEEPresenceStateT  saPlmEEPresenceState;
	SaPlmOperationalStateT saPlmEEOperationalState;
} SaPlmEeT;




typedef enum
{
	PLMS_CONFIG_UN_INITIALIZED,
	PLMS_CONFIG_INITIALIZED
}PLMS_CONFIG_STATE;


/* Datastructure to synchronize ha_state btw PLMS & HSM */
typedef struct
{
        pthread_mutex_t   mutex;
        pthread_cond_t    cond;
        SaUint32T         state;
}HSM_HA_STATE;

/* Datastructure to synchronize ha_state btw PLMS & HRB */
typedef struct
{
        pthread_mutex_t   mutex;
        pthread_cond_t    cond;
        SaUint32T         state;
}HRB_HA_STATE;


typedef  struct plms_ckpt_track_step_info
{
	SaNameT       	    dn_name;       /*key field */
	SaUint32T	    opr_type;
	SaPlmChangeStepT    change_step;    /*validate/start/completed*/
	SaNtfIdentifierT    root_correlation_id;
	struct plms_ckpt_track_step_info *next;
} PLMS_CKPT_TRACK_STEP_INFO;

/* Data structure to group entity list per track info */
typedef  struct plms_ckpt_entity_list
{
	SaNameT   entity_name;
	SaNameT   root_entity_name;
	struct plms_ckpt_entity_list *next;		
} PLMS_CKPT_ENTITY_LIST;

/* PLMS_CKPT_ENTITY_GROUP_INFO holds the entity group info needed 
to checkpoint */
typedef  struct  plms_ckpt_entity_group_info
{ 
	SaPlmEntityGroupHandleT   entity_group_handle;   /* Key field */ 
	MDS_DEST                  agent_mdest_id;
	PLMS_CKPT_ENTITY_LIST     *entity_list;
	SaUint8T                  track_flags;
	SaUint64T                 track_cookie; 
	struct  plms_ckpt_entity_group_info *next;
} PLMS_CKPT_ENTITY_GROUP_INFO;

typedef struct  plms_entity PLMS_ENTITY;

typedef struct plms_domain 
{
	SaPlmDomainT 	domain;
	PLMS_ENTITY 	*leftmost_child;
}PLMS_DOMAIN;

typedef struct plms_dependency 
{
	SaNameT			dn_name;
	SaUint32T		no_of_dep;
	SaPlmDependencyT        dependency;
	struct plms_dependency	*next;
}PLMS_DEPENDENCY;


/* PLMS_CLIENT_DOWN_LIST holds the database of all clients for database is not
 yet clened at Standby PLMS*/
typedef  struct plms_client_down_list
{
	SaPlmHandleT                 plm_handle;         /* Key field */
	MDS_DEST                     mdest_id;
	struct plms_client_down_list *next;
}PLMS_CKPT_CLIENT_INFO_LIST; 

typedef enum plms_hpi_support_model
{
	PLMS_HPI_SUPPORTED_SYSTEM,
	PLMS_HPI_NOT_SUPPORTED_SYSTEM,
	PLMS_HPI_DEFAULT_SUPPORT
} PLMS_HPI_SUPPORT_MODEL;

typedef struct
{
	SaStringT safHpiCfg;
	PLMS_HPI_SUPPORT_MODEL  hpi_support;
	SaHpiTimeT        extr_pending_timeout;
        SaHpiTimeT        insert_pending_timeout;

} PLMS_HPI_CONFIG;


typedef struct plms_cb
{             
        SaNameT                   comp_name;     /* Component name - "PLMS"               */
	SaStringT                 my_entity_path;
        SaAmfHAStateT             ha_state;      /* present AMF HA state of the component */
	bool                  csi_assigned;
	SaHpiDomainIdT     	  domain_id;
	PLMS_HPI_CONFIG           hpi_cfg;
	SaUint8T		  hpi_intf_up;
	void                      *hpi_intf_hdl;
	SaUint8T		  plmc_initialized;
        SaUint32T		  mds_hdl;
        MDS_DEST                  mdest_id;
	MDS_DEST		  hrb_dest;	
	V_DEST_RL                 mds_role;
	SaUint32T		  plms_self_id;
	SaUint32T		  plms_remote_id;
	NCS_NODE_ID               node_id;

	SaAmfHandleT              amf_hdl;       /*AMF handle, obtained thru AMF init */ 
	SaInvocationT             amf_invocation_id;
	bool                  is_quisced_set;
	SaImmHandleT              imm_hdl;       /* IMM handle, obtained thr IMM init*/   
	SaImmOiHandleT            oi_hdl;        /* Object Implementer handle */
	SaNtfHandleT              ntf_hdl;       /* NTF handle, obtained thr NTF init*/
        NCS_MBCSV_HDL             mbcsv_hdl;
        NCS_MBCSV_CKPT_HDL        mbcsv_ckpt_hdl;
	EDU_HDL                   edu_hdl;

        SYSF_MBX                  mbx;
        SaSelectionObjectT        amf_sel_obj; /* Selection Object for AMF events */
        SaSelectionObjectT        imm_sel_obj; /*Selection object to wait for IMM events */
        SaSelectionObjectT        mbcsv_sel_obj;
	NCS_SEL_OBJ               usr1_sel_obj; /* selection object for usr1 signal*/ 
	bool                  healthCheckStarted;

        /* Config database */
        PLMS_DOMAIN		   domain_info;
        NCS_PATRICIA_TREE          entity_info;       /*  PLMS_ENTITY*/
        NCS_PATRICIA_TREE          base_he_info;      /*  PLMS_HE_BASE_INFO */
        NCS_PATRICIA_TREE          base_ee_info;      /*  PLMS_EE_BASE_INFO */

        PLMS_DEPENDENCY		   *dependency_list;

	NCS_PATRICIA_TREE          epath_to_entity_map_info;    
	NCS_PATRICIA_TREE          client_info; 
	NCS_PATRICIA_TREE          entity_group_info;  
		
	SaUint32T		   is_initialized;
	SaUint32T		   async_update_cnt;
	PLMS_CKPT_TRACK_STEP_INFO  *step_info;
	PLMS_CKPT_ENTITY_GROUP_INFO   *ckpt_entity_grp_info;
	PLMS_CKPT_CLIENT_INFO_LIST    *client_down_list;
	PLMS_CKPT_TRACK_STEP_INFO *prev_trk_step_rec;
        SaPlmEntityGroupHandleT   prev_ent_grp_hdl;
	SaPlmHandleT              prev_client_info_hdl;
	NCS_LOCK                   cb_lock;
	bool                  client_info_cold_sync_done;
        bool                  entity_grp_cold_sync_done;
        bool                  trk_step_info_cold_sync_done;
	bool nid_started;	/**< true if started by NID */
} PLMS_CB;

typedef enum
{
	PLMS_HE_ENTITY,
	PLMS_EE_ENTITY
} PLMS_ENTITY_TYPE;



/* HPI Hot Swap State Models supported by PLM */ 
typedef enum plms_hpi_state_model
{
	PLMS_HPI_TWO_HOTSWAP_MODEL = 2,
	PLMS_HPI_THREE_HOTSWAP_MODEL,
	PLMS_HPI_PARTIAL_FIVE_HOTSWAP_MODEL,      
        PLMS_HPI_FULL_FIVE_HOTSWAP_MODEL      
} PLMS_HPI_STATE_MODEL;


/* Data structure to group plm entities per entity  group */
typedef  struct plms_group_entity   
{
	PLMS_ENTITY               *plm_entity;
	struct plms_group_entity  *next;
} PLMS_GROUP_ENTITY;

/* Data structure to group plm entities per entity  group */
typedef  struct plms_group_entity_root_list   
{
	PLMS_ENTITY               *plm_entity;
	PLMS_ENTITY               *subtree_root;
	struct plms_group_entity_root_list  *next;
} PLMS_GROUP_ENTITY_ROOT_LIST;


typedef struct plms_invocation_to_track_info PLMS_INVOCATION_TO_TRACK_INFO;
typedef struct  plms_client_info PLMS_CLIENT_INFO;
typedef  struct  plms_entity_group_info
{
	NCS_PATRICIA_NODE             pat_node; 
	SaPlmEntityGroupHandleT       entity_grp_hdl;   /* Key field */ 
	MDS_DEST                      agent_mdest_id;
	PLMS_GROUP_ENTITY_ROOT_LIST   *plm_entity_list;
	PLMS_CLIENT_INFO              *client_info;
	SaPlmGroupOptionsT            options;
	SaUint8T                      track_flags;
	SaUint64T                     track_cookie; 
	PLMS_INVOCATION_TO_TRACK_INFO  *invocation_list;
	SaUint8T                      invoke_callback;
} PLMS_ENTITY_GROUP_INFO;

/* Data structure to group entity list per track info */
typedef  struct plms_entity_group_info_list
{
	PLMS_ENTITY_GROUP_INFO                *ent_grp_inf;
	struct plms_entity_group_info_list    *next;		
} PLMS_ENTITY_GROUP_INFO_LIST;

/* Data structure to hold track info pertaining to particular operation */
typedef struct plms_track_info
{
	SaUint64T                   imm_adm_opr_id; 
	PLMS_ENTITY                 *root_entity;
	SaUint32T		    track_count;
	SaPlmChangeStepT            change_step;    /*validate/start/completed*/
	SaPlmTrackCauseT            track_cause; 
	SaNtfIdentifierT            root_correlation_id; 
	PLMS_ENTITY_GROUP_INFO_LIST *group_info_list;          
	PLMS_GROUP_ENTITY	    *aff_ent_list;
	SaInvocationT		    inv_id;
	SaPlmGroupChangesT	    grp_op;			
} PLMS_TRACK_INFO;

/* Data structure to map invocation to trackinfo */
struct plms_invocation_to_track_info
{
	SaInvocationT               invocation;
	PLMS_TRACK_INFO              *track_info;
	struct plms_invocation_to_track_info  *next;
} plms_invocation_to_track_info;


typedef struct plms_he_type_info
{
	SaPlmHETypeT          he_type;
	PLMS_INV_DATA         inv_data;
	struct plms_he_type_info *next;
} PLMS_HE_TYPE_INFO;
 

typedef struct
{
	NCS_PATRICIA_NODE        pat_node;
	SaNameT			 dn_name;
	SaPlmHEBaseTypeT         he_base_type;   /*key field */
	PLMS_HE_TYPE_INFO        *he_type_info_list; 
} PLMS_HE_BASE_INFO;

typedef struct plms_ee_type_info PLMS_EE_TYPE_INFO;

typedef struct 
{
	NCS_PATRICIA_NODE     pat_node;
	SaNameT		      dn_name;
	SaPlmEEBaseTypeT      ee_base_type;      /*Key field */
	PLMS_EE_TYPE_INFO      *ee_type_info_list;
} PLMS_EE_BASE_INFO;

struct plms_ee_type_info
{
	SaPlmEETypeT          ee_type;               /*Key field */ 
	PLMS_EE_BASE_INFO      *parent; 
	struct  plms_ee_type_info       *next;
};

typedef enum{
    PLMS_ISO_DEFAULT,
    PLMS_ISO_EE_TERMINATED,
    PLMS_ISO_HE_DEACTIVATED,
    PLMS_ISO_HE_RESET_ASSERT
}PLMS_ISOLATION_METHOD;

typedef enum{
	PLMS_MNGT_NONE,
	PLMS_MNGT_EE_LOCK,
	PLMS_MNGT_EE_UNLOCK,
	PLMS_MNGT_EE_TERM,
	PLMS_MNGT_EE_RESTART,
	PLMS_MNGT_EE_GET_OS_INFO,
	PLMS_MNGT_EE_INST,
	PLMS_MNGT_EE_ISOLATE,

	PLMS_MNGT_HE_DEACTIVATE,
	PLMS_MNGT_HE_ACTIVATE,
	PLMS_MNGT_HE_RESET_ASSERT,
	PLMS_MNGT_HE_RESET_DEASSERT,
	PLMS_MNGT_HE_COLD_RESET,
	PLMS_MNGT_HE_WARM_RESET,
	PLMS_MNGT_HE_ISOLATE,
	PLMS_MNGT_LOST_MAX
}PLMS_MNGT_LOST_TRIGGER;


/* PLM_ENTITY is the wrapper structure for SaPlmHeT and SaPlmEeT defined by the spec
   It contains other common fields related to HE and EE. It also contains parent_list 
   and child_list which will help in building containment hierarchy, dependency_list
   points to the list of entities that this entity is dependent on and rev_dependency_list
   points to the list of entities that are dependent on this entity
*/
struct  plms_entity
{
	NCS_PATRICIA_NODE        pat_node;
	SaNameT                  dn_name;       /*key field */
	SaStringT                dn_name_str;
	PLMS_ENTITY_TYPE         entity_type;
	union 
	{
		SaPlmHeT        he_entity;
		SaPlmEeT        ee_entity;
	}entity;

	SaUint32T               num_cfg_ent_paths;
	SaPlmReadinessStatusT   exp_readiness_status;
	PLMS_ENTITY_GROUP_INFO_LIST       *part_of_entity_groups;
	/* Not needed.*/
	SaBoolT                 invoke_callback;
	SaNtfIdentifierT        first_plm_ntf_id; 
	PLMS_TMR_EVT		tmr;

	PLMS_HPI_STATE_MODEL    state_model;    /* Hotswap State Model */
	SaUint32T               plmc_addr;      /* socket_id */  
	SaPlmTrackCauseT        adm_op_in_progress;
	SaInvocationT		imm_inv_id;
	PLMS_ENTITY_GROUP_INFO_LIST	*cbk_grp_list;
	SaUint32T               am_i_aff_ent;

	/* Dependency list */
	SaUint32T		min_no_dep;
	PLMS_GROUP_ENTITY	*dependency_list;
	PLMS_GROUP_ENTITY	*rev_dep_list;
	PLMS_TRACK_INFO		*trk_info;
	PLMS_ISOLATION_METHOD   iso_method;
	PLMS_MNGT_LOST_TRIGGER  mngt_lost_tri;
	SaUint32T               deact_in_pro;
	SaUint32T               act_in_pro;
	SaUint32T		ee_sock_ff;

	/* parent-Child entity pointers to store the containment hierarchy */
	struct  plms_entity     *parent;
	struct  plms_entity     *leftmost_child;
	struct  plms_entity     *right_sibling;
};




/* PLMS_CLIENT_INFO holds the database of all clients that have subscribed with PLM */
struct plms_client_info
{
	NCS_PATRICIA_NODE            pat_node; 
	SaPlmHandleT                 plm_handle;         /* Key field */
	MDS_DEST                     mdest_id;
	PLMS_ENTITY_GROUP_INFO_LIST  *entity_group_list;
};

/* Data structure to map entity path of HPI event to PLM entity */
typedef struct plms_epath_to_entity_map_info
{
	NCS_PATRICIA_NODE           pat_node;
	SaHpiEntityPathT 	    epath_key;
	SaInt8T		    	    *entity_path;    /* Key field  */
	PLMS_ENTITY                 *plms_entity;
} PLMS_EPATH_TO_ENTITY_MAP_INFO;




PLMS_CB *plms_cb;
HSM_HA_STATE hsm_ha_state;
HRB_HA_STATE hrb_ha_state;

typedef SaUint32T (*PLMS_ADM_FUNC_PTR) (PLMS_EVT *);
typedef SaUint32T (*PLMS_PRES_FUNC_PTR) (PLMS_EVT *);

/* Function declarations from plms_he_pres_fsm.c*/
SaUint32T plms_he_activate(PLMS_ENTITY *,SaUint32T, SaUint32T);
SaUint32T plms_he_deactivate(PLMS_ENTITY *,SaUint32T,SaUint32T);
SaUint32T plms_he_reset(PLMS_ENTITY *,SaUint32T,SaUint32T,SaUint32T);
SaUint32T plms_he_oos_to_np_process(PLMS_ENTITY *);
SaUint32T plms_he_insvc_to_np_process(PLMS_ENTITY *);
SaUint32T plms_hpi_hs_evt_process(PLMS_EVT *);

/* Function declaration from plms_plmc.c*/
SaUint32T plms_ee_unlock(PLMS_ENTITY *,SaUint32T,SaUint32T);
SaUint32T plms_ee_term(PLMS_ENTITY *,SaUint32T,SaUint32T);
SaUint32T plms_ee_lock(PLMS_ENTITY *,SaUint32T,SaUint32T);
SaUint32T plms_ee_reboot(PLMS_ENTITY *,SaUint32T,SaUint32T);
SaUint32T plms_ee_instantiate(PLMS_ENTITY *,SaUint32T,SaUint32T);
SaUint32T plms_mbx_tmr_handler(PLMS_EVT *);
SaUint32T plms_plmc_mbx_evt_process(PLMS_EVT *);
SaUint32T plms_isolate_and_mngt_lost_clear(PLMS_ENTITY *);
int32_t plms_plmc_error_cbk(plmc_lib_error *);
int32_t plms_plmc_connect_cbk(char *,char *);
int32_t plms_plmc_udp_cbk(udp_msg *);

/* Function declaration from plms_adm_fsm.c*/
SaUint32T plms_cbk_call(PLMS_TRACK_INFO *,SaUint8T);
SaUint32T plms_imm_adm_op_req_process(PLMS_EVT *);
SaUint32T plms_cbk_response_process(PLMS_EVT *);
void plms_deact_completed_cbk_call(PLMS_ENTITY *,PLMS_TRACK_INFO *);
void plms_deact_start_cbk_call(PLMS_ENTITY *,PLMS_TRACK_INFO *);

/* Function declaration from plms_utils.c*/
SaUint32T plms_readiness_impact_process(PLMS_EVT *);
SaUint32T plms_timer_start(timer_t *,void *,SaUint64T);
SaUint32T plms_timer_stop(PLMS_ENTITY *);


/* Function declaration from plms_imm.c */
void plms_get_str_from_dn_name(SaNameT *, SaStringT);

/* Function Declarations */
extern uint32_t plms_edp_plms_evt(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
                        NCSCONTEXT ptr, uint32_t *ptr_data_len,
	                EDU_BUF_ENV *buf_env, EDP_OP_TYPE op,
			         EDU_ERR *o_err);
SaUint32T plms_amf_register();
SaUint32T plms_mds_register();
void plms_mds_unregister();
SaUint32T plms_mds_change_role();
SaUint32T plms_mds_enc(MDS_CALLBACK_ENC_INFO *info, EDU_HDL *edu_hdl);
SaUint32T plms_mds_dec(MDS_CALLBACK_DEC_INFO *info, EDU_HDL *edu_hdl);
SaUint32T plms_mds_enc_flat(struct ncsmds_callback_info *, EDU_HDL *);
SaUint32T plms_mds_dec_flat(struct ncsmds_callback_info *, EDU_HDL *);
SaUint32T plm_send_mds_rsp(MDS_HDL mds_hdl,
                        NCSMDS_SVC_ID  from_svc,
			PLMS_SEND_INFO *s_info,
			PLMS_EVT       *evt);
void  plms_cb_dump_routine();
SaUint32T  plms_free_evt(PLMS_EVT *evt);
SaUint32T plms_cbk_validate_resp_ok_err_proc(PLMS_TRACK_INFO *);
SaUint32T plms_cbk_start_resp_ok_err_proc(PLMS_TRACK_INFO *);
uint32_t plms_mds_normal_send(MDS_HDL, NCSMDS_SVC_ID,NCSCONTEXT, MDS_DEST,
						NCSMDS_SVC_ID);
SaUint32T plms_hrb_mds_msg_sync_send(MDS_HDL mds_hdl,
					SaUint32T from_svc,
					SaUint32T to_svc,
					MDS_DEST    to_dest,
					PLMS_HPI_REQ *i_evt,
					PLMS_HPI_RSP **o_evt,
					SaUint32T timeout);
SaUint32T plms_proc_standby_active_role_change();
void plms_proc_quiesced_standby_role_change();
void plms_proc_quiesced_active_role_change();
void plms_proc_active_quiesced_role_change();
SaUint32T plms_proc_quiesced_ack_evt();
SaUint32T plms_imm_intf_initialize();
SaUint32T plms_read_hpi_config();
void plms_clean_agent_db(MDS_DEST agent_mdest_id,SaAmfHAStateT ha_state);
SaUint32T convert_string_to_epath(SaInt8T *epath_str,
                                      SaHpiEntityPathT *epath_ptr);
void plms_ee_adm_fsm_init(PLMS_ADM_FUNC_PTR plm_EE_adm_state_op[]
                                                [SA_PLM_ADMIN_OP_MAX]);
void plms_he_adm_fsm_init(PLMS_ADM_FUNC_PTR plm_HE_adm_state_op[]
                                                [SA_PLM_ADMIN_OP_MAX]);
void plms_he_pres_fsm_init(PLMS_PRES_FUNC_PTR plms_HE_pres_state_op[]
                                                [SA_PLM_HPI_HE_PRES_STATE_MAX]);
SaUint32T plms_tmr_handler_install();
SaUint32T plms_hsm_hrb_init();

SaUint64T  plm_handle_pool;
SaUint64T  entity_grp_hdl_pool;
void plm_imm_reinit_bg(PLMS_CB *cb);
SaUint32T plms_build_epath_to_entity_map_tree();
#endif   /* PLMS_H */
