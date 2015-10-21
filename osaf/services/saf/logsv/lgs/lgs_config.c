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

#define _GNU_SOURCE
#include "lgs_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <grp.h>
#include <utmp.h>

#include "configmake.h"

#include "saf_error.h"
#include "osaf_secutil.h"
#include "osaf_utility.h"
#include "logtrace.h"
#include "immutil.h"
#include "lgs_file.h"
#include "lgs_fmt.h"

extern struct ImmutilWrapperProfile immutilWrapperProfile;
static SaVersionT immVersion = { 'A', 2, 11 };

/* Mutex for making read and write of configuration data thread safe */
pthread_mutex_t lgs_config_data_mutex = PTHREAD_MUTEX_INITIALIZER;

/***
 * This file contains handling of Log service configuration including:
 *  - Configuration object
 *  - Environment variables
 *  - Default values
 *  - Verification of attribute values
 *  - Check-pointing
 */

/* The name of log service config object */
#define LGS_IMM_LOG_CONFIGURATION	"logConfig=1,safApp=safLogService"

/******************************************************************************
 * Configuration data
 ******************************************************************************/
/**
 * Used with lgs_cfg_get() cnfflag. Contains information about where
 * the configuration comes from.
 */
typedef enum {
	LGS_CNF_OBJ,	/* Config object */
	LGS_CNF_ENV,	/* Environment variable */
	LGS_CNF_DEF     /* Default value */
} lgs_conf_flg_t;

/**
 * The structure holding all log service configuration data
 */
typedef struct {
	/* --- Corresponds to IMM Class SaLogConfig --- */
	char logRootDirectory[PATH_MAX];
	char logDataGroupname[UT_NAMESIZE];
	char logStreamFileFormat[MAX_FIELD_SIZE];
	SaUint32T logMaxLogrecsize;
	SaUint32T logStreamSystemHighLimit;
	SaUint32T logStreamSystemLowLimit;
	SaUint32T logStreamAppHighLimit;
	SaUint32T logStreamAppLowLimit;
	SaUint32T logMaxApplicationStreams;
	SaUint32T logFileIoTimeout;
	SaUint32T logFileSysConfig;
	/* --- end correspond to IMM Class --- */

	/* Used for checkpointing time when files are closed */
	time_t chkp_file_close_time;

	bool OpenSafLogConfig_object_exist;

	lgs_conf_flg_t logRootDirectory_cnfflag;
	lgs_conf_flg_t logMaxLogrecsize_cnfflag;
	lgs_conf_flg_t logStreamSystemHighLimit_cnfflag;
	lgs_conf_flg_t logStreamSystemLowLimit_cnfflag;
	lgs_conf_flg_t logStreamAppHighLimit_cnfflag;
	lgs_conf_flg_t logStreamAppLowLimit_cnfflag;
	lgs_conf_flg_t logMaxApplicationStreams_cnfflag;
	lgs_conf_flg_t logFileIoTimeout_cnfflag;
	lgs_conf_flg_t logFileSysConfig_cnfflag;
	lgs_conf_flg_t logDataGroupname_cnfflag;
	lgs_conf_flg_t logStreamFileFormat_cnfflag;
} lgs_conf_t;

/**
 * LOG configuration
 * For more info about attributes see Log Programmers reference
 * See also IMM class OpenSafLogConfig
 * Distinguished name of config object is 'logConfig=1,safApp=safLogService'
 */

/* The default values */
static struct {
	const char *logRootDirectory;
	const char *logDataGroupname;
	const char *logStreamFileFormat;
	SaUint32T logStreamSystemHighLimit;
	SaUint32T logStreamSystemLowLimit;
	SaUint32T logStreamAppHighLimit;
	SaUint32T logStreamAppLowLimit;
	SaUint32T logMaxLogrecsize;
	SaUint32T logMaxApplicationStreams;
	SaUint32T logFileIoTimeout;
	SaUint32T logFileSysConfig;
} lgs_conf_def = {
	.logRootDirectory = PKGLOGDIR,
	.logDataGroupname = "",
	.logStreamFileFormat = DEFAULT_APP_SYS_FORMAT_EXP,
	.logStreamSystemHighLimit = 0,
	.logStreamSystemLowLimit = 0,
	.logStreamAppHighLimit = 0,
	.logStreamAppLowLimit = 0,
	.logMaxLogrecsize = 1024,
	.logMaxApplicationStreams = 64,
	.logFileIoTimeout = 500,
	.logFileSysConfig = 1,
};

static lgs_conf_t lgs_conf = {
	/*
	 * For the following flags, LGS_CNF_DEF means that no external
	 * configuration exists and the corresponding attributes hard-coded
	 * default value is used.Is set to false if configuration is found in
	 * IMM object or environment variable.
	 * See function lgs_logconf_get() for more info.
	 */
	.OpenSafLogConfig_object_exist = false,

	.logRootDirectory_cnfflag = LGS_CNF_DEF,
	.logStreamSystemHighLimit_cnfflag = LGS_CNF_DEF,
	.logStreamSystemLowLimit_cnfflag = LGS_CNF_DEF,
	.logStreamAppHighLimit_cnfflag = LGS_CNF_DEF,
	.logStreamAppLowLimit_cnfflag = LGS_CNF_DEF,
	.logDataGroupname_cnfflag = LGS_CNF_DEF,
	/*
	 * The following attributes cannot be configured in the config file
	 * Will be set to false if the attribute exists in the IMM config object
	 */
	.logMaxLogrecsize_cnfflag = LGS_CNF_DEF,
	.logMaxApplicationStreams_cnfflag = LGS_CNF_DEF,
	.logFileIoTimeout_cnfflag = LGS_CNF_DEF,
	.logFileSysConfig_cnfflag = LGS_CNF_DEF,
	.logStreamFileFormat_cnfflag = LGS_CNF_DEF,
};


/******************************************************************************
 * Internal functions
 ******************************************************************************/

static char *cnfflag_str(lgs_conf_flg_t cnfflag);
static int verify_all_init(void);

/******************************************************************************
 * Utility functions
 */

/**
 * Set default values for all parameters
 */
static void init_default(void)
{
	(void) strcpy(lgs_conf.logRootDirectory, lgs_conf_def.logRootDirectory);
	(void) strcpy(lgs_conf.logDataGroupname, lgs_conf_def.logDataGroupname);
	(void) strcpy(lgs_conf.logStreamFileFormat, lgs_conf_def.logStreamFileFormat);
	lgs_conf.logMaxLogrecsize = lgs_conf_def.logMaxLogrecsize;
	lgs_conf.logStreamSystemHighLimit = lgs_conf_def.logStreamSystemHighLimit;
	lgs_conf.logStreamSystemLowLimit = lgs_conf_def.logStreamSystemLowLimit;
	lgs_conf.logStreamAppHighLimit = lgs_conf_def.logStreamAppHighLimit;
	lgs_conf.logStreamAppLowLimit = lgs_conf_def.logStreamAppLowLimit;
	lgs_conf.logMaxApplicationStreams = lgs_conf_def.logMaxApplicationStreams;
	lgs_conf.logFileIoTimeout = lgs_conf_def.logFileIoTimeout;
	lgs_conf.logFileSysConfig = lgs_conf_def.logFileSysConfig;
}

/******************************************************************************
 * Check-pointing handling of configuration
 */

/**
 * Create configuration data buffer
 * Creates a buffer containing all configuration data to be updated.
 * Each time the function is called a configuration parameter is added.
 *
 * The buffer structure can be sent as a check-point message and the updating
 * is done on the Standby. It can also be used in the OI to create an update
 * buffer in the apply callback. Use lgs_cfg_update() for updating the
 * configuration.
 *
 * NOTE1: Parameter name and value is not validated
 * NOTE2: This function allocates memory pointed to by config_data in the
 *        lgs_config_chg_t structure. This memory has to bee freed after usage
 *
 * @param name_str[in]  String containing the parameter name
 * @param value_str[in] Parmeter value as a string
 * @param config_data[out] Filled in config data structure
 *                         NOTE! Must be initiated before first call {NULL, 0}
 *
 */
void lgs_cfgupd_list_create(char *name_str, char *value_str,
	lgs_config_chg_t *config_data)
{
	char *tmp_char_ptr = NULL;
	size_t prev_size = 0;
	char *cfg_param_str = NULL;
	size_t cfg_size = 0;

	TRACE_ENTER2("name_str '%s', value_str \"%s\"", name_str, value_str);

	cfg_size = strlen(name_str) + strlen(value_str) + 2;
	cfg_param_str = malloc(cfg_size);
	if (cfg_param_str == NULL) {
		TRACE("%s: malloc Fail Aborted", __FUNCTION__);
		osaf_abort(0);
	}
	sprintf(cfg_param_str, "%s=%s",name_str, value_str);

	size_t alloc_size = 
		strlen(cfg_param_str) + 1 + config_data->ckpt_buffer_size;

	if (config_data->ckpt_buffer_ptr == NULL) {
		/* Allocate memory for first chkpt data */
		tmp_char_ptr = (char *) malloc(alloc_size);
		if (tmp_char_ptr == NULL) {
			TRACE("%s: malloc Fail Aborted", __FUNCTION__);
			osaf_abort(0);
		}

		config_data->ckpt_buffer_ptr = tmp_char_ptr;
		config_data->ckpt_buffer_size = alloc_size;
		strcpy(tmp_char_ptr, cfg_param_str);
	} else {
		/* Add memory for more data */
		tmp_char_ptr = (char *) realloc(
			config_data->ckpt_buffer_ptr, alloc_size);
		if (tmp_char_ptr == NULL) {
			TRACE("%s: malloc Fail Aborted", __FUNCTION__);
			osaf_abort(0);
		}

		prev_size = config_data->ckpt_buffer_size;
		config_data->ckpt_buffer_ptr = tmp_char_ptr;
		config_data->ckpt_buffer_size = alloc_size;
		/* Add config data directly after the previous data */
		strcpy(tmp_char_ptr + prev_size, cfg_param_str);
	}

	free(cfg_param_str);
	TRACE_LEAVE();
}

/**
 * Read a config update buffer and get parameter name and value.
 * The first time the function is called next_param_ptr shall be
 * a pointer to the buffer containing the config data.
 * See ckpt_buffer_ptr in lgs_config_chg_t.
 * To get the next parameter the next_param_ptr[in] shall be set to the
 * return value from the previous call. NULL is returned when the last parameter
 * is read.
 * The last parameter cfgupd_ptr shall be a pointer to the buffer structure to
 * read
 *
 * NOTE: The string pointed to by cfgupd_ptr->ckpt_buffer_ptr is changed in
 *       this function! See strtok_r()
 *
 * @param name_str[out]
 * @param value_str[out]
 * @param next_param_ptr[in]
 * @param cfgupd_ptr[in]
 * 
 * @return next_param_ptr
 */
char *lgs_cfgupd_list_read(char **name_str, char **value_str,
	char *next_param_ptr, lgs_config_chg_t *cfgupd_ptr)
{
	char *bufend_ptr = NULL;
	char *param_ptr = NULL;
	char *next_ptr = NULL;

	if (next_param_ptr == NULL) {
		TRACE("%s() called with next_param_ptr == NULL",
			__FUNCTION__);
		goto done;
	}

	param_ptr = next_param_ptr;
	bufend_ptr = cfgupd_ptr->ckpt_buffer_ptr + cfgupd_ptr->ckpt_buffer_size;
	next_ptr = strrchr(param_ptr, '\0') + 1;

	if (next_ptr >= bufend_ptr) {
		next_ptr = NULL;
	}

	/* Get name and value */
	char *saveptr;
	*name_str = strtok_r(param_ptr, "=", &saveptr);
	*value_str = strtok_r(NULL, "=",  &saveptr);

	done:
	return next_ptr;
}

/**
 * Parse a configuration data buffer and update the configuration structure.
 * Used on Standby when receiving configuration check-point data and by OI
 * to update configuration at modify apply.
 *
 * NOTE1: Operates on a static data structure. Is not thread safe
 * NOTE2: Validation is done here. If validation fails we will be out of
 *        sync. A warning is logged to syslog
 * Comment: Check-pointed configuration data is always sent when the
 *          configuration object is updated or when applying cahnges in IO.
 *          This means that the corresponding cnfflag is set to LGS_CNF_OBJ
 *
 * @param config_data[in] Pointer to structure containing configuration
 *        data buffer
 *
 * @return -1 if validation error
 */
int lgs_cfg_update(const lgs_config_chg_t *config_data)
{
	char *bufend_ptr = NULL;
	char *allocmem_ptr = NULL;
	char *param_ptr = NULL;
	char *next_ptr = NULL;
	char *name_str = NULL;
	char *value_str = NULL;
	char *saveptr = NULL;
	int rc = 0;

	TRACE_ENTER();
	/* Validate config_data */
	if ((config_data->ckpt_buffer_ptr == NULL) ||
		(config_data->ckpt_buffer_size == 0)) {
		/* Note: Not considered as an error */
		TRACE("%s: No config data", __FUNCTION__);
		goto done;
	}

	/* Preserve content of config_data. We must use a copy of config_data
	 * since the information is changed by the strok() function. The
	 * original config_data must not be changed.
	 */
	allocmem_ptr = calloc(1,config_data->ckpt_buffer_size);
	param_ptr = allocmem_ptr;
	(void) memcpy(param_ptr, config_data->ckpt_buffer_ptr, config_data->ckpt_buffer_size);

	bufend_ptr = param_ptr + config_data->ckpt_buffer_size;

	/* Lock mutex while config data is written */
	osaf_mutex_lock_ordie(&lgs_config_data_mutex);

	while(1) {
		/* Next parameter */
		next_ptr = strrchr(param_ptr, '\0') + 1;

		/* Get name and value */
		name_str = strtok_r(param_ptr, "=", &saveptr);
		value_str = strtok_r(NULL, "=", &saveptr);
		if (value_str == NULL) {
			TRACE("%s: value_str is NULL", __FUNCTION__);
			value_str = "";
		}

		/* Update config data */
		if (strcmp(name_str, LOG_ROOT_DIRECTORY) == 0) {
			(void) snprintf(lgs_conf.logRootDirectory, PATH_MAX,
				"%s", value_str);
			lgs_conf.logRootDirectory_cnfflag = LGS_CNF_OBJ;
		} else if (strcmp(name_str, LOG_DATA_GROUPNAME) == 0) {
			(void) snprintf(lgs_conf.logDataGroupname, UT_NAMESIZE,
				"%s", value_str);
			lgs_conf.logDataGroupname_cnfflag = LGS_CNF_OBJ;
		} else if (strcmp(name_str, LOG_STREAM_FILE_FORMAT) == 0) {
			(void) snprintf(lgs_conf.logStreamFileFormat, MAX_FIELD_SIZE,
							"%s", value_str);
			lgs_conf.logStreamFileFormat_cnfflag = LGS_CNF_OBJ;
		} else if (strcmp(name_str, LOG_MAX_LOGRECSIZE) == 0) {
			lgs_conf.logMaxLogrecsize = (SaUint32T)
				strtoul(value_str, NULL, 0);
		} else if (strcmp(name_str, LOG_STREAM_SYSTEM_HIGH_LIMIT) == 0) {
			lgs_conf.logStreamSystemHighLimit = (SaUint32T)
				strtoul(value_str, NULL, 0);
		} else if (strcmp(name_str, LOG_STREAM_SYSTEM_LOW_LIMIT) == 0) {
			lgs_conf.logStreamSystemLowLimit = (SaUint32T)
				strtoul(value_str, NULL, 0);
		} else if (strcmp(name_str, LOG_STREAM_APP_HIGH_LIMIT) == 0) {
			lgs_conf.logStreamAppHighLimit = (SaUint32T)
				strtoul(value_str, NULL, 0);
		} else if (strcmp(name_str, LOG_STREAM_APP_LOW_LIMIT) == 0) {
			lgs_conf.logStreamAppLowLimit = (SaUint32T)
				strtoul(value_str, NULL, 0);
		} else if (strcmp(name_str, LOG_MAX_APPLICATION_STREAMS) == 0) {
			lgs_conf.logMaxApplicationStreams = (SaUint32T)
				strtoul(value_str, NULL, 0);
		} else if (strcmp(name_str, LOG_FILE_IO_TIMEOUT) == 0) {
			lgs_conf.logFileIoTimeout = (SaUint32T)
				strtoul(value_str, NULL, 0);
		} else if (strcmp(name_str, LOG_FILE_SYS_CONFIG) == 0) {
			lgs_conf.logFileSysConfig = (SaUint32T)
				strtoul(value_str, NULL, 0);
		}

		param_ptr = next_ptr;
		if (next_ptr >= bufend_ptr)
			break;
	}

	/* Config data is written. Mutex can be unlocked */
	osaf_mutex_unlock_ordie(&lgs_config_data_mutex);

	lgs_trace_config();

	/* Validate the configuration structure data
	 * For the moment Log a warning and do nothing.
	 * NOTE: Configuration that's failed is replaced by default values
	 */
	if (verify_all_init() == -1) {
		LOG_WA("%s: Verify fail for lgs configuration",
			__FUNCTION__);
		rc = -1;
	}

	if (allocmem_ptr != NULL)
		free(allocmem_ptr);

	done:
	return rc;
	TRACE_LEAVE();
}

/******************************************************************************
 * Verifying configuration attributes
 */

/**
 * Verify logRootDirectory; path to be used as log root directory
 * Rules:
 * - Length of root path <= PATH_MAX
 * - Root path must be a direcory where log can create files and write
 * 
 * @param root_str_in[in] Root path to verify
 * @return -1 on error
 */
int lgs_cfg_verify_root_dir(char *root_str_in)
{
	int rc = 0;
	size_t n_max = PATH_MAX+1;
	size_t n = strnlen(root_str_in, n_max);
	if (n > PATH_MAX) {
		LOG_NO("verify_root_dir Fail. Path > PATH_MAX");
		rc = -1;
		goto done;
	}

	if (lgs_path_is_writeable_dir_h(root_str_in) == false) {
		LOG_NO("path_is_writeable_dir... Fail");
		rc = -1;
		goto done;
	}

	done:
	return rc;
}

/**
 * Verify logDataGroupname
 * Rules:
 * - Empty group name is OK
 * - If group exist it must contain the user as which LOGD is running
 *
 * @param group_name[in]
 * @return -1 on error
 */
int lgs_cfg_verify_log_data_groupname(char *group_name)
{
	int rc = 0;

	if (group_name[0] == '\0') {
		TRACE("%s: Empty group name", __FUNCTION__);
	} else if (strlen(group_name) + 1 > UT_NAMESIZE) {
		LOG_WA("%s: data group > UT_NAMESIZE! Ignore.", __FUNCTION__);
		rc = -1;
	} else {
		uid_t uid = getuid();
		if (osaf_user_is_member_of_group(uid, group_name) == false) {
			LOG_WA("%s: osaf_user_is_member_of_group() Fail",
				__FUNCTION__);
			rc = -1;
		}
	}

	return rc;
}

/**
 * Verify logStreamFileFormat attribute value of App stream.
 * Rule:
 *   All tokens are in right format
 *
 * @param log_file_format [in]
 * @return -1 on error
 */
int lgs_cfg_verify_log_file_format(const char* log_file_format)
{
	int rc = 0;
	SaBoolT dummy;

	if (!lgs_is_valid_format_expression((const SaStringT)log_file_format,
										STREAM_TYPE_APPLICATION,
										&dummy)) {
		LOG_NO("logStreamFileFormat has invalid value = %s",
			   log_file_format);
		rc = -1;
	}

	return rc;
}

/**
 * Verify logMaxLogrecsize; Max size of a log record including header
 * Rules:
 * - Must be >= 150
 * - Must not larger than 65535 (UINT16_MAX)
 *
 * @param max_logrecsize_in[in]
 * @return -1 on error
 */
int lgs_cfg_verify_max_logrecsize(uint32_t max_logrecsize_in)
{
	int rc = 0;
	if ((max_logrecsize_in > SA_LOG_MAX_RECORD_SIZE) ||
		(max_logrecsize_in < SA_LOG_MIN_RECORD_SIZE)) {
		LOG_NO("verify_max_logrecsize Fail");
		rc = -1;
	}

	return rc;
}

/**
 * Verify mailbox limits
 * Rules:
 * Check that low limit < corresponding high limit
 * Both high and low limit can be 0
 *
 * @param high[in] High limit
 * @param low[in]  Low limit
 * @return -1 on error
 */
int lgs_cfg_verify_mbox_limit(uint32_t high, uint32_t low)
{
	int rc = 0;
	
	if ((low == 0) && (high == 0)) {
		TRACE("Both high and low is 0 Ok");
	} else if (high < low) {
		LOG_NO("high < low Fail");
		rc = -1;
	}

	return rc;
}

/**
 * Verify logMaxApplicationStreams
 * Rules:
 * Must have a value > 0
 *
 * @param max_app_streams[in]
 * @return -1 on error
 */
int lgs_cfg_verify_max_application_streams(uint32_t max_app_streams)
{
	int rc = 0;

	if (max_app_streams == 0) {
		LOG_NO("logMaxApplicationStreams == 0 Fail");
		rc = -1;
	}

	return rc;
}

/**
 * Verify logFileIoTimeout
 * Rules: timeout must not be larger than 15s as it will impact on amf healthcheck.
 *        - Maximum value is less than 5s
 *        - Minimum timeout is not less than 500ms
 * NOTE: This range has not been measured in real system yet.
 * @param log_file_io_timeout[in]
 * @return -1 on error
 */
int lgs_cfg_verify_file_io_timeout(uint32_t log_file_io_timeout)
{
	int rc = 0;

	if ((log_file_io_timeout < 500) || (log_file_io_timeout > 5000)) {
		LOG_NO("logFileIoTimeout has invalid value = %u",
			   log_file_io_timeout);
		rc = -1;
	}

	return rc;
}

/**
 * Verify logFileSysConfig
 * Rules:
 * - Must be 1 (use shared file sys) or 2 (write on both SC-nodes)
 * 
 * @param log_filesys_config[in]
 * @return -1 on error
 */
static int lgs_cfg_verify_log_filesys_config(uint32_t log_filesys_config)
{
	int rc = 0;

	if ((log_filesys_config != 1) && (log_filesys_config != 2)) {
		LOG_WA("logFileSysConfig has invalid value = %u",
			log_filesys_config);
		rc = -1;
	}

	return rc;
}


/**
 * Verify logRootDirectory; path to be used as log root directory
 * Rules:
 * - Length of root path <= PATH_MAX
 *
 * NOTE: It is not verified during init if root path is a writable directory
 *       It is no meaning to check since there is no fall back option
 *       If the problem exist it can be fixed by changing root path in IMM
 *       configuration object to a valid path.
 *
 * @param root_str_in[in] Root path to verify
 * @return -1 on error
 */
static int verify_root_dir_init(char *root_str_in)
{
	int rc = 0;
	size_t n_max = PATH_MAX+1;
	size_t n = strnlen(root_str_in, n_max);
	if (n > PATH_MAX) {
		LOG_NO("verify_root_dir Fail. Path > PATH_MAX");
		rc = -1;
		goto done;
	}

	done:
	return rc;
}

/**
 * Verify all parameters in the configuration structure. If a validation fail
 * value in parameter(s) is replaced with hard coded default and
 * cnfflag is set to LGS_CNF_DEF (config from default)
 * 
 * @return -1 on error
 */
static int verify_all_init(void)
{
	int rc = 0;

	TRACE_ENTER();

	if (verify_root_dir_init(lgs_conf.logRootDirectory) == -1) {
		strcpy(lgs_conf.logRootDirectory, lgs_conf_def.logRootDirectory);
		lgs_conf.logRootDirectory_cnfflag = LGS_CNF_DEF;
		rc = -1;
	}

	if (lgs_cfg_verify_log_data_groupname(lgs_conf.logDataGroupname) == -1) {
		strcpy(lgs_conf.logDataGroupname, lgs_conf_def.logDataGroupname);
		lgs_conf.logDataGroupname_cnfflag = LGS_CNF_DEF;
		rc = -1;
	}

	if (lgs_cfg_verify_log_file_format(lgs_conf.logStreamFileFormat) == -1) {
		strcpy(lgs_conf.logStreamFileFormat, lgs_conf_def.logStreamFileFormat);
		lgs_conf.logStreamFileFormat_cnfflag = LGS_CNF_DEF;
		rc = -1;
	}

	if (lgs_cfg_verify_max_logrecsize(lgs_conf.logMaxLogrecsize) == -1) {
		lgs_conf.logMaxLogrecsize = lgs_conf_def.logMaxLogrecsize;
		lgs_conf.logMaxLogrecsize_cnfflag = LGS_CNF_DEF;
		rc = -1;
	}

	/* System limits */
	if (lgs_cfg_verify_mbox_limit(lgs_conf.logStreamSystemHighLimit,
		lgs_conf.logStreamSystemLowLimit) == -1) {
		/* High and low limit are verified against each other
		 * Both limits will be set to default on error
		 */
		lgs_conf.logStreamSystemHighLimit = lgs_conf_def.logStreamSystemHighLimit;
		lgs_conf.logStreamSystemHighLimit_cnfflag = LGS_CNF_DEF;
		lgs_conf.logStreamSystemLowLimit = lgs_conf_def.logStreamSystemLowLimit;
		lgs_conf.logStreamSystemLowLimit_cnfflag = LGS_CNF_DEF;
		rc = -1;
	}

	/* App limits */
	if (lgs_cfg_verify_mbox_limit(lgs_conf.logStreamAppHighLimit,
		lgs_conf.logStreamAppLowLimit) == -1) {
		/* High and low limit are verified against each other
		 * Both limits will be set to default on error
		 */
		lgs_conf.logStreamAppHighLimit = lgs_conf_def.logStreamAppHighLimit;
		lgs_conf.logStreamAppHighLimit_cnfflag = LGS_CNF_DEF;
		lgs_conf.logStreamAppLowLimit = lgs_conf_def.logStreamAppLowLimit;
		lgs_conf.logStreamAppLowLimit_cnfflag = LGS_CNF_DEF;
		rc = -1;
	}

	if (lgs_cfg_verify_max_application_streams(lgs_conf.logMaxApplicationStreams) == -1) {
		lgs_conf.logMaxApplicationStreams = lgs_conf_def.logMaxApplicationStreams;
		lgs_conf.logMaxApplicationStreams_cnfflag = LGS_CNF_DEF;
		rc = -1;
	}

	if (lgs_cfg_verify_file_io_timeout(lgs_conf.logFileIoTimeout) == -1) {
		lgs_conf.logFileIoTimeout = lgs_conf_def.logFileIoTimeout;
		lgs_conf.logFileIoTimeout_cnfflag = LGS_CNF_DEF;
		rc = -1;
	}

	if (lgs_cfg_verify_log_filesys_config(lgs_conf.logFileSysConfig) == -1) {
		lgs_conf.logFileSysConfig = lgs_conf_def.logFileSysConfig;
		lgs_conf.logFileSysConfig_cnfflag = LGS_CNF_DEF;
		rc = -1;
	}
	TRACE_LEAVE();

	return rc;
}

/******************************************************************************
 * Reading configuration
 */

/**
 * Get Log configuration from IMM. See SaLogConfig class.
 * The configuration will be read from the configuration object
 * The cnfflag is set to LGS_CNF_OBJ (configured from IMM object) and lgs_conf
 * is updated if attribute value is found
 * Note: Validation is not done here
 */
static void read_logsv_config_obj_2(void) {
	SaImmHandleT omHandle;
	SaNameT objectName;
	SaImmAccessorHandleT accessorHandle;
	SaImmAttrValuesT_2 *attribute;
	SaImmAttrValuesT_2 **attributes;
	int i = 0;
	int n;

	int asetting = immutilWrapperProfile.errorsAreFatal;
	SaAisErrorT om_rc = SA_AIS_OK;

	TRACE_ENTER();

	/* NOTE: immutil init will osaf_assert if error */
	(void) immutil_saImmOmInitialize(&omHandle, NULL, &immVersion);
	(void) immutil_saImmOmAccessorInitialize(omHandle, &accessorHandle);

	n = snprintf((char *) objectName.value, SA_MAX_NAME_LENGTH, "%s",
			LGS_IMM_LOG_CONFIGURATION);
	if (n >= SA_MAX_NAME_LENGTH) {
		LOG_ER("%s: Fail Object name > SA_MAX_NAME_LENGTH", __FUNCTION__);
		osaf_abort(0); /* Should never happen */
	}
	objectName.length = strlen((char *) objectName.value);

	/* Get all attributes of the object */
	if (immutil_saImmOmAccessorGet_2(accessorHandle, &objectName, NULL, &attributes) != SA_AIS_OK) {
		LOG_NO("%s immutil_saImmOmAccessorGet_2 Fail", __FUNCTION__);
		goto done;
	}
	else {
		lgs_conf.OpenSafLogConfig_object_exist = true;
	}

	while ((attribute = attributes[i++]) != NULL) {
		void *value;

		if (attribute->attrValuesNumber == 0)
			continue;

		value = attribute->attrValues[0];

		if (!strcmp(attribute->attrName, LOG_ROOT_DIRECTORY)) {
			n = snprintf(lgs_conf.logRootDirectory, PATH_MAX, "%s",
					*((char **) value));
			if (n >= PATH_MAX) {
				LOG_WA("LOG root dir read from config object is > PATH_MAX");
				lgs_conf.logRootDirectory[0] = '\0';
			} else {
				lgs_conf.logRootDirectory_cnfflag = LGS_CNF_OBJ;
				TRACE("Conf obj; logRootDirectory: %s",
					lgs_conf.logRootDirectory);
			}
		} else if (!strcmp(attribute->attrName, LOG_DATA_GROUPNAME)) {
			n = snprintf(lgs_conf.logDataGroupname, UT_NAMESIZE, "%s",
					*((char **) value));
			if (n >= UT_NAMESIZE) {
				LOG_WA("LOG data group name read from config object is > UT_NAMESIZE");
				lgs_conf.logDataGroupname[0] = '\0';
			} else {
				lgs_conf.logDataGroupname_cnfflag = LGS_CNF_OBJ;
				TRACE("Conf obj; logDataGroupname: %s",
					lgs_conf.logDataGroupname);
			}
		} else if (!strcmp(attribute->attrName, LOG_STREAM_FILE_FORMAT)) {
			n = snprintf(lgs_conf.logStreamFileFormat, MAX_FIELD_SIZE, "%s",
						 *((char **) value));
			if (n >= MAX_FIELD_SIZE) {
				/* The attribute has invalid value - use default value instead */
				LOG_NO("Invalid logStreamFileFormat: %s",
					   lgs_conf.logStreamFileFormat);
				lgs_conf.logStreamFileFormat[0] = '\0';
			} else {
				lgs_conf.logStreamFileFormat_cnfflag = LGS_CNF_OBJ;
				TRACE("Conf obj; %s: %s", attribute->attrName,
					  lgs_conf.logStreamFileFormat);
			}
		} else if (!strcmp(attribute->attrName, LOG_MAX_LOGRECSIZE)) {
			lgs_conf.logMaxLogrecsize = *((SaUint32T *) value);
			lgs_conf.logMaxLogrecsize_cnfflag = LGS_CNF_OBJ;
			TRACE("Conf obj; logMaxLogrecsize: %u", lgs_conf.logMaxLogrecsize);
		} else if (!strcmp(attribute->attrName, LOG_STREAM_SYSTEM_HIGH_LIMIT)) {
			lgs_conf.logStreamSystemHighLimit = *((SaUint32T *) value);
			lgs_conf.logStreamSystemHighLimit_cnfflag = LGS_CNF_OBJ;
			TRACE("Conf obj; logStreamSystemHighLimit: %u",
				lgs_conf.logStreamSystemHighLimit);
		} else if (!strcmp(attribute->attrName, LOG_STREAM_SYSTEM_LOW_LIMIT)) {
			lgs_conf.logStreamSystemLowLimit = *((SaUint32T *) value);
			lgs_conf.logStreamSystemLowLimit_cnfflag = LGS_CNF_OBJ;
			TRACE("Conf obj; logStreamSystemLowLimit: %u",
				lgs_conf.logStreamSystemLowLimit);
		} else if (!strcmp(attribute->attrName, LOG_STREAM_APP_HIGH_LIMIT)) {
			lgs_conf.logStreamAppHighLimit = *((SaUint32T *) value);
			lgs_conf.logStreamAppHighLimit_cnfflag = LGS_CNF_OBJ;
			TRACE("Conf obj; logStreamAppHighLimit: %u",
				lgs_conf.logStreamAppHighLimit);
		} else if (!strcmp(attribute->attrName, LOG_STREAM_APP_LOW_LIMIT)) {
			lgs_conf.logStreamAppLowLimit = *((SaUint32T *) value);
			lgs_conf.logStreamAppLowLimit_cnfflag = LGS_CNF_OBJ;
			TRACE("Conf obj; logStreamAppLowLimit: %u",
				lgs_conf.logStreamAppLowLimit);
		} else if (!strcmp(attribute->attrName, LOG_MAX_APPLICATION_STREAMS)) {
			lgs_conf.logMaxApplicationStreams = *((SaUint32T *) value);
			lgs_conf.logMaxApplicationStreams_cnfflag = LGS_CNF_OBJ;
			TRACE("Conf obj; logMaxApplicationStreams: %u",
				lgs_conf.logMaxApplicationStreams);
		} else if (!strcmp(attribute->attrName, LOG_FILE_IO_TIMEOUT)) {
			lgs_conf.logFileIoTimeout = *((SaUint32T *) value);
			lgs_conf.logFileIoTimeout_cnfflag = LGS_CNF_OBJ;
			TRACE("Conf obj; logFileIoTimeout: %u", lgs_conf.logFileIoTimeout);
		} else if (!strcmp(attribute->attrName, LOG_FILE_SYS_CONFIG)) {
			lgs_conf.logFileSysConfig = *((SaUint32T *) value);
			lgs_conf.logFileSysConfig_cnfflag = LGS_CNF_OBJ;
			TRACE("Conf obj; logFileSysConfig: %u", lgs_conf.logFileSysConfig);
		}
	}

done:
	/* Do not abort if error when finalizing */
	immutilWrapperProfile.errorsAreFatal = 0;	/* Disable immutil abort */
	om_rc = immutil_saImmOmAccessorFinalize(accessorHandle);
	if (om_rc != SA_AIS_OK) {
		LOG_NO("%s immutil_saImmOmAccessorFinalize() Fail %d",__FUNCTION__, om_rc);
	}
	om_rc = immutil_saImmOmFinalize(omHandle);
	if (om_rc != SA_AIS_OK) {
		LOG_NO("%s immutil_saImmOmFinalize() Fail %d",__FUNCTION__, om_rc);
	}
	immutilWrapperProfile.errorsAreFatal = asetting; /* Enable again */

	TRACE_LEAVE();
}

/**
 * Read configuration from environment variables
 * Some configuration can be done using environment variables. In general
 * the rule is that the configuration object has precedence but there are
 * some deviations. The rules are applied here.
 *
 */
static void read_log_config_environ_var_2(void) {
	char *val_str;
	unsigned long int val_uint;
	int n;

	TRACE_ENTER();

	/* logRootDirectory
	 * Rule: Object has precedence
	 */
	if (lgs_conf.logRootDirectory_cnfflag == LGS_CNF_DEF) {
		/* Has not been set when reading config object */
		if ((val_str = getenv("LOGSV_ROOT_DIRECTORY")) != NULL) {
			n = snprintf(lgs_conf.logRootDirectory, PATH_MAX, "%s", val_str);
			if (n >= PATH_MAX) {
				/* Fail */
				LOG_WA("LOG root dir read from config file is > PATH_MAX");
				lgs_conf.logRootDirectory[0] = '\0';
			} else {
				lgs_conf.logRootDirectory_cnfflag = LGS_CNF_ENV;
			}
		} else {
			LOG_WA("No configuration data for logRootDirectory found");
		}
	}
	TRACE("logRootDirectory \"%s\", cnfflag '%s'",
		lgs_conf.logRootDirectory, cnfflag_str(lgs_conf.logRootDirectory_cnfflag));

	/* logDataGroupname
	 * Rule: Object has precedence
	 */
	if (lgs_conf.logDataGroupname_cnfflag == LGS_CNF_DEF) {
		if ((val_str = getenv("LOGSV_DATA_GROUPNAME")) != NULL) {
			n = snprintf(lgs_conf.logDataGroupname, UT_NAMESIZE, "%s", val_str);
			if (n >= UT_NAMESIZE) {
				/* Fail */
				LOG_WA("LOG data group name read from config file is > UT_NAMESIZE");
				lgs_conf.logDataGroupname[0] = '\0';
			} else {
				lgs_conf.logDataGroupname_cnfflag = LGS_CNF_ENV;
			}
		} else {
			LOG_NO("LOGSV_DATA_GROUPNAME not found");
		}
	}
	TRACE("logDataGroupname \"%s\", cnfflag '%s'",
		lgs_conf.logDataGroupname, cnfflag_str(lgs_conf.logDataGroupname_cnfflag));

	/* logMaxLogrecsize
	 * Rule: Object has precedence
	 */
	if (lgs_conf.logMaxLogrecsize_cnfflag == LGS_CNF_DEF) {
		if ((val_str = getenv("LOGSV_MAX_LOGRECSIZE")) != NULL) {
			/* errno = 0 is necessary as per the manpage of strtoul.
			 * Quoting here:
			 * NOTES:
			 * Since strtoul() can legitimately return 0 or ULONG_MAX
			 * (ULLONG_MAX for strtoull()) on both success and failure, the
			 * calling program should set errno to 0 before the call,
			 * and then determine if an error occurred by  checking  whether
			 * errno has a nonzero value after the call.
			 */
			errno = 0;
			val_uint = strtoul(val_str, NULL, 0);
			if ((errno != 0) || (val_uint > UINT_MAX)) {
				LOG_ER("Illegal value for LOGSV_MAX_LOGRECSIZE - %s, default %u",
						strerror(errno), lgs_conf.logMaxLogrecsize);
			} else {
				lgs_conf.logMaxLogrecsize = (SaUint32T) val_uint;
				lgs_conf.logMaxLogrecsize_cnfflag = LGS_CNF_ENV;
			}
		} else { /* No environment variable use default value */
			LOG_WA("No configuration data for logMaxLogrecsize found");
		}
	}
	TRACE("logMaxLogrecsize=%u, cnfflag '%s'",
		lgs_conf.logMaxLogrecsize, cnfflag_str(lgs_conf.logMaxLogrecsize_cnfflag));

	/* All ...SystemLimit
	 * Rule: Object has precedence
	 */
	/* logStreamSystemHighLimit */
	if (lgs_conf.logStreamSystemHighLimit_cnfflag == LGS_CNF_DEF) {
		if ((val_str = getenv("LOG_STREAM_SYSTEM_HIGH_LIMIT")) != NULL) {
			errno = 0;
			val_uint = strtoul(val_str, NULL, 0);
			if ((errno != 0) || (val_uint > UINT_MAX)) {
				lgs_conf.logStreamSystemHighLimit_cnfflag = LGS_CNF_DEF;
				LOG_WA("Illegal value for LOG_STREAM_SYSTEM_HIGH_LIMIT - %s, default %u",
						strerror(errno), lgs_conf.logStreamSystemHighLimit);
			} else {
				lgs_conf.logStreamSystemHighLimit = (SaUint32T) val_uint;
				lgs_conf.logStreamSystemHighLimit_cnfflag = LGS_CNF_ENV;
			}
		} else if (lgs_conf.logStreamSystemHighLimit_cnfflag == LGS_CNF_DEF) {
			/* No config data found use default value */
			LOG_WA("No configuration data for logStreamSystemHighLimit found");
		}
	}
	TRACE("logStreamSystemHighLimit=%u, cnfflag '%s'",
			lgs_conf.logStreamSystemHighLimit,
		cnfflag_str(lgs_conf.logStreamSystemHighLimit_cnfflag));

	/* logStreamSystemLowLimit */
	if (lgs_conf.logStreamSystemLowLimit_cnfflag == LGS_CNF_DEF) {
		if ((val_str = getenv("LOG_STREAM_SYSTEM_LOW_LIMIT")) != NULL) {
			errno = 0;
			val_uint = strtoul(val_str, NULL, 0);
			if ((errno != 0) || (val_uint > UINT_MAX)) {
				lgs_conf.logStreamSystemLowLimit_cnfflag = LGS_CNF_DEF;
				LOG_ER("Illegal value for LOG_STREAM_SYSTEM_LOW_LIMIT - %s, default %u",
						strerror(errno), lgs_conf.logStreamSystemLowLimit);
			} else {
				lgs_conf.logStreamSystemLowLimit = (SaUint32T) val_uint;
				lgs_conf.logStreamSystemLowLimit_cnfflag = LGS_CNF_ENV;
			}
		} else if (lgs_conf.logStreamSystemHighLimit_cnfflag == LGS_CNF_DEF) {
			/* No config data found use default value */
			LOG_WA("No configuration data for logStreamSystemLowLimit found");
		}
	}
	TRACE("logStreamSystemLowLimit=%u, cnfflag '%s'",
			lgs_conf.logStreamSystemLowLimit,
		cnfflag_str(lgs_conf.logStreamSystemLowLimit_cnfflag));

	/* logStreamAppHighLimit */
	if (lgs_conf.logStreamAppHighLimit_cnfflag == LGS_CNF_DEF) {
	if ((val_str = getenv("LOG_STREAM_APP_HIGH_LIMIT")) != NULL) {
		errno = 0;
		val_uint = strtoul(val_str, NULL, 0);
		if ((errno != 0) || (val_uint > UINT_MAX)) {
			lgs_conf.logStreamAppHighLimit_cnfflag = LGS_CNF_DEF;
			LOG_ER("Illegal value for LOG_STREAM_APP_HIGH_LIMIT - %s, default %u",
					strerror(errno), lgs_conf.logStreamAppHighLimit);
			} else {
				lgs_conf.logStreamAppHighLimit = (SaUint32T) val_uint;
				lgs_conf.logStreamAppHighLimit_cnfflag = LGS_CNF_ENV;
			}
		} else if (lgs_conf.logStreamSystemHighLimit_cnfflag == LGS_CNF_DEF) {
			/* No config data found use default value */
			LOG_WA("No configuration data for logStreamAppHighLimit found");
		}
	}
	TRACE("logStreamAppHighLimit=%u, cnfflag '%s'",
			lgs_conf.logStreamAppHighLimit,
		cnfflag_str(lgs_conf.logStreamAppHighLimit_cnfflag));

	/* logStreamAppLowLimit */
	if (lgs_conf.logStreamAppLowLimit_cnfflag == LGS_CNF_DEF) {
		if ((val_str = getenv("LOG_STREAM_APP_LOW_LIMIT")) != NULL) {
			errno = 0;
			val_uint = strtoul(val_str, NULL, 0);
			if ((errno != 0) || (val_uint > UINT_MAX)) {
				lgs_conf.logStreamAppLowLimit = true;
				LOG_ER("Illegal value for LOG_STREAM_APP_LOW_LIMIT - %s, default %u",
						strerror(errno), lgs_conf.logStreamAppLowLimit);
			} else {
				lgs_conf.logStreamAppLowLimit = (SaUint32T) val_uint;
				lgs_conf.logStreamAppLowLimit_cnfflag = LGS_CNF_ENV;
			}
		} else if (lgs_conf.logStreamSystemHighLimit_cnfflag == LGS_CNF_DEF) {
			/* No config data found use default value */
			LOG_WA("No configuration data for logStreamAppLowLimit found");
		}
	}
	TRACE("logStreamAppLowLimit=%u, cnfflag '%s'",
			lgs_conf.logStreamAppLowLimit,
		cnfflag_str(lgs_conf.logStreamAppLowLimit_cnfflag));
	/*
	 * End ...SystemLimit
	 */

	/* logMaxApplicationStreams
	 * Rule: Object has precedence
	 */
	if (lgs_conf.logMaxLogrecsize_cnfflag == LGS_CNF_DEF) {
		if ((val_str = getenv("LOG_MAX_APPLICATION_STREAMS")) != NULL) {
			errno = 0;
			val_uint = strtoul(val_str, NULL, 0);
			if ((errno != 0) || (val_uint > UINT_MAX)) {
				LOG_ER("Illegal value for LOG_MAX_APPLICATION_STREAMS - %s, default %u",
						strerror(errno), lgs_conf.logMaxApplicationStreams);
			} else {
				lgs_conf.logMaxApplicationStreams = (SaUint32T) val_uint;
				lgs_conf.logMaxApplicationStreams_cnfflag = LGS_CNF_ENV;
			}
		} else { /* No environment variable use default value */
			LOG_WA("No configuration data for logMaxApplicationStreams found");
		}
	}
	TRACE("logMaxApplicationStreams=%u, cnfflag '%s'",
			lgs_conf.logMaxApplicationStreams,
		cnfflag_str(lgs_conf.logMaxApplicationStreams_cnfflag));

	/* The following attributes cannot be configured using an environment
	 * variable. If we get here the following applies:
	 * Rule: If no value from object use default value
	 */
	/* logFileIoTimeout */
	if (lgs_conf.logFileIoTimeout_cnfflag == LGS_CNF_DEF) {
		LOG_WA("No configuration data for logFileIoTimeout found");
	}

	/* logFileSysConfig */
	if (lgs_conf.logFileSysConfig_cnfflag == LGS_CNF_DEF) {
		LOG_WA("No configuration data for logFileSysConfig found");
	}

	TRACE_LEAVE();
}

/**
 * Become class implementer for the SaLogStreamConfig class
 * If HA state is ACTIVE
 * 
 * @param immOiHandle[in]
 * @return -1 on error
 */
static int config_class_impl_set(SaImmOiHandleT immOiHandle, SaAmfHAStateT ha_state)
{
	int rc = 0;
	SaAisErrorT ais_rc = SA_AIS_OK;
	struct ImmutilWrapperProfile immutilWrapperProfile_tmp;

	TRACE_ENTER2("immOiHandle = %lld, ha_state = %d (1 = Active)",
		immOiHandle, ha_state);

	if (ha_state != SA_AMF_HA_ACTIVE) {
		/* We are standby and cannot become implementer */
		TRACE("HA state STANDBY");
		TRACE_LEAVE();
		return 0;
	}
	
	/* Save immutil settings */
	immutilWrapperProfile_tmp.errorsAreFatal = immutilWrapperProfile.errorsAreFatal;
	immutilWrapperProfile_tmp.nTries = immutilWrapperProfile.nTries;

	/* Allow missed sync of large data to complete */
	immutilWrapperProfile.nTries = 250;
	/* Do not assert on error */
	immutilWrapperProfile.errorsAreFatal = 0;

	ais_rc = immutil_saImmOiClassImplementerSet(immOiHandle, "OpenSafLogConfig");
	if (ais_rc != SA_AIS_OK) {
		/* immutil_saImmOiClassImplementerSet Fail
		 * Class may not exist
		 */
		LOG_WA("%s: saImmOiClassImplementerSet Fail %s",__FUNCTION__,
			saf_error(ais_rc));
		rc = -1;
	}

	/* Restore immutil settings */
	immutilWrapperProfile.errorsAreFatal = immutilWrapperProfile_tmp.errorsAreFatal;
	immutilWrapperProfile.nTries = immutilWrapperProfile_tmp.nTries;

	TRACE_LEAVE();
	return rc;
}

/******************************************************************************
 * Public functions for handling configuration information
 ******************************************************************************/

/**
 * Read the log service configuration data verify and update configuration
 * data structure
 */
void lgs_cfg_init(SaImmOiHandleT immOiHandle, SaAmfHAStateT ha_state)
{
	int int_rc = 0;

	TRACE_ENTER2("immOiHandle = %lld", immOiHandle);
	/* Initiate the default values for all parameters
	 */
	init_default();

	/* Read configuration step 1
	 * Become class implementer for SaLogStreamConfig class
	 * Read all values from the log service configuration object
	 */
	int_rc = config_class_impl_set(immOiHandle, ha_state);
	if (int_rc != -1) {
		read_logsv_config_obj_2();
	} else {
		TRACE("Failed to become class implementer. Config object not read");
	}

	/* Read configuration step 2
	 * Get configuration data from environment variables.
	 * This is done according to a set of rules (see inside function for
	 * details).
	 * Rules examples:
	 *  - Configuration object has precedence:
	 *    If a value exist both in the configuration object and in an
	 *    environment variable the value from the conf. obj. will be used.
	 *    If no value is found a hard coded default value is used
	 *
	 *  - Environment variable has precedence:
	 *    If a value exist both in the configuration object and in an
	 *    environment variable the value from the env. var. will be used.
	 *    If no value is found a hard coded default value is used
	 *
	 * - Other rules...
	 */
	read_log_config_environ_var_2();

	/* Verify all configuration. If any configuration fail it will
	 * be replaced by default value
	 */
	(void) verify_all_init();
	TRACE_LEAVE();
}

/**
 * Get log service configuration parameter from the configuration data struct.
 * The scope of configuration data is restricted to this file.
 * This function gives read access to the configuration data
 *
 * @param lgs_logconfGet_t param[in]
 *     Defines what configuration parameter to return
 *
 * @return void *
 *     Pointer to the parameter. See struct lgs_conf
 *
 */
const void *lgs_cfg_get(lgs_logconfGet_t param)
{
	void *value_ptr;
	/* Lock mutex while reading config data from struct */
	osaf_mutex_lock_ordie(&lgs_config_data_mutex);

	switch (param) {
	case LGS_IMM_LOG_ROOT_DIRECTORY:
		value_ptr = lgs_conf.logRootDirectory;
		break;
	case LGS_IMM_DATA_GROUPNAME:
		value_ptr = lgs_conf.logDataGroupname;
		break;
	case LGS_IMM_LOG_STREAM_FILE_FORMAT:
		value_ptr = lgs_conf.logStreamFileFormat;
		break;
	case LGS_IMM_LOG_MAX_LOGRECSIZE:
		value_ptr = &lgs_conf.logMaxLogrecsize;
		break;
	case LGS_IMM_LOG_STREAM_SYSTEM_HIGH_LIMIT:
		value_ptr = &lgs_conf.logStreamSystemHighLimit;
		break;
	case LGS_IMM_LOG_STREAM_SYSTEM_LOW_LIMIT:
		value_ptr = &lgs_conf.logStreamSystemLowLimit;
		break;
	case LGS_IMM_LOG_STREAM_APP_HIGH_LIMIT:
		value_ptr = &lgs_conf.logStreamAppHighLimit;
		break;
	case LGS_IMM_LOG_STREAM_APP_LOW_LIMIT:
		value_ptr = &lgs_conf.logStreamAppLowLimit;
		break;
	case LGS_IMM_LOG_MAX_APPLICATION_STREAMS:
		value_ptr = &lgs_conf.logMaxApplicationStreams;
		break;
	case LGS_IMM_FILEHDL_TIMEOUT:
		value_ptr = &lgs_conf.logFileIoTimeout;
		break;
	case LGS_IMM_LOG_FILESYS_CFG:
		value_ptr = &lgs_conf.logFileSysConfig;
		break;
	case LGS_IMM_LOG_OPENSAFLOGCONFIG_CLASS_EXIST:
		value_ptr = &lgs_conf.OpenSafLogConfig_object_exist;
		break;

	case LGS_IMM_LOG_NUMBER_OF_PARAMS:
	case LGS_IMM_LOG_NUMEND:
	default:
		LOG_ER("Invalid parameter %u",param);
		osaf_abort(0); /* Should never happen */
		break;
	}

	/* Done reading */
	osaf_mutex_unlock_ordie(&lgs_config_data_mutex);

	return value_ptr;
}

/**
 * Check if path is a writeable directory
 * We must have rwx access.
 * @param pathname
 * return: true  = Path is valid
 *         false = Path is invalid
 */
bool lgs_path_is_writeable_dir_h(const char *pathname)
{
	bool is_writeable_dir = false;

	lgsf_apipar_t apipar;
	lgsf_retcode_t api_rc;
	void *params_in_p;

	TRACE_ENTER();

	TRACE("%s - pathname \"%s\"",__FUNCTION__,pathname);

	size_t params_in_size = strlen(pathname)+1;
	if (params_in_size > PATH_MAX) {
		is_writeable_dir = false;
		LOG_WA("Path > PATH_MAX");
		goto done;
	}

	/* Allocate memory for parameter */
	params_in_p = malloc(params_in_size);

	/* Fill in path */
	memcpy(params_in_p, pathname, params_in_size);

	/* Fill in API structure */
	apipar.req_code_in = LGSF_CHECKDIR;
	apipar.data_in_size = params_in_size;
	apipar.data_in = params_in_p;
	apipar.data_out_size = 0;
	apipar.data_out = NULL;

	api_rc = log_file_api(&apipar);
	if (api_rc != LGSF_SUCESS) {
		TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
		is_writeable_dir = false;
	} else {
		if (apipar.hdl_ret_code_out == 0)
			is_writeable_dir = false;
		else
			is_writeable_dir = true;
	}

	free(params_in_p);

done:
	TRACE_LEAVE2("is_writeable_dir = %d",is_writeable_dir);
	return is_writeable_dir;
}

/***
 * Support older check-point protocols prior to version 5
 */
/**
 * Used for updating logRootDirectory on standby when check-pointed
 *
 * Set the logRootDirectory parameter in the lgs_conf struct
 * Used for holding data from config object
 *
 * @param root_path_str
 */
void lgs_rootpathconf_set(const char *root_path_str)
{
	/* Lock mutex while config data is written */
	osaf_mutex_lock_ordie(&lgs_config_data_mutex);

	(void) snprintf(lgs_conf.logRootDirectory, PATH_MAX, "%s",
		root_path_str);
	TRACE("%s logRootDirectory updated to \"%s\"",__FUNCTION__,
		lgs_conf.logRootDirectory);

	osaf_mutex_unlock_ordie(&lgs_config_data_mutex);
}

/**
 * Used for updating logRootDirectory on standby when check-pointed
 *
 * Set the logDataGroupname parameter in the lgs_conf struct
 * Used for holding data from config object
 *
 * @param root_path_str
 */
void lgs_groupnameconf_set(const char *data_groupname_str)
{
	/* Lock mutex while config data is written */
	osaf_mutex_lock_ordie(&lgs_config_data_mutex);

	(void) snprintf(lgs_conf.logDataGroupname, UT_NAMESIZE, "%s",
		data_groupname_str);
	TRACE("%s logDataGroupname updated to \"%s\"",__FUNCTION__,
		lgs_conf.logDataGroupname);

	osaf_mutex_unlock_ordie(&lgs_config_data_mutex);
}

/******************************************************************************
 * Handle Log service configuration runtime object
 * Used to display log service actual configuration
 ******************************************************************************/

/**
 * Create runtime object to show log service configuration
 *
 * @param immOiHandle[in]
 */
void conf_runtime_obj_create(SaImmOiHandleT immOiHandle)
{
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER();

	/* Construct object with dn:
	 * logConfig=currentConfig,safApp=safLogService
	 */
	char namestr[128];
	strcpy(namestr, "logConfig=currentConfig");
	char *nameptr = namestr;
	void *valarr[] = { &nameptr };
	const SaImmAttrValuesT_2 attr_logConfig = {
		.attrName = "logConfig",
		.attrValueType = SA_IMM_ATTR_SASTRINGT,
		.attrValuesNumber = 1,
		.attrValues = valarr
	};

	const SaImmAttrValuesT_2 *attrValues[] = {
		&attr_logConfig,
		NULL
	};

	SaNameT parent_name, *parent_name_p;
	strcpy((char *) parent_name.value, "safApp=safLogService");
	parent_name.length = strlen((char *) parent_name.value);
	parent_name_p = &parent_name;

	rc = saImmOiRtObjectCreate_2(immOiHandle,
			"OpenSafLogCurrentConfig",
			parent_name_p,
			attrValues);

	if (rc == SA_AIS_ERR_EXIST) {
		TRACE("Server runtime configuration object already exist");
	} else if (rc != SA_AIS_OK) {
		LOG_NO("%s: Cannot create config runtime object %s",
			__FUNCTION__, saf_error(rc));
	}

	TRACE_LEAVE();
}

/**
 * Handler for updating runtime configuration object attributes
 * Called from the SaImmOiRtAttrUpdateCallbackT type function
 *
 * @param immOiHandle[in]
 * @param attributeNames[in]
 */
void conf_runtime_obj_hdl(SaImmOiHandleT immOiHandle, const SaImmAttrNameT *attributeNames)
{
	SaImmAttrNameT attributeName;
	int i = 0;
	char *str_val = NULL;
	SaUint32T u32_val = 0;

	TRACE_ENTER();

	while ((attributeName = attributeNames[i++]) != NULL) {
		TRACE("Attribute %s", attributeName);
		if (!strcmp(attributeName, LOG_ROOT_DIRECTORY)) {
			str_val = (char *)
				lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY);
			(void)immutil_update_one_rattr(immOiHandle,
				LGS_CFG_RUNTIME_OBJECT,
				attributeName, SA_IMM_ATTR_SASTRINGT,
				&str_val);
		} else if (!strcmp(attributeName, LOG_DATA_GROUPNAME)) {
			str_val = (char *)
				lgs_cfg_get(LGS_IMM_DATA_GROUPNAME);
			(void)immutil_update_one_rattr(immOiHandle,
				LGS_CFG_RUNTIME_OBJECT,
				attributeName, SA_IMM_ATTR_SASTRINGT,
				&str_val);
		} else if (!strcmp(attributeName, LOG_STREAM_FILE_FORMAT)) {
			str_val = (char *)
				lgs_cfg_get(LGS_IMM_LOG_STREAM_FILE_FORMAT);
			(void)immutil_update_one_rattr(immOiHandle,
			   LGS_CFG_RUNTIME_OBJECT,
			   attributeName, SA_IMM_ATTR_SASTRINGT,
			   &str_val);
		} else if (!strcmp(attributeName, LOG_MAX_LOGRECSIZE)) {
			u32_val = *(SaUint32T *)
				lgs_cfg_get(LGS_IMM_LOG_MAX_LOGRECSIZE);
			(void) immutil_update_one_rattr(immOiHandle,
				LGS_CFG_RUNTIME_OBJECT,
				attributeName, SA_IMM_ATTR_SAUINT32T,
				&u32_val);
		} else if (!strcmp(attributeName, LOG_STREAM_SYSTEM_HIGH_LIMIT)) {
			u32_val = *(SaUint32T *)
				lgs_cfg_get(LGS_IMM_LOG_STREAM_SYSTEM_HIGH_LIMIT);
			(void) immutil_update_one_rattr(immOiHandle,
				LGS_CFG_RUNTIME_OBJECT,
				attributeName, SA_IMM_ATTR_SAUINT32T,
				&u32_val);
		} else if (!strcmp(attributeName, LOG_STREAM_SYSTEM_LOW_LIMIT)) {
			u32_val = *(SaUint32T *)
				lgs_cfg_get(LGS_IMM_LOG_STREAM_SYSTEM_LOW_LIMIT);
			(void) immutil_update_one_rattr(immOiHandle,
				LGS_CFG_RUNTIME_OBJECT,
				attributeName, SA_IMM_ATTR_SAUINT32T,
				&u32_val);
		} else if (!strcmp(attributeName, LOG_STREAM_APP_HIGH_LIMIT)) {
			u32_val = *(SaUint32T *)
				lgs_cfg_get(LGS_IMM_LOG_STREAM_APP_HIGH_LIMIT);
			(void) immutil_update_one_rattr(immOiHandle,
				LGS_CFG_RUNTIME_OBJECT,
				attributeName, SA_IMM_ATTR_SAUINT32T,
				&u32_val);
		} else if (!strcmp(attributeName, LOG_STREAM_APP_LOW_LIMIT)) {
			u32_val = *(SaUint32T *)
				lgs_cfg_get(LGS_IMM_LOG_STREAM_APP_LOW_LIMIT);
			(void) immutil_update_one_rattr(immOiHandle,
				LGS_CFG_RUNTIME_OBJECT,
				attributeName, SA_IMM_ATTR_SAUINT32T,
				&u32_val);
		} else if (!strcmp(attributeName, LOG_MAX_APPLICATION_STREAMS)) {
			u32_val = *(SaUint32T *)
				lgs_cfg_get(LGS_IMM_LOG_MAX_APPLICATION_STREAMS);
			(void) immutil_update_one_rattr(immOiHandle,
				LGS_CFG_RUNTIME_OBJECT,
				attributeName, SA_IMM_ATTR_SAUINT32T,
				&u32_val);
		} else if (!strcmp(attributeName, LOG_FILE_IO_TIMEOUT)) {
			u32_val = *(SaUint32T *)
				lgs_cfg_get(LGS_IMM_FILEHDL_TIMEOUT);
			(void) immutil_update_one_rattr(immOiHandle,
				LGS_CFG_RUNTIME_OBJECT,
				attributeName, SA_IMM_ATTR_SAUINT32T,
				&u32_val);
		} else if (!strcmp(attributeName, LOG_FILE_SYS_CONFIG)) {
			u32_val = *(SaUint32T *)
				lgs_cfg_get(LGS_IMM_LOG_FILESYS_CFG);
			(void) immutil_update_one_rattr(immOiHandle,
				LGS_CFG_RUNTIME_OBJECT,
				attributeName, SA_IMM_ATTR_SAUINT32T,
				&u32_val);
		} else {
			TRACE("%s: unknown attribute %s",
				__FUNCTION__, attributeName);
		}
	}

	TRACE_LEAVE();
}

/******************************************************************************
 * Trace functions
 */

static char *cnfflag_str(lgs_conf_flg_t cnfflag)
{
	switch (cnfflag) {
	case LGS_CNF_OBJ:
		return "Config object";
	case LGS_CNF_ENV:
		return "Environment variable";
	case LGS_CNF_DEF:
		return "Default value";
	default:
		return "Bad input";
	}
}

/**
 * Print all in lgs_conf to trace file
 */
void lgs_trace_config(void)
{
	/* Lock mutex while reading config data from struct */
	osaf_mutex_lock_ordie(&lgs_config_data_mutex);

	TRACE("===== LOG Configuration Start =====");
	TRACE("logRootDirectory\t\t \"%s\",\t %s",
		lgs_conf.logRootDirectory,
		cnfflag_str(lgs_conf.logRootDirectory_cnfflag));
	TRACE("logDataGroupname\t\t \"%s\",\t %s",
		lgs_conf.logDataGroupname,
		cnfflag_str(lgs_conf.logDataGroupname_cnfflag));
	TRACE("logStreamFileFormat\t\t \"%s\",\t %s",
		  lgs_conf.logStreamFileFormat,
		  cnfflag_str(lgs_conf.logStreamFileFormat_cnfflag));
	TRACE("logMaxLogrecsize\t\t %u,\t %s",
		lgs_conf.logMaxLogrecsize,
		cnfflag_str(lgs_conf.logMaxLogrecsize_cnfflag));
	TRACE("logStreamSystemHighLimit\t %u,\t %s",
		lgs_conf.logStreamSystemHighLimit,
		cnfflag_str(lgs_conf.logStreamSystemHighLimit_cnfflag));
	TRACE("logStreamSystemLowLimit\t %u,\t %s",
		lgs_conf.logStreamSystemLowLimit,
		cnfflag_str(lgs_conf.logStreamSystemLowLimit_cnfflag));
	TRACE("logStreamAppHighLimit\t %u,\t %s",
		lgs_conf.logStreamAppHighLimit,
		cnfflag_str(lgs_conf.logStreamAppHighLimit_cnfflag));
	TRACE("logStreamAppLowLimit\t\t %u,\t %s",
		lgs_conf.logStreamAppLowLimit,
		cnfflag_str(lgs_conf.logStreamAppLowLimit_cnfflag));
	TRACE("logMaxApplicationStreams\t %u,\t %s",
		lgs_conf.logMaxApplicationStreams,
		cnfflag_str(lgs_conf.logMaxApplicationStreams_cnfflag));
	TRACE("logFileIoTimeout\t\t %u,\t %s",
		lgs_conf.logFileIoTimeout,
		cnfflag_str(lgs_conf.logFileIoTimeout_cnfflag));
	TRACE("logFileSysConfig\t\t %u,\t %s",
		lgs_conf.logFileSysConfig,
		cnfflag_str(lgs_conf.logFileSysConfig_cnfflag));
	TRACE("OpenSafLogConfig_object_exist\t %s",
		lgs_conf.OpenSafLogConfig_object_exist ? "True": "False");
	TRACE("===== LOG Configuration End =====");

	/* Done reading */
	osaf_mutex_unlock_ordie(&lgs_config_data_mutex);
}

/**
 * Print configuration values read using lgs_cfg_get()
 */
void lgs_cfg_read_trace(void)
{
	TRACE("##### LOG Configuration parameter read start #####");
	TRACE("logRootDirectory\t\t \"%s\"", (char *) lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));
	TRACE("logDataGroupname\t\t \"%s\"", (char *) lgs_cfg_get(LGS_IMM_DATA_GROUPNAME));
	TRACE("logStreamFileFormat\t\t \"%s\"", (char *) lgs_cfg_get(LGS_IMM_LOG_STREAM_FILE_FORMAT));
	TRACE("logMaxLogrecsize\t\t %u", *(SaUint32T *) lgs_cfg_get(LGS_IMM_LOG_MAX_LOGRECSIZE));
	TRACE("logStreamSystemHighLimit\t %u", *(SaUint32T *) lgs_cfg_get(LGS_IMM_LOG_STREAM_SYSTEM_HIGH_LIMIT));
	TRACE("logStreamSystemLowLimit\t %u", *(SaUint32T *) lgs_cfg_get(LGS_IMM_LOG_STREAM_SYSTEM_LOW_LIMIT));
	TRACE("logStreamAppHighLimit\t %u", *(SaUint32T *) lgs_cfg_get(LGS_IMM_LOG_STREAM_APP_HIGH_LIMIT));
	TRACE("logStreamAppLowLimit\t\t %u", *(SaUint32T *) lgs_cfg_get(LGS_IMM_LOG_STREAM_APP_LOW_LIMIT));
	TRACE("logMaxApplicationStreams\t %u", *(SaUint32T *) lgs_cfg_get(LGS_IMM_LOG_MAX_APPLICATION_STREAMS));
	TRACE("logFileIoTimeout\t\t %u", *(SaUint32T *) lgs_cfg_get(LGS_IMM_FILEHDL_TIMEOUT));
	TRACE("logFileSysConfig\t\t %u", *(SaUint32T *) lgs_cfg_get(LGS_IMM_LOG_FILESYS_CFG));
	TRACE("##### LOG Configuration parameter read done  #####");
}
