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

  This module is the main include file for Global Lock Director (GLD).
  
******************************************************************************
*/

#ifndef LCK_LCKD_GLD_DL_API_H_
#define LCK_LCKD_GLD_DL_API_H_

#include "base/ncsgl_defs.h"
#include "base/ncs_lib.h"

uint32_t gl_gld_hdl;
uint32_t gld_lib_req(NCS_LIB_REQ_INFO *req_info);

#endif  // LCK_LCKD_GLD_DL_API_H_
