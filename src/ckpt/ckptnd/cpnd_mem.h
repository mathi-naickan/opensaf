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

  This file contains macros for CPND memory operations.
  
******************************************************************************
*/

#ifndef CKPT_CKPTND_CPND_MEM_H_
#define CKPT_CKPTND_CPND_MEM_H_

#include "base/ncssysf_mem.h"

/* Service Sub IDs for CPND */
typedef enum {
	CPND_SVC_SUB_ID_CPND_CB = CPSV_SVC_SUB_ID_MAX + 1,
	CPND_SVC_SUB_ID_CPND_CKPT_CLIST_NODE,
	CPND_SVC_SUB_ID_CPND_CKPT_LIST_NODE,
	CPND_SVC_SUB_ID_CPND_CKPT_NODE,
	CPND_SVC_SUB_ID_CPND_CKPT_CLIENT_NODE,
	CPND_SVC_SUB_ID_CPND_ALL_REPL_EVT_NODE,
	CPND_SVC_SUB_ID_CPND_UPDATE_DEST_LIST_NODE,
	CPND_SVC_SUB_ID_CPSV_CPND_DEST_INFO,
	CPND_SVC_SUB_ID_CPSV_CPND_UPDATE_DEST_INFO,
	CPND_SVC_SUB_ID_CPND_CKPT_SECTION_INFO,
	CPND_SVC_SUB_ID_DEFAULT_VAL,
	CPND_SVC_SUB_ID_CPND_SYNC_SEND_NODE,
	CPND_SVC_SUB_ID_CPND_CPD_DEFERRED_REQ_NODE,

	CPND_SVC_SUB_ID_MAX
} CPND_SVC_SUB_ID;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                        Memory Allocation and Release Macros 

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

#define m_MMGR_ALLOC_CPND_CB               (CPND_CB*)m_NCS_MEM_ALLOC(sizeof(CPND_CB), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPND, \
                                                CPND_SVC_SUB_ID_CPND_CB)

#define m_MMGR_FREE_CPND_CB(p)             m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPND, \
                                                CPND_SVC_SUB_ID_CPND_CB)

#define m_MMGR_ALLOC_CPND_CKPT_NODE       (CPND_CKPT_NODE *)m_NCS_MEM_ALLOC(sizeof(CPND_CKPT_NODE), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPND, \
                                                CPND_SVC_SUB_ID_CPND_CKPT_NODE)

#define m_MMGR_FREE_CPND_CKPT_NODE(p)      m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPND, \
                                                CPND_SVC_SUB_ID_CPND_CKPT_NODE)

#define m_MMGR_ALLOC_CPND_CKPT_CLIENT_NODE      (CPND_CKPT_CLIENT_NODE *)m_NCS_MEM_ALLOC(sizeof(CPND_CKPT_CLIENT_NODE), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPND, \
                                                CPND_SVC_SUB_ID_CPND_CKPT_CLIENT_NODE)

#define m_MMGR_FREE_CPND_CKPT_CLIENT_NODE(p)      m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPND, \
                                                CPND_SVC_SUB_ID_CPND_CKPT_CLIENT_NODE)
#define m_MMGR_ALLOC_CPND_ALL_REPL_EVT_NODE    (CPSV_CPND_ALL_REPL_EVT_NODE*)m_NCS_MEM_ALLOC(sizeof(CPSV_CPND_ALL_REPL_EVT_NODE), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPND, \
                                                CPND_SVC_SUB_ID_CPND_ALL_REPL_EVT_NODE)

#define m_MMGR_FREE_CPND_ALL_REPL_EVT_NODE(p)      m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPND, \
                                                CPND_SVC_SUB_ID_CPND_ALL_REPL_EVT_NODE)

#define m_MMGR_ALLOC_CPND_CKPT_CLIST_NODE  (CPND_CKPT_CLLIST_NODE *)m_NCS_MEM_ALLOC(sizeof(CPND_CKPT_CLLIST_NODE),\
                                           NCS_MEM_REGION_PERSISTENT, \
                                           NCS_SERVICE_ID_CPND, \
                                           CPND_SVC_SUB_ID_CPND_CKPT_CLIST_NODE)

#define m_MMGR_FREE_CPND_CKPT_CLIST_NODE(p)  m_NCS_MEM_FREE(p,\
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_CPND, \
                                            CPND_SVC_SUB_ID_CPND_CKPT_CLIST_NODE)

#define m_MMGR_ALLOC_CPND_DEST_INFO   (CPSV_CPND_DEST_INFO*)m_NCS_MEM_ALLOC(sizeof(CPSV_CPND_DEST_INFO), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                           NCS_SERVICE_ID_CPND, \
                                           CPND_SVC_SUB_ID_CPSV_CPND_DEST_INFO)

#define m_MMGR_FREE_CPND_DEST_INFO(p)   m_NCS_MEM_FREE(p,\
                                               NCS_MEM_REGION_PERSISTENT, \
                                           NCS_SERVICE_ID_CPND, \
                                           CPND_SVC_SUB_ID_CPSV_CPND_DEST_INFO)

#define m_MMGR_ALLOC_CPND_UPDATE_DEST_INFO (CPSV_CPND_UPDATE_DEST*)m_NCS_MEM_ALLOC(sizeof(CPSV_CPND_UPDATE_DEST), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                           NCS_SERVICE_ID_CPND, \
                                           CPND_SVC_SUB_ID_CPSV_CPND_UPDATE_DEST_INFO)

#define m_MMGR_FREE_CPND_UPDATE_DEST_INFO(p)   m_NCS_MEM_FREE(p,\
                                               NCS_MEM_REGION_PERSISTENT, \
                                           NCS_SERVICE_ID_CPND, \
                                           CPND_SVC_SUB_ID_CPSV_CPND_UPDATE_DEST_INFO)

#define m_MMGR_ALLOC_CPND_CKPT_LIST_NODE   (CPND_CKPT_CKPT_LIST_NODE *)m_NCS_MEM_ALLOC(sizeof(CPND_CKPT_CKPT_LIST_NODE), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                           NCS_SERVICE_ID_CPND, \
                                           CPND_SVC_SUB_ID_CPND_CKPT_LIST_NODE)

#define m_MMGR_FREE_CPND_CKPT_LIST_NODE(p)   m_NCS_MEM_FREE(p,\
                                               NCS_MEM_REGION_PERSISTENT, \
                                           NCS_SERVICE_ID_CPND, \
                                           CPND_SVC_SUB_ID_CPND_CKPT_LIST_NODE)

#define m_MMGR_ALLOC_CPND_UPDATE_DEST_LIST_NODE (CPSV_CPND_UPDATE_DEST*)m_NCS_MEM_ALLOC(sizeof(CPSV_CPND_WRITE_DEST), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                           NCS_SERVICE_ID_CPND, \
                                           CPND_SVC_SUB_ID_CPND_UPDATE_DEST_LIST_NODE)

#define m_MMGR_FREE_CPND_UPDATE_DEST_LIST_NODE(p)   m_NCS_MEM_FREE(p,\
                                               NCS_MEM_REGION_PERSISTENT, \
                                           NCS_SERVICE_ID_CPND, \
                                           CPND_SVC_SUB_ID_CPND_UPDATE_DEST_LIST_NODE)

#define m_MMGR_ALLOC_CPND_CKPT_SECTION_INFO   (CPND_CKPT_SECTION_INFO *)m_NCS_MEM_ALLOC(sizeof(CPND_CKPT_SECTION_INFO), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                           NCS_SERVICE_ID_CPND, \
                                           CPND_SVC_SUB_ID_CPND_CKPT_SECTION_INFO)

#define m_MMGR_FREE_CPND_CPND_CKPT_SECTION_INFO(p)   m_NCS_MEM_FREE(p,\
                                               NCS_MEM_REGION_PERSISTENT, \
                                           NCS_SERVICE_ID_CPND, \
                                           CPND_SVC_SUB_ID_CPND_CKPT_SECTION_INFO)

#define m_MMGR_ALLOC_CPND_SYNC_SEND_NODE  (CPND_SYNC_SEND_NODE *)m_NCS_MEM_ALLOC(sizeof(CPND_SYNC_SEND_NODE), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                           NCS_SERVICE_ID_CPND, \
                                           CPND_SVC_SUB_ID_CPND_SYNC_SEND_NODE)

#define m_MMGR_FREE_CPND_SYNC_SEND_NODE(p)   m_NCS_MEM_FREE(p,\
                                               NCS_MEM_REGION_PERSISTENT, \
                                           NCS_SERVICE_ID_CPND, \
                                           CPND_SVC_SUB_ID_CPND_SYNC_SEND_NODE)

#define m_MMGR_ALLOC_CPND_CPD_DEFERRED_REQ_NODE (CPND_CPD_DEFERRED_REQ_NODE *)m_NCS_MEM_ALLOC(sizeof(CPND_CPD_DEFERRED_REQ_NODE), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                           NCS_SERVICE_ID_CPND, \
                                           CPND_SVC_SUB_ID_CPND_CPD_DEFERRED_REQ_NODE)

#define m_MMGR_FREE_CPND_CPD_DEFERRED_REQ_NODE(p)   m_NCS_MEM_FREE(p,\
                                                       NCS_MEM_REGION_PERSISTENT, \
                                                    NCS_SERVICE_ID_CPND, \
                                                   CPND_SVC_SUB_ID_CPND_CPD_DEFERRED_REQ_NODE)

#define m_MMGR_ALLOC_CPND_DEFAULT(size)    m_NCS_MEM_ALLOC(size, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPND, \
                                                CPND_SVC_SUB_ID_DEFAULT_VAL)

#define m_MMGR_FREE_CPND_DEFAULT(p)        m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_CPND, \
                                                CPND_SVC_SUB_ID_DEFAULT_VAL)

#endif  // CKPT_CKPTND_CPND_MEM_H_
