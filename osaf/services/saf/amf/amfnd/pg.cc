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

  This file contains routines for processing PG events.

..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "avnd.h"

/* static function declarations */
static uint32_t avnd_pg_cbk_send(AVND_CB *, AVND_PG *, AVND_PG_TRK *, AVND_PG_MEM *);

static uint32_t avnd_pg_cbk_msg_send(AVND_CB *, AVND_PG_TRK *, AVSV_AMF_CBK_INFO *);

static uint32_t avnd_pg_track_stop(AVND_CB *, AVND_PG *);

static uint32_t avnd_pg_track_start(AVND_CB *, AVND_PG *, AVND_PG_TRK *);

static void avnd_pg_trk_rmv(AVND_CB *, AVND_PG *, AVND_PG_TRK_KEY *);

static uint32_t avnd_pg_start_rsp_prc(AVND_CB *cb, AVND_PG *pg, AVSV_D2N_PG_TRACK_ACT_RSP_MSG_INFO *info);

/****************************************************************************
  Name          : avnd_pg_start_rsp_prc
 
  Description   : This routine processes the pg reponce and gives apropriate
                  call-back.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t avnd_pg_start_rsp_prc(AVND_CB *cb, AVND_PG *pg, AVSV_D2N_PG_TRACK_ACT_RSP_MSG_INFO *info)
{
	AVND_PG_TRK *curr = 0, *prv = 0;
	uint32_t rc = NCSCC_RC_SUCCESS, i = 0;
	TRACE_ENTER();

	if (true == info->is_csi_exist) {	/* => +ve resp */
		/* set the exist flag to true */
		pg->is_exist = true;

		/* update the mem-list */
		osafassert(!pg->mem_list.n_nodes);
		for (i = 0; i < info->mem_list.numberOfItems; i++)
			avnd_pgdb_mem_rec_add(cb, pg, &info->mem_list.notification[i]);

		/* now send pending resp to all the appl */
		curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_FIRST(&pg->trk_list);
		while (curr) {
			prv = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_PREV(&curr->pg_dll_node);

			/* start pg tracking for this track rec */
			rc = avnd_pg_track_start(cb, pg, curr);
			if (NCSCC_RC_SUCCESS != rc)
				return rc;

			/* 
			 * Current flag is reset after sending the callback. 
			 * Check if other tracking is active. If not, delete this track record. 
			 */
			if (!curr->info.flags) {
				avnd_pgdb_trk_rec_del(cb, pg, &curr->info.key);
				curr = (prv) ? (AVND_PG_TRK *)m_NCS_DBLIST_FIND_NEXT(&prv->pg_dll_node) :
				    (AVND_PG_TRK *)m_NCS_DBLIST_FIND_FIRST(&pg->trk_list);
			} else
				curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_NEXT(&curr->pg_dll_node);
		}		/* while */

		/* if no other appl tracks this pg, stop tracking it */
		if (!pg->trk_list.n_nodes)
			rc = avnd_pg_track_stop(cb, pg);
	} else {		/* => -ve resp */

		/* send pending resp to all the appl */
		for (curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_FIRST(&pg->trk_list);
		     curr; curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_NEXT(&curr->pg_dll_node)) {
			rc = avnd_amf_resp_send(cb, AVSV_AMF_PG_START, SA_AIS_ERR_NOT_EXIST,
						0, &curr->info.key.mds_dest, &curr->info.mds_ctxt, NULL, false);
			if (NCSCC_RC_SUCCESS != rc)
				return rc;
		}		/* while */

		/* delete the pg rec */
		rc = avnd_pgdb_rec_del(cb, &pg->csi_name);
	}
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_ava_pg_start
 
  Description   : This routine processes the PG track start request from an 
                  application. It adds/updates the PG track record in the 
                  PG database. If PG tracking is already active on this node 
                  (by some other appln), the application is notified of the 
                  current members (if current flag is on). Else a request is
                  sent to AvD to start tracking this PG for this node.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_evt_ava_pg_start_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_AMF_API_INFO *api_info = &evt->info.ava.msg->info.api_info;
	AVSV_AMF_PG_START_PARAM *pg_start = &api_info->param.pg_start;
	AVND_PG_TRK_INFO trk_info;
	AVND_PG *pg = 0;
	AVND_PG_TRK *pg_trk = 0;
	bool is_fresh_pg = false;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* 
	 * Update pg db
	 */
	/* get the pg rec */
	pg = m_AVND_PGDB_REC_GET(cb->pgdb, pg_start->csi_name);

	/* if not found, add a new pg rec */
	if (!pg) {
		pg = avnd_pgdb_rec_add(cb, &pg_start->csi_name, &rc);
		if (NCSCC_RC_SUCCESS != rc)
			return rc;
		is_fresh_pg = true;
	}

	/* add/modify the track rec to the added/modified pg rec */
	trk_info.flags = pg_start->flags;
	trk_info.is_syn = pg_start->is_syn;
	trk_info.key.req_hdl = pg_start->hdl;
	trk_info.key.mds_dest = api_info->dest;
	trk_info.mds_ctxt = evt->mds_ctxt;
	pg_trk = avnd_pgdb_trk_rec_add(cb, pg, &trk_info);
	if (!pg_trk)
		return NCSCC_RC_FAILURE;

	/* 
	 * If fresh pg is created, send pg-start req to avd 
	 */
	if (true == is_fresh_pg) {
		rc = avnd_di_pg_act_send(cb, &pg_start->csi_name, AVSV_PG_TRACK_ACT_START, false);
		return rc;
	}

	/* 
	 * This isn't a fresh pg !!!
	 * If avd resp for pg-start is awaited, do nothing. 
	 * Else respond back to the application appropriately.
	 */
	if (true == pg->is_exist) {	/* => got a +ve resp from avd */
		/* start pg tracking for this track rec */
		rc = avnd_pg_track_start(cb, pg, pg_trk);
		if (NCSCC_RC_SUCCESS != rc)
			return rc;

		/* 
		 * Current flag is reset after sending the callback. 
		 * Check if other tracking is active. If not, delete this track record. 
		 */
		if (!pg_trk->info.flags) {
			avnd_pgdb_trk_rec_del(cb, pg, &pg_trk->info.key);

			/* if no other appl tracks this pg, stop tracking it */
			if (!pg->trk_list.n_nodes)
				rc = avnd_pg_track_stop(cb, pg);
		}
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_ava_pg_stop
 
  Description   : This routine processes the PG track stop request from an 
                  application. The corresponding track record is deleted from 
                  the PG Db. If there is no other application tracking this PG,
                  a stop request is sent to AvD. AvD then excludes this node 
                  from any further updates for this PG.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_evt_ava_pg_stop_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_AMF_API_INFO *api_info = &evt->info.ava.msg->info.api_info;
	AVSV_AMF_PG_STOP_PARAM *pg_stop = &api_info->param.pg_stop;
	AVND_PG_TRK_KEY key;
	AVND_PG *pg = 0;
	AVND_PG_TRK *pg_trk = 0;
	SaAisErrorT amf_rc = SA_AIS_OK;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* populate the track key */
	key.mds_dest = api_info->dest;
	key.req_hdl = pg_stop->hdl;

	/* get the pg & pg-track rec */
	pg = m_AVND_PGDB_REC_GET(cb->pgdb, pg_stop->csi_name);
	if (pg)
		pg_trk = m_AVND_PGDB_TRK_REC_GET(*pg, key);

	/* respond bk to the application */
	if (!pg || !pg_trk)
		amf_rc = SA_AIS_ERR_NOT_EXIST;
	rc = avnd_amf_resp_send(cb, AVSV_AMF_PG_STOP, amf_rc, 0, &api_info->dest, &evt->mds_ctxt, NULL, false);

	/* proceed with rest of the processing */
	if ((SA_AIS_OK == amf_rc) && (NCSCC_RC_SUCCESS == rc)) {
		/* delete the track rec */
		avnd_pgdb_trk_rec_del(cb, pg, &key);

		/* if no other appl tracks this pg, stop tracking it */
		if (!pg->trk_list.n_nodes)
			rc = avnd_pg_track_stop(cb, pg);
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_process_pg_track_start_rsp_on_fover
 
  Description   : This routine processes the AvD response to the PG track 
                  start request sent on the fail-over to the AVD.
 
  Arguments     : cb  - ptr to the AvND control block
                  info - ptr to the AvND event response.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t avnd_process_pg_track_start_rsp_on_fover(AVND_CB *cb, AVND_PG *pg,
						      AVSV_D2N_PG_TRACK_ACT_RSP_MSG_INFO *info)
{
	AVND_PG_TRK *curr = 0;
	uint32_t rc = NCSCC_RC_SUCCESS, i = 0;
	AVND_PG_MEM *pg_mem = 0, *mem_curr = 0, *mem_prv = 0;
	SaAmfProtectionGroupNotificationT *mem_info;
	TRACE_ENTER2("%u", info->is_csi_exist);

	if (true == info->is_csi_exist) {	/* => +ve resp */
		/* Walk through the list */
		for (i = 0; i < info->mem_list.numberOfItems; i++) {
			mem_info = &info->mem_list.notification[i];
			/* get the mem rec */
			pg_mem = m_AVND_PGDB_MEM_REC_GET(*pg, mem_info->member.compName);

			if (NULL == pg_mem) {
				pg_mem = avnd_pgdb_mem_rec_add(cb, pg, mem_info);
			} else {
				pg_mem->mem_exist = true;
				/* Compare the data and give call back of there is mis-match */
				if (pg_mem->info.member.haState != mem_info->member.haState) {
					pg_mem->info.member.haState = mem_info->member.haState;
					pg_mem->info.change = SA_AMF_PROTECTION_GROUP_STATE_CHANGE;
				} else
					continue;
			}

			pg_mem->mem_exist = true;

			/* inform all the appl */
			for (curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_FIRST(&pg->trk_list);
			     curr; curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_NEXT(&curr->pg_dll_node)) {
				rc = avnd_pg_cbk_send(cb, pg, curr, pg_mem);
				if (NCSCC_RC_SUCCESS != rc)
					return rc;
			}	/* for */
		}

		/* Now find out for which mem we have to send remove */
		mem_curr = (AVND_PG_MEM *)m_NCS_DBLIST_FIND_FIRST(&pg->mem_list);
		while (mem_curr) {
			mem_prv = (AVND_PG_MEM *)m_NCS_DBLIST_FIND_PREV(&mem_curr->pg_dll_node);

			if (true == mem_curr->mem_exist) {
				mem_curr->mem_exist = false;
				mem_curr = (AVND_PG_MEM *)m_NCS_DBLIST_FIND_NEXT(&mem_curr->pg_dll_node);
				continue;
			} else {
				pg_mem = avnd_pgdb_mem_rec_rmv(cb, pg, &mem_curr->info.member.compName);

				mem_curr =
				    (mem_prv) ? (AVND_PG_MEM *)m_NCS_DBLIST_FIND_NEXT(&mem_prv->pg_dll_node) :
				    (AVND_PG_MEM *)m_NCS_DBLIST_FIND_FIRST(&pg->mem_list);
			}

			/* We are here means some member is removed and we need to send call backs */
			for (curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_FIRST(&pg->trk_list); curr;
			     curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_NEXT(&curr->pg_dll_node)) {
				rc = avnd_pg_cbk_send(cb, pg, curr, pg_mem);
				if (NCSCC_RC_SUCCESS != rc)
					return rc;
			}	/* for */

			delete pg_mem;
		}
	} else {
		/* => this update is for csi deletion */
		pg->is_exist = false;

		/* inform all the appl */
		for (curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_FIRST(&pg->trk_list);
		     curr; curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_NEXT(&curr->pg_dll_node)) {
			rc = avnd_pg_cbk_send(cb, pg, curr, 0);
			if (NCSCC_RC_SUCCESS != rc)
				return rc;
		}		/* for */

		/* delete pg rec */
		rc = avnd_pgdb_rec_del(cb, &pg->csi_name);
	}
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_avd_pg_track_act_rsp_msg
 
  Description   : This routine processes the AvD response to the PG track 
                  start/stop request. The response to the PG start request 
                  contains either the list of current PG members (ie. comps) 
                  or whether the CSI exists in the cluster.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_evt_avd_pg_track_act_rsp_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_D2N_PG_TRACK_ACT_RSP_MSG_INFO *info = &evt->info.avd->msg_info.d2n_pg_track_act_rsp;
	AVND_PG *pg = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("Actn'%u'", info->actn);

	/* dont process unless AvD is up */
	if (!m_AVND_CB_IS_AVD_UP(cb))
		return rc;

	switch (info->actn) {
	case AVSV_PG_TRACK_ACT_START:
		{
			/* process the ack response */
			avnd_di_msg_ack_process(cb, info->msg_id_ack);

			/* get the pg rec */
			pg = m_AVND_PGDB_REC_GET(cb->pgdb, info->csi_name);
			TRACE("pg '%p', msg_on_fover '%u'", pg, info->msg_on_fover);
			if (true == info->msg_on_fover) {
				if (NULL != pg) {
					if (false == pg->is_exist)
						return avnd_pg_start_rsp_prc(cb, pg, info);
					else
						rc = avnd_process_pg_track_start_rsp_on_fover(cb, pg, info);
				}
				return rc;
			}

			if (!pg)
				return rc;

			osafassert(false == pg->is_exist);

			rc = avnd_pg_start_rsp_prc(cb, pg, info);

		}
		break;

	case AVSV_PG_TRACK_ACT_STOP:
		{
			/* process the ack response */
			avnd_di_msg_ack_process(cb, info->msg_id_ack);
		}
		break;

	default:
		osafassert(0);
	}			/* switch */

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_avd_pg_upd_msg
 
  Description   : This routine processes PG update message from AvD. It 
                  indicates the changes in the PG membership and also 
                  indicates if the corresponding CSI is administratively 
                  deleted from the cluster.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_evt_avd_pg_upd_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_D2N_PG_UPD_MSG_INFO *info = &evt->info.avd->msg_info.d2n_pg_upd;
	AVND_PG *pg = 0;
	AVND_PG_TRK *curr = 0;
	AVND_PG_MEM *chg_mem = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("is_csi_del=%u", info->is_csi_del);

	/* dont process unless AvD is up */
	if (!m_AVND_CB_IS_AVD_UP(cb))
		return rc;

	/* get the pg-rec */
	pg = m_AVND_PGDB_REC_GET(cb->pgdb, info->csi_name);
	if (!pg)
		return NCSCC_RC_FAILURE;

	osafassert(true == pg->is_exist);
	TRACE("is_csi_del '%u'", info->is_csi_del);
	if (true == info->is_csi_del) {
		/* => this update is for csi deletion */

		pg->is_exist = false;

		/* inform all the appl */
		for (curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_FIRST(&pg->trk_list);
		     curr; curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_NEXT(&curr->pg_dll_node)) {
			rc = avnd_pg_cbk_send(cb, pg, curr, 0);
			if (NCSCC_RC_SUCCESS != rc)
				return rc;
		}		/* for */

		/* delete pg rec */
		rc = avnd_pgdb_rec_del(cb, &pg->csi_name);
	} else {
		/* => this update is for csi updation */

		/* update the pg mem-list */
		TRACE("info->mem.change '%u'", info->mem.change);
		switch (info->mem.change) {
		case SA_AMF_PROTECTION_GROUP_ADDED:
		case SA_AMF_PROTECTION_GROUP_STATE_CHANGE:
			chg_mem = avnd_pgdb_mem_rec_add(cb, pg, &info->mem);
			break;

		case SA_AMF_PROTECTION_GROUP_REMOVED:
			chg_mem = avnd_pgdb_mem_rec_rmv(cb, pg, &info->mem.member.compName);
			break;

		default:
			osafassert(0);
		}		/* switch */

		if (NULL == chg_mem)
			return NCSCC_RC_FAILURE;

		/* inform all the appl */
		for (curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_FIRST(&pg->trk_list);
		     curr; curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_NEXT(&curr->pg_dll_node)) {
			rc = avnd_pg_cbk_send(cb, pg, curr, chg_mem);
			if (NCSCC_RC_SUCCESS != rc)
				return rc;
		}		/* for */

		/* we are done with sending cbk, mark the chnage field as no change */
		if (SA_AMF_PROTECTION_GROUP_REMOVED != chg_mem->info.change)
			chg_mem->info.change = SA_AMF_PROTECTION_GROUP_NO_CHANGE;

		/* if the member was previously removed, free it now */
		if (SA_AMF_PROTECTION_GROUP_REMOVED == chg_mem->info.change)
			delete chg_mem;
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_pg_finalize
 
  Description   : This routine removes all the PG track records that share 
                  the specified AMF handle & the mds-dest. It is invoked when
                  the application invokes saAmfFinalize for a certain handle
                  or AvA down event is received.
 
  Arguments     : cb   - ptr to the AvND control block
                  hdl  - amf-handle
                  dest - ptr to mds-dest (of the prc that finalizes)
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avnd_pg_finalize(AVND_CB *cb, SaAmfHandleT hdl, MDS_DEST *dest)
{
	AVND_PG_TRK_KEY key;
	AVND_PG *curr = 0, *prv = 0;
	TRACE_ENTER();

	/* populate the key */
	key.mds_dest = *dest;
	key.req_hdl = hdl;

	curr = (AVND_PG *)ncs_patricia_tree_getnext(&cb->pgdb, (uint8_t *)0);

	/* scan the entire pg db & delete the matching track recs */
	while (curr) {
		/* delete the matching track recs, if any */
		avnd_pg_trk_rmv(cb, curr, &key);

		/* if no other appl tracks this pg, stop tracking it */
		if (!curr->trk_list.n_nodes) {
			avnd_pg_track_stop(cb, curr);
			curr = (prv) ? m_AVND_PGDB_REC_GET_NEXT(cb->pgdb, prv->csi_name) :
			    (AVND_PG *)ncs_patricia_tree_getnext(&cb->pgdb, (uint8_t *)0);
		} else {
			prv = curr;
			curr = m_AVND_PGDB_REC_GET_NEXT(cb->pgdb, curr->csi_name);
		}
	}			/* while */

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : avnd_pg_trk_rmv
 
  Description   : This routine deletes the PG track records for the specified 
                  PG. If both mds-dest & amf-handle are specified in the key,
                  only the corresponding track record is deleted (if found). 
                  Else only mds-dest is available in the key and all the 
                  track records sharing this mds-dest are deleted. It is 
                  invoked when the application invokes saAmfFinalize for a 
                  certain handle or AvA down event is received.
 
  Arguments     : cb  - ptr to the AvND control block
                  pg  - ptr to the pg rec
                  key - ptr to the pg track key
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avnd_pg_trk_rmv(AVND_CB *cb, AVND_PG *pg, AVND_PG_TRK_KEY *key)
{
	AVND_PG_TRK *curr = 0, *prv = 0;
	TRACE_ENTER();

	if (key->req_hdl)
		/* => both mds-dest & hdl are specified.. delete the exact entry */
		avnd_pgdb_trk_rec_del(cb, pg, key);
	else {
		/* => only mds-dest is specified.. delete all the matching entries */
		curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_FIRST(&pg->trk_list);
		while (curr) {
			if (0 == memcmp(&curr->info.key.mds_dest, &key->mds_dest, sizeof(MDS_DEST))) {
				avnd_pgdb_trk_rec_del(cb, pg, &curr->info.key);
				curr = (prv) ? (AVND_PG_TRK *)m_NCS_DBLIST_FIND_NEXT(&prv->pg_dll_node) :
				    (AVND_PG_TRK *)m_NCS_DBLIST_FIND_FIRST(&pg->trk_list);
			} else {
				prv = curr;
				curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_NEXT(&curr->pg_dll_node);
			}
		}		/* while */
	}

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : avnd_pg_track_start
 
  Description   : This routine is a wrapper routine to start PG tracking 
                  for the specified PG track record. It responds back to the
                  application & invokes PG callabck if current flag is active. 
 
  Arguments     : cb - ptr to the AvND control block
                  pg - ptr to the pg rec
                  pg_trk - ptr to the pg track rec
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_pg_track_start(AVND_CB *cb, AVND_PG *pg, AVND_PG_TRK *pg_trk)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* send the response to the application */
	if (false == pg_trk->info.is_syn) {
		rc = avnd_amf_resp_send(cb, AVSV_AMF_PG_START, SA_AIS_OK,
					0, &pg_trk->info.key.mds_dest, &pg_trk->info.mds_ctxt, NULL, false);
		if (NCSCC_RC_SUCCESS != rc)
			return rc;
	}

	/* send the current members if current flag is on */
	if (m_AVND_PG_TRK_IS_CURRENT(pg_trk)) {
		rc = avnd_pg_cbk_send(cb, pg, pg_trk, 0);

		/* reset the current flag */
		m_AVND_PG_TRK_CURRENT_RESET(pg_trk);
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_pg_track_stop
 
  Description   : This routine is a wrapper routine to stop PG processing 
                  for the specified PG. It sends a stop request to AvD & then
                  deletes the pg record. This routine is triggered from 
                  various sources: saAmfPGStop API, saAmfFinalize, PG start 
                  response from AvD etc.
 
  Arguments     : cb - ptr to the AvND control block
                  pg - ptr to the pg rec
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_pg_track_stop(AVND_CB *cb, AVND_PG *pg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("%s",  pg->csi_name.value);

	/* send pg-stop req to avd */
	rc = avnd_di_pg_act_send(cb, &pg->csi_name, AVSV_PG_TRACK_ACT_STOP, false);
	if (NCSCC_RC_SUCCESS != rc) {
		TRACE("avnd_di_pg_act_send failed for = %s", pg->csi_name.value);
	}

	/* delete the pg rec */
	rc = avnd_pgdb_rec_del(cb, &pg->csi_name);

	TRACE_LEAVE2("rc '%u'", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_pg_cbk_send
 
  Description   : This routine prepares & sends the PG callback parameters to
                  the tracking application. The callback parameters depend on
                  the track flags.
 
  Arguments     : cb      - ptr to the AvND control block
                  pg      - ptr to the pg rec
                  trk     - ptr to the pg track rec
                  chg_mem - ptr to the member that is added/remove/modified (it 
                            is null if there's no change)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_pg_cbk_send(AVND_CB *cb, AVND_PG *pg, AVND_PG_TRK *trk, AVND_PG_MEM *chg_mem)
{
	AVND_PG_MEM *curr_mem = 0;
	AVSV_AMF_CBK_INFO *cbk_info = 0;
	AVSV_AMF_PG_TRACK_PARAM *pg_param = 0;
	uint32_t i = 0, rc = NCSCC_RC_SUCCESS;
	uint32_t number_of_items = 0;
	TRACE_ENTER();

	/* allocate cbk-info */
	cbk_info = new AVSV_AMF_CBK_INFO();

	pg_param = &cbk_info->param.pg_track;

	/* fill the common params */
	cbk_info->hdl = trk->info.key.req_hdl;
	cbk_info->type = AVSV_AMF_PG_TRACK;
	pg_param->csi_name = pg->csi_name;
	pg_param->mem_num = pg->mem_list.n_nodes;
	pg_param->err = SA_AIS_OK;	/* default err code */
	number_of_items = pg->mem_list.n_nodes;

	/* 
	 * Now fill other cbk params.
	 */
	if (true == pg->is_exist) {
		/* => this csi exists... invoke the cbk as per the track flags */
		if ((m_AVND_PG_TRK_IS_CURRENT(trk) || m_AVND_PG_TRK_IS_CHANGES(trk)) && (pg_param->mem_num)) {
	 /*** include all the current members ***/

			/* we need to include the comp which just got removed from group */
			if (chg_mem && (chg_mem->info.change == SA_AMF_PROTECTION_GROUP_REMOVED)
			    && m_AVND_PG_TRK_IS_CHANGES(trk))
				number_of_items = number_of_items + 1;

			/* allocate the buffer */
			pg_param->buf.notification = new SaAmfProtectionGroupNotificationT[number_of_items]();

			/* fill all the current members */
			for (curr_mem = (AVND_PG_MEM *)m_NCS_DBLIST_FIND_FIRST(&pg->mem_list), i = 0;
			     curr_mem; curr_mem = (AVND_PG_MEM *)m_NCS_DBLIST_FIND_NEXT(&curr_mem->pg_dll_node), i++) {
				pg_param->buf.notification[i] = curr_mem->info;
				if (chg_mem && (curr_mem != chg_mem))
					pg_param->buf.notification[i].change = SA_AMF_PROTECTION_GROUP_NO_CHANGE;
				pg_param->buf.numberOfItems++;
			}	/* for */

			/* if some comp was just removed, include him also */
			if (chg_mem && (chg_mem->info.change == SA_AMF_PROTECTION_GROUP_REMOVED)
			    && m_AVND_PG_TRK_IS_CHANGES(trk)) {
				pg_param->buf.notification[i] = chg_mem->info;
				pg_param->buf.numberOfItems++;
			}

		}

		if (chg_mem && m_AVND_PG_TRK_IS_CHANGES_ONLY(trk)) {
	 /*** include only the modified member ***/
			pg_param->buf.notification = new SaAmfProtectionGroupNotificationT[1];

			*pg_param->buf.notification = chg_mem->info;
			pg_param->buf.numberOfItems = 1;
		}
	} else
		/* => this csi ceases to exist */
		pg_param->err = SA_AIS_ERR_NOT_EXIST;

	/* now send the cbk msg */
	rc = avnd_pg_cbk_msg_send(cb, trk, cbk_info);

	/* reset the is_syn flag */
	trk->info.is_syn = false;

 	if ((NCSCC_RC_SUCCESS != rc) && cbk_info)
		amf_cbk_free(cbk_info);

	TRACE_LEAVE2("rc '%u'", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_pg_cbk_msg_send
 
  Description   : This routine sends the PG callback parameters to the 
                  tracking application.
 
  Arguments     : cb       - ptr to the AvND control block
                  trk      - ptr to the pg track rec
                  cbk_info - ptr to the cbk-info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None
******************************************************************************/
uint32_t avnd_pg_cbk_msg_send(AVND_CB *cb, AVND_PG_TRK *trk, AVSV_AMF_CBK_INFO *cbk_info)
{
	AVND_MSG msg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&msg, 0, sizeof(AVND_MSG));

	/* allocate ava message */
	msg.info.ava = new AVSV_NDA_AVA_MSG();

	/* populate the msg */
	msg.type = AVND_MSG_AVA;
	msg.info.ava->type = AVSV_AVND_AMF_CBK_MSG;
	msg.info.ava->info.cbk_info = cbk_info;

	/* send the message to AvA */
	rc = avnd_mds_send(cb, &msg, &trk->info.key.mds_dest,
			   ((m_AVND_PG_TRK_IS_CURRENT(trk)) && (true == trk->info.is_syn)) ? &trk->info.mds_ctxt : 0);
	if (NCSCC_RC_SUCCESS == rc)
		msg.info.ava = 0;

 	/* free the contents of avnd message */
	avnd_msg_content_free(cb, &msg);
	TRACE_LEAVE2("rc '%u'", rc);
	return rc;
}
