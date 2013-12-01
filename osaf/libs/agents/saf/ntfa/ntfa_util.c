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

#include <stdlib.h>
#include <syslog.h>
#include "ntfa.h"
#include "ntfsv_enc_dec.h"
#include "ntfsv_mem.h"
#include "osaf_utility.h"
#include "osaf_poll.h"

/* Variables used during startup/shutdown only */
static pthread_mutex_t ntfa_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned int ntfa_use_count;

/**
 * 
 * 
 * @return unsigned int
 */
static unsigned int ntfa_create(void)
{
	unsigned int rc = NCSCC_RC_SUCCESS;

	/* create and init sel obj for mds sync */
	m_NCS_SEL_OBJ_CREATE(&ntfa_cb.ntfs_sync_sel);
	pthread_mutex_lock(&ntfa_cb.cb_lock);
	ntfa_cb.ntfs_sync_awaited = 1;
	pthread_mutex_unlock(&ntfa_cb.cb_lock);

	/* register with MDS */
	if ((NCSCC_RC_SUCCESS != (rc = ntfa_mds_init(&ntfa_cb)))) {
		rc = NCSCC_RC_FAILURE;
		goto error;
	}

	/* Block and wait for indication from MDS meaning NTFS is up */
	osaf_poll_one_fd(m_GET_FD_FROM_SEL_OBJ(ntfa_cb.ntfs_sync_sel), 30000);

	pthread_mutex_lock(&ntfa_cb.cb_lock);
	ntfa_cb.ntfs_sync_awaited = 0;
	pthread_mutex_unlock(&ntfa_cb.cb_lock);

	/* No longer needed */
	m_NCS_SEL_OBJ_DESTROY(ntfa_cb.ntfs_sync_sel);

	/* TODO: fix env variable */
	ntfa_cb.ntf_var_data_limit = NTFA_VARIABLE_DATA_LIMIT;
	return rc;

 error:
	/* delete the ntfa init instances */
	ntfa_hdl_list_del(&ntfa_cb.client_list);

	return rc;
}

/**
 * 
 * 
 * @return unsigned int
 */
static void ntfa_destroy(void)
{
	/* delete the hdl db */
	ntfa_hdl_list_del(&ntfa_cb.client_list);

	/* unregister with MDS */
	ntfa_mds_finalize(&ntfa_cb);
}

/****************************************************************************
 * Name          : ntfa_clear_mbx
 *
 * Description   : This is the function which deletes all the messages from 
 *                 the mail box.
 *
 * Arguments     : arg     - argument to be passed.
 *                 msg     - Event start pointer.
 *
 * Return Values : true/false
 *
 * Notes         : None.
 *****************************************************************************/
static bool ntfa_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	ntfsv_msg_t *cbk, *pnext;

	pnext = cbk = (ntfsv_msg_t *)msg;
	while (pnext) {
		pnext = cbk->next;
		ntfa_msg_destroy(cbk);
		cbk = pnext;
	}
	return true;
}

/****************************************************************************
  Name          : ntfa_notification_hdl_rec_list_del
 
  Description   : This routine deletes a list of log stream records.
 
  Arguments     : pointer to the list of notifiction records anchor.
 
  Return Values : None
 
  Notes         : 
******************************************************************************/
static void ntfa_notification_hdl_rec_list_del(ntfa_notification_hdl_rec_t **plstr_hdl)
{
	ntfa_notification_hdl_rec_t *lstr_hdl;
	while ((lstr_hdl = *plstr_hdl) != NULL) {
		*plstr_hdl = lstr_hdl->next;
		ncshm_destroy_hdl(NCS_SERVICE_ID_NTFA, lstr_hdl->notification_hdl);
		free(lstr_hdl);
		lstr_hdl = NULL;
	}
}

static SaAisErrorT ntfa_alloc_callback_notification(SaNtfNotificationsT *notification,
						    ntfsv_send_not_req_t *not_cbk, ntfa_client_hdl_rec_t *hdl_rec)
{
	SaAisErrorT rc = SA_AIS_OK;
	ntfa_notification_hdl_rec_t *notification_hdl_rec;
	notification->notificationType = not_cbk->notificationType;

	switch (not_cbk->notificationType) {
	case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		TRACE_2("type: SA_NTF_TYPE_OBJECT_CREATE_DELETE");
		rc = saNtfObjectCreateDeleteNotificationAllocate(hdl_rec->local_hdl,
								 &notification->notification.
								 objectCreateDeleteNotification,
								 not_cbk->notification.objectCreateDelete.
								 notificationHeader.numCorrelatedNotifications,
								 not_cbk->notification.objectCreateDelete.
								 notificationHeader.lengthAdditionalText,
								 not_cbk->notification.objectCreateDelete.
								 notificationHeader.numAdditionalInfo,
								 not_cbk->notification.objectCreateDelete.numAttributes,
								 SA_NTF_ALLOC_SYSTEM_LIMIT);
		if (SA_AIS_OK == rc) {
			pthread_mutex_lock(&ntfa_cb.cb_lock);
			notification_hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_NTFA,
							      notification->notification.objectCreateDeleteNotification.
							      notificationHandle);
			if (notification_hdl_rec == NULL) {
				pthread_mutex_unlock(&ntfa_cb.cb_lock);
				TRACE("ncshm_take_hdl notificationHandle failed");
				rc = SA_AIS_ERR_BAD_HANDLE;
				break;
			}
			/* to be able to delelte cbk_notification in saNtfNotificationFree */
			notification_hdl_rec->cbk_notification = notification;
			rc = ntfsv_v_data_cp(&notification_hdl_rec->variable_data, &not_cbk->variable_data);
			ncshm_give_hdl(notification->notification.objectCreateDeleteNotification.notificationHandle);
			ntfsv_copy_ntf_obj_cr_del(&notification->notification.objectCreateDeleteNotification,
						  &not_cbk->notification.objectCreateDelete);
			pthread_mutex_unlock(&ntfa_cb.cb_lock);
		}
		break;
	case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		TRACE_2("type: SA_NTF_TYPE_ATTRIBUTE_CHANGE");
		rc = saNtfAttributeChangeNotificationAllocate(hdl_rec->local_hdl,
							      &notification->notification.attributeChangeNotification,
							      not_cbk->notification.attributeChange.notificationHeader.
							      numCorrelatedNotifications,
							      not_cbk->notification.attributeChange.notificationHeader.
							      lengthAdditionalText,
							      not_cbk->notification.attributeChange.notificationHeader.
							      numAdditionalInfo,
							      not_cbk->notification.attributeChange.numAttributes,
							      SA_NTF_ALLOC_SYSTEM_LIMIT);
		if (SA_AIS_OK == rc) {
			pthread_mutex_lock(&ntfa_cb.cb_lock);
			notification_hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_NTFA,
							      notification->notification.attributeChangeNotification.
							      notificationHandle);
			if (notification_hdl_rec == NULL) {
				pthread_mutex_unlock(&ntfa_cb.cb_lock);
				TRACE("ncshm_take_hdl notificationHandle failed");
				rc = SA_AIS_ERR_BAD_HANDLE;
				break;
			}
			/* to be able to delelte cbk_notification in saNtfNotificationFree */
			notification_hdl_rec->cbk_notification = notification;
			rc = ntfsv_v_data_cp(&notification_hdl_rec->variable_data, &not_cbk->variable_data);
			ncshm_give_hdl(notification->notification.attributeChangeNotification.notificationHandle);
			ntfsv_copy_ntf_attr_change(&notification->notification.attributeChangeNotification,
						   &not_cbk->notification.attributeChange);
			pthread_mutex_unlock(&ntfa_cb.cb_lock);
		}
		break;
	case SA_NTF_TYPE_STATE_CHANGE:
		TRACE_2("type: SA_NTF_TYPE_STATE_CHANGE");
		rc = saNtfStateChangeNotificationAllocate(hdl_rec->local_hdl,
							  &notification->notification.stateChangeNotification,
							  not_cbk->notification.stateChange.notificationHeader.
							  numCorrelatedNotifications,
							  not_cbk->notification.stateChange.notificationHeader.
							  lengthAdditionalText,
							  not_cbk->notification.stateChange.notificationHeader.
							  numAdditionalInfo,
							  not_cbk->notification.stateChange.numStateChanges,
							  SA_NTF_ALLOC_SYSTEM_LIMIT);
		if (SA_AIS_OK == rc) {
			pthread_mutex_lock(&ntfa_cb.cb_lock);
			notification_hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_NTFA,
							      notification->notification.stateChangeNotification.
							      notificationHandle);
			if (notification_hdl_rec == NULL) {
				pthread_mutex_unlock(&ntfa_cb.cb_lock);
				TRACE("ncshm_take_hdl notificationHandle failed");
				rc = SA_AIS_ERR_BAD_HANDLE;
				break;
			}
			/* to be able to delelte cbk_notification in saNtfNotificationFree */
			notification_hdl_rec->cbk_notification = notification;
			rc = ntfsv_v_data_cp(&notification_hdl_rec->variable_data, &not_cbk->variable_data);
			ncshm_give_hdl(notification->notification.stateChangeNotification.notificationHandle);
			ntfsv_copy_ntf_state_change(&notification->notification.stateChangeNotification,
						    &not_cbk->notification.stateChange);
			pthread_mutex_unlock(&ntfa_cb.cb_lock);
		}
		break;
	case SA_NTF_TYPE_ALARM:
		rc = saNtfAlarmNotificationAllocate(hdl_rec->local_hdl,
						    &notification->notification.alarmNotification,
						    not_cbk->notification.alarm.
						    notificationHeader.numCorrelatedNotifications,
						    not_cbk->notification.alarm.notificationHeader.lengthAdditionalText,
						    not_cbk->notification.alarm.notificationHeader.numAdditionalInfo,
						    not_cbk->notification.alarm.numSpecificProblems,
						    not_cbk->notification.alarm.numMonitoredAttributes,
						    not_cbk->notification.alarm.numProposedRepairActions,
						    SA_NTF_ALLOC_SYSTEM_LIMIT);
		if (SA_AIS_OK == rc) {
			pthread_mutex_lock(&ntfa_cb.cb_lock);
			notification_hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_NTFA,
							      notification->notification.
							      alarmNotification.notificationHandle);
			if (notification_hdl_rec == NULL) {
				pthread_mutex_unlock(&ntfa_cb.cb_lock);
				TRACE("ncshm_take_hdl notificationHandle failed");
				rc = SA_AIS_ERR_BAD_HANDLE;
				break;
			}
			/* to be able to delelte cbk_notification in saNtfNotificationFree */
			notification_hdl_rec->cbk_notification = notification;
			rc = ntfsv_v_data_cp(&notification_hdl_rec->variable_data, &not_cbk->variable_data);
			ncshm_give_hdl(notification->notification.alarmNotification.notificationHandle);
			ntfsv_copy_ntf_alarm(&notification->notification.alarmNotification,
					     &not_cbk->notification.alarm);
			pthread_mutex_unlock(&ntfa_cb.cb_lock);
		}
		break;
	case SA_NTF_TYPE_SECURITY_ALARM:
		TRACE_2("type: SA_NTF_TYPE_SECURITY_ALARM");
		rc = saNtfSecurityAlarmNotificationAllocate(hdl_rec->local_hdl,
							    &notification->notification.securityAlarmNotification,
							    not_cbk->notification.securityAlarm.notificationHeader.
							    numCorrelatedNotifications,
							    not_cbk->notification.securityAlarm.notificationHeader.
							    lengthAdditionalText,
							    not_cbk->notification.securityAlarm.notificationHeader.
							    numAdditionalInfo, SA_NTF_ALLOC_SYSTEM_LIMIT);
		if (SA_AIS_OK == rc) {
			pthread_mutex_lock(&ntfa_cb.cb_lock);
			notification_hdl_rec = ncshm_take_hdl(NCS_SERVICE_ID_NTFA,
							      notification->notification.securityAlarmNotification.
							      notificationHandle);
			if (notification_hdl_rec == NULL) {
				pthread_mutex_unlock(&ntfa_cb.cb_lock);
				TRACE("ncshm_take_hdl notificationHandle failed");
				rc = SA_AIS_ERR_BAD_HANDLE;
				break;
			}
			/* to be able to delelte cbk_notification in saNtfNotificationFree */
			notification_hdl_rec->cbk_notification = notification;
			rc = ntfsv_v_data_cp(&notification_hdl_rec->variable_data, &not_cbk->variable_data);
			ncshm_give_hdl(notification->notification.securityAlarmNotification.notificationHandle);
			ntfsv_copy_ntf_security_alarm(&notification->notification.securityAlarmNotification,
						      &not_cbk->notification.securityAlarm);
			pthread_mutex_unlock(&ntfa_cb.cb_lock);
		}
		break;
	default:
		LOG_ER("Unkown notification type");
		rc = SA_AIS_ERR_INVALID_PARAM;
	}

	return rc;
}

/****************************************************************************
  Name          : ntfa_hdl_cbk_rec_prc
 
  Description   : This routine invokes the registered callback routine & frees
                  the callback record resources.
 
  Arguments     : cb      - ptr to the NTFA control block
                  msg     - ptr to the callback message
                  reg_cbk - ptr to the registered callbacks
 
  Return Values : Error code
 
  Notes         : None
******************************************************************************/
static SaAisErrorT ntfa_hdl_cbk_rec_prc(ntfa_cb_t *cb, ntfsv_msg_t *msg, ntfa_client_hdl_rec_t *hdl_rec)
{
	SaNtfCallbacksT *reg_cbk = &hdl_rec->reg_cbk;
	ntfsv_cbk_info_t *cbk_info = &msg->info.cbk_info;
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;
	TRACE_ENTER2("callback type: %d", cbk_info->type);

	/* invoke the corresponding callback */
	switch (cbk_info->type) {
	case NTFSV_NOTIFICATION_CALLBACK:
		{
			if (reg_cbk->saNtfNotificationCallback) {
				SaNtfNotificationsT *notification;
				/* TODO: new notification from decode */
				notification = calloc(1, sizeof(SaNtfNotificationsT));
				if (notification == NULL) {
					rc = SA_AIS_ERR_NO_MEMORY;
					goto done;
				}
/* TODO: put in handle database when decode */
				rc = ntfa_alloc_callback_notification(notification,
								      cbk_info->param.notification_cbk, hdl_rec);
				if (rc != SA_AIS_OK) {
					/* not in handle struct */
					free(notification);
					goto done;
				}
				reg_cbk->saNtfNotificationCallback(cbk_info->subscriptionId, notification);
			}
		}
		break;
	case NTFSV_DISCARDED_CALLBACK:
		{
			if (reg_cbk->saNtfNotificationDiscardedCallback) {
				rc = SA_AIS_OK;
				reg_cbk->saNtfNotificationDiscardedCallback(cbk_info->subscriptionId,
									    cbk_info->param.discarded_cbk.
									    notificationType,
									    cbk_info->param.discarded_cbk.
									    numberDiscarded,
									    cbk_info->param.discarded_cbk.
									    discardedNotificationIdentifiers);
			}
			free(cbk_info->param.discarded_cbk.discardedNotificationIdentifiers);
		}
		break;
	default:
		TRACE("unsupported callback type: %d", cbk_info->type);
		rc = SA_AIS_ERR_LIBRARY;
		break;
	}
 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ntfa_hdl_cbk_dispatch_one
 
  Description   : This routine dispatches one pending callback.
 
  Arguments     : cb      - ptr to the NTFA control block
                  hdl_rec - ptr to the handle record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static SaAisErrorT ntfa_hdl_cbk_dispatch_one(ntfa_cb_t *cb, ntfa_client_hdl_rec_t *hdl_rec)
{
	ntfsv_msg_t *cbk_msg;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();
	/* Nonblk receive to obtain the message from priority queue */
	while (NULL != (cbk_msg = (ntfsv_msg_t *)
			m_NCS_IPC_NON_BLK_RECEIVE(&hdl_rec->mbx, cbk_msg))) {
		rc = ntfa_hdl_cbk_rec_prc(cb, cbk_msg, hdl_rec);
		ntfa_msg_destroy(cbk_msg);
		break;
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ntfa_hdl_cbk_dispatch_all
 
  Description   : This routine dispatches all the pending callbacks.
 
  Arguments     : cb      - ptr to the NTFA control block
                  hdl_rec - ptr to the handle record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t ntfa_hdl_cbk_dispatch_all(ntfa_cb_t *cb, ntfa_client_hdl_rec_t *hdl_rec)
{
	ntfsv_msg_t *cbk_msg;
	uint32_t rc = SA_AIS_OK;

	/* Recv all the cbk notifications from the queue & process them */
	do {
		if (NULL == (cbk_msg = (ntfsv_msg_t *)m_NCS_IPC_NON_BLK_RECEIVE(&hdl_rec->mbx, cbk_msg)))
			break;
		rc = ntfa_hdl_cbk_rec_prc(cb, cbk_msg, hdl_rec);
		/* now that we are done with this rec, free the resources */
		ntfa_msg_destroy(cbk_msg);
	} while (1);

	return rc;
}

/****************************************************************************
  Name          : ntfa_hdl_cbk_dispatch_block
 
  Description   : This routine blocks forever for receiving indications from 
                  NTFS. The routine returns when saEvtFinalize is executed on 
                  the handle.
 
  Arguments     : cb      - ptr to the NTFA control block
                  hdl_rec - ptr to the handle record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t ntfa_hdl_cbk_dispatch_block(ntfa_cb_t *cb, ntfa_client_hdl_rec_t *hdl_rec)
{
	ntfsv_msg_t *cbk_msg;
	uint32_t rc = SA_AIS_OK;

	for (;;) {
		if (NULL != (cbk_msg = (ntfsv_msg_t *)
			     m_NCS_IPC_RECEIVE(&hdl_rec->mbx, cbk_msg))) {
			rc = ntfa_hdl_cbk_rec_prc(cb, cbk_msg, hdl_rec);
			/* now that we are done with this rec, free the resources */
			ntfa_msg_destroy(cbk_msg);
		} else
			return rc;	/* FIX to handle finalize clean up of mbx */
	}
	return rc;
}

/**
 * 
 * 
 * @return unsigned int
 */
unsigned int ntfa_startup(void)
{
	unsigned int rc = NCSCC_RC_SUCCESS;
	pthread_mutex_lock(&ntfa_lock);

	TRACE_ENTER2("ntfa_use_count: %u", ntfa_use_count);
	if (ntfa_use_count > 0) {
		/* Already created, just increment the use_count */
		ntfa_use_count++;
		goto done;
	} else {
		if ((rc = ncs_agents_startup()) != NCSCC_RC_SUCCESS) {
			TRACE("ncs_agents_startup FAILED");
			goto done;
		}

		if ((rc = ntfa_create()) != NCSCC_RC_SUCCESS) {
			ncs_agents_shutdown();
			goto done;
		} else
			ntfa_use_count = 1;
	}

 done:
	pthread_mutex_unlock(&ntfa_lock);
	TRACE_LEAVE2("rc: %u, ntfa_use_count: %u", rc, ntfa_use_count);
	return rc;
}

/**
 * 
 * 
 * @return unsigned int
 */
unsigned int ntfa_shutdown(void)
{
	unsigned int rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("ntfa_use_count: %u", ntfa_use_count);
	pthread_mutex_lock(&ntfa_lock);

	if (ntfa_use_count > 1) {
		/* Users still exist, just decrement the use count */
		ntfa_use_count--;
	} else if (ntfa_use_count == 1) {
		ntfa_destroy();
		rc = ncs_agents_shutdown();
		ntfa_use_count = 0;
	}

	pthread_mutex_unlock(&ntfa_lock);
	TRACE_LEAVE2("rc: %u, ntfa_use_count: %u", rc, ntfa_use_count);
	return rc;
}

/****************************************************************************
 * Name          : ntfa_msg_destroy
 *
 * Description   : This is the function which is called to destroy an NTFSV msg.
 *
 * Arguments     : NTFSV_MSG *.
 *
 * Return Values : NONE
 *
 * Notes         : None.
 *****************************************************************************/
void ntfa_msg_destroy(ntfsv_msg_t *msg)
{
	if (NTFSV_NTFA_API_MSG == msg->type) {
		TRACE("NTFSV_NTFA_API_MSG");
	} else if (NTFSV_NTFS_CBK_MSG == msg->type) {
		TRACE("NTFSV_NTFS_CBK_MSG dealloc_notification");
		if (msg->info.cbk_info.type == NTFSV_NOTIFICATION_CALLBACK) {
			/*       TODO: fix in decode  */
			ntfsv_dealloc_notification(msg->info.cbk_info.param.notification_cbk);
			free(msg->info.cbk_info.param.notification_cbk);
		}
	} else {
		if(msg->info.api_resp_info.rc == SA_AIS_OK) {
			if(msg->info.api_resp_info.type == NTFSV_READ_NEXT_RSP && 
					(msg->info.api_resp_info.param.read_next_rsp.readNotification != NULL)) {
				if(msg->info.api_resp_info.param.read_next_rsp.
						readNotification->notificationType == SA_NTF_TYPE_ALARM)
					ntfsv_dealloc_notification(msg->info.api_resp_info.param.
							read_next_rsp.readNotification);
				else if(msg->info.api_resp_info.param.read_next_rsp.
						readNotification->notificationType == SA_NTF_TYPE_SECURITY_ALARM)
					ntfsv_dealloc_notification(msg->info.api_resp_info.param.
							read_next_rsp.readNotification);
				else
					TRACE("Reading of only Alarm and Security Alarm is supported");

				if(msg->info.api_resp_info.param.read_next_rsp.readNotification != NULL)	
					free(msg->info.api_resp_info.param.read_next_rsp.readNotification);
			}
		}
	}
	free(msg);
}

/****************************************************************************
  Name          : ntfa_find_hdl_rec_by_client_id
 
  Description   : This routine looks up a ntfa_client_hdl_rec by client_id
 
  Arguments     : cb      - ptr to the NTFA control block
                  client_id  - cluster wide unique allocated by NTFS

  Return Values : NTFA_CLIENT_HDL_REC * or NULL
 
  Notes         : None
******************************************************************************/
ntfa_client_hdl_rec_t *ntfa_find_hdl_rec_by_client_id(ntfa_cb_t *ntfa_cb, uint32_t client_id)
{
	ntfa_client_hdl_rec_t *ntfa_hdl_rec;

	for (ntfa_hdl_rec = ntfa_cb->client_list; ntfa_hdl_rec != NULL; ntfa_hdl_rec = ntfa_hdl_rec->next) {
		if (ntfa_hdl_rec->ntfs_client_id == client_id)
			return ntfa_hdl_rec;
	}

	return NULL;
}

/****************************************************************************
  Name          : ntfa_hdl_list_del
 
  Description   : This routine deletes all handles for this library.
 
  Arguments     : cb  - ptr to the NTFA control block
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
void ntfa_hdl_list_del(ntfa_client_hdl_rec_t **p_client_hdl)
{
	ntfa_client_hdl_rec_t *client_hdl;

	while ((client_hdl = *p_client_hdl) != NULL) {
		*p_client_hdl = client_hdl->next;
		ncshm_destroy_hdl(NCS_SERVICE_ID_NTFA, client_hdl->local_hdl);
	/** clean up the channel records for this ntfa-client
         **/
		ntfa_notification_hdl_rec_list_del(&client_hdl->notification_list);
	/** remove the association with hdl-mngr 
         **/
		free(client_hdl);
		client_hdl = 0;
	}
}

/****************************************************************************
  Name          : ntfa_notification_hdl_rec_del
 
  Description   : This routine deletes the a log stream handle record from
                  a list of log stream hdl records. 
 
  Arguments     : NTFA_notification_hdl_REC **list_head
                  NTFA_notification_hdl_REC *rm_node

 
  Return Values : None
 
  Notes         : 
******************************************************************************/
uint32_t ntfa_notification_hdl_rec_del(ntfa_notification_hdl_rec_t **list_head, ntfa_notification_hdl_rec_t *rm_node)
{
	/* Find the channel hdl record in the list of records */
	ntfa_notification_hdl_rec_t *list_iter = *list_head;

	/* If the to be removed record is the first record */
	if (list_iter == rm_node) {
		*list_head = rm_node->next;
	/** remove the association with hdl-mngr 
         **/
		ncshm_give_hdl(rm_node->notification_hdl);
		ncshm_destroy_hdl(NCS_SERVICE_ID_NTFA, rm_node->notification_hdl);
		free(rm_node);
		return NCSCC_RC_SUCCESS;
	} else {		/* find the rec */

		while (NULL != list_iter) {
			if (list_iter->next == rm_node) {
				list_iter->next = rm_node->next;
		/** remove the association with hdl-mngr 
                 **/
				ncshm_give_hdl(rm_node->notification_hdl);
				ncshm_destroy_hdl(NCS_SERVICE_ID_NTFA, rm_node->notification_hdl);
				free(rm_node);
				return NCSCC_RC_SUCCESS;
			}
			/* move onto the next one */
			list_iter = list_iter->next;
		}
	}
    /** The node couldn't be deleted **/
	TRACE("The node couldn't be deleted");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : ntfa_filter_hdl_rec_del
 
  Description   : This routine deletes the a log stream handle record from
                  a list of log stream hdl records. 
 
  Arguments     : NTFA_filter_hdl_REC **list_head
                  NTFA_filter_hdl_REC *rm_node

 
  Return Values : None
 
  Notes         : 
******************************************************************************/
uint32_t ntfa_filter_hdl_rec_del(ntfa_filter_hdl_rec_t **list_head, ntfa_filter_hdl_rec_t *rm_node)
{
	/* Find the filter hdl record in the list of records */
	ntfa_filter_hdl_rec_t *list_iter = *list_head;

	/* If the to be removed record is the first record */
	if (list_iter == rm_node) {
		*list_head = rm_node->next;
		/* remove the association with hdl-mngr */
		ncshm_give_hdl(rm_node->filter_hdl);
		ncshm_destroy_hdl(NCS_SERVICE_ID_NTFA, rm_node->filter_hdl);
		free(rm_node);
		return NCSCC_RC_SUCCESS;
	} else {		/* find the rec */

		while (NULL != list_iter) {
			if (list_iter->next == rm_node) {
				list_iter->next = rm_node->next;
				/* remove the association with hdl-mngr */
				ncshm_give_hdl(rm_node->filter_hdl);
				ncshm_destroy_hdl(NCS_SERVICE_ID_NTFA, rm_node->filter_hdl);
				free(rm_node);
				return NCSCC_RC_SUCCESS;
			}
			/* move onto the next one */
			list_iter = list_iter->next;
		}
	}
    /** The node couldn't be deleted **/
	TRACE("The node couldn't be deleted");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : ntfa_hdl_rec_del
 
  Description   : This routine deletes the a client handle record from
                  a list of client hdl records. 
 
  Arguments     : NTFA_CLIENT_HDL_REC **list_head
                  NTFA_CLIENT_HDL_REC *rm_node
 
  Return Values : None
 
  Notes         : The selection object is destroyed after all the means to 
                  access the handle record (ie. hdl db tree or hdl mngr) is 
                  removed. This is to disallow the waiting thread to access 
                  the hdl rec while other thread executes saAmfFinalize on it.
******************************************************************************/
uint32_t ntfa_hdl_rec_del(ntfa_client_hdl_rec_t **list_head, ntfa_client_hdl_rec_t *rm_node)
{
	uint32_t rc = NCSCC_RC_FAILURE;
	ntfa_client_hdl_rec_t *list_iter = *list_head;

	TRACE_ENTER();
/* TODO: free all resources allocated by the client */

	/* If the to be removed record is the first record */
	if (list_iter == rm_node) {
		*list_head = rm_node->next;

	/** detach & release the IPC 
         **/
		m_NCS_IPC_DETACH(&rm_node->mbx, ntfa_clear_mbx, NULL);
		m_NCS_IPC_RELEASE(&rm_node->mbx, NULL);

		ncshm_give_hdl(rm_node->local_hdl);
		ncshm_destroy_hdl(NCS_SERVICE_ID_NTFA, rm_node->local_hdl);
	/** Free the channel records off this hdl 
         **/
		ntfa_notification_hdl_rec_list_del(&rm_node->notification_list);

	/** free the hdl rec 
         **/
		free(rm_node);
		rc = NCSCC_RC_SUCCESS;
		goto out;
	} else {		/* find the rec */

		while (NULL != list_iter) {
			if (list_iter->next == rm_node) {
				list_iter->next = rm_node->next;

		/** detach & release the IPC */
				m_NCS_IPC_DETACH(&rm_node->mbx, ntfa_clear_mbx, NULL);
				m_NCS_IPC_RELEASE(&rm_node->mbx, NULL);

				ncshm_give_hdl(rm_node->local_hdl);
				ncshm_destroy_hdl(NCS_SERVICE_ID_NTFA, rm_node->local_hdl);
		/** Free the channel records off this ntfa_hdl  */
				ntfa_notification_hdl_rec_list_del(&rm_node->notification_list);

		/** free the hdl rec */
				free(rm_node);

				rc = NCSCC_RC_SUCCESS;
				goto out;
			}
			/* move onto the next one */
			list_iter = list_iter->next;
		}
	}
	TRACE("failed");

 out:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : ntfa_notification_hdl_rec_add
 
  Description   : This routine adds the notification handle record to the list
                  of handles in the client hdl record.
 
  Arguments     : NTFA_CLIENT_HDL_REC *hdl_rec
 
  Return Values : ptr to the channel handle record
 
  Notes         : None
******************************************************************************/
ntfa_notification_hdl_rec_t *ntfa_notification_hdl_rec_add(ntfa_client_hdl_rec_t **hdl_rec,
							   SaInt16T variableDataSize, SaAisErrorT *rc)
{
	ntfa_notification_hdl_rec_t *rec = calloc(1, sizeof(ntfa_notification_hdl_rec_t));
	*rc = SA_AIS_OK;
	if (rec == NULL) {
		*rc = SA_AIS_ERR_NO_MEMORY;
		TRACE("calloc failed");
		return NULL;
	}

	/* create the association with hdl-mngr */
	if (0 == (rec->notification_hdl = ncshm_create_hdl(NCS_HM_POOL_ID_COMMON,
							   NCS_SERVICE_ID_NTFA, (NCSCONTEXT)rec))) {
		TRACE("ncshm_create_hdl failed");
		free(rec);
		*rc = SA_AIS_ERR_NO_MEMORY;
		return NULL;
	}
	*rc = ntfsv_variable_data_init(&rec->variable_data, variableDataSize, ntfa_cb.ntf_var_data_limit);
	if (*rc != SA_AIS_OK) {
		free(rec);
		return NULL;
	}

    /** Initialize the parent handle **/
	rec->parent_hdl = *hdl_rec;

    /** Insert this record into the list of notification records */
	rec->next = (*hdl_rec)->notification_list;
	(*hdl_rec)->notification_list = rec;

	/* allocated for callback struct */
	rec->cbk_notification = NULL;

    /** Everything appears fine, so return the notification hdl. */
	return rec;
}

/****************************************************************************
  Name          : ntfa_filter_hdl_rec_add
 
  Description   : This routine adds the filter handle record to the list
                  of handles in the client hdl record.
 
  Arguments     : NTFA_CLIENT_HDL_REC *hdl_rec
 
  Return Values : ptr to the channel handle record
 
  Notes         : None
******************************************************************************/
ntfa_filter_hdl_rec_t *ntfa_filter_hdl_rec_add(ntfa_client_hdl_rec_t **hdl_rec)
{
	ntfa_filter_hdl_rec_t *rec = calloc(1, sizeof(ntfa_filter_hdl_rec_t));

	if (rec == NULL) {
		TRACE("calloc failed");
		return NULL;
	}

	/* create the association with hdl-mngr */
	if (0 == (rec->filter_hdl = ncshm_create_hdl(NCS_HM_POOL_ID_COMMON, NCS_SERVICE_ID_NTFA, (NCSCONTEXT)rec))) {
		TRACE("ncshm_create_hdl failed");
		free(rec);
		return NULL;
	}

    /** Initialize the parent handle **/
	rec->parent_hdl = *hdl_rec;

    /** Insert this record into the list of channel hdl records
     **/
	rec->next = (*hdl_rec)->filter_list;
	(*hdl_rec)->filter_list = rec;

    /** Everything appears fine, so return the 
     ** steam hdl.
     **/
	return rec;
}

/****************************************************************************
  Name          : ntfa_hdl_rec_add
 
  Description   : This routine adds the handle record to the ntfa cb.
 
  Arguments     : cb       - ptr tot he NTFA control block
                  reg_cbks - ptr to the set of registered callbacks
                  client_id   - obtained from NTFS.
 
  Return Values : ptr to the ntfa handle record
 
  Notes         : None
******************************************************************************/
ntfa_client_hdl_rec_t *ntfa_hdl_rec_add(ntfa_cb_t *cb, const SaNtfCallbacksT *reg_cbks, uint32_t client_id)
{
	ntfa_client_hdl_rec_t *rec = calloc(1, sizeof(ntfa_client_hdl_rec_t));

	if (rec == NULL) {
		TRACE("calloc failed");
		goto done;
	}

	/* create the association with hdl-mngr */
	if (0 == (rec->local_hdl = ncshm_create_hdl(NCS_HM_POOL_ID_COMMON, NCS_SERVICE_ID_NTFA, (NCSCONTEXT)rec))) {
		TRACE("ncshm_create_hdl failed");
		goto err_free;
	}

	/* store the registered callbacks */
	if (reg_cbks)
		memcpy((void *)&rec->reg_cbk, (void *)reg_cbks, sizeof(SaNtfCallbacksT));

    /** Associate with the client_id obtained from NTFS
     **/
	rec->ntfs_client_id = client_id;

    /** Initialize and attach the IPC/Priority queue
     **/

	if (m_NCS_IPC_CREATE(&rec->mbx) != NCSCC_RC_SUCCESS) {
		TRACE("m_NCS_IPC_CREATE failed");
		goto err_destroy_hdl;
	}

	if (m_NCS_IPC_ATTACH(&rec->mbx) != NCSCC_RC_SUCCESS) {
		TRACE("m_NCS_IPC_ATTACH failed");
		goto err_ipc_release;
	}

    /** Add this to the Link List of 
     ** CLIENT_HDL_RECORDS for this NTFA_CB 
     **/

	pthread_mutex_lock(&cb->cb_lock);
	/* add this to the start of the list */
	rec->next = cb->client_list;
	cb->client_list = rec;
	pthread_mutex_unlock(&cb->cb_lock);

	goto done;

 err_ipc_release:
	(void)m_NCS_IPC_RELEASE(&rec->mbx, NULL);

 err_destroy_hdl:
	ncshm_destroy_hdl(NCS_SERVICE_ID_NTFA, rec->local_hdl);

 err_free:
	free(rec);
	rec = NULL;

 done:
	return rec;
}

/****************************************************************************
  Name          : ntfa_hdl_cbk_dispatch
 
  Description   : This routine dispatches the pending callbacks as per the 
                  dispatch flags.
 
  Arguments     : cb      - ptr to the NTFA control block
                  hdl_rec - ptr to the handle record
                  flags   - dispatch flags
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
SaAisErrorT ntfa_hdl_cbk_dispatch(ntfa_cb_t *cb, ntfa_client_hdl_rec_t *hdl_rec, SaDispatchFlagsT flags)
{
	SaAisErrorT rc;

	switch (flags) {
	case SA_DISPATCH_ONE:
		rc = ntfa_hdl_cbk_dispatch_one(cb, hdl_rec);
		break;

	case SA_DISPATCH_ALL:
		rc = ntfa_hdl_cbk_dispatch_all(cb, hdl_rec);
		break;

	case SA_DISPATCH_BLOCKING:
		rc = ntfa_hdl_cbk_dispatch_block(cb, hdl_rec);
		break;

	default:
		TRACE("dispatch flag not valid");
		rc = SA_AIS_ERR_INVALID_PARAM;
		break;
	}			/* switch */

	return rc;
}

/*
 * To enable tracing early in saLogInitialize, use a GCC constructor
 */
__attribute__ ((constructor))
static void logtrace_init_constructor(void)
{
	char *value;

	/* Initialize trace system first of all so we can see what is going. */
	if ((value = getenv("NTFSV_TRACE_PATHNAME")) != NULL) {
		if (logtrace_init("ntfa", value, CATEGORY_ALL) != 0) {
			/* error, we cannot do anything */
			return;
		}
	}
}

/**
 * 
 * @param instance
 */
void ntfa_hdl_rec_destructor(ntfa_notification_hdl_rec_t *instance)
{
	ntfa_notification_hdl_rec_t *notificationInstance = instance;

	switch (notificationInstance->ntfNotificationType) {
	case SA_NTF_TYPE_ALARM:
		ntfsv_free_alarm(&notificationInstance->ntfNotification.ntfAlarmNotification);
		break;

	case SA_NTF_TYPE_STATE_CHANGE:
		ntfsv_free_state_change(&notificationInstance->ntfNotification.ntfStateChangeNotification);
		break;

	case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		ntfsv_free_obj_create_del(&notificationInstance->ntfNotification.ntfObjectCreateDeleteNotification);
		break;

	case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		ntfsv_free_attribute_change(&notificationInstance->ntfNotification.ntfAttributeChangeNotification);

		break;

	case SA_NTF_TYPE_SECURITY_ALARM:
		ntfsv_free_security_alarm(&notificationInstance->ntfNotification.ntfSecurityAlarmNotification);
		break;

	default:
		TRACE("Invalid Notification Type!");
		break;
	}
	if (NULL != notificationInstance->cbk_notification) {
		free(notificationInstance->cbk_notification);
	}
	TRACE_1("free v_data.p_base %p", notificationInstance->variable_data.p_base);
	free(notificationInstance->variable_data.p_base);
	notificationInstance->variable_data.p_base = NULL;
	notificationInstance->variable_data.size = 0;
}

/**
 * 
 * @param instance
 */
void ntfa_filter_hdl_rec_destructor(ntfa_filter_hdl_rec_t *filter_rec)
{
	switch (filter_rec->ntfType) {
	case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		ntfsv_filter_obj_cr_del_free(&filter_rec->notificationFilter.objectCreateDeleteNotificationfilter);
		break;
	case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		ntfsv_filter_attr_ch_free(&filter_rec->notificationFilter.attributeChangeNotificationfilter);
		break;
	case SA_NTF_TYPE_STATE_CHANGE:
		ntfsv_filter_state_ch_free(&filter_rec->notificationFilter.stateChangeNotificationfilter);
		break;
	case SA_NTF_TYPE_SECURITY_ALARM:
		ntfsv_filter_sec_alarm_free(&filter_rec->notificationFilter.securityAlarmNotificationfilter);
		break;
	case SA_NTF_TYPE_ALARM:
		ntfsv_filter_alarm_free(&filter_rec->notificationFilter.alarmNotificationfilter);
		break;
	default:
		osafassert(0);
	}
}

/****************************************************************************
  Name          : ntfa_reader_hdl_rec_add
 
  Description   : This routine adds the reader handle record to the list
                  of handles in the client hdl record.
 
  Arguments     : NTFA_CLIENT_HDL_REC *hdl_rec
 
  Return Values : ptr to the channel handle record
 
  Notes         : None
******************************************************************************/
ntfa_reader_hdl_rec_t *ntfa_reader_hdl_rec_add(ntfa_client_hdl_rec_t **hdl_rec)
{
	ntfa_reader_hdl_rec_t *rec = calloc(1, sizeof(ntfa_reader_hdl_rec_t));

	if (rec == NULL) {
		TRACE("calloc failed");
		return NULL;
	}

	/* create the association with hdl-mngr */
	if (0 == (rec->reader_hdl = ncshm_create_hdl(NCS_HM_POOL_ID_COMMON, NCS_SERVICE_ID_NTFA, (NCSCONTEXT)rec))) {
		TRACE("ncshm_create_hdl failed");
		free(rec);
		return NULL;
	}

    /** Initialize the parent handle **/
	rec->parent_hdl = *hdl_rec;

    /** Insert this record into the list of channel hdl records
     **/
	rec->next = (*hdl_rec)->reader_list;
	(*hdl_rec)->reader_list = rec;

    /** Everything appears fine, so return the 
     ** steam hdl.
     **/
	return rec;
}

/****************************************************************************
  Name          : ntfa_reader_hdl_rec_del
 
  Description   : This routine deletes the a log stream handle record from
                  a list of log stream hdl records. 
 
  Arguments     : NTFA_reader_hdl_REC **list_head
                  NTFA_reader_hdl_REC *rm_node

 
  Return Values : None
 
  Notes         : 
******************************************************************************/
uint32_t ntfa_reader_hdl_rec_del(ntfa_reader_hdl_rec_t **list_head, ntfa_reader_hdl_rec_t *rm_node)
{
	/* Find the channel hdl record in the list of records */
	ntfa_reader_hdl_rec_t *list_iter = *list_head;

	/* If the to be removed record is the first record */
	if (list_iter == rm_node) {
		*list_head = rm_node->next;
	/** remove the association with hdl-mngr 
         **/
		ncshm_give_hdl(rm_node->reader_hdl);
		ncshm_destroy_hdl(NCS_SERVICE_ID_NTFA, rm_node->reader_hdl);
		free(rm_node);
		return NCSCC_RC_SUCCESS;
	} else {		/* find the rec */

		while (NULL != list_iter) {
			if (list_iter->next == rm_node) {
				list_iter->next = rm_node->next;
		/** remove the association with hdl-mngr 
                 **/
				ncshm_give_hdl(rm_node->reader_hdl);
				ncshm_destroy_hdl(NCS_SERVICE_ID_NTFA, rm_node->reader_hdl);
				free(rm_node);
				return NCSCC_RC_SUCCESS;
			}
			/* move onto the next one */
			list_iter = list_iter->next;
		}
	}
    /** The node couldn't be deleted **/
	TRACE("The node couldn't be deleted");
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : ntfa_add_to_async_cbk_msg_list
 
  Description   : This routine create linked list 
 
  Arguments     : CLMSV_MSG ** head
                  CLMSV_MSG * new_node

  Return Values : None
 
  Notes         : 
******************************************************************************/
void ntfa_add_to_async_cbk_msg_list(ntfsv_msg_t ** head, ntfsv_msg_t * new_node)
{
	TRACE_ENTER();
	ntfsv_msg_t *temp;

	if (*head == NULL) {
		new_node->next = NULL;
		*head = new_node;
		TRACE("in the head");
	} else {
		temp = *head;
		while (temp->next != NULL)
			temp = temp->next;

		TRACE("in the tail");
		new_node->next = NULL;
		temp->next = new_node;
	}

	TRACE_LEAVE();
}
