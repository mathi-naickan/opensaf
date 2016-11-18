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

G_GLND_MEMFAIL(GLND_RESTART_RSC_LOCK_LIST_ALLOC_FAILED);
      return NULL;
  }

..............................................................................

  DESCRIPTION:

  This module is the main include file for Global Lock Node Director (GLND).
  
******************************************************************************
*/

#ifndef LCK_LCKND_GLND_H_
#define LCK_LCKND_GLND_H_

/* Common Headers */
#include "base/ncsgl_defs.h"

/* From /base/common/inc */
#include "base/ncs_lib.h"
#include "base/ncs_ubaid.h"
#include "mds/mds_papi.h"
#include "base/ncs_tmr.h"
#include "base/logtrace.h"
#include "base/ncs_edu_pub.h"
#include "base/ncsencdec_pub.h"
#include "base/ncs_util.h"
#include "base/ncs_main_papi.h"
#include "base/ncssysf_tsk.h"

/* GLSV common Include Files */
#include "lck/glsv_defs.h"
#include "lck/glsv_lck.h"

#include "glnd_tmr.h"
#include "glnd_cb.h"
#include "glnd_client.h"
#include "glnd_res.h"
#include "lck/lcknd/glnd_mem.h"
#include "lck/agent/gla_callbk.h"
#include "glnd_api.h"
#include "glnd_dl_api.h"
#include "glnd_restart.h"

#include "ckpt/saf/saCkpt.h"

/* events includes */
#include "lck/agent/gla_evt.h"
#include "lck/lckd/gld_evt.h"
#include "lck/lcknd/glnd_evt.h"

#include "glnd_mds.h"
#include "lck/lcknd/glnd_edu.h"

#include "lck/glsv_mem.h"
#include "base/daemon.h"

/*
#include "ckpt/cpsv_mem.h"
#include "ckpt/agent/cpa_mem.h"
*/

#endif  // LCK_LCKND_GLND_H_
