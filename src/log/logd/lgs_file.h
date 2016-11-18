/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2013 The OpenSAF Foundation
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

#ifndef LOG_LOGD_LGS_FILE_H_
#define LOG_LOGD_LGS_FILE_H_

#include <stdint.h>
#include <stddef.h>

#include "osaf/saf/saAis.h"

typedef enum {
  LGSF_SUCESS,    /* Operation done successfully */
  LGSF_BUSY,              /* A request is ongoing. File system may be unavailable */
  LGSF_TIMEOUT,   /* No answer received within timeout time */
  LGSF_FAIL,              /* Other fail */
  LGSF_NORETC             /* For internal use */
}lgsf_retcode_t;

typedef enum {
  LGSF_FILEOPEN,
  LGSF_FILECLOSE,
  LGSF_DELETE_FILE,
  LGSF_GET_NUM_LOGFILES,
  LGSF_MAKELOGDIR,
  LGSF_WRITELOGREC,
  LGSF_CREATECFGFILE,
  LGSF_RENAME_FILE,
  LGSF_CHECKPATH,
  LGSF_CHECKDIR,
  LGSF_OWN_LOGFILES,
  LGSF_GET_FILE_PAR,
  LGSF_NOREQ
}lgsf_treq_t;

/* Indata structure used with log_file_api() */
typedef struct {
  lgsf_treq_t req_code_in;/* Invokes the correct handler */
  int hdl_ret_code_out;   /* Return code from handler */
  size_t data_in_size;    /* Size of in-data buffer */
  void *data_in;  /* Buffer containing in data for the handler */
  size_t data_out_size;   /* Size of out-data buffer */
  void *data_out; /* Buffer containing out data from the handler */
}lgsf_apipar_t;

char *lgsf_retcode_str(lgsf_retcode_t rc);
uint32_t lgs_file_init();
lgsf_retcode_t log_file_api(lgsf_apipar_t *param_in);
void lgs_fd_list_add(int32_t fd);
int32_t lgs_fd_list_get();

#endif  // LOG_LOGD_LGS_FILE_H_
