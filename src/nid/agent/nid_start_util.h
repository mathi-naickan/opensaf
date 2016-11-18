/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2009 The OpenSAF Foundation
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

#ifndef NID_AGENT_NID_START_UTIL_H_
#define NID_AGENT_NID_START_UTIL_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include "osaf/saf/saAis.h"

extern unsigned int amf_comp_name_get_set_from_file(const char *env_name, SaNameT *dn);

#ifdef  __cplusplus
}
#endif

#endif  // NID_AGENT_NID_START_UTIL_H_
