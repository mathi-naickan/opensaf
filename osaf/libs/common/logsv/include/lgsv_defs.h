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

#ifndef LGSV_DEFS_H
#define LGSV_DEFS_H

#define LOG_RELEASE_CODE 'A'
#define LOG_MAJOR_VERSION 2
#define LOG_MINOR_VERSION 1

// Waiting time in library for sync send, unit 10ms
#define LGS_WAIT_TIME 1000

// The length of '_yyyymmdd_hhmmss_yyyymmdd_hhmmss.log' including null-termination char
#define LOG_TAIL_MAX 37

/**
 * The log file name format: <filename>_<createtime>_<closetime>.log
 * '_<createtime>_<closetime>.log' part is appended by logsv and has above format,
 * and '<filename>' part can set by user. Limit the length of <filename> from user,
 * to make sure the log file name does not exceed the maximum length (NAME_MAX).
 *
 * This macro defines the maximum length for <filename> part.
 */
#define LOG_NAME_MAX (NAME_MAX - LOG_TAIL_MAX)

#endif   /* LGSV_DEFS_H */
