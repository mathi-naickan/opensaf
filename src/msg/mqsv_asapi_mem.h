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

  DESCRIPTION: ASAPi messages Allocation & Free macros 
 
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#ifndef MSG_MQSV_ASAPI_MEM_H_
#define MSG_MQSV_ASAPI_MEM_H_

#include "base/ncssysf_mem.h"

/******************************************************************************
                           Service Sub IDs for MQSv -ASAPi
*******************************************************************************/
typedef enum {
	NCS_SERVICE_MQSV_ASAPi_SUB_ID_ASAPi_DEFAULT_VAL = 1,
	NCS_SERVICE_MQSV_ASAPi_SUB_ID_ASAPi_CACHE_INFO,
	NCS_SERVICE_MQSV_ASAPi_SUB_ID_ASAPi_QUEUE_INFO,
	NCS_SERVICE_MQSV_ASAPi_SUB_ID_ASAPi_MSG_INFO,

	NCS_SERVICE_MQSV_ASAPi_SUB_ID_MAX	/* This should be the last id */
} NCS_SERVICE_MQSV_ASAPi_SUB_ID;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                        Memory Allocation and Release Macros 

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
#define m_MMGR_ALLOC_ASAPi_DEFAULT_VAL(mem_size, svc_id) m_NCS_MEM_ALLOC( \
                                                mem_size, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                svc_id, \
                                                NCS_SERVICE_MQSV_ASAPi_SUB_ID_ASAPi_DEFAULT_VAL)

#define m_MMGR_FREE_ASAPi_DEFAULT_VAL(p, scv_id) m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                scv_id, \
                                                NCS_SERVICE_MQSV_ASAPi_SUB_ID_ASAPi_DEFAULT_VAL)

#define m_MMGR_ALLOC_ASAPi_CACHE_INFO(svc_id) m_NCS_MEM_ALLOC( \
                                                sizeof(ASAPi_CACHE_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                svc_id, \
                                                NCS_SERVICE_MQSV_ASAPi_SUB_ID_ASAPi_CACHE_INFO)

#define m_MMGR_FREE_ASAPi_CACHE_INFO(p, scv_id) m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                scv_id, \
                                                NCS_SERVICE_MQSV_ASAPi_SUB_ID_ASAPi_CACHE_INFO)

#define m_MMGR_ALLOC_ASAPi_QUEUE_INFO(svc_id) m_NCS_MEM_ALLOC( \
                                                sizeof(ASAPi_QUEUE_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                svc_id, \
                                                NCS_SERVICE_MQSV_ASAPi_SUB_ID_ASAPi_QUEUE_INFO)

#define m_MMGR_FREE_ASAPi_QUEUE_INFO(p, scv_id) m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                scv_id, \
                                                NCS_SERVICE_MQSV_ASAPi_SUB_ID_ASAPi_QUEUE_INFO)

#define m_MMGR_ALLOC_ASAPi_MSG_INFO(svc_id) m_NCS_MEM_ALLOC( \
                                                sizeof(ASAPi_MSG_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                svc_id, \
                                                NCS_SERVICE_MQSV_ASAPi_SUB_ID_ASAPi_MSG_INFO)

#define m_MMGR_FREE_ASAPi_MSG_INFO(p, scv_id) m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                scv_id, \
                                                NCS_SERVICE_MQSV_ASAPi_SUB_ID_ASAPi_MSG_INFO)

#endif  // MSG_MQSV_ASAPI_MEM_H_
