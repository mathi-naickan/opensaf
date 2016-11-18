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
 * Author(s):  GoAhead Software
 *
 */

#include "clmna.h"
#include "nid/agent/nid_start_util.h"

/*
 * This is the callback function which will be called when the AMF
 * health checksfor the component.
 *
 * @param invocation - Designates a particular invocation.
 * @param compName       - A pointer to the name of the component 
 * whose readiness stae the AMF  is setting.
 * @param checkType      - The type of healthcheck to be executed. 
 */
static void clmna_amf_health_chk_callback(SaInvocationT invocation, const SaNameT *compName,
					 SaAmfHealthcheckKeyT *checkType)
{
	saAmfResponse(clmna_cb->amf_hdl, invocation, SA_AIS_OK);
}

/*
 * The AMF callback function called when there is any 
 * change in the HA state.
 * @param invocation - This parameter designated a particular invocation of
 * this callback function. The invoke process return invocation when it 
 * responds to the AMF using the saAmfResponse() function.
 * @compName - A pointer to the name of the component whose readiness state 
 * the AMF is setting. 
 * @param haState - The new HA state to be assumed by the CSI identified by 
 * csiName.
 * @csiDescriptor - This will indicate whether or not the CSI for 
 * ativeCompName went through quiescing.
 */
static void clmna_amf_csi_set_callback(SaInvocationT invocation,
				      const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor)
{
	SaAisErrorT error = SA_AIS_OK;
	TRACE_ENTER();

	/* Update control block */
	clmna_cb->ha_state = haState;

	saAmfResponse(clmna_cb->amf_hdl, invocation, error);
	TRACE_LEAVE();
}

/*
 * This is the callback function which will be called 
 * when the AMF framework needs to terminate CLMS. This does
 * all required to destroy CLMS(except to unregister from AMF)
 *
 * @invocation - This parameter designated a particular 
 * invocation of this callback function. The invoke process return
 * invocation when it responds to the AMF using the saAmfResponse() 
 * function.
 * @compName - A pointer to the name of the component whose readiness
 * state the AMF is setting.
 */
void clmna_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName)
{
	TRACE_ENTER();

	saAmfResponse(clmna_cb->amf_hdl, invocation, SA_AIS_OK);

	/* Detach from IPC */
	m_NCS_IPC_DETACH(&clmna_cb->mbx, NULL, clmna_cb);

	TRACE_LEAVE();
	LOG_NO("Received AMF component terminate callback, exiting");
	_Exit(EXIT_SUCCESS);
}

/*
 * This callback routine is invoked by AMF during a CSI set removal operation. 
 *
 * @param invocation - This parameter designated a particular invocation of 
 * this callback function. The invoke process return invocation when it
 * responds to the AMF FrameWork using the saAmfResponse() function.
 * @param compName - A pointer to the name of the component whose readiness state
 * the AMF is setting.
 * @csiName        - A const pointer to csiName i
 * @csiFlags       - csi Flags
 */
void clmna_amf_csi_rmv_callback(SaInvocationT invocation,
				      const SaNameT *compName, const SaNameT *csiName, const SaAmfCSIFlagsT csiFlags)
{
	TRACE_ENTER();

	saAmfResponse(clmna_cb->amf_hdl, invocation, SA_AIS_OK);

	TRACE_LEAVE();
}

/*
 * Function to start the health checking with AMF 
 * @param:  CLMNA_CB - Control Block  
 * @return: SaAisErrorT 
 */
SaAisErrorT clmna_amf_healthcheck_start(CLMNA_CB * clmna_cb)
{
	SaAisErrorT error;
	SaAmfHealthcheckKeyT healthy;
	char *health_key;

	TRACE_ENTER();

    /** start the AMF health check **/
	memset(&healthy, 0, sizeof(healthy));
	health_key = getenv("CLMNA_ENV_HEALTHCHECK_KEY");

	if (health_key == NULL)
		strcpy((char *)healthy.key, "Default");
	else {
		if (strlen(health_key) > SA_AMF_HEALTHCHECK_KEY_MAX) {
			LOG_ER("amf_healthcheck_start(): Helthcheck key to long");
			return SA_AIS_ERR_NAME_TOO_LONG;
		}
		strcpy((char *)healthy.key, health_key);
	}

	healthy.keyLen = strlen((char *)healthy.key);

	error = saAmfHealthcheckStart(clmna_cb->amf_hdl, &clmna_cb->comp_name, &healthy,
				      SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_FAILOVER);

	if (error != SA_AIS_OK)
		LOG_ER("saAmfHealthcheckStart FAILED: %u", error);

	TRACE_LEAVE();
	return error;
}

/*  
 * Initializes and registers with AMF.  
 * @param: control block to CLMNA
 * @return : SaAisErrorT
 */

SaAisErrorT clmna_amf_init(CLMNA_CB * cb)
{
	SaAmfCallbacksT amfCallbacks;
	SaVersionT amf_version;
	SaAisErrorT error = SA_AIS_OK;

	TRACE_ENTER();

	if (cb->nid_started &&
		amf_comp_name_get_set_from_file("CLMNA_COMP_NAME_FILE", &cb->comp_name) != NCSCC_RC_SUCCESS)
		goto done;

	/* Initialize AMF callbacks */
	memset(&amfCallbacks, 0, sizeof(SaAmfCallbacksT));
	amfCallbacks.saAmfHealthcheckCallback = clmna_amf_health_chk_callback;
	amfCallbacks.saAmfCSISetCallback = clmna_amf_csi_set_callback;
	amfCallbacks.saAmfComponentTerminateCallback = clmna_amf_comp_terminate_callback;
	amfCallbacks.saAmfCSIRemoveCallback = clmna_amf_csi_rmv_callback;

	amf_version.releaseCode = 'B';
	amf_version.majorVersion = 0x01;
	amf_version.minorVersion = 0x01;

	/* Initialize the AMF library */
	error = saAmfInitialize(&cb->amf_hdl, &amfCallbacks, &amf_version);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfInitialize() FAILED: %u", error);
		goto done;
	}

	/* Obtain the AMF selection object to wait for AMF events */
	error = saAmfSelectionObjectGet(cb->amf_hdl, &cb->amf_sel_obj);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfSelectionObjectGet() FAILED: %u", error);
		goto done;
	}

	/* Get the component name */
	error = saAmfComponentNameGet(cb->amf_hdl, &cb->comp_name);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfComponentNameGet() FAILED: %u", error);
		goto done;
	}

	/* Register component with AMF */
	error = saAmfComponentRegister(cb->amf_hdl, &cb->comp_name, (SaNameT *)NULL);
	if (error != SA_AIS_OK) {
		LOG_ER("saAmfComponentRegister() FAILED: %u",error);
		goto done;
	}

	/* Start AMF healthchecks */
	if ((error = clmna_amf_healthcheck_start(cb)) != SA_AIS_OK){
		LOG_ER("clmna_amf_healthcheck_start() FAILED: %u",error);
		goto done;
	}

 done:
	TRACE_LEAVE();
	return error;

}

