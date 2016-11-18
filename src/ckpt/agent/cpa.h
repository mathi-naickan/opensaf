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

  This module is the main include file for CheckPoint Agent (CPA).

******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef CKPT_AGENT_CPA_H_
#define CKPT_AGENT_CPA_H_

#include "base/osaf_extended_name.h"
#include "ckpt/cpsv.h"
#include "cpa_dl_api.h"
#include "cpsv_papi.h"
#include "cpa_def.h"
#include "ckpt/agent/cpa_tmr.h"
#include "ckpt/cpsv_shm.h"
#include "cpa_cb.h"
#include "cpa_proc.h"
#include "cpa_mds.h"
#include "ckpt/agent/cpa_mem.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#endif  // CKPT_AGENT_CPA_H_
