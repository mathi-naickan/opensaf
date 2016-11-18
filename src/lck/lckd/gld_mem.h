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

#ifndef LCK_LCKD_GLD_MEM_H_
#define LCK_LCKD_GLD_MEM_H_

#include "base/ncssysf_mem.h"

/* Service Sub IDs for GLD */
typedef enum {
	NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_CB = 1,
	NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_DEFAULT_VAL,
	NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_RSC_INFO,
	NCS_SERVICE_GLD_SUB_ID_GLSV_NODE_LIST,
	NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_GLND_DETAILS,
	NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_EVT,
	NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_A2S_RSC_INFO,
	NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_A2S_RSC_DETAILS,
	NCS_SERVICE_GLD_SUB_ID_GLSV_A2S_NODE_LIST,
	NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_A2S_EVT,
	NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_GLND_RSC_REF,
	NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_RSC_MAP_INFO,
	NCS_SERVICE_GLD_SUB_ID_MAX
} NCS_SERVICE_GLD_SUB_ID;

#define m_MMGR_ALLOC_GLSV_GLD_DEFAULT(size)     m_NCS_MEM_ALLOC(size, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_DEFAULT_VAL)
#define m_MMGR_FREE_GLSV_GLD_DEFAULT(size)      m_NCS_MEM_FREE(p, \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_DEFAULT_VAL)

#define m_MMGR_ALLOC_GLSV_GLD_CB                (GLSV_GLD_CB*)m_NCS_MEM_ALLOC(sizeof(GLSV_GLD_CB), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_CB)
#define m_MMGR_FREE_GLSV_GLD_CB(p)              m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_CB)

#define m_MMGR_ALLOC_GLSV_GLD_RSC_INFO          (GLSV_GLD_RSC_INFO*)m_NCS_MEM_ALLOC(sizeof(GLSV_GLD_RSC_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_RSC_INFO)
#define m_MMGR_FREE_GLSV_GLD_RSC_INFO(p)        m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_RSC_INFO)

#define m_MMGR_ALLOC_GLSV_GLD_RSC_MAP_INFO     (GLSV_GLD_RSC_MAP_INFO*)m_NCS_MEM_ALLOC(sizeof(GLSV_GLD_RSC_MAP_INFO), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_RSC_MAP_INFO)
#define m_MMGR_FREE_GLSV_GLD_RSC_MAP_INFO(p)    m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_RSC_MAP_INFO)

#define m_MMGR_ALLOC_GLSV_GLD_A2S_RSC_DETAILS  (GLSV_GLD_A2S_RSC_DETAILS *)m_NCS_MEM_ALLOC(sizeof(GLSV_GLD_A2S_RSC_DETAILS), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_A2S_RSC_DETAILS)

#define m_MMGR_FREE_GLSV_GLD_A2S_RSC_DETAILS(p) m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_A2S_RSC_DETAILS)

#define m_MMGR_ALLOC_A2S_GLSV_NODE_LIST         (GLSV_A2S_NODE_LIST *)m_NCS_MEM_ALLOC(sizeof(GLSV_A2S_NODE_LIST), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_A2S_NODE_LIST)
#define m_MMGR_FREE_A2S_GLSV_NODE_LIST(p)       m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_A2S_NODE_LIST)
#define m_MMGR_ALLOC_GLSV_NODE_LIST             (GLSV_NODE_LIST*)m_NCS_MEM_ALLOC(sizeof(GLSV_NODE_LIST), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_NODE_LIST)
#define m_MMGR_FREE_GLSV_NODE_LIST(p)           m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_NODE_LIST)

#define m_MMGR_ALLOC_GLSV_GLD_GLND_DETAILS      (GLSV_GLD_GLND_DETAILS*)m_NCS_MEM_ALLOC(sizeof(GLSV_GLD_GLND_DETAILS), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_GLND_DETAILS)
#define m_MMGR_FREE_GLSV_GLD_GLND_DETAILS(p)    m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_GLND_DETAILS)
#define m_MMGR_ALLOC_GLSV_GLD_EVT              (GLSV_GLD_EVT*)m_NCS_MEM_ALLOC(sizeof(GLSV_GLD_EVT), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_EVT)

#define m_MMGR_FREE_GLSV_GLD_EVT(p)             m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_EVT)

#define m_MMGR_ALLOC_GLSV_GLD_A2S_EVT           (GLSV_GLD_A2S_CKPT_EVT*)m_NCS_MEM_ALLOC(sizeof(GLSV_GLD_A2S_CKPT_EVT), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_A2S_EVT)

#define m_MMGR_FREE_GLSV_GLD_A2S_EVT(p)         m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_A2S_EVT)

#define m_MMGR_ALLOC_GLSV_GLD_GLND_RSC_REF     (GLSV_GLD_GLND_RSC_REF*)m_NCS_MEM_ALLOC(sizeof(GLSV_GLD_EVT), \
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_GLND_RSC_REF)

#define m_MMGR_FREE_GLSV_GLD_GLND_RSC_REF(p)    m_NCS_MEM_FREE(p,\
                                                NCS_MEM_REGION_PERSISTENT, \
                                                NCS_SERVICE_ID_GLD, \
                                                NCS_SERVICE_GLD_SUB_ID_GLSV_GLD_GLND_RSC_REF)

#endif  // LCK_LCKD_GLD_MEM_H_
