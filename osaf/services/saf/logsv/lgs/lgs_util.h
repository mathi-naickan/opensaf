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
#include <saAis.h>
#include <saAmf.h>

#include "lgs_stream.h"
#include "lgs_evt.h"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */
#define LOG_VER_EXP "LOG_SVC_VERSION:"
#define FMAT_EXP "FORMAT:"
#define CFG_EXP_MAX_FILE_SIZE "MAX_FILE_SIZE:"
#define CFG_EXP_FIXED_LOG_REC_SIZE "FIXED_LOG_REC_SIZE:"
#define CFG_EXP_LOG_FULL_ACTION "LOG_FULL_ACTION:"

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

extern char *lgs_get_time(time_t *time_in);
extern int lgs_create_config_file_h(log_stream_t *stream);
extern void lgs_evt_destroy(lgsv_lgs_evt_t *evt);
extern SaTimeT lgs_get_SaTime(void);
extern int lgs_file_rename_h(const char *path, const char *old_name,
		const char *time_stamp, const char *suffix, char *new_name_out);
//extern uint32_t lgs_create_known_streams(lgs_cb_t *lgs_cb); /* Not used, no code */
extern void lgs_exit(const char *msg, SaAmfRecommendedRecoveryT rec_rcvr);
extern bool lgs_relative_path_check_ts(const char* path);
extern int lgs_make_reldir_h(const char* path);
extern int lgs_check_path_exists_h(const char *path_to_check);
extern int lgs_get_data_gid();
extern int lgs_own_log_files(log_stream_t *stream);
extern bool lgs_has_special_char(const char *str);
#endif   /* ifndef __LGS_UTIL_H */
