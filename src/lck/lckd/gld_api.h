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

#ifndef LCK_LCKD_GLD_API_H_
#define LCK_LCKD_GLD_API_H_

#include <stdint.h>
#include "amf/saf/saAmf.h"

uint32_t gld_se_lib_init(NCS_LIB_REQ_INFO *req_info);
uint32_t initialize_for_assignment(GLSV_GLD_CB *cb,
                                   SaAmfHAStateT ha_state);
uint32_t gld_se_lib_destroy(NCS_LIB_REQ_INFO *req_info);
void gld_process_mbx(SYSF_MBX *mbx);

uint32_t gld_cb_init(GLSV_GLD_CB *gld_cb);
uint32_t gld_cb_destroy(GLSV_GLD_CB *gld_cb);
bool gld_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg);
void gld_dump_cb();

#endif  // LCK_LCKD_GLD_API_H_
