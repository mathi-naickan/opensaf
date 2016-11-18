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

  This file contains macros for MQA memory operations.
  
******************************************************************************
*/

#ifndef MSG_AGENT_MQA_MEM_H_
#define MSG_AGENT_MQA_MEM_H_

/* Service Sub IDs for MQA */
typedef enum {
	NCS_SERVICE_MQA_SUB_ID_MQA_DEFAULT_VAL = 1,
	NCS_SERVICE_MQA_SUB_ID_MQA_CB,
	NCS_SERVICE_MQA_SUB_ID_MQA_PEND_CALLBK,
	NCS_SERVICE_MQA_SUB_ID_MQA_CALLBACK_INFO,
	NCS_SERVICE_MQA_SUB_ID_MQA_CLIENT_INFO,
	NCS_SERVICE_MQA_SUB_ID_MQA_TRACK_INFO,
	NCS_SERVICE_MQA_SUB_ID_MQA_TRACK_BUFFER_INFO,
	NCS_SERVICE_MQA_SUB_ID_MQA_QUEUE_INFO,
	NCS_SERVICE_MQA_SUB_ID_MQA_SENDERID_INFO,
	NCS_SERVICE_MQA_SUB_ID_MQSV_EVT,
	NCS_SERVICE_MQA_SUB_ID_MQA_TMR_NODE,
	NCS_SERVICE_MQA_SUB_ID_MQA_MQSV_MESSAGE,
	NCS_SERVICE_MQA_SUB_ID_MQA_MQP_CANCEL_REQ,
	NCS_SERVICE_MQA_SUB_ID_MQA_MQP_OPEN_RSP,
	NCS_SERVICE_MQA_SUB_ID_MAX
} NCS_SERVICE_MQA_SUB_ID;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                        Memory Allocation and Release Macros 

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

#define m_MMGR_ALLOC_MQA_DEFAULT_VAL(mem_size)  m_NCS_MEM_ALLOC( \
                                                mem_size, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_DEFAULT_VAL)

#define m_MMGR_FREE_MQA_DEFAULT_VAL(p)          if (p)  m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_DEFAULT_VAL)

#define m_MMGR_ALLOC_MQA_CLIENT_INFO                     (MQA_CLIENT_INFO*)m_NCS_MEM_ALLOC(sizeof(MQA_CLIENT_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_CLIENT_INFO)

#define m_MMGR_FREE_MQA_CLIENT_INFO(p)                if (p)    m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_CLIENT_INFO)

#define m_MMGR_ALLOC_MQA_TRACK_BUFFER_INFO(n)            (SaMsgQueueGroupNotificationT*)m_NCS_MEM_ALLOC(n *sizeof(SaMsgQueueGroupNotificationT), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_TRACK_BUFFER_INFO)

#define m_MMGR_FREE_MQA_TRACK_BUFFER_INFO(p)                if (p)    m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_TRACK_BUFFER_INFO)

#define m_MMGR_ALLOC_MQA_TRACK_INFO            (MQA_TRACK_INFO*)m_NCS_MEM_ALLOC(sizeof(MQA_TRACK_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_TRACK_INFO)

#define m_MMGR_FREE_MQA_TRACK_INFO(p)                if (p)    m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_TRACK_INFO)

#define m_MMGR_ALLOC_MQA_QUEUE_INFO                     (MQA_QUEUE_INFO*)m_NCS_MEM_ALLOC(sizeof(MQA_QUEUE_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_QUEUE_INFO)

#define m_MMGR_FREE_MQA_QUEUE_INFO(p)               if (p)     m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_QUEUE_INFO)

#define m_MMGR_ALLOC_MQA_SENDERID                     (MQA_SENDERID_INFO*)m_NCS_MEM_ALLOC(sizeof(MQA_SENDERID_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_SENDERID_INFO)

#define m_MMGR_FREE_MQA_SENDERID(p)                if (p)    m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_SENDERID_INFO)

#define m_MMGR_ALLOC_MQP_ASYNC_RSP_MSG          (MQP_ASYNC_RSP_MSG *)m_NCS_MEM_ALLOC(sizeof(MQP_ASYNC_RSP_MSG), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_PEND_CALLBK)

#define m_MMGR_FREE_MQP_ASYNC_RSP_MSG(p)        if (p) m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_PEND_CALLBK)

#define m_MMGR_ALLOC_MQA_EVT                    m_MMGR_ALLOC_MQSV_EVT(NCS_SERVICE_ID_MQA)

#define m_MMGR_FREE_MQA_EVT(p)                  if (p) m_MMGR_FREE_MQSV_EVT(p, NCS_SERVICE_ID_MQA)

#define m_MMGR_ALLOC_MQA_TMR_NODE               (MQA_TMR_NODE*)m_NCS_MEM_ALLOC(sizeof(MQA_TMR_NODE), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_TMR_NODE)

#define m_MMGR_FREE_MQA_TMR_NODE(p)                 if (p)   m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_TMR_NODE)

#define m_MMGR_ALLOC_MQA_MESSAGE(size)              (void *)m_NCS_MEM_ALLOC(size, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_MQSV_MESSAGE)

#define m_MMGR_FREE_MQA_MESSAGE(p)                if (p)    m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_MQSV_MESSAGE)

#define m_MMGR_ALLOC_MQA_CANCEL_REQ(size)              (void *)m_NCS_MEM_ALLOC(size, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_MQP_CANCEL_REQ)

#define m_MMGR_FREE_MQA_CANCEL_REQ(p)                if (p)    m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_MQP_CANCEL_REQ)

#define m_MMGR_ALLOC_MQA_OPEN_RSP(size)              (void *)m_NCS_MEM_ALLOC(size, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_MQP_OPEN_RSP)

#define m_MMGR_FREE_MQA_OPEN_RSP(p)                if (p)    m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_MQA, \
                                                NCS_SERVICE_MQA_SUB_ID_MQA_MQP_OPEN_RSP)

#endif  // MSG_AGENT_MQA_MEM_H_
