/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2016 The OpenSAF Foundation
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

#ifndef LOG_LOGD_LGS_RECOV_H_
#define LOG_LOGD_LGS_RECOV_H_

#include "log/logd/lgs.h"

int log_rtobj_list_add(const std::string &dn_str);
int log_rtobj_list_no();
int log_rtobj_list_find(const std::string &stream_name);
int log_rtobj_list_getnamepos();
char *log_rtobj_list_getname(int pos);
void log_rtobj_list_erase_one_pos(int pos);
void log_rtobj_list_free();
int lgs_restore_one_app_stream(
    const std::string &stream_name,
    uint32_t client_id,
    log_stream_t **o_stream
                               );
int log_stream_open_file_restore(log_stream_t *log_stream);
int log_close_rtstream_files(const std::string &stream_name);

#endif  // LOG_LOGD_LGS_RECOV_H_
