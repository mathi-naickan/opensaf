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
   
   mqd_saf_hlth_chk_cb................MQD health check callback 
   mqd_saf_readiness_state_cb.........MQD rediness state callback
   mqd_saf_csi_set_cb.................MQD component state callback
   mqd_saf_pend_oper_confirm_cb.......MQD pending operation callback
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#include "mqd.h"
#include "mqd_imm.h"
extern MQDLIB_INFO gl_mqdinfo;
static uns32 mqd_process_quisced_state(MQD_CB *pMqd, SaInvocationT invocation, SaAmfHAStateT haState);
/****************************************************************************
 PROCEDURE NAME : mqd_saf_hlth_chk_cb

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
void mqd_saf_hlth_chk_cb(SaInvocationT invocation, const SaNameT *compName, SaAmfHealthcheckKeyT *checkType)
{
	MQD_CB *pMqd = 0;
	uns32 rc = NCSCC_RC_SUCCESS;
	SaAisErrorT saErr = SA_AIS_OK;
	/* Get the COntrol Block Pointer */
	pMqd = ncshm_take_hdl(NCS_SERVICE_ID_MQD, gl_mqdinfo.inst_hdl);
	if (pMqd) {
		saAmfResponse(pMqd->amf_hdl, invocation, saErr);
		ncshm_give_hdl(pMqd->hdl);
	} else {
		rc = NCSCC_RC_FAILURE;
		m_LOG_MQSV_D(MQD_DONOT_EXIST, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
	}
	return;
}	/* End of mqd_saf_hlth_chk_cb() */

/****************************************************************************\
 PROCEDURE NAME : mqd_saf_csi_set_cb
 
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
void mqd_saf_csi_set_cb(SaInvocationT invocation,
			const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor)
{
	MQD_CB *pMqd = 0;
	MQSV_EVT *pEvt = 0;
	MQD_ND_DB_NODE *pNdNode = 0;
	uns32 rc = NCSCC_RC_SUCCESS;
	SaAisErrorT saErr = SA_AIS_OK;
	V_DEST_RL mds_role;
	NCSVDA_INFO vda_info;
	NODE_ID nodeid = 0;

	pMqd = ncshm_take_hdl(NCS_SERVICE_ID_MQD, gl_mqdinfo.inst_hdl);
	if (pMqd) {
		m_LOG_MQSV_D(MQD_CSI_SET_ROLE, NCSFL_LC_MQSV_INIT, NCSFL_SEV_NOTICE, haState, __FILE__, __LINE__);
		if ((SA_AMF_HA_QUIESCED == haState) && (pMqd->ha_state == SA_AMF_HA_ACTIVE)) {
			saErr = immutil_saImmOiImplementerClear(pMqd->immOiHandle);
			if (saErr != SA_AIS_OK) {
				mqd_genlog(NCSFL_SEV_ERROR, "saImmOiImplementerClear failed: err = %u \n", saErr);
			}
			mqd_process_quisced_state(pMqd, invocation, haState);
			ncshm_give_hdl(pMqd->hdl);
			return;
		}

		pMqd->ha_state = haState;	/* Set the HA State */
		if (0 != pMqd->ha_state) {
			/* Put it in MQD's Event Queue */
			pEvt = m_MMGR_ALLOC_MQSV_EVT(pMqd->my_svc_id);
			if (pEvt) {
				memset(pEvt, 0, sizeof(MQSV_EVT));
				pEvt->type = MQSV_EVT_MQD_CTRL;
				pEvt->msg.mqd_ctrl.type = MQD_MSG_COMP;
				pEvt->msg.mqd_ctrl.info.init = TRUE;

				/* Put it in MQD's Event Queue */
				rc = m_MQD_EVT_SEND(&pMqd->mbx, pEvt, NCS_IPC_PRIORITY_NORMAL);
				if (NCSCC_RC_SUCCESS != rc) {
					m_LOG_MQSV_D(MQD_MDS_MSG_COMP_EVT_SEND_FAILED, NCSFL_LC_MQSV_INIT,
						     NCSFL_SEV_NOTICE, haState, __FILE__, __LINE__);
					m_MMGR_FREE_MQSV_EVT(pEvt, pMqd->my_svc_id);
					ncshm_give_hdl(pMqd->hdl);
					return;
				}
			} else {
				m_LOG_MQSV_D(MQD_MEMORY_ALLOC_FAIL, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, haState,
					     __FILE__, __LINE__);
			}
		}

      /** Change the MDS role **/
		if (SA_AMF_HA_ACTIVE == pMqd->ha_state) {
			const SaImmOiImplementerNameT implementer_name = (SaImmOiImplementerNameT) "safMsgGrpService";
			/* If this is the active Director, become implementer */
			saErr = immutil_saImmOiImplementerSet(pMqd->immOiHandle, implementer_name);
			if (saErr != SA_AIS_OK){
				mqd_genlog(NCSFL_SEV_ERROR, "mqd_imm_declare_implementer failed: err = %u \n", saErr);
			}
			mds_role = V_DEST_RL_ACTIVE;
		} else {
			mds_role = V_DEST_RL_STANDBY;
		}
		memset(&vda_info, 0, sizeof(vda_info));

		vda_info.req = NCSVDA_VDEST_CHG_ROLE;
		vda_info.info.vdest_chg_role.i_vdest = pMqd->my_dest;
		vda_info.info.vdest_chg_role.i_new_role = mds_role;
		rc = ncsvda_api(&vda_info);
		if (NCSCC_RC_SUCCESS != rc) {
			m_LOG_MQSV_D(MQD_VDEST_CHG_ROLE_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__,
				     __LINE__);
			ncshm_give_hdl(pMqd->hdl);
			return;
		}
		rc = mqd_mbcsv_chgrole(pMqd);

		saAmfResponse(pMqd->amf_hdl, invocation, saErr);

		/*To check for the node down and clean up its info in the data base */
		for (pNdNode = (MQD_ND_DB_NODE *)ncs_patricia_tree_getnext(&pMqd->node_db, (uns8 *)0); pNdNode;
		     pNdNode = (MQD_ND_DB_NODE *)ncs_patricia_tree_getnext(&pMqd->node_db, (uns8 *)&nodeid)) {
			nodeid = pNdNode->info.nodeid;
			/* Post the event to MQD Thread */
			if (pNdNode->info.timer.is_expired == TRUE) {
				TRACE("NODE FOUND FOR CLEAN UP:CSI CALLBACK (TIMER EXPIRY CASE)");
				mqd_timer_expiry_evt_process(pMqd, &nodeid);
			} else {
				if (pNdNode->info.is_restarted == TRUE) {
					TRACE("NODE FOUND FOR CLEAN:CSI CALLBACK (MDS UP CASE)");
					pEvt = m_MMGR_ALLOC_MQSV_EVT(pMqd->my_svc_id);
					if (pEvt) {
						memset(pEvt, 0, sizeof(MQSV_EVT));
						pEvt->type = MQSV_EVT_MQD_CTRL;
						pEvt->msg.mqd_ctrl.type = MQD_ND_STATUS_INFO_TYPE;
						pEvt->msg.mqd_ctrl.info.nd_info.dest = pNdNode->info.dest;
						pEvt->msg.mqd_ctrl.info.nd_info.is_up = TRUE;

						/*      m_GET_TIME_STAMP(pEvt->msg.mqd_ctrl.info.nd_info.event_time); */
						/* Put it in MQD's Event Queue */
						rc = m_MQD_EVT_SEND(&pMqd->mbx, pEvt, NCS_IPC_PRIORITY_NORMAL);
						if (NCSCC_RC_SUCCESS != rc) {
							m_LOG_MQSV_D(MQD_MDS_MSG_COMP_EVT_SEND_FAILED,
								     NCSFL_LC_MQSV_INIT, NCSFL_SEV_NOTICE, haState,
								     __FILE__, __LINE__);
							m_MMGR_FREE_MQSV_EVT(pEvt, pMqd->my_svc_id);
							ncshm_give_hdl(pMqd->hdl);
							return;
						}
					}
				}
			}
		}

		ncshm_give_hdl(pMqd->hdl);
		if (rc != NCSCC_RC_SUCCESS) {
			return;
		}

	} else {
		m_LOG_MQSV_D(MQD_DONOT_EXIST, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
	}
	return;
}	/* End of mqd_saf_csi_set_cb() */

static uns32 mqd_process_quisced_state(MQD_CB *pMqd, SaInvocationT invocation, SaAmfHAStateT haState)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	V_DEST_RL mds_role;
	NCSVDA_INFO vda_info;

	pMqd->invocation = invocation;
	pMqd->is_quisced_set = TRUE;
	pMqd->ha_state = haState;
	memset(&vda_info, 0, sizeof(vda_info));

	mds_role = V_DEST_RL_QUIESCED;

	vda_info.req = NCSVDA_VDEST_CHG_ROLE;
	vda_info.info.vdest_chg_role.i_vdest = pMqd->my_dest;
	vda_info.info.vdest_chg_role.i_new_role = mds_role;
	rc = ncsvda_api(&vda_info);
	if (NCSCC_RC_SUCCESS != rc) {
		m_LOG_MQSV_D(MQD_QUISCED_VDEST_CHGROLE_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__,
			     __LINE__);
		ncshm_give_hdl(pMqd->hdl);
		return rc;
	} else
		m_LOG_MQSV_D(MQD_QUISCED_VDEST_CHGROLE_SUCCESS, NCSFL_LC_MQSV_INIT, NCSFL_SEV_NOTICE, rc, __FILE__,
			     __LINE__);

	return NCSCC_RC_SUCCESS;
}

void mqd_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName)
{
	MQD_CB *pMqd = 0;
	SaAisErrorT saErr = SA_AIS_OK;
	uns32 rc = NCSCC_RC_SUCCESS;

	pMqd = ncshm_take_hdl(NCS_SERVICE_ID_MQD, gl_mqdinfo.inst_hdl);
	if (pMqd) {
      /** Change the MDS role and MBCSV role to the standby**/
		saAmfResponse(pMqd->amf_hdl, invocation, saErr);
		ncshm_give_hdl(pMqd->hdl);
		sleep(1);
	} else {
		rc = NCSCC_RC_FAILURE;
		m_LOG_MQSV_D(MQD_DONOT_EXIST, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
	}
	LOG_NO("Received AMF component terminate callback, exiting");
	exit(0);
}

void mqd_amf_csi_rmv_callback(SaInvocationT invocation,
			      const SaNameT *compName, const SaNameT *csiName, SaAmfCSIFlagsT csiFlags)
{
	MQD_CB *pMqd = 0;
	uns32 rc = NCSCC_RC_SUCCESS;
	SaAisErrorT saErr = SA_AIS_OK;
	V_DEST_RL mds_role;
	NCSVDA_INFO vda_info;

	pMqd = ncshm_take_hdl(NCS_SERVICE_ID_MQD, gl_mqdinfo.inst_hdl);
	if (pMqd) {
      /** Change the MDS role and MBCSV role to the standby**/
		pMqd->ha_state = SA_AMF_HA_STANDBY;

		mds_role = V_DEST_RL_STANDBY;

		memset(&vda_info, 0, sizeof(vda_info));

		vda_info.req = NCSVDA_VDEST_CHG_ROLE;
		vda_info.info.vdest_chg_role.i_vdest = pMqd->my_dest;
		vda_info.info.vdest_chg_role.i_new_role = mds_role;
		rc = ncsvda_api(&vda_info);
		if (NCSCC_RC_SUCCESS != rc) {
			m_LOG_MQSV_D(MQD_CSI_REMOVE_CALLBK_CHGROLE_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, rc,
				     __FILE__, __LINE__);
			ncshm_give_hdl(pMqd->hdl);
			return;
		}
		rc = mqd_mbcsv_chgrole(pMqd);

		saAmfResponse(pMqd->amf_hdl, invocation, saErr);
		ncshm_give_hdl(pMqd->hdl);
		m_LOG_MQSV_D(MQD_CSI_REMOVE_CALLBK_SUCCESSFULL, NCSFL_LC_MQSV_INIT, NCSFL_SEV_NOTICE, rc, __FILE__,
			     __LINE__);

	} else
		m_LOG_MQSV_D(MQD_DONOT_EXIST, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
	return;
}
