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

#ifndef IMM_AGENT_IMMA_DEF_H_
#define IMM_AGENT_IMMA_DEF_H_
#include "base/saf_def.h" //to get NCS_SAF_MIN_ACCEPT_TIME

/* Macros for Validating Version */
#define IMMA_RELEASE_CODE 'A'
#define IMMA_MAJOR_VERSION 0x02
#define IMMA_MINOR_VERSION 0x11

#define IMMSV_WAIT_TIME  1000 /* Default MDS wait time in 10ms units =>10 sec*/




#endif  // IMM_AGENT_IMMA_DEF_H_
