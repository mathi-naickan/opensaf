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

  ******************************************************************************
  */

/** Module Inclusion Control...
 **/
#ifndef BASE_NCSSYSFPOOL_H_
#define BASE_NCSSYSFPOOL_H_

#include "base/ncs_hdl_pub.h"
#include "base/ncssysf_lck.h"
#include "base/ncssysf_mem.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*
** This file should eventually be deleted completely when all users have cleaned
** up their macros!
*/

#define m_NCS_MEM_ALLOC(nbytes, mem_region, service_id, sub_id) malloc((nbytes))
#define m_NCS_MEM_FREE(free_me, mem_region, service_id, sub_id) free((free_me))

char *ncs_fname(char *fpath);

#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCSSYSFPOOL_H_
