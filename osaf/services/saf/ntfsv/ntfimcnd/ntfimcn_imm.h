/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2013 The OpenSAF Foundation
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

#ifndef NTFIMCN_IMM_H
#define	NTFIMCN_IMM_H

#include "ntfimcn_main.h"

#ifdef	__cplusplus
extern "C" {
#endif

int ntfimcn_imm_init(ntfimcn_cb_t *cb);
void ntfimcn_special_applier_clear(void);

#ifdef	__cplusplus
}
#endif

#endif	/* NTFIMCN_IMM_H */

