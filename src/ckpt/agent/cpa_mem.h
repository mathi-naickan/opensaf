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

  This file contains macros for CPA memory operations.
  
******************************************************************************
*/

#ifndef CKPT_AGENT_CPA_MEM_H_
#define CKPT_AGENT_CPA_MEM_H_

#include "base/ncssysf_mem.h"

/* Service Sub IDs for CPA */
typedef enum {
	CPA_SVC_SUB_ID_CPA_CB = CPSV_SVC_SUB_ID_MAX + 1,
	CPA_SVC_SUB_ID_DEFAULT_VAL,
	CPA_SVC_SUB_ID_CPA_CLIENT_NODE,
	CPA_SVC_SUB_ID_CPA_LOCAL_CKPT_NODE,
	CPA_SVC_SUB_ID_CPA_SECT_ITER_NODE,
	CPA_SVC_SUB_ID_CPA_GLOBAL_CKPT_NODE,
	CPA_SVC_SUB_ID_CPA_CALLBACK_INFO,
	CPA_SVC_SUB_ID_SaCkptIOVectorElementT,
        CPA_SVC_SUB_ID_CPA_PROCESS_EVT_SYNC,
	CPA_SVC_SUB_ID_MAX
} CPA_SVC_SUB_ID;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                        Memory Allocation and Release Macros 

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

#define m_MMGR_ALLOC_CPA_CB               (CPA_CB*)m_NCS_MEM_ALLOC(sizeof(CPA_CB), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_CPA_CB)

#define m_MMGR_FREE_CPA_CB(p)             m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_CPA_CB)

#define m_MMGR_ALLOC_CPA_DEFAULT(size)    m_NCS_MEM_ALLOC(size, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_DEFAULT_VAL)

#define m_MMGR_FREE_CPA_DEFAULT(p)        m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_DEFAULT_VAL)

#define m_MMGR_ALLOC_CPA_CLIENT_NODE  m_NCS_MEM_ALLOC(sizeof(CPA_CLIENT_NODE), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_CPA_CLIENT_NODE)

#define m_MMGR_FREE_CPA_CLIENT_NODE(p)        m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_CPA_CLIENT_NODE)

#define m_MMGR_ALLOC_CPA_LOCAL_CKPT_NODE  m_NCS_MEM_ALLOC(sizeof(CPA_LOCAL_CKPT_NODE), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_CPA_LOCAL_CKPT_NODE)

#define m_MMGR_FREE_CPA_LOCAL_CKPT_NODE(p)        m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_CPA_LOCAL_CKPT_NODE)

#define m_MMGR_ALLOC_CPA_GLOBAL_CKPT_NODE   m_NCS_MEM_ALLOC(sizeof(CPA_GLOBAL_CKPT_NODE), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_CPA_GLOBAL_CKPT_NODE)

#define m_MMGR_FREE_CPA_GLOBAL_CKPT_NODE(p)  m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_CPA_GLOBAL_CKPT_NODE)

#define m_MMGR_ALLOC_CPA_SECT_ITER_NODE  m_NCS_MEM_ALLOC(sizeof(CPA_SECT_ITER_NODE), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_CPA_SECT_ITER_NODE)

#define m_MMGR_ALLOC_CPA_PROCESS_EVT_SYNC  m_NCS_MEM_ALLOC(sizeof(CPA_PROCESS_EVT_SYNC), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_CPA_PROCESS_EVT_SYNC)

#define m_MMGR_FREE_CPA_PROCESS_EVT_SYNC(p)      m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_CPA_PROCESS_EVT_SYNC)


#define m_MMGR_FREE_CPA_SECT_ITER_NODE(p)        m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_CPA_SECT_ITER_NODE)

#define m_MMGR_ALLOC_CPA_CALLBACK_INFO  m_NCS_MEM_ALLOC(sizeof(CPA_CALLBACK_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_CPA_CALLBACK_INFO)

#define m_MMGR_FREE_CPA_CALLBACK_INFO(p)        m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_CPA_CALLBACK_INFO)
#define m_MMGR_ALLOC_SaCkptIOVectorElementT(n) (SaCkptIOVectorElementT *)m_NCS_MEM_ALLOC((n) * sizeof(SaCkptIOVectorElementT), \
                                             NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_CPA, \
                                            CPA_SVC_SUB_ID_SaCkptIOVectorElementT)

#define m_MMGR_FREE_SaCkptIOVectorElementT(p)   m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPA, \
                                                CPA_SVC_SUB_ID_SaCkptIOVectorElementT)

#endif  // CKPT_AGENT_CPA_MEM_H_
