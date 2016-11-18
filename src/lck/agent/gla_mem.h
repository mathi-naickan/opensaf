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

  This file contains macros for GLA memory operations.
  
******************************************************************************
*/

#ifndef LCK_AGENT_GLA_MEM_H_
#define LCK_AGENT_GLA_MEM_H_

/* Service Sub IDs for GLA */
typedef enum {
	NCS_SERVICE_GLA_SUB_ID_GLA_DEFAULT_VAL = 1,
	NCS_SERVICE_GLA_SUB_ID_GLA_CB,
	NCS_SERVICE_GLA_SUB_ID_GLA_PEND_CALLBK,
	NCS_SERVICE_GLA_SUB_ID_GLA_CALLBACK_INFO,
	NCS_SERVICE_GLA_SUB_ID_GLA_CLIENT_INFO,
	NCS_SERVICE_GLA_SUB_ID_GLA_EVT,
	NCS_SERVICE_GLA_SUB_ID_GLA_RES_ID_INFO,
	NCS_SERVICE_GLA_SUB_ID_GLA_LOCK_ID_INFO,
	NCS_SERVICE_GLA_SUB_ID_GLA_CLIENT_RES_INFO,
	NCS_SERVICE_GLA_SUB_ID_MAX
} NCS_SERVICE_GLA_SUB_ID;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                        Memory Allocation and Release Macros 

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

#define m_MMGR_ALLOC_GLA_DEFAULT_VAL(mem_size)  m_NCS_MEM_ALLOC( \
                                                mem_size, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLA, \
                                                NCS_SERVICE_GLA_SUB_ID_GLA_DEFAULT_VAL)

#define m_MMGR_FREE_GLA_DEFAULT_VAL(p)          m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLA, \
                                                NCS_SERVICE_GLA_SUB_ID_GLA_DEFAULT_VAL)

#define m_MMGR_ALLOC_GLA_CB                     (GLA_CB*)m_NCS_MEM_ALLOC(sizeof(GLA_CB), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLA, \
                                                NCS_SERVICE_GLA_SUB_ID_GLA_CB)

#define m_MMGR_FREE_GLA_CB(p)                   m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLA, \
                                                NCS_SERVICE_GLA_SUB_ID_GLA_CB)

#define m_MMGR_ALLOC_GLA_CALLBACK_INFO          (GLSV_GLA_CALLBACK_INFO *)m_NCS_MEM_ALLOC(sizeof(GLSV_GLA_CALLBACK_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLA, \
                                                NCS_SERVICE_GLA_SUB_ID_GLA_CALLBACK_INFO)

#define m_MMGR_FREE_GLA_CALLBACK_INFO(p)        m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLA, \
                                                NCS_SERVICE_GLA_SUB_ID_GLA_CALLBACK_INFO)

#define m_MMGR_ALLOC_GLA_CLIENT_INFO            (GLA_CLIENT_INFO *)m_NCS_MEM_ALLOC(sizeof(GLA_CLIENT_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLA, \
                                                NCS_SERVICE_GLA_SUB_ID_GLA_CLIENT_INFO)

#define m_MMGR_FREE_GLA_CLIENT_INFO(p)          m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLA, \
                                                NCS_SERVICE_GLA_SUB_ID_GLA_CLIENT_INFO)

#define m_MMGR_ALLOC_GLA_EVT                    (GLSV_GLA_EVT *)m_NCS_MEM_ALLOC(sizeof(GLSV_GLA_EVT), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLA, \
                                                NCS_SERVICE_GLA_SUB_ID_GLA_EVT)

#define m_MMGR_FREE_GLA_EVT(p)                  m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLA, \
                                                NCS_SERVICE_GLA_SUB_ID_GLA_EVT)

#define m_MMGR_ALLOC_GLA_RES_ID_INFO            (GLA_RESOURCE_ID_INFO *)m_NCS_MEM_ALLOC(sizeof(GLA_RESOURCE_ID_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLA, \
                                                NCS_SERVICE_GLA_SUB_ID_GLA_RES_ID_INFO)

#define m_MMGR_FREE_GLA_RES_ID_INFO(p)          m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLA, \
                                                NCS_SERVICE_GLA_SUB_ID_GLA_RES_ID_INFO)

#define m_MMGR_ALLOC_GLA_CLIENT_RES_INFO        (GLA_CLIENT_RES_INFO*)m_NCS_MEM_ALLOC(sizeof(GLA_CLIENT_RES_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLA, \
                                                NCS_SERVICE_GLA_SUB_ID_GLA_CLIENT_RES_INFO)

#define m_MMGR_FREE_GLA_CLIENT_RES_INFO(p)      m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLA, \
                                                NCS_SERVICE_GLA_SUB_ID_GLA_CLIENT_RES_INFO)

#define m_MMGR_ALLOC_GLA_LOCK_ID_INFO          (GLA_LOCK_ID_INFO *)m_NCS_MEM_ALLOC(sizeof(GLA_LOCK_ID_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLA, \
                                                NCS_SERVICE_GLA_SUB_ID_GLA_LOCK_ID_INFO)

#define m_MMGR_FREE_GLA_LOCK_ID_INFO(p)         m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLA, \
                                                NCS_SERVICE_GLA_SUB_ID_GLA_LOCK_ID_INFO)

#endif  // LCK_AGENT_GLA_MEM_H_
