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

#ifndef BASE_SAF_MEM_H_
#define BASE_SAF_MEM_H_

#include "base/ncssysf_mem.h"

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

  Memory Allocation and Release Macros

  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

#define m_MMGR_ALLOC_EDP_SAAMFERRORBUFFERT      m_NCS_MEM_ALLOC(        \
    sizeof(SaAmfErrorBufferT),                                          \
    NCS_MEM_REGION_PERSISTENT,                                          \
    NCS_SERVICE_ID_SAF_COMMON,                                          \
    0)
#define m_MMGR_FREE_EDP_SAAMFERRORBUFFERT(p)    m_NCS_MEM_FREE(p,       \
                                                               NCS_MEM_REGION_PERSISTENT, \
                                                               NCS_SERVICE_ID_SAF_COMMON, \
                                                               0)

#define m_MMGR_ALLOC_EDP_SAAMFADDITIONALDATAT      m_NCS_MEM_ALLOC(     \
    sizeof(SaAmfAdditionalDataT),                                       \
    NCS_MEM_REGION_PERSISTENT,                                          \
    NCS_SERVICE_ID_SAF_COMMON,                                          \
    1)
#define m_MMGR_FREE_EDP_SAAMFADDITIONALDATAT(p)    m_NCS_MEM_FREE(p,    \
                                                                  NCS_MEM_REGION_PERSISTENT, \
                                                                  NCS_SERVICE_ID_SAF_COMMON, \
                                                                  1)

#define m_MMGR_ALLOC_EDP_SAAMFHEALTHCHECKKEYT      m_NCS_MEM_ALLOC(     \
    sizeof(SaAmfHealthcheckKeyT),                                       \
    NCS_MEM_REGION_PERSISTENT,                                          \
    NCS_SERVICE_ID_SAF_COMMON,                                          \
    2)
#define m_MMGR_FREE_EDP_SAAMFHEALTHCHECKKEYT(p)    m_NCS_MEM_FREE(p,    \
                                                                  NCS_MEM_REGION_PERSISTENT, \
                                                                  NCS_SERVICE_ID_SAF_COMMON, \
                                                                  2)

#define m_MMGR_ALLOC_EDP_SACLMNODEADDRESST      m_NCS_MEM_ALLOC(        \
    sizeof(SaClmNodeAddressT),                                          \
    NCS_MEM_REGION_PERSISTENT,                                          \
    NCS_SERVICE_ID_SAF_COMMON,                                          \
    3)
#define m_MMGR_FREE_EDP_SACLMNODEADDRESST(p)    m_NCS_MEM_FREE(p,       \
                                                               NCS_MEM_REGION_PERSISTENT, \
                                                               NCS_SERVICE_ID_SAF_COMMON, \
                                                               3)
#define m_MMGR_ALLOC_EDP_SANAMET                m_NCS_MEM_ALLOC(        \
    sizeof(SaNameT),                                                    \
    NCS_MEM_REGION_PERSISTENT,                                          \
    NCS_SERVICE_ID_SAF_COMMON,                                          \
    3)
#define m_MMGR_FREE_EDP_SANAMET(p)              m_NCS_MEM_FREE(p,       \
                                                               NCS_MEM_REGION_PERSISTENT, \
                                                               NCS_SERVICE_ID_SAF_COMMON, \
                                                               3)

#endif  // BASE_SAF_MEM_H_
