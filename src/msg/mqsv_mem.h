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

  DESCRIPTION: MQSv messages Allocation & Free macros 
 
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#ifndef MSG_MQSV_MEM_H_
#define MSG_MQSV_MEM_H_

#include "base/ncssysf_mem.h"

/******************************************************************************
                           Service Sub IDs for MQSv -ASAPi
*******************************************************************************/
typedef enum {
	NCS_SERVICE_MQSV_SUB_ID_MQSV_EVT = 1,

	NCS_SERVICE_MQSV_SUB_ID_MAX	/* This should be the last id */
} NCS_SERVICE_MQSV_SUB_ID;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                        Memory Allocation and Release Macros 

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
#define m_MMGR_ALLOC_MQSV_EVT(svc_id)        m_NCS_MEM_ALLOC( \
                                                sizeof(MQSV_EVT), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                svc_id, \
                                                NCS_SERVICE_MQSV_SUB_ID_MQSV_EVT)

#define m_MMGR_FREE_MQSV_EVT(p, scv_id)      m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                scv_id, \
                                                NCS_SERVICE_MQSV_SUB_ID_MQSV_EVT)

#define m_MMGR_FREE_MQSV_OS_MEMORY(ptr) m_NCS_MEM_FREE(ptr,NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_OS_SVCS, \
                                   0)

#endif  // MSG_MQSV_MEM_H_
