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
        
This include file contains AMF interaction logic for health-check and other
stuff.
*******************************************************************************/
#include "plms.h"
#include "plms_hsm.h"
#include "plms_mbcsv.h"
#include "nid_start_util.h"
/* HA AMF statemachine & State handler definitions */

/****************************************************************************
 * Name          : plms_quiescing_state_handler
 *
 * Description   : This function is called upon receving an Quiescing state
 *                 assignment from AMF.
 *
 * Arguments     : invocation     - This parameter designated a particular
 *                                  invocation of this callback function. The
 *                                  invoke process return invocation when it
 *                                  responds to the Avilability Management
 *                                  FrameWork using the saAmfResponse()
 *                                  function.
 *                 cb              -A pointer to the PLMS control block.
 *
 * Return Values : None
 *
 * Notes         : None
 *****************************************************************************/

SaUint32T plms_quiescing_state_handler(SaInvocationT invocation)
{
	SaAisErrorT error = SA_AIS_OK;
	PLMS_CB * cb = plms_cb;
	TRACE_ENTER();
	error = saAmfCSIQuiescingComplete(cb->amf_hdl, invocation, error);

	saAmfResponse(cb->amf_hdl, invocation, error);
	LOG_IN("I AM IN HA AMF QUIESCING STATE\n");
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : plms_quiesced_state_handler
 *
 * Description   : This function is called upon receving an Quiesced state
 *                 assignment from AMF.
 *
 * Arguments     : invocation     - This parameter designated a particular
 *                                  invocation of this callback function. The
 *                                  invoke process return invocation when it
 *                                  responds to the Avilability Management
 *                                  FrameWork using the saAmfResponse()
 *                                  function.
 *                 cb              -A pointer to the PLMS control block.
 *
 * Return Values : None
 *
 * Notes         : None
 *****************************************************************************/

SaUint32T plms_quiesced_state_handler(SaInvocationT invocation)
{

	PLMS_CB * cb = plms_cb;
	V_DEST_RL mds_role;

	/* Unregister with IMM as OI */
	plms_proc_active_quiesced_role_change();
	mds_role = V_DEST_RL_QUIESCED;
	TRACE_ENTER();
	m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_WRITE);

	/** set the CB's anchor value & mds role */

	cb->mds_role = mds_role;
	plms_mds_change_role();
	cb->amf_invocation_id = invocation;

	cb->is_quisced_set = TRUE;
	LOG_IN("I AM IN HA AMF QUIESCED STATE\n");

	m_NCS_UNLOCK(&cb->cb_lock,NCS_LOCK_WRITE);

	TRACE_LEAVE();

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
 * Name          : plms_invalid_state_handler
 *
 * Description   : This function is called upon receving an invalid state(as
 *                 in the FSM) assignment from AMF.
 *
 * Arguments     : invocation     - This parameter designated a particular
 *                                  invocation of this callback function. The
 *                                  invoke process return invocation when it
 *                                  responds to the Avilability Management
 *                                  FrameWork using the saAmfResponse()
 *                                  function.
 *
 * Return Values : None
 *
 * Notes         : None
 *****************************************************************************/

SaUint32T plms_invalid_state_handler(SaInvocationT invocation)
{
	PLMS_CB * cb = plms_cb;
	TRACE_ENTER();
	m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_READ);
	saAmfResponse(cb->amf_hdl, invocation, SA_AIS_ERR_BAD_OPERATION);
	m_NCS_UNLOCK(&cb->cb_lock,NCS_LOCK_READ);
	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : plms_amf_health_chk_callback
 *
 * Description   : This is the callback function which will be called 
 *                 when the AMF framework needs to health check for the component.
 *
 * Arguments     : invocation     - This parameter designated a particular 
 *                                  invocation of this callback function. The
 *                                  invoke process return invocation when it 
 *                                  responds to the Avilability Management 
 *                                  FrameWork using the saAmfResponse() 
 *                                  function.
 *                 compName       - A pointer to the name of the component 
 *                                  whose readiness state the Availability 
 *                                  Management Framework is setting.
 *                 checkType      - The type of healthcheck to be executed. 
 *
 * Return Values : None
 *
 * Notes         : At present we are just support a simple liveness check.
 *****************************************************************************/
void plms_amf_health_chk_callback(SaInvocationT invocation, const SaNameT *compName, SaAmfHealthcheckKeyT *checkType)
{
	PLMS_CB *cb = plms_cb;
	SaAisErrorT error = SA_AIS_OK;
	TRACE_ENTER();
	m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_READ);
	saAmfResponse(cb->amf_hdl, invocation, error);
	m_NCS_UNLOCK(&cb->cb_lock,NCS_LOCK_READ);
	TRACE_LEAVE();
	return;
}

/****************************************************************************
 * Name          : plms_amf_CSI_set_callback
 *
 * Description   : AMF callback function called 
 *                 when there is any change in the HA state.
 *
 * Arguments     : invocation     - This parameter designated a particular 
 *                                  invocation of this callback function. The 
 *                                  invoke process return invocation when it 
 *                                  responds to the Avilability Management 
 *                                  FrameWork using the saAmfResponse() 
 *                                  function.
 *                 compName       - A pointer to the name of the component 
 *                                  whose readiness stae the Availability 
 *                                  Management Framework is setting.
 *                 haState        - The new HA state to be assumeb by the 
 *                                  component service instance identified by 
 *                                  csiName.
 *                 csiDescriptor - This will indicate whether or not the 
 *                                  component service instance for 
 *                                  ativeCompName went through quiescing.
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void
plms_amf_CSI_set_callback(SaInvocationT invocation, const SaNameT *compName, 
		SaAmfHAStateT new_haState, SaAmfCSIDescriptorT csiDescriptor)
{
	PLMS_CB *cb = plms_cb;
	SaAisErrorT error = SA_AIS_OK;
	SaAmfHAStateT prev_haState;
	NCS_BOOL role_change = TRUE;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_WRITE);

	prev_haState = cb->ha_state;

	/* Invoke the appropriate state handler routine */
	switch (new_haState) {

	case SA_AMF_HA_ACTIVE:
		cb->mds_role = V_DEST_RL_ACTIVE;
		TRACE("MY HA ROLE : AMF ACTIVE\n");

		if (prev_haState == SA_AMF_HA_QUIESCED) {
			plms_proc_quiesced_active_role_change();
		}
		else if (prev_haState == SA_AMF_HA_STANDBY) {
			plms_proc_standby_active_role_change();
		}
		if(cb->hpi_cfg.hpi_support){
			if (cb->hpi_intf_up == FALSE) {
				TRACE("Got Active role, spawning HSM & HRB");
				rc = plms_hsm_hrb_init();
				if(NCSCC_RC_FAILURE == rc) {
					LOG_ER("hsm & hrb initialization failed");
                        		error = SA_AIS_ERR_FAILED_OPERATION;
					goto response;
				}
				cb->hpi_intf_up = TRUE;
			}
			if (prev_haState == SA_AMF_HA_STANDBY) {
				/* Build entity_path_to_entity mapping tree */
				rc = plms_build_epath_to_entity_map_tree();
				if( NCSCC_RC_SUCCESS != rc ){
					LOG_ER("Failed to build entity_path_to_entity mapping tree");
                        		error = SA_AIS_ERR_FAILED_OPERATION;
					goto response;
				}
			}
		}
		if( cb->hpi_intf_up ) {
			TRACE("PLMS sending Active role to HSM");
			pthread_mutex_lock(&hsm_ha_state.mutex);
			hsm_ha_state.state = V_DEST_RL_ACTIVE;
			pthread_cond_signal(&hsm_ha_state.cond);
			pthread_mutex_unlock(&hsm_ha_state.mutex);

			TRACE("PLMS sending Active role to HRB");
			pthread_mutex_lock(&hrb_ha_state.mutex);
			hrb_ha_state.state = SA_AMF_HA_ACTIVE;
			pthread_cond_signal(&hrb_ha_state.cond);
			pthread_mutex_unlock(&hrb_ha_state.mutex);
		}
                /* PLMC initialize */
                if(!cb->hpi_cfg.hpi_support && !cb->plmc_initialized){
                        TRACE("Initializing PLMC");
                        rc = plmc_initialize(plms_plmc_connect_cbk,
                                                plms_plmc_udp_cbk,
                                                plms_plmc_error_cbk);
                        if (rc){
                                LOG_ER("PLMC initialize failed.");
                                rc = NCSCC_RC_FAILURE;
                                goto response;
                        }
                        TRACE("PLMC initialize success.");
                        cb->plmc_initialized = TRUE;
                }


		cb->mds_role = V_DEST_RL_ACTIVE;
		break;
	case SA_AMF_HA_STANDBY:
		cb->mds_role = V_DEST_RL_STANDBY;
		LOG_IN("MY HA ROLE : AMF STANDBY\n");

		TRACE("Send signal to HSM indicating standby state");
		pthread_mutex_lock(&hsm_ha_state.mutex);
		hsm_ha_state.state = SA_AMF_HA_STANDBY;
		pthread_mutex_unlock(&hsm_ha_state.mutex);

		TRACE("Send signal to HRB indicating standby state");
		pthread_mutex_lock(&hrb_ha_state.mutex);
		hrb_ha_state.state = SA_AMF_HA_STANDBY;
		pthread_mutex_unlock(&hrb_ha_state.mutex);

		SaUint32T (* hsm_func_ptr)() = NULL;
		if(cb->hpi_cfg.hpi_support){
			/* Get the hsm Init func ptr */
			hsm_func_ptr = dlsym(cb->hpi_intf_hdl, "plms_hsm_session_close");
			if ( NULL == hsm_func_ptr ) {
				LOG_ER("dlsym() failed to get the hsm_func_ptr,error %s", dlerror());
				goto response;
			}

			/* Initialize HSM */
			rc = (* hsm_func_ptr)();
			if ( NCSCC_RC_SUCCESS != rc ) {
				LOG_ER("plms_session_close failed");
				goto response;
			}
		}

		/* PLMC finalize */
		if(cb->plmc_initialized){
			rc = plmc_destroy();
			if (rc){
				LOG_ER("PLMC destroy failed.");
				rc = NCSCC_RC_FAILURE;
			}
			cb->plmc_initialized = FALSE;
		}
		if (prev_haState == SA_AMF_HA_QUIESCED) {
			plms_proc_quiesced_standby_role_change();
		}
		break;
	case SA_AMF_HA_QUIESCED:
		plms_quiesced_state_handler(invocation);
		break;
	case SA_AMF_HA_QUIESCING:
		plms_quiescing_state_handler(invocation);
		break;
	default:
		LOG_IN("INVALID HA AMF STATE\n");
		plms_invalid_state_handler(invocation);
	}

	if (new_haState == SA_AMF_HA_QUIESCED)
		return;

	/* Update control block */
	cb->ha_state = new_haState;

	if (cb->csi_assigned == FALSE) {
		cb->csi_assigned = TRUE;
		/* We shall open checkpoint only once in our life time. currently doing at lib init  */
	} else if ((new_haState == SA_AMF_HA_ACTIVE) || (new_haState == SA_AMF_HA_STANDBY)) {	/* It is a switch over */
	/* check if this step is required */
	/*	cb->ckpt_state = COLD_SYNC_IDLE; */
	}

	if ((prev_haState == SA_AMF_HA_ACTIVE) && (new_haState == SA_AMF_HA_ACTIVE)) {
		role_change = FALSE;
	}

	if ((prev_haState == SA_AMF_HA_STANDBY) && (new_haState == SA_AMF_HA_STANDBY)) {
		role_change = FALSE;
	}

	if (role_change == TRUE) {
		if ((rc = plms_mds_change_role()) != NCSCC_RC_SUCCESS) {
			 LOG_ER("plms_mds_change_role FAILED");
			 error = SA_AIS_ERR_FAILED_OPERATION;
		         goto response;
		} else {
			if (cb->ha_state == SA_AMF_HA_ACTIVE) {
				/* FIXME: Is there any overhead to be done on New Active */
//				if (plms_imm_declare_implementer(cb->immOiHandle) != SA_AIS_OK)
//					printf("ClassImplementer Set Failed\n");
			}
			if (cb->ha_state == SA_AMF_HA_STANDBY) {
				/* FIXME : Do the over head processing needed to be done for standby state */
			}
		}
		TRACE_5("Inform MBCSV of HA state change to %s",
                        (new_haState == SA_AMF_HA_ACTIVE) ? "ACTIVE" : "STANDBY");

                if (plms_mbcsv_chgrole() != NCSCC_RC_SUCCESS) {
                        LOG_ER("Failed to change role");
                        error = SA_AIS_ERR_FAILED_OPERATION;
                        goto response;
                }
	}
response:
	/* Send the response to AMF */
	saAmfResponse(cb->amf_hdl, invocation, error);

	m_NCS_UNLOCK(&cb->cb_lock,NCS_LOCK_WRITE);
	TRACE_LEAVE();
} /* End of PLMS CSI Set callback */	

/****************************************************************************
 * Name          : plms_amf_comp_terminate_callback
 *
 * Description   : This is the callback function which will be called 
 *                 when the AMF framework needs to terminate PLMS. This does
 *                 all required operations to destroy PLMS(except to 
 *                 unregister from AMF)
 *
 * Arguments     : invocation     - This parameter designated a particular 
 *                                  invocation of this callback function. The
 *                                  invoke process return invocation when it 
 *                                  responds to the Avilability Management 
 *                                  FrameWork using the saAmfResponse() 
 *                                  function.
 *                 compName       - A pointer to the name of the component 
 *                                  whose readiness stae the Availability 
 *                                  Management Framework is setting.
 *
 * Return Values : None
 *
 * Notes         : At present we are just support a simple liveness check.
 *****************************************************************************/
void plms_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT 
					*compName)
{
	PLMS_CB *cb = plms_cb;
	SaAisErrorT error = SA_AIS_OK;

	TRACE_ENTER();
	m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_WRITE);

	saAmfResponse(cb->amf_hdl, invocation, error);
	/* FIXME : Clean up all internal structures */

	/* Detach from IPC */
	m_NCS_IPC_DETACH(&cb->mbx, NULL, cb);
	/* Disconnect from MDS */
	plms_mds_unregister();
	m_NCS_UNLOCK(&cb->cb_lock,NCS_LOCK_WRITE);

	/* Destroy the cb */

	sleep(1);
	exit(0);

	return;
}

/****************************************************************************
 * Name          : plms_amf_csi_rmv_callback
 *
 * Description   : This callback routine is invoked by AMF during a
 *                 CSI set removal operation. 
 *
 * Arguments     : invocation     - This parameter designated a particular
 *                                  invocation of this callback function. The
 *                                  invoke process return invocation when it
 *                                  responds to the Avilability Management
 *                                  FrameWork using the saAmfResponse()
 *                                  function.
 *                 compName       - A pointer to the name of the component
 *                                  whose readiness state the Availability
 *                                  Management Framework is setting.
 *                 csiName        - A const pointer to csiName
 *                 csiFlags       - csi Flags
 * Return Values : None 
 *****************************************************************************/
void
plms_amf_csi_rmv_callback(SaInvocationT invocation,
			 const SaNameT *compName, const SaNameT *csiName, const 							SaAmfCSIFlagsT csiFlags)
{
	PLMS_CB *cb = plms_cb;
	SaAisErrorT error = SA_AIS_OK;

	TRACE_ENTER();
	m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_READ);
	saAmfResponse(plms_cb->amf_hdl, invocation, error);
	m_NCS_UNLOCK(&cb->cb_lock,NCS_LOCK_READ);
	TRACE_LEAVE();
	return;
}

/*****************************************************************************\
 *  Name:          plms_healthcheck_start                                     * 
 *                                                                            *
 *  Description:   To start the health check                                  *
 *                                                                            *
 *  Arguments:     NULL                                                       * 
 *                                                                            * 
 *  Returns:       NCSCC_RC_SUCCESS   - everything is OK                      *
 *                 NCSCC_RC_FAILURE   -  failure                              *
 *  NOTE:                                                                     * 
\******************************************************************************/
static uns32 plms_healthcheck_start()
{
	SaAisErrorT error;
	SaAmfHealthcheckKeyT Healthy;
	char *health_key = 0;
	PLMS_CB * cb = plms_cb;

	TRACE_ENTER();

	if (cb->healthCheckStarted == TRUE) {
		return NCSCC_RC_SUCCESS;
	}

   /** start the AMF health check **/
	memset(&Healthy, 0, sizeof(Healthy));
	health_key = getenv("PLMS_ENV_HEALTHCHECK_KEY");
	if (health_key == NULL || strlen(health_key) > SA_AMF_HEALTHCHECK_KEY_MAX) {
		strcpy((char *)Healthy.key, "PL12");
		/* Log it */
	} else {
		strncpy((char *)Healthy.key, health_key, SA_AMF_HEALTHCHECK_KEY_MAX);
	}
	Healthy.keyLen = strlen((char *)Healthy.key);

	error = saAmfHealthcheckStart(cb->amf_hdl, &cb->comp_name, &Healthy,SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_NODE_FAILFAST);

	if (error != SA_AIS_OK) {
		LOG_ER("Health Check start failed");
		return NCSCC_RC_FAILURE;
	} else {
		cb->healthCheckStarted = TRUE;
		LOG_ER("Health Check started successfully");
		return NCSCC_RC_SUCCESS;
	}
}

/****************************************************************************
 * Name          : plms_amf_init
 *
 * Description   : PLMS initializes AMF for invoking process and registers 
 *                 the various callback functions.
 *
 * Arguments     : PLMS_CB - PLMS control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
SaUint32T plms_amf_init()
{
	PLMS_CB * cb = plms_cb;
	SaAmfCallbacksT amfCallbacks;
	SaVersionT amf_version;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

        if (amf_comp_name_get_set_from_file("PLMD_COMP_NAME_FILE", &cb->comp_name) != NCSCC_RC_SUCCESS)
                goto done;

	/* Initialize amf callbacks */
	memset(&amfCallbacks, 0, sizeof(SaAmfCallbacksT));

	amfCallbacks.saAmfHealthcheckCallback = plms_amf_health_chk_callback;
	amfCallbacks.saAmfCSISetCallback = plms_amf_CSI_set_callback;
	amfCallbacks.saAmfComponentTerminateCallback = plms_amf_comp_terminate_callback;
	amfCallbacks.saAmfCSIRemoveCallback = plms_amf_csi_rmv_callback;

	m_PLMS_GET_AMF_VER(amf_version);

	/*Initialize the amf library */

	rc = saAmfInitialize(&cb->amf_hdl, &amfCallbacks, &amf_version);

	if (rc != SA_AIS_OK) {
		LOG_ER("  plms_amf_init: saAmfInitialize() AMF initialization FAILED\n");
		goto done;
	}
	LOG_IN("  plms_amf_init: saAmfInitialize() AMF initialization SUCCESS\n");

	/* Obtain the amf selection object to wait for amf events */
	if (SA_AIS_OK != (rc = saAmfSelectionObjectGet(cb->amf_hdl, &cb->amf_sel_obj))) {
		LOG_ER("saAmfSelectionObjectGet() FAILED\n");
		goto done;
	}
	LOG_IN("saAmfSelectionObjectGet() SUCCESS\n");

	/* get the component name */

	rc = saAmfComponentNameGet(cb->amf_hdl, &cb->comp_name);
	if (rc != SA_AIS_OK) {
		LOG_ER("  plmss_amf_init: saAmfComponentNameGet() FAILED\n");
		goto done ;
	}

	rc = NCSCC_RC_SUCCESS;
done:
        TRACE_LEAVE2("%u, %s", rc, cb->comp_name.value);
        return rc;

}	/*End plms_amf_init */

/**************************************************************************
 Function: plms_amf_register

 Purpose:  Function which registers PLMS with AMF.  

 Input:    Pointer to the PLMS control block. 

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  Here we call plms_amf_init after reading the component name file and
         setting the environment varaiable in our own context.
         Proceed to register with AMF, since it has come up. 
**************************************************************************/
SaUint32T plms_amf_register()
{

	SaAisErrorT error;
	uns32 rc = NCSCC_RC_SUCCESS;
	PLMS_CB * cb = plms_cb;

	m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	/* Initialize amf framework for hc interface */
	if ((rc = plms_amf_init()) != NCSCC_RC_SUCCESS) {
		LOG_ER("AMF init failed");
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		return NCSCC_RC_FAILURE;
	}

	LOG_IN("AMF init SUCCESS");
	/* register PLMS component with AvSv */
	error = saAmfComponentRegister(cb->amf_hdl, &cb->comp_name, (SaNameT *)NULL);
	if (error != SA_AIS_OK) {
		LOG_ER("AMF Component Register failed");
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		return NCSCC_RC_FAILURE;
	}
	LOG_IN("AMF Component Register SUCCESS");
	if (NCSCC_RC_SUCCESS != (rc = plms_healthcheck_start())) {
		LOG_ER("PLMS Health Check Start failed");
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		return NCSCC_RC_FAILURE;
	}
	LOG_IN("PLMS Health Check Started Successfully SUCCESS");
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	return NCSCC_RC_SUCCESS;
}
