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

 _Public_ CPD abstractions and function prototypes

*******************************************************************************/

/*
 * Module Inclusion Control...
 */

#ifndef CKPT_CKPTD_CPD_DL_API_H_
#define CKPT_CKPTD_CPD_DL_API_H_

#include "base/ncsgl_defs.h"
#include "base/ncs_lib.h"

uint32_t cpd_lib_req(NCS_LIB_REQ_INFO *);

#endif  // CKPT_CKPTD_CPD_DL_API_H_
