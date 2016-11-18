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

  This module is the main include file for Global Lock Director (GLD).
  
******************************************************************************
*/

#ifndef LCK_LCKD_GLD_H_
#define LCK_LCKD_GLD_H_
typedef enum {

	NO_CRD_QA_VAL,
	V_CARD_QA_1,
	V_CARD_QA_2,

} V_CARD_QA;

/* Common Headers */
#include "base/ncsgl_defs.h"

/* From /base/common/inc */
#include "base/ncs_lib.h"
#include "base/ncs_ubaid.h"
#include "mbc/mbcsv_papi.h"
#include "mds/mds_papi.h"
#include "base/logtrace.h"
#include "base/ncs_edu_pub.h"
#include "base/ncsencdec_pub.h"
#include "mbc/mbcsv_papi.h"
#include "base/ncs_mda_pvt.h"
#include "base/ncs_mda_papi.h"
#include "base/ncs_util.h"
#include "base/ncssysf_def.h"
#include "base/ncs_main_papi.h"

#include "clm/saf/saClm.h"
#include "lck/glsv_defs.h"
#include "lck/glsv_lck.h"
#include "gld_cb.h"
#include "gld_amf.h"
#include "gld_dl_api.h"
#include "gld_api.h"
#include "lck/lckd/gld_evt.h"
#include "lck/lcknd/glnd_evt.h"
#include "gld_mds.h"
#include "gld_mem.h"
#include "lck/lcknd/glnd_mem.h"
#include "gld_edu.h"
#include "gld_tmr.h"
#include "gld_mbcsv.h"

#include "ckpt/cpsv_mem.h"
#include "ckpt/agent/cpa_mem.h"

/* IMM Headers */
#include "imm/saf/saImmOi.h"
#include "osaf/immutil/immutil.h"

#include "base/daemon.h"

#endif  // LCK_LCKD_GLD_H_
