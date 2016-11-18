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

  This file contains macros for memory operations for GLSV service that is common
  across the GLA,GLND and GLD
  
******************************************************************************
*/

#ifndef LCK_GLSV_MEM_H_
#define LCK_GLSV_MEM_H_

#include "base/ncssysf_mem.h"

/* Service Sub IDs for GLSV */
typedef enum {
	NCS_SERVICE_GLSV_SUB_ID_GLND_LOCK_LIST_INFO = 1,
	NCS_SERVICE_GLSV_SUB_ID_GLND_DD_INFO_LIST,
	NCS_SERVICE_GLSV_SUB_ID_MAX
} NCS_SERVICE_GLSV_SUB_ID;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                        Memory Allocation and Release Macros 

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

#define m_MMGR_ALLOC_GLSV_GLND_LOCK_LIST_INFO(mem_size,svc_id)       m_NCS_MEM_ALLOC( \
                                                                        mem_size, \
                                                                        NCS_MEM_REGION_PERSISTENT, \
                                                                        svc_id, \
                                                                        NCS_SERVICE_GLSV_SUB_ID_GLND_LOCK_LIST_INFO)
#define m_MMGR_FREE_GLSV_GLND_LOCK_LIST_INFO(p,svc_id)               m_NCS_MEM_FREE(p, \
                                                                        NCS_MEM_REGION_PERSISTENT, \
                                                                        svc_id, \
                                                                        NCS_SERVICE_GLSV_SUB_ID_GLND_LOCK_LIST_INFO)

#define m_MMGR_ALLOC_GLSV_GLND_DD_INFO_LIST(mem_size,svc_id)       m_NCS_MEM_ALLOC( \
                                                                        mem_size, \
                                                                        NCS_MEM_REGION_PERSISTENT, \
                                                                        svc_id, \
                                                                        NCS_SERVICE_GLSV_SUB_ID_GLND_DD_INFO_LIST)
#define m_MMGR_FREE_GLSV_GLND_DD_INFO_LIST(p,svc_id)               m_NCS_MEM_FREE(p, \
                                                                        NCS_MEM_REGION_PERSISTENT, \
                                                                        svc_id, \
                                                                        NCS_SERVICE_GLSV_SUB_ID_GLND_DD_INFO_LIST)

#endif  // LCK_GLSV_MEM_H_
