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
 * Author(s): Ericsson AB
 *
 */

#ifndef IMM_IMMPBED_IMMPBE_H_
#define IMM_IMMPBED_IMMPBE_H_

#include "imm/immpbe_dump.h"

void pbeDaemon(SaImmHandleT immHandle, void* dbHandle, 
	SaImmAdminOwnerHandleT ownerHandle, ClassMap* classIdMap,
	unsigned int objCount, bool pbe2, bool pbe2B);

#endif  // IMM_IMMPBED_IMMPBE_H_
