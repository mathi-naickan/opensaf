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

  DESCRIPTION: CPSv messages Allocation & Free macros 
 
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#ifndef CKPT_CPSV_MEM_H_
#define CKPT_CPSV_MEM_H_

#include "base/ncssysf_mem.h"

/******************************************************************************
                           Service Sub IDs Commonly used by CPA, CPND & CPD
*******************************************************************************/
typedef enum {
	CPSV_SVC_SUB_ID_CPSV_EVT = 1,
	CPSV_SVC_SUB_ID_CPSV_ND2A_READ_MAP,
	CPSV_SVC_SUB_ID_CPSV_ND2A_READ_DATA,
	CPSV_SVC_SUB_ID_CPSV_SaUint32T,
	CPSV_SVC_SUB_ID_CPSV_SaCkptSectionIdT,
	CPSV_SVC_SUB_ID_CPSV_DEFAULT_VAL,
	CPSV_SVC_SUB_ID_CPSV_CKPT_DATA,

	CPSV_SVC_SUB_ID_MAX = CPSV_SVC_SUB_ID_CPSV_SaUint32T	/* This should be the last id */
} CPSV_SVC_SUB_ID;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                        Memory Allocation and Release Macros 

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
#define m_MMGR_ALLOC_CPSV_EVT(svc_id)  (CPSV_EVT *)m_NCS_MEM_ALLOC( \
                                                      sizeof(CPSV_EVT), \
                                                      NCS_MEM_REGION_PERSISTENT, \
                                                      svc_id, \
                                                      CPSV_SVC_SUB_ID_CPSV_EVT)

#define m_MMGR_FREE_CPSV_EVT(p, scv_id)      m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                scv_id, \
                                                CPSV_SVC_SUB_ID_CPSV_EVT)

#define m_MMGR_ALLOC_CPSV_ND2A_READ_MAP(cnt, svc_id)                          \
          (CPSV_ND2A_READ_MAP *)m_NCS_MEM_ALLOC((cnt* sizeof(CPSV_ND2A_READ_MAP)), \
                                                 NCS_MEM_REGION_PERSISTENT, \
                                                 svc_id, \
                                                 CPSV_SVC_SUB_ID_CPSV_ND2A_READ_MAP)

#define m_MMGR_FREE_CPSV_ND2A_READ_MAP(p, scv_id)      m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                scv_id, \
                                                CPSV_SVC_SUB_ID_CPSV_ND2A_READ_MAP)

#define m_MMGR_ALLOC_CPSV_ND2A_READ_DATA(cnt, svc_id)                          \
          (CPSV_ND2A_READ_DATA *)m_NCS_MEM_ALLOC((cnt* sizeof(CPSV_ND2A_READ_DATA)), \
                                                 NCS_MEM_REGION_PERSISTENT, \
                                                 svc_id, \
                                                 CPSV_SVC_SUB_ID_CPSV_ND2A_READ_DATA)

#define m_MMGR_FREE_CPSV_ND2A_READ_DATA(p, scv_id)      m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                scv_id, \
                                                CPSV_SVC_SUB_ID_CPSV_ND2A_READ_DATA)

#define m_MMGR_ALLOC_CPSV_SaUint32T(cnt, svc_id)                          \
          (SaUint32T *)m_NCS_MEM_ALLOC((cnt* sizeof(SaUint32T)), \
                                                 NCS_MEM_REGION_PERSISTENT, \
                                                 svc_id, \
                                                 CPSV_SVC_SUB_ID_CPSV_SaUint32T)

#define m_MMGR_FREE_CPSV_SaUint32T(p, scv_id)      m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                scv_id, \
                                                CPSV_SVC_SUB_ID_CPSV_SaUint32T)
#define m_MMGR_ALLOC_CPSV_SaCkptSectionIdT(svc_id)                          \
          (SaCkptSectionIdT *)m_NCS_MEM_ALLOC((sizeof(SaCkptSectionIdT)), \
                                                 NCS_MEM_REGION_PERSISTENT, \
                                                 svc_id, \
                                                 CPSV_SVC_SUB_ID_CPSV_SaCkptSectionIdT)

#define m_MMGR_FREE_CPSV_SaCkptSectionIdT(p, scv_id)      m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                scv_id, \
                                                CPSV_SVC_SUB_ID_CPSV_SaCkptSectionIdT)

#define m_MMGR_ALLOC_CPSV_DEFAULT_VAL(cnt, svc_id)                          \
                                          (uint8_t *)m_NCS_MEM_ALLOC(cnt, \
                                                 NCS_MEM_REGION_PERSISTENT, \
                                                 svc_id, \
                                                 CPSV_SVC_SUB_ID_CPSV_DEFAULT_VAL)

#define m_MMGR_FREE_CPSV_DEFAULT_VAL(p, scv_id)      m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                scv_id, \
                                                CPSV_SVC_SUB_ID_CPSV_DEFAULT_VAL)

#define m_MMGR_ALLOC_CPSV_CKPT_DATA     (CPSV_CKPT_DATA *)m_NCS_MEM_ALLOC(sizeof(CPSV_CKPT_DATA), \
                                             NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_CPND, \
                                            CPSV_SVC_SUB_ID_CPSV_CKPT_DATA)
#define m_MMGR_FREE_CPSV_CKPT_DATA(p)   m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPND, \
                                                CPSV_SVC_SUB_ID_CPSV_CKPT_DATA)

#define m_MMGR_ALLOC_CPSV_SYS_MEMORY(size)  m_NCS_MEM_ALLOC(size,\
                                           NCS_MEM_REGION_PERSISTENT, \
                                           NCS_SERVICE_ID_OS_SVCS, \
                                           0)

#define m_MMGR_FREE_CPSV_SYS_MEMORY(p)   m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_OS_SVCS, \
                                                0)

#endif  // CKPT_CPSV_MEM_H_
