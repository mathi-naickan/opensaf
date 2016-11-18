/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2014 The OpenSAF Foundation
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
 */

/*
 * DESCRIPTION:
 *   This file defines extensions to the C header files provided by the Service
 *   Availability(TM) Forum AIS specifications. It contains extended prototypes
 *   and type definitions. The intention is to get these additions Included as a
 *   part of SAF standard specifications.
 */

#ifndef _SA_AIS_B_5_14_H
#define _SA_AIS_B_5_14_H

#ifdef  __cplusplus
extern "C" {
#endif

typedef const char* SaConstStringT;

#ifdef SA_EXTENDED_NAME_SOURCE
#define SA_MAX_UNEXTENDED_NAME_LENGTH 256

typedef struct {
    SaUint16T _opaque[129];
} SaNameT;

extern void
saAisNameLend(
    SaConstStringT value,
    SaNameT* name);

extern SaConstStringT
saAisNameBorrow(
    const SaNameT* name);
#endif /* SA_EXTENDED_NAME_SOURCE */

#ifdef  __cplusplus
}
#endif

#endif   /* _SA_AIS_B_5_14_H */
