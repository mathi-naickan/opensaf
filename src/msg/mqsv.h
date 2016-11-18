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
  
  MQSv event definitions.
    
******************************************************************************
*/

#ifndef MSG_MQSV_H_
#define MSG_MQSV_H_

/* Get General Global Definations */
#include "base/ncsgl_defs.h"

/* Get target's suite of header files...*/

/* From /base/common/inc */
#include "base/ncs_lib.h"
#include "base/ncs_main_papi.h"
#include "mds/mds_papi.h"
#include "base/ncs_mda_papi.h"

#include "base/ncs_svd.h"
#include "base/usrbuf.h"
#include "base/ncs_ubaid.h"
#include "base/ncsdlib.h"
#include "base/ncs_queue.h"
#include "base/ncs_saf.h"
#include "base/ncs_tmr.h"
#include "base/ncssysf_tsk.h"

/* EDU Includes... */
#include "base/ncs_edu_pub.h"
#include "base/ncsencdec_pub.h"
#include "base/ncs_saf_edu.h"

/* SAF defines. */
#include "osaf/saf/saAis.h"
#include "msg/saf/saMsg.h"
#include "base/saf_def.h"

/* Common MQSv defines */
#include "base/saf_mem.h"
#include "mqsv_mem.h"
#include "mqsv_def.h"
#include "mqsv_evt.h"
#include "msg/mqsv_edu.h"
#include "mqsv_mbedu.h"
/* ASAPi defines */
#include "mqsv_asapi.h"
#include "stdlib.h"

/* From /leap/os_svcs/leap_basic/inc */
#include "mqsv_common.h"
#include "base/ncs_util.h"

#endif  // MSG_MQSV_H_
