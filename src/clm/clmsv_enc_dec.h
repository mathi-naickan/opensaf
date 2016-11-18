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
 * licensing terms.
 *
 * Author(s):  Emerson Network Power
 */

#ifndef CLM_CLMSV_ENC_DEC_H_
#define CLM_CLMSV_ENC_DEC_H_

#include "clm/saf/saClm.h"
#include "base/ncsgl_defs.h"
#include "base/ncs_lib.h"
#include "mds/mds_papi.h"
#include "base/ncs_mda_pvt.h"
#include "mbc/mbcsv_papi.h"
#include "base/ncs_edu_pub.h"
#include "base/ncs_util.h"
#include "base/logtrace.h"
#include "clm/clmsv_msg.h"

uint32_t clmsv_decodeSaNameT(NCS_UBAID *uba, SaNameT *name);
uint32_t clmsv_decodeNodeAddressT(NCS_UBAID *uba, SaClmNodeAddressT *nodeAddress);
uint32_t clmsv_encodeSaNameT(NCS_UBAID *uba, SaNameT *name);

#endif  // CLM_CLMSV_ENC_DEC_H_
