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

  This file contains component health check routines.
 
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "avnd.h"

/* macro to determine if the healthcheck is comp-initiated */
#define m_AVND_COMP_HC_REC_IS_COMP_INITIATED(rec) \
            (rec->inv == SA_AMF_HEALTHCHECK_COMPONENT_INVOKED)

/* macro to determine if the healthcheck is amf-initiated */
#define m_AVND_COMP_HC_REC_IS_AMF_INITIATED(rec) \
            (rec->inv == SA_AMF_HEALTHCHECK_AMF_INVOKED)

/*** static function declarations */
static void avnd_comp_hc_param_val(AVND_CB *, AVSV_AMF_API_TYPE, uint8_t *,
				   AVND_COMP **, AVND_COMP_HC_REC **, SaAisErrorT *);

static AVND_COMP_HC_REC *avnd_comp_hc_rec_add(AVND_CB *, AVND_COMP *, AVSV_AMF_HC_START_PARAM *, MDS_DEST *);
static void avnd_comp_hc_rec_del(AVND_CB *, AVND_COMP *, AVND_COMP_HC_REC *);

static uint32_t avnd_comp_hc_rec_process(AVND_CB *, AVND_COMP *, AVND_COMP_HC_REC *, AVND_COMP_HC_OP, SaAisErrorT);

static uint32_t avnd_comp_hc_rec_stop(AVND_CB *, AVND_COMP *, AVND_COMP_HC_REC *);

static uint32_t avnd_comp_hc_rec_confirm(AVND_CB *, AVND_COMP *, AVND_COMP_HC_REC *, SaAisErrorT);

static uint32_t avnd_comp_hc_rec_tmr_exp(AVND_CB *, AVND_COMP *, AVND_COMP_HC_REC *);

/* the default fixed "magic" name for a command based health object */
static const char *hc_cmd_name = "osafHealthCheck";

/****************************************************************************
  Name          : avnd_evt_ava_hc_start
 
  Description   : This routine processes the healthcheck start message from 
                  AvA.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_evt_ava_hc_start_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_AMF_API_INFO *api_info = &evt->info.ava.msg->info.api_info;
	AVSV_AMF_HC_START_PARAM *hc_start = &api_info->param.hc_start;
	AVND_COMP *comp = 0;
	AVND_COMP_HC_REC *rec = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAisErrorT amf_rc = SA_AIS_OK;
	bool msg_from_avnd = false, int_ext_comp = false;

	TRACE_ENTER();

	if (AVND_EVT_AVND_AVND_MSG == evt->type) {
		/* This means that the message has come from proxy AvND to this AvND. */
		msg_from_avnd = true;
	}

	if (false == msg_from_avnd) {
		/* Check for internode or external coomponent first
		   If it is, then forward it to the respective AvND. */
		rc = avnd_int_ext_comp_hdlr(cb, api_info, &evt->mds_ctxt, &amf_rc, &int_ext_comp);
		if (true == int_ext_comp) {
			goto done;
		}
	}
	/* validate the healthcheck start message */
	avnd_comp_hc_param_val(cb, AVSV_AMF_HC_START, (uint8_t *)hc_start, &comp, 0, &amf_rc);
	TRACE_1("sending response:%d",amf_rc);
	/* send the response back to AvA */
	rc = avnd_amf_resp_send(cb, AVSV_AMF_HC_START, amf_rc, 0, &api_info->dest, &evt->mds_ctxt, comp, msg_from_avnd);

	/* now proceeed with the rest of the processing */
	if ((SA_AIS_OK == amf_rc) && (NCSCC_RC_SUCCESS == rc)) {
		if ((0 != (rec = avnd_comp_hc_rec_add(cb, comp, hc_start, &api_info->dest)))) {
			rc = avnd_comp_hc_rec_process(cb, comp, rec, AVND_COMP_HC_START, static_cast<SaAisErrorT>(0));
		} else
			rc = NCSCC_RC_FAILURE;
	}

 done:
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("avnd_evt_ava_hc_start():%s:Hdl=%llx,InvType: %u, Err Rcvr:%u",\
				    hc_start->comp_name.value, hc_start->hdl, hc_start->inv_type, hc_start->rec_rcvr.raw);
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_ava_hc_stop
 
  Description   : This routine processes the healthcheck stop message from 
                  AvA.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_evt_ava_hc_stop_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_AMF_API_INFO *api_info = &evt->info.ava.msg->info.api_info;
	AVSV_AMF_HC_STOP_PARAM *hc_stop = &api_info->param.hc_stop;
	AVND_COMP *comp = 0;
	AVND_COMP_HC_REC *rec = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAisErrorT amf_rc = SA_AIS_OK;
	bool msg_from_avnd = false, int_ext_comp = false;

	TRACE_ENTER();

	if (AVND_EVT_AVND_AVND_MSG == evt->type) {
		/* This means that the message has come from proxy AvND to this AvND. */
		msg_from_avnd = true;
	}

	if (false == msg_from_avnd) {
		/* Check for internode or external coomponent first
		   If it is, then forward it to the respective AvND. */
		rc = avnd_int_ext_comp_hdlr(cb, api_info, &evt->mds_ctxt, &amf_rc, &int_ext_comp);
		if (true == int_ext_comp) {
			goto done;
		}
	}

	/* validate the healthcheck stop message */
	avnd_comp_hc_param_val(cb, AVSV_AMF_HC_STOP, (uint8_t *)hc_stop, &comp, &rec, &amf_rc);

	/* send the response back to AvA */
	rc = avnd_amf_resp_send(cb, AVSV_AMF_HC_STOP, amf_rc, 0, &api_info->dest, &evt->mds_ctxt, comp, msg_from_avnd);

	/* stop the healthcheck */
	if ((SA_AIS_OK == amf_rc) && (NCSCC_RC_SUCCESS == rc))
		rc = avnd_comp_hc_rec_process(cb, comp, rec, AVND_COMP_HC_STOP, static_cast<SaAisErrorT>(0));

 done:
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("avnd_evt_ava_hc_stop():%s : Hdl:%llx",hc_stop->comp_name.value, hc_stop->hdl);
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_ava_hc_confirm
 
  Description   : This routine processes the healthcheck confirm message from 
                  AvA.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_evt_ava_hc_confirm_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_AMF_API_INFO *api_info = &evt->info.ava.msg->info.api_info;
	AVSV_AMF_HC_CONFIRM_PARAM *hc_confirm = &api_info->param.hc_confirm;
	AVND_COMP *comp = 0;
	AVND_COMP_HC_REC *rec = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAisErrorT amf_rc = SA_AIS_OK;
	bool msg_from_avnd = false, int_ext_comp = false;

	TRACE_ENTER();

	if (AVND_EVT_AVND_AVND_MSG == evt->type) {
		/* This means that the message has come from proxy AvND to this AvND. */
		msg_from_avnd = true;
	}

	if (false == msg_from_avnd) {
		/* Check for internode or external coomponent first
		   If it is, then forward it to the respective AvND. */
		rc = avnd_int_ext_comp_hdlr(cb, api_info, &evt->mds_ctxt, &amf_rc, &int_ext_comp);
		if (true == int_ext_comp) {
			goto done;
		}
	}

	/* validate the healthcheck confirm message */
	avnd_comp_hc_param_val(cb, AVSV_AMF_HC_CONFIRM, (uint8_t *)hc_confirm, &comp, &rec, &amf_rc);

	/* send the response back to AvA */
	rc = avnd_amf_resp_send(cb, AVSV_AMF_HC_CONFIRM, amf_rc, 0,
				&api_info->dest, &evt->mds_ctxt, comp, msg_from_avnd);

	/* healthcheck confirm processing */
	if ((SA_AIS_OK == amf_rc) && (NCSCC_RC_SUCCESS == rc))
		rc = avnd_comp_hc_rec_process(cb, comp, rec, AVND_COMP_HC_CONFIRM, hc_confirm->hc_res);

 done:
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("avnd_evt_ava_hc_confirm():%s: Hdl=%llx, hc_res=%u",\
				    hc_confirm->comp_name.value, hc_confirm->hdl, hc_confirm->hc_res);
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_tmr_hc
 
  Description   : This routine processes the healthcheck timer expiry.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_evt_tmr_hc_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_TMR_EVT *tmr = &evt->info.tmr;
	AVND_COMP_HC_REC *rec = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* retrieve the healthcheck record */
	if ((0 == (rec = (AVND_COMP_HC_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVND, tmr->opq_hdl))))
		goto done;

	/* 
	 * the record may be deleted as a part of the expiry processing. 
	 * hence returning the record to the hdl mngr.
	 */
	ncshm_give_hdl(tmr->opq_hdl);

	if (NCSCC_RC_SUCCESS == m_AVND_CHECK_FOR_STDBY_FOR_EXT_COMP(cb, rec->comp->su->su_is_external))
		goto done;

	/* process the timer expiry */
	rc = avnd_comp_hc_rec_process(cb, rec->comp, rec, AVND_COMP_HC_TMR_EXP, static_cast<SaAisErrorT>(0));

done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_comp_hc_param_val
 
  Description   : This routine validates the component healthcheck start/stop
                  & confirm parameters.
 
  Arguments     : cb       - ptr to the AvND control block
                  api_type - healthcheck API
                  param    - ptr to the healthcheck start/stop/confirm params
                  o_comp   - double ptr to the comp (o/p)
                  o_hc_rec - double ptr to the comp healthcheck record (o/p)
                  o_amf_rc - double ptr to the amf-rc (o/p)
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
void avnd_comp_hc_param_val(AVND_CB *cb,
			    AVSV_AMF_API_TYPE api_type,
			    uint8_t *param, AVND_COMP **o_comp, AVND_COMP_HC_REC **o_hc_rec, SaAisErrorT *o_amf_rc)
{
	*o_amf_rc = SA_AIS_OK;
	AVSV_HLT_KEY hlt_chk;
	uint32_t l_num;
	AVND_COMP_HC_REC tmp_hc_rec;

	switch (api_type) {
	case AVSV_AMF_HC_START:
		{
			AVSV_AMF_HC_START_PARAM *hc_start;

			hc_start = (AVSV_AMF_HC_START_PARAM *)param;

			/* get the comp */
			if (0 == (*o_comp = m_AVND_COMPDB_REC_GET(cb->compdb, hc_start->comp_name))) {
				*o_amf_rc = SA_AIS_ERR_NOT_EXIST;
				return;
			}

			/* non-existing component should not interact with AMF */
			if (m_AVND_COMP_PRES_STATE_IS_UNINSTANTIATED(*o_comp) ||
			    m_AVND_COMP_PRES_STATE_IS_INSTANTIATIONFAILED(*o_comp) ||
			    m_AVND_COMP_PRES_STATE_IS_TERMINATING(*o_comp) ||
			    m_AVND_COMP_PRES_STATE_IS_TERMINATIONFAILED(*o_comp)) {
				*o_amf_rc = SA_AIS_ERR_TRY_AGAIN;
				return;
			}

			/* non proxy component should not start/stop/confirm health check for any other component */
			if (m_CMP_HORDER_SANAMET(hc_start->comp_name, hc_start->proxy_comp_name)) {
				if (!m_AVND_COMP_TYPE_IS_PROXIED(*o_comp)
				    || m_CMP_HORDER_SANAMET((*o_comp)->pxy_comp->name,
							    hc_start->proxy_comp_name)) {
					*o_amf_rc = SA_AIS_ERR_NOT_EXIST;
					return;
				}
			}
			memset(&hlt_chk, 0, sizeof(AVSV_HLT_KEY));
			hlt_chk.comp_name.length = hc_start->comp_name.length;
			memcpy(hlt_chk.comp_name.value, hc_start->comp_name.value,
				hlt_chk.comp_name.length);
			l_num = hc_start->hc_key.keyLen;
			hlt_chk.key_len = l_num;
			memcpy(hlt_chk.name.key, hc_start->hc_key.key, hc_start->hc_key.keyLen);
			hlt_chk.name.keyLen = hc_start->hc_key.keyLen;

			/* get the record from healthcheck database */
			if (0 == avnd_hcdb_rec_get(cb, &hlt_chk)) {
				/* HC instance did not exist, look for HC type */
				if (nullptr == avnd_hctypedb_rec_get(&(*o_comp)->saAmfCompType, &hc_start->hc_key)) {
					*o_amf_rc = SA_AIS_ERR_NOT_EXIST;
					return;
				}
			}

			memset(&tmp_hc_rec, '\0', sizeof(AVND_COMP_HC_REC));
			tmp_hc_rec.key = hlt_chk.name;
			tmp_hc_rec.req_hdl = hc_start->hdl;
			/* determine if this healthcheck is already active */
			if (0 != m_AVND_COMPDB_REC_HC_GET(**o_comp, tmp_hc_rec)) {
				*o_amf_rc = SA_AIS_ERR_EXIST;
				return;
			}
		}
		break;

	case AVSV_AMF_HC_STOP:
		{
			AVSV_AMF_HC_STOP_PARAM *hc_stop;

			hc_stop = (AVSV_AMF_HC_STOP_PARAM *)param;

			/* get the comp */
			if (0 == (*o_comp = m_AVND_COMPDB_REC_GET(cb->compdb, hc_stop->comp_name))) {
				*o_amf_rc = SA_AIS_ERR_NOT_EXIST;
				return;
			}

			/* non proxy component should not start/stop/confirm health check for any other component */
			if (m_CMP_HORDER_SANAMET(hc_stop->comp_name, hc_stop->proxy_comp_name)) {
				if (!m_AVND_COMP_TYPE_IS_PROXIED(*o_comp)
				    || m_CMP_HORDER_SANAMET((*o_comp)->pxy_comp->name,
							    hc_stop->proxy_comp_name)) {
					*o_amf_rc = SA_AIS_ERR_NOT_EXIST;
					return;
				}
			}
			memset(&hlt_chk, 0, sizeof(AVSV_HLT_KEY));
			hlt_chk.comp_name.length = hc_stop->comp_name.length;
			memcpy(hlt_chk.comp_name.value, hc_stop->comp_name.value,
			       hlt_chk.comp_name.length);
			l_num = hc_stop->hc_key.keyLen;
			hlt_chk.key_len = l_num;
			memcpy(hlt_chk.name.key, hc_stop->hc_key.key, hc_stop->hc_key.keyLen);
			hlt_chk.name.keyLen = hc_stop->hc_key.keyLen;

			memset(&tmp_hc_rec, '\0', sizeof(AVND_COMP_HC_REC));
			tmp_hc_rec.key = hlt_chk.name;
			tmp_hc_rec.req_hdl = hc_stop->hdl;
			/* get the record from component healthcheck list */
			if (0 == (*o_hc_rec = m_AVND_COMPDB_REC_HC_GET(**o_comp, tmp_hc_rec))) {
				*o_amf_rc = SA_AIS_ERR_NOT_EXIST;
				return;
			}
		}
		break;

	case AVSV_AMF_HC_CONFIRM:
		{
			AVSV_AMF_HC_CONFIRM_PARAM *hc_confirm;

			hc_confirm = (AVSV_AMF_HC_CONFIRM_PARAM *)param;

			/* get the comp */
			if (0 == (*o_comp = m_AVND_COMPDB_REC_GET(cb->compdb, hc_confirm->comp_name))) {
				*o_amf_rc = SA_AIS_ERR_NOT_EXIST;
				return;
			}

			/* non proxy component should not start/stop/confirm health check for any other component */
			if (m_CMP_HORDER_SANAMET(hc_confirm->comp_name, hc_confirm->proxy_comp_name)) {
				if (!m_AVND_COMP_TYPE_IS_PROXIED(*o_comp)
				    || m_CMP_HORDER_SANAMET((*o_comp)->pxy_comp->name,
							    hc_confirm->proxy_comp_name)) {
					*o_amf_rc = SA_AIS_ERR_NOT_EXIST;
					return;
				}
			}
			memset(&hlt_chk, 0, sizeof(AVSV_HLT_KEY));
			hlt_chk.comp_name.length = hc_confirm->comp_name.length;
			memcpy(hlt_chk.comp_name.value, hc_confirm->comp_name.value,
			       hlt_chk.comp_name.length);
			l_num = hc_confirm->hc_key.keyLen;
			hlt_chk.key_len = l_num;
			memcpy(hlt_chk.name.key, hc_confirm->hc_key.key, hc_confirm->hc_key.keyLen);
			hlt_chk.name.keyLen = hc_confirm->hc_key.keyLen;

			memset(&tmp_hc_rec, '\0', sizeof(AVND_COMP_HC_REC));
			tmp_hc_rec.key = hlt_chk.name;
			tmp_hc_rec.req_hdl = hc_confirm->hdl;
			/* get the record from component healthcheck list */
			if (0 == (*o_hc_rec = m_AVND_COMPDB_REC_HC_GET(**o_comp, tmp_hc_rec))) {
				*o_amf_rc = SA_AIS_ERR_NOT_EXIST;
				return;
			}

			/* is this a AMF invoked HC */
			if (SA_AMF_HEALTHCHECK_COMPONENT_INVOKED != (*o_hc_rec)->inv) {
				*o_amf_rc = SA_AIS_ERR_NOT_EXIST;
				return;
			}
		}
		break;

	default:
		osafassert(0);
	}			/* switch */

	/* npi comps dont interact with amf */
	if (!m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(*o_comp) && !m_AVND_COMP_TYPE_IS_PROXIED(*o_comp)) {
		*o_amf_rc = SA_AIS_ERR_BAD_OPERATION;
		return;
	}

	return;
}

/****************************************************************************
  Name          : avnd_comp_hc_rec_add
 
  Description   : This routine adds the healthcheck record for the component.
 
  Arguments     : cb        - ptr to the AvND control block
                  comp      - ptr the the component
                  hc_start  - ptr to the healthcheck start parameters
                  dest      - ptr to the mds-dest of the process that started 
                              the healthcheck
 
  Return Values : ptr to the healthcheck record.
 
  Notes         : None.
******************************************************************************/
AVND_COMP_HC_REC *avnd_comp_hc_rec_add(AVND_CB *cb, AVND_COMP *comp, AVSV_AMF_HC_START_PARAM *hc_start, MDS_DEST *dest)
{
	AVND_COMP_HC_REC *rec = 0;
	AVND_HC *hc = 0;
	AVSV_HLT_KEY hlt_chk;
	uint32_t l_num;
	TRACE_ENTER2();

	rec = new AVND_COMP_HC_REC();

	/* create the association with hdl-mngr */
	if ((0 == (rec->opq_hdl = ncshm_create_hdl(cb->pool_id, NCS_SERVICE_ID_AVND, (NCSCONTEXT)rec))))
		goto err;

	memset(&hlt_chk, 0, sizeof(AVSV_HLT_KEY));
	hlt_chk.comp_name.length = hc_start->comp_name.length;
	memcpy(hlt_chk.comp_name.value, hc_start->comp_name.value,
		hlt_chk.comp_name.length);
	l_num = hc_start->hc_key.keyLen;
	hlt_chk.key_len = l_num;
	memcpy(hlt_chk.name.key, hc_start->hc_key.key, hc_start->hc_key.keyLen);
	hlt_chk.name.keyLen = hc_start->hc_key.keyLen;

	/* get the record from hc db */
	if ((hc = avnd_hcdb_rec_get(cb, &hlt_chk)) != nullptr) {
		rec->period = hc->period;
		rec->max_dur = hc->max_dur;
	} else {
		/* HC instance did not exist, look for HC type */
		AVND_HCTYPE *hctype = avnd_hctypedb_rec_get(&comp->saAmfCompType, &hc_start->hc_key);
		if (hctype != nullptr) {
			rec->period = hctype->saAmfHctDefPeriod;
			rec->max_dur = hctype->saAmfHctDefMaxDuration;
		} else
			goto err;
	}

	rec->key = hc_start->hc_key;

	/* store the params sent by the component */
	rec->inv = hc_start->inv_type;
	rec->rec_rcvr.raw = hc_start->rec_rcvr.raw;
	rec->req_hdl = hc_start->hdl;
	rec->dest = *dest;

	/* store the comp bk ptr */
	rec->comp = comp;
	rec->comp_name = comp->name;

	rec->status = AVND_COMP_HC_STATUS_STABLE;

	/* add the record to the healthcheck list */
	m_AVND_COMPDB_REC_HC_ADD(*comp, *rec);

	TRACE_LEAVE();
	return rec;

err:
	/* Undo the changes in case of error.*/
	if (rec) {
		if (rec->opq_hdl)
			ncshm_destroy_hdl(NCS_SERVICE_ID_AVND, rec->opq_hdl);

		/* stop the healthcheck timer */
		if (m_AVND_TMR_IS_ACTIVE(rec->tmr)) {
			m_AVND_TMR_COMP_HC_STOP(cb, *rec);
		}

		/* free the record */
		delete rec;
	}

	TRACE_LEAVE();
	return 0;
}

/****************************************************************************
  Name          : avnd_comp_hc_rec_del
 
  Description   : This routine deletes the healthcheck record from the 
                  healthcheck list maintained by the component.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr the the component
                  rec  - ptr to healthcheck record
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avnd_comp_hc_rec_del(AVND_CB *cb, AVND_COMP *comp, AVND_COMP_HC_REC *rec)
{
	TRACE_ENTER();
	/* remove the association with hdl-mngr */
	if (rec->opq_hdl)
		ncshm_destroy_hdl(NCS_SERVICE_ID_AVND, rec->opq_hdl);

	/* stop the healthcheck timer */
	if (m_AVND_TMR_IS_ACTIVE(rec->tmr)) {
		m_AVND_TMR_COMP_HC_STOP(cb, *rec);
	}

	/* unlink from the comp-hc list */
	m_AVND_COMPDB_REC_HC_REM(*comp, *rec);
	TRACE_LEAVE();
	/* free the record */
	delete rec;
}

/****************************************************************************
  Name          : avnd_comp_hc_rec_del_all
 
  Description   : This routine clears all the healthcheck records.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the component
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avnd_comp_hc_rec_del_all(AVND_CB *cb, AVND_COMP *comp)
{
	AVND_COMP_HC_REC *rec = 0;
	TRACE_ENTER2("Comp '%s'", comp->name.value);

	/* scan & delete each healthcheck record */
	while (0 != (rec = (AVND_COMP_HC_REC *)m_NCS_DBLIST_FIND_FIRST(&comp->hc_list))) {
		avnd_comp_hc_rec_del(cb, comp, rec);
	}
	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : avnd_comp_hc_rec_process
 
  Description   : This routine does the healthcheck processing for various 
                  triggers.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr the the component
                  rec  - ptr to the healthcheck record
                  op   - operation for this record
                  res  - healthcheck result (valid only for confirm op)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_comp_hc_rec_process(AVND_CB *cb, AVND_COMP *comp, AVND_COMP_HC_REC *rec, AVND_COMP_HC_OP op, SaAisErrorT res)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	switch (op) {
	case AVND_COMP_HC_START:
		rc = avnd_comp_hc_rec_start(cb, comp, rec);
		break;

	case AVND_COMP_HC_STOP:
		rc = avnd_comp_hc_rec_stop(cb, comp, rec);
		break;

	case AVND_COMP_HC_CONFIRM:
		rc = avnd_comp_hc_rec_confirm(cb, comp, rec, res);
		break;

	case AVND_COMP_HC_TMR_EXP:
		rc = avnd_comp_hc_rec_tmr_exp(cb, comp, rec);
		break;

	default:
		osafassert(0);
		break;
	}

	/* 
	 * BEWARE !!! 
	 * rec variable may point to a memory location that is already freed (as 
	 * a part of component error processing in hc-confirm & hc-tmr expiry 
	 * processing). Dont access it.
	 */

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_hc_rec_start
 
  Description   : This routine starts the healthcheck for a component as per
                  the request params.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr the the component
                  rec  - ptr to the healthcheck record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_comp_hc_rec_start(AVND_CB *cb, AVND_COMP *comp, AVND_COMP_HC_REC *rec)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* Invoke the hc callbk for AMF initiated healthcheck */
	if (m_AVND_COMP_HC_REC_IS_AMF_INITIATED(rec)) {
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_HC, rec, 0);

		if (NCSCC_RC_SUCCESS == rc)
			rec->status = AVND_COMP_HC_STATUS_WAIT_FOR_RESP;
	}

	/* now start the periodic timer (relevant both for comp-initiated & amf-initiated) */
	if (NCSCC_RC_SUCCESS == rc) {
		m_AVND_TMR_COMP_HC_START(cb, *rec, rc);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_hc_cmd_start
 
  Description   : This routine starts the healthcheck for a component as per
                  the request params.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr the the component
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
******************************************************************************/
uint32_t avnd_comp_hc_cmd_start(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc;
	AVSV_AMF_HC_START_PARAM hc = {0};

	TRACE_ENTER2("'%s'", comp->name.value);

	/* create a HC key with a default name */
	strncpy((char*)hc.hc_key.key, hc_cmd_name, sizeof(hc.hc_key.key)-1);

	hc.hc_key.keyLen = strlen((char*)hc.hc_key.key);
	hc.inv_type = SA_AMF_HEALTHCHECK_AMF_INVOKED;
	hc.comp_name = comp->name;

	AVND_COMP_HC_REC *rec = avnd_comp_hc_rec_add(cb, comp, &hc, &comp->reg_dest);

	if (rec) {
		// timeout for the HC cmd is HC duration
		comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_HC - 1].timeout = rec->max_dur;

		// be nice and allow the component to start properly by just starting the
		// timer here and do first health check at timeout instead
		rc = avnd_start_tmr(cb, &rec->tmr, AVND_TMR_HC, rec->period, rec->opq_hdl);
		osafassert(rc == NCSCC_RC_SUCCESS);
	} else {
		LOG_WA("'%s' has HC command configured but no HC named '%s' exist",
			   comp->name.value, hc.hc_key.key);
		rc = NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return rc;
}

/**
 * Find the command based health check record using the magic name
 * @param comp
 * 
 * @return AVND_COMP_HC_REC*
 */
static AVND_COMP_HC_REC *find_hc_rec(const AVND_COMP *comp)
{
	AVND_COMP_HC_REC *rec = nullptr;

	TRACE_ENTER2("'%s'", comp->name.value);

	for (rec = (AVND_COMP_HC_REC *)m_NCS_DBLIST_FIND_FIRST(&comp->hc_list);
		  rec != nullptr;
		  rec = (AVND_COMP_HC_REC *)m_NCS_DBLIST_FIND_NEXT(&rec->comp_dll_node)) {

		if (strncmp((char*)rec->key.key, hc_cmd_name, sizeof(rec->key.key)) == 0)
			break;
	}

	return rec;
}

/****************************************************************************
  Name          : avnd_comp_hc_cmd_restart
 
  Description   : This routine restarts the healthcheck for a component as per
                  the request params.
 
  Arguments     : comp - ptr the the component
 
  Return Values : -
******************************************************************************/
void avnd_comp_hc_cmd_restart(AVND_COMP *comp)
{
	AVND_COMP_HC_REC *rec = find_hc_rec(comp);
	/* This may come because of script responding after component was terminated by lock-in. */
	if(rec == nullptr)
		return;

	/*
	   Before terminating/cleaning up a NPI comp, AMFND stops timer for periodically
	   invoking command based health check for it. At the time of launching TERMINATE/CLEANUP
	   command, if already invoked Health Check command is in progress then its response may 
	   come during the termination phase . If comp is not found instantiated then do not start 
	   health check timer again.  Exit status of any pending Terminate/cleanup scripts will 
	   take care of comp's fault management.
	 */
	if (!m_AVND_COMP_PRES_STATE_IS_INSTANTIATED(comp)) {
		TRACE_1("'%s' not instantiated, not starting Command based HC timer",comp->name.value);
		rec->status = AVND_COMP_HC_STATUS_STABLE;
		return;
	}
	if (rec->status == AVND_COMP_HC_STATUS_SND_TMR_EXPD) {
		uint32_t rc = avnd_start_tmr(avnd_cb, &rec->tmr, AVND_TMR_HC, rec->period, rec->opq_hdl);
		osafassert(rc == NCSCC_RC_SUCCESS);
		rec->status = AVND_COMP_HC_STATUS_STABLE;
	} else
		rec->status = AVND_COMP_HC_STATUS_STABLE;
}

/****************************************************************************
  Name          : avnd_comp_hc_cmd_stop
 
  Description   : This routine stops the healthcheck for a component as per
                  the request params.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr the the component
 
  Return Values : -
******************************************************************************/
void avnd_comp_hc_cmd_stop(AVND_CB *cb, AVND_COMP *comp)
{
	AVND_COMP_HC_REC *rec = find_hc_rec(comp);

	if (rec != nullptr)
		avnd_stop_tmr(cb, &rec->tmr);
}

/****************************************************************************
  Name          : avnd_comp_hc_rec_stop
 
  Description   : This routine stops the healthcheck for a component.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr the the component
                  rec  - ptr to the healthcheck record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_comp_hc_rec_stop(AVND_CB *cb, AVND_COMP *comp, AVND_COMP_HC_REC *rec)
{
	AVND_COMP_CBK *cbk_rec = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* 
	 * It may so happen that the callbk record is added to the callbk list 
	 * (waiting for a response from the comp) & the comp stopped that 
	 * healthcheck. The response timer expiry will then be treated as component 
	 * failure. To avoid this, clear the HC callbk record for AMF initiated 
	 * healthchecks.
	 */
	if (m_AVND_COMP_HC_REC_IS_AMF_INITIATED(rec)) {
		m_AVND_COMPDB_CBQ_HC_CBK_GET(comp, rec->key, cbk_rec);
		if (cbk_rec)
			/* pop & delete this record */
			avnd_comp_cbq_rec_pop_and_del(cb, comp, cbk_rec, true);
	}

	/* delete the record */
	avnd_comp_hc_rec_del(cb, comp, rec);

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_hc_rec_confirm
 
  Description   : This routine does the healthcheck confirm processing in 
                  response to mesage received from the comp.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr the the component
                  rec  - ptr to the healthcheck record
                  res  - healthcheck result reported by the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_comp_hc_rec_confirm(AVND_CB *cb, AVND_COMP *comp, AVND_COMP_HC_REC *rec, SaAisErrorT res)
{
	AVND_ERR_INFO err_info;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* it has to be comp-initiated healthcheck */
	osafassert(m_AVND_COMP_HC_REC_IS_COMP_INITIATED(rec));

	if (SA_AIS_OK == res) {
		/* restart the periodic timer */
		m_AVND_TMR_COMP_HC_STOP(cb, *rec);
		m_AVND_TMR_COMP_HC_START(cb, *rec, rc);
	} else {
		/* process comp failure */
		err_info.src = AVND_ERR_SRC_HC;
		err_info.rec_rcvr.raw = rec->rec_rcvr.raw;
		rc = avnd_err_process(cb, comp, &err_info);
	}
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_comp_hc_rec_tmr_exp
 
  Description   : This routine resends the healthcheck callback to the 
                  component.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr the the component
                  rec  - ptr to the healthcheck record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_comp_hc_rec_tmr_exp(AVND_CB *cb, AVND_COMP *comp, AVND_COMP_HC_REC *rec)
{
	AVND_ERR_INFO err_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%s - %s, sts: %u", comp->name.value, rec->key.key, rec->status);

	/* There is a chance that the term command has been issued to comp and
	   the timer has expired and it is in mail box. 
	   So, if the component is not in healthy state, then don't start HC. */

	if (!m_AVND_COMP_PRES_STATE_IS_INSTANTIATED(comp)) {
		TRACE_1("'%s' not instantiated, not starting HC", comp->name.value);
		rec->status = AVND_COMP_HC_STATUS_STABLE;
		return rc;
	}

	if (m_AVND_COMP_HC_REC_IS_AMF_INITIATED(rec)) {
		if (rec->status == AVND_COMP_HC_STATUS_STABLE)
			if (comp->is_hc_cmd_configured &&
				(strncmp((char*)rec->key.key, hc_cmd_name, sizeof(rec->key.key)) == 0)) {
				rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_HC);
				if (rc == NCSCC_RC_SUCCESS) {
					rec->status = AVND_COMP_HC_STATUS_WAIT_FOR_RESP;
					rc = avnd_start_tmr(cb, &rec->tmr, AVND_TMR_HC, rec->period, rec->opq_hdl);
				}
			} else {
				/* same as healthcheck start processing */
				rc = avnd_comp_hc_rec_start(cb, comp, rec);
			}
		else if (rec->status == AVND_COMP_HC_STATUS_WAIT_FOR_RESP) {
			rec->status = AVND_COMP_HC_STATUS_SND_TMR_EXPD;
		}
	} else {
		/* process comp failure */
		err_info.src = AVND_ERR_SRC_HC;
		err_info.rec_rcvr.raw = rec->rec_rcvr.raw;
		rc = avnd_err_process(cb, comp, &err_info);
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_comp_hc_finalize
 
  Description   : This routine removes all the component healthcheck records 
                  that share the specified AMF handle & the mds-dest. It is 
                  invoked when the application invokes saAmfFinalize for a 
                  certain handle.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr the the component
                  hdl  - amf-handle
                  dest - ptr to mds-dest (of the prc that finalize)
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avnd_comp_hc_finalize(AVND_CB *cb, AVND_COMP *comp, SaAmfHandleT hdl, MDS_DEST *dest)
{
	AVND_COMP_HC_REC *curr = 0, *prv = 0;

	curr = (AVND_COMP_HC_REC *)m_NCS_DBLIST_FIND_FIRST(&comp->hc_list);

	/* scan the entire comp-hc list & delete the matching records */
	while (curr) {
		prv = (AVND_COMP_HC_REC *)m_NCS_DBLIST_FIND_PREV(&curr->comp_dll_node);

		if ((curr->req_hdl == hdl) && !memcmp(&curr->dest, dest, sizeof(MDS_DEST))) {
			avnd_comp_hc_rec_del(cb, comp, curr);
			curr = (prv) ? (AVND_COMP_HC_REC *)m_NCS_DBLIST_FIND_NEXT(&prv->comp_dll_node) :
			    (AVND_COMP_HC_REC *)m_NCS_DBLIST_FIND_FIRST(&comp->hc_list);
		} else
			curr = (AVND_COMP_HC_REC *)m_NCS_DBLIST_FIND_NEXT(&curr->comp_dll_node);
	}			/* while */

	return;
}

/****************************************************************************
  Name          : avnd_dblist_hc_rec_cmp
      
  Description   : This routine compares the AVND_COMP_HC_REC structures.
                  It is used by DLL library.
 
  Arguments     : key1 - ptr to the 1st key
                  key2 - ptr to the 2nd key
 
  Return Values : 0, if keys are equal
                  1, if key1 is greater than key2
                  2, if key1 is lesser than key2
   
  Notes         : None.
******************************************************************************/
uint32_t avnd_dblist_hc_rec_cmp(uint8_t *key1, uint8_t *key2)
{
	int i = 0;
	AVND_COMP_HC_REC *rec1, *rec2;

	rec1 = (AVND_COMP_HC_REC *)key1;
	rec2 = (AVND_COMP_HC_REC *)key2;

	i = avsv_dblist_sahckey_cmp((uint8_t *)&rec1->key, (uint8_t *)&rec2->key);

	if (i == 0)
		return avsv_dblist_uns64_cmp((uint8_t *)&rec1->req_hdl, (uint8_t *)&rec2->req_hdl);

	return ((i > 0) ? 1 : 2);
}
