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

#ifndef __LGS_UTIL_H
#define __LGS_UTIL_H

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */
#include <string.h>

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

extern char *lgs_get_time(void);
extern int lgs_create_config_file(log_stream_t *stream);
extern void lgs_evt_destroy(lgsv_lgs_evt_t *evt);
extern SaTimeT lgs_get_SaTime(void);
extern int lgs_file_rename(const char *path, const char *old_name, const char *time_stamp, const char *suffix);
extern uint32_t lgs_create_known_streams(lgs_cb_t *lgs_cb);
extern void lgs_exit(const char *msg, SaAmfRecommendedRecoveryT rec_rcvr);
extern bool lgs_relative_path_check(const char* path);
extern int lgs_make_dir(const char* root, const char* path);

#endif   /* ifndef __LGS_UTIL_H */
