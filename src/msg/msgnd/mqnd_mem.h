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

#ifndef MSG_MSGND_MQND_MEM_H_
#define MSG_MSGND_MQND_MEM_H_

/* Sub service ID of MQND serices */
typedef enum ncs_service_MQND_sub_id {
	NCS_MQND_SVC_SUB_ID_CB,
	NCS_MQND_SVC_SUB_ID_QUEUE_NODE,
	NCS_MQND_SVC_SUB_ID_DEFAULT,
	NCS_SERVICE_MQND_SUB_ID_MQND_MQA_LIST_NODE,
	NCS_SERVICE_MQND_SUB_ID_MQND_QUEUE_CKPT_INFO,
	NCS_MQND_SVC_SUB_ID_QNAME_NODE,
	NCS_MQND_SVC_SUB_ID_QTRANSFER_EVT_NODE,
	NCS_MQND_SVC_SUB_ID_MAX
} NCS_SERVICE_MQND_SUB_ID;

#define m_MMGR_ALLOC_MQND_CB (MQND_CB*)m_NCS_MEM_ALLOC(sizeof(MQND_CB), \
                                   NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_MQND, \
                                   NCS_MQND_SVC_SUB_ID_CB)
#define m_MMGR_FREE_MQND_CB(ptr) m_NCS_MEM_FREE(ptr,NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_MQND, \
                                   NCS_MQND_SVC_SUB_ID_CB)

#define m_MMGR_ALLOC_MQND_QUEUE_NODE (MQND_QUEUE_NODE*)m_NCS_MEM_ALLOC(sizeof(MQND_QUEUE_NODE), \
                                   NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_MQND, \
                                   NCS_MQND_SVC_SUB_ID_QUEUE_NODE)
#define m_MMGR_FREE_MQND_QUEUE_NODE(ptr) m_NCS_MEM_FREE(ptr,NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_MQND, \
                                   NCS_MQND_SVC_SUB_ID_QUEUE_NODE)

#define m_MMGR_ALLOC_MQND_DEFAULT(size) m_NCS_MEM_ALLOC(size, \
                                   NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_MQND, \
                                   NCS_MQND_SVC_SUB_ID_DEFAULT)
#define m_MMGR_FREE_MQND_DEFAULT(ptr) m_NCS_MEM_FREE(ptr,NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_MQND, \
                                   NCS_MQND_SVC_SUB_ID_DEFAULT)

#define m_MMGR_ALLOC_MQND_MQA_LIST_NODE (MQND_MQA_LIST_NODE *) m_NCS_MEM_ALLOC( \
                                   sizeof(MQND_MQA_LIST_NODE), \
                                   NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_MQND, \
                                   NCS_SERVICE_MQND_SUB_ID_MQND_MQA_LIST_NODE)

#define m_MMGR_FREE_MQND_MQA_LIST_NODE(p) m_NCS_MEM_FREE(p, \
                                   NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_MQND, \
                                   NCS_SERVICE_MQND_SUB_ID_MQND_MQA_LIST_NODE)

#define m_MMGR_ALLOC_MQND_QUEUE_CKPT_INFO (MQND_QUEUE_CKPT_INFO *)m_NCS_MEM_ALLOC( \
                                   sizeof(MQND_QUEUE_CKPT_INFO), \
                                   NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_MQND, \
                                   NCS_SERVICE_MQND_SUB_ID_MQND_QUEUE_CKPT_INFO)

#define m_MMGR_FREE_MQND_QUEUE_CKPT_INFO(p) if(p) m_NCS_MEM_FREE(p, \
                                   NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_MQND, \
                                   NCS_SERVICE_MQND_SUB_ID_MQND_QUEUE_CKPT_INFO)

#define m_MMGR_ALLOC_MQND_QNAME_NODE (MQND_QNAME_NODE*)m_NCS_MEM_ALLOC(sizeof(MQND_QNAME_NODE), \
                                   NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_MQND, \
                                   NCS_MQND_SVC_SUB_ID_QNAME_NODE)
#define m_MMGR_FREE_MQND_QNAME_NODE(ptr) m_NCS_MEM_FREE(ptr,NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_MQND, \
                                   NCS_MQND_SVC_SUB_ID_QNAME_NODE)

#define m_MMGR_ALLOC_MQND_QTRANSFER_EVT_NODE (MQND_QTRANSFER_EVT_NODE*)m_NCS_MEM_ALLOC(sizeof(MQND_QTRANSFER_EVT_NODE), \
                                   NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_MQND, \
                                   NCS_MQND_SVC_SUB_ID_QTRANSFER_EVT_NODE)
#define m_MMGR_FREE_MQND_QTRANSFER_EVT_NODE(ptr) m_NCS_MEM_FREE(ptr,NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_MQND, \
                                   NCS_MQND_SVC_SUB_ID_QTRANSFER_EVT_NODE)

#endif  // MSG_MSGND_MQND_MEM_H_
