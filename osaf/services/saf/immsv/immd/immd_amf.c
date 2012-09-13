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

#include <logtrace.h>
#include <nid_start_util.h>
#include "immd.h"
#include "immsv.h"

/**
 * Return string describing HA state
 * @param haState
 * 
 * @return const char*
 */
static const char *ha_state_name(SaAmfHAStateT haState)
{
	switch (haState) {
	case SA_AMF_HA_ACTIVE:
		return "SA_AMF_HA_ACTIVE";
		break;
	case SA_AMF_HA_STANDBY:
		return "SA_AMF_HA_STANDBY";
		break;
	case SA_AMF_HA_QUIESCED:
		return "SA_AMF_HA_QUIESCED";
		break;
	case SA_AMF_HA_QUIESCING:
		return "SA_AMF_HA_QUIESCING";
		break;
	default:
		return "UNKNOWN";
		break;
	}
}

/****************************************************************************
 PROCEDURE NAME : immd_saf_hlth_chk_cb

 DESCRIPTION    : This function SAF callback function which will be called 
                  when the AMF framework needs to health for the component.
 
 ARGUMENTS      : invocation     - This parameter designated a particular 
                                   invocation of this callback function. The
                                   invoke process return invocation when it 
                                   responds to the Avilability Management 
                                   FrameWork using the saAmfResponse() 
                                   function.
                  compName       - A pointer to the name of the component 
                                   whose readiness stae the Availability 
                                   Management Framework is setting.
                  checkType      - The type of healthcheck to be executed. 
 
  RETURNS       : None 
  NOTES         : At present there is only support for a simple liveness check.
*****************************************************************************/
static void immd_saf_hlth_chk_cb(SaInvocationT invocation, const SaNameT *compName, SaAmfHealthcheckKeyT *checkType)
{
	TRACE_ENTER();
	saAmfResponse(immd_cb->amf_hdl, invocation, SA_AIS_OK);
	TRACE_LEAVE();
}

/****************************************************************************
 * Name          : amf_active_state_handler
 *
 * Description   : This function is called upon receving an active state
 *                 assignment from AMF.
 *
 * Arguments     : invocation - Designates a particular invocation.
 *                 cb         - A pointer to the IMMD control block. 
 *****************************************************************************/
static SaAisErrorT amf_active_state_handler(IMMD_CB *cb, SaInvocationT invocation)
{
	SaAisErrorT error = SA_AIS_OK;
	LOG_IN("AMF HA ACTIVE request");

	cb->mds_role = V_DEST_RL_ACTIVE;

	return error;
}

/****************************************************************************
 * Name          : amf_standby_state_handler
 *
 * Description   : This function is called upon receving an standby state
 *                 assignment from AMF.
 *
 * Arguments     : invocation - Designates a particular invocation.
 *                 cb         - A pointer to the IMMD control block. 
 *
 *****************************************************************************/
static SaAisErrorT amf_standby_state_handler(IMMD_CB *cb, SaInvocationT invocation)
{
	LOG_IN("AMF HA STANDBY request");
	cb->mds_role = V_DEST_RL_STANDBY;
	return SA_AIS_OK;
}

/****************************************************************************
 * Name          : amf_quiescing_state_handler
 *
 * Description   : This function is called upon receving an Quiescing state
 *                 assignment from AMF.
 *
 * Arguments     : invocation - Designates a particular invocation.
 *                 cb         - A pointer to the IMMD control block. 
 *
 *****************************************************************************/
static SaAisErrorT amf_quiescing_state_handler(IMMD_CB *cb, SaInvocationT invocation)
{
	LOG_IN("AMF HA QUIESCING request");
	/*anything to close down ? */
	return saAmfCSIQuiescingComplete(cb->amf_hdl, invocation, SA_AIS_OK);
}

/****************************************************************************
 * Name          : amf_quiesced_state_handler
 *
 * Description   : This function is called upon receving an Quiesced state
 *                 assignment from AMF.
 *
 * Arguments     : invocation - Designates a particular invocation.
 *                 cb         - A pointer to the IMMD control block. 
 *****************************************************************************/
static SaAisErrorT amf_quiesced_state_handler(IMMD_CB *cb, SaInvocationT invocation)
{
	LOG_IN("AMF HA QUIESCED request");

	/*
	 ** Change the MDS VDSET role to Quiesced. Wait for MDS callback with type
	 ** MDS_CALLBACK_QUIESCED_ACK. Then change MBCSv role. Don't change
	 ** cb->ha_state now.
	 */

	cb->mds_role = V_DEST_RL_QUIESCED;
	immd_mds_change_role(cb);
	cb->amf_invocation = invocation;
	cb->is_quiesced_set = true;
	TRACE_LEAVE();
	return SA_AIS_OK;
}

/****************************************************************************\
 PROCEDURE NAME : immd_saf_csi_set_cb
 
 DESCRIPTION    : This is a SAF callback function which will be called 
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
                 csiDescriptor  - 

 RETURNS       : Nothing.
\*****************************************************************************/
static void immd_saf_csi_set_cb(SaInvocationT invocation,
				const SaNameT *compName, SaAmfHAStateT new_haState, SaAmfCSIDescriptorT csiDescriptor)
{
	SaAisErrorT error = SA_AIS_OK;
	SaAmfHAStateT prev_ha_state;
	bool role_change = true;
	uint32_t rc = NCSCC_RC_SUCCESS;
	IMMD_CB *cb = immd_cb;

	TRACE_ENTER();

	prev_ha_state = cb->ha_state;

	/* Invoke the appropriate state handler routine */
	switch (new_haState) {
	case SA_AMF_HA_ACTIVE:
		error = amf_active_state_handler(cb, invocation);
		break;
	case SA_AMF_HA_STANDBY:
		error = amf_standby_state_handler(cb, invocation);
		break;
	case SA_AMF_HA_QUIESCED:
		/* switch over */
		error = amf_quiesced_state_handler(cb, invocation);
		break;
	case SA_AMF_HA_QUIESCING:
		/* shut down */
		error = amf_quiescing_state_handler(cb, invocation);
		break;
	default:
		LOG_WA("invalid state: %d ", new_haState);
		error = SA_AIS_ERR_FAILED_OPERATION;
		break;
	}

	if (error != SA_AIS_OK)
		goto response;

	if (new_haState == SA_AMF_HA_QUIESCED) {
		/*Note: should we not change state in cb->ha_state here.
		   This is done in immd_mds_quiesced_ack_process */
		goto done;
	}

	/* Update control block */
	cb->ha_state = new_haState;

	TRACE_5("New-state: %s, prev-state: %s", ha_state_name(new_haState), ha_state_name(prev_ha_state));

	/* Handle active to active role change. */
	if (prev_ha_state == new_haState) {
		TRACE_5("No role change!");	/* bug? */
		role_change = false;
	}

	if (role_change) {
		if ((rc = immd_mds_change_role(cb)) != NCSCC_RC_SUCCESS) {
			LOG_WA("immd_mds_change_role FAILED");
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto response;
		}

		TRACE_5("Inform MBCSV of HA state change to %s",
			(new_haState == SA_AMF_HA_ACTIVE) ? "ACTIVE" : "STANDBY");

		if (immd_mbcsv_chgrole(cb) != NCSCC_RC_SUCCESS) {
			LOG_WA("immd_mbcsv_chgrole FAILED");
			error = SA_AIS_ERR_FAILED_OPERATION;
			goto response;
		}

		if (new_haState == SA_AMF_HA_ACTIVE) {
			/* Change of role to active => We may need to elect new coord */
			immd_proc_elect_coord(cb, true);
			immd_db_purge_fevs(cb);
		}
	}

 response:
	saAmfResponse(cb->amf_hdl, invocation, error);
 done:
	TRACE_LEAVE();
}

/****************************************************************************
 * Name          : immd_amf_comp_terminate_callback
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
static void immd_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName)
{
	IMMD_CB *cb = immd_cb;
	SaAisErrorT saErr = SA_AIS_OK;
	LOG_IN("IMMD - AMF Component Termination Callback Invoked, exiting...");

	saAmfResponse(cb->amf_hdl, invocation, saErr);
	immd_mds_unregister(cb);
	/* unreg with mbcp also ?? */
	sleep(1);
	LOG_NO("Received AMF component terminate callback, exiting");
	exit(0);
}

/****************************************************************************
 * Name          : immd_amf_csi_rmv_callback
 *
 * Description   : TBD
 *
 *
 * Return Values : None 
 *****************************************************************************/
static void immd_amf_csi_rmv_callback(SaInvocationT invocation,
				      const SaNameT *compName, const SaNameT *csiName, SaAmfCSIFlagsT csiFlags)
{
	IMMD_CB *cb = immd_cb;
	SaAisErrorT saErr = SA_AIS_OK;
	TRACE_ENTER();
	LOG_IN("IMMD - AMF CSI Remove Callback Invoked");

	saAmfResponse(cb->amf_hdl, invocation, saErr);
	TRACE_LEAVE();
}

/**
 * Thread start routine to register with AMF and start health check
 * Error handling: exits process at failure
 * @param __cb control block pointer
 */
static void* amf_init_start(void *notused)
{
	SaAisErrorT error;
	const char *health_key;
	SaAmfHealthcheckKeyT healthy;

	TRACE_ENTER();

	error = saAmfComponentRegister(immd_cb->amf_hdl, &immd_cb->comp_name, NULL);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfComponentRegister failed: %u, exiting", error);
		exit(1);
	}

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

	error = saAmfHealthcheckStart(immd_cb->amf_hdl, &immd_cb->comp_name,
		&healthy, SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_FAILOVER);

	if (error != SA_AIS_OK) {
		LOG_ER("saAmfHealthcheckStart failed: %u, exiting", error);
		exit(1);
	}

	TRACE_LEAVE();
	return NULL;
}

/****************************************************************************
 * Name          : amf_init
 *
 * Description   : Initialize AMF for involking process and register
 *                 the various callback functions.
 *
 * Arguments     : immd_cb  - IMMD control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t immd_amf_init(IMMD_CB *immd_cb)
{
	static SaAmfCallbacksT amfCallbacks;
	SaVersionT amf_version;
	SaAisErrorT error;
	uint32_t res = NCSCC_RC_FAILURE;
	pthread_t thread;
	pthread_attr_t attr;

	TRACE_ENTER();

	if (amf_comp_name_get_set_from_file("IMMD_COMP_NAME_FILE",
			&immd_cb->comp_name) != NCSCC_RC_SUCCESS)
		goto done;

	memset(&amfCallbacks, 0, sizeof(SaAmfCallbacksT));

	amfCallbacks.saAmfHealthcheckCallback = immd_saf_hlth_chk_cb;
	amfCallbacks.saAmfCSISetCallback = immd_saf_csi_set_cb;
	amfCallbacks.saAmfComponentTerminateCallback = immd_amf_comp_terminate_callback;
	amfCallbacks.saAmfCSIRemoveCallback = immd_amf_csi_rmv_callback;

	m_IMMSV_GET_AMF_VER(amf_version);

	/*
	 * Perform the non blocking part of initialization.
	 * If the AMF implementation of these calls would change and become
	 * synchronous, this code would have to be changed too.
	 */
	error = saAmfInitialize(&immd_cb->amf_hdl, &amfCallbacks, &amf_version);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfInitialize failed");
		goto done;
	}

	error = saAmfComponentNameGet(immd_cb->amf_hdl, &immd_cb->comp_name);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfComponentNameGet failed");
		goto done;
	}

	error = saAmfSelectionObjectGet(immd_cb->amf_hdl, &immd_cb->amf_sel_obj);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfSelectionObjectGet failed");
		goto done;
	}

	/* Start a thread to take care of the blocking part of initialization */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&thread, NULL, amf_init_start, NULL) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		goto done;
	}

	pthread_attr_destroy(&attr);

	res = NCSCC_RC_SUCCESS;

 done:
	TRACE_LEAVE2("%u, %s", res, immd_cb->comp_name.value);
	return res;
}
