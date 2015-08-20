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

#ifndef LGS_CONFIG_H
#define	LGS_CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include "saImmOi.h"
#include "saAmf.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {
	LGS_IMM_LOG_ROOT_DIRECTORY,
	LGS_IMM_DATA_GROUPNAME,
	LGS_IMM_LOG_MAX_LOGRECSIZE,
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
	char *ckpt_buffer_ptr;		/* Buffer for config data */
	uint64_t ckpt_buffer_size;	/* Total size of the buffer */
}lgs_config_chg_t;

/* 
 * For function information see code file lgs_config.c
 */
void lgs_cfg_init(SaImmOiHandleT immOiHandle, SaAmfHAStateT ha_state);
const void *lgs_cfg_get(lgs_logconfGet_t param);
bool lgs_path_is_writeable_dir_h(const char *pathname);
void lgs_cfgupd_list_create(char *name_str, char *value_str,
	lgs_config_chg_t *config_data);
char *lgs_cfgupd_list_read(char **name_str, char **value_str,
	char *next_param_ptr, lgs_config_chg_t *cfgupd_ptr);
int lgs_cfg_update(const lgs_config_chg_t *config_data);

/*
 * Functions for updating some parameters. Used to support check-point before
 * version 5
 */
void lgs_rootpathconf_set(const char *root_path_str);
void lgs_groupnameconf_set(const char *data_groupname_str);

/*
 * Parameter value validation functions. Validates parameters. See function
 * headers for validation rules
 */
int lgs_cfg_verify_root_dir(char *root_str_in);
int lgs_cfg_verify_log_data_groupname(char *group_name);
int lgs_cfg_verify_max_logrecsize(uint32_t max_logrecsize_in);
int lgs_cfg_verify_mbox_limit(uint32_t high, uint32_t low);
int lgs_cfg_verify_max_application_streams(uint32_t max_app_streams);

/*
 *  Trace functions
 */
void lgs_trace_config(void);
void lgs_cfg_read_trace(void);

#ifdef	__cplusplus
}
#endif

#endif	/* LGS_CONFIG_H */
