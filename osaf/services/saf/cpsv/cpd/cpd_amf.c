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
..............................................................................

..............................................................................

  DESCRIPTION: This file includes following routines:
   
   cpd_saf_hlth_chk_cb................CPD health check callback 
   cpd_saf_readiness_state_cb.........CPD rediness state callback
   cpd_saf_csi_set_cb.................CPD component state callback
   cpd_saf_pend_oper_confirm_cb.......CPD pending operation callback
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#include "cpd.h"
#include "immutil.h"
#include "cpd_imm.h"
#define NCS_2_0 1
#if NCS_2_0			/* Required for NCS 2.0 */
extern uint32_t gl_cpd_cb_hdl;
extern const SaImmOiImplementerNameT implementer_name;


/****************************************************************************
 PROCEDURE NAME : cpd_saf_hlth_chk_cb

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
  NOTES         : At present we are just support a simple liveness check.
*****************************************************************************/
void cpd_saf_hlth_chk_cb(SaInvocationT invocation, const SaNameT *compName, SaAmfHealthcheckKeyT *checkType)
{
	CPD_CB *cb = 0;
	SaAisErrorT saErr = SA_AIS_OK;
	
	/* Get the COntrol Block Pointer */
	cb = ncshm_take_hdl(NCS_SERVICE_ID_CPD, gl_cpd_cb_hdl);
	if (cb) {
		saAmfResponse(cb->amf_hdl, invocation, saErr);
		ncshm_give_hdl(cb->cpd_hdl);
	} else {
		LOG_ER("Failed to retrieve cpd handle %u",gl_cpd_cb_hdl);
	}
	return;
}	/* End of cpd_saf_hlth_chk_cb() */

/****************************************************************************\
 PROCEDURE NAME : cpd_saf_csi_set_cb
 
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

void cpd_saf_csi_set_cb(SaInvocationT invocation,
			const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor)
{
	CPD_CB *cb = 0;
	SaAisErrorT saErr = SA_AIS_OK;
	V_DEST_RL mds_role;
	/*  V_CARD_QA   anchor; */
	NCSVDA_INFO vda_info;
	uint32_t rc = NCSCC_RC_SUCCESS;
	CPD_CPND_INFO_NODE *node_info = NULL;
	MDS_DEST prev_dest;

	TRACE_ENTER();
	cb = ncshm_take_hdl(NCS_SERVICE_ID_CPD, gl_cpd_cb_hdl);
	if (cb) {

		if ((cb->ha_state == SA_AMF_HA_STANDBY) && (haState == SA_AMF_HA_ACTIVE)) {
			if (cb->cold_or_warm_sync_on == true) {
				TRACE("STANDBY cpd_saf_csi_set_cb -cb->cold_or_warm_sync_on == true ");
				saErr = SA_AIS_ERR_TRY_AGAIN;
				saAmfResponse(cb->amf_hdl, invocation, saErr);
				ncshm_give_hdl(cb->cpd_hdl);
				TRACE_4("cpd vdest change role failed");
				m_CPSV_DBG_SINK(NCSCC_RC_FAILURE,
						"cpd_role_change: Failed to send Error report to AMF for Active role assignment during Cold-Sync");
				return;
			}
		}

		if ((cb->ha_state == SA_AMF_HA_ACTIVE) && (haState == SA_AMF_HA_QUIESCED)) {
			if (cb->cold_or_warm_sync_on == true) {
				TRACE("ACTIVE cpd_saf_csi_set_cb -cb->cold_or_warm_sync_on == true ");
			}
		}

		/* cb->ha_state = haState; */

		/*     saAmfResponse(cb->amf_hdl, invocation, saErr); */

   		if (SA_AMF_HA_ACTIVE == haState) {
		        /** Change the MDS role **/
			cb->ha_state = haState;
			mds_role = V_DEST_RL_ACTIVE;
			TRACE("ACTIVE STATE");

			/* If this is the active server, become implementer again. */
			/* If this is the active Director, become implementer */
			saErr = immutil_saImmOiImplementerSet(cb->immOiHandle, implementer_name);
			if (saErr != SA_AIS_OK){
				LOG_ER("cpd immOiImplmenterSet failed with err = %u",saErr);
				exit(EXIT_FAILURE);
			}
			/*   anchor   = cb->cpd_anc; */
		} else if (SA_AMF_HA_QUIESCED == haState) {
			mds_role = V_DEST_RL_QUIESCED;
			cb->amf_invocation = invocation;
			cb->is_quiesced_set = true;
			memset(&vda_info, 0, sizeof(vda_info));

			vda_info.req = NCSVDA_VDEST_CHG_ROLE;
			vda_info.info.vdest_chg_role.i_vdest = cb->cpd_dest_id;
			/* vda_info.info.vdest_chg_role.i_anc = anchor; */
			vda_info.info.vdest_chg_role.i_new_role = mds_role;
			rc = ncsvda_api(&vda_info);
			if (NCSCC_RC_SUCCESS != rc) {
				m_LEAP_DBG_SINK_VOID;
				LOG_ER("cpd vdest change role failed");
				ncshm_give_hdl(cb->cpd_hdl);
				TRACE_LEAVE();
				return;
			}
			ncshm_give_hdl(cb->cpd_hdl);
			TRACE_2("cpd csi set cb success");
			TRACE_LEAVE();
			return;
		} else {
      		        /** Change the MDS role **/
		        cb->ha_state = haState;
			mds_role = V_DEST_RL_STANDBY;
			TRACE("STANDBY STATE");
			/*   anchor   = cb->cpd_anc; */
		}
		memset(&vda_info, 0, sizeof(vda_info));

		vda_info.req = NCSVDA_VDEST_CHG_ROLE;
		vda_info.info.vdest_chg_role.i_vdest = cb->cpd_dest_id;
		/* vda_info.info.vdest_chg_role.i_anc = anchor; */
		vda_info.info.vdest_chg_role.i_new_role = mds_role;
		rc = ncsvda_api(&vda_info);
		if (NCSCC_RC_SUCCESS != rc) {
			m_LEAP_DBG_SINK_VOID;
			LOG_ER("cpd vdest change role failed");
			ncshm_give_hdl(cb->cpd_hdl);
			TRACE_LEAVE();
			return;
		}
		if (cpd_mbcsv_chgrole(cb) != NCSCC_RC_SUCCESS) {
			TRACE_4("cpd mbcsv chgrole failed");
		}

		/** Set the CB's anchor value */
		/*      cb->cpd_anc= anchor;  */
		saAmfResponse(cb->amf_hdl, invocation, saErr);
		ncshm_give_hdl(cb->cpd_hdl);

		if (SA_AMF_HA_ACTIVE == cb->ha_state) {
			cpd_cpnd_info_node_getnext(&cb->cpnd_tree, NULL, &node_info);
			while (node_info) {
				prev_dest = node_info->cpnd_dest;
				if (node_info->timer_state == 2) {
					TRACE	
						("THE TIMER STATE IS 2 MEANS TIMER EXPIRED BUT STILL DID NOT GET ACTIVE STATE");
					cpd_process_cpnd_down(cb, &node_info->cpnd_dest);
				}
				cpd_cpnd_info_node_getnext(&cb->cpnd_tree, &prev_dest, &node_info);

			}
			TRACE_2("cpd csi set cb success I AM ACTIVE ");
		}
		if (SA_AMF_HA_STANDBY == cb->ha_state)
			TRACE_2("cpd csi set cb success I AM STANDBY");

	} else {
		TRACE_4("cpd donot exist");
	}
	TRACE_LEAVE();
	return;
}	/* End of cpd_saf_csi_set_cb() */

/****************************************************************************
 * Name          : cpd_amf_init
 *
 * Description   : CPD initializes AMF for involking process and registers 
 *                 the various callback functions.
 *
 * Arguments     : cpd_cb  - CPD control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t cpd_amf_init(CPD_CB *cpd_cb)
{
	SaAmfCallbacksT amfCallbacks;
	SaVersionT amf_version;
	SaAisErrorT error;
	uint32_t res = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	memset(&amfCallbacks, 0, sizeof(SaAmfCallbacksT));

	amfCallbacks.saAmfHealthcheckCallback = cpd_saf_hlth_chk_cb;
	amfCallbacks.saAmfCSISetCallback = cpd_saf_csi_set_cb;
	amfCallbacks.saAmfComponentTerminateCallback = cpd_amf_comp_terminate_callback;
	amfCallbacks.saAmfCSIRemoveCallback = cpd_amf_csi_rmv_callback;

	m_CPSV_GET_AMF_VER(amf_version);

	error = saAmfInitialize(&cpd_cb->amf_hdl, &amfCallbacks, &amf_version);

	if (error != SA_AIS_OK) {
		LOG_ER("saAmfInitialize failed with Error:%u",error);
		res = NCSCC_RC_FAILURE;
	}
	if (error == SA_AIS_OK)
		TRACE_2("cpd amf init success");

	TRACE_LEAVE();
	return (res);
}

/****************************************************************************
 * Name          : cpd_amf_de_init
 *
 * Description   : CPD uninitializes AMF for involking process.
 *
 * Arguments     : cpd_cb  - CPD control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
void cpd_amf_de_init(CPD_CB *cpd_cb)
{
	if (saAmfFinalize(cpd_cb->amf_hdl) != SA_AIS_OK)
		TRACE_4("cpd amf destroy failed");
}

/****************************************************************************
 * Name          : cpd_amf_register
 *
 * Description   : CPD registers with AMF for involking process.
 *
 * Arguments     : cpd_cb  - Ifsv control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t cpd_amf_register(CPD_CB *cpd_cb)
{
	SaAisErrorT error;
	TRACE_ENTER();
	/* get the component name */
	error = saAmfComponentNameGet(cpd_cb->amf_hdl, &cpd_cb->comp_name);
	if (error != SA_AIS_OK) {
		LOG_ER("cpd amf compname get failed with Error: %u",error);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	if (saAmfComponentRegister(cpd_cb->amf_hdl, &cpd_cb->comp_name, (SaNameT *)NULL) == SA_AIS_OK) {
		TRACE_LEAVE2("cpd amf register success for %s",cpd_cb->comp_name.value);
		return NCSCC_RC_SUCCESS;
	} else {
		TRACE_LEAVE2("cpd Amf component register failed for %s",cpd_cb->comp_name.value);
		return NCSCC_RC_FAILURE;
	}
}

/****************************************************************************
 * Name          : cpd_amf_deregister
 *
 * Description   : CPD deregisters with AMF for involking process.
 *
 * Arguments     : cpd_cb  - Ifsv control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t cpd_amf_deregister(CPD_CB *cpd_cb)
{
	SaNameT comp_name = { 5, "CPD" };
	if (saAmfComponentUnregister(cpd_cb->amf_hdl, &comp_name, (SaNameT *)NULL) == SA_AIS_OK)
		return NCSCC_RC_SUCCESS;
	else
		return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : cpd_amf_comp_terminate_callback
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
void cpd_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName)
{
	CPD_CB *cb = 0;
	SaAisErrorT saErr = SA_AIS_OK;
	TRACE_ENTER();
	cb = ncshm_take_hdl(NCS_SERVICE_ID_CPD, gl_cpd_cb_hdl);
	if (cb) {
		saAmfResponse(cb->amf_hdl, invocation, saErr);
		ncshm_give_hdl(cb->cpd_hdl);
	}
	LOG_NO("Received AMF component terminate callback, exiting");
	TRACE_LEAVE();
	exit(0);
}

/****************************************************************************
 * Name          : cpd_amf_csi_rmv_callback
 *
 * Description   : TBD
 *
 *
 * Return Values : None 
 *****************************************************************************/
	void
cpd_amf_csi_rmv_callback(SaInvocationT invocation,
		const SaNameT *compName, const SaNameT *csiName, SaAmfCSIFlagsT csiFlags)
{
	CPD_CB *cb = 0;
	SaAisErrorT saErr = SA_AIS_OK;

	TRACE_ENTER();
	cb = ncshm_take_hdl(NCS_SERVICE_ID_CPD, gl_cpd_cb_hdl);
	if (cb) {
		saAmfResponse(cb->amf_hdl, invocation, saErr);
		ncshm_give_hdl(cb->cpd_hdl);
	}
	
	TRACE_2("cpd amf csi rmv cb invoked");
	TRACE_LEAVE();
	return;
}

#endif
