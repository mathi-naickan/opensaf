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

  This file contains the processing related to fail-over of AVD.
 
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "avnd.h"

static uns32 avnd_send_pg_start_on_fover(AVND_CB *cb);

/****************************************************************************
  Name          : avnd_send_pg_start_on_fover
 
  Description   : This routing will get called on AVD fail-over to send the
                  PG start messages to the new AVD.
 
  Arguments     : cb  - ptr to the AvND control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uns32 avnd_send_pg_start_on_fover(AVND_CB *cb)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVND_PG *pg = 0;
	SaNameT csi_name;

	memset(&csi_name, '\0', sizeof(SaNameT));

	while (NULL != (pg = m_AVND_PGDB_REC_GET_NEXT(cb->pgdb, csi_name))) {
		rc = avnd_di_pg_act_send(cb, &pg->csi_name, AVSV_PG_TRACK_ACT_START, TRUE);

		if (NCSCC_RC_SUCCESS != rc)
			break;

		csi_name = pg->csi_name;
	}
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_avd_verify_message
 
  Description   : This routine processes the data verify message sent by newly
                  Active AVD.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_evt_avd_verify_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_DND_LIST *list = &((cb)->dnd_list);
	AVND_DND_MSG_LIST *rec = 0, t_rec;
	AVSV_D2N_DATA_VERIFY_MSG_INFO *info;
	uns32 rcv_id;
	NCS_BOOL msg_found = FALSE;

	TRACE_ENTER();

	info = &evt->info.avd->msg_info.d2n_data_verify;

	m_AVND_LOG_FOVER_EVTS(NCSFL_SEV_NOTICE, AVND_LOG_FOVR_VERIFY_MSG_RCVD, NCSCC_RC_SUCCESS);

	rcv_id = info->rcv_id_cnt;

	/* Update some of the fields sent along with the verify message. */
	/* Send operation response for these two..TBD - Parag */
	cb->su_failover_max = info->su_failover_max;
	cb->su_failover_prob = info->su_failover_prob;

	m_AVND_LOG_FOVER_EVTS(NCSFL_SEV_DEBUG, AVND_LOG_FOVR_AVD_SND_ID_CNT, info->snd_id_cnt);

	m_AVND_LOG_FOVER_EVTS(NCSFL_SEV_DEBUG, AVND_LOG_FOVR_AVND_RCV_ID_CNT, cb->rcv_msg_id);

	/*
	 * Verify message ID received in the message. Send Ack if send ID count
	 * received matches with the AVND's receive message ID. Send NACK in case of
	 * mismatch. Reset the receive message ID count. Here after AVD is going to 
	 * Start with 0.
	 */
	avnd_di_ack_nack_msg_send(cb, info->snd_id_cnt, 0);
	/* Log error */

	/*
	 * We are done with use of rev_msg_id count. Now it is time
	 * re-set it since all new messages we are going to get with 
	 * the new counter value.
	 */
	cb->rcv_msg_id = 0;

	/*
	 * Validate send Message ID. Re-send all the messages in the AVND message
	 * retransmit queue for which message ID is greater than the received
	 * send message ID.
	 */
	m_AVND_LOG_FOVER_EVTS(NCSFL_SEV_DEBUG, AVND_LOG_FOVR_AVD_RCV_ID_CNT, info->rcv_id_cnt);

	m_AVND_LOG_FOVER_EVTS(NCSFL_SEV_DEBUG, AVND_LOG_FOVR_AVND_SND_ID_CNT, cb->snd_msg_id);

	/* 
	 * Send all the records in the queue, for which send ID is greater
	 * than the receive ID count received in the verify message.
	 * Otherwise this exercise also helps us in cleaning all the messages 
	 * currently pending in the Queue and are not acked.
	 */
	for (rec = list->head; NULL != rec;) {
		t_rec = *rec;

		/* 
		 * Since AVD is telling us that he has received till recv_id, we should
		 * always find msg with ID (rcv_id + 1). Delete and remove all the 
		 * messages which are there before this messages in the queue. Once we
		 * find this message with message id (rcv_id+1) then send all the messages
		 * there in the queue after that.
		 */
		if ((rcv_id + 1) > (*((uns32 *)(&rec->msg.info.avd->msg_info)))) {
			/* pop & delete */
			m_AVND_DIQ_REC_FIND_POP(cb, rec);
			m_AVND_LOG_FOVER_EVTS(NCSFL_SEV_DEBUG,
					      AVND_LOG_FOVR_REC_DEL, *((uns32 *)(&rec->msg.info.avd->msg_info)));
			avnd_diq_rec_del(cb, rec);
		} else {
			avnd_diq_rec_send(cb, rec);

			m_AVND_LOG_FOVER_EVTS(NCSFL_SEV_DEBUG,
					      AVND_LOG_FOVR_REC_SENT, *((uns32 *)(&rec->msg.info.avd->msg_info)));

			msg_found = TRUE;
		}

		rec = t_rec.next;
	}

	if ((cb->snd_msg_id != info->rcv_id_cnt) && (msg_found == FALSE)) {
		/* Log error, seems to be some problem. */
		m_AVND_LOG_FOVER_EVTS(NCSFL_SEV_EMERGENCY, AVND_LOG_FOVR_REC_NOT_FOUND, info->rcv_id_cnt);
		return NCSCC_RC_FAILURE;
	}

	/* 
	 * Send PG tracking (START) message to new Active.
	 */
	avnd_send_pg_start_on_fover(cb);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
