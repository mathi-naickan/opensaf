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

  DESCRIPTION: The master include for all MBSCv and user *.C files.

  ******************************************************************************
  */

/*
 * Module Inclusion Control...
 */

#ifndef MBC_MBCSV_H_
#define MBC_MBCSV_H_

/* Get general definitions.....*/

#include "base/ncsgl_defs.h"

/* Get target's suite of header files...*/


/* From /base/common/inc */

#include "base/ncs_svd.h"
#include "base/usrbuf.h"
#include "base/ncs_ubaid.h"
#include "base/ncsencdec_pub.h"

#include "base/ncs_lib.h"

#include "base/ncs_mda_papi.h"
#include "mds/mds_papi.h"
#include "base/logtrace.h"
/* From targsvcs/common/inc */

#include "mds/mds_papi.h"

#include "osaf/saf/saAis.h"

typedef uint64_t MBCSV_ANCHOR;

/* From /base/products/rms/inc */
#include "mbc/mbcsv_papi.h"
#include "mbcsv_evts.h"
#include "mbcsv_tmr.h"
#include "mbcsv_evt_msg.h"
#include "mbcsv_env.h"
#include "mbcsv_mbx.h"
#include "mbcsv_pwe_anc.h"
#include "mbcsv_mem.h"
#include "mbcsv_mds.h"
#include "mbc/mbcsv_dl_api.h"

#endif  // MBC_MBCSV_H_
