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
  FILE NAME: cpd_tmr.c

  DESCRIPTION: CPD Timer Processing Routines

******************************************************************************/

#include "cpd.h"

/****************************************************************************
 * Name          : cpd_timer_expiry
 *
 * Description   : This function which is registered with the OS tmr function,
 *                 which will post a message to the corresponding mailbox 
 *                 depending on the component type.
 *
 *****************************************************************************/
void cpd_timer_expiry(NCSCONTEXT uarg)
{
/*   uns32 hdl = (uns32)uarg;
   CPD_TMR *tmr = NULL; */
	CPD_TMR *tmr = (CPD_TMR *)uarg;
	NCS_IPC_PRIORITY priority = NCS_IPC_PRIORITY_HIGH;
	CPD_CB *cb;
	CPSV_EVT *evt = NULL;
	uns32 cpd_hdl = m_CPD_GET_CB_HDL;

	/* post a message to the corresponding component */
	if ((cb = (CPD_CB *)ncshm_take_hdl(NCS_SERVICE_ID_CPD, cpd_hdl)) == NULL)
		return;

/*   tmr = (CPD_TMR *)ncshm_take_hdl(NCS_SERVICE_ID_CPD, hdl); */
	if (tmr) {
		evt = m_MMGR_ALLOC_CPSV_EVT(NCS_SERVICE_ID_CPD);
		if (evt) {
			evt->type = CPSV_EVT_TYPE_CPD;
			evt->info.cpd.type = CPD_EVT_TIME_OUT;

			switch (tmr->type) {

			case CPD_TMR_TYPE_CPND_RETENTION:
				evt->info.cpd.info.tmr_info.type = CPD_TMR_TYPE_CPND_RETENTION;
				evt->info.cpd.info.tmr_info.info.cpnd_dest = tmr->info.cpnd_dest;
				break;

			default:
				break;
				/*   m_LOG_CPND_EVT(CPND_EVT_UNKNOWN, NCSFL_SEV_ERROR); */

			}

			/*  ncshm_give_hdl(hdl); */

			/* Post the event to CPD Thread */
			m_NCS_IPC_SEND(&cb->cpd_mbx, evt, priority);
		}
		ncshm_give_hdl(cpd_hdl);
	}
	return;
}

/****************************************************************************
 * Name          : cpd_tmr_start
 *
 * Description   : This function which is used to start the CPD Timer
 *
 *****************************************************************************/
uns32 cpd_tmr_start(CPD_TMR *tmr, uns32 duration)
{
	if (tmr->tmr_id == TMR_T_NULL) {
		m_NCS_TMR_CREATE(tmr->tmr_id, duration, cpd_timer_expiry, (void *)tmr);
	}

	if (tmr->is_active == FALSE) {
		m_NCS_TMR_START(tmr->tmr_id, (uns32)duration, cpd_timer_expiry, (void *)tmr);
		tmr->is_active = TRUE;
	} else {
		m_NCS_TMR_STOP(tmr->tmr_id);
		m_NCS_TMR_START(tmr->tmr_id, (uns32)duration, cpd_timer_expiry, (void *)tmr);
	}

	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : cpd_tmr_stop
 *
 * Description   : This function which is used to stop the CPD Timer
 *
 * Arguments     : tmr      - Timer needs to be stoped.
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void cpd_tmr_stop(CPD_TMR *tmr)
{
	if (tmr->is_active == TRUE) {
		tmr->is_active = FALSE;
		m_NCS_TMR_STOP(tmr->tmr_id);

	}
	if (tmr->tmr_id != TMR_T_NULL) {
		m_NCS_TMR_DESTROY(tmr->tmr_id);
		tmr->tmr_id = TMR_T_NULL;
	}
	return;
}
