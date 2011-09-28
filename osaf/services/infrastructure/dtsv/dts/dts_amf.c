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
  FILE NAME: dts_amf.C

  DESCRIPTION: DTS AMF callback and initialization routines.

  FUNCTIONS INCLUDED in this module:
  dts_amf_init.................................DTS initialize with AMF.
  dts_saf_readiness_state_callback ........... DTS SAF readiness callback.
  dts_saf_CSI_set_callback.................... DTS SAF HA state callback.
  dts_saf_pend_oper_confirm_callback.......... DTS SAF oper confirm callback.
  dts_saf_health_chk_callback................. DTS SAF Health Check callback.  
  dts_saf_CSI_rem_callback   ................. DTS SAF CSI remove callback.  
  dts_saf_comp_terminate_callback............. DTS SAF Terminate comp callback.  

******************************************************************************/

#include <configmake.h>
#include "dts.h"

#define m_DTS_COMP_NAME_FILE PKGLOCALSTATEDIR "/dts_comp_name"
#define DTS_COMP_FILE_NAME_LEN 256

static void dts_saf_CSI_set_callback(SaInvocationT invocation,
				     const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor);

static void
dts_saf_health_chk_callback(SaInvocationT invocation, const SaNameT *compName, SaAmfHealthcheckKeyT *checkType);

static void dts_saf_CSI_rem_callback(SaInvocationT invocation,
				     const SaNameT *compName, const SaNameT *csiName, const SaAmfCSIFlagsT csiFlags);

static void dts_saf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName);

static uns32 dts_healthcheck_start(DTS_CB *cb);

/****************************************************************************
 * Name          : dts_amf_init
 *
 * Description   : DTS initializes AMF for involking process and registers 
 *                 the various callback functions.
 *
 * Arguments     : dts_cb_inst  - DTS control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : Changed the function to do intialize of callbacks & selection 
 *                 objects. Called from dts_amf_register. 
 *****************************************************************************/
uns32 dts_amf_init(DTS_CB *dts_cb_inst)
{
	SaAmfCallbacksT amfCallbacks;
	SaVersionT amf_version;
	SaAisErrorT error;

	memset(&amfCallbacks, 0, sizeof(SaAmfCallbacksT));

	amfCallbacks.saAmfHealthcheckCallback = dts_saf_health_chk_callback;
	amfCallbacks.saAmfCSISetCallback = dts_saf_CSI_set_callback;
	amfCallbacks.saAmfCSIRemoveCallback = dts_saf_CSI_rem_callback;
	amfCallbacks.saAmfComponentTerminateCallback = dts_saf_comp_terminate_callback;

	m_DTS_GET_AMF_VER(amf_version);

	error = saAmfInitialize((SaAmfHandleT *)&dts_cb_inst->amf_hdl, &amfCallbacks, &amf_version);

	if (error != SA_AIS_OK) {
		/* Log */
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_amf_init: DTS AMF Initialization fails.");
	}

	/* get the communication Handle */
	error = saAmfSelectionObjectGet(dts_cb_inst->amf_hdl, &dts_cb_inst->dts_amf_sel_obj);
	if (error != SA_AIS_OK) {
		/* log the error */
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_amf_init: DTS AMF get selection object get failed.");
	}

	return (NCSCC_RC_SUCCESS);
}

 /*****************************************************************************\
 *  Name:          dts_amf_finalize
 *
 *  Description:   To Finalize the session with the AMF 
 * 
 *  Arguments:     cb - DTS's control block 
 *
 *  Returns:       NCSCC_RC_SUCCESS   - everything is OK
 *                 NCSCC_RC_FAILURE   -  failure
 *  NOTE: 
 \*****************************************************************************/
/* Unregistration and Finalization with AMF Library */
uns32 dts_amf_finalize(DTS_CB *dts_cb_inst)
{
	SaAisErrorT status = SA_AIS_OK;

	/* delete the fd from the select list */
	memset(&dts_cb_inst->dts_amf_sel_obj, 0, sizeof(SaSelectionObjectT));

	/* Disable the health monitoring */
	status = saAmfHealthcheckStop(dts_cb_inst->amf_hdl, &dts_cb_inst->comp_name, &dts_cb_inst->health_chk_key);

	if (status != SA_AIS_OK) {
		/* log the error */
		m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_amf_finalize: Helath Check stop failed");
		/* continue finalization */
	}

	/* Unregister with AMF */
	status = saAmfComponentUnregister(dts_cb_inst->amf_hdl, &dts_cb_inst->comp_name, NULL);
	if (status != SA_AIS_OK) {
		/* log the error */
		m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_amf_finalize: Component unregistered failed!!");
		/* continue finalization */
	}

	/* Finalize */
	status = saAmfFinalize(dts_cb_inst->amf_hdl);
	if (status != SA_AIS_OK) {
		/* log the error */
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_amf_finalize: Component Finalized failed!!");
	}

	dts_cb_inst->amf_init = FALSE;
	m_LOG_DTS_API(DTS_AMF_FINALIZE);

	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
 Function: dts_amf_register

 Purpose:  Function which registers DTS with AMF.  

 Input:    None 

 Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE

 Notes:  Here we call dts_amf_init after reading the component name file and
         setting the environment varaiable in our own context.
         Proceed to register with AMF, since it has come up. 
\**************************************************************************/
uns32 dts_amf_register(DTS_CB *inst)
{
	SaAisErrorT error;
	char comp_name[256] = { 0 };
	char compfilename[DTS_COMP_FILE_NAME_LEN] = { 0 };
	FILE *fp;

	m_DTS_LK(&inst->lock);
	m_LOG_DTS_LOCK(DTS_LK_LOCKED, &inst->lock);

	/* Read the component name file now, AMF should have populated it by now */
	assert(sprintf(compfilename, "%s", m_DTS_COMP_NAME_FILE) < sizeof(compfilename));

	fp = fopen(compfilename, "r");	/* PKGLOCALSTATEDIR/dts_comp_name */
	if (fp == NULL) {
		m_DTS_UNLK(&inst->lock);
		m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_amf_register:Unable to open file");
	}

	if (fscanf(fp, "%s", comp_name) != 1) {
		fclose(fp);
		m_DTS_UNLK(&inst->lock);
		m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_amf_register:Unable to retrieve component name");
	}

	if (setenv("SA_AMF_COMPONENT_NAME", comp_name, 1) == -1) {
		fclose(fp);
		m_DTS_UNLK(&inst->lock);
		m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
				      "dts_amf_register:Unable to set SA_AMF_COMPONENT_NAME enviroment variable");
	}
	fclose(fp);
	fp = NULL;

	if (NCSCC_RC_SUCCESS != dts_amf_init(inst)) {
		m_DTS_UNLK(&inst->lock);
		m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_amf_register: AMF Initialization failed.");
	}

	/* Get component name */
	error = saAmfComponentNameGet(inst->amf_hdl, &inst->comp_name);
	if (error != SA_AIS_OK) {
		m_DTS_UNLK(&inst->lock);
		m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_amf_register: Component name get failed.");

	}

	/* register DTS component with AvSv */
	error = saAmfComponentRegister(inst->amf_hdl, &inst->comp_name, (SaNameT *)NULL);
	if (error != SA_AIS_OK) {
		/* log the error */
		m_DTS_UNLK(&inst->lock);
		m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_amf_register: DTS AMF Component registration failed.");
	}

	if (NCSCC_RC_SUCCESS != dts_healthcheck_start(inst)) {
		m_DTS_UNLK(&inst->lock);
		m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_amf_register: Health check start fail.");
	}

	m_LOG_DTS_API(DTS_AMF_INIT_SUCCESS);

	inst->amf_init = TRUE;

	m_DTS_UNLK(&inst->lock);
	m_LOG_DTS_LOCK(DTS_LK_UNLOCKED, &inst->lock);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : dts_saf_CSI_set_callback
 *
 * Description   : This function SAF callback function which will be called 
 *                 when there is any change in the HA state.
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
 *                 csiName        - A pointer to the name of the new component
 *                                  service instance to be supported by the 
 *                                  component or of an alreadt supported 
 *                                  component service instance whose HA state 
 *                                  is to be changed.
 *                 csiFlags       - A value of the choiceflag type which 
 *                                  indicates whether the HA state change must
 *                                  be applied to a new component service 
 *                                  instance or to all component service 
 *                                  instance currently supported by the 
 *                                  component.
 *                 haState        - The new HA state to be assumeb by the 
 *                                  component service instance identified by 
 *                                  csiName.
 *                 activeCompName - A pointer to the name of the component that
 *                                  currently has the active state or had the
 *                                  active state for this component serivce 
 *                                  insance previously. 
 *                 transitionDesc - This will indicate whether or not the 
 *                                  component service instance for 
 *                                  ativeCompName went through quiescing.
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
static void dts_saf_CSI_set_callback(SaInvocationT invocation,
				     const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor)
{
	DTS_CB *dts_cb_inst = &dts_cb;
	SaAisErrorT error = SA_AIS_OK;
	SaAmfHAStateT prev_haState = dts_cb_inst->ha_state;

	if (((SA_AMF_CSI_ADD_ONE == csiDescriptor.csiFlags) &&
	     (dts_cb_inst->csi_set == TRUE)) ||
	    (((SA_AMF_CSI_ADD_ONE != csiDescriptor.csiFlags) && (dts_cb_inst->csi_set == FALSE)))) {
		m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_saf_CSI_set_callback: ERROR in operation!!");
		return;
	} else {
		if (FALSE == dts_cb_inst->csi_set) {
			dts_cb_inst->csi_set = TRUE;
		}

		/* Store CSI set cb invocation hdl to dts_cb for use of giving 
		 * SaAmfResponse for Act to Quiesced transition */
		dts_cb_inst->csi_cb_invocation = invocation;

		/* Invoke dts_role_change to do valid role change action */
		if (dts_role_change(dts_cb_inst, haState) != NCSCC_RC_SUCCESS) {
			m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_saf_CSI_set_callback: Role change failed");
			saAmfResponse(dts_cb_inst->amf_hdl, invocation, SA_AIS_ERR_FAILED_OPERATION);
			fflush(stdout);
			return;
		}

		/* Change ha_state of dts_cb now. Change now done in dts_role_change */
		/*dts_cb_inst->ha_state = haState; */

		/* Smik - Need to verify if saAmfResponse shud be sent before or
		 *        after msg is dispatched to DTS mbx.
		 * Also check whether the transition is frm Act to Quiesced. For this
		 * case don't send SaAmfResponse now. 
		 */
		if ((dts_cb_inst->ha_state == SA_AMF_HA_ACTIVE) && (haState == SA_AMF_HA_QUIESCED)) {
			/* No SaAmfResponse. Do nothing. Usually it would be the same. */
		} else {
			saAmfResponse(dts_cb_inst->amf_hdl, invocation, error);
			m_LOG_DTS_API(DTS_CSI_SET_CB_RESP);
			fflush(stdout);
		}
		if (((prev_haState == SA_AMF_HA_STANDBY) ||
		     (prev_haState == SA_AMF_HA_QUIESCED)) && (haState == SA_AMF_HA_ACTIVE)) {

			if (dts_stby_update_dta_config() != NCSCC_RC_SUCCESS) {
				if (saAmfComponentErrorReport
				    (dts_cb_inst->amf_hdl, &dts_cb_inst->comp_name, 0, SA_AMF_COMPONENT_RESTART,
				     0) != SA_AIS_OK) {
					m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
						       "dts_saf_CSI_set_callback: Failed to send Error report to AMF");
				}

				m_DTS_DBG_SINK(NCSCC_RC_FAILURE,
					       "dts_saf_CSI_set_callback: Failed to update dta config");
				return;
			}

		}
	}

	fflush(stdout);
	return;
}

/****************************************************************************
 * Name          : dts_saf_health_chk_callback
 *
 * Description   : This function SAF callback function which will be called 
 *                 when the AMF framework needs to health for the component.
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
 *                 checkType      - The type of healthcheck to be executed. 
 *
 * Return Values : None
 *
 * Notes         : At present we are just support a simple liveness check.
 *****************************************************************************/
static void
dts_saf_health_chk_callback(SaInvocationT invocation, const SaNameT *compName, SaAmfHealthcheckKeyT *checkType)
{
	DTS_CB *dts_cb_inst = &dts_cb;
	SaAisErrorT error = SA_AIS_OK;

	if (dts_cb_inst->created == FALSE) {
		m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_saf_health_chk_callback: DTS does not exist");
		return;
	} else {
		saAmfResponse(dts_cb_inst->amf_hdl, invocation, error);
	}

	ncs_logmsg(NCS_SERVICE_ID_DTSV, DTS_LID_API, DTS_FC_API,
		   NCSFL_LC_API, NCSFL_SEV_INFO, "TI", DTS_AMF_HEALTH_CHECK);
	return;
}

/****************************************************************************
 * Name          : dts_saf_CSI_rem_callback
 *
 * Description   : This function SAF callback function which will be called 
 *                 when the AMF framework needs remove the CSI assignment.
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
 *                 csiName        - A pointer to the name of the new component
 *                                  service instance to be supported by the 
 *                                  component or of an alreadt supported 
 *                                  component service instance whose HA state 
 *                                  is to be changed.
 *                 csiFlags       - A value of the choiceflag type which 
 *                                  indicates whether the HA state change must
 *                                  be applied to a new component service 
 *                                  instance or to all component service 
 *                                  instance currently supported by the 
 *                                  component.
 *
 * Return Values : None
 *
 * Notes         : This is not completed - TBD.
 *****************************************************************************/
static void dts_saf_CSI_rem_callback(SaInvocationT invocation,
				     const SaNameT *compName, const SaNameT *csiName, const SaAmfCSIFlagsT csiFlags)
{
	DTS_CB *dts_cb_inst = &dts_cb;
	SaAisErrorT error = SA_AIS_OK;

	if (dts_cb_inst->created == FALSE) {
		m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_saf_CSI_rem_callback : DTS does not exist");
		return;
	} else {
		dts_cb_inst->csi_set = FALSE;
		saAmfResponse(dts_cb_inst->amf_hdl, invocation, error);

		m_LOG_DTS_API(DTS_CSI_RMV_CB_RESP);
	}

	return;
}

/****************************************************************************
 * Name          : dts_saf_comp_terminate_callback
 *
 * Description   : This function SAF callback function which will be called 
 *                 when the AMF needs to terminate the component.
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
 * Notes         : This is not completed - TBD.
 *****************************************************************************/
static void dts_saf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName)
{
	DTS_CB *dts_cb_inst = &dts_cb;
	SaAisErrorT error = SA_AIS_OK;

	m_LOG_DTS_API(DTS_CSI_TERM_CB);
	/*Send response before amf_handle is destroyed */
	saAmfResponse(dts_cb_inst->amf_hdl, invocation, error);
	/* Now destroy dts lib */
	dts_lib_destroy();
	LOG_NO("Received AMF component terminate callback, exiting");
	exit(0);
}

/*****************************************************************************\
 *  Name:          dts_healthcheck_start                           * 
 *                                                                            *
 *  Description:   To start the health check                                  *
 *                                                                            *
 *  Arguments:     NCSSA_CB* - Control Block                                  * 
 *                                                                            * 
 *  Returns:       NCSCC_RC_SUCCESS   - everything is OK                      *
 *                 NCSCC_RC_FAILURE   -  failure                              *
 *  NOTE:                                                                     * 
\******************************************************************************/
static uns32 dts_healthcheck_start(DTS_CB *cb)
{
	SaAisErrorT saf_status = SA_AIS_OK;

	if (cb->healthCheckStarted == TRUE) {
		return NCSCC_RC_SUCCESS;
	}

	/* start the healthcheck */
	saf_status = saAmfHealthcheckStart(cb->amf_hdl,
					   &cb->comp_name,
					   &cb->health_chk_key, cb->invocationType, cb->recommendedRecovery);
	if (saf_status != SA_AIS_OK) {
		/* log the error */
		return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_healthcheck_start: Failed to start health check.");
	}

	cb->healthCheckStarted = TRUE;

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************\
 *  Name:          dts_amf_sigusr1_handler                                    * 
 *                                                                            *
 *  Description:   to initiate DTS to register with AMF                       *
 *                                                                            *
 *  Arguments:     int i_sug_num -  Signal received                           *
 *                                                                            * 
 *  Returns:       Nothing                                                    *  
 *  NOTE:                                                                     * 
\*****************************************************************************/
/* signal from AMF, time to register with AMF */
void dts_amf_sigusr1_handler(int i_sig_num)
{
	DTS_CB *inst = &dts_cb;
	/* ignore the signal */
	signal(SIGUSR1, SIG_IGN);

	/* Just raise an indication on the selection object here */
	m_NCS_SEL_OBJ_IND(inst->sighdlr_sel_obj);

	/*if (NCSCC_RC_SUCCESS != dts_amf_init(inst))
	   {
	   return m_DTS_DBG_SINK(NCSCC_RC_FAILURE, "dts_amf_sigusr1_handler: AMF Initialization failed.");
	   }

	   inst->amf_init = TRUE; */

}
