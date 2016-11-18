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

  FILE NAME: mqd_api.h

..............................................................................

  DESCRIPTION:
  
  API decleration for MQD library..
    
******************************************************************************/
#ifndef MSG_MSGD_MQD_API_H_
#define MSG_MSGD_MQD_API_H_

#define MQD_COMP_NAME      "MQD"
#define MQD_TASK_PRIORITY  5
#define MQD_TASK_STACKSIZE NCS_STACKSIZE_HUGE
#define MQSV_MQD_REV       1	/* MQD Release 1.00 */

/*****************************EVT HANDLER ***********************************/
typedef uint32_t (*MQD_EVT_HANDLER) (struct mqsv_evt *, struct mqd_cb *);
/****************************************************************************/

uint32_t mqd_track_add(NCS_QUEUE *, MDS_DEST *, MDS_SVC_ID);
uint32_t mqd_track_del(NCS_QUEUE *, MDS_DEST *);
uint32_t mqd_mds_init(MQD_CB *);
uint32_t mqd_mds_shut(MQD_CB *);
uint32_t mqd_evt_process(MQSV_EVT *);
uint32_t mqd_asapi_evt_hdlr(MQSV_EVT *, MQD_CB *);
uint32_t mqd_ctrl_evt_hdlr(MQSV_EVT *, MQD_CB *);

uint32_t mqd_asapi_dereg_hdlr(MQD_CB *, ASAPi_DEREG_INFO *, MQSV_SEND_INFO *);
uint32_t mqd_mds_send_rsp(MQD_CB *cb, MQSV_SEND_INFO *s_info, MQSV_EVT *evt);
void mqd_nd_restart_update_dest_info(MQD_CB *pMqd, MDS_DEST dest);
void mqd_nd_down_update_info(MQD_CB *pMqd, MDS_DEST dest);
bool mqd_obj_cmp(void *key, void *elem);

#endif  // MSG_MSGD_MQD_API_H_
