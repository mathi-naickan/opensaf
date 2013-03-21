/*
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
 * Author(s): GoAhead Software
 *
 */

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <poll.h>
#include <new>
#include <vector>
#include <string>
#include <ncssysf_ipc.h>

#include <saAis.h>
#include <saSmf.h>
#include "SmfCallback.hh"
#include "SmfUpgradeStep.hh"
#include "SmfUpgradeProcedure.hh"
#include "SmfProcedureThread.hh"
#include "logtrace.h"

#include <smfsv_evt.h>
#include <saf_error.h>
#include "smfd.h"
#include "smfd_smfnd.h"


/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// ------class SmfCallback ------------------------------------------------
//
// SmfCallback implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SaAisErrorT SmfCallback::execute(std::string & step_dn)
{
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();
	/* compose an event and send it to all SMF-NDs */
	rc = send_callback_msg(SA_SMF_UPGRADE, step_dn);
	TRACE_LEAVE2("rc = %s", saf_error(rc));
	return rc;
}
//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SaAisErrorT SmfCallback::rollback(std::string & step_dn)
{
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();
	/* compose an event and send it to all SMF-NDs */
	rc = send_callback_msg(SA_SMF_ROLLBACK, step_dn);
	TRACE_LEAVE2("rc = %s", saf_error(rc));
	return rc;
}
//------------------------------------------------------------------------------
// send_callback_msg()
//------------------------------------------------------------------------------
SaAisErrorT SmfCallback::send_callback_msg(SaSmfPhaseT phase, std::string & step_dn) 
{
	std::string dn;
	SYSF_MBX *cbk_mbx; 
	SMFSV_EVT smfsv_evt, *evt;
	NCSMDS_INFO    mds_info;
	struct pollfd fds[1];
	SMFD_SMFND_ADEST_INVID_MAP *temp=NULL, *new_inv_id=NULL;
	SMF_RESP_EVT *rsp_evt;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAisErrorT ais_err = SA_AIS_OK;
	SaInvocationT inv_id_sent;

	TRACE_ENTER2("callback invoked atAction %d", m_atAction);
	switch (m_atAction) {
		case beforeLock:
		case beforeTermination:
		case afterImmModification:
		case afterInstantiation:
		case afterUnlock:
		{
			cbk_mbx = &((m_procedure)->getProcThread()->getCbkMbx());
			dn = step_dn;
			break;
		}
		case atProcInitAction:
		case atProcWrapupAction:
		{
			cbk_mbx = &((m_procedure)->getProcThread()->getCbkMbx());
			dn = m_procedure->getDn();
			break;	
		}
		case atCampInit:
		case atCampVerify:
		case atCampBackup:
		case atCampRollback:
		case atCampCommit:
		case atCampInitAction:
		case atCampWrapupAction:
		case atCampCompleteAction:
		{
			dn = (SmfCampaignThread::instance())->campaign()->getDn();
			cbk_mbx = &(SmfCampaignThread::instance()->getCbkMbx());
			break;
		}
		case atAdminVerify:
		{
			dn = (SmfCampaignThread::instance())->campaign()->getDn();;
			cbk_mbx = &(SmfCampaignThread::instance()->getCbkMbx());
			break;
		}
		default:
			/* log */
			TRACE_LEAVE();
			return SA_AIS_ERR_FAILED_OPERATION;
	}

	/* compose an event and send it to all SMF-NDs */
	memset(&smfsv_evt, 0, sizeof(SMFSV_EVT));
	smfsv_evt.type = SMFSV_EVT_TYPE_SMFND;
	smfsv_evt.fr_dest = smfd_cb->mds_dest;
	smfsv_evt.fr_svc = NCSMDS_SVC_ID_SMFD;
	/*smfsv_evt.fr_node_id = ;*/

	smfsv_evt.info.smfnd.type = SMFND_EVT_CBK_RSP;
	smfsv_evt.info.smfnd.event.cbk_req_rsp.evt_type = SMF_CLBK_EVT;
       	smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.inv_id = ++(smfd_cb->cbk_inv_id);
       	smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.camp_phase = phase;

       	smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.cbk_label.labelSize = strlen(m_callbackLabel.c_str());
	TRACE_2("cbk label c_str() %s, size %llu", m_callbackLabel.c_str(), 
		smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.cbk_label.labelSize);
	if (smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.cbk_label.labelSize == 0) {
		TRACE_LEAVE();
		return SA_AIS_ERR_FAILED_OPERATION;
	}
       	smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.cbk_label.label = (SaUint8T*)
		calloc ((smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.cbk_label.labelSize+1), sizeof(char));
       	strcpy((char *)smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.cbk_label.label, m_callbackLabel.c_str()); 
	if (m_stringToPass.c_str() != NULL) {
		smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.params = 
			(char *)calloc(strlen(m_stringToPass.c_str())+1, sizeof(char));
       		strcpy(smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.params, m_stringToPass.c_str());
		smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.params_len = strlen(m_stringToPass.c_str());
		TRACE_2("stringToPass %s", m_stringToPass.c_str());
	}

	smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.object_name.length = strlen(dn.c_str());
	memcpy(smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.object_name.value, dn.c_str(), 
		smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.object_name.length);
	TRACE_2("dn %s, size %d", dn.c_str(), 
		smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.object_name.length);

	inv_id_sent = smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.inv_id;
	if (m_time != 0){
		smfd_cb_lock();
		new_inv_id = (SMFD_SMFND_ADEST_INVID_MAP*)calloc (1, sizeof(SMFD_SMFND_ADEST_INVID_MAP));
		new_inv_id->inv_id = inv_id_sent;
		new_inv_id->no_of_cbks = smfd_cb->no_of_smfnd;
		new_inv_id->cbk_mbx = cbk_mbx;
		new_inv_id->next_invid = NULL;

		/* Add the new_inv_id in the smfd_cb->smfnd_list */
		temp = smfd_cb->smfnd_list;
		if (temp == NULL) {
			smfd_cb->smfnd_list = new_inv_id;
		}
		else {
			while (temp->next_invid != NULL) {
				temp = temp->next_invid;
			}
			temp->next_invid = new_inv_id;
		}
		smfd_cb_unlock();
	}
	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = smfd_cb->mds_handle;
	mds_info.i_svc_id = NCSMDS_SVC_ID_SMFD;
	mds_info.i_op = MDS_SEND;
	
	mds_info.info.svc_send.i_msg = (NCSCONTEXT) &smfsv_evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_SMFND;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_BCAST;
	mds_info.info.svc_send.info.bcast.i_bcast_scope = NCSMDS_SCOPE_NONE;
	
	TRACE("before mds send ");
	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("mds send failed, rc=%d", rc);
		/* Remove/Free the inv_id from the list */
		ais_err = SA_AIS_ERR_FAILED_OPERATION;
		goto rem_invid;
	}

	TRACE_2("mds send successful, rc=%d", rc);
	/* Wait for the response on select */
	NCS_SEL_OBJ mbx_fd;
	mbx_fd = ncs_ipc_get_sel_obj(cbk_mbx);
	fds[0].fd = mbx_fd.rmv_obj;
	fds[0].events = POLLIN;

	TRACE_2("before poll, fds[0].fd = %d", fds[0].fd);
	while (true) {
		/* m_time is given in nano sec (SaTimeT), poll only accepts milliseconds as timeout   */
		/* m_time values smaller than 1000000 nano sec will result in a poll with timeout = 1 */
		int polltimeout = (m_time > 0 && m_time < 1000000) ? 1 : m_time/ 1000000 ;
		if (poll(fds, 1, polltimeout) == -1) {
			if (errno == EINTR) {
				continue;
			}

			LOG_NO ("poll failed, error = %s", strerror(errno));
			ais_err = SA_AIS_ERR_FAILED_OPERATION;
			goto rem_invid;
		}

		break;
	}

	if (fds[0].revents & POLLIN) { 
		/* receive from the mailbox fd */
		if (NULL != (evt = (SMFSV_EVT *) m_NCS_IPC_NON_BLK_RECEIVE(cbk_mbx, evt))) {
			/* check if the consolidated response has same inv_id that was sent from here */
			rsp_evt = &evt->info.smfd.event.cbk_rsp.evt.resp_evt;
			TRACE_2("Got response for the inv_id %llu, err %d", rsp_evt->inv_id, rsp_evt->err);
			/* below check is only for sanity,may not be reqd */
			if (rsp_evt->inv_id != inv_id_sent) {
				LOG_ER ("Waited for the consolidated response for inv_id %llu, but got %llu, err %d", \
					inv_id_sent, rsp_evt->inv_id, rsp_evt->err);
				ais_err = SA_AIS_ERR_FAILED_OPERATION;
				free(evt);
				goto rem_invid;
			}
			/* check the response if it is success or failure */
			if (rsp_evt->err == SA_AIS_ERR_FAILED_OPERATION) {
				LOG_ER("Got failure response for the inv_id %llu", inv_id_sent);
				ais_err = rsp_evt->err;
				free(evt);
				goto rem_invid;
			}
			free(evt);
		}
	}
rem_invid:
	if (m_time != 0) {
		smfd_cb_lock();
		temp = smfd_cb->smfnd_list;
		if (temp != NULL) {
			if (temp->inv_id == new_inv_id->inv_id) {
				smfd_cb->smfnd_list = temp->next_invid;
			}
			else {
				while (temp->next_invid->inv_id != new_inv_id->inv_id) {
					temp = temp->next_invid;
				}
				temp->next_invid = new_inv_id->next_invid;
			}
			/* free new_inv_id */
			free(new_inv_id);
		}
		smfd_cb_unlock();
	}
	free(smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.cbk_label.label);
	if (smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.params != NULL) {
		free(smfsv_evt.info.smfnd.event.cbk_req_rsp.evt.cbk_evt.params);
	}
	TRACE_LEAVE();
	return ais_err;
}
