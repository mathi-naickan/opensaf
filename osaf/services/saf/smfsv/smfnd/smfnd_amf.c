/*      OpenSAF
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
 * This file implements the AMF interface for SMFD.
 * The interface exist of one exported function: smfd_amf_init().
 * The AMF callback functions except a number of exported functions from
 * other modules.
 */

#include "smfnd.h"

/****************************************************************************
 * Name          : amf_health_chk_callback
 *
 * Description   : This is the callback function which will be called 
 *                 when the AMF framework initiates a health check for the component.
 *
 * Arguments     : invocation - Designates a particular invocation.
 *                 compName       - A pointer to the name of the component 
 *                                  whose readiness stae the Availability 
 *                                  Management Framework is setting.
 *                 checkType      - The type of healthcheck to be executed. 
 *
 * Return Values : None
 *
 * Notes         : None
 *****************************************************************************/
static void amf_health_chk_callback(SaInvocationT invocation,
				    const SaNameT * compName, SaAmfHealthcheckKeyT * checkType)
{
	saAmfResponse(smfnd_cb->amf_hdl, invocation, SA_AIS_OK);
}

/****************************************************************************
 * Name          : amf_csi_set_callback
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
static void amf_csi_set_callback(SaInvocationT invocation,
				 const SaNameT * compName, SaAmfHAStateT new_haState, SaAmfCSIDescriptorT csiDescriptor)
{
	SaAisErrorT result = SA_AIS_OK;

	TRACE_ENTER();

	/* Nothing to be done. We are always active */
	smfnd_cb->ha_state = new_haState;

	saAmfResponse(smfnd_cb->amf_hdl, invocation, result);
	TRACE_LEAVE();
}

/****************************************************************************
 * Name          : amf_comp_terminate_callback
 *
 * Description   : This is the callback function which will be called 
 *                 when the AMF framework want to terminate SMFND. This does
 *                 all required to destroy SMFND(except to unregister from AMF)
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
 * Notes         : None
 *****************************************************************************/
static void amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT * compName)
{
	TRACE_ENTER();

	saAmfResponse(smfnd_cb->amf_hdl, invocation, SA_AIS_OK);

	TRACE_LEAVE();
	sleep(1);
	LOG_NO("Received AMF component terminate callback, exiting");
	exit(0);
}

/****************************************************************************
 * Name          : amf_csi_rmv_callback
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
 *                                  whose readiness stae the Availability
 *                                  Management Framework is setting.
 *                 csiName        - A const pointer to csiName
 *                 csiFlags       - csi Flags
 * Return Values : None 
 *****************************************************************************/
static void amf_csi_rmv_callback(SaInvocationT invocation,
				 const SaNameT * compName, const SaNameT * csiName, const SaAmfCSIFlagsT csiFlags)
{
	TRACE_ENTER();
	saAmfResponse(smfnd_cb->amf_hdl, invocation, SA_AIS_OK);
	TRACE_LEAVE();
}

/*****************************************************************************\
 *  Name:          amf_healthcheck_start                           * 
 *                                                                            *
 *  Description:   To start the health check                                  *
 *                                                                            *
 *  Arguments:     SMFND_CB* - Control Block                                   * 
 *                                                                            * 
 *  Returns:       SA_AIS_OK    - everything is OK                            *
 *                 SA_AIS_ERR_* -  failure                                    *
 *  NOTE:                                                                     * 
\******************************************************************************/
static SaAisErrorT amf_healthcheck_start(smfnd_cb_t * cb)
{
	SaAisErrorT result;
	SaAmfHealthcheckKeyT healthy;
	char *health_key;

	TRACE_ENTER();

    /** start the AMF health check **/
	memset(&healthy, 0, sizeof(healthy));
	health_key = getenv("SMFSV_ENV_HEALTHCHECK_KEY");

	if (health_key == NULL) {
		strcpy((char *)healthy.key, "A1B2");
	} else {
		if (strlen(health_key) > SA_AMF_HEALTHCHECK_KEY_MAX) {
			LOG_ER("amf_healthcheck_start(): Helthcheck key to long");
			return SA_AIS_ERR_NAME_TOO_LONG;
		}
		strcpy((char *)healthy.key, health_key);
	}

	healthy.keyLen = strlen((const char *)healthy.key);

	result = saAmfHealthcheckStart(cb->amf_hdl, &cb->comp_name, &healthy,
				       SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_FAILOVER);

	if (result != SA_AIS_OK)
		LOG_ER("saAmfHealthcheckStart FAILED: %u", result);

	TRACE_LEAVE();
	return result;
}

/**************************************************************************
 Function: smfnd_amf_init

 Purpose:  Function which initializes SMFND with AMF.  

 Input:    None 

 Returns:  SA_AIS_OK    - everything is OK
           SA_AIS_ERR_* - failure

**************************************************************************/
SaAisErrorT smfnd_amf_init(smfnd_cb_t * cb)
{
	SaAmfCallbacksT amfCallbacks;
	SaVersionT amf_version;
	SaAisErrorT result;

	TRACE_ENTER();

	/* Initialize AMF callbacks */
	memset(&amfCallbacks, 0, sizeof(SaAmfCallbacksT));
	amfCallbacks.saAmfHealthcheckCallback = amf_health_chk_callback;
	amfCallbacks.saAmfCSISetCallback = amf_csi_set_callback;
	amfCallbacks.saAmfComponentTerminateCallback = amf_comp_terminate_callback;
	amfCallbacks.saAmfCSIRemoveCallback = amf_csi_rmv_callback;

	amf_version.releaseCode = 'B';
	amf_version.majorVersion = 0x01;
	amf_version.minorVersion = 0x01;

	/* Initialize the AMF library */
	result = saAmfInitialize(&cb->amf_hdl, &amfCallbacks, &amf_version);
	if (result != SA_AIS_OK) {
		LOG_ER("saAmfInitialize() FAILED: %u", result);
		goto done;
	}

	/* Obtain the AMF selection object to wait for AMF events */
	result = saAmfSelectionObjectGet(cb->amf_hdl, &cb->amfSelectionObject);
	if (result != SA_AIS_OK) {
		LOG_ER("saAmfSelectionObjectGet() FAILED: %u", result);
		goto done;
	}

	/* Get the component name */
	result = saAmfComponentNameGet(cb->amf_hdl, &cb->comp_name);
	if (result != SA_AIS_OK) {
		LOG_ER("saAmfComponentNameGet() FAILED: %u", result);
		goto done;
	}

	/* Register component with AMF */
	result = saAmfComponentRegister(cb->amf_hdl, &cb->comp_name, (SaNameT *) NULL);
	if (result != SA_AIS_OK) {
		LOG_ER("saAmfComponentRegister() FAILED");
		goto done;
	}

	/* Start AMF healthchecks */
	if ((result = amf_healthcheck_start(cb)) != SA_AIS_OK) {
		goto done;
	}

 done:
	TRACE_LEAVE();
	return result;
}
