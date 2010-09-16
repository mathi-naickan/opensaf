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

/*************************************************************************//**
*  @file	plms_hsm.c                                                   
*  @brief	This module contains the functionality of HPI Session Manager
*		HSM is component of PLMS and it blocks to receive the HPI   
*               events on open HPI session. It publishes the received HPI    
*    		events to PLM main thread				     
*									    
*  @author      Emerson Network Power					    
* 
*****************************************************************************/

/*******************************************************************//**
 *   INCLUDE FILES
***********************************************************************/
#include <pthread.h>

#include "ncssysf_def.h"

#include "plms.h"
#include "plms_evt.h"
#include "plms_hsm.h"
#include "plms_hrb.h"
#include <SaHpi.h>

static PLMS_HSM_CB _hsm_cb;
PLMS_HSM_CB     *hsm_cb = &_hsm_cb;

/***********************************************************************
*   FUNCTION PROTOTYPES
***********************************************************************/
SaUint32T plms_hsm_initialize(PLMS_HPI_CONFIG *hpi_cfg);
SaUint32T plms_hsm_finalize(void);
static SaUint32T hsm_get_hotswap_model(SaHpiRptEntryT *rpt_entry,
                             	       SaUint32T      *hotswap_state_model); 
static SaUint32T hsm_send_hotswap_event(SaHpiRptEntryT   *rpt_entry, 
					SaUint32T hotswap_state_model, 
					SaHpiHsStateT hotswap_state,
					SaHpiHsStateT prev_hotswap_state,
					SaUint32T  retriev_idr_info); 
SaUint32T hsm_correct_length(SaHpiIdrFieldT *thisfield);
SaUint32T hsm_get_idr_info(SaHpiRptEntryT  *rpt_entry, 
			   PLMS_INV_DATA  *inv_data);
static SaUint32T hsm_get_idr_product_info(SaHpiRptEntryT  *rpt_entry,
                                	SaHpiIdrIdT       idr_id,
					PLMS_INV_DATA     *inv_data);
static SaUint32T hsm_get_idr_board_info(SaHpiRptEntryT    *rpt_entry, 
					SaHpiIdrIdT       idr_id,
					PLMS_INV_DATA     *inv_data);
static SaUint32T hsm_get_idr_chassis_info(SaHpiRptEntryT  *rpt_entry, 
				 	SaHpiIdrIdT 	  idr_id,
				 	PLMS_INV_DATA     *inv_data);
static SaUint32T hsm_session_reopen();
SaUint32T plms_hsm_session_close();
static SaUint32T hsm_discover_and_dispatch();
static void *plms_hsm();

/*******************************************************************//**
* @brief	 This function initializes the HSM control block
*                structure and	opens a session with HPI.It creates
*                and starts plm_hsm thread
*
* @param[in]	 hpi_cfg	
*
* @return	 NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
***********************************************************************/
SaUint32T plms_hsm_initialize(PLMS_HPI_CONFIG *hpi_cfg)
{
	PLMS_HSM_CB        *cb = hsm_cb;
	SaHpiSessionIdT    session_id = 0;
	SaUint32T	   rc, retry = 0;
	pthread_t 	   thread_id;
	pthread_attr_t 	   attr;
	struct sched_param thread_priority;
	SaUint32T          policy;

	TRACE_ENTER();
	
	/* Open a session on the SAHPI_UNSPECIFIED_DOMAIN_ID */
	while(retry++ < PLMS_MAX_HPI_SESSION_OPEN_RETRIES){
		rc = saHpiSessionOpen(SAHPI_UNSPECIFIED_DOMAIN_ID,
				      &session_id, NULL);
		if(SA_OK != rc){
			LOG_ER("SessionOpen failed with err:%d",rc);
			continue;
		}else
			break;
	}
	if (SA_OK != rc){
		LOG_ER("SessionOpen failed with err:%d after retrying%d times",
							rc,retry);
		return NCSCC_RC_FAILURE;
	}else{
		cb->session_id = session_id;

		/* Update the HRB session_id */
		rc = pthread_mutex_lock(&hrb_cb->mutex);
		if(rc){
			LOG_CR("Failed to take HRB lock ret value:%d error:%s",
							rc, strerror(errno));
			assert(0);
		}

		hrb_cb->session_id = session_id;

		rc = pthread_mutex_unlock(&hrb_cb->mutex);
		if(rc){
			LOG_CR("Failed to unlock the HRB lock ret :%d err:%s",
							rc, strerror(errno));
			assert(0);
		}
		TRACE("saHpiSessionOpen success sess_id:%u",session_id);

		plms_cb->domain_id  = SAHPI_UNSPECIFIED_DOMAIN_ID;
	}

	/*TBD, need to recheck the timeout values*/	
	cb->extr_pending_timeout =(SaHpiTimeT)hpi_cfg->extr_pending_timeout * 
								  1000000000;

	cb->insert_pending_timeout=(SaHpiTimeT)hpi_cfg->insert_pending_timeout *
								  1000000000;

	/*Set the auto-insert timeout which is domain specific and applies to
	 * all the reource which support managed hot swap */
	rc = saHpiAutoInsertTimeoutSet(session_id, cb->insert_pending_timeout);
	if (SA_OK != rc){
		if (SA_ERR_HPI_READ_ONLY == rc){
			/* Allow for the case where the insertion timeout is 
			read-only. */
			TRACE("Auto insert timeout is readonly ret value is:%d",
									rc);
		}
		else{
			TRACE("Error in setting Auto Ins timeout ret val:%d", 
									rc);  
		}
	}

	/* Initialize thread attribute */
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&attr, PLMS_HSM_STACKSIZE);

	
	/* Create PLM_HSM thread */
	rc = pthread_create(&thread_id, &attr, plms_hsm, NULL);
	if(rc){
		LOG_CR("HSM pthread_create FAILED ret code:%d error:%s", rc,
						strerror(errno));

		/* Close the HPI session */
		rc = saHpiSessionClose(cb->session_id);
        	if (SA_OK != rc)
			LOG_ER("HSM:Close session return error: %d:\n",rc);

		return NCSCC_RC_FAILURE;
	}
        /* scheduling parameters of the thread */
	memset(&thread_priority, 0, sizeof(thread_priority));
        thread_priority.sched_priority = PLMS_HRB_TASK_PRIORITY;
        policy = SCHED_OTHER;
        pthread_setschedparam(thread_id, policy, &thread_priority);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
/***************************************************************************
 * @brief	Closes HPI session and terminates HSM thread
 *
 * @param[in] 
 *
 * @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE 
 ***************************************************************************/
SaUint32T plms_hsm_finalize(void)
{
	PLMS_HSM_CB     *cb = hsm_cb;
	SaErrorT        rc;

	/* Close the HPI session */
	rc = saHpiSessionClose(cb->session_id);
        if (SA_OK != rc)
		LOG_ER("HSM:Close session return error: %d:\n",rc);


	/* Kill the HSM thread */
	pthread_cancel(cb->threadid);
	
	return NCSCC_RC_SUCCESS;
}    
/***********************************************************************
 * @brief	 This task is spawned by PLM main task.This function 
 *               publishes the outstanding hpi events which apparently 
 *               happened before the session is established.It blocks 
 *               on HPI to receive HPI event and publishes the received
 *               events to PLM main thread
 * @param[in] 
 *                        
 * @return	void	
***********************************************************************/
static void *plms_hsm(void)
{
	PLMS_HSM_CB       *cb = hsm_cb;
	SaHpiRptEntryT    rpt_entry;
	PLMS_HPI_STATE_MODEL  hotswap_state_model;
	SaHpiEventT	  event;
	SaHpiRdrT   	  rdr;
	SaHpiHsStateT  	  state;
	SaUint32T	  retriev_idr_info = 0;
	SaInt32T	  rc,ret;
	SaInt32T	  got_new_active = FALSE;

	TRACE_ENTER();

	rc = pthread_mutex_lock(&hsm_ha_state.mutex);
	if(rc){
                        LOG_CR("HSM: Failed to take hsm_ha_state lock, exiting \
                        the thread, ret value:%d err:%s", rc, strerror(errno));
                        assert(0);
        }
	if(hsm_ha_state.state != SA_AMF_HA_ACTIVE){
		TRACE("HSM: Thread going to block till Active state is set");
		pthread_cond_wait(&hsm_ha_state.cond,&hsm_ha_state.mutex);
	}
	
	rc = pthread_mutex_unlock(&hsm_ha_state.mutex);
	if(rc){
                        LOG_CR("HSM:Failed to unlock hsm_ha_state lock,exiting \
                        the thread, ret value:%d err:%s", rc, strerror(errno));
                        assert(0);
        }

	/* Subscribe to receive events on this HPI session */ 
	rc =  saHpiSubscribe(cb->session_id);
	if( SA_OK != rc ){
		LOG_ER("HSM:saHpiSubscribe failed ret val is:%d",rc);
		hsm_session_reopen();
	}
	TRACE("HSM:saHpiSubscribe Suceess");

	/* Discover and dispatch the pending hotswap events */
	hsm_discover_and_dispatch();

	/* PLMC initialize */
        if( !plms_cb->plmc_initialized && plms_cb->ha_state == SA_AMF_HA_ACTIVE ) {
                rc = plmc_initialize(plms_plmc_connect_cbk,plms_plmc_udp_cbk,plms_plmc_error_cbk);
                if (rc) {
                        LOG_ER("PLMC initialize failed");
                        rc = NCSCC_RC_FAILURE;
                        exit(0);
                }
		plms_cb->plmc_initialized = TRUE;
		TRACE("PLMC initialization Success.");
        }

	TRACE("HSM:Blocking to receive events on HPI session");
	while(TRUE){
		rc = pthread_mutex_lock(&hsm_ha_state.mutex);
		if(rc){
			LOG_CR("HSM: Failed to take hsm_ha_state lock, exiting \
			the thread, ret value:%d err:%s", rc, strerror(errno));
			assert(0);
		}
		if(hsm_ha_state.state != SA_AMF_HA_ACTIVE){
			/* Wait on condition variable for the HA role from PLMS main thread */
			TRACE("HSM:Received Standby state,thread going to block till Active state is set");
			pthread_cond_wait(&hsm_ha_state.cond,&hsm_ha_state.mutex);
			got_new_active = TRUE;
		}
		rc = pthread_mutex_unlock(&hsm_ha_state.mutex);
		if(rc){
			LOG_CR("HSM:Failed to unlock hsm_ha_state lock,exiting \
			the thread, ret value:%d err:%s", rc, strerror(errno));
			assert(0);
		}
		if(got_new_active){
			/* Open the session on New active*/
			hsm_session_reopen();

			/* Rediscover the resources */
			hsm_discover_and_dispatch();

			got_new_active = FALSE;

			/* PLMC initialize */
			if( !plms_cb->plmc_initialized ) {
				rc = plmc_initialize(plms_plmc_connect_cbk,plms_plmc_udp_cbk,plms_plmc_error_cbk);
				if (rc) {
					LOG_ER("PLMC initialize failed");
					rc = NCSCC_RC_FAILURE;
					exit(0);
				}
				plms_cb->plmc_initialized = TRUE;
				TRACE("PLMC initialization Success.");
			}
		}

		ret = saHpiEventGet(cb->session_id, SAHPI_TIMEOUT_BLOCK, 
					&event, &rdr, &rpt_entry, NULL);
		rc = pthread_mutex_lock(&hsm_ha_state.mutex);
		if(rc){
			LOG_CR("HSM: Failed to take hsm_ha_state lock,exiting thread, ret value:%d err:%s",rc,strerror(errno));
			assert(0);
		}
		if(hsm_ha_state.state != SA_AMF_HA_ACTIVE){
			rc = pthread_mutex_unlock(&hsm_ha_state.mutex);
			if(rc){
				LOG_CR("HSM:Failed to unlock hsm_ha_state,exiting thread,ret value:%d err:%s",rc,strerror(errno));
				assert(0);
			}
			continue;
		}
		rc = pthread_mutex_unlock(&hsm_ha_state.mutex);
		if(rc){
			LOG_CR("HSM:Failed to unlock hsm_ha_state,exiting thread,ret value:%d err:%s",rc,strerror(errno));
			assert(0);
		}
		if( SA_OK != ret ){
			LOG_ER("HSM:saHpiEventGet failed, ret val is:%d",rc);
			/* Reopen the session */
			hsm_session_reopen();

			hsm_discover_and_dispatch();
			continue;
		}

		TRACE("HSM:Receieved event for res_id:%u Evt type:%u ",rpt_entry.ResourceId,event.EventType);

		/* Get the Hotswap State model for this resource */
		rc = hsm_get_hotswap_model(&rpt_entry,&hotswap_state_model);
		if(rc == NCSCC_RC_FAILURE){
			continue;
		}
		if( hotswap_state_model == PLMS_HPI_TWO_HOTSWAP_MODEL || 
			hotswap_state_model == PLMS_HPI_THREE_HOTSWAP_MODEL ){

			/* Get the Power state */
			rc = saHpiResourcePowerStateGet(cb->session_id,rpt_entry.ResourceId,&state);
			if( SA_OK != rc){
				LOG_ER("HSM1:saHpiResourcePowerStateGet failed for res:%u with ret val:%d",
								rpt_entry.ResourceId,rc);
				continue;
			}
			TRACE("HSM:saHpiResourcePowerStateGet succeeded for res:%u with ret val:%d state:%d",
								rpt_entry.ResourceId,rc,state);

			if (state == SAHPI_POWER_ON) {
				state = SAHPI_HS_STATE_ACTIVE;
			} else {
				state = SAHPI_HS_STATE_INACTIVE;
			}
		} else {
			/* Get the hotswap state */
			rc = saHpiHotSwapStateGet(cb->session_id,rpt_entry.ResourceId,&state);
			if( SA_OK != rc )
			{
				LOG_ER("HSM:saHpiHotSwapStateGet failed for res with state model as:%d and ret val:%d",
											hotswap_state_model,rc);
				continue;
			}
			TRACE("HSM:Hotswap state of res:%u is :%u",rpt_entry.ResourceId,state);
		}

		/* If it is a resource restore event( communication lost and
		got restored immediately ) ,retrieve the hotswap state after
		communication is restored */
		if (event.EventType == SAHPI_ET_RESOURCE){
			if(event.EventDataUnion.ResourceEvent.ResourceEventType
				== SAHPI_RESE_RESOURCE_RESTORED){
			hsm_send_hotswap_event(&rpt_entry, 
			hotswap_state_model, state, 
			event.EventDataUnion.HotSwapEvent.PreviousHotSwapState,
			retriev_idr_info); 

			}
		}

		/*After IPMC upgrade, Sensor events with sensor_type as 
		SAHPI_ENTITY_PRESENCE will be generated, send the status after 
		upgrade to check that the state has not changed */
		if (event.EventType == SAHPI_ET_SENSOR){
			if(event.EventDataUnion.SensorEvent.SensorType == 
						   SAHPI_ENTITY_PRESENCE){
			hsm_send_hotswap_event(&rpt_entry,
			hotswap_state_model, state,
			event.EventDataUnion.HotSwapEvent.PreviousHotSwapState,
			retriev_idr_info); 
			}
		}

		if (event.EventType == SAHPI_ET_HOTSWAP){ 
			if(hotswap_state_model  == PLMS_HPI_FULL_FIVE_HOTSWAP_MODEL){ 
				if (event.EventDataUnion.HotSwapEvent.HotSwapState ==
						  SAHPI_HS_STATE_EXTRACTION_PENDING ||
				     event.EventDataUnion.HotSwapEvent.HotSwapState ==
						    SAHPI_HS_STATE_INSERTION_PENDING){
					/* Cancel the hotswap polcy */ 
					rc = saHpiHotSwapPolicyCancel(cb->session_id,rpt_entry.ResourceId);
					if (SA_OK != rc)
						LOG_ER("Error taking control of res:%d ret val:%d",
									rpt_entry.ResourceId,rc); 
					 
					/* Set the AutoExtractionTimeout */
					rc = saHpiAutoExtractTimeoutSet(cb->session_id,rpt_entry.ResourceId,
									cb->extr_pending_timeout);
					if (SA_OK != rc)
						LOG_ER("AutoExtractTimeoutSet failed for res:%u ret val:%d",
										rpt_entry.ResourceId,rc);

				}
			}
			hsm_send_hotswap_event(&rpt_entry,hotswap_state_model,state,
			event.EventDataUnion.HotSwapEvent.PreviousHotSwapState,retriev_idr_info);

		}
	}

	TRACE_LEAVE();
}
/***********************************************************************
 * @brief	 This function discovers and publishes the outstanding  
 *                hpi events which apparently happened before the session 
 *                is established.
 *                        
 * @param[in] 
 *                        
 * @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE 
***********************************************************************/
static SaUint32T hsm_discover_and_dispatch()
{
	PLMS_HSM_CB       *cb = hsm_cb;
	SaHpiDomainInfoT  prev_domain_info;
	SaHpiDomainInfoT  latest_domain_info;
	SaHpiEntryIdT     current;
	SaHpiEntryIdT     next;
	PLMS_HPI_STATE_MODEL  hotswap_state_model;
	SaHpiRptEntryT    rpt_entry;
	SaHpiHsStateT  	  state;
	SaHpiHsStateT  	  previous_state = 0;
	SaUint32T	  retriev_idr_info = FALSE;
	SaUint32T	  prev_domain_op_status = NCSCC_RC_SUCCESS;
	SaUint32T	  rc = NCSCC_RC_SUCCESS;
	static SaUint32T	rpt_retry_count = 0;

	TRACE_ENTER();
			
	/* Discover the HPI resources on this session */   
	rc = saHpiDiscover(cb->session_id);
	if( SA_OK != rc ){
		LOG_ER("HSM:saHpiDiscover failed, ret val is:%d",rc);
		hsm_session_reopen();

		rc = saHpiDiscover(cb->session_id);
		if( SA_OK != rc ){
			LOG_ER("HSM:saHpiDiscover failed after reopening the \
			session, exiting the thread, ret val is:%d",rc);
			assert(0);
		}
	}
	TRACE("HSM:saHpiDiscover Success sess_id:%d",cb->session_id);
	/* Get the update count of domain_info*/	
	rc = saHpiDomainInfoGet(cb->session_id, &prev_domain_info);
	if( SA_OK != rc){
		LOG_ER("HSM:saHpiDomainInfoGet failed with \
				ret val:%d",rc); 
		prev_domain_op_status = NCSCC_RC_FAILURE;
	}

	/* Process the list of RPT entries on this session */
	next = SAHPI_FIRST_ENTRY;
	do{
		current = next;
		/* Get the RPT entry */
		rc = saHpiRptEntryGet(cb->session_id, current,&next, &rpt_entry);
		if (SA_OK != rc){
			 /* error getting RPT entry */
			 if (SA_ERR_HPI_NOT_PRESENT == rc) {
				if(current == SAHPI_FIRST_ENTRY)
				 	LOG_ER("RPT table is eempty ret val is:%d",rc);
				else
					  LOG_ER("saHpiRptEntryGet retruned SA_ERR_HPI_NOT_PRESENT \
					  	breaking from the loop");
				rc = NCSCC_RC_FAILURE;
				break;
			 }
			 if(SA_ERR_HPI_INVALID_PARAMS == rc){
				LOG_ER("Invalid params for saHpiRptEntryGet,breaking from the loop");
				rc = NCSCC_RC_FAILURE;
				break;
			 }else {
				/* Reopen the session once to handle session issues if any */
				if(rpt_retry_count){
					LOG_ER("saHpiRptEntryGet failed after reopening the sess retval:%u exiting",rc);
					assert(0);
				}
				hsm_session_reopen();
				rc = saHpiDiscover(cb->session_id);
				if( SA_OK != rc ){
					LOG_ER("saHpiDiscover failed, exiting fom the thread, retval:%u",rc);
					assert(0);
				}
				next = SAHPI_FIRST_ENTRY;
				rpt_retry_count++;
				continue;
			 }

		}
		TRACE("Retrieved RPT entry for res_id:%u ",rpt_entry.ResourceId);

		if(rpt_entry.ResourceCapabilities & SAHPI_CAPABILITY_EVENT_LOG){
			rc = saHpiEventLogClear(cb->session_id,
					      rpt_entry.ResourceId);
			if (SA_OK != rc)
				LOG_ER("saHpiEventLogClear failed ret val:%u",rc);
		}

		/* Get the Hotswap State model for this resource */
		rc = hsm_get_hotswap_model(&rpt_entry,&hotswap_state_model);
		if(rc == NCSCC_RC_FAILURE){
			/* Resoure is not a FRU, so go to next */
			continue;
		}

		if(hotswap_state_model  == PLMS_HPI_FULL_FIVE_HOTSWAP_MODEL){ 
			/*Cancel the default hot swap policy */ 
			rc = saHpiHotSwapPolicyCancel(cb->session_id,
						rpt_entry.ResourceId);
			if (SA_OK != rc){
				 if (SA_ERR_HPI_INVALID_REQUEST == rc){
					  /* Allow for the case where the 
					  hotswap policy cannot be canceledd
					  has already begun to execute */
					  LOG_ER("HSM:Hotswap policy cannot \
					  	be cancelled for res:%d",
					  	rpt_entry.ResourceId);
					 
				}						
				LOG_ER("Error taking control of res:%d \
					ret val:%d",rpt_entry.ResourceId,rc);
			}

			rc = saHpiAutoExtractTimeoutSet(cb->session_id,
						rpt_entry.ResourceId,
						cb->extr_pending_timeout);
			if (SA_OK != rc){
				LOG_ER("AutoExtractTimeoutSet failed for res:%u\
				ret val:%d",rpt_entry.ResourceId,rc);
			}
		  
		}

		if( hotswap_state_model == PLMS_HPI_TWO_HOTSWAP_MODEL || 
			hotswap_state_model == PLMS_HPI_THREE_HOTSWAP_MODEL ){

			/* Get the Power state */
			rc = saHpiResourcePowerStateGet(cb->session_id,rpt_entry.ResourceId,&state);
			if( SA_OK != rc){
				LOG_ER("HSM1:saHpiResourcePowerStateGet failed for res:%u with ret val:%d",
								rpt_entry.ResourceId,rc);
				continue;
			}
			TRACE("HSM:saHpiResourcePowerStateGet Succeded for res:%u with ret val:%d state:%d",
								rpt_entry.ResourceId,rc,state);

			if (state == SAHPI_POWER_ON) {
				state = SAHPI_HS_STATE_ACTIVE;
			} else {
				state = SAHPI_HS_STATE_INACTIVE;
			}

		} else {
			/* Get the hotswap state */
			rc = saHpiHotSwapStateGet(cb->session_id,rpt_entry.ResourceId,&state);
			if( SA_OK != rc){
				LOG_ER("HSM:saHpiHotSwapStateGet failed for res:%u with ret val:%d",
								rpt_entry.ResourceId,rc);
				continue;
			}
			TRACE("Hotswap state of res:%u is :%u", rpt_entry.ResourceId,state);
		}
		/* Retrieve IDR info if the state is 
		SAHPI_HS_STATE_INSERTION_PENDING or SAHPI_HS_STATE_ACTIVE */
		if ( SAHPI_HS_STATE_INSERTION_PENDING == state ||
		     SAHPI_HS_STATE_ACTIVE == state )
			retriev_idr_info = TRUE;
			
		/* Send the outstanding hot_swap event*/
		hsm_send_hotswap_event(&rpt_entry, hotswap_state_model, state,
					previous_state,retriev_idr_info);
		if(SAHPI_LAST_ENTRY == next && 
			 NCSCC_RC_SUCCESS == prev_domain_op_status ){
			/* Get the update count of domain_info*/	
			rc = saHpiDomainInfoGet(cb->session_id, 
					&latest_domain_info);
			if( SA_OK != rc){
				LOG_ER("HSM:saHpiDomainInfoGet failed with ret val:%d",rc); 
				return rc;
			}

			if(prev_domain_info.DatUpdateCount !=
				latest_domain_info.DatUpdateCount){
				next = SAHPI_FIRST_ENTRY;
				prev_domain_info.DatUpdateCount =
				latest_domain_info.DatUpdateCount;
				continue;
			}
		}
	} while (next != SAHPI_LAST_ENTRY); 
	/* Reset the retry count */
	rpt_retry_count = 0;

	TRACE_LEAVE();
	return rc;
}
/*******************************************************************************
 * @brief	This function finds the hotswap state model 
 *                        
 * @param[in]	rpt_entry 
 * @param[out]  hotswap_state_model 
 *
 * @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE 
*******************************************************************************/
static SaUint32T hsm_get_hotswap_model(SaHpiRptEntryT *rpt_entry,
                             	       SaUint32T *hotswap_state_model) 
{

	if(rpt_entry == NULL){
		LOG_ER("HSM:Invalid rpt_entry");
		return NCSCC_RC_FAILURE;
	}			

	if (rpt_entry->ResourceCapabilities & SAHPI_CAPABILITY_FRU)
	{
		if(rpt_entry->ResourceCapabilities & 
			SAHPI_CAPABILITY_MANAGED_HOTSWAP)
		{
			if (rpt_entry->HotSwapCapabilities & 
				SAHPI_HS_CAPABILITY_AUTOEXTRACT_READ_ONLY)
			{
				/*Set the capability as PARTIAL_FIVE_HOTSWAP */
				*hotswap_state_model = 
				PLMS_HPI_PARTIAL_FIVE_HOTSWAP_MODEL;
			}
			else
			{
				/*Set the capability as FULLL_FIVE_HOTSWAP */
				*hotswap_state_model = 
				PLMS_HPI_FULL_FIVE_HOTSWAP_MODEL;
			}
		}
		else
		{
			if(rpt_entry->ResourceCapabilities &
				SAHPI_CAPABILITY_POWER)
			{
				/*Set the capability as THREE_HOTSWAP_MODEL */
				*hotswap_state_model = 
				PLMS_HPI_THREE_HOTSWAP_MODEL;

			}
			else
			{
				/*Set the capability as HPI_TWO_HOTSWAP_MODEL */
				*hotswap_state_model =
				PLMS_HPI_TWO_HOTSWAP_MODEL;
			}
		}

	}
	else{
		if(rpt_entry->ResourceCapabilities & SAHPI_CAPABILITY_POWER)
		{
			/*Set the capability as PLMS_HPI_THREE_HOTSWAP_MODEL */
			*hotswap_state_model = PLMS_HPI_THREE_HOTSWAP_MODEL; 
			TRACE("Resource :%d is not a FRU but has power capability",rpt_entry->ResourceId);
		} else{
			TRACE("Resource :%d is not a FRU",rpt_entry->ResourceId);
			return NCSCC_RC_FAILURE;
		}
	}
	TRACE("Hotswap State model for res:%u is:%u res_capab is:%x entity_type is:%u",
	      		rpt_entry->ResourceId,*hotswap_state_model,
	      		rpt_entry->ResourceCapabilities,rpt_entry->ResourceEntity.Entry[0].EntityType);
	
	return NCSCC_RC_SUCCESS;
}
/*******************************************************************//**
 * @brief	 Sends hotswap event to PLM main thread
 *                  
 *                        
 * @param[in]	rpt_entry 
 * @param[in]   hotswap_state_model 
 *		hotswap_state
 *		prev_hotswap_state
 *              retriev_idr_info          
 *
 * @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE 
***********************************************************************/
static SaUint32T hsm_send_hotswap_event(SaHpiRptEntryT   *rpt_entry, 
					SaUint32T hotswap_state_model, 
					SaHpiHsStateT hotswap_state,
					SaHpiHsStateT prev_hotswap_state,
					SaUint32T  retriev_idr_info) 
{
	PLMS_CB            *cb = plms_cb;
	PLMS_EVT	   *plms_evt = NULL;
	PLMS_EVT_REQ	   req_evt;
	PLMS_INV_DATA  	   plm_inv_data;
	SaHpiEntityPathT   entity_path;
	SaInt8T	   	   *entity_path_str = NULL;
	SaUint32T          i, rc;

	TRACE_ENTER();

	if(NULL == rpt_entry){
		LOG_ER("Invalid rpt_entry");
		return NCSCC_RC_FAILURE;
	}			

	/* Get the entity path */
	memset(&entity_path, 0, sizeof(SaHpiEntityPathT));
	for( i=0;  i < SAHPI_MAX_ENTITY_PATH; i++ ) {
		entity_path.Entry[i] = rpt_entry->ResourceEntity.Entry[i];

		/* Stop copying when we see SAHPI_ENT_ROOT */
		if (rpt_entry->ResourceEntity.Entry[i].EntityType == 
			SAHPI_ENT_ROOT)
			break;
	}

	memset(&req_evt, 0, sizeof(PLMS_EVT_REQ));
	
	req_evt.req_type = PLMS_HPI_EVT_T;
	req_evt.hpi_evt.sa_hpi_evt.Severity = SAHPI_CRITICAL;
	req_evt.hpi_evt.sa_hpi_evt.EventType = SAHPI_ET_HOTSWAP;
	req_evt.hpi_evt.sa_hpi_evt.EventDataUnion.HotSwapEvent.HotSwapState =  							 hotswap_state; 
	req_evt.hpi_evt.sa_hpi_evt.EventDataUnion.HotSwapEvent.PreviousHotSwapState = prev_hotswap_state; 


	/* convert SaHpiEntityPathT to string format */
	rc = convert_entitypath_to_string(&entity_path,&entity_path_str); 
	if(NCSCC_RC_FAILURE == rc){ 
		/* Log */
		return NCSCC_RC_FAILURE;
	}
	req_evt.hpi_evt.entity_path =  entity_path_str;

	/* Copy entity key to req_evt*/
	memcpy(&req_evt.hpi_evt.epath_key, &entity_path, sizeof(entity_path));
	req_evt.hpi_evt.state_model = hotswap_state_model; 

	if(TRUE != retriev_idr_info){
		if((( hotswap_state_model == PLMS_HPI_FULL_FIVE_HOTSWAP_MODEL ||
		hotswap_state_model == PLMS_HPI_PARTIAL_FIVE_HOTSWAP_MODEL )  &&
		hotswap_state == SAHPI_HS_STATE_INSERTION_PENDING)  ||
		( (hotswap_state_model == PLMS_HPI_THREE_HOTSWAP_MODEL  ||
		hotswap_state_model == PLMS_HPI_TWO_HOTSWAP_MODEL)   &&   
		hotswap_state == SAHPI_HS_STATE_ACTIVE )){
			retriev_idr_info = TRUE;
		}
	}
	
	/* Retrieve IDR information */
	if(retriev_idr_info){
		memset(&plm_inv_data, 0, sizeof(plm_inv_data));

		rc = hsm_get_idr_info( rpt_entry,&plm_inv_data);
		if(NCSCC_RC_SUCCESS == rc ){
			memcpy(&req_evt.hpi_evt.inv_data,&plm_inv_data,
						sizeof(PLMS_INV_DATA));
		}

	}        

	plms_evt = (PLMS_EVT *)malloc(sizeof(PLMS_EVT));
	if(NULL == plms_evt){
		LOG_ER("Memory allocation failed ret:%s",strerror(errno));
		assert(0);
	}
	memset(plms_evt, 0, sizeof(PLMS_EVT));
	plms_evt->req_res = PLMS_REQ;
	plms_evt->req_evt = req_evt;

	TRACE("Sending Hotswap_event for res:%u entity:%s  State:%u \n",
		rpt_entry->ResourceId,entity_path_str,hotswap_state);

	/*  post event to PLM mailbox */
	rc = m_NCS_IPC_SEND(&cb->mbx,(NCSCONTEXT)plms_evt,
			MDS_SEND_PRIORITY_MEDIUM);
	if (rc != NCSCC_RC_SUCCESS)
		    LOG_ER("m_NCS_IPC_SEND failed error %u", rc);

	TRACE_LEAVE();
	return rc;
}

/***********************************************************************
 * @brief	 This function retrieves Inventory Data  Record (IDR) for
 *		 the given resource 
 *                        
 * @param[in]	 rpt_entry 
 * @param[in]	 idr_id 
 * @param[out]	 inv_data 
 *
 * @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE 
***********************************************************************/
static SaUint32T hsm_get_idr_chassis_info(SaHpiRptEntryT  *rpt_entry,
                                          SaHpiIdrIdT     idr_id,
                                          PLMS_INV_DATA   *inv_data)
{
        SaHpiEntryIdT        next_area, next_field, area_id;
        SaHpiIdrAreaHeaderT  area_info;
        SaHpiIdrFieldT       thisField;
        PLMS_HSM_CB          *cb = hsm_cb;
        SaUint32T            err = SA_OK;
        SaHpiEntryIdT        fieldId;
        area_id = SAHPI_FIRST_ENTRY;

	/* First make sure that we find the chassis info area */
        while ((err == SA_OK) && (area_id != SAHPI_LAST_ENTRY)) {
                /* get the chassis_info_area header */
                err = saHpiIdrAreaHeaderGet(cb->session_id,
                                        rpt_entry->ResourceId,
                                        idr_id,
                                        SAHPI_IDR_AREATYPE_UNSPECIFIED,
                                        area_id,
                                        &next_area,
                                        &area_info);
                /* Check out what Area it is */
                if (area_info.Type == SAHPI_IDR_AREATYPE_CHASSIS_INFO) {
                        break;
                }
                area_id = next_area;
        }
        if (area_info.Type != SAHPI_IDR_AREATYPE_CHASSIS_INFO)
                return NCSCC_RC_FAILURE;

	/* Okay, now get all fields we are interested in */
        fieldId = SAHPI_FIRST_ENTRY;
        while ((err == SA_OK) && (fieldId != SAHPI_LAST_ENTRY)) {
                err = saHpiIdrFieldGet(cb->session_id,  rpt_entry->ResourceId,
                                        idr_id,
                                        area_info.AreaId,
                                        SAHPI_IDR_FIELDTYPE_UNSPECIFIED,
                                        fieldId, &next_field,
                                        &thisField);
                if (err == SA_OK) {
                        if (thisField.Type == SAHPI_IDR_FIELDTYPE_CHASSIS_TYPE){
                                /* copy thisfield to chassevent structure */
                                inv_data->chassis_area.chassis_type =
                                                thisField.Field.Data[0];
                        } else
                        if (thisField.Type == SAHPI_IDR_FIELDTYPE_SERIAL_NUMBER){
                                /* copy product_name into event structure */
                                inv_data->chassis_area.serial_no.DataLength =
                                                thisField.Field.DataLength;
                                memcpy(inv_data->chassis_area.serial_no.Data,
                                        thisField.Field.Data,
                                        thisField.Field.DataLength);

                        } else
                        if (thisField.Type == SAHPI_IDR_FIELDTYPE_PART_NUMBER){
                                /* copy product_name into event structure */
                                inv_data->chassis_area.part_no.DataLength =
                                                thisField.Field.DataLength;
                                memcpy(inv_data->chassis_area.part_no.Data,
                                        thisField.Field.Data,
                                        inv_data->chassis_area.part_no.DataLength);

                        }
                } else {
                        LOG_ER("hsm_get_idr_chassis_info failed error val is:%d",err);
                        return NCSCC_RC_FAILURE;
                }
                fieldId = next_field;
        }
        return NCSCC_RC_SUCCESS;

}



/***********************************************************************
 * @brief	 This function retrieves Inventory Data  Record (IDR) for
 *		 the given resource 
 *                        
 * @param[in]	 rpt_entry 
 * @param[in]	 idr_id 
 * @param[out]	 inv_data 
 *
 * @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE 
***********************************************************************/
static SaUint32T hsm_get_idr_board_info(SaHpiRptEntryT  *rpt_entry, 
				SaHpiIdrIdT idr_id,
				PLMS_INV_DATA  *inv_data)
{
	PLMS_HSM_CB       *cb = hsm_cb;
	SaHpiEntryIdT   next_area, next_field, area_id;
	SaHpiIdrAreaHeaderT area_info;
	SaHpiIdrFieldT thisField;
	SaHpiEntryIdT  fieldId;
	SaUint32T      err = SA_OK;
	area_id = SAHPI_FIRST_ENTRY;

	/* get the BOARD_INFO area header for the given resource */
	/* First we need to make sure we can find the board info */
        while ((err == SA_OK) && (area_id != SAHPI_LAST_ENTRY)) {
                err = saHpiIdrAreaHeaderGet(cb->session_id,
                                        rpt_entry->ResourceId,
                                        idr_id,
                                        SAHPI_IDR_AREATYPE_UNSPECIFIED,
                                        area_id,
                                        &next_area,
                                        &area_info);
                /* Check out what Area it is */
                if (area_info.Type == SAHPI_IDR_AREATYPE_BOARD_INFO) {
                        break;
                }
                area_id = next_area;
        }
        if (area_info.Type != SAHPI_IDR_AREATYPE_BOARD_INFO)
                return NCSCC_RC_FAILURE;

	/* Now extract all fields we can find and are looking for */
        fieldId = SAHPI_FIRST_ENTRY;
        while ((err == SA_OK) && (fieldId != SAHPI_LAST_ENTRY)) {
                err = saHpiIdrFieldGet(cb->session_id,  rpt_entry->ResourceId,
                                        idr_id,
                                        area_info.AreaId,
                                        SAHPI_IDR_FIELDTYPE_UNSPECIFIED,
                                        fieldId, &next_field,
                                        &thisField);
                if (err == SA_OK) {
                        if (thisField.Type == SAHPI_IDR_FIELDTYPE_MANUFACTURER){
                                /* copy product_name into event structure */
				hsm_correct_length(&thisField);
                                inv_data->board_area.manufacturer_name.
                                        DataLength = thisField.Field.DataLength;
                                memcpy(inv_data->board_area.manufacturer_name.
                                        Data, thisField.Field.Data, thisField.
                                                        Field.DataLength);
                        } else
                        if (thisField.Type == SAHPI_IDR_FIELDTYPE_PRODUCT_NAME){
                                /* copy product_name into event structure */
				hsm_correct_length(&thisField);
                                inv_data->board_area.product_name.DataLength =
                                                thisField.Field.DataLength;
                                memcpy(inv_data->board_area.product_name.Data,
                                        thisField.Field.Data, thisField.Field.
                                                                DataLength);
                        } else
                        if (thisField.Type == SAHPI_IDR_FIELDTYPE_SERIAL_NUMBER){
                                /* copy product_name into event structure */
				hsm_correct_length(&thisField);
                                inv_data->board_area.serial_no.DataLength =
                                                thisField.Field.DataLength;
                                memcpy(inv_data->board_area.serial_no.Data,
                                        thisField.Field.Data, thisField.Field.
                                                                DataLength);
                        } else
                        if (thisField.Type == SAHPI_IDR_FIELDTYPE_PART_NUMBER){
                                /* copy product_name into event structure */
				hsm_correct_length(&thisField);
                                inv_data->board_area.part_no.DataLength =
                                                thisField.Field.DataLength;
                                memcpy(inv_data->board_area.part_no.Data,
                                        thisField.Field.Data, inv_data->
                                                board_area.part_no.DataLength);
                        } else
                        if (thisField.Type == SAHPI_IDR_FIELDTYPE_FILE_ID){
                                /* copy product_name into event structure */
				hsm_correct_length(&thisField);
                                inv_data->board_area.fru_field_id.DataLength =
                                                thisField.Field.DataLength;
                                memcpy(inv_data->board_area.fru_field_id.Data,
                                        thisField.Field.Data, inv_data->
                                        board_area.fru_field_id.DataLength);
                        }
                } else {
                        LOG_ER("hsm_get_idr_board_info failed error val is:%d",err);
                        return NCSCC_RC_FAILURE;
                }
                fieldId = next_field;
        }
	return NCSCC_RC_SUCCESS;
}
/***********************************************************************
 * @brief	 This function retrieves Inventory Data  Record (IDR) for
 *		 the given resource 
 *                        
 * @param[in]	 rpt_entry 
 * @param[in]	 idr_id 
 * @param[out]	 inv_data
 *
 * @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE 
***********************************************************************/
static SaUint32T hsm_get_idr_product_info(SaHpiRptEntryT  *rpt_entry, 
				SaHpiIdrIdT idr_id,
				PLMS_INV_DATA  *inv_data)
{
	PLMS_HSM_CB       *cb = hsm_cb;
	SaHpiEntryIdT   next_area, next_field, area_id;
	SaHpiIdrAreaHeaderT area_info;
	SaHpiIdrFieldT thisField;
	SaHpiEntryIdT  fieldId;
	SaUint32T      err = SA_OK;
	area_id = SAHPI_FIRST_ENTRY;

	/* get the PRODUCT_INFO area header for the given resource */
	/* First we need to make sure we can find the product info */
        while ((err == SA_OK) && (area_id != SAHPI_LAST_ENTRY)) {
                /* get the chassis_info_area header */
                err = saHpiIdrAreaHeaderGet(cb->session_id,
                                        rpt_entry->ResourceId,
                                        idr_id,
                                        SAHPI_IDR_AREATYPE_UNSPECIFIED,
                                        area_id,
                                        &next_area,
                                        &area_info);
                /* Check out what Area it is */
                if (area_info.Type == SAHPI_IDR_AREATYPE_PRODUCT_INFO) {
                        break;
                }
                area_id = next_area;
        }
        if (area_info.Type != SAHPI_IDR_AREATYPE_PRODUCT_INFO)
                return NCSCC_RC_FAILURE;

	/* Now extract all fields we can find and are looking for */
        fieldId = SAHPI_FIRST_ENTRY;
        while ((err == SA_OK) && (fieldId != SAHPI_LAST_ENTRY)) {
                err = saHpiIdrFieldGet(cb->session_id,  rpt_entry->ResourceId,
                                        idr_id,
                                        area_info.AreaId,
                                        SAHPI_IDR_FIELDTYPE_UNSPECIFIED,
                                        fieldId, &next_field,
                                        &thisField);
                if (err == SA_OK) {
                        if (thisField.Type == SAHPI_IDR_FIELDTYPE_MANUFACTURER){
                                /* copy product_name into event structure */
				hsm_correct_length(&thisField);
                                inv_data->product_area.manufacturer_name.
                                        DataLength = thisField.Field.DataLength;
                                memcpy(inv_data->product_area.manufacturer_name
                                        .Data, thisField.Field.Data,
                                        inv_data->product_area.manufacturer_name
                                                                .DataLength);
                        } else
                        if (thisField.Type == SAHPI_IDR_FIELDTYPE_PRODUCT_NAME){
                                /* copy product_name into event structure */
				hsm_correct_length(&thisField);
                                inv_data->product_area.product_name.DataLength =
                                                thisField.Field.DataLength;
                                memcpy(inv_data->product_area.product_name.Data,
                                        thisField.Field.Data, inv_data->
                                        product_area.product_name.DataLength);
                        } else
                        if (thisField.Type == SAHPI_IDR_FIELDTYPE_SERIAL_NUMBER){
                                /* copy product_name into event structure */
				hsm_correct_length(&thisField);
                                inv_data->product_area.serial_no.DataLength =
                                                thisField.Field.DataLength;
                                memcpy(inv_data->product_area.serial_no.Data,
                                        thisField.Field.Data, inv_data->
                                        product_area.serial_no.DataLength);
                        } else
                        if (thisField.Type == SAHPI_IDR_FIELDTYPE_PART_NUMBER){
                                /* copy product_name into event structure */
				hsm_correct_length(&thisField);
                                inv_data->product_area.part_no.DataLength =
                                                thisField.Field.DataLength;
                                memcpy(inv_data->product_area.part_no.Data,
                                        thisField.Field.Data, inv_data->
                                        product_area.part_no.DataLength);
                        } else
                        if (thisField.Type ==
                                        SAHPI_IDR_FIELDTYPE_PRODUCT_VERSION){
                                /* copy product_name into event structure */
				hsm_correct_length(&thisField);
                                inv_data->product_area.product_version.
                                                DataLength = thisField.Field.
                                                                DataLength;
                                memcpy(inv_data->product_area.product_version.
                                                Data, thisField.Field.Data,
                                        inv_data->product_area.product_version.
                                                                DataLength);
                        } else
                        if (thisField.Type == SAHPI_IDR_FIELDTYPE_ASSET_TAG){
                                /* copy product_name into event structure */
				hsm_correct_length(&thisField);
                                inv_data->product_area.asset_tag.DataLength =
                                                thisField.Field.DataLength;
                                memcpy(inv_data->product_area.asset_tag.Data,
                                        thisField.Field.Data, inv_data->
                                        product_area.asset_tag.DataLength);
                        } else
                        if (thisField.Type == SAHPI_IDR_FIELDTYPE_FILE_ID){
                                /* copy product_name into event structure */
				hsm_correct_length(&thisField);
                                inv_data->product_area.fru_field_id.DataLength =
                                                thisField.Field.DataLength;
                                memcpy(inv_data->product_area.fru_field_id.Data,
                                        thisField.Field.Data, inv_data->
                                        product_area.fru_field_id.DataLength);
                        }
                } else {
                        LOG_ER("hsm_get_idr_board_info failed error val is:%d",err);
                        return NCSCC_RC_FAILURE;
                }
                fieldId = next_field;
        }

        return NCSCC_RC_SUCCESS;
}

/***********************************************************************
 * @brief        This function retrieves SaHpiIdrFieldT and calgulate
 *               the correct field length
 *
 * @param[in]    thisfield
 * @param[out]   thisfield
 *
 * @return      NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
***********************************************************************/
SaUint32T hsm_correct_length(SaHpiIdrFieldT *thisfield)
{
        char *field_ptr;
        SaHpiUint8T fieldlength;
        fieldlength = thisfield->Field.DataLength;
	if (fieldlength == 0)
		return 0;
	else
		fieldlength--;
        field_ptr = (char *)thisfield->Field.Data;
        while (isspace(field_ptr[fieldlength]) || (field_ptr[fieldlength]
                                                                == '\0')) {
                if (fieldlength == 0) {
                        thisfield->Field.DataLength = 0;
                        return 0;
                }
                fieldlength--;
        }
        thisfield->Field.DataLength = fieldlength + 1;
        return 0;
}

/***********************************************************************
 * @brief	 This function retrieves Inventory Data  Record (IDR) for
 *		 the given resource 
 *                        
 * @param[in]	 rpt_entry 
 * @param[out]	 inv_data 
 *                        
 * @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE 
***********************************************************************/
SaUint32T hsm_get_idr_info(SaHpiRptEntryT  *rpt_entry, 
			PLMS_INV_DATA  *inv_data)
{
	PLMS_HSM_CB     *cb = hsm_cb;
	SaHpiEntryIdT   current_entry, next_entry;
	SaHpiRdrT       rdr;
/*	SaHpiIdrInfoT   idr_info; */
	SaHpiIdrIdT 	idr_id;
	SaUint32T       err;

	TRACE_ENTER();

	if(NULL == rpt_entry)
		return NCSCC_RC_FAILURE;
		
	TRACE("HSM:Retrieving IDR for res:%u \n",rpt_entry->ResourceId);

	/* find the RDR which has got inventory data associated with it */
	current_entry  = next_entry = SAHPI_FIRST_ENTRY;
	do{
		err = saHpiRdrGet(cb->session_id, rpt_entry->ResourceId, 
				current_entry, &next_entry, &rdr); 
		if (SA_OK != err){
			LOG_ER("HSM:RDR get failed with err:%d",err);
			if(SA_ERR_HPI_CAPABILITY == err){
				LOG_ER("HSM:RDR table is empty ");
				return NCSCC_RC_FAILURE;
			}
			if(SA_ERR_HPI_NOT_PRESENT == err &&
				SAHPI_FIRST_ENTRY == current_entry ){
				return NCSCC_RC_FAILURE;
			}
			else{
				hsm_session_reopen();
				saHpiDiscover(cb->session_id);
				current_entry = next_entry  = SAHPI_FIRST_ENTRY;
			}
		}
		else{
			if (rdr.RdrType == SAHPI_INVENTORY_RDR){
				TRACE("Found Inventory RDR");
				break;
			}
			current_entry = next_entry;
		}
	} while(next_entry  != SAHPI_LAST_ENTRY);

	if(next_entry  == SAHPI_LAST_ENTRY){
		LOG_ER("HSM:RDR table is empty ");
		return NCSCC_RC_FAILURE;
	}

	idr_id = rdr.RdrTypeUnion.InventoryRec.IdrId;
	
#if 0
	/* this call is not required */
	err = saHpiIdrInfoGet(cb->session_id, rpt_entry->ResourceId, idr_id,
			&idr_info);
	if ( err != SA_OK )
	{
		TRACE("HSM:call to saHpiIdrInfoGet failed err:%u",err);
		return(NCSCC_RC_FAILURE);
	}
#endif

	/* Retrieve  idr board info */
	hsm_get_idr_board_info(rpt_entry, idr_id, inv_data);

	/* Retrieve  idr cahassis info */
	hsm_get_idr_chassis_info(rpt_entry, idr_id, inv_data);
	
	/* Retrieve  idr product info */
	hsm_get_idr_product_info(rpt_entry, idr_id, inv_data);

	TRACE_LEAVE();

	return NCSCC_RC_SUCCESS;
}
/***********************************************************************
* @brief	 This function re-initializes the HPI session and 
*		rediscovers the resources
* 
* @param[in] 
*                
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE 
***********************************************************************/
static SaUint32T hsm_session_reopen()
{ 
	PLMS_HSM_CB       *cb = hsm_cb;
	SaHpiSessionIdT   session_id;
	SaInt32T	  rc = 0, retry = 1;
	SaInt32T	  err;
	
	TRACE_ENTER();

	/* Close the session */
	if(cb->session_id){
		if((err = saHpiSessionClose(cb->session_id))!= SA_OK)
			LOG_ER("HSM:Close session return error: %d:\n",err);
	}

	/* Reset the session_id */
	cb->session_id = 0;

	/* Reset the HRB session_id */
	pthread_mutex_lock(&hrb_cb->mutex);
	hrb_cb->session_id = 0;
	pthread_mutex_unlock(&hrb_cb->mutex);

	/* Open a session on the SAHPI_UNSPECIFIED_DOMAIN_ID */
	while(1){
		err = saHpiSessionOpen(SAHPI_UNSPECIFIED_DOMAIN_ID,
						&session_id, NULL);
		if(SA_OK != err){
			LOG_ER("HSM:saHpiSessionOpen failed with err:%d \
                                after retrying %d times",err,retry);
			if(retry++ < PLMS_MAX_HPI_SESSION_OPEN_RETRIES ){
				sleep(2);
				continue;
			}
			else{
				LOG_ER("saHpiSessionOpen failed after \
                                retres, so exiting");
				assert(0);
			}

		}
		else{
			cb->session_id = session_id;

			/* Update the HRB session_id */
			pthread_mutex_lock(&hrb_cb->mutex);
			hrb_cb->session_id = session_id;
			pthread_mutex_unlock(&hrb_cb->mutex);
			TRACE("HSM: saHpiSessionOpen success");
			break;

		}
	}

	/*Set the auto-insert timeout which is domain specific and applies to
        * all the reource which support managed hot swap */
	err =saHpiAutoInsertTimeoutSet(session_id,  cb->insert_pending_timeout);
	if (SA_OK != err){
		if (err == SA_ERR_HPI_READ_ONLY){
			/* Allow for the case where the insertion timeout is
			read-only. */
			TRACE("HSM: saHpiAutoInsertTimeoutSet is \
							read-only");
		}
		else{
			TRACE("HSM:Error in setting Auto Insertion timeout \
                                                return value is:%d", err);
		}
	}else
	
	/* Subscribe to receive events on this HPI session */ 
	rc =  saHpiSubscribe(cb->session_id);
	if( SA_OK != rc ){
		/* TBD Need to re check this flow once more */
		LOG_ER("HSM:saHpiSubscribe failed after session reopen \
				ret val is:%d",rc);
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
/***********************************************************************
* @brief	 This function closes HPI session
* 
* @param[in] 
*                
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE 
***********************************************************************/
SaUint32T plms_hsm_session_close()
{
	PLMS_HSM_CB        *cb = hsm_cb;
	SaUint32T	   rc = 0;
	/* Close the HPI session */
        rc = saHpiSessionClose(cb->session_id);
        if (SA_OK != rc){
        	LOG_ER("HSM:Close session return error: %d:\n",rc);
	        return NCSCC_RC_FAILURE;
	}

	/* Reset the session_id */
	cb->session_id = 0;
	return NCSCC_RC_SUCCESS;
}
