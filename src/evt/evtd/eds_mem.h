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
  
*****************************************************************************/

#ifndef EVT_EVTD_EDS_MEM_H_
#define EVT_EVTD_EDS_MEM_H_

#include "eds.h"
#include "base/ncssysf_mem.h"

/* Service Sub IDs for EDS */
typedef enum {
	NCS_SERVICE_EDS_REC = 1,
	NCS_SERVICE_EDS_WORKLIST,
	NCS_SERVICE_EDS_SUBLIST,
	NCS_SERVICE_EDS_SUBREC,
	NCS_SERVICE_EDS_COPEN_REC,
	NCS_SERVICE_EDS_COPEN_LIST,
	NCS_SERVICE_EDS_CHANNEL_NAME,
	NCS_SERVICE_EDS_FILTER,
	NCS_SERVICE_SUB_ID_EDS_CB,
	NCS_SERVICE_SUB_ID_EDSV_EDS_EVT,
	NCS_SERVICE_SUB_ID_EDS_RETAINED_EVT,
	NCS_SERVICE_EDS_CNAME_REC,
	NCS_SERVICE_EDA_DOWN_LIST,
	NCS_SERVICE_EDS_CLUSTER_NODE_LIST,
} NCS_SERVICE_EDS_SUBID;

/****************************************
 * Memory Allocation and Release Macros *
 ***************************************/

#define m_MMGR_ALLOC_EDS_REC(size)      m_NCS_MEM_ALLOC(size, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_REC)

#define m_MMGR_FREE_EDS_REC(p)          m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_REC)

#define m_MMGR_ALLOC_EDS_WORKLIST(size)     m_NCS_MEM_ALLOC(size, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_WORKLIST)

#define m_MMGR_FREE_EDS_WORKLIST(p)         m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_WORKLIST)

#define m_MMGR_ALLOC_EDS_SUBLIST(size)      m_NCS_MEM_ALLOC(size, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_SUBLIST)

#define m_MMGR_FREE_EDS_SUBLIST(p)          m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_SUBLIST)

#define m_MMGR_ALLOC_EDS_SUBREC(size)       m_NCS_MEM_ALLOC(size, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_SUBREC)

#define m_MMGR_FREE_EDS_SUBREC(p)           m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_SUBREC)

#define m_MMGR_ALLOC_EDS_COPEN_REC(size)    m_NCS_MEM_ALLOC(size, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_COPEN_REC)

#define m_MMGR_FREE_EDS_COPEN_REC(p)        m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_COPEN_REC)

#define m_MMGR_ALLOC_EDS_COPEN_LIST(size)   m_NCS_MEM_ALLOC(size, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_COPEN_LIST)

#define m_MMGR_FREE_EDS_COPEN_LIST(p)       m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_COPEN_LIST)

#define m_MMGR_ALLOC_EDS_CHAN_NAME(size)    m_NCS_MEM_ALLOC(size, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_CHANNEL_NAME)

#define m_MMGR_FREE_EDS_CHAN_NAME(p)        m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_CHANNEL_NAME)

/** New definitions **/

#define m_MMGR_ALLOC_EDS_CB                 (EDS_CB*)m_NCS_MEM_ALLOC(sizeof(EDS_CB),\
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_SUB_ID_EDS_CB)

#define m_MMGR_FREE_EDS_CB(p)               m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_SUB_ID_EDS_CB)

#define m_MMGR_ALLOC_EDSV_EDS_EVT           (EDSV_EDS_EVT *)m_NCS_MEM_ALLOC(sizeof(EDSV_EDS_EVT),\
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_SUB_ID_EDSV_EDS_EVT)

#define m_MMGR_FREE_EDSV_EDS_EVT(p)         m_NCS_MEM_FREE(p,\
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_SUB_ID_EDSV_EDS_EVT)

#define m_MMGR_ALLOC_EDS_RETAINED_EVT       (EDS_RETAINED_EVT_REC *)m_NCS_MEM_ALLOC(sizeof(EDS_RETAINED_EVT_REC),\
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_SUB_ID_EDS_RETAINED_EVT)

#define m_MMGR_FREE_EDS_RETAINED_EVT(p)     m_NCS_MEM_FREE(p,\
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_SUB_ID_EDS_RETAINED_EVT)

#define m_MMGR_ALLOC_EDS_CNAME_REC(size)    m_NCS_MEM_ALLOC(size, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_CNAME_REC)

#define m_MMGR_FREE_EDS_CNAME_REC(p)        m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_CNAME_REC)\

#define m_MMGR_ALLOC_EDA_DOWN_LIST(size)    m_NCS_MEM_ALLOC(size, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDA_DOWN_LIST)

#define m_MMGR_FREE_EDA_DOWN_LIST(p)        m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDA_DOWN_LIST)

#define m_MMGR_ALLOC_CLUSTER_NODE_LIST(size)    m_NCS_MEM_ALLOC(size, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_CLUSTER_NODE_LIST)

#define m_MMGR_FREE_CLUSTER_NODE_LIST(p)        m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDS, \
                                            NCS_SERVICE_EDS_CLUSTER_NODE_LIST)

#endif  // EVT_EVTD_EDS_MEM_H_
