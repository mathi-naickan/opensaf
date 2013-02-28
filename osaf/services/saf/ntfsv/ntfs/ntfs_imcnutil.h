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

#ifndef NTFS_IMCNUTIL_H
#define	NTFS_IMCNUTIL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef HAVE_NTFIMCN
#define NTFIMCN_PROC_NAME "osafntfimcnd"

void init_ntfimcn_(SaAmfHAStateT ha_state);
void restart_ntfimcn_(SaAmfHAStateT ha_state);
int stop_ntfimcn_(void);
#define init_ntfimcn(a) init_ntfimcn_(a)
#define restart_ntfimcn(a) restart_ntfimcn_(a)
#define stop_ntfimcn() stop_ntfimcn_()

#else /* HAVE_NTFIMCN */
#define init_ntfimcn(a)
#define restart_ntfimcn(a)
#define stop_ntfimcn() 0
#endif /* HAVE_NTFIMCN */

#ifdef	__cplusplus
}
#endif

#endif	/* NTFS_IMCNUTIL_H */

