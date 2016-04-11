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
/**
 *             Interface between Admin and Com
 *
 *   This file contains the interface between the Admin (c++ framework) and the 
 *   comunication layer (c part) mds, mbcsv and amf upcalls.
 *
 */
#include "saAis.h"
#include "saNtf.h"
#include "ntfsv_msg.h"
#include "ntfs_com.h"
#include "ntfsv_enc_dec.h"
#include "ntfsv_mem.h"
#include "ntfs_mbcsv.h"
#if DISCARDED_TEST
	/* TODO REMOVE TEST */
   int disc_test_cntr=1;
#endif

int activeController()
{
	return (ntfs_cb->ha_state == SA_AMF_HA_ACTIVE);
}

void client_added_res_lib(SaAisErrorT error, unsigned int clientId, MDS_DEST mdsDest, MDS_SYNC_SND_CTXT *mdsCtxt,
	SaVersionT *version)
{
	uint32_t rc;
	ntfsv_msg_t msg;
	ntfsv_ckpt_msg_t ckpt;
	TRACE_ENTER2("clientId: %u, rv: %u", clientId, error);
	msg.type = NTFSV_NTFA_API_RESP_MSG;
	msg.info.api_resp_info.type = NTFSV_INITIALIZE_RSP;
	msg.info.api_resp_info.rc = error;
	msg.info.api_resp_info.param.init_rsp.client_id = clientId;
	rc = ntfs_mds_msg_send(ntfs_cb, &msg, &mdsDest, mdsCtxt, MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_LEAVE2("ntfs_mds_msg_send FAILED rc = %u", (unsigned int)rc);
		return;
	}

	if (error == SA_AIS_OK) {
		memset(&ckpt, 0, sizeof(ckpt));
		ckpt.header.ckpt_rec_type = NTFS_CKPT_INITIALIZE_REC;
		ckpt.header.num_ckpt_records = 1;
		ckpt.header.data_len = 1;
		ckpt.ckpt_rec.reg_rec.client_id = clientId;
		ckpt.ckpt_rec.reg_rec.mds_dest = mdsDest;
		ckpt.ckpt_rec.reg_rec.version =*version;
		update_standby(&ckpt, NCS_MBCSV_ACT_ADD);
	}
	TRACE_LEAVE();
}

void client_removed_res_lib(SaAisErrorT error, unsigned int clientId, MDS_DEST mdsDest, MDS_SYNC_SND_CTXT *mdsCtxt)
{
	uint32_t rc;
	ntfsv_msg_t msg;
	ntfsv_ckpt_msg_t ckpt;
	TRACE_ENTER2("clientId: %u, rv: %u", clientId, error);
	msg.type = NTFSV_NTFA_API_RESP_MSG;
	msg.info.api_resp_info.type = NTFSV_FINALIZE_RSP;
	msg.info.api_resp_info.rc = error;
	rc = ntfs_mds_msg_send(ntfs_cb, &msg, &mdsDest, mdsCtxt, MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_LEAVE2("ntfs_mds_msg_send FAILED");
		/* TODO: what to do exit here? */
		return;
	}

	memset(&ckpt, 0, sizeof(ckpt));
	ckpt.header.ckpt_rec_type = NTFS_CKPT_FINALIZE_REC;
	ckpt.header.num_ckpt_records = 1;
	ckpt.header.data_len = 1;
	ckpt.ckpt_rec.finalize_rec.client_id = clientId;
	update_standby(&ckpt, NCS_MBCSV_ACT_RMV);
	TRACE_LEAVE();
}

/**
 *   Response to the lib on a saNtfNotificationSubscribe request.
 *
 *   @param error    error code sent back to the library
 *   @param subId
 *   @param mdsDest       
 *   @param mdsCtxt 
 *
 */
void subscribe_res_lib(SaAisErrorT error, SaNtfSubscriptionIdT subId, MDS_DEST mdsDest, MDS_SYNC_SND_CTXT *mdsCtxt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ntfsv_msg_t msg;
	TRACE_ENTER();

	msg.type = NTFSV_NTFA_API_RESP_MSG;
	msg.info.api_resp_info.type = NTFSV_SUBSCRIBE_RSP;
	msg.info.api_resp_info.rc = error;
	msg.info.api_resp_info.param.subscribe_rsp.subscriptionId = subId;
	TRACE_4("subscriptionId: %u, rv: %u", subId, error);

	rc = ntfs_mds_msg_send(ntfs_cb, &msg, &mdsDest, mdsCtxt, MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("ntfs_mds_msg_send FAILED");
		/* TODO: what to do exit here? */
	}

	TRACE_LEAVE();
};

/**
 *   Response to the lib after saNtfNotificationUnsubscribe request
 *
 *   @param error    error code sent back to the library
 *   @param subId    Subscribtion id for the canceled subscription
 */
void unsubscribe_res_lib(SaAisErrorT error, SaNtfSubscriptionIdT subId, MDS_DEST mdsDest, MDS_SYNC_SND_CTXT *mdsCtxt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ntfsv_msg_t msg;
	TRACE_ENTER();

	msg.type = NTFSV_NTFA_API_RESP_MSG;
	msg.info.api_resp_info.type = NTFSV_UNSUBSCRIBE_RSP;
	msg.info.api_resp_info.rc = error;
	msg.info.api_resp_info.param.unsubscribe_rsp.subscriptionId = subId;
	TRACE_4("subscriptionId: %u, rv: %u", subId, error);

	rc = ntfs_mds_msg_send(ntfs_cb, &msg, &mdsDest, mdsCtxt, MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("ntfs_mds_msg_send FAILED");
		/* TODO: what to do exit here? */
	}

	TRACE_LEAVE();
};

/**
 *   response to the lib on a saNtfNotificationSend request 
 *   @param error    error code sent back to the client
 *   @param notificationId
 *   @param mdsCtxt
 *   @param frDest
 *
 */
void notfication_result_lib(SaAisErrorT error,
			    SaNtfIdentifierT notificationId, MDS_SYNC_SND_CTXT *mdsCtxt, MDS_DEST frDest)
{
	uint32_t rc;
	ntfsv_msg_t msg;
	TRACE_ENTER();

	msg.type = NTFSV_NTFA_API_RESP_MSG;
	msg.info.api_resp_info.type = NTFSV_SEND_NOT_RSP;
	msg.info.api_resp_info.rc = error;
	msg.info.api_resp_info.param.send_not_rsp.notificationId = notificationId;
	TRACE_4("not_id: %llu, rv: %u", notificationId, error);
	rc = ntfs_mds_msg_send(ntfs_cb, &msg, &frDest, mdsCtxt, MDS_SEND_PRIORITY_HIGH);

	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("ntfs_mds_msg_send FAILED");
		/* TODO: what to do exit here? */
	}
	TRACE_LEAVE();
};

/**
 *   Response to the lib on a saNtfNotificationReadInitialize
 *   request
 *  
 *   @param error     error code sent back to the library
 *   @param readerId  out parameter, unique id for the new reader
 *                    set by admin to identify a reader.
 */
void new_reader_res_lib(SaAisErrorT error, unsigned int readerId, MDS_DEST mdsDest, MDS_SYNC_SND_CTXT *mdsCtxt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ntfsv_msg_t msg;
	TRACE_ENTER();

	msg.type = NTFSV_NTFA_API_RESP_MSG;
	msg.info.api_resp_info.type = NTFSV_READER_INITIALIZE_RSP;
	msg.info.api_resp_info.rc = error;
	msg.info.api_resp_info.param.reader_init_rsp.readerId = readerId;
	TRACE_4("readerId: %u, rv: %u", readerId, error);

	rc = ntfs_mds_msg_send(ntfs_cb, &msg, &mdsDest, mdsCtxt, MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("ntfs_mds_msg_send FAILED");
		/* TODO: what to do exit here? */
	}

/* TODO: async update*/
};

/**
 *   response to the lib on a saNtfNotificationReadNext request
 *   @param error     error code sent back to the library
 *   @param notification  The next notification read or NULL if none exist
 */
void read_next_res_lib(SaAisErrorT error,
		       ntfsv_send_not_req_t *notification, MDS_DEST mdsDest, MDS_SYNC_SND_CTXT *mdsCtxt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ntfsv_msg_t msg;
	TRACE_ENTER();

	msg.type = NTFSV_NTFA_API_RESP_MSG;
	msg.info.api_resp_info.type = NTFSV_READ_NEXT_RSP;
	msg.info.api_resp_info.rc = error;
	msg.info.api_resp_info.param.read_next_rsp.readNotification = notification;
	if (msg.info.api_resp_info.rc == SA_AIS_OK) {
		SaNtfNotificationHeaderT *header;
		ntfsv_get_ntf_header(notification, &header);
		TRACE_4("notId: %llu, rv: %u", *header->notificationId, error);
	}

	rc = ntfs_mds_msg_send(ntfs_cb, &msg, &mdsDest, mdsCtxt, MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("ntfs_mds_msg_send FAILED");
		/* TODO: what to do exit here? */
	}
	TRACE_LEAVE();

/* TODO: async update*/
};

/**
 *   response to the lib on a saNtfNotificationReadFinalize request
 *   @param error     error code sent back to the library
 */
void delete_reader_res_lib(SaAisErrorT error, MDS_DEST mdsDest, MDS_SYNC_SND_CTXT *mdsCtxt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ntfsv_msg_t msg;
	TRACE_ENTER();

	msg.type = NTFSV_NTFA_API_RESP_MSG;
	msg.info.api_resp_info.type = NTFSV_READER_FINALIZE_RSP;
	msg.info.api_resp_info.rc = error;

	/* TODO: remove this param */
	msg.info.api_resp_info.param.reader_finalize_rsp.reader_id = 0;
	TRACE_4(" rv: %u", error);
	rc = ntfs_mds_msg_send(ntfs_cb, &msg, &mdsDest, mdsCtxt, MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("ntfs_mds_msg_send FAILED");
		/* TODO: what to do exit here? */
	}

/* TODO: async update*/
};

/**
 *   Send notification to a lib which will be received by the
 *   SaNtfNotificationCallbackT function.
 *
 *   @param dispatchInfo contains all information about the notification.
 *
 *   @return return value == NCSCC_RC_SUCCESS if ok
 */
int send_notification_lib(ntfsv_send_not_req_t *dispatchInfo, uint32_t client_id, MDS_DEST mds_dest)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ntfsv_msg_t msg;
	SaNtfNotificationHeaderT *header;
	int discarded = 0;
	
	TRACE_ENTER();
	ntfsv_get_ntf_header(dispatchInfo, &header);
	TRACE_3("client id: %u, not_id: %llu", client_id, *header->notificationId);	
	
#if DISCARDED_TEST
	/* TODO REMOVE TEST */
	if ((disc_test_cntr % 20)) {
		TRACE_3("FAKE DISKARDED");
		/* Allways confirm if not success notification will be put in discarded list. */
		notificationSentConfirmed(client_id, dispatchInfo->subscriptionId, *header->notificationId, NTFS_NOTIFICATION_DISCARDED);
		return NCSCC_RC_FAILURE;
	}
	/* END TODO REMOVE TEST */
#endif
	memset(&msg, 0, sizeof(ntfsv_msg_t));
	msg.type = NTFSV_NTFS_CBK_MSG;
	msg.info.cbk_info.type = NTFSV_NOTIFICATION_CALLBACK;
	msg.info.cbk_info.ntfs_client_id = client_id;
	msg.info.cbk_info.subscriptionId = dispatchInfo->subscriptionId;
	msg.info.cbk_info.param.notification_cbk = dispatchInfo;
	rc = ntfs_mds_msg_send(ntfs_cb, &msg, &mds_dest, NULL,	/* send regular msg */
			       MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		discarded = NTFS_NOTIFICATION_DISCARDED;
		LOG_ER("ntfs_mds_msg_send to ntfa failed rc: %d", (int)rc);
	} 
	/* Allways confirm if not success notification will be put in discarded list. */
	notificationSentConfirmed(client_id, dispatchInfo->subscriptionId, *header->notificationId, discarded);
	TRACE_LEAVE();
	return (rc);
};

void sendLoggedConfirm(SaNtfIdentifierT notificationId)
{
	TRACE_ENTER();
	notificationLoggedConfirmed(notificationId);
	TRACE_LEAVE();
};

/**
 * Send discarded notifications to a lib which will be received by the
 * saNtfDiscarded callback function.
 *
 *   @param discardedInfo struct that contains a list of notification id's for
 *                        the notifications to be discarded.
 *
 *   @return          return value = 1 if ok -1 if failed
 */
int send_discard_notification_lib(ntfsv_discarded_info_t *discardedInfo, uint32_t c_id, SaNtfSubscriptionIdT s_id, MDS_DEST mds_dest)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ntfsv_msg_t msg;
	TRACE_ENTER();
#if DISCARDED_TEST
	/* TODO REMOVE TEST */
	if ((disc_test_cntr % 20)) {
		TRACE_3("FAKE DISKARDED");
		return NCSCC_RC_FAILURE;
	}
	/* END TODO REMOVE TEST */
#endif
	memset(&msg, 0, sizeof(ntfsv_msg_t));
	msg.type = NTFSV_NTFS_CBK_MSG;
	msg.info.cbk_info.type = NTFSV_DISCARDED_CALLBACK;
	msg.info.cbk_info.ntfs_client_id = c_id;
	msg.info.cbk_info.subscriptionId = s_id;
	msg.info.cbk_info.param.discarded_cbk.notificationType = discardedInfo->notificationType;
	msg.info.cbk_info.param.discarded_cbk.numberDiscarded = discardedInfo->numberDiscarded;
	msg.info.cbk_info.param.discarded_cbk.discardedNotificationIdentifiers = discardedInfo->discardedNotificationIdentifiers;

	rc = ntfs_mds_msg_send(ntfs_cb, &msg, &mds_dest, NULL,	/* send regular msg */
					 MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_1("ntfs_mds_msg_send to ntfa failed rc: %d", (int)rc);
	} else {
		sendNotConfirmUpdate(c_id, s_id, 0, NTFS_NOTIFICATION_DISCARDED_LIST_SENT);
	}
	TRACE_LEAVE();
	return (rc);
}

/* Send sync data to standby */
int sendSyncGlobals(const struct NtfGlobals *ntfGlobals, NCS_UBAID *uba)
{
	TRACE_2("syncGlobals notid: %llu", ntfGlobals->notificationId);
	if (0 == ntfsv_enc_64bit_msg(uba, ntfGlobals->notificationId))
		return 0;
	if (0 == ntfsv_enc_64bit_msg(uba, ntfGlobals->clientIdCounter))
		return 0;
	return 1;
}

int sendNewNotification(unsigned int connId, ntfsv_send_not_req_t *notificationInfo, NCS_UBAID *uba)
{
	return ntfsv_enc_not_msg(uba, notificationInfo);
};

int sendNoOfClients(uint32_t num_rec, NCS_UBAID *uba)
{
	TRACE_2("num_rec: %u", num_rec);
	return enc_ckpt_reserv_header(uba, NTFS_CKPT_INITIALIZE_REC, num_rec, 0);
}

int sendNewClient(unsigned int clientId, MDS_DEST mdsDest, SaVersionT *version, NCS_UBAID *uba)
{
	ntfs_ckpt_reg_msg_t client_rec;

	client_rec.client_id = clientId;
	client_rec.mds_dest = mdsDest;
	client_rec.version = *version;
	if (0 == enc_mbcsv_client_msg(uba, &client_rec))
		return 0;
	return 1;
};

int sendNoOfNotifications(uint32_t num_rec, NCS_UBAID *uba)
{
	TRACE_2("num_rec: %u", num_rec);
	return enc_ckpt_reserv_header(uba, NTFS_CKPT_NOTIFICATION, num_rec, 0);
}

int sendNoOfSubscriptions(uint32_t num_rec, NCS_UBAID *uba)
{
	TRACE_2("num_rec: %u", num_rec);
	return enc_ckpt_reserv_header(uba, NTFS_CKPT_SUBSCRIBE, num_rec, 0);
}

int sendNewSubscription(ntfsv_subscribe_req_t *s, NCS_UBAID *uba)
{
	TRACE_2("numdiscarded: %u", s->d_info.numberDiscarded);
	if (0 == ntfsv_enc_subscribe_msg(uba, s))
		return 0;
	return 1;
};

int syncLoggedConfirm(unsigned int logged, NCS_UBAID *uba)
{
	TRACE_ENTER2("NOT IMPLEMENTED");
	if (0 == ntfsv_enc_32bit_msg(uba, logged))
		return 0;
	TRACE_LEAVE();
	return 1;
}

void sendMapNoOfSubscriptionToNotification(unsigned int noOfSubcriptions, NCS_UBAID *uba)
{
	TRACE_2("MapNoOfSubcriptions: %d", noOfSubcriptions);
	if (0 == ntfsv_enc_32bit_msg(uba, noOfSubcriptions))
		TRACE_2("encode failed");
	return;
}

void sendMapSubscriptionToNotification(unsigned int clientId, unsigned int subscriptionId, NCS_UBAID *uba)
{
	TRACE_2("Subcription: %d", subscriptionId);
	if (0 == ntfsv_enc_32bit_msg(uba, subscriptionId))
		TRACE_2("encode failed");
	if (0 == ntfsv_enc_32bit_msg(uba, clientId))
		TRACE_2("encode failed");
	return;
}

void sendSubscriptionUpdate(ntfsv_subscribe_req_t* s)
{
	ntfsv_ckpt_msg_t ckpt;

	memset(&ckpt, 0, sizeof(ckpt));
	ckpt.header.ckpt_rec_type = NTFS_CKPT_SUBSCRIBE;
	ckpt.header.num_ckpt_records = 1;
	ckpt.header.data_len = 1;
	ckpt.ckpt_rec.subscribe.arg = *s;
	/* discardedInfo not checkpointed here */
	ckpt.ckpt_rec.subscribe.arg.d_info.numberDiscarded = 0;
	ckpt.ckpt_rec.subscribe.arg.d_info.notificationType = 0;
	update_standby(&ckpt, NCS_MBCSV_ACT_ADD);
}

void sendUnsubscribeUpdate(unsigned int connectionId, unsigned int subscriptionId)
{
	ntfsv_ckpt_msg_t ckpt;

	memset(&ckpt, 0, sizeof(ckpt));
	ckpt.header.ckpt_rec_type = NTFS_CKPT_UNSUBSCRIBE;
	ckpt.header.num_ckpt_records = 1;
	ckpt.header.data_len = 1;
	ckpt.ckpt_rec.unsubscribe.arg.client_id = connectionId;
	ckpt.ckpt_rec.unsubscribe.arg.subscriptionId = subscriptionId;
	update_standby(&ckpt, NCS_MBCSV_ACT_ADD);
}

void sendNotificationUpdate(unsigned int clientId, ntfsv_send_not_req_t *notification)
{
	ntfsv_ckpt_msg_t ckpt;

	memset(&ckpt, 0, sizeof(ckpt));
	ckpt.header.ckpt_rec_type = NTFS_CKPT_NOTIFICATION;
	ckpt.header.num_ckpt_records = 1;
	ckpt.header.data_len = 1;
	ckpt.ckpt_rec.notification.arg = notification;
	update_standby(&ckpt, NCS_MBCSV_ACT_ADD);
}

void sendLoggedConfirmUpdate(SaNtfIdentifierT notificationId)
{
	ntfsv_ckpt_msg_t ckpt;
	TRACE_ENTER2("notId: %llu", notificationId);

	memset(&ckpt, 0, sizeof(ckpt));
	ckpt.header.ckpt_rec_type = NTFS_CKPT_NOT_LOG_CONFIRM;
	ckpt.header.num_ckpt_records = 1;
	ckpt.header.data_len = 1;
	ckpt.ckpt_rec.log_confirm.notificationId = notificationId;
	update_standby(&ckpt, NCS_MBCSV_ACT_ADD);
	TRACE_LEAVE();
}

void sendNotConfirmUpdate(unsigned int clientId, SaNtfSubscriptionIdT subscriptionId, SaNtfIdentifierT notificationId, int discarded)
{
	ntfsv_ckpt_msg_t ckpt;
	TRACE_ENTER2("client: %u, subId: %u, notId: %llu", clientId, subscriptionId, notificationId);
	memset(&ckpt, 0, sizeof(ckpt));
	ckpt.header.ckpt_rec_type = NTFS_CKPT_NOT_SEND_CONFIRM;
	ckpt.header.num_ckpt_records = 1;
	ckpt.header.data_len = 1;
	ckpt.ckpt_rec.send_confirm.clientId = clientId;
	ckpt.ckpt_rec.send_confirm.subscriptionId = subscriptionId;
	ckpt.ckpt_rec.send_confirm.notificationId = notificationId;
	ckpt.ckpt_rec.send_confirm.discarded = discarded;
	update_standby(&ckpt, NCS_MBCSV_ACT_ADD);
	TRACE_LEAVE();
}


/**
 * @brief  Send Membership status of node to a lib on that node.
 *  
 * @param SaClmClusterChangesT (CLM status of node) 
 * @param client_id 
 * @param mdsDest of client
 *
 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 */
uint32_t send_clm_node_status_lib(SaClmClusterChangesT cluster_change, unsigned int client_id, MDS_DEST mdsDest)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	ntfsv_msg_t msg;

	TRACE_ENTER();
	TRACE_3("change:%u, client_id: %u", cluster_change, client_id);

	memset(&msg, 0, sizeof(ntfsv_msg_t));
	msg.type = NTFSV_NTFS_CBK_MSG;
	msg.info.cbk_info.type = NTFSV_CLM_NODE_STATUS_CALLBACK;
	msg.info.cbk_info.ntfs_client_id = client_id;
	msg.info.cbk_info.subscriptionId = 0;
	msg.info.cbk_info.param.clm_node_status_cbk.clm_node_status = cluster_change;
	rc = ntfs_mds_msg_send(ntfs_cb, &msg, &mdsDest, NULL, MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("ntfs_mds_msg_send to ntfa failed rc: %d", (int)rc);
	} 

	TRACE_LEAVE();
	return rc;
}

/**
 * @brief Checks if NTFS has already initialized with CLM service. 
 *
 * @return true/false.
 */
bool is_clm_init()
{
	return (((ntfs_cb->clm_hdl != 0) && (ntfs_cb->clm_initialized == true)) ? true : false);
}
