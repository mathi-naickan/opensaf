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

#ifndef LCK_AGENT_GLA_DL_API_H_
#define LCK_AGENT_GLA_DL_API_H_

 uint32_t gla_lib_req(NCS_LIB_REQ_INFO *req_info);
unsigned int ncs_gla_startup(void);
unsigned int ncs_gla_shutdown(void);

#endif  // LCK_AGENT_GLA_DL_API_H_
