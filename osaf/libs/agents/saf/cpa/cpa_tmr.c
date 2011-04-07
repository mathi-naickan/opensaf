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
  FILE NAME: mqsv_tmr.c

  DESCRIPTION: CPA Timer Processing Routines

******************************************************************************/

#include "cpa.h"

/****************************************************************************
 * Name          : cpa_timer_expiry
 *
 * Description   : This function which is registered with the OS tmr function,
 *                 which will post a message to the corresponding mailbox 
 *                 depending on the component type.
 *
 *****************************************************************************/
void cpa_timer_expiry(NCSCONTEXT uarg)
{
	CPA_CB *cb = NULL;
	CPSV_EVT evt;

	CPA_TMR *tmr = (CPA_TMR *)uarg;
	uns32 hdl;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (cb == NULL)
		return;

	if (tmr != NULL) {
		hdl = tmr->uarg;
		memset(&evt, 0, sizeof(CPSV_EVT));
		evt.info.cpa.type = CPA_EVT_TIME_OUT;
		evt.info.cpa.info.tmr_info.type = tmr->type;
		if ((tmr->type == CPA_TMR_TYPE_OPEN) || (tmr->type == CPA_TMR_TYPE_SYNC)) {
			evt.info.cpa.info.tmr_info.lcl_ckpt_hdl = tmr->info.ckpt.lcl_ckpt_hdl;
			evt.info.cpa.info.tmr_info.client_hdl = tmr->info.ckpt.client_hdl;
			evt.info.cpa.info.tmr_info.invocation = tmr->info.ckpt.invocation;
		}

		ncshm_give_hdl(hdl);

		rc = cpa_process_evt(cb, &evt);

		if (tmr->tmr_id != TMR_T_NULL) {
			m_NCS_TMR_DESTROY(tmr->tmr_id);
			tmr->tmr_id = TMR_T_NULL;
		}
	}

	m_CPA_GIVEUP_CB;
	return;
}

/****************************************************************************
 * Name          : cpa_tmr_start
 *
 * Description   : This function which is used to start the CPA Timer
 *
 *****************************************************************************/
uns32 cpa_tmr_start(CPA_TMR *tmr, uns32 duration)
{
	if (tmr->tmr_id == TMR_T_NULL) {
		m_NCS_TMR_CREATE(tmr->tmr_id, duration, cpa_timer_expiry, (void *)tmr);
	}

	if (tmr->is_active == FALSE) {
		m_NCS_TMR_START(tmr->tmr_id, (uns32)duration, cpa_timer_expiry, (void *)tmr);
		tmr->is_active = TRUE;
	} else {
		m_NCS_TMR_STOP(tmr->tmr_id);
		m_NCS_TMR_START(tmr->tmr_id, (uns32)duration, cpa_timer_expiry, (void *)tmr);
	}

	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : cpa_tmr_stop
 *
 * Description   : This function which is used to stop the CPA Timer
 *
 * Arguments     : tmr      - Timer needs to be stoped.
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void cpa_tmr_stop(CPA_TMR *tmr)
{
	if (tmr->is_active == TRUE) {
		tmr->is_active = FALSE;
		m_NCS_TMR_STOP(tmr->tmr_id);
	}
	else {
		return;
	}	
	
	if (tmr->tmr_id != TMR_T_NULL) {
		m_NCS_TMR_DESTROY(tmr->tmr_id);
		tmr->tmr_id = TMR_T_NULL;
	}
	return;
}
