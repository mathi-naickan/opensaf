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
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************
..............................................................................
  DESCRIPTION:  MDS LOG header

  ******************************************************************************
  */

#ifndef MDS_MDS_LOG_H_
#define MDS_MDS_LOG_H_

#include <stdint.h>
#include "base/ncsgl_defs.h"
#include "base/ncs_lib.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* Logging utility functions  - Phani */

extern int gl_mds_log_level;

enum {
  NCSMDS_LC_CRITICAL = 1,
  NCSMDS_LC_ERR = 2,
  NCSMDS_LC_NOTIFY = 3,
  NCSMDS_LC_INFO = 4,
  NCSMDS_LC_DBG = 5
};

#define m_MDS_LOG_CRITICAL log_mds_critical
#define m_MDS_LOG_ERR log_mds_err
#define m_MDS_LOG_NOTIFY log_mds_notify
#define m_MDS_LOG_INFO log_mds_info
#define m_MDS_LOG_DBG log_mds_dbg

#define m_MDS_ENTER() m_MDS_LOG_DBG(">> %s", __FUNCTION__)
#define m_MDS_LEAVE() m_MDS_LOG_DBG("<< %s", __FUNCTION__)

uint32_t mds_log_init(const char *log_file_name);
void log_mds_dbg(const char *fmt, ...)
    __attribute__ ((format(printf, 1, 2)));
void log_mds_info(const char *fmt, ...)
    __attribute__ ((format(printf, 1, 2)));
void log_mds_notify(const char *fmt, ...)
    __attribute__ ((format(printf, 1, 2)));
void log_mds_err(const char *fmt, ...)
    __attribute__ ((format(printf, 1, 2)));
void log_mds_critical(const char *fmt, ...)
    __attribute__ ((format(printf, 1, 2)));

#ifdef  __cplusplus
}
#endif

#endif  // MDS_MDS_LOG_H_
