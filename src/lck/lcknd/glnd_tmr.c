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

  This file contains GLND timer interface routines.
 
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "lck/lcknd/glnd.h"

/*****************************************************************************
  PROCEDURE NAME : glnd_start_tmr

  DESCRIPTION    : Starts the GLND timer. If the timer is already active, it 
                   is restarted (ie. stopped & started without reallocating the 
                   tmr block).

  ARGUMENTS      : cb     - ptr to the GLND control block
                  tmr    - ptr to the GLND timer block
                  type    - timer type
                  period - timer period
                  uarg   - opaque handle that is returned on timer expiry

  RETURNS        : NCSCC_RC_SUCCESS - Success
               NCSCC_RC_FAILURE  - Failure

  NOTES         : None
*****************************************************************************/
uint32_t glnd_start_tmr(GLND_CB *cb, GLND_TMR *tmr, GLND_TMR_TYPE type, SaTimeT period, uint32_t uarg)
{
	SaTimeT  my_period = (m_GLSV_CONVERT_SATIME_TEN_MILLI_SEC(period));

	if (GLND_TMR_MAX <= type) {
		LOG_ER("GLND timer start failure: type %d ", type);
		return NCSCC_RC_FAILURE;
	}

	if (tmr->tmr_id == TMR_T_NULL) {
		tmr->type = type;
		m_NCS_TMR_CREATE(tmr->tmr_id, my_period, glnd_tmr_exp, (void *)tmr);
	}

	if (tmr->is_active == true) {
		m_NCS_TMR_STOP(tmr->tmr_id);
		tmr->is_active = false;
	}

	tmr->opq_hdl = uarg;
	tmr->cb_hdl = cb->cb_hdl_id;
	m_NCS_TMR_START(tmr->tmr_id, my_period, glnd_tmr_exp, (void *)tmr);
	tmr->is_active = true;

	if (TMR_T_NULL == tmr->tmr_id) {
		LOG_ER("GLND timer start fail: type %d", type);
		return NCSCC_RC_FAILURE;
	}

	TRACE("Started GLND Timer for %d @ %lld ticks", type, my_period);

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_stop_tmr

  DESCRIPTION    : Stops the GLND timer.

  ARGUMENTS      : tmr    - ptr to the GLND timer block
               
  RETURNS        : void

  NOTES         : None
*****************************************************************************/
void glnd_stop_tmr(GLND_TMR *tmr)
{
	/* If timer type is invalid just return */
	if (tmr == NULL) {
		LOG_ER("GLND timer stop failure");
		return;
	}

	if (tmr != NULL && GLND_TMR_MAX <= tmr->type) {
		LOG_ER("GLND timer stop failure: tmr_type %d", tmr->type);
		return;
	}

	/* Stop the timer if it is active... */
	if (tmr->is_active == true) {
		TRACE("Stopped GLND Timer for: tmr_type %d", tmr->type);
		m_NCS_TMR_STOP(tmr->tmr_id);
		tmr->is_active = false;
	}

	/* Destroy the timer if it exists.. */
	if (tmr->tmr_id != TMR_T_NULL) {
		m_NCS_TMR_DESTROY(tmr->tmr_id);
		tmr->tmr_id = TMR_T_NULL;
	}
	return;
}

/*****************************************************************************
  PROCEDURE NAME : glnd_tmr_evt_map
  DESCRIPTION    : Maps a timer type to the corresponding GLND evt type.
  ARGUMENTS      : tmr_type - timer type  
  RETURNS        : GLND event type
  NOTES         : None
*****************************************************************************/
static GLSV_GLND_EVT_TYPE glnd_tmr_evt_map(GLND_TMR_TYPE tmr_type)
{
	switch (tmr_type) {
	case GLND_TMR_RES_LOCK_REQ_TIMEOUT:
		return GLSV_GLND_EVT_RSC_LOCK_TIMEOUT;
	case GLND_TMR_RES_NM_LOCK_REQ_TIMEOUT:
		return GLSV_GLND_EVT_NM_RSC_LOCK_TIMEOUT;
	case GLND_TMR_RES_NM_UNLOCK_REQ_TIMEOUT:
		return GLSV_GLND_EVT_NM_RSC_UNLOCK_TIMEOUT;
	case GLND_TMR_RES_REQ_TIMEOUT:
		return GLSV_GLND_EVT_RSC_OPEN_TIMEOUT;
	case GLND_TMR_AGENT_INFO_GET_WAIT:
		return GLSV_GLND_EVT_AGENT_INFO_GET_TIMEOUT;
	default:
		return GLSV_GLND_EVT_MAX;
	}
}

/*****************************************************************************
  PROCEDURE NAME : glnd_tmr_exp

  DESCRIPTION    : GLND timer expiry callback routine.It sends corresponding
                  timer events to GLND.

  ARGUMENTS      : uarg - ptr to the GLND timer block

  RETURNS        : void

  NOTES         : None
*****************************************************************************/
void glnd_tmr_exp(void *uarg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	GLND_CB *cb = 0;
	GLND_TMR *tmr = (GLND_TMR *)uarg;
	GLSV_GLND_EVT *evt = 0;

	/* retrieve GLND CB */
	cb = (GLND_CB *)ncshm_take_hdl(NCS_SERVICE_ID_GLND, tmr->cb_hdl);
	if (!cb) {
		LOG_ER("GLND cb take handle failed");
		return;
	}

	if (tmr->is_active) {
		tmr->is_active = false;

		/* create & send the timer event */
		evt = m_MMGR_ALLOC_GLND_EVT;
		if (evt) {
			/* assign the timer evt */
			evt->type = glnd_tmr_evt_map(tmr->type);
			evt->info.tmr.opq_hdl = tmr->opq_hdl;

			evt->glnd_hdl = tmr->cb_hdl;
			rc = glnd_evt_local_send(cb, evt, NCS_IPC_PRIORITY_HIGH);
			if (rc != NCSCC_RC_SUCCESS) {
				LOG_ER("GLND mds send failure: from mds_dest %u , evt_type %d",
						     m_NCS_NODE_ID_FROM_MDS_DEST(cb->glnd_mdest_id), evt->type);
			}

		}
	}

	/* return GLND CB */
	ncshm_give_hdl(tmr->cb_hdl);

	return;

}
