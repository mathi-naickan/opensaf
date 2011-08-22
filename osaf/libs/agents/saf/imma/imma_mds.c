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
 * Author(s): Ericsson AB
 *
 */

/*****************************************************************************

  DESCRIPTION:

  This file contains routines used by IMMA library for MDS Interface.

*****************************************************************************/

#include "imma.h"
#include "ncs_util.h"

static uns32 imma_mds_enc_flat(IMMA_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info);
static uns32 imma_mds_dec_flat(IMMA_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info);
static uns32 imma_mds_rcv(IMMA_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info);
static uns32 imma_mds_svc_evt(IMMA_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt);
static uns32 imma_mds_get_handle(IMMA_CB *cb);
static uns32 imma_mds_enc(IMMA_CB *cb, MDS_CALLBACK_ENC_INFO *info);
static uns32 imma_mds_dec(IMMA_CB *cb, MDS_CALLBACK_DEC_INFO *info);

/* Message Format Verion Tables */
MDS_CLIENT_MSG_FORMAT_VER imma_immnd_msg_fmt_table[IMMA_WRT_IMMND_SUBPART_VER_RANGE] = { 1 };

MDS_CLIENT_MSG_FORMAT_VER imma_immd_msg_fmt_table[IMMA_WRT_IMMD_SUBPART_VER_RANGE] = { 1 };

/****************************************************************************
 * Name          : imma_mds_get_handle
 *
 * Description   : This function Gets the Handles of local MDS destination
 *
 * Arguments     : cb   : IMMA control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

uns32 imma_mds_get_handle(IMMA_CB *cb)
{
	NCSADA_INFO arg;
	uns32 rc;

	memset(&arg, 0, sizeof(NCSADA_INFO));
	arg.req = NCSADA_GET_HDLS;
	rc = ncsada_api(&arg);

	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_3("Failed to get mds handle");
		return rc;
	}
	cb->imma_mds_hdl = arg.info.adest_get_hdls.o_mds_pwe1_hdl;
	return rc;
}

/****************************************************************************
  Name          : imma_mds_register
 
  Description   : This routine registers the IMMA Service with MDS.
 
  Arguments     : imma_cb - ptr to the IMMA control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

uns32 imma_mds_register(IMMA_CB *cb)
{
	NCSMDS_INFO svc_info;
	MDS_SVC_ID subs_id[1] = { NCSMDS_SVC_ID_IMMND };
	uns32 rc = NCSCC_RC_SUCCESS;

	/* STEP1: Get the MDS Handle */
	if (imma_mds_get_handle(cb) != NCSCC_RC_SUCCESS)
		return NCSCC_RC_FAILURE;

	/* memset the svc_info */
	memset(&svc_info, 0, sizeof(NCSMDS_INFO));

	/* STEP 2 : Install on ADEST with MDS with service ID NCSMDS_SVC_ID_IMMA. */
	svc_info.i_mds_hdl = cb->imma_mds_hdl;
	svc_info.i_svc_id = cb->sv_id;
	svc_info.i_op = MDS_INSTALL;

	svc_info.info.svc_install.i_yr_svc_hdl = 0;

	/* node specific */
	svc_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_INTRANODE;

	/* callback */
	svc_info.info.svc_install.i_svc_cb = imma_mds_callback;

	/* IMMA owns the mds queue */
	svc_info.info.svc_install.i_mds_q_ownership = FALSE;

	if ((rc = ncsmds_api(&svc_info)) != NCSCC_RC_SUCCESS) {
		TRACE_3("mds register A failed rc:%u", rc);
		return rc;
	}
	cb->imma_mds_dest = svc_info.info.svc_install.o_dest;

	/* STEP 3: Subscribe to IMMND up/down events */
	svc_info.i_op = MDS_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_INTRANODE;
	svc_info.info.svc_subscribe.i_svc_ids = &subs_id[0];

	if ((rc = ncsmds_api(&svc_info)) != NCSCC_RC_SUCCESS) {
		TRACE_3("mds register B failed rc:%u", rc);
		goto error;
	}

	return NCSCC_RC_SUCCESS;

 error:
	/* Uninstall with the mds */
	imma_mds_unregister(cb);
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : imma_mds_unreg
 *
 * Description   : This function un-registers the IMMA Service with MDS.
 *
 * Arguments     : cb   : IMMA control Block pointer.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/
void imma_mds_unregister(IMMA_CB *cb)
{
	NCSMDS_INFO arg;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* Un-install your service into MDS. 
	   No need to cancel the services that are subscribed */
	memset(&arg, 0, sizeof(NCSMDS_INFO));

	arg.i_mds_hdl = cb->imma_mds_hdl;
	arg.i_svc_id = cb->sv_id;
	arg.i_op = MDS_UNINSTALL;

	rc = ncsmds_api(&arg);

	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_3("MDS unregister failed");
	}
	return;
}

/****************************************************************************
  Name          : imma_mds_callback
 
  Description   : This callback routine will be called by MDS on event arrival
 
  Arguments     : info - pointer to the mds callback info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 imma_mds_callback(struct ncsmds_callback_info *info)
{
	IMMA_CB *cb = &imma_cb;
	uns32 rc = NCSCC_RC_FAILURE;

	if (info == NULL)
		return rc;

	switch (info->i_op) {
	case MDS_CALLBACK_COPY:
		rc = NCSCC_RC_FAILURE;
		break;
	case MDS_CALLBACK_ENC_FLAT:
		if (1) {
			rc = imma_mds_enc_flat(cb, &info->info.enc_flat);
		} else {
			rc = imma_mds_enc(cb, &info->info.enc_flat);
		}
		break;

	case MDS_CALLBACK_DEC_FLAT:
		if (1) {
			rc = imma_mds_dec_flat(cb, &info->info.dec_flat);
		} else {
			rc = imma_mds_dec(cb, &info->info.dec_flat);
		}
		break;

	case MDS_CALLBACK_RECEIVE:
		rc = imma_mds_rcv(cb, &info->info.receive);
		break;

	case MDS_CALLBACK_SVC_EVENT:
		rc = imma_mds_svc_evt(cb, &info->info.svc_evt);
		break;

	case MDS_CALLBACK_ENC:
		/*ABT: This case should never occurr. IMMA should only communicate
		   with IMMND, which is always on same processor, which means enc_flat
		   should always be the case. BUT FEVS messsages have to be encoded
		   for multiple hops => we have to encode non-flat. */
		/*rc = imma_mds_enc_flat(cb, &info->info.enc_flat); */
		rc = imma_mds_enc(cb, &info->info.enc);
		break;

	case MDS_CALLBACK_DEC:
		rc = imma_mds_dec(cb, &info->info.dec);
		break;

	default:
		TRACE_3("Invalid MDS callback");
		break;
	}

	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_1("MDS callback failed rc:%u", rc);
	}

	return rc;
}

/****************************************************************************
  Name          : imma_mds_enc_flat
 
  Description   : This function encodes an events sent from IMMA.
 
  Arguments     : cb    : IMMA control Block.
                  enc_info  : Info for encoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uns32 imma_mds_enc_flat(IMMA_CB *cb, MDS_CALLBACK_ENC_FLAT_INFO *info)
{
	IMMSV_EVT *evt;
	NCS_UBAID *uba = info->io_uba;
	uns32 rc = NCSCC_RC_SUCCESS;

	if (info->i_to_svc_id == NCSMDS_SVC_ID_IMMND) {
		info->o_msg_fmt_ver =
		    m_NCS_ENC_MSG_FMT_GET(info->i_rem_svc_pvt_ver,
					  IMMA_WRT_IMMND_SUBPART_VER_MIN,
					  IMMA_WRT_IMMND_SUBPART_VER_MAX, imma_immnd_msg_fmt_table);
#if 0				/*ABT DOES NOT WORK */
		if (!info->o_msg_fmt_ver) {
			TRACE("INCOMPATIBLE  MSG FORMAT IN ENCODE FLAT VER %d "
			      "IMMA->IMMND\n", info->i_rem_svc_pvt_ver);
			return NCSCC_RC_FAILURE;
		}
#endif   /*ABT DOES NOT WORK */

		/* as all the event structures are flat */
		evt = (IMMSV_EVT *)info->i_msg;
		rc = immsv_evt_enc_flat( /*&cb->edu_hdl, */ evt, uba);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_3("Mds encode flat failed rc:%u", rc);
		}
		return rc;
	}

	TRACE_1("Message encoded by IMMA should be for IMMND only");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : imma_mds_dec_flat
 
  Description   : This function decodes an events sent to IMMA.
 
  Arguments     : cb    : IMMA control Block.
                  dec_info  : Info for decoding
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uns32 imma_mds_dec_flat(IMMA_CB *cb, MDS_CALLBACK_DEC_FLAT_INFO *info)
{
	IMMSV_EVT *evt;
	NCS_UBAID *uba = info->io_uba;
	uns32 rc = NCSCC_RC_SUCCESS;

#if 0				/*NOTE ABT DOES NOT WORK */
	if (info->i_fr_svc_id == NCSMDS_SVC_ID_IMMND) {
		if (!m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
					       IMMA_WRT_IMMND_SUBPART_VER_MIN,
					       IMMA_WRT_IMMND_SUBPART_VER_MAX, imma_immnd_msg_fmt_table)) {
			TRACE_1("INVALID MSG FORMAT VERSION IN DECODE FLAT,VER %d "
				"for IMMND->IMMA\n", info->i_msg_fmt_ver);
			return NCSCC_RC_FAILURE;
		}
	} else if (info->i_fr_svc_id == NCSMDS_SVC_ID_IMMD) {
		if (!m_NCS_MSG_FORMAT_IS_VALID(info->i_msg_fmt_ver,
					       IMMA_WRT_IMMD_SUBPART_VER_MIN,
					       IMMA_WRT_IMMD_SUBPART_VER_MAX, imma_immd_msg_fmt_table)) {
			TRACE_1("INVALID MSG FORMAT VERSION IN DECODE FLAT,VER %d "
				"for IMMD->IMMA\n", info->i_msg_fmt_ver);
			return NCSCC_RC_FAILURE;
		}
	}
#endif   /*ABT DOES NOT WORK */

	if (info->i_fr_svc_id == NCSMDS_SVC_ID_IMMND || info->i_fr_svc_id == NCSMDS_SVC_ID_IMMD) {
		evt = (IMMSV_EVT *)calloc(1, sizeof(IMMSV_EVT));
		if (evt == NULL) {
			TRACE_3("Memory allocation failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		info->o_msg = evt;
		rc = immsv_evt_dec_flat(uba, evt);
		if (rc != NCSCC_RC_SUCCESS) {
			free(evt);
			info->o_msg = NULL;
		}
		return rc;
	}

	TRACE_3("IMMA MDS received messsage from unexpected sender");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : imma_mds_rcv
 *
 * Description   : MDS will call this function on receiving IMMA.
 *
 * Arguments     : cb - IMMA Control Block
 *                 rcv_info - MDS Receive information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 imma_mds_rcv(IMMA_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info)
{
	IMMSV_EVT *evt = (IMMSV_EVT *)rcv_info->i_msg;

	evt->sinfo.ctxt = rcv_info->i_msg_ctxt;
	evt->sinfo.dest = rcv_info->i_fr_dest;
	evt->sinfo.to_svc = rcv_info->i_fr_svc_id;

	/* Process the received event at IMMA */
	imma_process_evt(cb, evt);

	/*Note: Pointer structures are freed in imma_proc_free_pointers  */
	free(evt);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : imma_mds_svc_evt
 *
 * Description   : IMMA is informed when MDS events occurr that he has 
 *                 subscribed to
 *
 * Arguments     : 
 *   cb          : IMMA control Block.
 *   svc_evt    : Svc evt info.
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 imma_mds_svc_evt(IMMA_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *svc_evt)
{
    NCS_BOOL locked = FALSE;
	switch (svc_evt->i_change) {
	case NCSMDS_DOWN:
		TRACE_3("IMMND DOWN");
		m_NCS_LOCK(&cb->immnd_sync_lock,NCS_LOCK_WRITE);
		cb->is_immnd_up = FALSE; 
		m_NCS_UNLOCK(&cb->immnd_sync_lock,NCS_LOCK_WRITE);

        cb->dispatch_clients_to_resurrect = 0; /* Stop active resurrections */
		if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE)!=NCSCC_RC_SUCCESS) {
            TRACE_4("Locking failed");
            assert(0);
        }
        locked = TRUE;
		imma_mark_clients_stale(cb);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
        locked = FALSE;
		break;

	case NCSMDS_UP:
		TRACE_3("IMMND UP");
		m_NCS_LOCK(&cb->immnd_sync_lock,NCS_LOCK_WRITE);/*special sync lock*/
		cb->is_immnd_up = TRUE;
		cb->immnd_mds_dest = svc_evt->i_dest;
		if (cb->immnd_sync_awaited == TRUE)
			m_NCS_SEL_OBJ_IND(cb->immnd_sync_sel);
		m_NCS_UNLOCK(&cb->immnd_sync_lock,NCS_LOCK_WRITE);/*special sync lock*/

		if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE)!=NCSCC_RC_SUCCESS){
            TRACE_4("Locking failed");
            assert(0);
        }
        locked = TRUE;
        /* Check again if some clients have been exposed during down time. 
           Also determine if there are candidates for active resurrection.
           Inform IMMND of highest used client id. Increases chances of success
           in resurrections. 
         */
        imma_determine_clients_to_resurrect(cb, &locked);
        if (!locked) {
            if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
                TRACE_4("Locking failed");
                assert(0);
            }
            locked = TRUE;
        }
        /*imma_process_stale_clients(cb);
	  The active resurrect is postponed until after immnd sync is 
	  completed. imma_process_stale_clients now invoked in imma_proc.c
	  when event IMMA_EVT_ND2A_PROC_STALE_CLIENTS is received.
	 */

        m_NCS_UNLOCK(&cb->cb_lock,NCS_LOCK_WRITE);
        locked = FALSE;
		break;

	default:
		break;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : imma_mds_enc

  Description   : This function encodes events sent from IMMA to remote IMMND.

  Arguments     : cb    : IMMA control Block.
                  info  : Info for encoding

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uns32 imma_mds_enc(IMMA_CB *cb, MDS_CALLBACK_ENC_INFO *enc_info)
{
	IMMSV_EVT *evt;
	NCS_UBAID *uba = enc_info->io_uba;

	if (enc_info->i_to_svc_id == NCSMDS_SVC_ID_IMMND) {
		enc_info->o_msg_fmt_ver =
		    m_NCS_ENC_MSG_FMT_GET(enc_info->i_rem_svc_pvt_ver,
					  IMMA_WRT_IMMND_SUBPART_VER_MIN,
					  IMMA_WRT_IMMND_SUBPART_VER_MAX, imma_immnd_msg_fmt_table);
	}
#if 0				/*ABT DOES NOT WORK */
	if (!enc_info->o_msg_fmt_ver) {
		TRACE_1("INCOMPATIBLE MSG FORMAT IN ENCODE FULL VER %d IMMA->IMMND\n", enc_info->i_rem_svc_pvt_ver);
		return NCSCC_RC_FAILURE;
	}
#endif   /*ABT DOES NOT WORK */

	evt = (IMMSV_EVT *)enc_info->i_msg;

	return immsv_evt_enc( /*&cb->edu_hdl, */ evt, uba);
}

/****************************************************************************
  Name          : imma_mds_dec

  Description   : This function decodes an events sent to IMMA.

  Arguments     : cb    : IMMA control Block.
                  info  : Info for decoding

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
static uns32 imma_mds_dec(IMMA_CB *cb, MDS_CALLBACK_DEC_INFO *dec_info)
{
	IMMSV_EVT *evt;
	NCS_UBAID *uba = dec_info->io_uba;
	uns32 rc = NCSCC_RC_SUCCESS;

#if 0				/*ABT DOES NOT WORK */
	if (dec_info->i_fr_svc_id == NCSMDS_SVC_ID_IMMND) {
		if (!m_NCS_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
					       IMMA_WRT_IMMND_SUBPART_VER_MIN,
					       IMMA_WRT_IMMND_SUBPART_VER_MAX, imma_immnd_msg_fmt_table)) {
			TRACE_1("INVALID MSG FORMAT VERSION IN DECODE FULL: %d IMMND->IMMA\n", dec_info->i_msg_fmt_ver);
			return NCSCC_RC_FAILURE;

		}
	} else if (dec_info->i_fr_svc_id == NCSMDS_SVC_ID_IMMD) {
		if (!m_NCS_MSG_FORMAT_IS_VALID(dec_info->i_msg_fmt_ver,
					       IMMA_WRT_IMMD_SUBPART_VER_MIN,
					       IMMA_WRT_IMMD_SUBPART_VER_MAX, imma_immd_msg_fmt_table)) {
			TRACE_1("INVALID MSG FORMAT VERSION IN DECODE FULL: %d IMMD->IMMA\n", dec_info->i_msg_fmt_ver);
			return NCSCC_RC_FAILURE;
		}
	}
#endif   /*ABT DOES NOT WORK */

	evt = calloc(1, sizeof(IMMSV_EVT));	/* calloc zeroes memory */
	if (!evt)
		return NCSCC_RC_FAILURE;

	dec_info->o_msg = (NCSCONTEXT)evt;

	rc = immsv_evt_dec( /*&cb->edu_hdl, */ uba, evt);

	if (rc != NCSCC_RC_SUCCESS) {
		free(dec_info->o_msg);
		dec_info->o_msg = NULL;
	}
	return rc;
}

/****************************************************************************
  Name          : imma_mds_msg_sync_send
 
  Description   : This routine sends the IMMA message to IMMND.
 
  Arguments     :
                  uns32 imma_mds_hdl Handle of IMMA
                  MDS_DEST  *destination - destintion to send to
                  IMMSV_EVT   *i_evt - IMMSV_EVT pointer
                  IMMSV_EVT   **o_evt - IMMSV_EVT pointer to result data
                  timeout - timeout value in 10 ms 
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 imma_mds_msg_sync_send(uns32 imma_mds_hdl,
			     MDS_DEST *destination, IMMSV_EVT *i_evt, IMMSV_EVT **o_evt, uns32 timeout)
{
	IMMA_CB *cb = &imma_cb;
	NCSMDS_INFO mds_info;
	uns32 rc;

	if (!i_evt)
		return NCSCC_RC_FAILURE;

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = imma_mds_hdl;
	mds_info.i_svc_id = cb->sv_id;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_IMMND;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SNDRSP;

	/* fill the send rsp strcuture */

	/* timeto wait in 10ms */
	mds_info.info.svc_send.info.sndrsp.i_time_to_wait = timeout;
	mds_info.info.svc_send.info.sndrsp.i_to_dest = *destination;

	/* send the message */
	rc = ncsmds_api(&mds_info);

	if (rc == NCSCC_RC_SUCCESS)
		*o_evt = mds_info.info.svc_send.info.sndrsp.o_rsp;

	return rc;
}

/****************************************************************************
  Name          : imma_mds_msg_send
 
  Description   : This routine sends the IMMA message to IMMND.
 
  Arguments     : uns32 imma_mds_hdl Handle of IMMA
                  MDS_DEST  *destination - destintion to send to
                  IMMSV_EVT   *i_evt - IMMSV_EVT pointer
                  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 imma_mds_msg_send(uns32 imma_mds_hdl, MDS_DEST *destination, IMMSV_EVT *i_evt, uns32 to_svc)
{

	NCSMDS_INFO mds_info;
	uns32 rc;
	IMMA_CB *cb = &imma_cb;

	if (!i_evt)
		return NCSCC_RC_FAILURE;

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = imma_mds_hdl;
	mds_info.i_svc_id = cb->sv_id;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_send.i_to_svc = to_svc;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;

	/* fill the send rsp strcuture */
	mds_info.info.svc_send.info.snd.i_to_dest = *destination;

	/* send the message */
	rc = ncsmds_api(&mds_info);

	return rc;
}
