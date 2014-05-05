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

#include "lgs_filehdl.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <unistd.h>

#include <configmake.h>
#include <logtrace.h>
#include <ncsgl_defs.h>
#include <osaf_utility.h>

#include "lgs.h"
#include "lgs_util.h"
#include "lgs_file.h"


extern pthread_mutex_t lgs_ftcom_mutex;	/* For locking communication */

/*****************************************************************************
 * File operation handlers:
 * This is the part of a file system handling function that shall execute in
 * the file handling thread. 
 *****************************************************************************/

/**
 * Check if path is a writeable directory
 * 
 * @param indata[in] path[]
 * @param outdata[out] Not used
 * @param max_outsize[in] Must be 0
 * @return 0 if not writable, 1 if writable
 */
int path_is_writeable_dir_hdl(void *indata, void *outdata, size_t max_outsize)
{
	int is_writeable_dir = 0;
	struct stat pathstat;
	
	char *pathname = (char *) indata;

	TRACE_ENTER();
	
	TRACE("%s - pathname \"%s\"",__FUNCTION__,pathname);

	/* Check if the pathname violates security rules e.g. contains ../ */
	if (lgs_relative_path_check_ts(pathname) || stat(pathname, &pathstat) != 0) {
		LOG_NO("Path %s not allowed", pathname);
		goto done;
	}

	/* Check if the path points to a directory */
	if (!S_ISDIR(pathstat.st_mode)) {
		LOG_NO("%s is not a directory", pathname);
		goto done;
	}

	/* Check if we have correct permissions. Note that we check permissions for
	 * real UID
	 */
	if (access(pathname, (R_OK | W_OK | X_OK)) != 0) {
		LOG_NO("permission denied for %s, error %s", pathname, strerror(errno));
		goto done;
	}

	is_writeable_dir = 1;
done:
	TRACE_LEAVE2("is_writeable_dir = %d",is_writeable_dir);
	return is_writeable_dir;	
}

/**
 * Check if a path exists. See also stat(..)
 * 
 * @param indata[in] path[]
 * @param outdata[out] Not used
 * @param max_outsize[in] Must be 0
 * @return (-1) if not exists, 0 if exists
 */
int check_path_exists_hdl(void *indata, void *outdata, size_t max_outsize)
{
	struct stat pathstat;
	char *path_str = (char *) indata;
	int rc = 0;
	
	rc = stat(path_str, &pathstat);
	TRACE("%s - path_str \"%s\", rc=%d",__FUNCTION__,path_str,rc);
	TRACE("%s - errno \"%s\"",__FUNCTION__,strerror(errno));
	return rc;
}

/**
 * Rename a file
 * 
 * @param indata[in] old_path[], new_path[]
 * @param outdata[out] Not used
 * @param max_outsize[in] Must be 0
 * @return (-1) on error, errno is logged
 */
int rename_file_hdl(void *indata, void *outdata, size_t max_outsize)
{
	int rc = 0;
	
	TRACE_ENTER();
	size_t old_path_size = *((size_t *) indata);
	char *old_path = (char *) indata + sizeof(size_t);
	char *new_path = old_path + old_path_size;
	
	if ((rc = rename(old_path, new_path)) == -1)
		LOG_NO("rename: FAILED - %s", strerror(errno));

	TRACE_LEAVE();
	return rc;
}

/**
 * Create a configuration file.
 * Creates the file, write its content and close the file
 * 
 * @param indata[in] ccfh_t, logFileFormat string, file_path string
 * @param outdata[out] Not used
 * @param max_outsize[in] Must be 0
 * @return (-1) on error, errno is logged
 */
int create_config_file_hdl(void *indata, void *outdata, size_t max_outsize)
{
	int rc = 0;
	FILE *filp;
	ccfh_t *params_in = (ccfh_t *) indata;
	char *logFileFormat = (char *) (indata + sizeof(ccfh_t));
	char *file_path = (logFileFormat + params_in->logFileFormat_size);
	
	TRACE_ENTER();
	
	TRACE("%s - file_path \"%s\"",__FUNCTION__,file_path);
fopen_retry:
	if ((filp = fopen(file_path, "w")) == NULL) {
		if (errno == EINTR)
			goto fopen_retry;

		LOG_NO("Could not open '%s' - %s", file_path, strerror(errno));
		rc = -1;
		goto done;
	}

	/* version */
	if ((rc = fprintf(filp, "%s %c.%d.%d\n", LOG_VER_EXP,
			params_in->version.releaseCode,
			params_in->version.majorVersion,
			params_in->version.minorVersion)) == -1) {
		TRACE("%s - Print version failed",__FUNCTION__);
		goto fprintf_done;
	}

	/* Format expression */
	if ((rc = fprintf(filp, "%s%s\n", FMAT_EXP, logFileFormat)) == -1) {
		TRACE("%s - Print Format expression failed",__FUNCTION__);
		goto fprintf_done;
	}

	/* Max logfile size */
	if ((rc = fprintf(filp, "%s %llu\n", CFG_EXP_MAX_FILE_SIZE, params_in->maxLogFileSize)) == -1) {
		TRACE("%s - Print Max logfile size failed",__FUNCTION__);
		goto fprintf_done;
	}

	/* Fixed log record size */
	if ((rc = fprintf(filp, "%s %d\n", CFG_EXP_FIXED_LOG_REC_SIZE, params_in->fixedLogRecordSize)) == -1) {
		TRACE("%s - Print Fixed log record size failed",__FUNCTION__);
		goto fprintf_done;
	}

	/* Log file full action */
	rc = fprintf(filp, "%s %s %d\n", CFG_EXP_LOG_FULL_ACTION, DEFAULT_ALM_ACTION, params_in->maxFilesRotated);
	if (rc == -1) {
		TRACE("%s - Print version failed",__FUNCTION__);
	}

 fprintf_done:
	if (rc == -1)
		LOG_NO("Could not write to \"%s\"", file_path);

fclose_retry:
	if ((rc = fclose(filp)) == -1) {
		if (errno == EINTR)
			goto fclose_retry;

		LOG_NO("Could not close \"%s\" - \"%s\"", file_path, strerror(errno));
	}

done:
	TRACE_LEAVE2("rc = %d", rc);
	return rc;	
}

/**
 * Write a log record to file
 * The file must be opened for append
 * 
 * @param indata[in] Type wlrh_t
 * @param outdata[out], int errno, 0 if no error
 * @param max_outsize[in], always sizeof(int)
 * @return (-1) on error or number of written bytes
 */
int write_log_record_hdl(void *indata, void *outdata, size_t max_outsize, bool *timeout_f)
{
	int rc, bytes_written = 0;
	off_t file_length = 0;
	wlrh_t *params_in = (wlrh_t *) indata;
	/* The logrecord is stored in the indata buffer right after the
	 * wlrh_t structure
	 */
	char *logrecord = (char *) (indata + sizeof(wlrh_t));
	int *errno_out_p = (int *) outdata;
	*errno_out_p = 0;

//#define LLD_DELAY_WRTST /* LLDTEST */
#ifdef LLD_DELAY_WRTST /* Make "file system" hang for n sec at first write */
	static bool lld_once_f = true;
	const unsigned int lld_sleep_sec = 10;
#endif
	
	TRACE_ENTER();
	
 retry:
	rc = write(params_in->fd, &logrecord[bytes_written],
		 params_in->record_size - bytes_written);
	if (rc == -1) {
		if (errno == EINTR)
			goto retry;

		LOG_ER("%s - write FAILED: %s",__FUNCTION__, strerror(errno));
		*errno_out_p = errno;
		goto done;
	} else {
		/* Handle partial writes */
		bytes_written += rc;
		if (bytes_written < params_in->record_size)
			goto retry;
	}
#ifdef LLD_DELAY_WRTST /* LLDTEST Wait first time thread is used */
	if (strstr(logrecord, "xxx")) {
		if (lld_once_f == true) {
			lld_once_f = false;
			TRACE("LLDTEST xxx Hang write");
			//TRACE("LLDTEST: logrecord \"%s\"",logrecord);
			sleep(lld_sleep_sec);
			TRACE("LLDTEST End of sleep");
		}
	}
	if (strstr(logrecord, "yyy")) {
		lld_once_f = true;
		TRACE("LLDTEST yyy Rearmed Hang write");
	}
#endif
 
	/* If the thread was hanging and has timed out and the log record was
	 * written it is invalid and shall be removed from file (log service has
	 * returned SA_AIS_TRY_AGAIN). 
	 */
	osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK */
	if (*timeout_f == true) {
		TRACE("Timeout, removing last log record");
		file_length = lseek(params_in->fd, -bytes_written, SEEK_END);
		if (file_length != -1) {
			do { 
				rc = ftruncate(params_in->fd, file_length);
			} while ((rc == -1) && (errno == EINTR));
		}

		if (file_length == -1) {
			LOG_WA("%s - lseek error, Could not remove redundant log record, %s",
					__FUNCTION__,strerror(errno));
		} else if (rc == -1) {
			LOG_WA("%s - ftruncate error, Could not remove redundant log record, %s",
					__FUNCTION__,strerror(errno));			
		}
	}
	osaf_mutex_unlock_ordie(&lgs_ftcom_mutex); /* UNLOCK */
 
done:
	TRACE_LEAVE2("rc = %d",rc);
	return rc;
}

/**
 * Make directory. Handles creation of directory path.
 * Creates the relative directory in the directory given by the root path.
 * If the root path does not exist/is not available a root path is created
 * based on the default path.
 * 
 * TBD (Separate ticket):
 * Default path taken from the configuration define PKGLOGDIR.
 * Settings: Read, Write, Exec for User, Group, Other
 * 
 * @param indata[in], Type mld_in_t
 * @param outdata[out], char *, new root dir if changed otherwise '\0'
 * @param max_outsize[in]
 * @return (-1) if error
 */
int make_log_dir_hdl(void *indata, void *outdata, size_t max_outsize)
{
	int rc = 0;
	int mldh_rc = 0;
	mld_in_t *params_in = (mld_in_t *) indata;
	char *out_path = (char *) outdata;
	
	char *relpath = params_in->rel_path;
	char *rootpath = params_in->root_dir;
	char dir_to_make[PATH_MAX];
	char mpath[PATH_MAX];
	char *spath_p;
	char *epath_p;
	struct stat statbuf;
	int n;
	
	TRACE_ENTER();
	
	TRACE("rootpath \"%s\"",rootpath);
	TRACE("relpath \"%s\"",relpath);
	
	/* 
	 * Create root directory if it does not exists.
	 * TBD. Handle via separate ticket
	 * (Create the default root path regardless of what is set in the
	 * configuration object.)
	 */
	out_path[0] = '\0';
	if (lstat(rootpath, &statbuf) != 0) {
#if 0
		rootpath = PKGLOGDIR;
#endif
		strncpy(out_path, rootpath, max_outsize);
		n = snprintf(out_path, max_outsize, "%s", rootpath);
		if (n >= max_outsize) {
			LOG_WA("Invalid root path > max_outsize");
			mldh_rc = -1;
			goto done;
		}
		LOG_NO("LOG Root path does not exist. Will be created");
		TRACE("%s - create rootpath \"%s\"",__FUNCTION__,rootpath);
	}
	
	/*
	 * Create root path without preceding '/' and ending '/'
	 * Example: ro1/ro2/ro3
	 * Check that not > PATH_MAX
	 */
	n = snprintf(mpath, PATH_MAX, "%s", rootpath);
	if (n >= PATH_MAX) {
		LOG_WA("Could not create path, rootpath > PATH_MAX");
		mldh_rc = -1;
		goto done;
	}
	char *rootpp = mpath;
	while (*rootpp == '/') rootpp++; /* Remove preceding '/' */
	while (mpath[strlen(mpath)-1] == '/') { /* Remove trailing '/' if needed */
		mpath[strlen(mpath)-1] = '\0';
	}

	/*
	 * Add relative path. Shall end with '/'
	 * Example: ro1/ro2/ro3/re1/re2/re3/
	 * Check that not > PATH_MAX
	 */
	if (relpath[strlen(relpath)-1] != '/') {
		n = snprintf(dir_to_make, PATH_MAX, "/%s/%s/", rootpp, relpath);
	} else {
		n = snprintf(dir_to_make, PATH_MAX, "/%s/%s", rootpp, relpath);
	}
	if (n >= PATH_MAX) {
		LOG_WA("Could not create path > PATH_MAX");
		mldh_rc = -1;
		goto done;
	}
	TRACE("%s - Path to create \"%s\"",__FUNCTION__,dir_to_make);
		
	/* Create the path */
	int path_len = 0;
	spath_p = epath_p = dir_to_make;
	while ((epath_p = strchr(epath_p, '/')) != NULL) {
		if (epath_p == spath_p) {
			epath_p++;
			continue; /* Don't try to create path "/" */
		}
		epath_p++;
		path_len = epath_p - spath_p;
		strncpy(mpath, spath_p, path_len);
		mpath[path_len] = '\0';
		rc = mkdir(mpath, S_IRWXU | S_IRWXG | S_IRWXO);
		if ((rc != 0) && (errno != EEXIST)) {
			LOG_ER("Making directory error %s",strerror(errno));
			mldh_rc = -1;
			goto done;
		}
	}
	TRACE("%s - Dir \"%s\" created",__FUNCTION__, mpath);
	
done:
	TRACE_LEAVE2("mldh_rc = %u", mldh_rc);
	return mldh_rc;
}

/**
 * Open/create a file for append in non blocking mode.
 * Note: The file is opened in NONBLOCK mode directly. This makes it possible
 *       that the open succeeds but the following write will fail. To avoid
 *       this the file can be opened without the O_NONBLOCK flag and set this
 *       flag using fcntl(). But write handling is done so that if a write
 *       fails the log file will always be reopened.
 *
 * @param indata[in], Null-terminated string containing filename to open
 * @param outdata[out], int errno, 0 if no error
 * @param max_outsize[in], always sizeof(int)
 * @return file descriptor or -1 if error
 */
int fileopen_hdl(void *indata, void *outdata, size_t max_outsize)
{
	int errno_save = 0;
	char *filepath = (char *) indata;
	int *errno_out_p = (int *) outdata;
	int fd;
	
	TRACE_ENTER();
	
	TRACE("%s - filepath \"%s\"",__FUNCTION__,filepath);
open_retry:
	fd = open(filepath, O_CREAT | O_RDWR | O_APPEND | O_NONBLOCK,
							S_IRUSR | S_IWUSR | S_IRGRP);

	if (fd == -1) {
		if (errno == EINTR)
			goto open_retry;
		/* save errno for caller logging */
		errno_save = errno;
		/* Do not log with higher severity here to avoid flooding the log.
		 * Can be called in context of log_stream_write */
		LOG_IN("Could not open: %s - %s", filepath, strerror(errno));
	}

	*errno_out_p = errno_save;
	TRACE_LEAVE();
	return fd;
}

/**
 * Close with retry at EINTR
 * @param indata[in], of type fileclose_hdl_par_t
 * @param outdata[out], Not used, can be set to NULL
 * @param max_outsize[in], must be set to 0
 * @return (-1) if error
 */
int fileclose_hdl(void *indata, void *outdata, size_t max_outsize)
{
	int rc = 0;
	int fd;
	
	fd = *(char *) indata;
	TRACE_ENTER2("fd=%d", fd);

	rc = close(fd);

	if (rc == -1) {
		LOG_ER("fileclose() %s",strerror(errno));
	}
	TRACE_LEAVE2("rc=%d", rc);
	return rc;
}

/**
 * Delete config file.
 * @param indata[in], pathname string
 * @param outdata[out], always NULL
 * @param max_outsize[in], must be set to 0
 * @return (-1) if error
 */
int delete_file_hdl(void *indata, void *outdata, size_t max_outsize)
{
	int rc = 0;
	char *pathname = (char *) indata;
	
	TRACE_ENTER();
	
	if ((rc = unlink(pathname)) == -1) {
		if (errno == ENOENT)
			rc = 0;
		else
			LOG_NO("could not unlink: %s - %s", pathname, strerror(errno));
	}

	TRACE_LEAVE();
	return rc;
}

/******************************************************************************
 * Utility functions for get_number_of_log_files_hdl
 */
static int check_oldest(char *line, char *fname_prefix, int fname_prefix_size, int *old_date, int *old_time)
{
	int date, time, c, d;
	date = time = c = d = 0;
	int len = 0;
	char name_format[SA_MAX_NAME_LENGTH];
	char time_stamps[] = "_%d_%d_%d_%d.log";

	len = strlen(time_stamps);
	len += fname_prefix_size;

	strncpy(name_format, fname_prefix, fname_prefix_size);
	name_format[fname_prefix_size] = '\0';
	TRACE_3("fname: %s", name_format);
	strncat(name_format, time_stamps, SA_MAX_NAME_LENGTH-1);
	if (sscanf(line, name_format, &date, &time, &c, &d) == 4) {
		TRACE_3("line: arg1: %d 2: %d 3: %d 4: %d ok", date, time, c, d);
		if (date < *old_date || *old_date == -1) {
			*old_date = date;
			*old_time = time;
			return 1;
		} else if ((date == *old_date) && (time < *old_time)) {
			*old_date = date;
			*old_time = time;
			return 1;
		}
	} else if (sscanf(line, name_format, &date, &time) == 2) {
		TRACE_3("line: arg1: %d 2: %d ok", date, time);
		if (date < *old_date || *old_date == -1) {
			*old_date = date;
			*old_time = time;
			return 1;
		} else if ((date == *old_date) && (time < *old_time)) {
			*old_date = date;
			*old_time = time;
			return 1;
		}
	} else {
		TRACE_3("no match");
	}
	return 0;
}

/* Filter function used by scandir. */
static char file_prefix[SA_MAX_NAME_LENGTH];
static int filter_func(const struct dirent *finfo)
{
	int ret;
	ret = strncmp(file_prefix, finfo->d_name, strlen(file_prefix));
	return !ret;
}

/**
 * Return number of log files in a dir and the name of the oldest file.
 * @param indata, see gnolfh_in_t
 * @param outdata, char *oldest_file
 * @param max_outsize, Max size for oldest_file string
 * 
 * @return int, number of logfiles or -1 if error
 */
int get_number_of_log_files_hdl(void *indata, void *outdata, size_t max_outsize)
{
	struct dirent **namelist;
	int n, old_date = -1, old_time = -1, old_ind = -1, files, i, failed = 0;
	char path[PATH_MAX];
	gnolfh_in_t *params_in;
	char *oldest_file;
	int rc = 0;
	
	TRACE_ENTER();
	
	params_in = (gnolfh_in_t *) indata;
	oldest_file = (char *) outdata;
	
	/* Initialize the filter */
	n = snprintf(file_prefix, SA_MAX_NAME_LENGTH, "%s", params_in->file_name);
	if (n >= SA_MAX_NAME_LENGTH) {
		rc = -1;
		LOG_WA("file_prefix > SA_MAX_NAME_LENGTH");
		goto done_exit;
	}

	n = snprintf(path, PATH_MAX, "%s/%s",
			params_in->logsv_root_dir, params_in->pathName);
	if (n >= PATH_MAX) {
		LOG_WA("path > PATH_MAX");
		rc = -1;
		goto done_exit;
	}

	files = n = scandir(path, &namelist, filter_func, alphasort);
	if (n == -1 && errno == ENOENT) {
		rc = 0;
		goto done_exit;
	}

	if (n < 0) {
		LOG_WA("scandir:%s - %s", strerror(errno), path);
		rc = -1;
		goto done_exit;
	}
	
	if (n == 0) {
		rc = files;
		goto done_exit;
	}

	while (n--) {
		TRACE_3("%s", namelist[n]->d_name);
		if (check_oldest(namelist[n]->d_name, params_in->file_name,
				 strlen(params_in->file_name), &old_date, &old_time)) {
			old_ind = n;
		} else {
			failed++;	/* wrong format */
		}
	}
	if (old_ind != -1) {
		TRACE_1("oldest: %s", namelist[old_ind]->d_name);
		n = snprintf(oldest_file, max_outsize, "%s/%s",
				path, namelist[old_ind]->d_name);
		if (n >= max_outsize) {
			LOG_WA("oldest_file > max_outsize");
			rc = -1;
			goto done_free;
		} else {
			rc = (files - failed);
		}
	} else {
		TRACE("Only file/files with wrong format found");
	}

done_free:
	/* Free scandir allocated memory */
	for (i = 0; i < files; i++)
		free(namelist[i]);
	free(namelist);

done_exit:	
	TRACE_LEAVE();
	return rc;
}
