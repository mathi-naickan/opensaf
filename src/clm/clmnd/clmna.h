/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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
 * Author(s):  GoAhead Software
 *
 */

#ifndef CLM_CLMND_CLMNA_H_
#define CLM_CLMND_CLMNA_H_

#include "clm/clmnd/cb.h"
extern CLMNA_CB *clmna_cb;

SaAisErrorT clmna_amf_init(CLMNA_CB * cb);
void clmna_process_mbx(SYSF_MBX *mbx);

#endif  // CLM_CLMND_CLMNA_H_
