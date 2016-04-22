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

#ifndef NTFA_H
#define NTFA_H

#include <pthread.h>
#include <assert.h>
#include <limits.h>
#include <saNtf.h>

#include <ncs_main_papi.h>
#include <ncssysf_ipc.h>
#include <mds_papi.h>
#include <ncs_hdl_pub.h>
#include <ncsencdec_pub.h>
#include <ncs_util.h>
#include <logtrace.h>

#include "ntfsv_msg.h"
#include "ntfsv_defs.h"

#define NTFA_SVC_PVT_SUBPART_VERSION  1
#define NTFA_WRT_NTFS_SUBPART_VER_AT_MIN_MSG_FMT 1
#define NTFA_WRT_NTFS_SUBPART_VER_AT_MAX_MSG_FMT 1
#define NTFA_WRT_NTFS_SUBPART_VER_RANGE             \
        (NTFA_WRT_NTFS_SUBPART_VER_AT_MAX_MSG_FMT - \
         NTFA_WRT_NTFS_SUBPART_VER_AT_MIN_MSG_FMT + 1)

#define NTFA_VARIABLE_DATA_LIMIT (SHRT_MAX)

/*
 * Data associated with a notification
 */
typedef struct ntfa_notification_hdl_rec {
	SaNtfNotificationTypeT ntfNotificationType;
	unsigned int notification_hdl;	/* notifiction stream HDL from handle mgr */

	union {
		SaNtfObjectCreateDeleteNotificationT ntfObjectCreateDeleteNotification;
		SaNtfAttributeChangeNotificationT ntfAttributeChangeNotification;
		SaNtfStateChangeNotificationT ntfStateChangeNotification;
		SaNtfAlarmNotificationT ntfAlarmNotification;
		SaNtfSecurityAlarmNotificationT ntfSecurityAlarmNotification;
	} ntfNotification;
	struct ntfa_notification_hdl_rec *next;	/* next pointer for the list in ntfa_client_hdl_rec_t */
	struct ntfa_client_hdl_rec *parent_hdl;	/* Back Pointer to the client instantiation */
	SaNtfNotificationsT *cbk_notification;
	v_data variable_data;
	bool is_longdn_agent_owner;
} ntfa_notification_hdl_rec_t;

/*
 * Data associated with a notification filter
 */
typedef struct ntfa_filter_hdl_rec {
   SaNtfNotificationFilterHandleT filter_hdl;
	SaNtfNotificationTypeT ntfType;

	union {
		SaNtfObjectCreateDeleteNotificationFilterT objectCreateDeleteNotificationfilter;
		SaNtfAttributeChangeNotificationFilterT attributeChangeNotificationfilter;
		SaNtfStateChangeNotificationFilterT stateChangeNotificationfilter;
		SaNtfAlarmNotificationFilterT alarmNotificationfilter;
		SaNtfSecurityAlarmNotificationFilterT securityAlarmNotificationfilter;
	} notificationFilter; 

	SaNtfHandleT ntfHandle;
	 
	struct ntfa_filter_hdl_rec *next;	/* next pointer */
	struct ntfa_client_hdl_rec *parent_hdl;	/* Back Pointer to the client instantiation */
} ntfa_filter_hdl_rec_t;

/*
 * List of subscriptions made for filter handles of an ntf service instance
 */
typedef struct subscriberList {
	SaNtfHandleT subscriberListNtfHandle;
	SaNtfSubscriptionIdT subscriberListSubscriptionId;
	ntfsv_filter_ptrs_t filters; /* remember the filters used by this subscriber */
	struct subscriberList *prev;
	struct subscriberList *next;
} ntfa_subscriber_list_t;

/* NTFA reader record */
typedef struct ntfa_reader_hdl_rec {
	unsigned int reader_id;	/* handle value returned by NTFS for this client */
	SaNtfHandleT ntfHandle;
	unsigned int reader_hdl;	/* READER handle from handle mgr */

	ntfsv_filter_ptrs_t filters; /* remember the filters used by this reader */
	SaNtfSearchCriteriaT searchCriteria; /* remember the searchCriteria for recovery */

	struct ntfa_reader_hdl_rec *next;	/* next pointer for the list in ntfa_cb_t */
	struct ntfa_client_hdl_rec *parent_hdl;	/* Back Pointer to the client instantiation */
} ntfa_reader_hdl_rec_t;

/* NTFA client record */
typedef struct ntfa_client_hdl_rec {
	unsigned int ntfs_client_id;	/* handle value returned by NTFS for this client */
	unsigned int local_hdl;	/* LOG handle (derived from hdl-mngr) */
	SaNtfCallbacksT reg_cbk;	/* callbacks registered by the application */
	ntfa_notification_hdl_rec_t *notification_list;	/* List of Allocated notifications */
	ntfa_filter_hdl_rec_t *filter_list;	/* List of allocated filters */
	ntfa_reader_hdl_rec_t *reader_list;
	SYSF_MBX mbx;		/* priority q mbx b/w MDS & Library */
	struct ntfa_client_hdl_rec *next;	/* next pointer for the list in ntfa_cb_t */
	bool valid;		/* handle is valid if it's known by NTF server, used for headless hydra */
	bool is_stale_client;  /* Status of client based on the CLM status of node.*/
	SaVersionT version; /* the API version is being used by client, used for recover after headless */
} ntfa_client_hdl_rec_t;

/*
 * The NTFA control block is the master anchor structure for all NTFA
 * instantiations within a process.
 */
typedef enum {
	NTFA_NTFSV_NONE = 0,
	NTFA_NTFSV_DOWN,
	NTFA_NTFSV_NO_ACTIVE,
	NTFA_NTFSV_NEW_ACTIVE,
	NTFA_NTFSV_UP
}ntfa_ntfsv_state_t;

typedef struct {
	pthread_mutex_t cb_lock;	/* CB lock */
	ntfa_client_hdl_rec_t *client_list;	/* NTFA client handle database */
	ntfa_reader_hdl_rec_t *reader_list;
	MDS_HDL mds_hdl;	/* MDS handle */
	MDS_DEST ntfs_mds_dest;	/* NTFS absolute/virtual address */

	/* NTFS NTFA sync params */
	int ntfs_sync_awaited;
	NCS_SEL_OBJ ntfs_sync_sel;
	SaUint32T ntf_var_data_limit;	/* max allowed variableDataSize */
	/* NTF Server state */
	ntfa_ntfsv_state_t ntfa_ntfsv_state;
	SaClmClusterChangesT clm_node_state; /*Reflects CLM status of this node(for future use).*/
} ntfa_cb_t;

/* ntfa_saf_api.c */
extern ntfa_cb_t ntfa_cb;

/* list of subscriptions for this process */
extern ntfa_subscriber_list_t *subscriberNoList;

/* ntfa_mds.c */
extern uint32_t ntfa_mds_init(ntfa_cb_t *cb);
extern void ntfa_mds_finalize(ntfa_cb_t *cb);
extern uint32_t ntfa_mds_msg_sync_send(ntfa_cb_t *cb, ntfsv_msg_t *i_msg, ntfsv_msg_t **o_msg, uint32_t timeout);
extern uint32_t ntfa_mds_msg_async_send(ntfa_cb_t *cb, ntfsv_msg_t *i_msg, uint32_t prio);
extern void ntfsv_ntfa_evt_free(struct ntfsv_msg *);

/* ntfa_init.c */
extern unsigned int ntfa_startup(void);
extern unsigned int ntfa_shutdown(bool forced);

/* ntfa_hdl.c */
extern SaAisErrorT ntfa_hdl_cbk_dispatch(ntfa_cb_t *, ntfa_client_hdl_rec_t *, SaDispatchFlagsT);
extern ntfa_client_hdl_rec_t *ntfa_hdl_rec_add(ntfa_cb_t *ntfa_cb, const SaNtfCallbacksT *reg_cbks, uint32_t client_id);
extern ntfa_notification_hdl_rec_t *ntfa_notification_hdl_rec_add(ntfa_client_hdl_rec_t **hdl_rec,
								  SaInt16T variableDataSize, SaAisErrorT *rc);
extern ntfa_filter_hdl_rec_t *ntfa_filter_hdl_rec_add(ntfa_client_hdl_rec_t **hdl_rec);
extern void ntfa_hdl_list_del(ntfa_client_hdl_rec_t **);
extern uint32_t ntfa_hdl_rec_del(ntfa_client_hdl_rec_t **, ntfa_client_hdl_rec_t *);
extern void ntfa_hdl_rec_force_del(ntfa_client_hdl_rec_t **, ntfa_client_hdl_rec_t *);
extern uint32_t ntfa_notification_hdl_rec_del(ntfa_notification_hdl_rec_t **, ntfa_notification_hdl_rec_t *);
extern uint32_t ntfa_filter_hdl_rec_del(ntfa_filter_hdl_rec_t **, ntfa_filter_hdl_rec_t *);
extern bool ntfa_validate_ntfa_client_hdl(ntfa_cb_t *ntfa_cb, ntfa_client_hdl_rec_t *find_hdl_rec);

/* ntfa_util.c */
extern ntfa_client_hdl_rec_t *ntfa_find_hdl_rec_by_client_id(ntfa_cb_t *ntfa_cb, uint32_t client_id);
extern void ntfa_msg_destroy(ntfsv_msg_t *msg);
extern void ntfa_notification_destructor(ntfa_notification_hdl_rec_t *instance);
extern void ntfa_filter_destructor(ntfa_filter_hdl_rec_t
					   *notificationFilterInstance);
extern ntfa_reader_hdl_rec_t *ntfa_reader_hdl_rec_add(ntfa_client_hdl_rec_t **hdl_rec);
extern uint32_t ntfa_reader_hdl_rec_del(ntfa_reader_hdl_rec_t **, ntfa_reader_hdl_rec_t *);
extern void ntfa_add_to_async_cbk_msg_list(ntfsv_msg_t ** head, ntfsv_msg_t * new_node);
extern uint32_t ntfa_ntfs_msg_proc(ntfa_cb_t *cb, ntfsv_msg_t *ntfsv_msg, MDS_SEND_PRIORITY_TYPE prio);
extern void ntfa_update_ntfsv_state(ntfa_ntfsv_state_t changedState);
extern SaAisErrorT ntfa_copy_ntf_filter_ptrs(ntfsv_filter_ptrs_t* pDes,
								const ntfsv_filter_ptrs_t* pSrc);
extern SaAisErrorT ntfa_del_ntf_filter_ptrs(ntfsv_filter_ptrs_t* filter_ptrs);
extern void ntfa_notify_handle_invalid(); 
#endif   /* !NTFA_H */
