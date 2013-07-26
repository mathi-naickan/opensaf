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
  FILE NAME: GLND_AMF.C

  DESCRIPTION: GLND AMF callback routines.

  FUNCTIONS INCLUDED in this module:
******************************************************************************/

#include "glnd.h"

/* 
  DESCRIPTION: GLND AMF callback routines.

  FUNCTIONS INCLUDED in this module:
  glnd_saf_readiness_state_callback ........... GLND SAF readiness callback.
  glnd_saf_health_chk_callback................. GLND SAF Health Check callback.

******************************************************************************/

#include "glnd.h"
void glnd_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName);
void glnd_saf_health_chk_callback(SaInvocationT invocation,
				  const SaNameT *compName, const SaAmfHealthcheckKeyT *checkType);

void glnd_amf_CSI_set_callback(SaInvocationT invocation, const SaNameT *compName, SaAmfHAStateT haState,
			       SaAmfCSIDescriptorT csiDescriptor);

void glnd_amf_csi_rmv_callback(SaInvocationT invocation,
			       const SaNameT *compName, const SaNameT *csiName, SaAmfCSIFlagsT csiFlags);

/****************************************************************************
 * Name          : glnd_saf_health_chk_callback
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
void glnd_saf_health_chk_callback(SaInvocationT invocation,
				  const SaNameT *compName, const SaAmfHealthcheckKeyT *checkType)
{
	GLND_CB *glnd_cb;
	SaAisErrorT error = SA_AIS_OK;
	TRACE_ENTER2("component name %s", compName->value);

	/* take the handle */
	glnd_cb = (GLND_CB *)m_GLND_TAKE_GLND_CB;
	if (!glnd_cb) {
		LOG_ER("GLND cb take handle failed");
		goto end;
	}
	if (saAmfResponse(glnd_cb->amf_hdl, invocation, error) != SA_AIS_OK)
		LOG_ER("GLND amf response failed");
	/* giveup the handle */
	m_GLND_GIVEUP_GLND_CB;
end:
	TRACE_LEAVE();
	return;
}

/****************************************************************************
 * Name          :glnd_amf_comp_terminate_callback 
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
void glnd_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName)
{
	GLND_CB *glnd_cb;
	SaAisErrorT error = SA_AIS_OK;
	TRACE_ENTER2("Component Name: %s", compName->value);

	/* take the handle */
	glnd_cb = (GLND_CB *)m_GLND_TAKE_GLND_CB;
	if (!glnd_cb) {
		LOG_ER("GLND cb take handle failed");
	} else {
		if (saAmfResponse(glnd_cb->amf_hdl, invocation, error) != SA_AIS_OK)
			LOG_ER("GLND amf response failed");
		/* giveup the handle */
		m_GLND_GIVEUP_GLND_CB;
	}
	LOG_NO("Received AMF component terminate callback, exiting");
	TRACE_LEAVE();

	_Exit(EXIT_SUCCESS);
}

/****************************************************************************
 * Name          : glnd_amf_init
 *
 * Description   : GLD initializes AMF for involking process and registers 
 *                 the various callback functions.
 *
 * Arguments     : glnd_cb  - Ifsv control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glnd_amf_init(GLND_CB *glnd_cb)
{
	SaAmfCallbacksT amfCallbacks;
	SaVersionT amf_version;
	SaAisErrorT error;
	uint32_t res = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&amfCallbacks, 0, sizeof(SaAmfCallbacksT));

	amfCallbacks.saAmfHealthcheckCallback = (SaAmfHealthcheckCallbackT)glnd_saf_health_chk_callback;
	amfCallbacks.saAmfCSISetCallback = glnd_amf_CSI_set_callback;
	amfCallbacks.saAmfComponentTerminateCallback = glnd_amf_comp_terminate_callback;
	amfCallbacks.saAmfCSIRemoveCallback = glnd_amf_csi_rmv_callback;

	m_GLSV_GET_AMF_VER(amf_version);

	error = saAmfInitialize(&glnd_cb->amf_hdl, &amfCallbacks, &amf_version);
	if (error != SA_AIS_OK) {
		res = NCSCC_RC_FAILURE;
		TRACE_2("Amf initialization failed");
		goto end;
	}

	error = saAmfComponentNameGet(glnd_cb->amf_hdl, &glnd_cb->comp_name);
	if (error != SA_AIS_OK) {
		res = NCSCC_RC_FAILURE;
		TRACE_2("saAmfComponentNameGet api failed");
	}
end:
	TRACE_LEAVE2("%s return value %d", (res == NCSCC_RC_SUCCESS)?"SUCCESS":"FAILURE",res);
	return res;
}

/****************************************************************************\
 PROCEDURE NAME :glnd_amf_CSI_set_callback 

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
 RETURNS       : None.
\*****************************************************************************/
void glnd_amf_CSI_set_callback(SaInvocationT invocation,
			       const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor)
{
	GLND_CB *glnd_cb = NULL;
	SaAisErrorT saErr = SA_AIS_OK;
	uint32_t cb_hdl = m_GLND_RETRIEVE_GLND_CB_HDL;
	TRACE_ENTER2("component Name %s hastate: %d", compName->value, haState);

	/* Get the CB from the handle */
	glnd_cb = ncshm_take_hdl(NCS_SERVICE_ID_GLND, cb_hdl);

	if (!glnd_cb) {
		LOG_ER("GLND cb take handle failed");
		goto end;
	}

	if (glnd_cb) {
		glnd_cb->ha_state = haState;	/* Set the HA State */
		TRACE("glnd_amf_CSI_set_callback setting the state as %d", haState);

	}

	saAmfResponse(glnd_cb->amf_hdl, invocation, saErr);

	/* giveup the handle */
	ncshm_give_hdl(cb_hdl);
end:
	TRACE_LEAVE();
	return;
}

/****************************************************************************
 * Name          : glnd_amf_csi_rmv_callback
 *
 * Description   : TBD
 *
 *
 * Return Values : None
 *****************************************************************************/
void glnd_amf_csi_rmv_callback(SaInvocationT invocation,
			       const SaNameT *compName, const SaNameT *csiName, SaAmfCSIFlagsT csiFlags)
{
	GLND_CB *glnd_cb;
	SaAisErrorT error = SA_AIS_OK;
	TRACE_ENTER2("Component Name %s", compName->value);

	/* take the handle */
	glnd_cb = (GLND_CB *)m_GLND_TAKE_GLND_CB;
	if (!glnd_cb) {
		LOG_ER("GLND cb take handle failed");
		goto end;
	}
	if (saAmfResponse(glnd_cb->amf_hdl, invocation, error) != SA_AIS_OK)
		LOG_ER("GLND amf response failed");
	/* giveup the handle */
	m_GLND_GIVEUP_GLND_CB;
end:
	TRACE_LEAVE();
	return;
}

/****************************************************************************
 * Name          : glnd_amf_de_init
 *
 * Description   : GLND uninitializes AMF for involking process.
 *
 * Arguments     : glnd_cb  - Ifsv control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
void glnd_amf_de_init(GLND_CB *glnd_cb)
{
	if (saAmfFinalize(glnd_cb->amf_hdl) != SA_AIS_OK)
		LOG_ER("GLND amf destroy failed");
}

/****************************************************************************
 * Name          : glnd_amf_register
 *
 * Description   : GLND registers with AMF for involking process.
 *
 * Arguments     : glnd_cb  - Ifsv control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glnd_amf_register(GLND_CB *glnd_cb)
{
	if (saAmfComponentRegister(glnd_cb->amf_hdl, &glnd_cb->comp_name, (SaNameT *)NULL) == SA_AIS_OK)
		return NCSCC_RC_SUCCESS;
	else
		return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : glnd_amf_deregister
 *
 * Description   : GLND deregisters with AMF for involking process.
 *
 * Arguments     : glnd_cb  - Ifsv control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t glnd_amf_deregister(GLND_CB *glnd_cb)
{
	if (saAmfComponentUnregister(glnd_cb->amf_hdl, &glnd_cb->comp_name, (SaNameT *)NULL) == SA_AIS_OK)
		return NCSCC_RC_SUCCESS;
	else
		return NCSCC_RC_FAILURE;
}
