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

  This file contains AvND timer interface routines.
 
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "avnd.h"

/*****************************************************************************
  PROCEDURE NAME : avnd_start_tmr

  DESCRIPTION    : Starts the AvND timer. If the timer is already active, it 
               is restarted (ie. stopped & started without reallocating the 
               tmr block).

  ARGUMENTS      : cb     - ptr to the AvND control block
               tmr    - ptr to the AvND timer block
               type    - timer type
               period - timer period (in nano seconds)
               uarg   - opaque handle that is returned on timer expiry

  RETURNS        : NCSCC_RC_SUCCESS - Success
               NCSCC_RC_FAILURE  - Failure

  NOTES         : None
*****************************************************************************/
uns32 avnd_start_tmr(AVND_CB *cb, AVND_TMR *tmr, AVND_TMR_TYPE type, SaTimeT period, uns32 uarg)
{
	m_INIT_CRITICAL;

	if (AVND_TMR_MAX <= tmr->type)
		return NCSCC_RC_FAILURE;

	if (tmr->tmr_id == TMR_T_NULL) {
		tmr->type = type;
		m_NCS_TMR_CREATE(tmr->tmr_id, period / AVSV_NANOSEC_TO_LEAPTM, avnd_tmr_exp, (void *)tmr);
	}

	m_START_CRITICAL;
	if (tmr->is_active == TRUE) {
		m_NCS_TMR_STOP(tmr->tmr_id);
		tmr->is_active = FALSE;
	}
	tmr->opq_hdl = uarg;
	m_NCS_TMR_START(tmr->tmr_id, (uns32)(period / AVSV_NANOSEC_TO_LEAPTM), avnd_tmr_exp, (void *)tmr);
	tmr->is_active = TRUE;
	m_END_CRITICAL;

	if (TMR_T_NULL == tmr->tmr_id)
		return NCSCC_RC_FAILURE;

	/* log */
	m_AVND_LOG_TMR(type, AVND_LOG_TMR_START, AVND_LOG_TMR_SUCCESS, NCSFL_SEV_INFO);

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : avnd_stop_tmr

  DESCRIPTION    : Stops the AvND timer.

  ARGUMENTS      : cb     - ptr to the AvND control block
               tmr    - ptr to the AvND timer block
               type    - timer type
               period - timer period
               uarg   - opaque handle that is returned on timer expiry

  RETURNS        : void

  NOTES         : None
*****************************************************************************/
void avnd_stop_tmr(AVND_CB *cb, AVND_TMR *tmr)
{
	m_INIT_CRITICAL;

	/* If timer type is invalid just return */
	if (AVND_TMR_MAX <= tmr->type)
		return;

	/* Stop the timer if it is active... */
	m_START_CRITICAL;
	if (tmr->is_active == TRUE) {
		m_NCS_TMR_STOP(tmr->tmr_id);
		tmr->is_active = FALSE;
	}
	m_END_CRITICAL;

	/* Destroy the timer if it exists.. */
	if (tmr->tmr_id != TMR_T_NULL) {
		m_NCS_TMR_DESTROY(tmr->tmr_id);
		tmr->tmr_id = TMR_T_NULL;
	}

	/* log */
	m_AVND_LOG_TMR(tmr->type, AVND_LOG_TMR_STOP, AVND_LOG_TMR_SUCCESS, NCSFL_SEV_INFO);

	return;
}

/*****************************************************************************
  PROCEDURE NAME : avnd_tmr_exp

  DESCRIPTION    : AvND timer expiry callback routine. It sends corresponding
               timer events to AvND.

  ARGUMENTS      : uarg - ptr to the AvND timer block

  RETURNS        : void

  NOTES         : None
*****************************************************************************/
void avnd_tmr_exp(void *uarg)
{
	AVND_EVT_TYPE type = AVND_EVT_INVALID;
	uns32 rc = NCSCC_RC_SUCCESS;
	AVND_CB *cb = avnd_cb;
	AVND_TMR *tmr = (AVND_TMR *)uarg;
	AVND_EVT *evt = 0;

	/* 
	 * Check if this timer was stopped after "avnd_timer_expiry" was called 
	 * but before entering this critical secition. This should be taken 
	 * care by the Timer lib. But looks like it's not done.
	 */
	if (TRUE == tmr->is_active) {
		tmr->is_active = FALSE;

		/* determine the event type */
		if (AVND_TMR_QSCING_CMPL_RESP == tmr->type) {
			type = AVND_EVT_TMR_QSCING_CMPL;
		} else {
			type = (tmr->type - AVND_TMR_HC) + AVND_EVT_TMR_HC;
		}

		/* create & send the timer event */
		evt = avnd_evt_create(cb, type, 0, 0, (void *)&tmr->opq_hdl, 0, 0);
		if (evt) {
			rc = avnd_evt_send(cb, evt);
		} else {
			rc = NCSCC_RC_FAILURE;
		}
	}

	/* if failure, free the event */
	if (NCSCC_RC_SUCCESS != rc && evt)
		avnd_evt_destroy(evt);

	m_AVND_LOG_TMR(tmr->type, AVND_LOG_TMR_EXPIRY, AVND_LOG_TMR_SUCCESS, NCSFL_SEV_INFO);
}
