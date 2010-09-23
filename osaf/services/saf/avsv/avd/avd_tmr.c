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

  DESCRIPTION:This module does the initialisation of TIMER interface and 
  provides functionality for adding, deleting and expiry of timers.
  The expiry routine packages the timer expiry information as local
  event information and posts it to the main loop for processing.

..............................................................................

  FUNCTIONS INCLUDED in this module:

  avd_start_tmr - Starts the AvD timer.
  avd_stop_tmr - Stops the AvD timer.
  avd_tmr_exp - AvD timer expiry callback routine.

  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#include "avd.h"

/*****************************************************************************
  PROCEDURE NAME : avd_start_tmr

  DESCRIPTION    : Starts the AvD timer. If the timer is already active, it 
               is restarted (ie. stopped & started without reallocating the 
               tmr block).

  ARGUMENTS      : 
               tmr    - ptr to the AvD timer block
               period - timer period in nano seconds

  RETURNS        : NCSCC_RC_SUCCESS - Success
                   NCSCC_RC_FAILURE - Failure

  NOTES         : The timer related info needs to be filled in the 
  timer block before calling this routine.
*****************************************************************************/
uns32 avd_start_tmr(AVD_CL_CB *cb, AVD_TMR *tmr, SaTimeT period)
{
	uns32 tmr_period;

	TRACE_ENTER2("%u", tmr->type);

	tmr_period = (uns32)(period / AVSV_NANOSEC_TO_LEAPTM);

	if (AVD_TMR_MAX <= tmr->type) {
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, tmr->type);
		return NCSCC_RC_FAILURE;
	}

	if (tmr->tmr_id == TMR_T_NULL) {
		m_NCS_TMR_CREATE(tmr->tmr_id, tmr_period, avd_tmr_exp, (void *)tmr);
	}

	if (tmr->is_active == TRUE) {
		tmr->is_active = FALSE;
		m_NCS_TMR_STOP(tmr->tmr_id);
	}

	m_NCS_TMR_START(tmr->tmr_id, tmr_period, avd_tmr_exp, (void *)tmr);
	tmr->is_active = TRUE;

	if (TMR_T_NULL == tmr->tmr_id)
		return NCSCC_RC_FAILURE;

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
  PROCEDURE NAME : avd_stop_tmr

  DESCRIPTION    : Stops the AvD timer.

  ARGUMENTS      : 
                   tmr    - ptr to the AvD timer block
  RETURNS        : void

  NOTES         : None
*****************************************************************************/
void avd_stop_tmr(AVD_CL_CB *cb, AVD_TMR *tmr)
{
	TRACE_ENTER2("%u", tmr->type);

	/* If timer type is invalid just return */
	if (AVD_TMR_MAX <= tmr->type) {
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, tmr->type);
		return;
	}

	/* Stop the timer if it is active... */
	if (tmr->is_active == TRUE) {
		tmr->is_active = FALSE;
		m_NCS_TMR_STOP(tmr->tmr_id);
	}

	/* Destroy the timer if it exists.. */
	if (tmr->tmr_id != TMR_T_NULL) {
		m_NCS_TMR_DESTROY(tmr->tmr_id);
		tmr->tmr_id = TMR_T_NULL;
	}
}

/*****************************************************************************
  PROCEDURE NAME : avd_tmr_exp

  DESCRIPTION    : AvD timer expiry callback routine. It sends corresponding
               timer events to AvDs main loop from the timer thread context.

  ARGUMENTS      : uarg - ptr to the AvD timer block

  RETURNS        : void

  NOTES         : None
*****************************************************************************/
void avd_tmr_exp(void *uarg)
{
	AVD_CL_CB *cb = avd_cb;
	AVD_TMR *tmr = (AVD_TMR *)uarg;
	AVD_EVT *evt = AVD_EVT_NULL;

	TRACE_ENTER();

	/* 
	 * Check if this timer was stopped after "avd_tmr_exp" was called 
	 * but before entering this code.
	 */
	if (TRUE == tmr->is_active) {
		tmr->is_active = FALSE;

		/* create & send the timer event */
		evt = calloc(1, sizeof(AVD_EVT));
		assert(evt);

		evt->info.tmr = *tmr;
		evt->rcv_evt = (tmr->type - AVD_TMR_CL_INIT) + AVD_EVT_TMR_CL_INIT;

		if (m_NCS_IPC_SEND(&cb->avd_mbx, evt, NCS_IPC_PRIORITY_VERY_HIGH) != NCSCC_RC_SUCCESS) {
			LOG_ER("avd_tmp_exp: ipc send failed");
			free(evt);
		}
	}

	TRACE_LEAVE();
}
