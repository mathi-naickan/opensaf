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
 * Author(s): Wind River Systems
 *
 */

/**
 * This file defines APIs for daemon management.
 *
 * TODO
 *
 */

#ifndef BASE_DAEMON_H_
#define BASE_DAEMON_H_

#ifdef  __cplusplus
extern "C" {
#endif

void daemonize(int argc, char *argv[]);
void daemonize_as_user(const char *username, int argc, char *argv[]);
void daemon_exit(void);
void daemon_sigterm_install(int *term_fd);

#ifdef __cplusplus
}
#endif

#endif  // BASE_DAEMON_H_
