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

#ifndef EVT_EVTD_EDS_H_
#define EVT_EVTD_EDS_H_

#include "base/ncsgl_defs.h"

/* From /base/common/pubinc */
#include "base/ncs_svd.h"
#include "base/usrbuf.h"
#include "base/ncs_ubaid.h"
#include "base/ncsencdec_pub.h"
#include "base/ncs_lib.h"

/* From targsvcs/common/inc */
#include "mds/mds_papi.h"
#include "base/ncs_mda_pvt.h"
#include "mds/mds_papi.h"

/* MBCSV header */
#include "mbc/mbcsv_papi.h"

/* IMMSV header */
#include "imm/saf/saImmOi.h"
#include "osaf/immutil/immutil.h"

/* EDU header */
#include "base/ncs_edu_pub.h"
#include "base/ncs_saf_edu.h"

/** EDS specific files **/
#include "evt/edsv_msg.h"

#include "eds_cb.h"
#include "eds_dl_api.h"
#include "eds_evt.h"
#include "eds_mds.h"
#include "eds_mem.h"
#include "evt/edsv_mem.h"
#include "eds_amf.h"

/** EDSV specific files **/
#include "evt/edsv_defs.h"
#include "evt/edsv_util.h"

/* Edsv checkpoint header */
#include "eds_ckpt.h"

/* CLM header */
#include "clm/saf/saClm.h"

/* IMM Headers */
#include "imm/saf/saImmOi.h"

#include "base/ncssysf_def.h"

#include "base/daemon.h"

/* EDS CB global handle declaration */
uint32_t gl_eds_hdl;

#endif  // EVT_EVTD_EDS_H_
