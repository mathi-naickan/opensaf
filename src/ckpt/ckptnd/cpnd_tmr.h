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

#ifndef CKPT_CKPTND_CPND_TMR_H_
#define CKPT_CKPTND_CPND_TMR_H_

#include "base/ncssysf_tmr.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t cpnd_tmr_start(CPND_TMR *tmr, SaTimeT duration);

#ifdef __cplusplus
}
#endif

#endif  // CKPT_CKPTND_CPND_TMR_H_
