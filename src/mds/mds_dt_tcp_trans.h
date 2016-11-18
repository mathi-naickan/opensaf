/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.z
 *
 * Author(s): GoAhead Software
 *
 */

#ifndef MDS_MDS_DT_TCP_TRANS_H_
#define MDS_MDS_DT_TCP_TRANS_H_

#include "mds_dt.h"
#include "mds_log.h"
#include "base/ncssysf_def.h"
#include "base/ncssysf_tsk.h"
#include "base/ncssysf_mem.h"


typedef enum dtm_lib_types {
  MDTM_LIB_UP_TYPE = 1,
  MDTM_LIB_DOWN_TYPE = 2,
  MDTM_LIB_NODE_UP_TYPE = 3,
  MDTM_LIB_NODE_DOWN_TYPE = 4,
  MDTM_LIB_MESSAGE_TYPE = 5,
} MDTM_LIB_TYPES;


uint32_t mds_mdtm_send_tcp(MDTM_SEND_REQ *req);

#endif  // MDS_MDS_DT_TCP_TRANS_H_
