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

  This file contains macros for memory operations.
  
******************************************************************************
*/

#ifndef LCK_LCKND_GLND_MEM_H_
#define LCK_LCKND_GLND_MEM_H_

#include "base/ncssysf_mem.h"

/* Service Sub IDs for GLND */
typedef enum {
	NCS_SERVICE_GLND_SUB_ID_GLND_DEFAULT_VAL = 1,
	NCS_SERVICE_GLND_SUB_ID_GLND_CB,
	NCS_SERVICE_GLND_SUB_ID_GLND_EVT,
	NCS_SERVICE_GLND_SUB_ID_GLND_CLIENT_INFO,
	NCS_SERVICE_GLND_SUB_ID_GLND_AGENT_INFO,
	NCS_SERVICE_GLND_SUB_ID_GLND_RESOURCE_INFO,
	NCS_SERVICE_GLND_SUB_ID_GLND_CLIENT_RES_LIST,
	NCS_SERVICE_GLND_SUB_ID_GLND_RESOURCE_REQ_LIST,
	NCS_SERVICE_GLND_SUB_ID_GLND_RES_LOCK_LIST_INFO,
	NCS_SERVICE_GLND_SUB_ID_GLND_RES_MASTER_LIST_INFO,
	NCS_SERVICE_GLND_SUB_ID_GLND_LOCK_LIST_INFO,
	NCS_SERVICE_GLND_SUB_ID_GLND_CLIENT_RES_LOCK_LIST_REQ,
	NCS_SERVICE_GLND_SUB_ID_GLND_DD_INFO_LIST,
	NCS_SERVICE_GLND_SUB_ID_GLND_RESTART_RES_INFO,
	NCS_SERVICE_GLND_SUB_ID_GLND_RESTART_CLIENT_INFO,
	NCS_SERVICE_GLND_SUB_ID_GLND_RESTART_RES_LOCK_LIST_INFO,
	NCS_SERVICE_GLND_SUB_ID_GLND_RESTART_BACKUP_EVT,
	NCS_SERVICE_GLND_SUB_ID_GLND_RESTART_RES_LIST,
	NCS_SERVICE_GLND_SUB_ID_GLND_RESTART_BACKUP_EVT_INFO,
	NCS_SERVICE_GLND_SUB_ID_MAX
} NCS_SERVICE_GLND_SUB_ID;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                        Memory Allocation and Release Macros 

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

#define m_MMGR_ALLOC_GLND_DEFAULT_VAL(mem_size) m_NCS_MEM_ALLOC( \
                                                mem_size, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_DEFAULT_VAL)

#define m_MMGR_FREE_GLND_DEFAULT_VAL(p)         m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_DEFAULT_VAL)

#define m_MMGR_ALLOC_GLND_CB                    (GLND_CB*)m_NCS_MEM_ALLOC(sizeof(GLND_CB), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_CB)

#define m_MMGR_FREE_GLND_CB(p)                  m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_CB)

#define m_MMGR_ALLOC_GLND_EVT                   (GLSV_GLND_EVT*)m_NCS_MEM_ALLOC(sizeof(GLSV_GLND_EVT), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_EVT)

#define m_MMGR_FREE_GLND_EVT(p)                 m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_EVT)

#define m_MMGR_ALLOC_GLND_CLIENT_INFO           (GLND_CLIENT_INFO*)m_NCS_MEM_ALLOC(sizeof(GLND_CLIENT_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_CLIENT_INFO)

#define m_MMGR_FREE_GLND_CLIENT_INFO(p)         m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_CLIENT_INFO)

#define m_MMGR_ALLOC_GLND_AGENT_INFO            (GLND_AGENT_INFO*)m_NCS_MEM_ALLOC(sizeof(GLND_AGENT_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_AGENT_INFO)

#define m_MMGR_FREE_GLND_AGENT_INFO(p)          m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_AGENT_INFO)

#define m_MMGR_ALLOC_GLND_RESOURCE_INFO         (GLND_RESOURCE_INFO*)m_NCS_MEM_ALLOC(sizeof(GLND_RESOURCE_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_RESOURCE_INFO)

#define m_MMGR_ALLOC_GLND_RESTART_RES_INFO         (GLND_RESTART_RES_INFO*)m_NCS_MEM_ALLOC(sizeof(GLND_RESTART_RES_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_RESTART_RES_INFO)

#define m_MMGR_FREE_GLND_RESTART_RES_INFO(p)    m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_RESTART_RES_INFO)

#define m_MMGR_ALLOC_GLND_RESTART_CLIENT_INFO         (GLSV_EVT_RESTART_CLIENT_INFO*)m_NCS_MEM_ALLOC(sizeof(GLSV_EVT_RESTART_CLIENT_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_RESTART_CLIENT_INFO)

#define m_MMGR_ALLOC_GLND_RESTART_BACKUP_EVT_INFO    (GLSV_RESTART_BACKUP_EVT_INFO*)m_NCS_MEM_ALLOC(sizeof(GLSV_RESTART_BACKUP_EVT_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_RESTART_BACKUP_EVT_INFO)

#define m_MMGR_FREE_GLND_RESTART_BACKUP_EVT_INFO(p)  m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_RESTART_BACKUP_EVT_INFO)

#define m_MMGR_ALLOC_GLND_RESTART_RES_LIST         (GLSV_EVT_RESTART_RES_INFO*)m_NCS_MEM_ALLOC(sizeof(GLSV_EVT_RESTART_RES_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_RESTART_RES_LIST)

#define m_MMGR_FREE_GLND_RESOURCE_INFO(p)       m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_RESOURCE_INFO)

#define m_MMGR_ALLOC_GLND_CLIENT_RES_LIST       (GLND_CLIENT_LIST_RESOURCE*)m_NCS_MEM_ALLOC(sizeof(GLND_CLIENT_LIST_RESOURCE), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_CLIENT_RES_LIST)

#define m_MMGR_FREE_GLND_CLIENT_RES_LIST(p)     m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_CLIENT_RES_LIST)

#define m_MMGR_ALLOC_GLND_RESOURCE_REQ_LIST     (GLND_RESOURCE_REQ_LIST*)m_NCS_MEM_ALLOC(sizeof(GLND_RESOURCE_REQ_LIST), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_RESOURCE_REQ_LIST)

#define m_MMGR_FREE_GLND_RESOURCE_REQ_LIST(p)   m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_RESOURCE_REQ_LIST)

#define m_MMGR_ALLOC_GLND_RES_LOCK_LIST_INFO    (GLND_RES_LOCK_LIST_INFO*)m_NCS_MEM_ALLOC(sizeof(GLND_RES_LOCK_LIST_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_RES_LOCK_LIST_INFO)

#define m_MMGR_ALLOC_GLND_RESTART_RES_LOCK_LIST_INFO    (GLND_RESTART_RES_LOCK_LIST_INFO*)m_NCS_MEM_ALLOC(sizeof(GLND_RESTART_RES_LOCK_LIST_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_RESTART_RES_LOCK_LIST_INFO)

#define m_MMGR_FREE_GLND_RESTART_RES_LOCK_LIST_INFO(p)  m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_RESTART_RES_LOCK_LIST_INFO)

#define m_MMGR_FREE_GLND_RES_LOCK_LIST_INFO(p)  m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_RES_LOCK_LIST_INFO)

#define m_MMGR_ALLOC_GLND_LOCK_LIST_INFO        (GLND_LOCK_LIST_INFO*)m_NCS_MEM_ALLOC(sizeof(GLND_LOCK_LIST_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_LOCK_LIST_INFO)

#define m_MMGR_FREE_GLND_LOCK_LIST_INFO(p)      m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_LOCK_LIST_INFO)

#define m_MMGR_ALLOC_GLND_CLIENT_RES_LOCK_LIST_REQ     (GLND_CLIENT_LIST_RESOURCE_LOCK_REQ*)m_NCS_MEM_ALLOC(sizeof(GLND_CLIENT_LIST_RESOURCE_LOCK_REQ), \
                                                       NCS_MEM_REGION_PERSISTENT, \
                                                       NCS_SERVICE_ID_GLND, \
                                                       NCS_SERVICE_GLND_SUB_ID_GLND_CLIENT_RES_LOCK_LIST_REQ)

#define m_MMGR_FREE_GLND_CLIENT_RES_LOCK_LIST_REQ(p)   m_NCS_MEM_FREE(p,\
                                                       NCS_MEM_REGION_PERSISTENT, \
                                                       NCS_SERVICE_ID_GLND, \
                                                       NCS_SERVICE_GLND_SUB_ID_GLND_CLIENT_RES_LOCK_LIST_REQ)

#define m_MMGR_ALLOC_GLND_DD_INFO_LIST                (GLSV_GLND_DD_INFO_LIST*)m_NCS_MEM_ALLOC(sizeof(GLND_CLIENT_LIST_RESOURCE_LOCK_REQ), \
                                                       NCS_MEM_REGION_PERSISTENT, \
                                                       NCS_SERVICE_ID_GLND, \
                                                       NCS_SERVICE_GLND_SUB_ID_GLND_DD_INFO_LIST)

#define m_MMGR_FREE_GLND_DD_INFO_LIST(p)               m_NCS_MEM_FREE(p,\
                                                       NCS_MEM_REGION_PERSISTENT, \
                                                       NCS_SERVICE_ID_GLND, \
                                                       NCS_SERVICE_GLND_SUB_ID_GLND_DD_INFO_LIST)

#define m_MMGR_ALLOC_GLND_RES_MASTER_LIST_INFO(cnt)  (GLSV_GLND_RSC_MASTER_INFO_LIST *) \
                                                m_NCS_MEM_ALLOC(cnt * sizeof(GLSV_GLND_RSC_MASTER_INFO_LIST), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_RES_MASTER_LIST_INFO)

#define m_MMGR_FREE_GLND_RES_MASTER_LIST_INFO(p)  m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLND, \
                                                NCS_SERVICE_GLND_SUB_ID_GLND_RES_MASTER_LIST_INFO)

#endif  // LCK_LCKND_GLND_MEM_H_
