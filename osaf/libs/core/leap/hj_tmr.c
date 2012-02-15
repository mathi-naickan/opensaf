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

  MODULE NAME: NCS_TMR.C

..............................................................................

  DESCRIPTION: Contains RP timer libirary, for optimization of timers.

******************************************************************************
*/

#include "ncs_tmr.h"

/**************************************************************************
* Function: ncs_rp_tmr_left_over
* 
* Purpose:  This is the function which will give the time left our in sec.
*
* Input: *tmr_cb  -  timer CB
*        tmr_id   -  timer ID
*  
* Returns:  uint32_t - timer difference
*
* Notes:  
**************************************************************************/
uint32_t ncs_rp_tmr_left_over(NCS_RP_TMR_CB *tmr_cb, NCS_RP_TMR_HDL tmr_id)
{
	NCS_RP_TMR_INFO *tmr_info;
	time_t now;
	uint32_t tmr_left = 0;

	tmr_info = (NCS_RP_TMR_INFO *)tmr_id;
	if (tmr_info == NULL)
		return (0);
	m_GET_TIME_STAMP(now);
	tmr_left = (uint32_t)difftime(now, tmr_info->tmr_trig_at);
	if (tmr_left < tmr_info->tmr_value)
		tmr_left = (tmr_info->tmr_value - tmr_left);
	else
		tmr_left = 0;

	return (tmr_left);
}

/**************************************************************************
* Function: rp_tmr_time_left_in_sec
* 
* Purpose:  This is the function which will the difference between the timer 
*           trigered and the present timer in seconds.
*
* Input: tmr_trig_at - This is the timer at which the timer is triggered, 
*                      which needs to get the difference with the present 
*                      timer.
*  
* Returns:  uint32_t - timer difference
*
* Notes:  
**************************************************************************/
uint32_t rp_tmr_time_left_in_sec(time_t tmr_trig_at, uint32_t tmr_value)
{
	uint32_t tmr_left = 0;
	time_t now;

	m_GET_TIME_STAMP(now);
	tmr_left = (uint32_t)difftime(now, tmr_trig_at);
	if (tmr_left < tmr_value)
		tmr_left = (tmr_value - tmr_left);
	else
		tmr_left = 0;

	return (tmr_left);
}

/**************************************************************************
* Function: ncs_rp_tmr_init
* 
* Purpose:  This is the function which will initialize the RP timer CB.
*
* Input: *tmr_init_info - This is the timer information with which the RP 
*                         timer CB needs to be initialized.
*  
* Returns:  NCS_RP_TMR_CB * - RP timer CB.
*
* Notes:  
**************************************************************************/
NCS_RP_TMR_CB *ncs_rp_tmr_init(NCS_RP_TMR_INIT *tmr_init_info)
{
	NCS_RP_TMR_CB *tmr_cb;

	tmr_cb = (NCS_RP_TMR_CB *)m_RP_TMR_ALLOC(sizeof(NCS_RP_TMR_CB), tmr_init_info->svc_id,
						 tmr_init_info->svc_sub_id);

	if (tmr_cb == NULL) {
		m_RP_TMR_LOG_MSG("ncs_rp_tmr_init Os alloc failed", 0);
		return (NULL);
	}

	memset(tmr_cb, 0, sizeof(NCS_RP_TMR_CB));

	tmr_cb->tmr_callback = tmr_init_info->tmr_callback;
	tmr_cb->callback_arg = tmr_init_info->callback_arg;
	tmr_cb->svc_id = tmr_init_info->svc_id;
	tmr_cb->svc_sub_id = tmr_init_info->svc_sub_id;
	tmr_cb->tmr_ganularity = tmr_init_info->tmr_ganularity;
	tmr_cb->active = false;

	/* initialize the lock */
	m_NCS_LOCK_INIT(&tmr_cb->tmr_lock);
	m_NCS_TMR_CREATE(tmr_cb->tmr_id, m_RP_TMR_DEF_PERIOD, tmr_cb->tmr_callback, tmr_cb->callback_arg);
	return (tmr_cb);
}

/**************************************************************************
* Function: ncs_rp_tmr_create
* 
* Purpose:  This is the function which is used to create and initalize the timer.
*
* Input: *tmr_cb - This is the RP timer CB.
*  
* Returns:  uint32_t  - timer Id, which needs to be refered further to access 
*                    that timer.
*
* Notes: It is all up to the RP to prevent redudenet timer start.
**************************************************************************/
NCS_RP_TMR_HDL ncs_rp_tmr_create(NCS_RP_TMR_CB *tmr_cb)
{
	NCS_RP_TMR_INFO *tmr_info;

	/* allocate an leaf node and add it to the bucket */
	tmr_info = (NCS_RP_TMR_INFO *)m_RP_TMR_ALLOC(sizeof(NCS_RP_TMR_INFO), tmr_cb->svc_id, tmr_cb->svc_sub_id);

	if (tmr_info == NULL) {
		m_RP_TMR_LOG_MSG("ncs_rp_tmr_start NCS_RP_TMR_INFO OS alloc failed", 0);
		return (0);
	}
	memset(tmr_info, 0, sizeof(NCS_RP_TMR_INFO));
	return ((NCS_RP_TMR_HDL)tmr_info);
}

/**************************************************************************
* Function: ncs_rp_tmr_start
* 
* Purpose:  This is the function which is used to start a timer..
*
* Input: *tmr_cb - RP timer CB.
*        tmr_id  - timer ID
*        period  - timer period
*        callbk  - time expiry call back
*        *arg    - argument
*
*
* Returns:  NCSCC_RC_SUCCESS/FAILURE
*
**************************************************************************/
uint32_t ncs_rp_tmr_start(NCS_RP_TMR_CB *tmr_cb, NCS_RP_TMR_HDL tmr_id, uint32_t period, RP_TMR_CALLBACK callbk, void *arg)
{
	NCS_RP_TMR_INFO *tmr_info;
	NCS_RP_TMR_INFO *tmr_list;
	NCS_RP_TMR_INFO *prev_info;
	uint32_t left_sec = 0;

	m_NCS_LOCK(&tmr_cb->tmr_lock, NCS_LOCK_WRITE);

	do {
		/* timer Id possible validation */
		if (tmr_id == NULL) {
			m_RP_TMR_LOG_MSG("ncs_rp_tmr_start got invalid timer id", tmr_id);
			break;
		}

		/* if the period is "0" then immediately invoke that callback */
		if (period == 0) {
			if ((callbk != NULL) && (arg != NULL)) {
				callbk(arg);
			}
			break;
		}
		tmr_list = tmr_cb->start_list;
		tmr_info = (NCS_RP_TMR_INFO *)tmr_id;

		if ((tmr_info->pnext != NULL) || (tmr_info->pprev != NULL) || (tmr_info->tmr_value != 0x0)) {
			m_RP_TMR_LOG_MSG("ncs_rp_tmr_start timer is already started", tmr_id);
			break;
		}
		tmr_info->callback_arg = arg;
		tmr_info->rp_tmr_callback = callbk;
		tmr_info->tmr_value = period;

		if (tmr_list == NULL) {
			/* this is the first time the timer is getting added to the node */
			/* create the timer list */
			m_RP_TMR_LOG_MSG("ncs_rp_tmr_start adding the timer to the first bucket", tmr_id);

			/* fill the tmr list info */

			tmr_info->callback_arg = arg;
			tmr_info->rp_tmr_callback = callbk;
			tmr_info->tmr_value = period;

			/* put in the CB start pointer */
			tmr_cb->start_list = tmr_info;

			m_GET_TIME_STAMP(tmr_info->tmr_trig_at);
			/* start the OS timer */
			m_RP_TMR_LOG_MSG("ncs_rp_tmr_start Os timer Started", period);
			if (tmr_cb->active == false) {
				m_NCS_TMR_START(tmr_cb->tmr_id,
						(period * 100), tmr_cb->tmr_callback, tmr_cb->callback_arg);
				tmr_cb->active = true;
			}
		} else {
			/* check whether the time period for the give timer is same as the 
			 * timer already available
			 */
			/* check whether the first trigered timer is greater than the new 
			 * received timer
			 */
			left_sec = rp_tmr_time_left_in_sec(tmr_list->tmr_trig_at, tmr_list->tmr_value);

			if (left_sec > period) {
				/* the received timer is less that the current timer triggered 
				 * so stop the existing Os timer and start the Os timer with a 
				 * new value
				 */
				m_RP_TMR_LOG_MSG("ncs_rp_tmr_start got less than the existing tmr", tmr_id);

				/* fill the tmr bucket */

				tmr_info->callback_arg = arg;
				tmr_info->rp_tmr_callback = callbk;
				tmr_info->tmr_value = period;

				tmr_info->pnext = tmr_list;
				tmr_list->pprev = tmr_info;

				/* put this info in the CB */
				tmr_cb->start_list = tmr_info;

				m_GET_TIME_STAMP(tmr_info->tmr_trig_at);

				/* Stop the OS timer */
				m_RP_TMR_LOG_MSG("ncs_rp_tmr_start Os timer Stoped", 0);
				m_NCS_TMR_STOP(tmr_cb->tmr_id);
				tmr_cb->active = false;

				/* start the OS timer */
				m_RP_TMR_LOG_MSG("ncs_rp_tmr_start Os timer Started", period);
				if (tmr_cb->active == false) {
					m_NCS_TMR_START(tmr_cb->tmr_id,
							(period * 100), tmr_cb->tmr_callback, tmr_cb->callback_arg);
					tmr_cb->active = true;
				}

			} else {
				/* code will come here if we have a equal or greater timer, 
				 * then the present one */
				m_RP_TMR_LOG_MSG("ncs_rp_tmr_start got greater than the existing tmr", tmr_id);

				prev_info = tmr_list;
				for (; tmr_list != NULL; tmr_list = tmr_list->pnext) {
					left_sec = rp_tmr_time_left_in_sec(tmr_list->tmr_trig_at, tmr_list->tmr_value);
					if (left_sec >= period) {
						/* insert my node */
						if (tmr_list->pnext != NULL) {
							tmr_list->pnext->pprev = tmr_info;
							tmr_info->pnext = tmr_list->pnext;
						}
						tmr_list->pnext = tmr_info;
						tmr_info->pprev = tmr_list;
						break;
					}
					prev_info = tmr_list;
				}

				if (tmr_list == NULL) {
					/* Timer node is fit in the last bucket */
					/* since the next this guy is greater then me, i need to be 
					 * added in between this guy and previous guy */

					/* fill the tmr bucket */

					tmr_info->callback_arg = arg;
					tmr_info->rp_tmr_callback = callbk;
					tmr_info->tmr_value = period;
					prev_info->pnext = tmr_info;
					tmr_info->pprev = prev_info;
				}
				tmr_info->tmr_value = period;
				m_GET_TIME_STAMP(tmr_info->tmr_trig_at);
			}
		}
	} while (0);
	m_NCS_UNLOCK(&tmr_cb->tmr_lock, NCS_LOCK_WRITE);
	return (NCSCC_RC_SUCCESS);
}

/**************************************************************************
* Function: ncs_rp_tmr_stop
* 
* Purpose:  This is the function which is used to start a timer..
*
* Input: *tmr_cb - RP timer CB.
*        tmr_id  - timer ID.
*  
*
* Returns:  NCSCC_RC_SUCCESS/FAILURE
*
**************************************************************************/
uint32_t ncs_rp_tmr_stop(NCS_RP_TMR_CB *tmr_cb, NCS_RP_TMR_HDL tmr_id)
{
	NCS_RP_TMR_INFO *tmr_info;
	uint32_t period = 0;
	uint32_t res = NCSCC_RC_SUCCESS;

	m_NCS_LOCK(&tmr_cb->tmr_lock, NCS_LOCK_WRITE);
	do {
		tmr_info = (NCS_RP_TMR_INFO *)tmr_id;
		if ((tmr_info == NULL) || ((tmr_info->pnext == NULL) &&
					   (tmr_info->pprev == NULL) && (tmr_info->tmr_value == 0x0))) {
			m_RP_TMR_LOG_MSG("ncs_rp_tmr_stop timer is not started already", tmr_info);
			res = NCSCC_RC_FAILURE;
			break;
		}
		if (tmr_info->pprev == NULL) {
			m_RP_TMR_LOG_MSG("ncs_rp_tmr_stop Os timer Stoped", 0);
			m_NCS_TMR_STOP(tmr_cb->tmr_id);
			tmr_cb->active = false;
			/* iam the first node attached to the bucket */
			tmr_cb->start_list = tmr_info->pnext;
			if (tmr_info->pnext != NULL) {
				tmr_info->pnext->pprev = NULL;
				period = rp_tmr_time_left_in_sec(tmr_info->pnext->tmr_trig_at,
								 tmr_info->pnext->tmr_value);
				/*(tmr_cb->start_buck->tmr_value - rp_tmr_time_left_in_sec(
				   tmr_cb->start_buck->tmr_trig_at, tmr_cb->start_buck->tmr_value)); */
				m_RP_TMR_LOG_MSG("ncs_rp_tmr_stop Os timer Started", period);
				if (tmr_cb->active == false) {
					m_NCS_TMR_START(tmr_cb->tmr_id, (period * 100), tmr_cb->tmr_callback,
							tmr_cb->callback_arg);
					tmr_cb->active = true;
				}
			}
		} else {
			/* iam the middle or the last node */
			tmr_info->pprev->pnext = tmr_info->pnext;
			if (tmr_info->pnext != NULL) {	/* iam the last node */
				tmr_info->pnext->pprev = tmr_info->pprev;
			}
		}
		tmr_info->pnext = NULL;
		tmr_info->pprev = NULL;
		tmr_info->tmr_value = 0x0;
	} while (0);
	m_NCS_UNLOCK(&tmr_cb->tmr_lock, NCS_LOCK_WRITE);
	return (res);
}

/**************************************************************************
* Function: ncs_rp_tmr_delete
* 
* Purpose:  This is the function which is used free the timer.
*
* Input: *tmr_cb - RP timer CB.
*        tmr_id  - timer ID.
*  
*
* Returns:  NCSCC_RC_SUCCESS/FAILURE
*
**************************************************************************/
uint32_t ncs_rp_tmr_delete(NCS_RP_TMR_CB *tmr_cb, NCS_RP_TMR_HDL tmr_id)
{
	NCS_RP_TMR_INFO *tmr_info;
	uint32_t res = NCSCC_RC_SUCCESS;

	m_NCS_LOCK(&tmr_cb->tmr_lock, NCS_LOCK_WRITE);
	do {
		tmr_info = (NCS_RP_TMR_INFO *)tmr_id;
		if (tmr_info == NULL) {
			m_RP_TMR_LOG_MSG("ncs_rp_tmr_delete timer is not started already", tmr_info);
			res = NCSCC_RC_FAILURE;
			break;
		}

		if ((tmr_info->pnext != NULL) || (tmr_info->pprev != NULL) || (tmr_info->tmr_value != 0)) {
			m_RP_TMR_LOG_MSG("ncs_rp_tmr_delete timer timer is not stoped", tmr_info);
			ncs_rp_tmr_stop(tmr_cb, tmr_id);
		}
		m_RP_TMR_FREE(tmr_info, tmr_cb->svc_id, tmr_cb->svc_sub_id);
	} while (0);
	m_NCS_UNLOCK(&tmr_cb->tmr_lock, NCS_LOCK_WRITE);
	return (res);
}

/**************************************************************************
* Function: ncs_rp_tmr_destory
* 
* Purpose:  This is the function which is used destory the RP timer.
*
* Input: **pptmr_cb - RP timer CB.
*  
*
* Returns:  NCSCC_RC_SUCCESS/FAILURE
*
**************************************************************************/
uint32_t ncs_rp_tmr_destory(NCS_RP_TMR_CB **pptmr_cb)
{
	NCS_RP_TMR_INFO *tmr_info;
	NCS_RP_TMR_INFO *pres_tmr_info;
	NCS_RP_TMR_CB *tmr_cb = *pptmr_cb;

	m_NCS_LOCK(&tmr_cb->tmr_lock, NCS_LOCK_WRITE);
	do {
		tmr_info = tmr_cb->start_list;
		/* Stop the OS timer */
		m_RP_TMR_LOG_MSG("ncs_rp_tmr_destory Os timer Stoped", 0);
		m_NCS_TMR_STOP(tmr_cb->tmr_id);
		tmr_cb->active = false;
		m_RP_TMR_LOG_MSG("ncs_rp_tmr_destory Os timer destroyed", 0);
		m_NCS_TMR_DESTROY(tmr_cb->tmr_id);

		/* remove all the buckets and node */
		pres_tmr_info = tmr_info;
		for (; tmr_info != NULL;) {
			tmr_info = tmr_info->pnext;
			m_RP_TMR_FREE(pres_tmr_info, tmr_cb->svc_id, tmr_cb->svc_sub_id);
			pres_tmr_info = tmr_info;
		}

	} while (0);
	m_NCS_UNLOCK(&tmr_cb->tmr_lock, NCS_LOCK_WRITE);
	/* destroy the lock */
	m_NCS_LOCK_DESTROY(&tmr_cb->tmr_lock);
	/* free the RP control block */
	m_RP_TMR_FREE(tmr_cb, tmr_cb->svc_id, tmr_cb->svc_sub_id);
	*pptmr_cb = NULL;
	return (NCSCC_RC_SUCCESS);
}

/**************************************************************************
* Function: ncs_rp_tmr_exp
* 
* Purpose:  This is the function which will be called when the timer 
*           expiries.
*
* Input: *tmr_cb - RP timer CB.
*  
*
* Returns:  NCSCC_RC_SUCCESS/FAILURE
*
**************************************************************************/
uint32_t ncs_rp_tmr_exp(NCS_RP_TMR_CB *tmr_cb)
{
	NCS_RP_TMR_INFO *tmr_list;
	NCS_RP_TMR_INFO *prev_tmr_info;
	RP_TMR_CALLBACK call_back;
	void *arg;
	uint32_t tmr_diff = 0;
	uint32_t res = NCSCC_RC_SUCCESS;

	m_NCS_LOCK(&tmr_cb->tmr_lock, NCS_LOCK_WRITE);
	do {
		if ((tmr_cb == NULL) || (tmr_cb->start_list == NULL)) {
			m_RP_TMR_LOG_MSG("ncs_rp_tmr_exp received post rp timer destroy call", 0);
			res = NCSCC_RC_FAILURE;
			break;
		}
		m_RP_TMR_LOG_MSG("ncs_rp_tmr_exp received successfully", 0);

		tmr_list = tmr_cb->start_list;
		prev_tmr_info = tmr_list;

		m_NCS_TMR_STOP(tmr_cb->tmr_id);
		/* now OS timer is in active */
		tmr_cb->active = false;
		for (; tmr_list != NULL;) {
			tmr_diff = rp_tmr_time_left_in_sec(tmr_list->tmr_trig_at, tmr_list->tmr_value);
			/* if (tmr_diff >= tmr_list->tmr_value) */
			if (tmr_diff == 0) {

				m_RP_TMR_LOG_MSG("ncs_rp_tmr_exp time difference is", tmr_diff);
				m_RP_TMR_LOG_MSG("ncs_rp_tmr_exp tmr expiry is called", tmr_list);

				call_back = tmr_list->rp_tmr_callback;
				arg = tmr_list->callback_arg;
				if (tmr_list->pprev == NULL) {
					tmr_cb->start_list = tmr_list->pnext;
				} else {
					tmr_list->pprev->pnext = tmr_list->pnext;
				}
				if (tmr_list->pnext != NULL)
					tmr_list->pnext->pprev = tmr_list->pprev;

				prev_tmr_info = tmr_list;
				tmr_list = tmr_list->pnext;
				/* poison the pointers */
				prev_tmr_info->pnext = NULL;
				prev_tmr_info->pprev = NULL;
				prev_tmr_info->tmr_value = 0;	/* this will identify for repeat STOP */

				/* need to call the timer callback . But before calling unlock the lock 
				   as it may lead to deadlock with other thread if the callback function 
				   acquires someother lock */

				m_NCS_UNLOCK(&tmr_cb->tmr_lock, NCS_LOCK_WRITE);
				call_back(arg);
				m_NCS_LOCK(&tmr_cb->tmr_lock, NCS_LOCK_WRITE);

			} else {
				break;
			}
		}
		if (tmr_cb->start_list != NULL) {
			tmr_diff = rp_tmr_time_left_in_sec(tmr_cb->start_list->tmr_trig_at,
							   tmr_cb->start_list->tmr_value);

			/* start the OS timer */
			m_RP_TMR_LOG_MSG("ncs_rp_tmr_exp tmr expiry is called", prev_tmr_info);
			m_RP_TMR_LOG_MSG("ncs_rp_tmr_stop Os timer Started", tmr_diff);
			if (tmr_cb->active == false) {
				m_NCS_TMR_START(tmr_cb->tmr_id,
						(tmr_diff * 100), tmr_cb->tmr_callback, tmr_cb->callback_arg);
				tmr_cb->active = true;
			}
		}
	} while (0);
	m_NCS_UNLOCK(&tmr_cb->tmr_lock, NCS_LOCK_WRITE);
	return (res);
}
