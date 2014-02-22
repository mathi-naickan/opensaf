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

#ifndef _EDS_H
#define _EDS_H

#include "ncsgl_defs.h"

/* From /base/common/pubinc */
#include "ncs_svd.h"
#include "usrbuf.h"
#include "ncs_ubaid.h"
#include "ncsencdec_pub.h"
#include "ncs_lib.h"

/* From targsvcs/common/inc */
#include "mds_papi.h"
#include "ncs_mda_pvt.h"
#include "mds_papi.h"

/* MBCSV header */
#include "mbcsv_papi.h"

/* IMMSV header */
#include "saImmOi.h"
#include "immutil.h"

/* EDU header */
#include "ncs_edu_pub.h"
#include "ncs_saf_edu.h"

/** EDS specific files **/
#include "edsv_msg.h"

#include "eds_cb.h"
#include "eds_dl_api.h"
#include "eds_evt.h"
#include "eds_mds.h"
#include "eds_mem.h"
#include "edsv_mem.h"
#include "eds_amf.h"

/** EDSV specific files **/
#include "edsv_defs.h"
#include "edsv_util.h"

/* Edsv checkpoint header */
#include "eds_ckpt.h"

/* CLM header */
#include "saClm.h"

/* IMM Headers */
#include "saImmOi.h"

#include "ncssysf_def.h"

#include <daemon.h>

/* EDS CB global handle declaration */
uint32_t gl_eds_hdl;

#endif   /* _EDS_H */
