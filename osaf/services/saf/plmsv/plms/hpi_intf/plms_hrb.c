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
*                                                                            *
*  MODULE NAME:  plms_hrb.c                                                   *
*                                                                            *
*                                                                            *
*  DESCRIPTION                                                               *
*  This module contains the functionality of HPI Session Manager.            *
*  HSM is component of PLM and it blocks to receive the HPI events on open   *
*  HPI session. It publishes the received HPI events to PLM main thread      *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
 *   INCLUDE FILES
*****************************************************************************/
#include <pthread.h>

#include "ncssysf_def.h"

#include "plms.h"
#include "plms_hrb.h"
#include "plms_hsm.h"
#include "plms_evt.h"

static PLMS_HRB_CB _hrb_cb;
PLMS_HRB_CB     *hrb_cb = &_hrb_cb;

/***********************************************************************
*   FUNCTION PROTOTYPES
***********************************************************************/
SaUint32T  plms_hrb_initialize();
SaUint32T  plms_hrb_finalize();
static bool hrb_cleanup_mbx(NCSCONTEXT arg, NCSCONTEXT msg);
static void *plms_hrb();
static SaUint32T hrb_process_hpi_req( PLMS_HPI_REQ  *hpi_req );
static SaUint32T hrb_get_resourceid(SaInt8T *epath_str, 
				   SaUint32T epath_len,
				   SaHpiRptEntryT    *rpt_entry,
				   SaHpiResourceIdT  *resourceid);

/***********************************************************************
* Name          : plms_hrb_initialize
*
* Description   : This function initializes the HRB control block
*                 and creates plms_hrb thread, It registers to MDS  
*		  to receive requests from PLMS
*
* Arguments     : 
*
* Return Values : NCSCC_RC_SUCCESS
*		  NCSCC_RC_FAILURE
***********************************************************************/
SaUint32T plms_hrb_initialize()
{
	PLMS_HRB_CB      *cb =  hrb_cb;
	pthread_t        thread_id;
	pthread_attr_t   attr;
	struct sched_param thread_priority;
	SaUint32T        policy;
	SaUint32T	 rc;

	TRACE_ENTER();

	/* create the mail box and attach it */
	if ((rc = m_NCS_IPC_CREATE(&cb->mbx)) != NCSCC_RC_SUCCESS){
		LOG_ER("error creating mail box err val:%d",rc);
		return NCSCC_RC_FAILURE;
	}

	if ((rc = m_NCS_IPC_ATTACH(&cb->mbx)) != NCSCC_RC_SUCCESS){
		LOG_ER("error attaching mail box err val:%d",rc);
		return NCSCC_RC_FAILURE;
	}

	/* Initialize with the MDS */
	if(hrb_mds_initialize() != NCSCC_RC_SUCCESS){
		LOG_ER("HRB: mds initialization failed");
		return NCSCC_RC_FAILURE;
	}

	/* Initialize thread attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_attr_setstacksize(&attr, PLMS_HRB_STACKSIZE);

	/* Create PLMS_HRB thread */
	rc = pthread_create(&thread_id, &attr, plms_hrb, NULL);
	if(rc){
		LOG_ER("pthread_create FAILED ret code:%d error:%s",
				rc,strerror(errno));
		return NCSCC_RC_FAILURE;
	}
	
	/*scheduling parameters of the thread */
        memset(&thread_priority, 0, sizeof(thread_priority));
	thread_priority.sched_priority = PLMS_HRB_TASK_PRIORITY;
	policy = SCHED_OTHER;
	pthread_setschedparam(thread_id, policy, &thread_priority);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/***********************************************************************
* Name          : plms_hrb_finalize
*
* Description   : This function un-registers with  MDS, detaches mailbox 
*                 and terminates the HRB thread
*
* Arguments     : 
*
* Return Values :NCSCC_RC_SUCCESS
*		 NCSCC_RC_FAILURE
*
* Notes         : None.
***********************************************************************/
SaUint32T plms_hrb_finalize()
{
	PLMS_HRB_CB *cb = hrb_cb;
	SaUint32T   rc;

	/* detach the mail box */
        if ((rc = m_NCS_IPC_DETACH(&cb->mbx, hrb_cleanup_mbx, NULL)) 
		!= NCSCC_RC_SUCCESS)
		LOG_ER("m_NCS_IPC_DETACH failed with error:%d",rc);

        /* delete the mailbox */
        if ((rc = m_NCS_IPC_RELEASE(&cb->mbx, NULL)) != NCSCC_RC_SUCCESS)
		LOG_ER("HRB:m_NCS_IPC_DETACH failed with error:%d",rc);

	/* derigister with the MDS */	
        if ((rc = hrb_mds_finalize()) != NCSCC_RC_SUCCESS)
		LOG_ER("hrb_mds_finalize failed with error:%d",rc);
		

	/* Kill the HRB thread */
	pthread_cancel(cb->thread_id);

	TRACE("exiing plms_hrb_finalize with ret value:%d",rc);

        return NCSCC_RC_SUCCESS;
}
/****************************************************************************
 * Name          : hrb_cleanup_mbx
 *
 * Description   : This is the function which deletes all the messages from
 *                 the mail box.
 *
 * Arguments     : arg     - argument to be passed.
 *                 msg     - Event start pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS
 *		   NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static bool hrb_cleanup_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	PLMS_HPI_REQ   *req = (PLMS_HPI_REQ *)msg;
        PLMS_HPI_REQ   *next_req = req;

        while (next_req) {
                next_req = req->next;
                free(req);
                req = next_req;
        }
        return true;
}
/***************************************************************************
 * Name          : plms_hrb
 *
 *
 * Description :  This thread forwards the HPI request sent from PLM to HPID
 *                This thread is spawned by PLM main task.It receives requests 
 *                from PLM main thread through MDS and  invokes appropriate  
 *                HPI call
 *
 * Arguments     : 
 * Return Values : 
*****************************************************************************/
static void *plms_hrb(void)
{
	PLMS_HRB_CB   *cb = hrb_cb;
	PLMS_HPI_REQ   *hpi_req = NULL;
	NCS_SEL_OBJ    mbx_fd = m_NCS_IPC_GET_SEL_OBJ(&cb->mbx);
	NCS_SEL_OBJ_SET     sel_obj;

	TRACE_ENTER();

	m_NCS_SEL_OBJ_ZERO(&sel_obj);
	m_NCS_SEL_OBJ_SET(mbx_fd,&sel_obj);
	
	/* Wait on condition variable for the HA role from PLMS main thread */
	pthread_mutex_lock(&hrb_ha_state.mutex);
	if(hrb_ha_state.state != SA_AMF_HA_ACTIVE){
		TRACE("hrb waiting on cond variable for Active state");
		pthread_cond_wait(&hrb_ha_state.cond,&hrb_ha_state.mutex);
	}
	pthread_mutex_unlock(&hrb_ha_state.mutex);

	while (m_NCS_SEL_OBJ_SELECT(mbx_fd,&sel_obj,0,0,0) != -1){
		
		/* process the Mail box */
		if (m_NCS_SEL_OBJ_ISSET(mbx_fd,&sel_obj)){
			/* Process messages delivered on mailbox */ 
			while(NULL != (hpi_req =
		(PLMS_HPI_REQ *)m_NCS_IPC_NON_BLK_RECEIVE(&cb->mbx,hpi_req))){
				/*Process the received message*/ 
				hrb_process_hpi_req(hpi_req); 

				if(hpi_req->entity_path)
					free(hpi_req->entity_path);
				free(hpi_req);
			}
					  

		}

		/* do the fd set for the select obj */
		m_NCS_SEL_OBJ_SET(mbx_fd,&sel_obj);

		if(hrb_ha_state.state != SA_AMF_HA_ACTIVE){
			/* Wait on condition variable for the HA role from 
			PLMS main thread */
			LOG_NO("HRB:Received Standby state, thread going \
				     to block till Active state is set");
			pthread_mutex_lock(&hrb_ha_state.mutex);
			if(hrb_ha_state.state != SA_AMF_HA_ACTIVE){
				TRACE("hrb waiting on cond variable for \
				Active state");
				pthread_cond_wait(&hrb_ha_state.cond, 
						&hrb_ha_state.mutex);
			}
			pthread_mutex_unlock(&hrb_ha_state.mutex);
		}
	}
	TRACE_LEAVE();
	return NULL;
}

/**********************************************************************
* Name          : hrb_process_hpi_req
*
* Description   : 
*
*
* Arguments     : plms_hpi_req (Input)
*
* Return Values : NCSCC_RC_SUCCESS
*                 NCSCC_RC_FAILURE
* Notes         : 
***********************************************************************/
static SaUint32T hrb_process_hpi_req( PLMS_HPI_REQ  *hpi_req )
{
	PLMS_HRB_CB        *cb = hrb_cb;
	SaHpiResourceIdT   resourceid = 0;
	PLMS_HPI_RSP       *response = NULL;
	PLMS_INV_DATA  	   inv_data; 
	SaHpiRptEntryT     rpt_entry;
	SaUint32T	   rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	if( NULL == hpi_req ){
		LOG_ER("HRB: Invalid request fromPLMS");
		return NCSCC_RC_FAILURE;
	}
	if( NULL == hpi_req->entity_path ){
		LOG_ER("HRB:Invalid entity_path in the hpi_req");
		return NCSCC_RC_FAILURE;
	}
	
	TRACE("Processing hpi request for res:%s cmd:%d",
			hpi_req->entity_path,hpi_req->cmd);
	/*Get Resource_id for the corresponding entity_path*/
	if(NCSCC_RC_FAILURE == hrb_get_resourceid(hpi_req->entity_path,
						hpi_req->entity_path_len,
						&rpt_entry,&resourceid))
		return NCSCC_RC_FAILURE;

	response = (PLMS_HPI_RSP *)malloc(sizeof(PLMS_HPI_RSP));
	if(NULL == response){
		LOG_ER("HRB:PLMS_HPI_RSP memory alloc failed error val:%s",
							strerror(errno));
		assert(0);
	}

	memset(response,0,sizeof(PLMS_HPI_RSP));

	switch (hpi_req->cmd)
	{
#if 0
		case PLMS_HPI_CMD_STANDBY_SET:
			/* Wait on condition variable for the HA role from 
			PLMS main thread */
			LOG_NO("HRB:Received Standby state, thread going \
				     to block till Active state is set");
			pthread_mutex_lock(&hrb_ha_state.mutex);
			if(hrb_ha_state.state != SA_AMF_HA_ACTIVE){
				TRACE("hrb waiting on cond variable for \
				Active state");
				pthread_cond_wait(&hrb_ha_state.cond, 
						&hrb_ha_state.mutex);
			}
			pthread_mutex_unlock(&hrb_ha_state.mutex);

			break; 
#endif

		case PLMS_HPI_CMD_ACTION_REQUEST:
			/* insertion or extraction request depending upon 
			the hpi_req->arg  type */
			rc = saHpiHotSwapActionRequest(cb->session_id,
						      resourceid,hpi_req->arg);
			break; 

		case PLMS_HPI_CMD_RESOURCE_ACTIVE_SET:
			/* Transition from INSERTION PENDING or 
			EXTRACTION PENDING state to ACTIVE */ 
			rc = saHpiResourceActiveSet(cb->session_id,
						   resourceid);
			break;

		case PLMS_HPI_CMD_RESOURCE_INACTIVE_SET:
			/* Transition from INSERTION PENDING or 
			EXTRACTION PENDING state to INACTIVE */
			rc =  saHpiResourceInactiveSet(cb->session_id,
							resourceid);
			break;

		case PLMS_HPI_CMD_RESOURCE_POWER_ON:
		case PLMS_HPI_CMD_RESOURCE_POWER_OFF:
			/* Perform power control operation
			be based on the value of hpi_req->arg*/ 
			rc = saHpiResourcePowerStateSet(cb->session_id,
						resourceid, hpi_req->arg); 
			break;

		case PLMS_HPI_CMD_RESOURCE_RESET:
			rc = saHpiResourceResetStateSet(
				cb->session_id,
				resourceid, 
				(SaHpiResetActionT)hpi_req->arg);
			if (SA_OK != rc && hpi_req->arg == SAHPI_WARM_RESET ){
				TRACE("warm reset failed ret val:%u so \
					attempting cold reset",rc);
				rc = saHpiResourceResetStateSet(
						cb->session_id,
						resourceid,
						(SaHpiResetActionT)
						SAHPI_COLD_RESET);
				TRACE("cold reset saHpiResourceResetStateSet \
					ret val:%u",rc);
			}
			break;

		case PLMS_HPI_CMD_RESOURCE_IDR_GET:
			/* Retrieve IDR info */
			rc = hsm_get_idr_info(&rpt_entry,&inv_data); 
			break;

		default:
			LOG_ER("HRB:Invalid request from PLMS");
			return NCSCC_RC_FAILURE;
	}
	TRACE("Processing Status for hpi request for res:%u HPI resp:%u",
			resourceid,rc);

	if(rc)
		 response->ret_val = NCSCC_RC_FAILURE; 
	else
		response->ret_val  = NCSCC_RC_SUCCESS; 

	rc = hrb_mds_msg_send(response, hpi_req->mds_context);
	
	TRACE_LEAVE2("RET VAL:%d",rc);
	return rc;
}

/***********************************************************************
* Name          : hrb_get_resourceid
*
* Description   : This function gets the resource-id of the resource given
*                         its entity path.
*
* Arguments     : epath_str(Input)
*		  epath_len(Input)
*		  rpt_entry(output)
*		  resourceid(Output)
*
* Return Values :  NCSCC_RC_SUCCESS
*		   NCSCC_RC_FAILURE.
*
* Notes         : 
***********************************************************************/
static SaUint32T hrb_get_resourceid(SaInt8T *epath_str, 
				   SaUint32T epath_len,
				   SaHpiRptEntryT    *entry,
				   SaHpiResourceIdT  *resourceid)
{
	PLMS_HRB_CB        *cb = hrb_cb;
	SaHpiEntityPathT  epath;
	SaHpiEntryIdT     current, next;
	SaHpiRptEntryT    rpt_entry;
	SaUint32T	  rc = NCSCC_RC_SUCCESS;

	if( NULL == epath_str){
		LOG_ER("Invalid epath_str to hrb_get_resourceid");
		return NCSCC_RC_FAILURE;
	}
	/* convert the canonical entity path string to entity path structure */
	if (NCSCC_RC_FAILURE == convert_string_to_epath(epath_str, 
						    &epath)){
		LOG_ER("HSM: resource does not exist for given entity_path");
		return NCSCC_RC_FAILURE;
	}

	/* Iterate through the RPT table to get the resource_id of the 
	corresponding entity_path */ 
	next = SAHPI_FIRST_ENTRY;
	do{
		current = next;
		rc = saHpiRptEntryGet(cb->session_id, current,
				&next, &rpt_entry);
		if (SA_OK != rc){
			LOG_ER("HSM:saHpiRptEntryGet failed, ret val\
                                                        is:%d",rc);
			/* TBD, can we rediscover? */
                                 break;
                }

		/* got the entry, check for matching entity path */
		if (memcmp(&epath, (int8_t *)&rpt_entry.ResourceEntity.Entry,
			sizeof(SaHpiEntityT))){
			continue;
		}
		else{
			*resourceid = rpt_entry.ResourceId;
			entry = &rpt_entry;
			break;
		}  
	} while (next != SAHPI_LAST_ENTRY);

	return rc;
}
