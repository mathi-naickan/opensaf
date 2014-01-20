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
 * Author(s): Ericsson AB
 *
 */

#include <alloca.h>
#include <time.h>
#include <limits.h>

#include <saClm.h>

#include <ncs_main_papi.h>

#include "smfd.h"
#include "smfd_smfnd.h"
#include "smfd_evt.h"
#include "smfsv_defs.h"
#include "smfsv_evt.h"

void proc_callback_rsp(smfd_cb_t *, SMFSV_EVT *);
void proc_quiesced_ack(smfd_cb_t *, SMFSV_EVT *);

/****************************************************************************
 * Name          : proc_mds_info
 *
 * Description   : Handle a mds info event 
 *
 * Arguments     : cb  - SMFND control block  
 *                 evt - The MDS_INFO event
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
static void proc_mds_info(smfd_cb_t * cb, SMFSV_EVT * evt)
{
	smfsv_mds_info *mds_info = &evt->info.smfd.event.mds_info;

	switch (mds_info->change) {

	case NCSMDS_RED_UP:
		/* get the peer mds_red_up */

		break;

	case NCSMDS_UP:

		if (mds_info->svc_id == NCSMDS_SVC_ID_SMFD) {
			return;
		}

		if (mds_info->svc_id == NCSMDS_SVC_ID_SMFND) {
			smfnd_up(mds_info->node_id, mds_info->dest, mds_info->rem_svc_pvt_ver);
			cb->no_of_smfnd++;
		}
		break;

	case NCSMDS_DOWN:

		if (mds_info->svc_id == NCSMDS_SVC_ID_SMFD) {
			return;
		}

		if (mds_info->svc_id == NCSMDS_SVC_ID_SMFND) {
			smfnd_down(mds_info->node_id);
			cb->no_of_smfnd--;
		}
		break;

	default:

		break;
	}

	return;
}

/****************************************************************************
 * Name          : smfd_process_mbx
 *
 * Description   : This is the function which process the IPC mail box of 
 *                 SMFD 
 *
 * Arguments     : mbx  - This is the mail box pointer on which SMFD is 
 *                        going to block.  
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void smfd_process_mbx(SYSF_MBX * mbx)
{
	SMFSV_EVT *evt = (SMFSV_EVT *) m_NCS_IPC_NON_BLK_RECEIVE(mbx, evt);

	if (evt != NULL) {
		if (evt->type != SMFSV_EVT_TYPE_SMFD) {
			LOG_ER("SMFD received wrong event type %d", evt->type);
			goto err;
		}

		switch (evt->info.smfd.type) {
		case SMFD_EVT_MDS_INFO:
			{
				proc_mds_info(smfd_cb, evt);
				break;
			}
		case SMFD_EVT_CMD_RSP:
			{
				/* The CMD RSP is always received synchronized so skip it here */
				break;
			}
		case SMFD_EVT_CBK_RSP:
			{
				proc_callback_rsp(smfd_cb, evt);
				break;
			}
		case SMFD_EVT_QUIESCED_ACK:
			{
				proc_quiesced_ack(smfd_cb, evt);
				break;
			}
		default:
			{
				LOG_ER("SMFND received unknown event %d",
				       evt->info.smfnd.type);
				break;
			}
		}
	}

 err:
	smfsv_evt_destroy(evt);
}

void proc_callback_rsp(smfd_cb_t *cb, SMFSV_EVT *evt)
{
	SMF_EVT *cbk_rsp = &evt->info.smfd.event.cbk_rsp;
	uint32_t      rc = NCSCC_RC_SUCCESS;
	SMFSV_EVT *new_evt;
	
	TRACE_ENTER();
	if (cbk_rsp->evt_type == SMF_CLBK_EVT) {
		/* Ignore, log and return, SMF-D should not receive this */
		TRACE_LEAVE2("Received SMF_CLBK_EVT, which should not be the case");
		return;
	}
	else if (cbk_rsp->evt_type == SMF_RSP_EVT) {
		smfd_cb_lock();
		SMFD_SMFND_ADEST_INVID_MAP *prev=NULL, *temp = smfd_cb->smfnd_list;

		TRACE_2("Received evt_type: %d, inv_id: %llu, err: %d", cbk_rsp->evt_type,
			cbk_rsp->evt.resp_evt.inv_id, cbk_rsp->evt.resp_evt.err);
		while (temp != NULL) {
			if (temp->inv_id == cbk_rsp->evt.resp_evt.inv_id)
			{
				/* check the response */
				TRACE_2("found the node with inv_id: %llu", cbk_rsp->evt.resp_evt.inv_id);
				if (cbk_rsp->evt.resp_evt.err == SA_AIS_ERR_FAILED_OPERATION) {
					temp->no_of_cbks = 0;
				}
				else if (cbk_rsp->evt.resp_evt.err == SA_AIS_OK) {
					temp->no_of_cbks--;
				}
				if (temp->no_of_cbks == 0) {
					/* all responses are received for this inv_id */
					TRACE("last response received, cleaning up the node");
					if (prev == NULL) {
						smfd_cb->smfnd_list = smfd_cb->smfnd_list->next_invid;
					}
					else {
						prev->next_invid = temp->next_invid;
					}

					new_evt = (SMFSV_EVT *)calloc (1, sizeof(SMFSV_EVT));
					if (new_evt == NULL) {
						LOG_CR("No mem allocated for event");
						free(temp);
						break; /* from the while, otherwise return from function */
                                        }
					memcpy (new_evt, evt, sizeof(SMFSV_EVT));
					rc = m_NCS_IPC_SEND(temp->cbk_mbx,
								new_evt,
								NCS_IPC_PRIORITY_HIGH); 
					if (rc != NCSCC_RC_SUCCESS) {
						LOG_CR("IPC send failed %d, %s", rc, strerror(errno));
					}
					free(temp);
					break; /* from the while, otherwise return from function */
				}
			}
			prev = temp;
			temp = temp->next_invid;
		}
		smfd_cb_unlock();
	}
	TRACE_LEAVE();
	return;
}

/****************************************************************************
 * Name          : proc_quiesced_ack
 *
 * Description   : This is the function which is called when smfd receives an
 *                 quiesced ack event from MDS 
 *
 * Arguments     : evt  - Message that was posted to the SMFSV Mail box.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/
void proc_quiesced_ack(smfd_cb_t *cb, SMFSV_EVT *evt)
{
	TRACE_ENTER();

	if (cb->is_quiesced_set == true) {
		/* Update control block */
		cb->is_quiesced_set = false;
		cb->ha_state = SA_AMF_HA_QUIESCED;

		/* Respond to AMF */
                if (saAmfResponse(cb->amf_hdl, cb->amf_invocation_id, SA_AIS_OK) != SA_AIS_OK) {
                        LOG_ER("saAmfResponse failed");
                }
	} else
		LOG_ER("Received SMFD_EVT_QUIESCED_ACK message but is_quiesced_set==false");

	TRACE_LEAVE();
}
