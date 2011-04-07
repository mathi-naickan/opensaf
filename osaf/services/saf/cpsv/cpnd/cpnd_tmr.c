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

  DESCRIPTION: CPND Timer Processing Routines

******************************************************************************/

#include "cpnd.h"

/****************************************************************************
 * Name          : cpnd_timer_expiry
 *
 * Description   : This function which is registered with the OS tmr function,
 *                 which will post a message to the corresponding mailbox 
 *                 depending on the component type.
 *
 *****************************************************************************/
void cpnd_timer_expiry(NCSCONTEXT uarg)
{
	CPND_TMR *tmr = (CPND_TMR *)uarg;
	NCS_IPC_PRIORITY priority = NCS_IPC_PRIORITY_HIGH;
	CPND_CB *cb = NULL;
	CPSV_EVT *evt = NULL;

	if  (tmr == NULL)  {

		TRACE("CPND: Tmr Mailbox Processing: tmr invalid ");
		/* Fall through to free memory */
		return;
	}

	if ((cb = (CPND_CB *)ncshm_take_hdl(NCS_SERVICE_ID_CPND, gl_cpnd_cb_hdl)) == NULL) {
		TRACE("ncshm_take_hdl  returned CPND_CB as NULL");
		return;
	}

	/* post a message to the corresponding component */
	evt = m_MMGR_ALLOC_CPSV_EVT(NCS_SERVICE_ID_CPND);
	if (evt  == NULL)
	{
		TRACE("CPND: Mem alloc fail ");
		return;
	}

	/* Populate evt->info.cpnd.info.tmr_info 
	   evt->info.cpnd.info.tmr_info = *tmr; */
	memset(evt, 0, sizeof(CPSV_EVT));

	evt->type = CPSV_EVT_TYPE_CPND;
	evt->info.cpnd.type = CPND_EVT_TIME_OUT;
	evt->info.cpnd.info.tmr_info.cpnd_tmr = tmr;

	switch (tmr->type) {

			case CPND_TMR_TYPE_RETENTION:
				evt->info.cpnd.info.tmr_info.type = CPND_TMR_TYPE_RETENTION;
				evt->info.cpnd.info.tmr_info.ckpt_id = tmr->ckpt_id;
				break;
			case CPND_TMR_TYPE_NON_COLLOC_RETENTION:
				evt->info.cpnd.info.tmr_info.type = CPND_TMR_TYPE_NON_COLLOC_RETENTION;
				evt->info.cpnd.info.tmr_info.ckpt_id = tmr->ckpt_id;
				break;
			case CPND_TMR_TYPE_SEC_EXPI:
				evt->info.cpnd.info.tmr_info.type = CPND_TMR_TYPE_SEC_EXPI;
				evt->info.cpnd.info.tmr_info.lcl_sec_id = tmr->lcl_sec_id;
				evt->info.cpnd.info.tmr_info.ckpt_id = tmr->ckpt_id;
				break;

			case CPND_ALL_REPL_RSP_EXPI:
				evt->info.cpnd.info.tmr_info.type = CPND_ALL_REPL_RSP_EXPI;
				evt->info.cpnd.info.tmr_info.ckpt_id = tmr->ckpt_id;
				evt->info.cpnd.info.tmr_info.agent_dest = tmr->agent_dest;
				evt->info.cpnd.info.tmr_info.write_type = tmr->write_type;
				break;
			case CPND_TMR_OPEN_ACTIVE_SYNC:
				evt->info.cpnd.info.tmr_info.type = CPND_TMR_OPEN_ACTIVE_SYNC;
				evt->info.cpnd.info.tmr_info.ckpt_id = tmr->ckpt_id;
				evt->info.cpnd.info.tmr_info.invocation = tmr->invocation;
				evt->info.cpnd.info.tmr_info.sinfo = tmr->sinfo;
				evt->info.cpnd.info.tmr_info.lcl_ckpt_hdl = tmr->lcl_ckpt_hdl;
				break;
			default:
				m_LOG_CPND_CL(CPND_EVT_UNKNOWN, CPND_FC_EVT, NCSFL_SEV_ERROR, __FILE__, __LINE__);
				TRACE(" Invalid    tmr->type %d",  tmr->type);
				m_MMGR_FREE_CPSV_EVT(evt, NCS_SERVICE_ID_CPND);
				goto done;

	}

	/* Post the event to CPND Thread */
	m_NCS_IPC_SEND(&cb->cpnd_mbx, evt, priority);
done:

	ncshm_give_hdl(gl_cpnd_cb_hdl);
	return;
}

/****************************************************************************
 * Name          : cpnd_tmr_start
 *
 * Description   : This function which is used to start the CPND Timer
 *
 *****************************************************************************/
uns32 cpnd_tmr_start(CPND_TMR *tmr, SaTimeT duration)
{
	if (tmr->tmr_id == TMR_T_NULL) {
		m_NCS_TMR_CREATE(tmr->tmr_id, duration, cpnd_timer_expiry, (void *)tmr);
	}

	if (tmr->is_active == FALSE) {
		m_NCS_TMR_START(tmr->tmr_id, (uns32)duration, cpnd_timer_expiry, (void *)tmr);
		tmr->is_active = TRUE;
	} else if (tmr->is_active == TRUE) {
		m_NCS_TMR_STOP(tmr->tmr_id);
		m_NCS_TMR_START(tmr->tmr_id, (uns32)duration, cpnd_timer_expiry, (void *)tmr);
	}

	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : cpnd_tmr_stop
 *
 * Description   : This function which is used to stop the CPND Timer
 *
 * Arguments     : tmr      - Timer needs to be stoped.
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void cpnd_tmr_stop(CPND_TMR *tmr)
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
