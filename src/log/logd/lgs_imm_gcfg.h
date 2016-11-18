/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2016 The OpenSAF Foundation
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

#ifndef LOG_LOGD_LGS_IMM_GCFG_H_
#define LOG_LOGD_LGS_IMM_GCFG_H_

#include <string>

void lgs_start_gcfg_applier();
void lgs_stop_gcfg_applier();
std::string lgs_get_networkname();

#endif  // LOG_LOGD_LGS_IMM_GCFG_H_

