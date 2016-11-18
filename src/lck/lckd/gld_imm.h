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
 */

#ifndef LCK_LCKD_GLD_IMM_H_
#define LCK_LCKD_GLD_IMM_H_

#include "lck/lckd/gld.h"
#include "osaf/immutil/immutil.h"
#include "imm/saf/saImm.h"

extern void gld_imm_declare_implementer(GLSV_GLD_CB *cb);
extern void gld_imm_reinit_bg(GLSV_GLD_CB * cb);
extern SaAisErrorT gld_imm_init(GLSV_GLD_CB *cb);
extern SaAisErrorT create_runtime_object(char *rname, SaTimeT create_time, SaImmOiHandleT immOiHandle);

extern SaAisErrorT gld_saImmOiRtAttrUpdateCallback(SaImmOiHandleT immOiHandle,
					    const SaNameT *objectName, const SaImmAttrNameT *attributeNames);

#endif  // LCK_LCKD_GLD_IMM_H_
