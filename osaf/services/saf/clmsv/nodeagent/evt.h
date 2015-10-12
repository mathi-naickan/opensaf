/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010,2015 The OpenSAF Foundation
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
 *             Ericsson AB
 *
 */

#ifndef EVT_H
#define EVT_H

#include <stdbool.h>

typedef NCS_IPC_MSG CLMNA_MBX_MSG;
typedef enum clmna_evt_type {
	CLMNA_EVT_INVALID = 0,
	CLMNA_EVT_DUMMY_MSG,
	CLMNA_EVT_MAX_MSG
} CLMNA_EVT_TYPE;

typedef struct clmna_evt_tags {
	CLMNA_MBX_MSG next;
	CLMNA_EVT_TYPE type;
	bool caused_by_timer_expiry;
} CLMNA_EVT;


#endif
