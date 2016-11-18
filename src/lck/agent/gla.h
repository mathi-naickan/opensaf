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

  This module is the main include file for Global Lock Agent (GLA).
  
******************************************************************************
*/

#ifndef LCK_AGENT_GLA_H_
#define LCK_AGENT_GLA_H_

/* Common Headers */
#include "base/ncsgl_defs.h"
#include "base/ncs_util.h"

/* From /base/common/inc */
#include "base/ncs_lib.h"
#include "base/ncs_ubaid.h"
#include "mds/mds_papi.h"
#include "base/logtrace.h"
#include "base/ncsencdec_pub.h"
#include "base/ncs_main_papi.h"

/* GLA Porting Include Files */
#include "lck/glsv_defs.h"
#include "lck/glsv_lck.h"
#include "gla_mem.h"

#include "gla_dl_api.h"
#include "lck/agent/gla_callbk.h"
#include "lck/agent/gla_evt.h"
#include "lck/lcknd/glnd_evt.h"
#include "gla_cb.h"
#include "gla_mds.h"

#endif  // LCK_AGENT_GLA_H_
