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

  This module is the main include file for IMM Node Director (IMMND).

*****************************************************************************/

#ifndef IMM_IMMND_IMMND_H_
#define IMM_IMMND_IMMND_H_

#ifdef HAVE_CONFIG_H
#include "osaf/config.h"
#endif

#include "imm/immsv.h"
#include "immnd_cb.h"
#include "immnd_init.h"

#ifdef HAVE_IMM_PBE
#define ENABLE_PBE 1
#else
#define ENABLE_PBE 0
#endif


#endif  // IMM_IMMND_IMMND_H_
