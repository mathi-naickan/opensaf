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
#ifndef AVD_H
#define AVD_H

#include "logtrace.h"

#include "amf.h"
#include "imm.h"

#include "ncsencdec_pub.h"
#include "amf_d2nmsg.h"

/* Porting Include Files */
#include "def.h"

/* AMFD-AMFND common EDPs */
#include "amf_d2nedu.h"

#include "mbcsv_papi.h"
#include "ckpt.h"

/* Director specific include files */
#include "timer.h"
#include "util.h"
#include "cb.h"
#include "pg.h"

#include "evt.h"
#include "proc.h"

#include "mds.h"

/* Checkpointing specific include files */
#include "ckpt_msg.h"
#include "ckpt_edu.h"
#include "ckpt_updt.h"
#include <stdbool.h>

#endif
