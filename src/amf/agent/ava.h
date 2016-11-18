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

  This module is the main include file for Availability Agent (AvA).
  
******************************************************************************
*/

#ifndef AMF_AGENT_AVA_H_
#define AMF_AGENT_AVA_H_
#include "base/ncs_main_papi.h"
#include "base/ncsencdec_pub.h"
#include "amf/amf.h"
#include "amf/agent/ava_dl_api.h"
/* Porting Include Files */
#include "amf/agent/ava_def.h"

#include "amf/amf_amfparam.h"
#include "amf/amf_n2avamsg.h"
#include "amf/amf_n2avaedu.h"

#include "amf/agent/ava_hdl.h"
#include "amf/agent/ava_mds.h"
#include "amf/agent/ava_cb.h"
#include "base/osaf_extended_name.h"

#include "base/logtrace.h"

/* AvA CB global handle declaration */
extern uint32_t gl_ava_hdl;

#endif  // AMF_AGENT_AVA_H_
