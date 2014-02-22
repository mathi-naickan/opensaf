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

#ifndef GLND_H
#define GLND_H

/* Common Headers */
#include "ncsgl_defs.h"

/* From /base/common/inc */
#include "ncs_lib.h"
#include "ncs_ubaid.h"
#include "mds_papi.h"
#include "ncs_tmr.h"
#include "logtrace.h"
#include "ncs_edu_pub.h"
#include "ncsencdec_pub.h"
#include "ncs_util.h"
#include "ncs_main_papi.h"
#include "ncssysf_tsk.h"

/* GLSV common Include Files */
#include "glsv_defs.h"
#include "glsv_lck.h"

#include "glnd_tmr.h"
#include "glnd_cb.h"
#include "glnd_client.h"
#include "glnd_res.h"
#include "glnd_mem.h"
#include "gla_callbk.h"
#include "glnd_api.h"
#include "glnd_dl_api.h"
#include "glnd_restart.h"

#include "saCkpt.h"

/* events includes */
#include "gla_evt.h"
#include "gld_evt.h"
#include "glnd_evt.h"

#include "glnd_mds.h"
#include "glnd_edu.h"

#include "glsv_mem.h"
#include <daemon.h>

/*
#include "cpsv_mem.h"
#include "cpa_mem.h"
*/

#endif   /* !GLND_H */
