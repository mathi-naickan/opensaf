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

#ifndef LOG_LOGD_LGS_FILEHDL_H_
#define LOG_LOGD_LGS_FILEHDL_H_

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <utmp.h>

#include "osaf/saf/saAis.h"
#include "log/logd/lgs_util.h"

/*****************************************************************************
 * Handlers:
 * This is the part of a file system handling function that shall execute in
 * the file handling thread.
 * Defines for in and out parameters for each handler
 *****************************************************************************/

/*
 * fileopen_hdl(..)
 * Inpar char str[]
 * No typedef needed
 * Ret code file descriptor or -1 if error
 * Outpar int errno_save
 * No typedef needed
 */
typedef struct {
  char *filepath; /* Complete path to file */
  char groupname[UT_NAMESIZE];    /* Name of owner group */
} fopen_in_t;

/*
 * fileclose_hdl(..)
 * Inpar int fd
 * No typedef needed
 * No out parameters
 */

/*
 * delete_file_hdl(..)
 * Inpar char str[]
 * No typedef needed
 * No out parameters
 */

/*
 * make_log_dir_hdl(..)
 * Ret code -1 if error
 * Out parameter is a string of max PATH_MAX
 * Parameter data_out_size must be set to PATH_MAX
 */
typedef struct {
  char *root_dir; /* Implementer defined root directory */
  char *rel_path; /* Path to create */
} mld_in_t;

/*
 * get_number_of_log_files_hdl(..)
 */
typedef struct {
  /* File name prefix (name part before time stamps) */
  char *file_name;
  /* logsv_root_dir + pathName makes up the path to the log files
   * Note: The whole path cannot be a longer string than PATH_MAX
   */
  char *logsv_root_dir;
  char *pathName;
} gnolfh_in_t;

/*
 * write_log_record_hdl(..)
 * Outpar int errno_save
 */
typedef struct {
  int fd; /* File descriptor for current log file */
  size_t record_size; /* Size of logrecord including '\0' */
  SaUint32T fixedLogRecordSize;
  const void *lgs_rec;  /* Pointer to allocated log record */
}wlrh_t;

/*
 * create_config_file_hdl(..)
 * No out parameters
 */
typedef struct {
  /* Information to write in the config file */
  SaVersionT version;
  size_t logFileFormat_size; /* Size of logFileFormat incl. '\0' */
  SaUint64T maxLogFileSize;
  SaUint32T fixedLogRecordSize;
  SaUint32T maxFilesRotated;
  /* File information */
  size_t file_path_size; /* Size of file_path incl. '\0' */
} ccfh_t;
/* char logFileFormat[]
 * char file_path[]
 * Strings of varying length that shall be added
 * to the indata buffer directly after the ccfh_t
 */

/* own_log_files_by_group_hdl(..)
 * No out parameters
 */
typedef struct {
  char *dir_path; /* Full path to stream directory */
  /* File name prefix (name part before time stamps) */
  char *file_name;
  char groupname[UT_NAMESIZE];    /* Name of owner group */
} olfbgh_t;

/*
 * rename_file_hdl(..)
 *
 * Inpar:
 * size_t oldpath_str_size
 * char oldpath_str[], string of varying size
 * char newpath_str[], string of varying size
 *
 * No out parameters
 */

/*
 * check_path_exists_hdl(..)
 *
 * Inpar:
 * char path_str[], string containing path to check
 *
 * No out parameters
 */

/*
 * path_is_writeable_dir_hdl(...)
 *
 * Inpar:
 * size_t path str size
 * char path_str[], strin of varying size
 *
 * No out parameters
 */

/*
 * lgs_get_file_params_hdl(...)
 * Ret code -1 if error
 */
/* IN params */
typedef struct {
  /* File name prefix (name part before time stamps) */
  char *file_name;
  /* Full path to directory root path + rel path */
  char *file_path;
} gfp_in_t;

/* OUT params */
typedef struct {
  char *curFileName;
  uint32_t curFileSize;   /* Bytes written to current log file */
  uint32_t logRecordId;   /* log record identifier increased for each record */
} gfp_out_t;


int path_is_writeable_dir_hdl(void *indata, void *outdata, size_t max_outsize);
int check_path_exists_hdl(void *indata, void *outdata, size_t max_outsize);
int rename_file_hdl(void *indata, void *outdata, size_t max_outsize);
int create_config_file_hdl(void *indata, void *outdata, size_t max_outsize);
int write_log_record_hdl(void *indata, void *outdata, size_t max_outsize, bool *timeout_f);
int make_log_dir_hdl(void *indata, void *outdata, size_t max_outsize);
int fileopen_hdl(void *indata, void *outdata, size_t max_outsize, bool *timeout_f);
int fileclose_hdl(void *indata, void *outdata, size_t max_outsize);
int delete_file_hdl(void *indata, void *outdata, size_t max_outsize);
int get_number_of_log_files_hdl(void *indata, void *outdata, size_t max_outsize);
int own_log_files_by_group_hdl(void *indata, void *outdata, size_t max_outsize);
int lgs_get_file_params_hdl(void *indata, void *outdata, size_t max_outsize);

#endif  // LOG_LOGD_LGS_FILEHDL_H_

