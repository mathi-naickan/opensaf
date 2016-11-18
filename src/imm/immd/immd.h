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
 * Author(s): Ericsson AB
 *
 */

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:

  This module is the main include file for IMM Director (IMMD).

******************************************************************************
*/

#ifndef IMM_IMMD_IMMD_H_
#define IMM_IMMD_IMMD_H_

#include <stdint.h>
#include "amf/saf/saAmf.h"

#include "imm/immsv.h"
#include "mbc/mbcsv_papi.h"
#include "immd_cb.h"
#include "imm/immd/immd_proc.h"
#include "immd_mds.h"
#include "immd_red.h"
#include "immd_sbedu.h"
#include "base/ncs_mda_pvt.h"

IMMD_CB *immd_cb;

extern uint32_t initialize_for_assignment(IMMD_CB *cb, SaAmfHAStateT ha_state);

#endif  // IMM_IMMD_IMMD_H_
