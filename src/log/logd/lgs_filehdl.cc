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

#include "log/logd/lgs_filehdl.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "base/logtrace.h"
#include "base/osaf_time.h"

#include "log/logd/lgs.h"

extern pthread_mutex_t lgs_ftcom_mutex; /* For locking communication */

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
int path_is_writeable_dir_hdl(void *indata, void *outdata, size_t max_outsize) {
  int is_writeable_dir = 0;
  struct stat pathstat;

  char *pathname = (char *) indata;

  TRACE_ENTER();

  TRACE("%s - pathname \"%s\"",__FUNCTION__,pathname);

  osaf_mutex_unlock_ordie(&lgs_ftcom_mutex); /* UNLOCK  Critical section */

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

  osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK after critical section */

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
int check_path_exists_hdl(void *indata, void *outdata, size_t max_outsize) {
  struct stat pathstat;
  char *path_str = (char *) indata;
  int rc = 0;

  osaf_mutex_unlock_ordie(&lgs_ftcom_mutex); /* UNLOCK  Critical section */

  rc = stat(path_str, &pathstat);
  TRACE("%s - path_str \"%s\", rc=%d",__FUNCTION__,path_str,rc);
  TRACE("%s - errno \"%s\"",__FUNCTION__,strerror(errno));

  osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK after critical section */

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
int rename_file_hdl(void *indata, void *outdata, size_t max_outsize) {
  int rc = 0;
  size_t old_path_size = *static_cast<size_t *>(indata);
  char *old_path = static_cast<char *>(indata) + sizeof(size_t);
  char *new_path = old_path + old_path_size;

  TRACE_ENTER();

  osaf_mutex_unlock_ordie(&lgs_ftcom_mutex); /* UNLOCK  Critical section */

  if ((rc = rename(old_path, new_path)) == -1)
    LOG_NO("rename: FAILED - %s", strerror(errno));

  osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK after critical section */

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
int create_config_file_hdl(void *indata, void *outdata, size_t max_outsize) {
  int rc = 0;
  FILE *filp;
  ccfh_t *params_in = static_cast<ccfh_t *>(indata);
  char *logFileFormat = static_cast<char *>(indata) + sizeof(ccfh_t);
  char *file_path = (logFileFormat + params_in->logFileFormat_size);

  TRACE_ENTER();

  TRACE("%s - file_path \"%s\"",__FUNCTION__,file_path);

  osaf_mutex_unlock_ordie(&lgs_ftcom_mutex); /* UNLOCK  Critical section */

  /* Create the config file */
  do {
    if ((filp = fopen(file_path, "w")) != NULL)
      break;
  } while (errno == EINTR);
  if (filp == NULL) {
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

  /* Close the file */
  rc = fclose(filp);
  if (rc == -1) {
    LOG_NO("Could not close \"%s\" - \"%s\"", file_path, strerror(errno));
  }

done:
  osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK after critical section */
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
int write_log_record_hdl(void *indata, void *outdata, size_t max_outsize, bool *timeout_f) {
  int rc = 0;
  uint32_t bytes_written = 0;
  off_t file_length = 0;
  wlrh_t *params_in = static_cast<wlrh_t *>(indata);
  /* Get log record pointed by lgs_rec pointer */
  char *logrecord = const_cast<char *>(static_cast<const char*>(params_in->lgs_rec));
  int *errno_out_p = static_cast<int *>(outdata);
  *errno_out_p = 0;

  TRACE_ENTER();

  osaf_mutex_unlock_ordie(&lgs_ftcom_mutex); /* UNLOCK  Critical section */

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
  osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK after critical section */

  /* If the thread was hanging and has timed out and the log record was
   * written it is invalid and shall be removed from file (log service has
   * returned SA_AIS_TRY_AGAIN).
   */
  if (*timeout_f == true) {
    LOG_NO("Timeout, removing last log record");
    file_length = lseek(params_in->fd, -((off_t)bytes_written), SEEK_END);
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

done:
  /*
    Log record memory is allocated by the caller and this memory is
    used or referred by main thread as well as log handler thread.

    In most cases, the caller thread is blocked until the log handler thread
    finishs the request (e.g: finish writing log record to file).
    Therefore, once the caller thread is unblocked, it is safe to free
    the log record memory as the log handler thread no longer uses it.

    But in time-out case, it is unsure when the log handler thread
    use the log record memory.

    To make sure there is no corruption of memory usage in case of time-out,
    We leave the log record memory freed at the end of this function.

    It is never a good idea to allocate and free memory in different places.
    But consider it as a trade-off to have a better performance of LOGsv
    as time-out occurs very rarely.
  */
  if ((*timeout_f == true) && (logrecord != NULL)) {
    free(logrecord);
    logrecord = NULL;
  }

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
int make_log_dir_hdl(void *indata, void *outdata, size_t max_outsize) {
  int rc = 0;
  int mldh_rc = 0;
  mld_in_t *params_in = static_cast<mld_in_t *>(indata);
  std::string relpath = params_in->rel_path;
  std::string rootpath = params_in->root_dir;
  std::string dir_to_make;
  std::string mpath;
  const char *spath_p = NULL;
  const char *epath_p = NULL;
  int path_len = 0;

  TRACE_ENTER();

  TRACE("rootpath \"%s\"", rootpath.c_str());
  TRACE("relpath \"%s\"", relpath.c_str());

  osaf_mutex_unlock_ordie(&lgs_ftcom_mutex); /* UNLOCK  Critical section */
  /*
   * Create root directory if it does not exists.
   * Create root path without preceding '/' and ending '/'
   * Example: ro1/ro2/ro3
   */

  /* Remove preceding '/' */
  rootpath.erase(0, rootpath.find_first_not_of('/'));
  /* Remove trailing '/' */
  rootpath.erase(rootpath.find_last_not_of('/') + 1);

  /*
   * Add relative path. Shall end with '/'
   * Example: ro1/ro2/ro3/re1/re2/re3/
   */
  if (relpath[relpath.size() - 1] != '/') {
    dir_to_make = "/" + rootpath + "/" + relpath + "/";
  } else {
    dir_to_make = "/" + rootpath + "/" + relpath;
  }

  TRACE("%s - Path to create \"%s\"", __FUNCTION__, dir_to_make.c_str());

  /* Create the path */
  spath_p = epath_p = dir_to_make.c_str();
  while ((epath_p = strchr(epath_p, '/')) != NULL) {
    if (epath_p == spath_p) {
      epath_p++;
      continue; /* Don't try to create path "/" */
    }

    epath_p++;
    path_len = epath_p - spath_p;

    mpath.assign(spath_p, path_len);
    rc = mkdir(mpath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    if ((rc != 0) && (errno != EEXIST)) {
      LOG_ER("mkdir \"%s\" Fail %s", mpath.c_str(), strerror(errno));
      mldh_rc = -1;
      goto done;
    }
  }
  TRACE("%s - Dir \"%s\" created", __FUNCTION__, mpath.c_str());

done:
  osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK after critical section */
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
int fileopen_hdl(void *indata, void *outdata, size_t max_outsize, bool *timeout_f) {
  int errno_save = 0;
  fopen_in_t *param_in = static_cast<fopen_in_t *>(indata);
  int *errno_out_p = static_cast<int *>(outdata);
  int fd;
  gid_t gid;

  TRACE_ENTER();

  TRACE("%s - filepath \"%s\"",__FUNCTION__,param_in->filepath);
  osaf_mutex_unlock_ordie(&lgs_ftcom_mutex); /* UNLOCK  Critical section  */

  gid = lgs_get_data_gid(param_in->groupname);

open_retry:
  fd = open(param_in->filepath, O_CREAT | O_RDWR | O_APPEND | O_NONBLOCK,
            S_IRUSR | S_IWUSR | S_IRGRP);

  if (fd == -1) {
    if (errno == EINTR)
      goto open_retry;
    /* save errno for caller logging */
    errno_save = errno;
    /* Do not log with higher severity here to avoid flooding the log.
     * Can be called in context of log_stream_write */
    LOG_IN("Could not open: %s - %s", param_in->filepath,
           strerror(errno));
  } else {
    if (fchown(fd, (uid_t)-1, gid) == -1){
      LOG_WA("Failed to change log file ownership, %s", strerror(errno));
    }
  }

  osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK after critical section */

  *errno_out_p = errno_save;

  /* If the file was opened but thread was hanging and has timed out than
   * opening the file is reported as failed. This means that the file has to
   * be closed again in order to not get a stray fd.
   */
  if ((*timeout_f == true) && (fd != -1)) {
    (void) close(fd);
    fd = -1;
    errno_save = 0;
  }

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
int fileclose_hdl(void *indata, void *outdata, size_t max_outsize) {
  int rc = 0;
  int *fd = static_cast<int *>(indata);

  TRACE_ENTER2("fd=%d", *fd);

  osaf_mutex_unlock_ordie(&lgs_ftcom_mutex); /* UNLOCK critical section */
  /* Flush and synchronize the file before closing to guaranty that the file
   * is not written to after it's closed
   */
  if ((rc = fdatasync(*fd)) == -1) {
    if ((errno == EROFS) || (errno == EINVAL)) {
      TRACE("Synchronization is not supported for this file");
    } else {
      LOG_NO("%s: fdatasync() error \"%s\"",__FUNCTION__, strerror(errno));
    }
  }

  /* Close the file */
  rc = close(*fd);
  if (rc == -1) {
    LOG_ER("fileclose() %s",strerror(errno));
  } else {
    // When file system is busy, operations on files will take time.
    // In that case, file handle thread will get timeout and the `requester`
    // will put the `fd` into one link list to do retry next time.
    // But if closing file succesfully, let the `requester` knows and
    // no need to send `close file request` again.
    *fd = -1;
  }

  osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK after critical section */
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
int delete_file_hdl(void *indata, void *outdata, size_t max_outsize) {
  int rc = 0;
  char *pathname = static_cast<char *>(indata);

  TRACE_ENTER();

  osaf_mutex_unlock_ordie(&lgs_ftcom_mutex); /* UNLOCK critical section */
  if ((rc = unlink(pathname)) == -1) {
    if (errno == ENOENT)
      rc = 0;
    else
      LOG_NO("could not unlink: %s - %s", pathname, strerror(errno));
  }

  osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK after critical section */

  TRACE_LEAVE();
  return rc;
}

/******************************************************************************
 * Utility functions for get_number_of_log_files_hdl
 */
static int check_oldest(char *line, char *fname_prefix, int fname_prefix_size, int *old_date, int *old_time) {
  int date, time, c, d;
  date = time = c = d = 0;
  int len = 0;
  std::string name_format;
  char time_stamps[] = "_%d_%d_%d_%d.log";

  len = strlen(time_stamps);
  len += fname_prefix_size;

  name_format = std::string(fname_prefix);
  TRACE_3("fname: %s", name_format.c_str());

  name_format = name_format + time_stamps;
  if (sscanf(line, name_format.c_str(), &date, &time, &c, &d) >= 2) {
    TRACE_3("%s: line: arg 1: %d 2: %d 3: %d 4: %d ok", __FUNCTION__,
            date, time, c, d);
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
static std::string file_prefix;

static int filter_func(const struct dirent *finfo) {
  int ret;
  int filenameLen = strlen(finfo->d_name) - strlen(".log");

  ret = strncmp(file_prefix.c_str(), finfo->d_name, file_prefix.size())
      || strcmp(finfo->d_name + filenameLen, ".log");

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
int get_number_of_log_files_hdl(void *indata, void *outdata, size_t max_outsize) {
  struct dirent **namelist;
  int n, old_date = -1, old_time = -1, old_ind = -1, files, i, failed = 0;
  std::string path;
  gnolfh_in_t *params_in;
  char *oldest_file;
  int rc = 0;

  TRACE_ENTER();

  params_in = static_cast<gnolfh_in_t *>(indata);
  oldest_file = static_cast<char *>(outdata);

  /* Initialize the filter */
  file_prefix = params_in->file_name;
  path = std::string(params_in->logsv_root_dir) + "/" + params_in->pathName;

  osaf_mutex_unlock_ordie(&lgs_ftcom_mutex); /* UNLOCK critical section */
  files = n = scandir(path.c_str(), &namelist, filter_func, alphasort);
  osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK after critical section */

  if (n == -1 && errno == ENOENT) {
    rc = 0;
    goto done_exit;
  }

  if (n < 0) {
    LOG_WA("scandir:%s - %s", strerror(errno), path.c_str());
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
      failed++;       /* wrong format */
    }
  }
  if (old_ind != -1) {
    TRACE_1("oldest: %s", namelist[old_ind]->d_name);
    n = snprintf(oldest_file, max_outsize, "%s/%s",
                 path.c_str(), namelist[old_ind]->d_name);
    if (n < 0 || static_cast<uint32_t>(n) >= max_outsize) {
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

/**
 * Change the ownership of all log files to a new group.
 * The input directory will be scanned and any file suffixed by ".log"
 * and prefixed by input file_name will be marked as log files.
 * @param indata, see olfbgh_in_t
 * @param outdata, not used
 * @param max_outsize, not used
 *
 * @return int, 0 on success or -1 if error
 */
int own_log_files_by_group_hdl(void *indata, void *outdata, size_t max_outsize) {
  struct dirent **namelist;
  int n, files, i;
  olfbgh_t *params_in;
  int rc = 0;
  gid_t gid = 0;

  TRACE_ENTER();

  params_in = static_cast<olfbgh_t *>(indata);

  /* Set file prefix filter */
  file_prefix = params_in->file_name;

  osaf_mutex_unlock_ordie(&lgs_ftcom_mutex); /* UNLOCK critical section */
  files = n = scandir(params_in->dir_path, &namelist, filter_func, alphasort);

  if (n == -1 && errno == ENOENT) {
    rc = 0;
    goto done_exit;
  }

  if (n < 0) {
    LOG_WA("scandir:%s - %s", strerror(errno), params_in->dir_path);
    rc = -1;
    goto done_exit;
  }

  if (n == 0) {
    rc = files;
    goto done_exit;
  }

  gid = lgs_get_data_gid(params_in->groupname);

  while (n--) {
    std::string file;
    file = std::string(params_in->dir_path) + "/" + namelist[n]->d_name;

    TRACE_3("%s", file.c_str());

    if (chown(file.c_str(), (uid_t) -1, gid) != 0) {
      LOG_WA("Failed to change the ownership of %s - %s", namelist[n]->d_name, strerror(errno));
    }
  }

  /* Free scandir allocated memory */
  for (i = 0; i < files; i++)
    free(namelist[i]);
  free(namelist);

done_exit:
  osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK after critical section */
  TRACE_LEAVE();
  return rc;
}

/* ****************************************************************************
 * lgs_get_file_params_hdl() with help functions
 *
 * Find the file that was current log file before server down.
 * Get file size
 * Get last written record Id by reading the first characters of the last
 * log record in the file
 *
 */

/**
 * Count the occurrences  of character <c> in a string
 * Count from end of string and stop when lim <c> is found or
 * beginning of string
 *
 * @param str[in] '\0' terminated string to count characters in
 * @param c[in]   Character to count
 * @param lim[in] Stop count when lim characters found
 * @return number of occurrences
 */
static int chr_cnt_b(char *str, char c, int lim) {
  int cnt = 0;

  if ((str == NULL) || (*str == '\0')) {
    TRACE("%s: Parameter error", __FUNCTION__);
    return 0;
  }

  char *ptr_str_end = strchr(str, '\0');
  char *ptr_str_pos = ptr_str_end - 1;

  while (1) {
    if (*ptr_str_pos == c)
      cnt++;

    /* End if beginning of file or char limit */
    if ((ptr_str_pos == str) || (cnt >= lim))
      break;

    ptr_str_pos--;
  }

  return cnt;
}

/**
 * Filter function used by scandir.
 * Find a current log file if it exist
 * - name as in file_name_find[]
 * - extension .log
 * - two timestamps (other log files has four)
 */
/* Filename prefix (no timestamps or extension */
static std::string file_name_find_g;
static int filter_logfile_name(const struct dirent *finfo) {
  bool name_found = false, ext_found = false;

  if (strstr(finfo->d_name, file_name_find_g.c_str()) != NULL)
    name_found = true;
  if (strstr(finfo->d_name, ".log") != NULL)
    ext_found = true;

  return (int) (name_found && ext_found);
}

/**
 * Get name and size of log file containing the last record Id. Is current log
 * file if not empty or last rotated if current log file is empty
 * Also give name of current log file if exist
 *
 * @param filepath_i[in]    Path to log file directory
 * @param filename_i[in]    Name prefix for file to search for
 * @param filename_o[out]   Name of file with Id. "" if no file found
 * @param curname_o[out]    Name of current log  file or "" if no current found
 * @param fsize[out]        Size of current log file
 * @return -1 on error filename_o is not valid
 */
static int filename_get(
    char *filepath_i, char *filename_i,
    std::string &filename_o, std::string &curname_o,
    uint32_t *fsize) {
  int rc = 0;
  int num_files = 0;
  int i = 0;
  struct dirent **namelist;
  struct stat statbuf;
  std::string file_path;
  double time_tmp;
  char *str_p = NULL;
  int len = 0;
  /* Time newest file */
  double time_new = 0;
  /* Index in namelist for newest and second newest file */
  int name_ix_prv = 0, name_ix_new = 0;
  /* Set to true if file is empty */
  bool empty_flg_prv = true, empty_flg_new = true;

  TRACE_ENTER();

  // /* Initiate out data */
  filename_o.clear();
  curname_o.clear();
  *fsize = 0;

  /* Create a list of all .log files that has
   * <filename_i> in <filepath_i>
   */
  file_name_find_g = filename_i;
  num_files = scandir(filepath_i, &namelist, filter_logfile_name, alphasort);
  if (num_files == -1) {
    TRACE("%s: scandir Fail %s", __FUNCTION__, strerror(errno));
    rc = -1;
    goto done;
  }

  if (num_files == 0) {
    /* There is no log file at all */
    TRACE("\t No log file at all found");
    goto done_free;
  }

  /* Special case, only one file is found
   */
  if (num_files == 1) {
    file_path = std::string(filepath_i) + "/" + namelist[0]->d_name;
    if (stat(file_path.c_str(), &statbuf) == -1) {
      TRACE("%s: stat() Fail %s", __FUNCTION__, strerror(errno));
      rc = -1;
      goto done_free;
    }

    /* Save found file name */
    filename_o = namelist[0]->d_name;

    /* Handle current log file output */
    goto done_hdl_cur;
  }

  /* Find the newest and the second newest file in the list
   * Return in filename_o name of newest file if not empty else
   * the second newest (if that also is empty return error)
   */
  time_new = time_tmp = 0;
  name_ix_prv = name_ix_new = 0;
  empty_flg_prv = empty_flg_new = false;

  for (i = 0; i < num_files; i++) {
    file_path = std::string(filepath_i) + "/" + namelist[i]->d_name;
    if (stat(file_path.c_str(), &statbuf) == -1) {
      TRACE("%s: stat() Fail %s", __FUNCTION__, strerror(errno));
      rc = -1;
      goto done_free;
    }

    time_tmp = osaf_timespec_to_double(&statbuf.st_mtim);
    if (time_tmp > time_new) {
      name_ix_prv = name_ix_new;
      name_ix_new = i;
      time_new = time_tmp;
      empty_flg_prv = empty_flg_new;
      if (statbuf.st_size == 0)
        empty_flg_new = true;
      else
        empty_flg_new = false;
    }
  }

  /* Give found filename for output */
  *fsize = 0;
  if (empty_flg_new == false) {
    /* Give the newest filename and its size. File is not empty */
    filename_o = namelist[name_ix_new]->d_name;
    *fsize = statbuf.st_size;
  } else if (empty_flg_prv == false) {
    /* Give the second newest filename. File is not empty */
    filename_o = namelist[name_ix_prv]->d_name;
  } else {
    /* Both files are empty. This is an error */
    TRACE("%s: Both newest and second newest are empy", __FUNCTION__);
    filename_o.clear();
    rc = -1;
  }

done_hdl_cur:
  /* Handle current log file output */
  len = strlen(filename_i);
  str_p = namelist[name_ix_new]->d_name + len;
  if (chr_cnt_b(str_p, '_', 4) == 2) {
    /* Newest is current log file */
    curname_o = namelist[name_ix_new]->d_name;
  }

done_free:
  /* Free namelist */
  for (i = 0; i < num_files; i++) {
    free(namelist[i]);
  }

  free(namelist);

done:
  TRACE_LEAVE();
  return rc;
}

/**
 * Get last record Id from the given file.
 * Each record begins with a record Id number. The wanted Id is the last Id
 * in the file
 *
 * @param file_name[in] Complete path to the file
 * @return record Id or -1 on error. Set to 0 if no error but no Id is found
 */
static int record_id_get(const char *file_path) {
  FILE *fd = NULL;
  int id_rc = 0;
  int i;
  int c;
  long endpos;

  char *read_line = NULL;
  size_t dummy_n = 0;
  ssize_t line_len;

  TRACE_ENTER();

  /* Open the file */
  while (1) {
    fd = fopen(file_path, "r");
    if (fd != NULL) {
      break;
    } else if (errno != EINTR){
      TRACE("%s fopen Fail %s",
            __FUNCTION__, strerror(errno));
      id_rc = -1;
      goto done;
    }
  }

  /* Get last pos in file */
  while(1) {
    if (fseek(fd, 0, SEEK_END) == -1) {
      if (errno == EINTR)
        continue;
      TRACE("\t %d fseek Fail %s",__LINE__, strerror(errno));
      id_rc = -1;
      goto done;
    }
    break;
  }
  endpos = ftell(fd);

  if (endpos == 0) {
    /* The file is empty. No Id to find */
    id_rc = 0;
    goto done;
  }

  /* Search from the end for '\n' (end of prev record)
   * or beginning of file
   */
  for (i = 2; i < endpos; i++) {
    while(1) {
      if (fseek(fd, -i, SEEK_END) == -1) {
        if (errno == EINTR)
          continue;
        TRACE("\t %d fseek Fail %s",
              __LINE__, strerror(errno));
        id_rc = -1;
        goto done;
      }
      break;
    }

    c = getc(fd);
    if (c == '\n')
      break;
  }

  /* Get pos for line start */
  c = getc(fd); /* Dummy read '\n' */

  /* Read the last record and get record id */
  line_len = getline(&read_line, &dummy_n, fd);
  if (line_len == -1) {
    TRACE("%s: getline Fail %s",
          __FUNCTION__, strerror(errno));
    id_rc = -1;
    goto done;
  }
  id_rc = atoi(read_line);
  if (id_rc == 0) {
    TRACE("%s: \"%s\" has no number. Id set to 0",
          __FUNCTION__, read_line);
    id_rc = 0;
    goto done_free;
  }

done_free:
  free(read_line);

done:
  if (fd != NULL)
    fclose(fd);

  TRACE_LEAVE();
  return id_rc;
}

/**
 * Get log file information:
 * - Name of current log file: <name prefix>_YYMMDD_HHMMSS
 * - Log record id
 *
 * Rules:
 * - If current log file not empty: Name = cur log file, Size = file size,
 *                                  Id = fr file, rc = OK
 * - If current log file empty no rotated: Name = cur log file, Size = 0,
 *                                         Id = 1, rc = OK
 * - If current log file empty is rotated: Name = cur log file, Size = 0,
 *                                         Id = fr last rotated, rc = OK
 * - If no log file at all:  Name = NULL, Size = 0, Id = 1, rc = OK
 * - If only rotated log file: Name = NULL, Size = 0, Id = fr rotated file,
 *                             rc = OK
 *
 * NOTE: This function allocates memory pointed to by par_out->curFileName.
 *       This memory has to be freed in the caller context thread.
 *
 * @param indata[in]      gfp_in_t
 *        file_name: File name prefix (name part before time stamps)
 *        file_path: Full path to directory root path + rel path
 *
 * @param outdata[out]    gfp_out_t
 *        curFileName: Current file name <name prefix>_YYMMDD_HHMMSS
 *        curFileSize: Bytes written to current log file (file size)
 *        logRecordId: log record identifier for last written log record

 * @param max_outsize[in] sizeof gfp_out_t (not used)
 *
 * @return (-1) on error, out data not valid
 */
int lgs_get_file_params_hdl(void *indata, void *outdata, size_t max_outsize) {
  gfp_in_t *par_in = static_cast<gfp_in_t *>(indata);
  gfp_out_t *par_out = static_cast<gfp_out_t *>(outdata);
  int rc = 0, int_rc = 0;
  std::string file_name;
  std::string file_name_cur;
  std::string file_path;
  int rec_id = 0;
  char *ptr_str;

  uint32_t file_size = 0;

  TRACE_ENTER();

  osaf_mutex_unlock_ordie(&lgs_ftcom_mutex); /* UNLOCK  Critical section */

  /* Initialize out parameters */
  par_out->curFileName = NULL;
  par_out->curFileSize = 0;
  par_out->logRecordId = 0;

  TRACE("file_path = %s, file_name = %s", par_in->file_path, par_in->file_name);
  /* Get log file to get info from and its size */
  int_rc = filename_get(
      par_in->file_path, par_in->file_name,
      file_name, file_name_cur, &file_size
                        );
  if (int_rc == -1) {
    TRACE("%s: filename_get Fail",__FUNCTION__);
    rc = -1;
    goto done;
  }

  if (file_name[0] == '\0') {
    TRACE("No file found.");
    par_out->logRecordId = 0;
    goto done;
  }

  /* Create the file path */
  file_path = std::string(par_in->file_path) + "/" + file_name;

  /* Find record id in file */
  rec_id = record_id_get(file_path.c_str());
  if (rec_id == -1) {
    TRACE("%s: record_id_get Fail", __FUNCTION__);
    rc = -1;
    goto done;
  }

  /* Fill in out data */
  par_out->logRecordId = rec_id;
  par_out->curFileSize = file_size;
  if (file_name_cur.empty() == false) {
    // This allocated memory will be freed by the caller
    par_out->curFileName = static_cast<char *>(
        calloc(1, file_name_cur.size() + 1));
    if (par_out->curFileName == NULL) {
      LOG_ER("%s Failed to allocate memory", __FUNCTION__);
      rc = -1;
      goto done;
    }

    strcpy(par_out->curFileName, file_name_cur.c_str());
    /* Remove extension */
    ptr_str = strstr(par_out->curFileName, ".log");
    if (ptr_str == NULL) {
      TRACE("%s: Could not find .log extension Fail", __FUNCTION__);
      free(par_out->curFileName);
      par_out->curFileName = NULL;
      rc = -1;
      goto done;
    }
    *ptr_str = '\0';
  }

done:
  osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK after critical section */
  TRACE_LEAVE2("rc = %d", rc);
  return rc;
}
