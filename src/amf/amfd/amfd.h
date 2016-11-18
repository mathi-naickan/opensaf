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

  This module is the main include file for Availability Director.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AMF_AMFD_AMFD_H_
#define AMF_AMFD_AMFD_H_

#include <cstdint>
#include "base/logtrace.h"

#include "amf/amf.h"
#include "amf/amfd/imm.h"

#include "base/ncsencdec_pub.h"
#include "amf/amf_d2nmsg.h"

/* Porting Include Files */
#include "amf/amfd/def.h"

/* AMFD-AMFND common EDPs */
#include "amf/amf_d2nedu.h"

#include "mbc/mbcsv_papi.h"
#include "amf/amfd/ckpt.h"

/* Director specific include files */
#include "amf/amfd/timer.h"
#include "amf/amfd/util.h"
#include "amf/amfd/cb.h"
#include "amf/amfd/pg.h"

#include "amf/amfd/evt.h"
#include "amf/amfd/proc.h"

#include "mds.h"

/* Checkpointing specific include files */
#include "amf/amfd/ckpt_msg.h"
#include "ckpt_edu.h"
#include "ckpt_updt.h"
#include "amf/saf/saAmf.h"

#endif  // AMF_AMFD_AMFD_H_
