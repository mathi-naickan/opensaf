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

  DESCRIPTION:

  This file contains library routines for handle database.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

******************************************************************************
*/

#include "ava.h"

static uint32_t ava_hdl_cbk_dispatch_one(AVA_CB **, AVA_HDL_REC **);
static uint32_t ava_hdl_cbk_dispatch_all(AVA_CB **, AVA_HDL_REC **);
static uint32_t ava_hdl_cbk_dispatch_block(AVA_CB **, AVA_HDL_REC **);

static uint32_t ava_hdl_cbk_rec_prc(AVSV_AMF_CBK_INFO *, OsafAmfCallbacksT *);

static void ava_hdl_pend_resp_list_del(AVA_CB *, AVA_PEND_RESP *);
static bool ava_hdl_cbk_ipc_mbx_del(NCSCONTEXT arg, NCSCONTEXT msg);

/****************************************************************************
  Name          : ava_hdl_init
 
  Description   : This routine initializes the handle database.
 
  Arguments     : hdl_db  - ptr to the handle database
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t ava_hdl_init(AVA_HDL_DB *hdl_db)
{
	NCS_PATRICIA_PARAMS param;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));

	/* init the hdl db tree */
	param.key_size = sizeof(uint32_t);

	rc = ncs_patricia_tree_init(&hdl_db->hdl_db_anchor, &param);
	if (NCSCC_RC_SUCCESS == rc)
		hdl_db->num = 0;
	else
		TRACE("Patricia tree init failed for Handled DB");

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ava_hdl_del
 
  Description   : This routine deletes the handle database.
 
  Arguments     : cb  - ptr to the AvA control block
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
void ava_hdl_del(AVA_CB *cb)
{
	AVA_HDL_DB *hdl_db = &cb->hdl_db;
	AVA_HDL_REC *hdl_rec = 0;
	TRACE_ENTER();

	/* scan the entire handle db & delete each record */
	while ((hdl_rec = (AVA_HDL_REC *)
		ncs_patricia_tree_getnext(&hdl_db->hdl_db_anchor, 0))) {
		ava_hdl_rec_del(cb, hdl_db, &hdl_rec);
	}

	/* there shouldn't be any record left */
	osafassert(!hdl_db->num);

	/* destroy the hdl db tree */
	ncs_patricia_tree_destroy(&hdl_db->hdl_db_anchor);

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : ava_hdl_rec_del
 
  Description   : This routine deletes the handle record.
 
  Arguments     : cb      - ptr tot he AvA control block
                  hdl_db  - ptr to the hdl db
                  hdl_rec - ptr to the hdl record
 
  Return Values : None
 
  Notes         : The selection object is destroyed after all the means to 
                  access the handle record (ie. hdl db tree or hdl mngr) is 
                  removed. This is to disallow the waiting thread to access 
                  the hdl rec while other thread executes saAmfFinalize on it.
******************************************************************************/
void ava_hdl_rec_del(AVA_CB *cb, AVA_HDL_DB *hdl_db, AVA_HDL_REC **_hdl_rec)
{
	AVA_HDL_REC *hdl_rec = *_hdl_rec;
	uint32_t hdl = hdl_rec->hdl;
	TRACE_ENTER();

	/* pop the hdl rec */
	ncs_patricia_tree_del(&hdl_db->hdl_db_anchor, &hdl_rec->hdl_node);

	/* detach the mail box */
	m_NCS_IPC_DETACH(&hdl_rec->callbk_mbx, ava_hdl_cbk_ipc_mbx_del, hdl_rec);

	/* delete the mailbox */
	m_NCS_IPC_RELEASE(&hdl_rec->callbk_mbx, NULL);

	/* clean the pend resp list */
	ava_hdl_pend_resp_list_del(cb, &hdl_rec->pend_resp);

	/* remove the association with hdl-mngr */
	ncshm_destroy_hdl(NCS_SERVICE_ID_AVA, hdl_rec->hdl);

	/* free the hdl rec */
	free(hdl_rec);
	*_hdl_rec = NULL;

	/* update the no of records */
	hdl_db->num--;

	TRACE_LEAVE2("Handle = %x, successfully deleted from Handle DB, num handles = %d",hdl, hdl_db->num);
	return;
}

/****************************************************************************
  Name          : ava_hdl_cbk_rec_del
 
  Description   : This routine deletes the specified callback record in the 
                  pending callback list.
 
  Arguments     : rec - ptr to the pending callbk rec
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
void ava_hdl_cbk_rec_del(AVA_PEND_CBK_REC *rec)
{
	/* delete the callback info */
	if (rec->cbk_info)
		avsv_amf_cbk_free(rec->cbk_info);

	/* delete the record */
	free(rec);
}

/**
 * This routine scans the ipc mailbox queue and deletes all the records.
 *
 * @param arg
 * @param msg
 *
 * @returns bool
 */
static bool ava_hdl_cbk_ipc_mbx_del(NCSCONTEXT arg, NCSCONTEXT msg)
{
	AVA_PEND_CBK_REC *callbk, *pnext;

	TRACE_ENTER();

	pnext = callbk = (AVA_PEND_CBK_REC *)msg;

	while (pnext) {
		pnext = callbk->next;
		ava_hdl_cbk_rec_del(callbk);
		callbk = pnext;
	}

	TRACE_LEAVE();
	return true;
}

/****************************************************************************
  Name          : ava_hdl_rec_add
 
  Description   : This routine adds the handle record to the handle db.
 
  Arguments     : cb       - ptr tot he AvA control block
                  hdl_db   - ptr to the hdl db
                  reg_cbks - ptr to the set of registered callbacks
 
  Return Values : ptr to the handle record
 
  Notes         : None
******************************************************************************/
AVA_HDL_REC *ava_hdl_rec_add(AVA_CB *cb, AVA_HDL_DB *hdl_db, const OsafAmfCallbacksT *reg_cbks)
{
	AVA_HDL_REC *rec = 0;
	TRACE_ENTER();

	/* allocate the hdl rec */
	if (!(rec = static_cast<AVA_HDL_REC*>(calloc(1, sizeof(AVA_HDL_REC))))) {
		LOG_CR("Error occurred calling calloc");
		osafassert(0);
	}

	/* create the association with hdl-mngr */
	if (!(rec->hdl = ncshm_create_hdl(cb->pool_id, NCS_SERVICE_ID_AVA, (NCSCONTEXT)rec))) {
		LOG_CR("Error occurred during creation of handle");
		osafassert(0);
	}

	/* store the registered callbacks */
	if (reg_cbks)
		memcpy((void *)&rec->reg_cbk, (void *)reg_cbks, sizeof(OsafAmfCallbacksT));
	/* add the record to the hdl db */
	rec->hdl_node.key_info = (uint8_t *)&rec->hdl;
	if (ncs_patricia_tree_add(&hdl_db->hdl_db_anchor, &rec->hdl_node)
	    != NCSCC_RC_SUCCESS) {
		LOG_CR("Patricia tree add failed ");
		osafassert(0);
	}

	/* update the no of records */
	hdl_db->num++;

	TRACE_LEAVE2("Handle = %x successfully added to Handle DB, num hdls = %d",rec->hdl,hdl_db->num);
	return rec;
}

/****************************************************************************
  Name          : ava_hdl_cbk_param_add
 
  Description   : This routine adds the callback parameters to the pending 
                  callback list.
 
  Arguments     : cb       - ptr to the AvA control block
                  hdl_rec  - ptr to the handle record
                  cbk_info - ptr to the callback parameters
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : This routine reuses the callback info ptr that is received 
                  from MDS thus avoiding an extra copy.
******************************************************************************/
uint32_t ava_hdl_cbk_param_add(AVA_CB *cb, AVA_HDL_REC *hdl_rec, AVSV_AMF_CBK_INFO *cbk_info)
{
	AVA_PEND_CBK_REC *rec = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* allocate the callbk rec */
	if (!(rec = static_cast<AVA_PEND_CBK_REC*>(calloc(1, sizeof(AVA_PEND_CBK_REC))))) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* populate the callbk parameters */
	rec->cbk_info = cbk_info;

	/* now push it to the pending list */
	rc = m_NCS_IPC_SEND(&hdl_rec->callbk_mbx, rec, NCS_IPC_PRIORITY_NORMAL);

done:
	if ((NCSCC_RC_SUCCESS != rc) && rec)
		ava_hdl_cbk_rec_del(rec);

	TRACE_LEAVE2("Callback param successfully added for handle: %x", hdl_rec->hdl);
	return rc;
}

/****************************************************************************
  Name          : ava_hdl_cbk_dispatch
 
  Description   : This routine dispatches the pending callbacks as per the 
                  dispatch flags.
 
  Arguments     : cb      - ptr to the AvA control block
                  hdl_rec - ptr to the handle record
                  flags   - dispatch flags
 
  Return Values : SA_AIS_OK/SA_AIS_ERR_<CODE>
 
  Notes         : None
******************************************************************************/
uint32_t ava_hdl_cbk_dispatch(AVA_CB **cb, AVA_HDL_REC **hdl_rec, SaDispatchFlagsT flags)
{
	uint32_t rc = SA_AIS_OK;
	TRACE_ENTER();

	switch (flags) {
	case SA_DISPATCH_ONE:
		rc = ava_hdl_cbk_dispatch_one(cb, hdl_rec);
		break;

	case SA_DISPATCH_ALL:
		rc = ava_hdl_cbk_dispatch_all(cb, hdl_rec);
		break;

	case SA_DISPATCH_BLOCKING:
		rc = ava_hdl_cbk_dispatch_block(cb, hdl_rec);
		break;

	default:
		osafassert(0);
	}			/* switch */

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ava_hdl_cbk_dispatch_one
 
  Description   : This routine dispatches one pending callback.
 
  Arguments     : cb      - ptr to the AvA control block
                  hdl_rec - ptr to the handle record
 
  Return Values : SA_AIS_OK/SA_AIS_ERR_<CODE>
 
  Notes         : None.
******************************************************************************/
uint32_t ava_hdl_cbk_dispatch_one(AVA_CB **cb, AVA_HDL_REC **hdl_rec)
{
	AVA_PEND_RESP *list_resp = &(*hdl_rec)->pend_resp;
	AVA_PEND_CBK_REC *rec = 0;
	uint32_t hdl = (*hdl_rec)->hdl;
	OsafAmfCallbacksT reg_cbk;
	uint32_t rc = SA_AIS_OK;
	TRACE_ENTER();

	memset(&reg_cbk, 0, sizeof(OsafAmfCallbacksT));
	memcpy(&reg_cbk, &(*hdl_rec)->reg_cbk, sizeof(OsafAmfCallbacksT));

	/* pop the rec from the mailbox queue */
	rec = (AVA_PEND_CBK_REC *)m_NCS_IPC_NON_BLK_RECEIVE(&(*hdl_rec)->callbk_mbx, NULL);

	if (rec) {

		if (rec->cbk_info->type != AVSV_AMF_PG_TRACK) {
			/* push this record into pending response list */
			m_AVA_HDL_PEND_RESP_PUSH(list_resp, (AVA_PEND_RESP_REC *)rec);
			m_AVA_HDL_CBK_REC_IN_DISPATCH_SET(rec);
		}

		/* release the cb lock & return the hdls to the hdl-mngr */
		m_NCS_UNLOCK(&(*cb)->lock, NCS_LOCK_WRITE);
		ncshm_give_hdl(hdl);

		/* process the callback list record */
		rc = ava_hdl_cbk_rec_prc(rec->cbk_info, &reg_cbk);

		m_NCS_LOCK(&(*cb)->lock, NCS_LOCK_WRITE);

		if (0 == (*hdl_rec = static_cast<AVA_HDL_REC*>(ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)))) {
			/* hdl is already finalized */
			ava_hdl_cbk_rec_del(rec);
			TRACE_LEAVE2("Handle is already finalized");
			return rc;
		}

		/* if we are done with this rec, free it */
		if ((rec->cbk_info->type != AVSV_AMF_PG_TRACK) && m_AVA_HDL_IS_CBK_RESP_DONE(rec)) {
			m_AVA_HDL_PEND_RESP_POP(list_resp, rec, rec->cbk_info->inv);
			ava_hdl_cbk_rec_del(rec);
		} else if (rec->cbk_info->type == AVSV_AMF_PG_TRACK) {
			/* PG Track cbk do not have any response */
			ava_hdl_cbk_rec_del(rec);
		} else {
			m_AVA_HDL_CBK_REC_IN_DISPATCH_RESET(rec);
		}

	}
	else
		TRACE_3("No record to process the dispatch()");

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ava_hdl_cbk_dispatch_all
 
  Description   : This routine dispatches all the pending callbacks.
 
  Arguments     : cb      - ptr to the AvA control block
                  hdl_rec - ptr to the handle record
 
  Return Values : SA_AIS_OK/SA_AIS_ERR_<CODE>
 
  Notes         : Refer to the notes in ava_hdl_cbk_dispatch_one().
******************************************************************************/
uint32_t ava_hdl_cbk_dispatch_all(AVA_CB **cb, AVA_HDL_REC **hdl_rec)
{
	AVA_PEND_RESP *list_resp = &(*hdl_rec)->pend_resp;
	AVA_PEND_CBK_REC *rec = 0;
	uint32_t hdl = (*hdl_rec)->hdl;
	OsafAmfCallbacksT reg_cbk;
	uint32_t rc = SA_AIS_OK;
	TRACE_ENTER();

	memset(&reg_cbk, 0, sizeof(OsafAmfCallbacksT));
	memcpy(&reg_cbk, &(*hdl_rec)->reg_cbk, sizeof(OsafAmfCallbacksT));

	/* pop all the records from the mailbox & process them */
	do {
		rec = (AVA_PEND_CBK_REC *)m_NCS_IPC_NON_BLK_RECEIVE(&(*hdl_rec)->callbk_mbx, NULL);
		if (!rec)
			break;

		if (rec->cbk_info->type != AVSV_AMF_PG_TRACK) {
			/* push this record into pending response list */
			m_AVA_HDL_PEND_RESP_PUSH(list_resp, (AVA_PEND_RESP_REC *)rec);
			m_AVA_HDL_CBK_REC_IN_DISPATCH_SET(rec);
		}

		/* release the cb lock & return the hdls to the hdl-mngr */
		m_NCS_UNLOCK(&(*cb)->lock, NCS_LOCK_WRITE);
		ncshm_give_hdl(hdl);

		/* process the callback list record */
		rc = ava_hdl_cbk_rec_prc(rec->cbk_info, &reg_cbk);

		m_NCS_LOCK(&(*cb)->lock, NCS_LOCK_WRITE);

		/* is it finalized ? */
		if (!(*hdl_rec = static_cast<AVA_HDL_REC*>(ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl)))) {
			ava_hdl_cbk_rec_del(rec);
			TRACE("Handle is already finalized");
			break;
		}

		/* if we are done with this rec, free it */
		if ((rec->cbk_info->type != AVSV_AMF_PG_TRACK) && m_AVA_HDL_IS_CBK_RESP_DONE(rec)) {
			m_AVA_HDL_PEND_RESP_POP(list_resp, rec, rec->cbk_info->inv);
			ava_hdl_cbk_rec_del(rec);
		} else if (rec->cbk_info->type == AVSV_AMF_PG_TRACK) {
			/* PG Track cbk do not have any response */
			ava_hdl_cbk_rec_del(rec);
		} else {
			m_AVA_HDL_CBK_REC_IN_DISPATCH_RESET(rec);
		}

	} while (1);

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ava_hdl_cbk_dispatch_block
 
  Description   : This routine blocks forever for receiving indications from 
                  AvND. The routine returns when saAmfFinalize is executed on 
                  the handle.
 
  Arguments     : cb      - ptr to the AvA control block
                  hdl_rec - ptr to the handle record
 
  Return Values : SA_AIS_OK/SA_AIS_ERR_<CODE>
 
  Notes         : None
******************************************************************************/
uint32_t ava_hdl_cbk_dispatch_block(AVA_CB **cb, AVA_HDL_REC **hdl_rec)
{
	AVA_PEND_RESP *list_resp = &(*hdl_rec)->pend_resp;
	uint32_t hdl = (*hdl_rec)->hdl;
	AVA_PEND_CBK_REC *rec = 0;
	OsafAmfCallbacksT reg_cbk;
	uint32_t rc = SA_AIS_OK;
	TRACE_ENTER();

	memset(&reg_cbk, 0, sizeof(OsafAmfCallbacksT));
	memcpy(&reg_cbk, &(*hdl_rec)->reg_cbk, sizeof(OsafAmfCallbacksT));

	/* release all lock and handle - we are abt to go into deep sleep */
	m_NCS_UNLOCK(&(*cb)->lock, NCS_LOCK_WRITE);
	
	/* wait on the IPC mailbox for messages */
	while ((rec = (AVA_PEND_CBK_REC *)m_NCS_IPC_RECEIVE(&(*hdl_rec)->callbk_mbx, NULL)) != NULL) {
		ncshm_give_hdl(hdl);

		m_NCS_LOCK(&(*cb)->lock, NCS_LOCK_WRITE);

		/* get all handle and lock */
		*hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl);

		if (!(*hdl_rec)) {
			m_NCS_UNLOCK(&(*cb)->lock, NCS_LOCK_WRITE);
			TRACE_4("Select returned, unable to take handle");
			break;
		}

		if (rec->cbk_info->type != AVSV_AMF_PG_TRACK) {
			/* push this record into pending response list */
			m_AVA_HDL_PEND_RESP_PUSH(list_resp, (AVA_PEND_RESP_REC *)rec);
			m_AVA_HDL_CBK_REC_IN_DISPATCH_SET(rec);
		}

		/* release the cb lock & return the hdls to the hdl-mngr */
		ncshm_give_hdl(hdl);
		m_NCS_UNLOCK(&(*cb)->lock, NCS_LOCK_WRITE);

		/* process the callback list record */
		rc = ava_hdl_cbk_rec_prc(rec->cbk_info, &reg_cbk);

		/* take cb lock, so that any call to SaAmfResponce() will block */
		m_NCS_LOCK(&(*cb)->lock, NCS_LOCK_WRITE);

		*hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl);
		if (NULL == *hdl_rec) {
			ava_hdl_cbk_rec_del(rec);
			TRACE_4("Unable to retrieve handle record");
			m_NCS_UNLOCK(&(*cb)->lock, NCS_LOCK_WRITE);
			break;
		}

		/* if we are done with this rec, free it */
		if ((rec->cbk_info->type != AVSV_AMF_PG_TRACK) && m_AVA_HDL_IS_CBK_RESP_DONE(rec)) {
			m_AVA_HDL_PEND_RESP_POP(list_resp, rec, rec->cbk_info->inv);
			ava_hdl_cbk_rec_del(rec);
		} else if (rec->cbk_info->type == AVSV_AMF_PG_TRACK) {
			/* PG Track cbk do not have any response */
			ava_hdl_cbk_rec_del(rec);
		} else {
			m_AVA_HDL_CBK_REC_IN_DISPATCH_RESET(rec);
		}

		/* end of another critical section */
		m_NCS_UNLOCK(&(*cb)->lock, NCS_LOCK_WRITE);

	}			/*while */

	if (*hdl_rec)
		ncshm_give_hdl(hdl);

	m_NCS_LOCK(&(*cb)->lock, NCS_LOCK_WRITE);
	*hdl_rec = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, hdl);
	
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ava_hdl_cbk_rec_prc
 
  Description   : This routine invokes the registered callback routine.
 
  Arguments     : cb      - ptr to the AvA control block
                  hdl_rec - ptr to the hdl rec
                  cbk_rec - ptr to the callback record
 
  Return Values : None
 
  Notes         : - It may so happen that the callbacks that are dispatched may
                  finalize on the amf handle. Release AMF handle to the handle
                  manager before dispatching. Else Finalize blocks while 
                  destroying the association with handle manager.
                  - Since AMF supports LongDn, though AMF client still has
                  not supported LongDn, the callback comes may contain LongDn
                  SaNameT. AMF Agent needs to drop this callback so that AMF
                  client will not able to access LongDn SaNameT, otherwise
                  client could be crashed.
******************************************************************************/
uint32_t ava_hdl_cbk_rec_prc(AVSV_AMF_CBK_INFO *info, OsafAmfCallbacksT *reg_cbk)
{
	uint32_t rc = SA_AIS_OK;
	uint32_t i;
	TRACE_ENTER2("CallbackType = %d",info->type);

	/* invoke the corresponding callback */
	switch (info->type) {
		case AVSV_AMF_HC: {
			AVSV_AMF_HC_PARAM *hc = &info->param.hc;
			if (!ava_sanamet_is_valid(&hc->comp_name)) {
				rc = SA_AIS_ERR_NAME_TOO_LONG;
			}
			if (rc == SA_AIS_OK && reg_cbk->saAmfHealthcheckCallback) {
					TRACE("Invoking Healthcheck Callback:"
						"InvocationId = %llx, Component Name =  %s, Healthcheck Key = %s",
						info->inv, osaf_extended_name_borrow(&hc->comp_name), hc->hc_key.key);
					reg_cbk->saAmfHealthcheckCallback(info->inv, &hc->comp_name, &hc->hc_key);
			}

			break;
		}
		case AVSV_AMF_COMP_TERM: {
			AVSV_AMF_COMP_TERM_PARAM *comp_term = &info->param.comp_term;
			if (!ava_sanamet_is_valid(&comp_term->comp_name)) {
				rc = SA_AIS_ERR_NAME_TOO_LONG;
			}
			if (rc == SA_AIS_OK && reg_cbk->saAmfComponentTerminateCallback) {
					TRACE("Invoking component's saAmfComponentTerminateCallback: InvocationId = %llx,"
						 "component name = %s",info->inv, osaf_extended_name_borrow(&comp_term->comp_name));
					reg_cbk->saAmfComponentTerminateCallback(info->inv, &comp_term->comp_name);
			}
			break;
		}
		case AVSV_AMF_CSI_SET: {
			AVSV_AMF_CSI_SET_PARAM *csi_set = &info->param.csi_set;

			if (!ava_sanamet_is_valid(&csi_set->csi_desc.csiName) ||
				!ava_sanamet_is_valid(&csi_set->comp_name)) {
				rc = SA_AIS_ERR_NAME_TOO_LONG;
			}
			bool actv_or_stdby = true;
			if ((csi_set->ha == SA_AMF_HA_ACTIVE) && 
					(csi_set->csi_desc.csiStateDescriptor.activeDescriptor.transitionDescriptor != SA_AMF_CSI_NEW_ASSIGN)) {
				actv_or_stdby = ava_sanamet_is_valid(&csi_set->csi_desc.csiStateDescriptor.activeDescriptor.activeCompName);
			} else if (csi_set->ha == SA_AMF_HA_STANDBY) {
				actv_or_stdby = ava_sanamet_is_valid(&csi_set->csi_desc.csiStateDescriptor.standbyDescriptor.activeCompName);
			}
			if (!actv_or_stdby)
				rc = SA_AIS_ERR_NAME_TOO_LONG;

			if (rc == SA_AIS_OK && reg_cbk->saAmfCSISetCallback) {
				TRACE("CSISet: CSIName = %s, CSIFlags = %d, HA state = %d",
					osaf_extended_name_borrow(&csi_set->csi_desc.csiName),csi_set->csi_desc.csiFlags,csi_set->ha);
				if ((csi_set->ha == SA_AMF_HA_ACTIVE) && 
					(csi_set->csi_desc.csiStateDescriptor.activeDescriptor.transitionDescriptor != SA_AMF_CSI_NEW_ASSIGN))
					TRACE("CSISet: Active Transition Descriptor = %u, Active Component Name = %s",
							csi_set->csi_desc.csiStateDescriptor.activeDescriptor.transitionDescriptor,
							osaf_extended_name_borrow(&csi_set->csi_desc.csiStateDescriptor.activeDescriptor.activeCompName));
				if (csi_set->ha == SA_AMF_HA_STANDBY)
					TRACE("CSISet: ActiveCompName = %s, StandbyRank = %u",
							osaf_extended_name_borrow(&csi_set->csi_desc.csiStateDescriptor.standbyDescriptor.activeCompName),
							csi_set->csi_desc.csiStateDescriptor.standbyDescriptor.standbyRank);
				TRACE("Invoking component's saAmfCSISetCallback: InvocationId = %llx, component name = %s",
					info->inv,osaf_extended_name_borrow(&csi_set->comp_name));
				reg_cbk->saAmfCSISetCallback(info->inv,
					&csi_set->comp_name, csi_set->ha, csi_set->csi_desc);
			}

			break;
		}
		case AVSV_AMF_CSI_REM: {
			AVSV_AMF_CSI_REM_PARAM *csi_rem = &info->param.csi_rem;
			if (!ava_sanamet_is_valid(&csi_rem->comp_name) ||
				!ava_sanamet_is_valid(&csi_rem->csi_name)) {
				rc = SA_AIS_ERR_NAME_TOO_LONG;
			}
			if (rc == SA_AIS_OK && reg_cbk->saAmfCSIRemoveCallback) {
				TRACE("Invoking component's saAmfCSIRemoveCallback: InvocationId = %llx, component name = %s,"
					"CSIName = %s",info->inv,osaf_extended_name_borrow(&csi_rem->comp_name),
					osaf_extended_name_borrow(&csi_rem->csi_name));
				reg_cbk->saAmfCSIRemoveCallback(info->inv,
									&csi_rem->comp_name,
									&csi_rem->csi_name, csi_rem->csi_flags);
			}
			break;
		}
		case AVSV_AMF_PG_TRACK:	{
			AVSV_AMF_PG_TRACK_PARAM *pg_track = &info->param.pg_track;

			if(ava_B4_ver_used(0)) {
				SaAmfProtectionGroupNotificationBufferT_4 buf ={0};
				for (i = 0 ; i < pg_track->buf.numberOfItems ; i++) {
					if (!ava_sanamet_is_valid(&pg_track->buf.notification[i].member.compName)) {
						rc = SA_AIS_ERR_NAME_TOO_LONG;
						break;
					}
				}
				if (rc == SA_AIS_OK && reg_cbk->saAmfProtectionGroupTrackCallback_4) {
					TRACE("PG track Information: Total number of items in buffer = %d",pg_track->buf.numberOfItems);
					/* copy the contents into a malloced buffer.. appl frees it */
					buf.numberOfItems = pg_track->buf.numberOfItems;
					buf.notification =
						static_cast<SaAmfProtectionGroupNotificationT_4*>(malloc((buf.numberOfItems+1) * sizeof(SaAmfProtectionGroupNotificationT_4)));
					if (buf.notification) {
						ava_cpy_protection_group_ntf(buf.notification, pg_track->buf.notification,
								pg_track->buf.numberOfItems, SA_AMF_HARS_READY_FOR_ASSIGNMENT);

						/*
						 * NOTE: This buf->notification is allocated by Agent, it's
						 * added sentinel element at the end so that it helps
						 * to free LongDn in saAmfProtectionGroupNotificationFree_4
						 * Sentinel element has @.change = 0
						 */
						buf.notification[buf.numberOfItems].change = static_cast<SaAmfProtectionGroupChangesT>(0);
						TRACE("Invoking PGTrack callback for CSIName = %s", osaf_extended_name_borrow(&pg_track->csi_name));
						reg_cbk->saAmfProtectionGroupTrackCallback_4(&pg_track->csi_name,
													&buf,
													  pg_track->mem_num, pg_track->err);
					} else {
						pg_track->err = SA_AIS_ERR_NO_MEMORY;
						LOG_CR("Notification is NULL: Invoking PGTrack Callback with error SA_AIS_ERR_NO_MEMORY");
						reg_cbk->saAmfProtectionGroupTrackCallback(&pg_track->csi_name,
											   0, 0, pg_track->err);
					}
				}
			}
			else /* B01 version is used */
			{
				SaAmfProtectionGroupNotificationBufferT buf;
				for (i = 0 ; i < pg_track->buf.numberOfItems ; i++) {
					if (!ava_sanamet_is_valid(&pg_track->buf.notification[i].member.compName)) {
						rc = SA_AIS_ERR_NAME_TOO_LONG;
						break;
					}
				}
				if (rc == SA_AIS_OK && reg_cbk->saAmfProtectionGroupTrackCallback) {
					TRACE("PG track Information: Total number of items in buffer = %d",pg_track->buf.numberOfItems);
					/* copy the contents into a malloced buffer.. appl frees it */
					buf.numberOfItems = pg_track->buf.numberOfItems;
					buf.notification = 0;
					buf.notification =
						static_cast<SaAmfProtectionGroupNotificationT*>(malloc(buf.numberOfItems * sizeof(SaAmfProtectionGroupNotificationT)));
					if (buf.notification) {
						memcpy(buf.notification, pg_track->buf.notification,
							   buf.numberOfItems * sizeof(SaAmfProtectionGroupNotificationT));
						TRACE("Invoking PGTrack callback for CSIName = %s",
								osaf_extended_name_borrow(&pg_track->csi_name));

						/* allocate LongDn strings for notification if any
						 * then client needs to free these LongDn string as well
						 */
						for (i=0 ; i < buf.numberOfItems; i++) {
							osaf_extended_name_alloc(
								osaf_extended_name_borrow(&pg_track->buf.notification[i].member.compName),
								&buf.notification[i].member.compName);
						}

						((SaAmfCallbacksT *)reg_cbk)->saAmfProtectionGroupTrackCallback(&pg_track->csi_name,
											   &buf,
											   pg_track->mem_num, pg_track->err);
					} else {
						pg_track->err = SA_AIS_ERR_NO_MEMORY;
						LOG_CR("Notification is NULL: Invoking PGTrack Callback with error SA_AIS_ERR_NO_MEMORY");
						((SaAmfCallbacksT *)reg_cbk)->saAmfProtectionGroupTrackCallback(&pg_track->csi_name,
											   0, 0, pg_track->err);
					}
				}
			}
			break;
		}
		case AVSV_AMF_PXIED_COMP_INST: {
			AVSV_AMF_PXIED_COMP_INST_PARAM *comp_inst = &info->param.pxied_comp_inst;
			if (!ava_sanamet_is_valid(&comp_inst->comp_name)) {
				rc = SA_AIS_ERR_NAME_TOO_LONG;
			}
			if (rc == SA_AIS_OK && reg_cbk->saAmfProxiedComponentInstantiateCallback) {
				TRACE("Invoking proxiedcomponentInstantiateCbk: proxied component name = %s",
						osaf_extended_name_borrow(&comp_inst->comp_name));
				reg_cbk->saAmfProxiedComponentInstantiateCallback(info->inv, &comp_inst->comp_name);
			}
			break;
		}
		case AVSV_AMF_PXIED_COMP_CLEAN: {
			AVSV_AMF_PXIED_COMP_CLEAN_PARAM *comp_clean = &info->param.pxied_comp_clean;
			if (!ava_sanamet_is_valid(&comp_clean->comp_name)) {
				rc = SA_AIS_ERR_NAME_TOO_LONG;
			}
			if (rc == SA_AIS_OK && reg_cbk->saAmfProxiedComponentCleanupCallback) {
				TRACE("Invoking proxiedcomponentcleanupCbk: proxied component name = %s",
						osaf_extended_name_borrow(&comp_clean->comp_name));
				reg_cbk->saAmfProxiedComponentCleanupCallback(info->inv, &comp_clean->comp_name);
			}

			break;
		}
		case AVSV_AMF_CSI_ATTR_CHANGE: {
		       AVSV_AMF_CSI_ATTR_CHANGE_PARAM *csi_attr_change = &info->param.csi_attr_change;
		       if (!ava_sanamet_is_valid(&csi_attr_change->csi_name)) {
			 rc = SA_AIS_ERR_NAME_TOO_LONG;
		       }
                       if ((rc == SA_AIS_OK) && (reg_cbk->osafCsiAttributeChangeCallback)) {
                         TRACE("Invoking osafCsiAttributeChangeCallback : InvocationId = %llx,\
                           csi = %s",info->inv, osaf_extended_name_borrow(&csi_attr_change->csi_name));
			 reg_cbk->osafCsiAttributeChangeCallback(info->inv, &csi_attr_change->csi_name, csi_attr_change->csiAttr);
                       }
               }
               break;

		default:
			osafassert(0);
			break;
	}			/* switch */

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ava_hdl_pend_resp_pop 
 
  Description   : This routine pops a matching rec from list of pending 
                  responses.
 
  Arguments     : list  - ptr to list of pend resp in HDL_REC 
                  key   - invocation number of the callback 
 
  Return Values : ptr to AVA_PEND_RESP_REC 
 
  Notes         : None. 
******************************************************************************/
AVA_PEND_RESP_REC *ava_hdl_pend_resp_pop(AVA_PEND_RESP *list, SaInvocationT key)
{
	TRACE_ENTER();

	if (!((list)->head)) {
		osafassert(!((list)->num));
		TRACE_LEAVE();
		return NULL;
	} else {
		if (key == 0) {
			TRACE_LEAVE();
			return NULL;
		} else {
			AVA_PEND_RESP_REC *tmp_rec = list->head;
			AVA_PEND_RESP_REC *p_tmp_rec = NULL;
			while (tmp_rec != NULL) {
				/* Found a match */
				if (tmp_rec->cbk_info->inv == key) {
					/* if this is the last rec */
					if (list->tail == tmp_rec) {
						/* copy the prev rec to tail */
						list->tail = p_tmp_rec;

						if (list->tail == NULL)
							list->head = NULL;
						else
							list->tail->next = NULL;

						list->num--;
						TRACE_LEAVE();
						return tmp_rec;
					} else {
						/* if this is the first rec */
						if (list->head == tmp_rec)
							list->head = tmp_rec->next;
						else
							p_tmp_rec->next = tmp_rec->next;

						tmp_rec->next = NULL;
						list->num--;
						TRACE_LEAVE();
						return tmp_rec;

					}
				}
				/* move on to next element */
				p_tmp_rec = tmp_rec;
				tmp_rec = tmp_rec->next;
			}
			TRACE_LEAVE();
			return NULL;
		}		/* else */
	}
}

/****************************************************************************
  Name          : ava_hdl_pend_resp_get 
 
  Description   : This routine gets a matching rec from list of pending 
                  responses.
 
  Arguments     : list  - ptr to list of pend resp in HDL_REC 
                  key   - invocation number of the callback 
 
  Return Values : ptr to AVA_PEND_RESP_REC 
 
  Notes         : None. 
******************************************************************************/
AVA_PEND_RESP_REC *ava_hdl_pend_resp_get(AVA_PEND_RESP *list, SaInvocationT key)
{
	TRACE_ENTER();
	if (!((list)->head)) {
		osafassert(!((list)->num));
		TRACE_LEAVE();
		return NULL;
	} else {
		if (key == 0) {
			TRACE_LEAVE();
			return NULL;
		} else {
			/* parse thru the single list (head is diff from other elements) and
			 *  search for rec matching inv handle number. 
			 */
			AVA_PEND_RESP_REC *tmp_rec = list->head;
			while (tmp_rec != NULL) {
				if (tmp_rec->cbk_info->inv == key) {
					TRACE_LEAVE();
					return tmp_rec;
				}
				tmp_rec = tmp_rec->next;
			}
			TRACE_LEAVE();
			return NULL;
		}		/* else */
	}
}

/****************************************************************************
  Name          : ava_hdl_pend_resp_list_del
 
  Description   : This routine scans the pending response list & deletes all 
                  the records.
 
  Arguments     : cb   - ptr to the AvA control block
                  list - ptr to the pending response list
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
void ava_hdl_pend_resp_list_del(AVA_CB *cb, AVA_PEND_RESP *list)
{
	AVA_PEND_RESP_REC *rec = 0;
	TRACE_ENTER();

	/* pop & delete all the records */
	do {
		m_AVA_HDL_PEND_CBK_POP(list, rec);
		if (!rec)
			break;

		/* delete the record */
		if (!m_AVA_HDL_IS_CBK_REC_IN_DISPATCH(rec))
			ava_hdl_cbk_rec_del((AVA_PEND_CBK_REC *)rec);
	} while (1);

	/* there shouldn't be any record left */
	osafassert((!list->num) && (!list->head) && (!list->tail));
	TRACE_LEAVE();
	return;
}

/**
 * Initializes the client mailbox to process pending callbacks
 * @param hdl_rec
 *
 * @returns uint32_t 
 */
uint32_t ava_callback_ipc_init(AVA_HDL_REC *hdl_rec)
{       
	uint32_t rc = NCSCC_RC_SUCCESS;
	if ((rc = m_NCS_IPC_CREATE(&hdl_rec->callbk_mbx)) == NCSCC_RC_SUCCESS) {
		if (m_NCS_IPC_ATTACH(&hdl_rec->callbk_mbx) == NCSCC_RC_SUCCESS) {
			return NCSCC_RC_SUCCESS;
		}
		m_NCS_IPC_RELEASE(&hdl_rec->callbk_mbx, NULL);
	}
	return rc;
}

