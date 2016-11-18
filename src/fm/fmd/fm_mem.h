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

#ifndef FM_FMD_FM_MEM_H_
#define FM_FMD_FM_MEM_H_

#include "base/ncssysf_mem.h"

typedef enum ncs_fm_service_sub_id {
	NCS_FM_SVC_SUB_ID_FM_EVT,
	NCS_FM_SVC_SUB_ID_FM_CB,
	NCS_FM_SVC_SUB_ID_FM_EDA,
	NCS_FM_SVC_SUB_ID_GFM_GFM,
	NCS_FM_SVC_SUB_ID_DEFAULT_VAL
} NCS_FM_SVC_SUB_ID;

#define m_MMGR_ALLOC_FM_CB         (FM_CB*)m_NCS_MEM_ALLOC(sizeof(FM_CB), \
                                   NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_GFM, \
                                   NCS_FM_SVC_SUB_ID_FM_CB)

#define m_MMGR_FREE_FM_CB(ptr)     m_NCS_MEM_FREE(ptr,NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_GFM, \
                                   NCS_FM_SVC_SUB_ID_FM_CB)

#define m_MMGR_ALLOC_FM_EVT        (FM_EVT*)m_NCS_MEM_ALLOC(sizeof(FM_EVT), \
                                   NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_GFM, \
                                   NCS_FM_SVC_SUB_ID_FM_EVT)

#define m_MMGR_FREE_FM_EVT(ptr)    m_NCS_MEM_FREE(ptr,NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_GFM, \
                                   NCS_FM_SVC_SUB_ID_FM_EVT)

#define m_MMGR_ALLOC_FM_FM_MSG     (GFM_GFM_MSG*)\
                                   m_NCS_MEM_ALLOC(sizeof(GFM_GFM_MSG), \
                                   NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_GFM, \
                                   NCS_FM_SVC_SUB_ID_GFM_GFM)

#define m_MMGR_FREE_FM_FM_MSG(p)   m_NCS_MEM_FREE(p, \
                                   NCS_MEM_REGION_PERSISTENT, \
                                   NCS_SERVICE_ID_GFM, \
                                   NCS_FM_SVC_SUB_ID_GFM_GFM)

#endif  // FM_FMD_FM_MEM_H_
