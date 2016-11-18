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

#ifndef EVT_AGENT_EDA_H_
#define EVT_AGENT_EDA_H_

#include "base/ncsgl_defs.h"
#include "base/ncs_main_papi.h"
#include "base/ncs_svd.h"
#include "base/usrbuf.h"
#include "base/ncs_ubaid.h"
#include "base/ncsencdec_pub.h"
#include "base/ncs_lib.h"

#include "mds/mds_papi.h"

#include "evt/edsv_msg.h"
#include "eda_cb.h"
#include "eda_hdl.h"
#include "eda_mds.h"
#include "eda_mem.h"
#include "evt/edsv_mem.h"
#include "evt/edsv_defs.h"
#include "evt/edsv_util.h"
#include "base/logtrace.h"

/* EDA CB global handle declaration */
uint32_t gl_eda_hdl;

/* EDA Default MDS timeout value */
#define EDA_MDS_DEF_TIMEOUT 100

/* EDSv maximum length of a pattern size */
#define EDSV_MAX_PATTERN_SIZE 255

/*Length of a default publisher name */
#define EDSV_DEF_NAME_LEN 4

/*Max limit for reserved events (from 0 - 1000)*/
#define MAX_RESERVED_EVENTID 1000

#endif  // EVT_AGENT_EDA_H_
