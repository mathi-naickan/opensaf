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

#ifndef DAEMON_H
#define DAEMON_H

#ifdef  __cplusplus
extern "C" {
#endif

extern void daemonize(int argc, char *argv[]);


#ifdef __cplusplus
}
#endif

#endif
