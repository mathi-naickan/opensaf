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
 * Author(s): Ericsson
 *
 */

#ifndef SAFLOG_H
#define SAFLOG_H

#include <saAis.h>
#include <saLog.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Log to SAF LOG system stream with priority and usr name
 * @param priority
 * @param logSvcUsrName
 * @param format
 */
extern void saflog(int priority, const SaNameT *logSvcUsrName,
	const char *format, ...) __attribute__ ((format(printf, 3, 4)));

extern void saflog_init(void);

#ifdef __cplusplus
}
#endif

#endif
