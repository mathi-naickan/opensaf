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
  FILE NAME: GLD_AMF.C

  DESCRIPTION: GLD AMF callback routines.

  FUNCTIONS INCLUDED in this module:
******************************************************************************/

#include "gld.h"
#include "gld_log.h"
#include "gld_imm.h"

/****************************************************************************
 * Name          : gld_amf_CSI_set_callback
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
void
gld_amf_CSI_set_callback(SaInvocationT invocation,
			 const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor)
{
	GLSV_GLD_CB *gld_cb;
	SaAisErrorT error = SA_AIS_OK;
	V_DEST_RL mds_role;

	gld_cb = m_GLSV_GLD_RETRIEVE_GLD_CB;
	if (gld_cb != NULL) {
		if (gld_cb->ha_state == SA_AMF_HA_ACTIVE && haState == SA_AMF_HA_QUIESCED) {
			mds_role = SA_AMF_HA_QUIESCED;

			gld_cb->invocation = invocation;
			gld_cb->is_quiasced = TRUE;

			/* Give up our IMM OI implementer role */
			error = immutil_saImmOiImplementerClear(gld_cb->immOiHandle);
			if (error != SA_AIS_OK) {
				gld_log(NCSFL_SEV_ERROR, "saImmOiImplementerClear failed: err = %d", error);
			}

			gld_mds_change_role(gld_cb, mds_role);

			return;
		}
		gld_cb->ha_state = haState;

		/* Call into MDS to set the role TBD. */
		if (gld_cb->ha_state == SA_AMF_HA_ACTIVE) {
			/* If this is the active Director, become implementer */
			gld_imm_declare_implementer(gld_cb);

			mds_role = V_DEST_RL_ACTIVE;
		} else {
			mds_role = V_DEST_RL_STANDBY;
		}
		gld_mds_change_role(gld_cb, mds_role);

		if (glsv_gld_mbcsv_chgrole(gld_cb) != NCSCC_RC_SUCCESS) {
			m_LOG_GLD_MBCSV(GLD_MBCSV_CHGROLE_FAILED, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		}

		saAmfResponse(gld_cb->amf_hdl, invocation, error);

		if (mds_role == V_DEST_RL_ACTIVE)
			gld_process_node_down_evts(gld_cb);

		m_GLSV_GLD_GIVEUP_GLD_CB;
		if (gld_cb->ha_state == SA_AMF_HA_ACTIVE)
			m_LOG_GLD_SVC_PRVDR(GLD_AMF_CSI_SET_HA_STATE_ACTIVE, NCSFL_SEV_INFO, __FILE__, __LINE__);
		else if (gld_cb->ha_state == SA_AMF_HA_STANDBY)
			m_LOG_GLD_SVC_PRVDR(GLD_AMF_CSI_SET_HA_STATE_STANDBY, NCSFL_SEV_INFO, __FILE__, __LINE__);
		else if (gld_cb->ha_state == SA_AMF_HA_QUIESCED)
			m_LOG_GLD_SVC_PRVDR(GLD_AMF_CSI_SET_HA_STATE_QUIESCED, NCSFL_SEV_INFO, __FILE__, __LINE__);
	}

	return;
}

/****************************************************************************
 * Name          : gld_amf_health_chk_callback
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
void gld_amf_health_chk_callback(SaInvocationT invocation, const SaNameT *compName, SaAmfHealthcheckKeyT *checkType)
{
	GLSV_GLD_CB *gld_cb;
	SaAisErrorT error = SA_AIS_OK;

	gld_cb = m_GLSV_GLD_RETRIEVE_GLD_CB;

	if (gld_cb != NULL) {
		saAmfResponse(gld_cb->amf_hdl, invocation, error);
		m_GLSV_GLD_GIVEUP_GLD_CB;
		m_LOG_GLD_SVC_PRVDR(GLD_AMF_RCVD_HEALTHCHK, NCSFL_SEV_INFO, __FILE__, __LINE__);
	}
	return;
}

/****************************************************************************
 * Name          : gld_amf_comp_terminate_callback
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
void gld_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName)
{
	GLSV_GLD_CB *gld_cb;
	SaAisErrorT error = SA_AIS_OK;

	gld_cb = m_GLSV_GLD_RETRIEVE_GLD_CB;

	saAmfResponse(gld_cb->amf_hdl, invocation, error);
	#if 0
	if (gld_cb != NULL) {
		saAmfResponse(gld_cb->amf_hdl, invocation, error);
		m_GLSV_GLD_GIVEUP_GLD_CB;
		m_LOG_GLD_SVC_PRVDR(GLD_AMF_RCVD_TERMINATE_CALLBACK, NCSFL_SEV_INFO, __FILE__, __LINE__);

		/* Unregister with MBCSv */
		glsv_gld_mbcsv_unregister(gld_cb);

		/* Disconnect from MDS */
		gld_mds_shut(gld_cb);
		ncshm_give_hdl(gl_gld_hdl);
		ncshm_destroy_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl);
		m_NCS_IPC_DETACH(&gld_cb->mbx, gld_clear_mbx, gld_cb);

		gld_cb_destroy(gld_cb);
		m_MMGR_FREE_GLSV_GLD_CB(gld_cb);

		m_GLSV_GLD_GIVEUP_GLD_CB;
	}
	#endif
	sleep(1);
	LOG_NO("Received AMF component terminate callback, exiting");
	exit(0);
}

/****************************************************************************
 * Name          : gld_amf_csi_rmv_callback
 *
 * Description   : TBD
 *
 *
 * Return Values : None 
 *****************************************************************************/
void
gld_amf_csi_rmv_callback(SaInvocationT invocation,
			 const SaNameT *compName, const SaNameT *csiName, const SaAmfCSIFlagsT csiFlags)
{
	GLSV_GLD_CB *gld_cb;
	SaAisErrorT error = SA_AIS_OK;

	gld_cb = (GLSV_GLD_CB *)m_GLSV_GLD_RETRIEVE_GLD_CB;

	if (gld_cb != NULL) {
		saAmfResponse(gld_cb->amf_hdl, invocation, error);
		m_GLSV_GLD_GIVEUP_GLD_CB;
		m_LOG_GLD_SVC_PRVDR(GLD_AMF_RCVD_HEALTHCHK, NCSFL_SEV_INFO, __FILE__, __LINE__);
	}
	return;
}

/****************************************************************************
 * Name          : gld_amf_init
 *
 * Description   : GLD initializes AMF for involking process and registers 
 *                 the various callback functions.
 *
 * Arguments     : gld_cb  - Ifsv control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 gld_amf_init(GLSV_GLD_CB *gld_cb)
{
	SaAmfCallbacksT amfCallbacks;
	SaVersionT amf_version;
	SaAisErrorT error;
	uns32 res = NCSCC_RC_SUCCESS;

	memset(&amfCallbacks, 0, sizeof(SaAmfCallbacksT));

	amfCallbacks.saAmfHealthcheckCallback = gld_amf_health_chk_callback;
	amfCallbacks.saAmfCSISetCallback = gld_amf_CSI_set_callback;
	amfCallbacks.saAmfComponentTerminateCallback = gld_amf_comp_terminate_callback;
	amfCallbacks.saAmfCSIRemoveCallback = gld_amf_csi_rmv_callback;

	m_GLSV_GET_AMF_VER(amf_version);

	error = saAmfInitialize(&gld_cb->amf_hdl, &amfCallbacks, &amf_version);

	if (error != SA_AIS_OK) {
		m_LOG_GLD_SVC_PRVDR(GLD_AMF_INIT_ERROR, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	/* get the component name */
	error = saAmfComponentNameGet(gld_cb->amf_hdl, &gld_cb->comp_name);
	if (error != SA_AIS_OK) {
		m_LOG_GLD_SVC_PRVDR(GLD_AMF_INIT_ERROR, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	m_LOG_GLD_SVC_PRVDR(GLD_AMF_INIT_SUCCESS, NCSFL_SEV_INFO, __FILE__, __LINE__);
	return (res);
}
