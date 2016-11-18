/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2011 The OpenSAF Foundation
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
/**
 *   Client header file.
 */

#ifndef NTF_TOOLS_NTFCONSUMER_H_
#define NTF_TOOLS_NTFCONSUMER_H_

#include "ntf/saf/saNtf.h"

/* common for ntfsubscribe and ntfread */
#define EXIT_IF_FALSE(expr) ntfsvtools_exitIfFalse(__FILE__, __LINE__, expr)

int verbose;
void ntfsv_tools_exitIfFalse(
    const char *file,
    unsigned int line, int expression);
char *error_output(SaAisErrorT result);
void saNtfNotificationCallback(
    SaNtfSubscriptionIdT subscriptionId,
    const SaNtfNotificationsT *notification);

#endif  // NTF_TOOLS_NTFCONSUMER_H_
