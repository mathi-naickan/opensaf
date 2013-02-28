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
#include "ntfs_com.h"
#include <alloca.h>
#include <time.h>
#include <limits.h>
#include "ntfs.h"
#include "ntfsv_enc_dec.h"
#include "ntfs_imcnutil.h"


#define m_NTFSV_FILL_ASYNC_UPDATE_FINALIZE(ckpt,client_id){ \
  ckpt.header.ckpt_rec_type=NTFS_CKPT_FINALIZE_REC; \
  ckpt.header.num_ckpt_records=1; \
  ckpt.header.data_len=1; \
  ckpt.ckpt_rec.finalize_rec.client_id= client_id;\
}

static uint32_t process_api_evt(ntfsv_ntfs_evt_t *evt);
static uint32_t proc_ntfa_updn_mds_msg(ntfsv_ntfs_evt_t *evt);
static uint32_t proc_mds_quiesced_ack_msg(ntfsv_ntfs_evt_t *evt);
static uint32_t proc_initialize_msg(ntfs_cb_t *, ntfsv_ntfs_evt_t *evt);
static uint32_t proc_finalize_msg(ntfs_cb_t *, ntfsv_ntfs_evt_t *evt);
static uint32_t proc_subscribe_msg(ntfs_cb_t *, ntfsv_ntfs_evt_t *evt);
static uint32_t proc_unsubscribe_msg(ntfs_cb_t *, ntfsv_ntfs_evt_t *evt);
static uint32_t proc_send_not_msg(ntfs_cb_t *, ntfsv_ntfs_evt_t *evt);
static uint32_t proc_reader_initialize_msg(ntfs_cb_t *, ntfsv_ntfs_evt_t *evt);
static uint32_t proc_reader_initialize_msg_2(ntfs_cb_t *, ntfsv_ntfs_evt_t *evt);
static uint32_t proc_reader_finalize_msg(ntfs_cb_t *, ntfsv_ntfs_evt_t *evt);
static uint32_t proc_read_next_msg(ntfs_cb_t *, ntfsv_ntfs_evt_t *evt);

static int ntf_version_is_valid(SaVersionT *ver) {
/* TODO: remove after upgrade to version A.02.01 */       
/* To be backward compatible during upgrade due to ticket (#544)(#634) */
	const SaVersionT alloved_ver = {'A', 0x02, 0x01};
	if (ver->releaseCode == alloved_ver.releaseCode &&
		ver->minorVersion == alloved_ver.minorVersion &&
		ver->majorVersion == alloved_ver.majorVersion)
		return 1;

	return((ver->releaseCode == NTF_RELEASE_CODE) && (0 < ver->majorVersion) && 
			 (ver->majorVersion <= NTF_MAJOR_VERSION));
}

static const
NTFSV_NTFS_EVT_HANDLER ntfs_ntfsv_top_level_evt_dispatch_tbl[NTFSV_NTFS_EVT_MAX] = {
	NULL,
	process_api_evt,
	proc_ntfa_updn_mds_msg,
	proc_ntfa_updn_mds_msg,
	proc_mds_quiesced_ack_msg
};

/* Dispatch table for NTFA_API realted messages */
static const
NTFSV_NTFS_NTFA_API_MSG_HANDLER ntfs_ntfa_api_msg_dispatcher[NTFSV_API_MAX] = {
	NULL,
	proc_initialize_msg,
	proc_finalize_msg,
	proc_subscribe_msg,
	proc_unsubscribe_msg,
	proc_send_not_msg,
	proc_reader_initialize_msg,
	proc_reader_finalize_msg,
	proc_read_next_msg,
	proc_reader_initialize_msg_2,
};

/****************************************************************************
 * Name          : proc_ntfa_updn_mds_msg
 *
 * Description   : This is the function which is called when ntfs receives any
 *                 a NTFA UP/DN message via MDS subscription.
 *
 * Arguments     : msg  - Message that was posted to the NTFS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t proc_ntfa_updn_mds_msg(ntfsv_ntfs_evt_t *evt)
{
	TRACE_ENTER();

	switch (evt->evt_type) {
	case NTFSV_NTFS_EVT_NTFA_UP:
		break;
	case NTFSV_NTFS_EVT_NTFA_DOWN:
		/* Remove this NTFA entry from our processing lists */
		clientRemoveMDS(evt->fr_dest);
		break;
	default:
		TRACE("Unknown evt type!!!");
		break;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : proc_mds_quiesced_ack_msg
 *
 * Description   : This is the function which is called when ntfs receives an
 *                       quiesced ack event from MDS 
 *
 * Arguments     : evt  - Message that was posted to the NTFS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t proc_mds_quiesced_ack_msg(ntfsv_ntfs_evt_t *evt)
{
	TRACE_ENTER();
	if (ntfs_cb->is_quisced_set == true) {
		ntfs_cb->ha_state = SA_AMF_HA_QUIESCED;
		/* Inform MBCSV of HA state change */
		if (ntfs_mbcsv_change_HA_state(ntfs_cb) != NCSCC_RC_SUCCESS)
			TRACE("ntfs_mbcsv_change_HA_state FAILED");

		/* Update control block */
		saAmfResponse(ntfs_cb->amf_hdl, ntfs_cb->amf_invocation_id, SA_AIS_OK);
		ntfs_cb->is_quisced_set = false;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : proc_rda_cb_msg
 *
 * Description   : This function processes the role change message from RDE.
 *                 It will change role to active if requested to.
 *
 * Arguments     : evt  - Message that was posted to the NTFS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t proc_rda_cb_msg(ntfsv_ntfs_evt_t *evt)
{
	uint32_t rc;

	TRACE_ENTER();

	if (evt->info.rda_info.io_role == PCS_RDA_ACTIVE) {
		SaAmfHAStateT old_ha_state = ntfs_cb->ha_state;
		LOG_NO("ACTIVE request");

		ntfs_cb->mds_role = V_DEST_RL_ACTIVE;
		if ((rc = ntfs_mds_change_role()) != NCSCC_RC_SUCCESS) {
			LOG_ER("ntfs_mds_change_role FAILED %u", rc);
			goto done;
		}

		ntfs_cb->ha_state = SA_AMF_HA_ACTIVE;
		if ((rc = ntfs_mbcsv_change_HA_state(ntfs_cb)) != NCSCC_RC_SUCCESS) {
			LOG_ER("ntfs_mbcsv_change_HA_state FAILED %u", rc);
			goto done;
		}

		restart_ntfimcn(ntfs_cb->ha_state);

		if (old_ha_state == SA_AMF_HA_STANDBY) {
			/* check for unsent notifictions and if notifiction is not logged */
			checkNotificationList();
		}
	}

	rc = NCSCC_RC_SUCCESS;

done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : ntfs_cb_init
 *
 * Description   : This function initializes the NTFS_CB including the 
 *                 Patricia trees.
 *                 
 *
 * Arguments     : ntfs_cb * - Pointer to the NTFS_CB.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t ntfs_cb_init(ntfs_cb_t *ntfs_cb)
{
	char *tmp;
	TRACE_ENTER();
	/* Assign Initial HA state */
	ntfs_cb->ha_state = NTFS_HA_INIT_STATE;
	ntfs_cb->csi_assigned = false;
	/* Assign Version. Currently, hardcoded, This will change later */
	ntfs_cb->ntf_version.releaseCode = NTF_RELEASE_CODE;
	ntfs_cb->ntf_version.majorVersion = NTF_MAJOR_VERSION;
	ntfs_cb->ntf_version.minorVersion = NTF_MINOR_VERSION;

	tmp = (char *)getenv("NTFSV_ENV_CACHE_SIZE");
	if (tmp) {
		ntfs_cb->cache_size =(unsigned int) atoi(tmp);
		TRACE("NTFSV_ENV_CACHE_SIZE configured value: %u", ntfs_cb->cache_size);
	} else {
		ntfs_cb->cache_size = NTFSV_READER_CACHE_DEFAULT; 
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

void update_standby(ntfsv_ckpt_msg_t *ckpt, uint32_t action)
{
	uint32_t async_rc = NCSCC_RC_SUCCESS;

	if (ntfs_cb->ha_state == SA_AMF_HA_ACTIVE) {
		async_rc = ntfs_send_async_update(ntfs_cb, ckpt, action);
		if (async_rc != NCSCC_RC_SUCCESS)
			TRACE("send_async_update FAILED");
	}
}

/**
 * Handle a initialize message
 * @param cb
 * @param evt
 * 
 * @return uns32
 */
static uint32_t proc_initialize_msg(ntfs_cb_t *cb, ntfsv_ntfs_evt_t *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAisErrorT ais_rc = SA_AIS_OK;
	SaVersionT *version;

	TRACE_ENTER2("dest %" PRIx64, evt->fr_dest);

	/* Validate the version */
	version = &(evt->info.msg.info.api_info.param.init.version);
	if (!ntf_version_is_valid(version)) {
		ais_rc = SA_AIS_ERR_VERSION;
		TRACE("version FAILED");
		client_added_res_lib(ais_rc, 0, evt->fr_dest, &evt->mds_ctxt);
		goto done;
	}
	clientAdded(0, evt->fr_dest, &evt->mds_ctxt);

 done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Handle an finalize message
 * @param cb
 * @param evt
 * 
 * @return uns32
 */
static uint32_t proc_finalize_msg(ntfs_cb_t *cb, ntfsv_ntfs_evt_t *evt)
{
	uint32_t client_id = evt->info.msg.info.api_info.param.finalize.client_id;

	TRACE_ENTER2("client_id %u", client_id);
	clientRemoved(client_id);
	client_removed_res_lib(SA_AIS_OK, client_id, evt->fr_dest, &evt->mds_ctxt);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : proc_subscribe_msg
 *
 * Description   : This is the function which is called when ntfs receives a
 *                 subscribe message.
 *
 * Arguments     : msg  - Message that was posted to the NTFS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t proc_subscribe_msg(ntfs_cb_t *cb, ntfsv_ntfs_evt_t *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	ntfsv_subscribe_req_t *subscribe_param = &(evt->info.msg.info.api_info.param.subscribe);

	TRACE_4("subscriptionId: %u", subscribe_param->subscriptionId);
	subscriptionAdded(*subscribe_param, &evt->mds_ctxt);

	TRACE_LEAVE();
	return rc;
}

/**
 * Handle a unsubscribe message
 * @param cb
 * @param evt
 * 
 * @return uns32
 */
static uint32_t proc_unsubscribe_msg(ntfs_cb_t *cb, ntfsv_ntfs_evt_t *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	ntfsv_unsubscribe_req_t *param = &(evt->info.msg.info.api_info.param.unsubscribe);

	TRACE_ENTER2("client_id %u, subscriptionId %u", param->client_id, param->subscriptionId);

	subscriptionRemoved(param->client_id, param->subscriptionId, &evt->mds_ctxt);
	TRACE_LEAVE();
	return rc;
}

static void print_header(SaNtfNotificationHeaderT *notificationHeader)
{
	SaTimeT totalTime;
	SaTimeT ntfTime = (SaTimeT)0;
	char time_str[24];

	/* Event type */
	TRACE_1("eventType = %d", (int)*notificationHeader->eventType);

	/* Notification Object */
	TRACE_1("notificationObject.length = %u\n", notificationHeader->notificationObject->length);

	/* Notifying Object */
	TRACE_1("notifyingObject->length = %u\n", notificationHeader->notifyingObject->length);

	/* Notification Class ID */
	TRACE_1("VendorID = %d\nmajorID = %d\nminorID = %d\n",
		notificationHeader->notificationClassId->vendorId,
		notificationHeader->notificationClassId->majorId, notificationHeader->notificationClassId->minorId);

	/* Event Time */
	ntfTime = *notificationHeader->eventTime;

	totalTime = (ntfTime / (SaTimeT)SA_TIME_ONE_SECOND);
	(void)strftime(time_str, sizeof(time_str), "%d-%m-%Y %T", localtime((const time_t *)&totalTime));

	TRACE_1("eventTime = %lld = %s\n", (SaTimeT)ntfTime, time_str);

	/* Notification ID */
	TRACE_1("notificationID = %llu\n", *notificationHeader->notificationId);

	/* Length of Additional text */
	TRACE_1("lengthadditionalText = %d\n", notificationHeader->lengthAdditionalText);

	/* Additional text */
	TRACE_1("additionalText = %s\n", notificationHeader->additionalText);
}

/****************************************************************************
 * Name          : proc_send_not_msg
 *
 * Description   : This is the function which is called when ntfs receives a
 *                 NTFSV_SEND_NOT_REQ message.
 *
 * Arguments     : msg  - Message that was posted to the Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t proc_send_not_msg(ntfs_cb_t *cb, ntfsv_ntfs_evt_t *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	ntfsv_send_not_req_t *param = evt->info.msg.info.api_info.param.send_notification;
	
	if (param->notificationType == SA_NTF_TYPE_ALARM) {
		print_header(&param->notification.alarm.notificationHeader);
	}
	notificationReceived(param->client_id, param->notificationType, param, &evt->mds_ctxt);

	/* The allocated resources in ntfsv_enc_dec.c is freed in the destructor in NtfNotification */
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : proc_reader_initialize_msg
 *
 * Description   : This is the function which is called when ntfs receives a
 *                 reader_initialize message.
 *
 * Arguments     : msg  - Message that was posted to the NTFS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t proc_reader_initialize_msg(ntfs_cb_t *cb, ntfsv_ntfs_evt_t *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	ntfsv_reader_init_req_t *reader_initialize_param = &(evt->info.msg.info.api_info.param.reader_init);

	TRACE_4("client_id: %u", reader_initialize_param->client_id);
	newReader(reader_initialize_param->client_id, reader_initialize_param->searchCriteria, NULL, &evt->mds_ctxt);
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : proc_reader_initialize_msg_2
 *
 * Description   : This is the function which is called when ntfs receives a
 *                 reader_initialize message. The reader_initialize_msg_2
 *                 includes filter.
 * Arguments     : msg  - Message that was posted to the NTFS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t proc_reader_initialize_msg_2(ntfs_cb_t *cb, ntfsv_ntfs_evt_t *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	ntfsv_reader_init_req_2_t *rp = &(evt->info.msg.info.api_info.param.reader_init_2);

	TRACE_4("client_id: %u", rp->head.client_id);
	newReader(rp->head.client_id, rp->head.searchCriteria, &rp->f_rec, &evt->mds_ctxt);
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : proc_reader_finalize_msg
 *
 * Description   : This is the function which is called when ntfs receives a
 *                 reader_finalize message.
 *
 * Arguments     : msg  - Message that was posted to the NTFS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t proc_reader_finalize_msg(ntfs_cb_t *cb, ntfsv_ntfs_evt_t *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	ntfsv_reader_finalize_req_t *reader_finalize_param = &(evt->info.msg.info.api_info.param.reader_finalize);

	TRACE_4("client_id: %u", reader_finalize_param->client_id);
	deleteReader(reader_finalize_param->client_id, reader_finalize_param->readerId, &evt->mds_ctxt);

/*  if (ais_rv == SA_AIS_OK)                                                 */
/*  {                                                                        */
/*     async_rc = ntfs_subscription_initialize_async_update(cb,              */
/*                                                  reader_finalize_param);*/
/*     if (async_rc != NCSCC_RC_SUCCESS)                                     */
/*        TRACE("ntfs_send_reader_finalize_async_update failed");          */
/*  }                                                                        */
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : proc_read_next_msg
 *
 * Description   : This is the function which is called when ntfs receives a
 *                 read_next message.
 *
 * Arguments     : msg  - Message that was posted to the NTFS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t proc_read_next_msg(ntfs_cb_t *cb, ntfsv_ntfs_evt_t *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	ntfsv_read_next_req_t *read_next_param = &(evt->info.msg.info.api_info.param.read_next);

	TRACE_4("client_id: %u", read_next_param->client_id);
	readNext(read_next_param->client_id,
		 read_next_param->readerId, read_next_param->searchDirection, &evt->mds_ctxt);
/*  if (ais_rv == SA_AIS_OK)                                                 */
/*  {                                                                        */
/*     async_rc = ntfs_subscription_initialize_async_update(cb,              */
/*                                                  read_next_param);*/
/*     if (async_rc != NCSCC_RC_SUCCESS)                                     */
/*        TRACE("ntfs_send_read_next_async_update failed");          */
/*  }                                                                        */
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : process_api_evt
 *
 * Description   : This is the function which is called when ntfs receives an
 *                 event either because of an API Invocation or other internal
 *                 messages from NTFA clients
 *
 * Arguments     : evt  - Message that was posted to the NTFS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t process_api_evt(ntfsv_ntfs_evt_t *evt)
{
	if (evt->evt_type == NTFSV_NTFS_NTFSV_MSG) {
		/* ignore one level... */
		if ((evt->info.msg.type >= NTFSV_NTFA_API_MSG) && (evt->info.msg.type < NTFSV_MSG_MAX)) {
			if ((evt->info.msg.info.api_info.type >= NTFSV_INITIALIZE_REQ) &&
			    (evt->info.msg.info.api_info.type < NTFSV_API_MAX)) {
				if (ntfs_ntfa_api_msg_dispatcher[evt->info.msg.info.api_info.type] (ntfs_cb, evt) !=
				    NCSCC_RC_SUCCESS) {
					TRACE_2("ntfs_ntfa_api_msg_dispatcher FAILED type: %d",
						(int)evt->info.msg.type);
				}
			}
		}
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ntfs_process_mbx
 *
 * Description   : This is the function which process the IPC mail box of 
 *                 NTFS 
 *
 * Arguments     : mbx  - This is the mail box pointer on which NTFS is 
 *                        going to block.  
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void ntfs_process_mbx(SYSF_MBX *mbx)
{
	ntfsv_ntfs_evt_t *msg;

	msg = (ntfsv_ntfs_evt_t *)m_NCS_IPC_NON_BLK_RECEIVE(mbx, msg);
	if (msg != NULL) {
		if (ntfs_cb->ha_state == SA_AMF_HA_ACTIVE) {
			if ((msg->evt_type >= NTFSV_NTFS_NTFSV_MSG) && (msg->evt_type <= NTFSV_NTFS_EVT_NTFA_DOWN)) {
				ntfs_ntfsv_top_level_evt_dispatch_tbl[msg->evt_type] (msg);
			} else if (msg->evt_type == NTFSV_EVT_QUIESCED_ACK) {
				proc_mds_quiesced_ack_msg(msg);
			} else
				TRACE("message type invalid");
		} else {
			if (msg->evt_type == NTFSV_NTFS_EVT_NTFA_DOWN) {
				ntfs_ntfsv_top_level_evt_dispatch_tbl[msg->evt_type] (msg);
			}
			if (msg->evt_type == NTFSV_EVT_RDA) {
				proc_rda_cb_msg(msg);
			}
		}

		ntfs_evt_destroy(msg);
	}
}
