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

  DESCRIPTION:

  This file contains macros for EDA memory operations.
  
*****************************************************************************/
#ifndef EVT_AGENT_EDA_MEM_H_
#define EVT_AGENT_EDA_MEM_H_

#include "evt/agent/eda.h"

/* Service Sub IDs for EDA */
typedef enum {
	NCS_SERVICE_SUB_ID_EDA_CB = 1,
	NCS_SERVICE_SUB_ID_EDA_CLIENT_HDL_REC,
	NCS_SERVICE_SUB_ID_EDA_CHANNEL_HDL_REC,
	NCS_SERVICE_SUB_ID_EDA_EVENT_HDL_REC,
	NCS_SERVICE_SUB_ID_EDA_SUBSC_REC,
	NCS_SERVICE_SUB_ID_EDA_SUB_MAX
	    /* Refer to edsv_mem.h for more SUB_IDs */
} NCS_SERVICE_EDA_SUBID;

/****************************************
 * Memory Allocation and Release Macros *
 ***************************************/

/* SAF Handles: EVT, CHANNEL, and EVENT */
#define m_MMGR_ALLOC_EDA_CB               (EDA_CB *)m_NCS_MEM_ALLOC(sizeof(EDA_CB), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_EDA, \
                                                NCS_SERVICE_SUB_ID_EDA_CB)

#define m_MMGR_FREE_EDA_CB(p)              m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_EDA, \
                                                NCS_SERVICE_SUB_ID_EDA_CB)

#define m_MMGR_ALLOC_EDA_CLIENT_HDL_REC   (EDA_CLIENT_HDL_REC *)m_NCS_MEM_ALLOC(sizeof(EDA_CLIENT_HDL_REC), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_EDA, \
                                                NCS_SERVICE_SUB_ID_EDA_CLIENT_HDL_REC)

#define m_MMGR_FREE_EDA_CLIENT_HDL_REC(p)       m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_EDA, \
                                                NCS_SERVICE_SUB_ID_EDA_CLIENT_HDL_REC)

#define m_MMGR_ALLOC_EDA_CHANNEL_HDL_REC (EDA_CHANNEL_HDL_REC *)m_NCS_MEM_ALLOC(sizeof(EDA_CHANNEL_HDL_REC), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_EDA, \
                                                NCS_SERVICE_SUB_ID_EDA_CHANNEL_HDL_REC)

#define m_MMGR_FREE_EDA_CHANNEL_HDL_REC(p)  m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_EDA, \
                                                NCS_SERVICE_SUB_ID_EDA_CHANNEL_HDL_REC)

#define m_MMGR_ALLOC_EDA_EVENT_HDL_REC   (EDA_EVENT_HDL_REC *)m_NCS_MEM_ALLOC(sizeof(EDA_EVENT_HDL_REC), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_EDA, \
                                                NCS_SERVICE_SUB_ID_EDA_EVENT_HDL_REC)

#define m_MMGR_FREE_EDA_EVENT_HDL_REC(p)    m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_EDA, \
                                                NCS_SERVICE_SUB_ID_EDA_EVENT_HDL_REC)

#define m_MMGR_ALLOC_EDA_SUBSC_REC   (EDA_SUBSC_REC *)m_NCS_MEM_ALLOC(sizeof(EDA_SUBSC_REC), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDA_SUBSC_REC)

#define m_MMGR_FREE_EDA_SUBSC_REC(p)  m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDA_SUBSC_REC)

#endif  // EVT_AGENT_EDA_MEM_H_
