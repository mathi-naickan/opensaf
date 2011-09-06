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
  
  MQSv event definitions.
    
******************************************************************************
*/

#ifndef MQA_DEF_H
#define MQA_DEF_H

/* All the defines go here. */
#define MQA_RELEASE_CODE 'B'
#define MQA_MAJOR_VERSION 3
#define MQA_MINOR_VERSION 1

#define MQA_BASE_MAJOR_VERSION 1
#define MQA_BASE_MINOR_VERSION 1

#define m_MQA_VER_IS_VALID(ver) \
   ( (ver->releaseCode == 'B') && \
     (ver->major == 0x01 || ver->major == 0xff) && \
     (ver->minor == 0x01 || ver->minor == 0xff) )

#define MQA_ASYNC_TIMEOUT_DEFAULT   10	/* timeout in secs  */
#define MQSV_SENDERID_CLEANUP_INTERVAL      100	/* timeout in s */
#define MQA_TRY_AGAIN_WAIT m_NCS_TASK_SLEEP(200)	/*Time in milli seconds */
#define m_MQSV_MQA_RETRIEVE_MQA_CB  ncshm_take_hdl(NCS_SERVICE_ID_MQA, gl_mqa_hdl)
#define m_MQSV_MQA_GIVEUP_MQA_CB    ncshm_give_hdl(gl_mqa_hdl)
/* to memset the name->value[length] to remaining bytes till the end of the array */
#define m_MQSV_SET_SANAMET(name) \
{\
   memset((void *)&name->value[name->length],0,(size_t)(SA_MAX_NAME_LENGTH - name->length)); \
}

/* function prototypes for client handling*/
EXTERN_C MQA_CLIENT_INFO *mqa_client_tree_find_and_add(MQA_CB *mqa_cb, SaMsgHandleT hdl_id, NCS_BOOL flag);
EXTERN_C uns32 mqa_client_tree_delete_node(MQA_CB *mqa_cb, MQA_CLIENT_INFO *client_info);
EXTERN_C void mqa_queue_reader(NCSCONTEXT context);

EXTERN_C MQA_QUEUE_INFO *mqa_queue_tree_find_and_add(MQA_CB *mqa_cb, SaMsgQueueHandleT hdl_id,
						     NCS_BOOL flag, MQA_CLIENT_INFO *client_info,
						     SaMsgQueueOpenFlagsT openFlags);
EXTERN_C uns32 mqa_queue_tree_delete_node(MQA_CB *mqa_cb, SaMsgQueueHandleT hdl_id);
EXTERN_C MQA_TRACK_INFO *mqa_track_tree_find_and_add(MQA_CLIENT_INFO *client_info, SaNameT *group, NCS_BOOL flag);

/* function prototypes for mds registration and registration */
EXTERN_C uns32 mqa_mds_register(MQA_CB *mqa_cb);
EXTERN_C void mqa_mds_unregister(MQA_CB *mqa_cb);

/* function prototypes for timeout table creation and destruction */

EXTERN_C void mqa_timer_table_destroy(MQA_CB *mqa_cb);
EXTERN_C uns32 mqa_timer_table_init(MQA_CB *mqa_cb);
EXTERN_C uns32 mqa_stop_and_delete_timer(MQP_ASYNC_RSP_MSG *mqa_callbk_info);
EXTERN_C uns32 mqa_stop_and_delete_timer_by_invocation(void *key);
EXTERN_C uns32 mqa_create_and_start_senderid_timer(void);
EXTERN_C uns32 mqa_destroy_senderid_timers(MQA_CB *mqa_cb);
EXTERN_C uns32 mqa_create_and_start_timer(MQP_ASYNC_RSP_MSG *mqa_callback, SaInvocationT invocation);
/* queue prototypes */

EXTERN_C void mqsv_mqa_callback_queue_destroy(struct mqa_client_info *client_info);
EXTERN_C uns32 mqsv_mqa_callback_queue_init(struct mqa_client_info *client_info);
EXTERN_C uns32 mqsv_mqa_callback_queue_write(struct mqa_cb *mqa_cb, SaMsgHandleT handle, MQP_ASYNC_RSP_MSG *clbk_info);

/* callback prototypes */
EXTERN_C uns32 mqa_hdl_callbk_dispatch_one(struct mqa_cb *cb, SaMsgHandleT msgHandle);
EXTERN_C uns32 mqa_hdl_callbk_dispatch_all(struct mqa_cb *cb, SaMsgHandleT msgHandle);
EXTERN_C uns32 mqa_hdl_callbk_dispatch_block(MQA_CB *mqa_cb, SaMsgHandleT msgHandle);
EXTERN_C uns32 mqa_asapi_msghandler(ASAPi_MSG_INFO *asapi_msg);

EXTERN_C uns32 mqa_mds_msg_sync_send(uns32 mqa_mds_hdl,
				     MDS_DEST *destination, MQSV_EVT *i_evt, MQSV_EVT **o_evt, uns32 timeout);

EXTERN_C uns32 mqa_mds_msg_async_send(uns32 mqa_mds_hdl, MDS_DEST *destination, MQSV_EVT *i_evt, uns32 to_svc);

EXTERN_C uns32 mqa_mds_msg_sync_send_direct(uns32 mqa_mds_hdl,
					    MDS_DEST *destination,
					    MQSV_DSEND_EVT *i_evt, MQSV_DSEND_EVT **o_evt, uns32 timeout, uns32 length);

EXTERN_C uns32 mqa_mds_msg_sync_reply_direct(uns32 mqa_mds_hdl,
					     MDS_DEST *destination,
					     MQSV_DSEND_EVT *i_evt,
					     uns32 timeout, MDS_SYNC_SND_CTXT *context, uns32 length);

EXTERN_C uns32 mqa_mds_msg_async_send_direct(uns32 mqa_mds_hdl,
					     MDS_DEST *destination,
					     MQSV_DSEND_EVT *i_evt,
					     uns32 to_svc, MDS_SEND_PRIORITY_TYPE priority, uns32 length);

EXTERN_C uns32 mqa_mds_msg_async_reply_direct(uns32 mqa_mds_hdl,
					      MDS_DEST *destination,
					      MQSV_DSEND_EVT *i_evt, MDS_SYNC_SND_CTXT *context, uns32 length);

EXTERN_C void mqa_flx_log_reg();
EXTERN_C void mqa_flx_log_dereg();

#endif
