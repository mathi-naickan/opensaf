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
 * licensing terms.z
 *
 * Author(s): GoAhead Software
 *
 */
#ifndef DTM_SOCKET_H
#define DTM_SOCKET_H

#define SOCKET_ERROR()  -1
#define SDDR_IN_USE (errno == EADDRINUSE)
#define IS_BLOCKIN_ERROR(a) ((a == EALREADY) || (a == EINPROGRESS) || (a == EAGAIN))
#define IS_CONNECTED(a) 0
#define SOCKET_RESET(a) ((a == EPIPE) || (a == ECONNRESET))
#define SR_SOCKET_NON_BLOCKING(a) \
 fcntl(m_sockfd,F_SETFL,fcntl(a,F_GETFD) | O_NONBLOCK)

#endif
