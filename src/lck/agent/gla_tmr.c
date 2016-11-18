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

/*******************************************************************************
  DESCRIPTION:

  This file contains EDS timer interface routines.
 
..............................................................................

  FUNCTIONS INCLUDED in this module:
  
*******************************************************************************/
#include "gla.h"

/*****************************************************************************
  PROCEDURE NAME : gla_start_tmr

  DESCRIPTION    : Starts the GLA timer. If the timer is already active, it 
                   is restarted (ie. stopped & started without reallocating the 
                   tmr block).

  ARGUMENTS      : cb     - ptr to the GLA control block
                   tmr    - ptr to the GLA timer block
                   type   - timer type
                   period - timer period
                   uarg   - opaque handle that is returned on timer expiry

  RETURNS        : NCSCC_RC_SUCCESS - Success
                   NCSCC_RC_FAILURE  - Failure

  NOTES         : None
*****************************************************************************/
uint32_t gla_start_tmr(GLA_TMR *tmr)
{
	SaTimeT  period = (m_GLSV_CONVERT_SATIME_TEN_MILLI_SEC(GLSV_GLA_TMR_DEFAULT_TIMEOUT));
		
	if (tmr->tmr_id == TMR_T_NULL) {
		m_NCS_TMR_CREATE(tmr->tmr_id, period, gla_tmr_exp, (void *)tmr);
	}

	if (tmr->is_active == true) {
		m_NCS_TMR_STOP(tmr->tmr_id);
		tmr->is_active = false;
	}

	m_NCS_TMR_START(tmr->tmr_id, period, gla_tmr_exp, (void *)tmr);

	if (TMR_T_NULL == tmr->tmr_id) {
		return NCSCC_RC_FAILURE;
	}
	tmr->is_active = true;

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : gla_stop_tmr

  DESCRIPTION    : Stops the GLA timer.

  ARGUMENTS      : tmr    - ptr to the GLA timer block
               
  RETURNS        : void

  NOTES          : None
*****************************************************************************/
void gla_stop_tmr(GLA_TMR *tmr)
{
	
	/* Stop the timer if it is active... */
	if (tmr->is_active == true) {
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
  PROCEDURE NAME : gla_tmr_exp

  DESCRIPTION    : GLA timer expiry callback routine.It sends corresponding
                   timer events to GLA.

  ARGUMENTS      : uarg - ptr to the GLA timer block

  RETURNS        : void

  NOTES         : None
*****************************************************************************/
void gla_tmr_exp(void *uarg)
{
	GLA_CB *gla_cb = NULL;
	GLA_TMR *tmr = (GLA_TMR *)uarg;
	GLSV_GLA_CALLBACK_INFO *gla_clbk_info;

	/* retrieve GLA CB */
	gla_cb = (GLA_CB *)m_GLSV_GLA_RETRIEVE_GLA_CB;
	if (!gla_cb) {
		return;
	}

	/* check the event type */
	if (tmr->is_active) {
		tmr->is_active = false;
		/* Destroy the timer if it exists.. */
		if (tmr->tmr_id != TMR_T_NULL) {
			m_NCS_TMR_DESTROY(tmr->tmr_id);
			tmr->tmr_id = TMR_T_NULL;
		}
		/*allocate the memory */
		gla_clbk_info = m_MMGR_ALLOC_GLA_CALLBACK_INFO;
		if (!gla_clbk_info) {
			return;

		}
		memset(gla_clbk_info, 0, sizeof(GLSV_GLA_CALLBACK_INFO));
		switch (tmr->clbk_info.callback_type) {
		case GLSV_LOCK_RES_OPEN_CBK:
			gla_clbk_info->callback_type = GLSV_LOCK_RES_OPEN_CBK;
			gla_clbk_info->resourceId = tmr->clbk_info.resourceId;
			gla_clbk_info->params.res_open.error = SA_AIS_ERR_TIMEOUT;
			gla_clbk_info->params.res_open.invocation = tmr->clbk_info.invocation;

			break;

		case GLSV_LOCK_GRANT_CBK:
			gla_clbk_info->callback_type = GLSV_LOCK_GRANT_CBK;
			gla_clbk_info->resourceId = tmr->clbk_info.resourceId;
			gla_clbk_info->params.lck_grant.error = SA_AIS_ERR_TIMEOUT;
			gla_clbk_info->params.lck_grant.lcl_lockId = tmr->clbk_info.lcl_lockId;
			gla_clbk_info->params.lck_grant.resourceId = tmr->clbk_info.resourceId;
			gla_clbk_info->params.lck_grant.invocation = tmr->clbk_info.invocation;

			break;

		case GLSV_LOCK_UNLOCK_CBK:
			gla_clbk_info->callback_type = GLSV_LOCK_UNLOCK_CBK;
			gla_clbk_info->resourceId = tmr->clbk_info.resourceId;
			gla_clbk_info->params.unlock.error = SA_AIS_ERR_TIMEOUT;
			gla_clbk_info->params.unlock.lockId = tmr->clbk_info.lcl_lockId;
			gla_clbk_info->params.unlock.resourceId = tmr->clbk_info.resourceId;
			gla_clbk_info->params.unlock.invocation = tmr->clbk_info.invocation;
			break;
		case GLSV_LOCK_WAITER_CBK:
			break;
		}
		/* Put it in place it in the Queue */
		glsv_gla_callback_queue_write(gla_cb, tmr->client_hdl, gla_clbk_info);

	}

	/* return GLA_CB */
	if (gla_cb)
		m_GLSV_GLA_GIVEUP_GLA_CB;

	return;
}
