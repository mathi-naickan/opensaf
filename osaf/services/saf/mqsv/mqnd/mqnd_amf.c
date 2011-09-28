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
  FILE NAME: MQND_AMF.C

  DESCRIPTION: MQND AMF callback routines.

  FUNCTIONS INCLUDED in this module:
******************************************************************************/

#include "mqnd.h"

/* 
  DESCRIPTION: MQND AMF callback routines.

  FUNCTIONS INCLUDED in this module:
  mqnd_saf_readiness_state_callback ........... MQND SAF readiness callback.
  mqnd_saf_health_chk_callback................. MQND SAF Health Check callback.

******************************************************************************/

#include "mqnd.h"

static void mqnd_saf_health_chk_callback(SaInvocationT invocation, const SaNameT *compName,
					 SaAmfHealthcheckKeyT *checkType);
static void mqnd_amf_csi_rmv_callback(SaInvocationT invocation,
				      const SaNameT *compName, const SaNameT *csiName, SaAmfCSIFlagsT csiFlags);

static void mqnd_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName);

static void mqnd_amf_CSI_set_callback(SaInvocationT invocation,
				      const SaNameT *compName,
				      SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor);

/****************************************************************************
 * Name          : mqnd_saf_health_chk_callback
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
static void
mqnd_saf_health_chk_callback(SaInvocationT invocation, const SaNameT *compName, SaAmfHealthcheckKeyT *checkType)
{
	MQND_CB *mqnd_cb;
	SaAisErrorT error = SA_AIS_OK;
	uns32 cb_hdl = m_MQND_GET_HDL();
	uns32 rc = NCSCC_RC_SUCCESS;

	/* Get the CB from the handle */
	mqnd_cb = ncshm_take_hdl(NCS_SERVICE_ID_MQND, cb_hdl);

	if (!mqnd_cb) {
		rc = NCSCC_RC_FAILURE;
		m_LOG_MQSV_ND(MQND_CB_HDL_TAKE_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
		return;
	}
	if ((rc = saAmfResponse(mqnd_cb->amf_hdl, invocation, error)) != SA_AIS_OK)
		m_LOG_MQSV_ND(MQND_AMF_RESPONSE_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
	/* giveup the handle */
	ncshm_give_hdl(cb_hdl);
	return;
}

/****************************************************************************
 * Name          : mqnd_amf_init
 *
 * Description   : GLD initializes AMF for involking process and registers 
 *                 the various callback functions.
 *
 * Arguments     : mqnd_cb  - Ifsv control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 mqnd_amf_init(MQND_CB *mqnd_cb)
{
	SaAmfCallbacksT amfCallbacks;
	SaVersionT amf_version;
	SaAisErrorT error;
	uns32 res = NCSCC_RC_SUCCESS;

	memset(&amfCallbacks, 0, sizeof(SaAmfCallbacksT));

	amfCallbacks.saAmfHealthcheckCallback = mqnd_saf_health_chk_callback;
	amfCallbacks.saAmfCSISetCallback = mqnd_amf_CSI_set_callback;
	amfCallbacks.saAmfComponentTerminateCallback = mqnd_amf_comp_terminate_callback;
	amfCallbacks.saAmfCSIRemoveCallback = mqnd_amf_csi_rmv_callback;

	m_MQSV_GET_AMF_VER(amf_version);

	error = saAmfInitialize(&mqnd_cb->amf_hdl, &amfCallbacks, &amf_version);

	if (error != SA_AIS_OK) {
		res = NCSCC_RC_FAILURE;
		m_LOG_MQSV_ND(MQND_AMF_INIT_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, error, __FILE__, __LINE__);
	} else
		m_LOG_MQSV_ND(MQND_AMF_INITIALISATION_SUCCESS, NCSFL_LC_MQSV_INIT, NCSFL_SEV_NOTICE, error, __FILE__,
			      __LINE__);
	error = saAmfComponentNameGet(mqnd_cb->amf_hdl, &mqnd_cb->comp_name);
	if (error != SA_AIS_OK) {
		res = NCSCC_RC_FAILURE;
		m_LOG_MQSV_ND(MQND_AMF_COMP_NAME_GET_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, error, __FILE__,
			      __LINE__);
	}

	return (res);
}

/****************************************************************************
 * Name          : mqnd_amf_de_init
 *
 * Description   : MQND uninitializes AMF for involking process.
 *
 * Arguments     : mqnd_cb  - MQND control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
void mqnd_amf_de_init(MQND_CB *mqnd_cb)
{
	uns32 rc = SA_AIS_OK;
	if ((rc = saAmfFinalize(mqnd_cb->amf_hdl)) != SA_AIS_OK)
		m_LOG_MQSV_ND(MQND_AMF_DESTROY_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
}

/****************************************************************************
 * Name          : mqnd_amf_register
 *
 * Description   : MQND registers with AMF for involking process.
 *
 * Arguments     : mqnd_cb  - Ifsv control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 mqnd_amf_register(MQND_CB *mqnd_cb)
{
	SaAisErrorT error;

	/* Get the component name */
	error = saAmfComponentNameGet(mqnd_cb->amf_hdl, &mqnd_cb->comp_name);
	if (error != SA_AIS_OK) {
		m_LOG_MQSV_ND(MQND_AMF_COMP_NAME_GET_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, error, __FILE__,
			      __LINE__);
		return NCSCC_RC_FAILURE;
	}
	m_LOG_MQSV_ND(MQND_AMF_COMP_NAME_GET_SUCCESS, NCSFL_LC_MQSV_INIT, NCSFL_SEV_NOTICE, error, __FILE__, __LINE__);

	if ((error = saAmfComponentRegister(mqnd_cb->amf_hdl, &mqnd_cb->comp_name, (SaNameT *)NULL)) == SA_AIS_OK) {
		m_LOG_MQSV_ND(MQND_AMF_REGISTRATION_SUCCESS, NCSFL_LC_MQSV_INIT, NCSFL_SEV_NOTICE, error, __FILE__,
			      __LINE__);
		return NCSCC_RC_SUCCESS;
	} else {
		m_LOG_MQSV_ND(MQND_AMF_REGISTER_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, error, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
}

/****************************************************************************
 * Name          : mqnd_amf_deregister
 *
 * Description   : MQND deregisters with AMF for involking process.
 *
 * Arguments     : mqnd_cb  - Ifsv control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 mqnd_amf_deregister(MQND_CB *mqnd_cb)
{
	SaAisErrorT error;
	if ((error = saAmfComponentUnregister(mqnd_cb->amf_hdl, &mqnd_cb->comp_name, (SaNameT *)NULL)) == SA_AIS_OK) {
		m_LOG_MQSV_ND(MQND_AMF_COMP_UNREGISTER_SUCCESS, NCSFL_LC_MQSV_INIT, NCSFL_SEV_NOTICE, error, __FILE__,
			      __LINE__);
		return NCSCC_RC_SUCCESS;
	} else {
		m_LOG_MQSV_ND(MQND_AMF_COMP_UNREGISTER_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, error, __FILE__,
			      __LINE__);
		return NCSCC_RC_FAILURE;
	}
}

static void mqnd_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName)
{
	MQND_CB *mqnd_cb = 0;
	SaAisErrorT saErr = SA_AIS_OK;

	uns32 cb_hdl = m_MQND_GET_HDL();

	/* Get the CB from the handle */
	mqnd_cb = ncshm_take_hdl(NCS_SERVICE_ID_MQND, cb_hdl);

	if (!mqnd_cb) {
		m_LOG_MQSV_ND(MQND_CB_HDL_TAKE_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, NCSCC_RC_FAILURE, __FILE__,
			      __LINE__);
		return;
	}

	saAmfResponse(mqnd_cb->amf_hdl, invocation, saErr);
	m_LOG_MQSV_ND(MQND_AMF_TERM_CLBK_CALLED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_NOTICE, saErr, __FILE__, __LINE__);

	/* giveup the handle */
	ncshm_give_hdl(cb_hdl);
	sleep(1);
	LOG_NO("Received AMF component terminate callback, exiting");
	exit(0);

}

static void mqnd_amf_CSI_set_callback(SaInvocationT invocation,
				      const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor)
{
	MQND_CB *mqnd_cb;
	SaAisErrorT saErr = SA_AIS_OK;

	uns32 cb_hdl = m_MQND_GET_HDL();

	/* Get the CB from the handle */
	mqnd_cb = ncshm_take_hdl(NCS_SERVICE_ID_MQND, cb_hdl);

	if (!mqnd_cb) {
		m_LOG_MQSV_ND(MQND_CB_HDL_TAKE_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, NCSCC_RC_FAILURE, __FILE__,
			      __LINE__);
		return;
	}

	if (mqnd_cb) {
		mqnd_cb->ha_state = haState;	/* Set the HA State */
	}
	m_LOG_MQSV_ND(MQND_AMF_CSI_SET_CLBK_CALLED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_NOTICE, haState, __FILE__, __LINE__);

	/* TBD CSI Set processing at MQND */
	if (haState == SA_AMF_HA_ACTIVE) {
		/* In integration CPSV code is not getting called 60265 */
		if (!mqnd_cb->is_restart_done) {
			mqnd_cb->mqa_timer.type = MQND_TMR_TYPE_MQA_EXPIRY;
			mqnd_cb->mqa_timer.uarg = mqnd_cb->cb_hdl;
			mqnd_cb->mqa_timer.is_active = FALSE;
			mqnd_cb->mqa_timer.tmr_id = 0;
			/*Starting the timer When this timer expired CPSV initialization is done */
			mqnd_tmr_start(&mqnd_cb->mqa_timer, (unsigned)MQND_MQA_EXPIRY_TIMER);
			m_LOG_MQSV_ND(MQND_MQA_TMR_STARTED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_NOTICE, saErr, __FILE__,
				      __LINE__);
		}
	}

	saAmfResponse(mqnd_cb->amf_hdl, invocation, saErr);

	/* giveup the handle */
	ncshm_give_hdl(cb_hdl);
	return;
}

static void
mqnd_amf_csi_rmv_callback(SaInvocationT invocation,
			  const SaNameT *compName, const SaNameT *csiName, SaAmfCSIFlagsT csiFlags)
{
	MQND_CB *mqnd_cb = 0;
	SaAisErrorT saErr = SA_AIS_OK;

	uns32 cb_hdl = m_MQND_GET_HDL();

	/* Get the CB from the handle */
	mqnd_cb = ncshm_take_hdl(NCS_SERVICE_ID_MQND, cb_hdl);

	if (!mqnd_cb) {
		m_LOG_MQSV_ND(MQND_CB_HDL_TAKE_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, NCSCC_RC_FAILURE, __FILE__,
			      __LINE__);
		return;
	}
	m_LOG_MQSV_ND(MQND_AMF_CSI_RMV_CLBK_CALLED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_NOTICE, saErr, __FILE__, __LINE__);

	saAmfResponse(mqnd_cb->amf_hdl, invocation, saErr);

	/* giveup the handle */
	ncshm_give_hdl(cb_hdl);
	return;
}
