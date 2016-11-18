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

  ******************************************************************************
  */
#ifndef MDS_MDA_MEM_H_
#define MDS_MDA_MEM_H_

#include "base/ncssysf_mem.h"

#define VDS_MEM_NCSVDA_INFO 1

#define m_MMGR_ALLOC_NCSVDA_INFO                                \
  (NCSVDA_INFO *)m_NCS_MEM_ALLOC(sizeof(NCSVDA_INFO),           \
                                 NCS_MEM_REGION_TRANSIENT,      \
                                 NCS_SERVICE_ID_VDS,            \
                                 VDS_MEM_NCSVDA_INFO)

#define m_MMGR_FREE_NCSVDA_INFO(p)  m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                                   NCS_SERVICE_ID_VDS,  \
                                                   VDS_MEM_NCSVDA_INFO)

#endif  // MDS_MDA_MEM_H_
