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
  MODULE NAME: FM 

  DESCRIPTION: fm callback routines.

  FUNCTIONS INCLUDED in this module:
  fm_amf_take_hdl.............................. Get FM handle
  fm_amf_give_hdl.............................. Release FM handle
  fm_saf_CSI_set_callback.................... fm SAF HA state callback.
  fm_saf_health_chk_callback................. fm SAF Health Check callback.  
  fm_saf_CSI_rem_callback   ................. fm SAF CSI remove callback.  
  fm_saf_comp_terminate_callback............. fm SAF Terminate comp callback.  
  fm_amf_init................................ fm AMF initialization method.
  fm_amf_register............................ fm AMF registration method.
  fm_amf_healthcheck_start................... fm AMF health check start method

******************************************************************************/

#include "fm.h"
uint32_t gl_fm_hdl;

uint32_t fm_amf_init(FM_AMF_CB *fm_amf_cb);
static uint32_t fm_amf_register(FM_AMF_CB *fm_amf_cb);
static uint32_t fm_amf_healthcheck_start(FM_AMF_CB *fm_amf_cb);
static FM_AMF_CB *fm_amf_take_hdl(void);
static void fm_amf_give_hdl(void);
static char *ha_role_string[] = { "ACTIVE", "STANDBY", "QUIESCED",
	"QUIESCING"
};

/****************************************************************************
 * Name          : fm_amf_take_hdl
 *
 * Description   : Get the poiter to FM AMF control block.
 *
 * Arguments     : None.
 *
 * Return Values : FM_AMF_CB control block pointer.
 *
 * Notes         : None.
 *****************************************************************************/
FM_AMF_CB *fm_amf_take_hdl(void)
{
	FM_CB *fm_cb = NULL;

	/* Take handle */
	fm_cb = ncshm_take_hdl(NCS_SERVICE_ID_GFM, gl_fm_hdl);

	return &fm_cb->fm_amf_cb;
}

/****************************************************************************
 * Name          : fm_amf_give_hdl
 *
 * Description   : Get the pointer to FM AMF control block.
 *
 * Arguments     : None.
 *
 * Return Values : FM_AMF_CB control block pointer.
 *
 * Notes         : None.
 *****************************************************************************/
void fm_amf_give_hdl(void)
{
	/* Give CB hdl */
	ncshm_give_hdl(gl_fm_hdl);

	return;
}

/****************************************************************************
 * Name          : fm_saf_CSI_set_callback
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
 *                                  whose readiness stae the Availability 
 *                                  Management Framework is setting.
 *                 haState        - The new HA state to be assumeb by the 
 *                                  component service instance identified by 
 *                                  csiName. 
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void fm_saf_CSI_set_callback(SaInvocationT invocation,
			     const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor)
{
	FM_AMF_CB *fm_amf_cb;
	SaAisErrorT error = SA_AIS_OK;
	TRACE_ENTER();    
	syslog(LOG_INFO, "fm_saf_CSI_set_callback: Comp %s, state %s", compName->value, ha_role_string[haState - 1]);
	fm_amf_cb = fm_amf_take_hdl();
	if (fm_amf_cb != NULL) {
		fm_cb->amf_state = haState;
		error = saAmfResponse(fm_amf_cb->amf_hdl, invocation, error);
		fm_cb->csi_assigned = true;
	}
	fm_amf_give_hdl();
	TRACE_LEAVE();
	return;
}

/****************************************************************************
 * Name          : fm_saf_health_chk_callback
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
 * Notes         : None
 *****************************************************************************/
void fm_saf_health_chk_callback(SaInvocationT invocation, const SaNameT *compName, SaAmfHealthcheckKeyT *checkType)
{
	FM_AMF_CB *fm_amf_cb;
	SaAisErrorT error = SA_AIS_OK;
	TRACE_ENTER();
	fm_amf_cb = fm_amf_take_hdl();
	if (fm_amf_cb != NULL) {
		error = saAmfResponse(fm_amf_cb->amf_hdl, invocation, error);
	}
	fm_amf_give_hdl();
	TRACE_LEAVE();
	return;
}

/****************************************************************************
 * Name          : fm_saf_CSI_rem_callback
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
void fm_saf_CSI_rem_callback(SaInvocationT invocation,
			     const SaNameT *compName, const SaNameT *csiName, const SaAmfCSIFlagsT csiFlags)
{
	FM_AMF_CB *fm_amf_cb;
	SaAisErrorT error = SA_AIS_OK;
	TRACE_ENTER();

	syslog(LOG_INFO, "fm_saf_CSI_rem_callback: Comp %s", compName->value);

	fm_amf_cb = fm_amf_take_hdl();
	if (fm_amf_cb != NULL) {
		error = saAmfResponse(fm_amf_cb->amf_hdl, invocation, error);
	}
	fm_amf_give_hdl();

	TRACE_LEAVE();
	return;
}

/****************************************************************************
 * Name          : fm_saf_comp_terminate_callback
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
void fm_saf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName)
{
	FM_AMF_CB *fm_amf_cb;
	SaAisErrorT error = SA_AIS_OK;
	TRACE_ENTER();
	syslog(LOG_INFO, "fm_saf_comp_terminate_callback: Comp %s", compName->value);
	fm_amf_cb = fm_amf_take_hdl();
	if (fm_amf_cb != NULL) {
		error = saAmfResponse(fm_amf_cb->amf_hdl, invocation, error);
	}
	fm_amf_give_hdl();
	LOG_NO("Received AMF component terminate callback, exiting");
	TRACE_LEAVE();
	exit(0);
}

/****************************************************************************
 * Name          : fm_amf_init
 *
 * Description   : FM initializes AMF for involking process and registers 
 *                 the various callback functions.
 *
 * Arguments     : fm_amf_cb  - FM control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t fm_amf_init(FM_AMF_CB *fm_amf_cb)
{
	SaAmfCallbacksT amfCallbacks;
	SaVersionT amf_version;
	SaAisErrorT amf_error;
	SaNameT sname;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	memset(&amfCallbacks, 0, sizeof(SaAmfCallbacksT));
	if (amf_comp_name_get_set_from_file("FM_COMP_NAME_FILE", &sname) != NCSCC_RC_SUCCESS)
		return NCSCC_RC_FAILURE;
 
	amfCallbacks.saAmfHealthcheckCallback = fm_saf_health_chk_callback;
	amfCallbacks.saAmfCSISetCallback = fm_saf_CSI_set_callback;
	amfCallbacks.saAmfCSIRemoveCallback = fm_saf_CSI_rem_callback;
	amfCallbacks.saAmfComponentTerminateCallback = fm_saf_comp_terminate_callback;

	m_FM_GET_AMF_VER(amf_version);

	amf_error = saAmfInitialize(&fm_amf_cb->amf_hdl, &amfCallbacks, &amf_version);
	if (amf_error != SA_AIS_OK) {
		syslog(LOG_ERR, "FM_COND_AMF_INIT_FAILED: amf_rc %d", amf_error);
		return NCSCC_RC_FAILURE;
	}
	memset(&sname, 0, sizeof(sname));
	amf_error = saAmfComponentNameGet(fm_amf_cb->amf_hdl, &sname);
	if (amf_error != SA_AIS_OK) {
		syslog(LOG_ERR, "FM_COND_AMF_GET_NAME_FAILED: amf_rc %d", amf_error);
		return NCSCC_RC_FAILURE;
	}
	strcpy(fm_amf_cb->comp_name, (char *)sname.value);

	/* 
	 ** Get the AMF selection object 
	 */
	amf_error = saAmfSelectionObjectGet(fm_amf_cb->amf_hdl, &fm_amf_cb->amf_fd);
	if (amf_error != SA_AIS_OK) {
		syslog(LOG_ERR, "FM_COND_AMF_GET_OBJ_FAILED: amf_rc %d", amf_error);
		return NCSCC_RC_FAILURE;
	}

	/* 
	 ** Register FM with AMF 
	 */
	rc = fm_amf_register(fm_amf_cb);
	if (rc != NCSCC_RC_SUCCESS)
		rc = NCSCC_RC_FAILURE;

	/* 
	 ** Start component healthcheck 
	 */
	rc = fm_amf_healthcheck_start(fm_amf_cb);
	if (rc != NCSCC_RC_SUCCESS) {
		 return NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : fm_amf_register
 *
 * Description   : FM registers with AMF to tell AMF that it is ready 
 *
 * Arguments     : fm_amf_cb  - FM control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t fm_amf_register(FM_AMF_CB *fm_amf_cb)
{
	SaAisErrorT amf_error;
	uint32_t res = NCSCC_RC_SUCCESS;
	SaNameT sname;
	
	TRACE_ENTER();
	sname.length = strlen(fm_amf_cb->comp_name);
	strcpy((char *)sname.value, fm_amf_cb->comp_name);

	/* 
	 * register FM component with AvSv 
	 */
	amf_error = saAmfComponentRegister(fm_amf_cb->amf_hdl, &sname, (SaNameT *)NULL);
	if (amf_error != SA_AIS_OK) {
		syslog(LOG_ERR, "FM_COND_AMF_REG_FAILED: amf_rc %d", amf_error);
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return (res);
}
/****************************************************************************
 * Name          : fm_amf_healthcheck_start
 *
 * Description   : FM informs AMF to stat healthcheck 
 *
 * Arguments     : fm_amf_cb  - FM control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t fm_amf_healthcheck_start(FM_AMF_CB *fm_amf_cb)
{
	SaAisErrorT amf_error;
	SaAmfHealthcheckKeyT Healthy;
	SaNameT SaCompName;
	char *phlth_ptr;
	char hlth_str[256];

	/*
	 ** Start the AMF health check 
	 */
	memset(&SaCompName, 0, sizeof(SaCompName));
	strcpy((char *)SaCompName.value, fm_amf_cb->comp_name);
	SaCompName.length = strlen(fm_amf_cb->comp_name);

	memset(&Healthy, 0, sizeof(Healthy));
	phlth_ptr = getenv("FMS_HA_ENV_HEALTHCHECK_KEY");
	if (phlth_ptr == NULL) {
		/*
		 ** default health check key 
		 */
		strcpy(hlth_str, "BAD10");
	} else {
		strcpy(hlth_str, phlth_ptr);
	}
	strcpy((char *)Healthy.key, hlth_str);
	Healthy.keyLen = strlen((char *)Healthy.key);

	amf_error = saAmfHealthcheckStart(fm_amf_cb->amf_hdl, &SaCompName, &Healthy,
					  SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_RESTART);
	if (amf_error != SA_AIS_OK) {
		syslog(LOG_ERR, "FM_COND_AMF_HEALTH_CHK_START_FAIL:"
		       " Healthcheck key: %s amf_rc %d", Healthy.key, amf_error);
		return NCSCC_RC_FAILURE;
	}

	return (NCSCC_RC_SUCCESS);
}

