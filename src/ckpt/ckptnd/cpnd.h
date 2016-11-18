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

  This module is the main include file for CheckPoint Agent (CPND).

******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef CKPT_CKPTND_CPND_H_
#define CKPT_CKPTND_CPND_H_

#include <stdint.h>
#include "base/osaf_extended_name.h"
#include "base/ncssysf_def.h"
#include "base/ncs_main_papi.h"
#include "base/ncssysf_tsk.h"
#include "ckpt/cpsv.h"
#include "cpnd_tmr.h"
#include "cpnd_mem.h"

#include "cpnd_dl_api.h"
#include "ckpt/cpsv_shm.h"
#include "cpnd_cb.h"

#include "cpnd_init.h"
#include "cpnd_sec.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include "ckpt/cpsv_shm.h"
#include "ckpt/cpsv_evt.h"

#endif  // CKPT_CKPTND_CPND_H_
