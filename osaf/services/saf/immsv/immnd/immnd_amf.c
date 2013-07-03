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

#include "immnd.h"
#include <nid_start_util.h>

/****************************************************************************
 * Name          : immnd_saf_health_chk_callback
 *
 * Description   : This function SAF callback function which will be called 
 *                 when the AMF framework needs to health for the component.
 *
 * Arguments     : invocation     - This parameter designated a particular 
 *                                  invocation of this callback function. The
 *                                  invoke process return invocation when it 
 *                                  responds to the Availability Management 
 *                                  FrameWork using the saAmfResponse() 
 *                                  function.
 *                 compName       - A pointer to the name of the component 
 *                                  whose readiness stae the Availability 
 *                                  Management Framework is setting.
 *                 checkType      - The type of healthcheck to be executed. 
 *
 * Return Values : None
 *
 * Notes         : At present we are just support a simple liveness check.
 *****************************************************************************/
static void immnd_saf_health_chk_callback(SaInvocationT invocation,
					  const SaNameT *compName, SaAmfHealthcheckKeyT *checkType)
{
	TRACE_ENTER();

	if (saAmfResponse(immnd_cb->amf_hdl, invocation, SA_AIS_OK) != SA_AIS_OK)
		LOG_ER("Amf Response Failed");
}

/****************************************************************************
 * Name          : immnd_amf_comp_terminate_callback
 *
 * Description   : This function SAF callback function which will be called 
 *                 when the AMF framework needs to terminate GLSV. This does
 *                 all required to destroy GLSV(except to unregister from AMF)
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
static void immnd_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName)
{
	TRACE_ENTER();
	saAmfResponse(immnd_cb->amf_hdl, invocation, SA_AIS_OK);
	sleep(1);
	TRACE_LEAVE();
	LOG_NO("Received AMF component terminate callback, exiting");
	exit(0);
}

/****************************************************************************
 * Name          : immnd_amf_csi_rmv_callback
 *
 * Description   : TBD
 *
 *
 * Return Values : None 
 *****************************************************************************/
static void immnd_amf_csi_rmv_callback(SaInvocationT invocation,
				       const SaNameT *compName, const SaNameT *csiName, SaAmfCSIFlagsT csiFlags)
{
	TRACE_ENTER();
	saAmfResponse(immnd_cb->amf_hdl, invocation, SA_AIS_OK);
	TRACE_LEAVE();
	return;
}

/****************************************************************************\
 PROCEDURE NAME : immnd_saf_csi_set_cb
 
 DESCRIPTION    : This function SAF callback function which will be called 
                  when there is any change in the HA state.
 
 ARGUMENTS      : invocation     - This parameter designated a particular 
                                  invocation of this callback function. The 
                                  invoke process return invocation when it 
                                  responds to the Avilability Management 
                                  FrameWork using the saAmfResponse() 
                                  function.
                 compName       - A pointer to the name of the component 
                                  whose readiness stae the Availability 
                                  Management Framework is setting.
                 haState        - The new HA state to be assumeb by the 
                                  component service instance identified by 
                                  csiName.
\****************************************************************************/
static void immnd_saf_csi_set_cb(SaInvocationT invocation,
				 const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor)
{
	SaAisErrorT saErr = SA_AIS_OK;

	TRACE_ENTER();
	immnd_cb->ha_state = haState;	/* Set the HA State */
	/*TODO: this ha_state is never accessed by any other part of immnd.
	   Perhaps it should be? Perhaps we should wait in responding until
	   we have synced? Perhaps we should check this state as being active
	   before allowing any service request to be completed? In particular
	   mutating requests such as create a new CCB. */

	saAmfResponse(immnd_cb->amf_hdl, invocation, saErr);
	TRACE_2("CSI Set Callback Invoked ha-state:%u", haState);
	TRACE_LEAVE();
	return;
}

/**
 * Thread start routine to register with AMF and start health check
 * Error handling: exits process at failure
 * @param __cb control block pointer
 */
static void* amf_init_start(void *__cb)
{
	SaAisErrorT error;
	IMMND_CB *cb = (IMMND_CB *) __cb;
	const char *health_key;
	SaAmfHealthcheckKeyT healthy;

	TRACE_ENTER();

	error = saAmfComponentRegister(cb->amf_hdl, &cb->comp_name, NULL);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfComponentRegister failed: %u, exiting", error);
		exit(1);
	}

	/*   start the AMF Health Check  */
	memset(&healthy, 0, sizeof(healthy));
	health_key = getenv("IMMSV_ENV_HEALTHCHECK_KEY");

	if (health_key == NULL) {
		strncpy((char *)healthy.key, "A1B2", sizeof(healthy.key));
		healthy.keyLen = (SaUint16T) strlen((char *)healthy.key);
	} else {
		healthy.keyLen = (SaUint16T) strlen((char *)health_key);
		if (healthy.keyLen < sizeof(healthy.key)) {
			strncpy((char *)healthy.key, health_key, sizeof(healthy.key));
		} else {
			LOG_ER("Health check key too long:%u, exiting", healthy.keyLen);
			exit(1);
		}
	}

	error = saAmfHealthcheckStart(cb->amf_hdl, &cb->comp_name, &healthy,
				      SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_RESTART);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfHealthcheckStart failed: %u, exiting", error);
		exit(1);
	}

	TRACE_LEAVE();
	return NULL;
}

/****************************************************************************
 * Name          : immnd_amf_init
 *
 * Description   : IMMND initializes AMF for invoking process and registers
 *                 the various callback functions. Non blocking.
 *
 * Arguments     : immnd_cb - control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *                 At SUCCESS the following control block members are initialized:
 *                    amf_hdl
 *                    comp_name
 *                    amf_sel_obj
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t immnd_amf_init(IMMND_CB *cb)
{
	SaAmfCallbacksT amfCallbacks;
	SaVersionT version;
	SaAisErrorT error;
	uint32_t res = NCSCC_RC_FAILURE;
	pthread_t thread;
	pthread_attr_t attr;

	TRACE_ENTER();

	if (cb->nid_started &&
		(amf_comp_name_get_set_from_file("IMMND_COMP_NAME_FILE",
			&cb->comp_name) != NCSCC_RC_SUCCESS))
		goto done;

	memset(&amfCallbacks, 0, sizeof(SaAmfCallbacksT));
	amfCallbacks.saAmfHealthcheckCallback = immnd_saf_health_chk_callback;
	amfCallbacks.saAmfCSISetCallback = immnd_saf_csi_set_cb;
	amfCallbacks.saAmfComponentTerminateCallback = immnd_amf_comp_terminate_callback;
	amfCallbacks.saAmfCSIRemoveCallback = immnd_amf_csi_rmv_callback;

	m_IMMSV_GET_AMF_VER(version);

	/*
	 * Perform the non blocking part of initialization.
	 * If the AMF implementation of these calls would change and become
	 * synchronous, this code would have to be changed too.
	 */
	error = saAmfInitialize(&cb->amf_hdl, &amfCallbacks, &version);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfInitialize failed");
		goto done;
	}

	error = saAmfComponentNameGet(cb->amf_hdl, &cb->comp_name);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfComponentNameGet failed");
		goto done;
	}

	error = saAmfSelectionObjectGet(cb->amf_hdl, &cb->amf_sel_obj);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfSelectionObjectGet failed");
		goto done;
	}

	/* Start a thread to take care of the blocking part of initialization */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&thread, &attr, amf_init_start, cb) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		goto done;
	}

	pthread_attr_destroy(&attr);

	res = NCSCC_RC_SUCCESS;

 done:
	TRACE_LEAVE2("%u, %s", res, cb->comp_name.value);
	return (res);
}
