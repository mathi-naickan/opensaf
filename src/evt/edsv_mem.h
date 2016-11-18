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
#ifndef EVT_EDSV_MEM_H_
#define EVT_EDSV_MEM_H_

#include "evt/agent/eda.h"
#include "base/ncssysf_mem.h"

/* Service Sub IDs for EDA */
typedef enum {
   /** Must start from where NCS_SERVICE_EDA_SUBID ends 
    ** as there is no service id for EDSV
    **/
	NCS_SERVICE_SUB_ID_EDSV_MSG = NCS_SERVICE_SUB_ID_EDA_SUB_MAX,
	NCS_SERVICE_SUB_ID_EDSV_EVENT_PATTERN_ARRAY,
	NCS_SERVICE_SUB_ID_EDSV_EVENT_PATTERNS,
	NCS_SERVICE_SUB_ID_EDSV_EVENT_FILTER_ARRAY,
	NCS_SERVICE_SUB_ID_EDSV_EVENT_FILTERS,
	NCS_SERVICE_SUB_ID_EDSV_EVENT_DATA,
	NCS_SERVICE_SUB_ID_EDSV_CKPT_MSG
} NCS_SERVICE_EDSV_SUBID;

/****************************************
 * Memory Allocation and Release Macros *
 ***************************************/
#define m_MMGR_ALLOC_EDSV_MSG        (EDSV_MSG *)m_NCS_MEM_ALLOC(sizeof(EDSV_MSG), \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_MSG)

#define m_MMGR_FREE_EDSV_MSG(p)         m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_MSG)

#define m_MMGR_ALLOC_EVENT_PATTERN_ARRAY  (SaEvtEventPatternArrayT *) \
                                             m_NCS_MEM_ALLOC(sizeof(SaEvtEventPatternArrayT), \
                                                       NCS_MEM_REGION_PERSISTENT, \
                                                       NCS_SERVICE_ID_EDA, \
                                                       NCS_SERVICE_SUB_ID_EDSV_EVENT_PATTERN_ARRAY)

#define m_MMGR_FREE_EVENT_PATTERN_ARRAY(p)   m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_EDA, \
                                                NCS_SERVICE_SUB_ID_EDSV_EVENT_PATTERN_ARRAY)

#define m_MMGR_ALLOC_EVENT_PATTERNS(n)  (SaEvtEventPatternT *) \
                                             m_NCS_MEM_ALLOC((n * sizeof(SaEvtEventPatternT)), \
                                                       NCS_MEM_REGION_PERSISTENT, \
                                                       NCS_SERVICE_ID_EDA, \
                                                       NCS_SERVICE_SUB_ID_EDSV_EVENT_PATTERNS)

#define m_MMGR_FREE_EVENT_PATTERNS(p)        m_NCS_MEM_FREE(p, \
                                                       NCS_MEM_REGION_PERSISTENT, \
                                                       NCS_SERVICE_ID_EDA, \
                                                       NCS_SERVICE_SUB_ID_EDSV_EVENT_PATTERNS)

#define m_MMGR_ALLOC_FILTER_ARRAY   (SaEvtEventFilterArrayT *) \
                                             m_NCS_MEM_ALLOC(sizeof(SaEvtEventFilterArrayT), \
                                                       NCS_MEM_REGION_PERSISTENT, \
                                                       NCS_SERVICE_ID_EDA, \
                                                       NCS_SERVICE_SUB_ID_EDSV_EVENT_FILTER_ARRAY)

#define m_MMGR_FREE_FILTER_ARRAY(p)          m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_EDA, \
                                                NCS_SERVICE_SUB_ID_EDSV_EVENT_FILTER_ARRAY)

#define m_MMGR_ALLOC_EVENT_FILTERS(n)  (SaEvtEventFilterT *) \
                                             m_NCS_MEM_ALLOC((n * sizeof(SaEvtEventFilterT)), \
                                                       NCS_MEM_REGION_PERSISTENT, \
                                                       NCS_SERVICE_ID_EDA, \
                                                       NCS_SERVICE_SUB_ID_EDSV_EVENT_FILTERS)

#define m_MMGR_FREE_EVENT_FILTERS(p)        m_NCS_MEM_FREE(p, \
                                                       NCS_MEM_REGION_PERSISTENT, \
                                                       NCS_SERVICE_ID_EDA, \
                                                       NCS_SERVICE_SUB_ID_EDSV_EVENT_FILTERS)

#define m_MMGR_ALLOC_EDSV_EVENT_DATA(size)   m_NCS_MEM_ALLOC( \
                                            size, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_EVENT_DATA)

#define m_MMGR_FREE_EDSV_EVENT_DATA(p)      m_NCS_MEM_FREE(p, \
                                            NCS_MEM_REGION_PERSISTENT, \
                                            NCS_SERVICE_ID_EDA, \
                                            NCS_SERVICE_SUB_ID_EDSV_EVENT_DATA)

#endif  // EVT_EDSV_MEM_H_
