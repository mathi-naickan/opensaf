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

#ifndef MSG_AGENT_MQA_DEF_H_
#define MSG_AGENT_MQA_DEF_H_

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
MQA_CLIENT_INFO *mqa_client_tree_find_and_add(MQA_CB *mqa_cb, SaMsgHandleT hdl_id, bool flag);
uint32_t mqa_client_tree_delete_node(MQA_CB *mqa_cb, MQA_CLIENT_INFO *client_info);
void mqa_queue_reader(NCSCONTEXT context);

MQA_QUEUE_INFO *mqa_queue_tree_find_and_add(MQA_CB *mqa_cb, SaMsgQueueHandleT hdl_id,
						     bool flag, MQA_CLIENT_INFO *client_info,
						     SaMsgQueueOpenFlagsT openFlags);
uint32_t mqa_queue_tree_delete_node(MQA_CB *mqa_cb, SaMsgQueueHandleT hdl_id);
MQA_TRACK_INFO *mqa_track_tree_find_and_add(MQA_CLIENT_INFO *client_info, SaNameT *group, bool flag);

/* function prototypes for mds registration and registration */
uint32_t mqa_mds_register(MQA_CB *mqa_cb);
void mqa_mds_unregister(MQA_CB *mqa_cb);

/* function prototypes for timeout table creation and destruction */

void mqa_timer_table_destroy(MQA_CB *mqa_cb);
uint32_t mqa_timer_table_init(MQA_CB *mqa_cb);
uint32_t mqa_stop_and_delete_timer(MQP_ASYNC_RSP_MSG *mqa_callbk_info);
uint32_t mqa_stop_and_delete_timer_by_invocation(void *key);
uint32_t mqa_create_and_start_senderid_timer(void);
uint32_t mqa_destroy_senderid_timers(MQA_CB *mqa_cb);
uint32_t mqa_create_and_start_timer(MQP_ASYNC_RSP_MSG *mqa_callback, SaInvocationT invocation);
/* queue prototypes */

void mqsv_mqa_callback_queue_destroy(struct mqa_client_info *client_info);
uint32_t mqsv_mqa_callback_queue_init(struct mqa_client_info *client_info);
uint32_t mqsv_mqa_callback_queue_write(struct mqa_cb *mqa_cb, SaMsgHandleT handle, MQP_ASYNC_RSP_MSG *clbk_info);

/* callback prototypes */
uint32_t mqa_hdl_callbk_dispatch_one(struct mqa_cb *cb, SaMsgHandleT msgHandle);
uint32_t mqa_hdl_callbk_dispatch_all(struct mqa_cb *cb, SaMsgHandleT msgHandle);
uint32_t mqa_hdl_callbk_dispatch_block(MQA_CB *mqa_cb, SaMsgHandleT msgHandle);
uint32_t mqa_asapi_msghandler(ASAPi_MSG_INFO *asapi_msg);

uint32_t mqa_mds_msg_sync_send(uint32_t mqa_mds_hdl,
				     MDS_DEST *destination, MQSV_EVT *i_evt, MQSV_EVT **o_evt, SaTimeT timeout);

uint32_t mqa_mds_msg_async_send(uint32_t mqa_mds_hdl, MDS_DEST *destination, MQSV_EVT *i_evt, uint32_t to_svc);

uint32_t mqa_mds_msg_sync_send_direct(uint32_t mqa_mds_hdl,
					    MDS_DEST *destination,
					    MQSV_DSEND_EVT *i_evt, MQSV_DSEND_EVT **o_evt, SaTimeT timeout, uint32_t length);

uint32_t mqa_mds_msg_sync_reply_direct(uint32_t mqa_mds_hdl,
					     MDS_DEST *destination,
					     MQSV_DSEND_EVT *i_evt,
					     SaTimeT timeout, MDS_SYNC_SND_CTXT *context, uint32_t length);

uint32_t mqa_mds_msg_async_send_direct(uint32_t mqa_mds_hdl,
					     MDS_DEST *destination,
					     MQSV_DSEND_EVT *i_evt,
					     uint32_t to_svc, MDS_SEND_PRIORITY_TYPE priority, uint32_t length);

uint32_t mqa_mds_msg_async_reply_direct(uint32_t mqa_mds_hdl,
					      MDS_DEST *destination,
					      MQSV_DSEND_EVT *i_evt, MDS_SYNC_SND_CTXT *context, uint32_t length);

void mqa_flx_log_reg();
void mqa_flx_log_dereg();

#endif  // MSG_AGENT_MQA_DEF_H_
