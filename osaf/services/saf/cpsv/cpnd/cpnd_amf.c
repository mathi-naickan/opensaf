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
  FILE NAME: CPND_AMF.C

  DESCRIPTION: CPND AMF callback routines.

  FUNCTIONS INCLUDED in this module:
******************************************************************************/

#include "cpnd.h"

/* 
  DESCRIPTION: CPND AMF callback routines.

  FUNCTIONS INCLUDED in this module:
  cpnd_saf_readiness_state_callback ........... CPND SAF readiness callback.
  cpnd_saf_health_chk_callback................. CPND SAF Health Check callback.

******************************************************************************/

#include "cpnd.h"

/****************************************************************************
 * Name          : cpnd_saf_health_chk_callback
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
void
cpnd_saf_health_chk_callback(SaInvocationT invocation, const SaNameT *compName, const SaAmfHealthcheckKeyT *checkType)
{
	CPND_CB *cpnd_cb = NULL;
	SaAisErrorT error = SA_AIS_OK;
	uns32 cb_hdl = m_CPND_GET_CB_HDL;

	/* Get the CB from the handle */
	cpnd_cb = ncshm_take_hdl(NCS_SERVICE_ID_CPND, cb_hdl);

	if (!cpnd_cb) {
		m_LOG_CPND_CL(CPND_CB_HDL_TAKE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return;
	}

	if (saAmfResponse(cpnd_cb->amf_hdl, invocation, error) != SA_AIS_OK) {
		m_LOG_CPND_CL(CPND_AMF_RESPONSE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		ncshm_give_hdl(cb_hdl);
		return;
	}

	/* giveup the handle */
	ncshm_give_hdl(cb_hdl);
	return;
}

/****************************************************************************
 * Name          : cpnd_amf_init
 *
 * Description   : CPND initializes AMF for involking process and registers 
 *                 the various callback functions.
 *
 * Arguments     : cpnd_cb  - Ifsv control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_amf_init(CPND_CB *cpnd_cb)
{
	SaAmfCallbacksT amfCallbacks;
	SaVersionT amf_version;
	SaAisErrorT error;
	uns32 res = NCSCC_RC_SUCCESS;

	memset(&amfCallbacks, 0, sizeof(SaAmfCallbacksT));
	amfCallbacks.saAmfHealthcheckCallback = (SaAmfHealthcheckCallbackT)cpnd_saf_health_chk_callback;
	amfCallbacks.saAmfCSISetCallback = cpnd_saf_csi_set_cb;
	amfCallbacks.saAmfComponentTerminateCallback = cpnd_amf_comp_terminate_callback;
	amfCallbacks.saAmfCSIRemoveCallback = (SaAmfCSIRemoveCallbackT)cpnd_amf_csi_rmv_callback;

	m_CPSV_GET_AMF_VER(amf_version);

	error = saAmfInitialize(&cpnd_cb->amf_hdl, &amfCallbacks, &amf_version);

	if (error != SA_AIS_OK) {
		m_LOG_CPND_CL(CPND_AMF_INIT_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		res = NCSCC_RC_FAILURE;
	}
	return (res);
}

/****************************************************************************
 * Name          : cpnd_amf_de_init
 *
 * Description   : CPND uninitializes AMF for involking process.
 *
 * Arguments     : cpnd_cb  - CPND control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
void cpnd_amf_de_init(CPND_CB *cpnd_cb)
{
	if (saAmfFinalize(cpnd_cb->amf_hdl) != SA_AIS_OK)
		m_LOG_CPND_CL(CPND_AMF_DESTROY_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
}

/****************************************************************************
 * Name          : cpnd_amf_register
 *
 * Description   : CPND registers with AMF for involking process.
 *
 * Arguments     : cpnd_cb  - Ifsv control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_amf_register(CPND_CB *cpnd_cb)
{

	SaAisErrorT error;

	/* get the component name */
	error = saAmfComponentNameGet(cpnd_cb->amf_hdl, &cpnd_cb->comp_name);
	if (error != SA_AIS_OK) {
		m_LOG_CPND_CL(CPND_AMF_COMP_NAME_GET_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	if (saAmfComponentRegister(cpnd_cb->amf_hdl, &cpnd_cb->comp_name, (SaNameT *)NULL) == SA_AIS_OK)
		return NCSCC_RC_SUCCESS;
	else {
		m_LOG_CPND_CL(CPND_AMF_COMP_REG_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	m_LOG_CPND_CL(CPND_AMF_REGISTER_SUCCESS, CPND_FC_GENERIC, NCSFL_SEV_NOTICE, __FILE__, __LINE__);
}

/****************************************************************************
 * Name          : cpnd_amf_deregister
 *
 * Description   : CPND deregisters with AMF for involking process.
 *
 * Arguments     : cpnd_cb  - Ifsv control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_amf_deregister(CPND_CB *cpnd_cb)
{
	SaNameT comp_name;
	SaAisErrorT error;

	/* get the component name */
	error = saAmfComponentNameGet(cpnd_cb->amf_hdl, &comp_name);
	if (error != SA_AIS_OK) {
		m_LOG_CPND_CL(CPND_AMF_COMP_NAME_GET_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	if (saAmfComponentUnregister(cpnd_cb->amf_hdl, &comp_name, (SaNameT *)NULL) == SA_AIS_OK)
		return NCSCC_RC_SUCCESS;
	else {
		m_LOG_CPND_CL(CPND_AMF_COMP_UNREG_FAILED, CPND_FC_GENERIC, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

}

/****************************************************************************
 * Name          : cpnd_amf_comp_terminate_callback
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
void cpnd_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName)
{
	CPND_CB *cb = NULL;
	SaAisErrorT saErr = SA_AIS_OK;

	cb = ncshm_take_hdl(NCS_SERVICE_ID_CPND, gl_cpnd_cb_hdl);
	if (cb == NULL) {
		m_LOG_CPND_CL(CPND_AMF_TERM_CB_INVOKED, CPND_FC_GENERIC, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return;
	}
	saAmfResponse(cb->amf_hdl, invocation, saErr);
	ncshm_give_hdl(gl_cpnd_cb_hdl);
	m_LOG_CPND_CL(CPND_AMF_TERM_CB_INVOKED, CPND_FC_GENERIC, NCSFL_SEV_NOTICE, __FILE__, __LINE__);
	sleep(1);
	LOG_NO("Received AMF component terminate callback, exiting");
	exit(0);
}

/****************************************************************************
 * Name          : cpnd_amf_csi_rmv_callback
 *
 * Description   : TBD
 *
 *
 * Return Values : None 
 *****************************************************************************/
void
cpnd_amf_csi_rmv_callback(SaInvocationT invocation,
			  const SaNameT *compName, const SaNameT *csiName, const SaAmfCSIFlagsT *csiFlags)
{
	CPND_CB *cb = NULL;
	SaAisErrorT saErr = SA_AIS_OK;

	cb = ncshm_take_hdl(NCS_SERVICE_ID_CPND, gl_cpnd_cb_hdl);
	if (cb == NULL) {
		m_LOG_CPND_CL(CPND_CSI_RMV_CB_INVOKED, CPND_FC_GENERIC, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return;
	}
	saAmfResponse(cb->amf_hdl, invocation, saErr);
	ncshm_give_hdl(gl_cpnd_cb_hdl);

	m_LOG_CPND_CL(CPND_CSI_RMV_CB_INVOKED, CPND_FC_GENERIC, NCSFL_SEV_NOTICE, __FILE__, __LINE__);

	TRACE("THIS IS IN RMV CALLBACK");
	return;
}

/****************************************************************************\
 PROCEDURE NAME : cpnd_saf_csi_set_cb
 
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
                 csiName        - A pointer to the name of the new component
                                  service instance to be supported by the 
                                  component or of an alreadt supported 
                                  component service instance whose HA state 
                                  is to be changed.
                 csiFlags       - A value of the choiceflag type which 
                                  indicates whether the HA state change must
                                  be applied to a new component service 
                                  instance or to all component service 
                                  instance currently supported by the 
                                  component.
                 haState        - The new HA state to be assumeb by the 
                                  component service instance identified by 
                                  csiName.
                 activeCompName - A pointer to the name of the component that
                                  currently has the active state or had the
                                  active state for this component serivce 
                                  insance previously. 
                 transitionDesc - This will indicate whether or not the 
                                  component service instance for 
                                  ativeCompName went through quiescing.
 RETURNS       : None.
\*****************************************************************************/

void cpnd_saf_csi_set_cb(SaInvocationT invocation,
			 const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor)
{
	CPND_CB *cb = NULL;
	SaAisErrorT saErr = SA_AIS_OK;

	cb = ncshm_take_hdl(NCS_SERVICE_ID_CPND, gl_cpnd_cb_hdl);
	if (cb) {
		cb->ha_state = haState;	/* Set the HA State */

		saAmfResponse(cb->amf_hdl, invocation, saErr);
		ncshm_give_hdl(gl_cpnd_cb_hdl);
		m_LOG_CPND_CL(CPND_CSI_CB_INVOKED, CPND_FC_GENERIC, NCSFL_SEV_NOTICE, __FILE__, __LINE__);
	} else {
		m_LOG_CPND_CL(CPND_CB_RETRIEVAL_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
	}
	return;
}	/* End of cpnd_saf_csi_set_cb() */
