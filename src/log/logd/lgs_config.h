/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2015 The OpenSAF Foundation
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

#ifndef LOG_LOGD_LGS_CONFIG_H_
#define LOG_LOGD_LGS_CONFIG_H_

#include <stdbool.h>
#include <stdint.h>
#include <string>

#include "imm/saf/saImmOi.h"
#include "amf/saf/saAmf.h"

#define LGS_CFG_RUNTIME_OBJECT "logConfig=currentConfig,safApp=safLogService"

#define LOG_ROOT_DIRECTORY "logRootDirectory"
#define LOG_DATA_GROUPNAME "logDataGroupname"
#define LOG_MAX_LOGRECSIZE "logMaxLogrecsize"
#define LOG_STREAM_FILE_FORMAT "logStreamFileFormat"
#define LOG_STREAM_SYSTEM_HIGH_LIMIT "logStreamSystemHighLimit"
#define LOG_STREAM_SYSTEM_LOW_LIMIT "logStreamSystemLowLimit"
#define LOG_STREAM_APP_HIGH_LIMIT "logStreamAppHighLimit"
#define LOG_STREAM_APP_LOW_LIMIT "logStreamAppLowLimit"
#define LOG_MAX_APPLICATION_STREAMS "logMaxApplicationStreams"
#define LOG_FILE_IO_TIMEOUT "logFileIoTimeout"
#define LOG_FILE_SYS_CONFIG "logFileSysConfig"

typedef enum {
  LGS_IMM_LOG_ROOT_DIRECTORY,
  LGS_IMM_DATA_GROUPNAME,
  LGS_IMM_LOG_MAX_LOGRECSIZE,
  LGS_IMM_LOG_STREAM_FILE_FORMAT,
  LGS_IMM_LOG_STREAM_SYSTEM_HIGH_LIMIT,
  LGS_IMM_LOG_STREAM_SYSTEM_LOW_LIMIT,
  LGS_IMM_LOG_STREAM_APP_HIGH_LIMIT,
  LGS_IMM_LOG_STREAM_APP_LOW_LIMIT,
  LGS_IMM_LOG_MAX_APPLICATION_STREAMS,
  LGS_IMM_FILEHDL_TIMEOUT,
  LGS_IMM_LOG_FILESYS_CFG,

  LGS_IMM_LOG_NUMBER_OF_PARAMS,
  LGS_IMM_LOG_OPENSAFLOGCONFIG_CLASS_EXIST,

  LGS_IMM_LOG_NUMEND
} lgs_logconfGet_t;

/**
 * Structure for log server configuration changing data
 */
typedef struct config_chkpt {
  char *ckpt_buffer_ptr;          /* Buffer for config data */
  uint64_t ckpt_buffer_size;      /* Total size of the buffer */
}lgs_config_chg_t;

/*
 * For function information see code file lgs_config.c
 */
void lgs_cfg_init(SaImmOiHandleT immOiHandle, SaAmfHAStateT ha_state);
const void *lgs_cfg_get(lgs_logconfGet_t param);
bool lgs_path_is_writeable_dir_h(const std::string &pathname);
void lgs_cfgupd_list_create(const char *name_str, char *value_str,
                            lgs_config_chg_t *config_data);
char *lgs_cfgupd_list_read(char **name_str, char **value_str,
                           char *next_param_ptr, lgs_config_chg_t *cfgupd_ptr);
int lgs_cfg_update(const lgs_config_chg_t *config_data);

/*
 * Functions for updating some parameters. Used to support check-point before
 * version 5
 */
void lgs_rootpathconf_set(const std::string &root_path_str);
void lgs_groupnameconf_set(const char *data_groupname_str);

/*
 * Parameter value validation functions. Validates parameters. See function
 * headers for validation rules
 */
int lgs_cfg_verify_root_dir(const std::string &root_str_in);
int lgs_cfg_verify_log_data_groupname(char *group_name);
int lgs_cfg_verify_log_file_format(const char* log_file_format);
int lgs_cfg_verify_max_logrecsize(uint32_t max_logrecsize_in);
int lgs_cfg_verify_mbox_limit(uint32_t high, uint32_t low);
int lgs_cfg_verify_max_application_streams(uint32_t max_app_streams);
int lgs_cfg_verify_file_io_timeout(uint32_t log_file_io_timeout);

/*
 * Handle runtime object for showing actual configuration
 */
void conf_runtime_obj_create(SaImmOiHandleT immOiHandle);
void conf_runtime_obj_hdl(SaImmOiHandleT immOiHandle, const SaImmAttrNameT *attributeNames);

/*
 *  Trace functions
 */
void lgs_trace_config();
void lgs_cfg_read_trace();

#endif  // LOG_LOGD_LGS_CONFIG_H_

