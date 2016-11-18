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

/*****************************************************************************
 * Important information
 * ---------------------
 * To prevent log service thread from "hanging" if communication with NFS is
 * not working or is very slow all functions using file I/O must run their file
 * handling in a separate thread.
 * For more information on how to do that see the following files:
 * lgs_file.h, lgs_file.c, lgs_filendl.c and lgs_filehdl.h
 * Examples can be found in file lgs_stream.c, e.g. function fileopen(...)
 */

#include "log/logd/lgs_util.h"

#include <stdlib.h>
#include <grp.h>
#include "base/osaf_time.h"

#include "log/logd/lgs.h"
#include "log/logd/lgs_file.h"
#include "log/logd/lgs_filehdl.h"
#include "base/osaf_timerfd.h"

#define ALARM_STREAM_ENV_PREFIX "ALARM"
#define NOTIFICATION_STREAM_ENV_PREFIX "NOTIFICATION"
#define SYSTEM_STREAM_ENV_PREFIX "SYSTEM"
#define LGS_CREATE_CLOSE_TIME_LEN 16
#define START_YEAR 1900

// The length of '_yyyymmdd_hhmmss_yyyymmdd_hhmmss.log' including null-termination char
#define LOG_TAIL_MAX (std::string("_yyyymmdd_hhmmss_yyyymmdd_hhmmss.log").size())

/**
 * Create config file according to spec.
 * @param root_path[in]
 * @param stream[in]
 *
 * @return -1 on error
 */
int lgs_create_config_file_h(const std::string &root_path, log_stream_t *stream) {
  lgsf_apipar_t apipar;
  lgsf_retcode_t api_rc;
  void *params_in = NULL;
  ccfh_t *header_in_p;
  size_t params_in_size;
  char *logFileFormat_p;
  char *pathname_p;

  int rc;
  uint32_t n = 0;
  std::string pathname;
  size_t logFileFormat_size = 0;
  size_t pathname_size = 0;

  TRACE_ENTER();

  /* check the existence of logsv_root_dir/pathName,
   * check that the path is safe.
   * If ok, create the path if it doesn't already exits
   */
  pathname = root_path + "/" + stream->pathName;
  if (pathname.size() >= PATH_MAX) {
    LOG_WA("logsv_root_dir + pathName > PATH_MAX");
    rc = -1;
    goto done;
  }

  if (lgs_relative_path_check_ts(pathname) == true) {
    LOG_WA("Directory path \"%s\" not allowed", pathname.c_str());
    rc = -1;
    goto done;
  }

  if (lgs_make_reldir_h(stream->pathName) != 0) {
    LOG_WA("Create directory '%s/%s' failed",
           root_path.c_str(), stream->pathName.c_str());
    rc = -1;
    goto done;
  }

  /* create absolute path for config file */
  pathname += "/" + stream->fileName + ".cfg";
  if (pathname.size() > PATH_MAX) {
    LOG_WA("Complete pathname > PATH_MAX");
    rc = -1;
    goto done;
  }
  TRACE("%s - Config file path \"%s\"", __FUNCTION__, pathname.c_str());

  /*
   * Create the configuration file.
   * Open the file, write it's content and close the file
   */

  /* Allocate memory for parameters */
  params_in_size = sizeof(ccfh_t) + (strlen(stream->logFileFormat) + 1) +
      (pathname.size() + 1);
  params_in = malloc(params_in_size);

  /* Set pointers to allocated memory */
  header_in_p = static_cast<ccfh_t *>(params_in);
  logFileFormat_p = static_cast<char *>(params_in) + sizeof(ccfh_t);
  logFileFormat_size = params_in_size - sizeof(ccfh_t);
  pathname_p = logFileFormat_p + strlen(stream->logFileFormat) + 1;
  pathname_size = logFileFormat_size - (strlen(stream->logFileFormat) + 1);

  /* Fill in in parameters */
  header_in_p->version.releaseCode = lgs_cb->log_version.releaseCode;
  header_in_p->version.majorVersion = lgs_cb->log_version.majorVersion;
  header_in_p->version.minorVersion = lgs_cb->log_version.minorVersion;
  header_in_p->logFileFormat_size = strlen(stream->logFileFormat)+1;
  header_in_p->maxLogFileSize = stream->maxLogFileSize;
  header_in_p->fixedLogRecordSize = stream->fixedLogRecordSize;
  header_in_p->maxFilesRotated = stream->maxFilesRotated;

  n = snprintf(logFileFormat_p, logFileFormat_size, "%s", stream->logFileFormat);
  if (n >= logFileFormat_size) {
    rc = -1;
    LOG_WA("Log file format string too long");
    goto done_free;
  }

  n = snprintf(pathname_p, pathname_size, "%s", pathname.c_str());
  if (n >= pathname_size) {
    rc = -1;
    LOG_WA("Path name string too long");
    goto done_free;
  }

  /* Fill in API structure */
  apipar.req_code_in = LGSF_CREATECFGFILE;
  apipar.data_in_size = params_in_size;
  apipar.data_in = params_in;
  apipar.data_out_size = 0;
  apipar.data_out = NULL;

  api_rc = log_file_api(&apipar);
  if (api_rc != LGSF_SUCESS) {
    TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
    rc = -1;
  } else {
    rc = apipar.hdl_ret_code_out;
  }

done_free:
  free(params_in);

done:
  TRACE_LEAVE2("rc = %d", rc);
  return rc;
}

/**
 * Get date and time string as mandated by SAF LOG file naming
 * Format: YYYYMMDD_HHMMSS
 *
 * @param time_in
 *        Time to format.
 *        If NULL time is fetched using osaf_clock_gettime()
 * @return char*
 */
char *lgs_get_time(time_t *time_in) {
  struct tm *timeStampData;
  static char timeStampString[LGS_CREATE_CLOSE_TIME_LEN];
  char srcString[5];
  uint32_t stringSize;
  time_t testTime;
  struct timespec testime_tspec;

  if (time_in == NULL) {
    osaf_clock_gettime(CLOCK_REALTIME, &testime_tspec);
    testTime = testime_tspec.tv_sec;
  } else {
    testTime = *time_in;
  }
  struct tm tm_info;
  timeStampData = localtime_r(&testTime, &tm_info);
  osafassert(timeStampData);

  stringSize = 5 * sizeof(char);
  snprintf(srcString, (size_t)stringSize, "%d", (timeStampData->tm_year + START_YEAR));

  strncpy(timeStampString, srcString, stringSize);

  stringSize = 3 * sizeof(char);
  snprintf(srcString, (size_t)stringSize, "%02d", (timeStampData->tm_mon + 1));

  strncat(timeStampString, srcString, stringSize);

  snprintf(srcString, (size_t)stringSize, "%02d", (timeStampData->tm_mday));

  strncat(timeStampString, srcString, stringSize);

  strncat(timeStampString, "_", (2 * sizeof(char)));

  snprintf(srcString, (size_t)stringSize, "%02d", (timeStampData->tm_hour));
  strncat(timeStampString, srcString, stringSize);

  snprintf(srcString, (size_t)stringSize, "%02d", (timeStampData->tm_min));
  strncat(timeStampString, srcString, stringSize);

  snprintf(srcString, (size_t)stringSize, "%02d", (timeStampData->tm_sec));
  strncat(timeStampString, srcString, stringSize);

  timeStampString[15] = '\0';
  return timeStampString;
}

SaTimeT lgs_get_SaTime() {
  SaTimeT logTime;
  struct timespec curtime_tspec;

  osaf_clock_gettime(CLOCK_REALTIME, &curtime_tspec);
  logTime = ((unsigned)curtime_tspec.tv_sec * 1000000000ULL) + (unsigned)curtime_tspec.tv_nsec;
  return logTime;
}

/**
 * Rename a file to include a timestamp in the name
 * @param root_path[in]
 * @param rel_path[in]
 * @param old_name[in]
 * @param time_stamp[in]
 *        Can be set to NULL but then new_name must be the complete new name
 *        including time stamps but without suffix
 * @param suffix[in]
 * @param new_name[in/out]
 *        Pointer to char string
 *        Filename of renamed file. Can be set to NULL
 *
 * @return -1 if error
 */
int lgs_file_rename_h(
    const std::string &root_path,
    const std::string &rel_path,
    const std::string &old_name,
    const std::string &time_stamp,
    const std::string &suffix,
    std::string &new_name) {
  int rc;
  std::string oldpath;
  std::string newpath;
  std::string new_name_loc;
  lgsf_apipar_t apipar;
  void *params_in_p = NULL;
  size_t params_in_size;
  lgsf_retcode_t api_rc;
  char *oldpath_in_p;
  char *newpath_in_p;
  size_t *oldpath_str_size_p;
  size_t oldpath_size = 0;
  size_t newpath_size = 0;

  TRACE_ENTER();

  oldpath = root_path + "/" + rel_path + "/" + old_name + suffix;
  if (oldpath.size() >= PATH_MAX) {
    LOG_ER("Cannot rename file, old path > PATH_MAX");
    rc = -1;
    goto done;
  }

  if (time_stamp.empty() == false) {
    new_name_loc = old_name + "_" + time_stamp + suffix;
  } else {
    new_name_loc = new_name + suffix;
  }

  newpath = root_path + "/" + rel_path + "/" + new_name_loc;
  if (newpath.size() >= PATH_MAX) {
    LOG_ER("Cannot rename file, new path > PATH_MAX");
    rc = -1;
    goto done;
  }

  if (new_name.empty() == false) {
    new_name = new_name_loc;
  }

  TRACE_4("Rename file from %s", oldpath.c_str());
  TRACE_4("              to %s", newpath.c_str());

  /* Allocate memory for parameters */
  oldpath_size = oldpath.size() + 1;
  newpath_size = newpath.size() + 1;
  params_in_size = sizeof(size_t) + oldpath_size + newpath_size;

  params_in_p = malloc(params_in_size);

  /* Fill in pointer addresses */
  oldpath_str_size_p = static_cast<size_t *>(params_in_p);
  oldpath_in_p = static_cast<char *>(params_in_p) + sizeof(size_t);
  newpath_in_p = oldpath_in_p + oldpath_size;

  /* Fill in parameters */
  *oldpath_str_size_p = oldpath_size;
  memcpy(oldpath_in_p, oldpath.c_str(), oldpath_size);
  memcpy(newpath_in_p, newpath.c_str(), newpath_size);

  /* Fill in API structure */
  apipar.req_code_in = LGSF_RENAME_FILE;
  apipar.data_in_size = params_in_size;
  apipar.data_in = params_in_p;
  apipar.data_out_size = 0;
  apipar.data_out = NULL;

  api_rc = log_file_api(&apipar);
  if (api_rc != LGSF_SUCESS) {
    TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
    rc = -1;
  } else {
    rc = apipar.hdl_ret_code_out;
  }

  free(params_in_p);

done:
  TRACE_LEAVE();
  return rc;
}

void lgs_exit(const char *msg, SaAmfRecommendedRecoveryT rec_rcvr) {
  LOG_ER("Exiting with message: %s", msg);
  (void)saAmfComponentErrorReport(lgs_cb->amf_hdl, &lgs_cb->comp_name, 0, rec_rcvr, SA_NTF_IDENTIFIER_UNUSED);
  exit(EXIT_FAILURE);
}

/****************************************************************************
 *
 * lgs_lga_entry_valid
 *
 *  Searches the cb->client_tree for an reg_id entry whos MDS_DEST equals
 *  that passed DEST and returns true if itz found.
 *
 * This routine is typically used to find the validity of the lga down rec from standby
 * LGA_DOWN_LIST as  LGA client has gone away.
 *
 ****************************************************************************/
bool lgs_lga_entry_valid(lgs_cb_t *cb, MDS_DEST mds_dest) {
  log_client_t *rp = NULL;

  rp = reinterpret_cast<log_client_t *>(ncs_patricia_tree_getnext(&cb->client_tree, NULL));

  while (rp != NULL) {
    if (m_NCS_MDS_DEST_EQUAL(&rp->mds_dest, &mds_dest)) {
      return true;
    }

    rp = reinterpret_cast<log_client_t *>(
        ncs_patricia_tree_getnext(&cb->client_tree,
                                  reinterpret_cast<uint8_t *>(&rp->client_id_net)));
  }

  return false;
}

/**
 * Send a write log ack (callback request) to a client
 *
 * @param client_id
 * @param invocation
 * @param error
 * @param mds_dest
 */
void lgs_send_write_log_ack(uint32_t client_id, SaInvocationT invocation, SaAisErrorT error, MDS_DEST mds_dest) {
  uint32_t rc;
  NCSMDS_INFO mds_info = {0};
  lgsv_msg_t msg;

  TRACE_ENTER();

  msg.type = LGSV_LGS_CBK_MSG;
  msg.info.cbk_info.type = LGSV_WRITE_LOG_CALLBACK_IND;
  msg.info.cbk_info.lgs_client_id = client_id;
  msg.info.cbk_info.inv = invocation;
  msg.info.cbk_info.write_cbk.error = error;

  mds_info.i_mds_hdl = lgs_cb->mds_hdl;
  mds_info.i_svc_id = NCSMDS_SVC_ID_LGS;
  mds_info.i_op = MDS_SEND;
  mds_info.info.svc_send.i_msg = &msg;
  mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_LGA;
  mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;
  mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
  mds_info.info.svc_send.info.snd.i_to_dest = mds_dest;

  rc = ncsmds_api(&mds_info);
  if (rc != NCSCC_RC_SUCCESS)
    LOG_NO("Failed (%u) to send of WRITE ack to: %" PRIx64, rc, mds_dest);

  TRACE_LEAVE();
}

/**
 * Free all dynamically allocated memory for a WRITE
 * @param param
 */
void lgs_free_write_log(const lgsv_write_log_async_req_t *param) {
  TRACE_ENTER();

  if (param->logRecord->logHdrType == SA_LOG_GENERIC_HEADER) {
    SaLogGenericLogHeaderT *genLogH = &param->logRecord->logHeader.genericHdr;
    free(param->logRecord->logBuffer->logBuf);
    free(param->logRecord->logBuffer);
    free(genLogH->notificationClassId);
    osaf_extended_name_free(const_cast<SaNameT *>(genLogH->logSvcUsrName));
    free((void *)genLogH->logSvcUsrName);
    free(param->logRecord);
  } else {
    SaLogNtfLogHeaderT *ntfLogH = &param->logRecord->logHeader.ntfHdr;
    free(param->logRecord->logBuffer->logBuf);
    free(param->logRecord->logBuffer);
    free(ntfLogH->notificationClassId);
    osaf_extended_name_free(ntfLogH->notifyingObject);
    free(ntfLogH->notifyingObject);
    osaf_extended_name_free(ntfLogH->notificationObject);
    free(ntfLogH->notificationObject);
    free(param->logRecord);
  }

  TRACE_LEAVE();
}

/**
 * Check if a relative ("/../") path occurs in the path.
 * Note: This function must be thread safe
 *
 * @param path
 * @return true if path is not allowed
 */
bool lgs_relative_path_check_ts(const std::string &path) {
  bool rc = false;
  int len_path = path.size();
  if (len_path >= 3) {
    if (path.find("/../") != std::string::npos ||
        path.find("/..") != std::string::npos) {
      /* /..dir/ is not allowed, however /dir../ is allowed. */
      rc = true;
    } else if (path[0] == '.' && path[1] == '.' && path[2] == '/') {
      rc = true;
    }
  }
  return rc;
}

/**
 * Create directory structure, if not already created.
 * The structure is created in the log service root directory.
 *
 * TBD: Fix with separate ticket.
 * Note: If the lgsv root directory does not exist a root directory is
 * created based on the default path in PKGLOGDIR. The path in the configuration
 * object is not updated. The reason is to make it possible for the log service
 * to work even if there is no existing root path in the file system.
 *
 * @param path, Path relative log service root directory
 * @return -1 on error
 */
int lgs_make_reldir_h(const std::string &path) {
  lgsf_apipar_t apipar;
  lgsf_retcode_t api_rc;
  int rc = -1;
  mld_in_t params_in;

  TRACE_ENTER();

  const std::string logsv_root_dir = static_cast<const char *>(
      lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));

  TRACE("logsv_root_dir \"%s\"", logsv_root_dir.c_str());
  TRACE("path \"%s\"", path.c_str());

  params_in.root_dir = const_cast<char *>(logsv_root_dir.c_str());
  params_in.rel_path = const_cast<char *>(path.c_str());
  if ((strlen(params_in.root_dir) + strlen(params_in.rel_path)) >= PATH_MAX) {
    LOG_WA("root_dir + rel_path > PATH_MAX");
    goto done;
  }

  /* Fill in API structure */
  apipar.req_code_in = LGSF_MAKELOGDIR;
  apipar.data_in_size = sizeof(mld_in_t);
  apipar.data_in = &params_in;
  apipar.data_out_size = 0;
  apipar.data_out = NULL;
  api_rc = log_file_api(&apipar);
  if (api_rc != LGSF_SUCESS) {
    TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
    rc = -1;
  } else {
    rc = apipar.hdl_ret_code_out;
  }

done:
  TRACE_LEAVE2("rc = %d",rc);
  return rc;
}

/**
 * Check if a path exists. See also stat(..)
 *
 * @param path_to_check
 * @return (-1) if not exists, 0 if exists
 */
int lgs_check_path_exists_h(const char *path_to_check) {
  lgsf_apipar_t apipar;
  lgsf_retcode_t api_rc;
  void *params_in_p;
  int rc = 0;

  TRACE_ENTER2("path \"%s\"",path_to_check);
  /* Allocate memory for parameter */
  size_t params_in_size = strlen(path_to_check)+1;
  params_in_p = malloc(params_in_size);

  /* Fill in path */
  memcpy(params_in_p, path_to_check, params_in_size);

  /* Fill in API structure */
  apipar.req_code_in = LGSF_CHECKPATH;
  apipar.data_in_size = params_in_size;
  apipar.data_in = params_in_p;
  apipar.data_out_size = 0;
  apipar.data_out = NULL;

  api_rc = log_file_api(&apipar);
  if (api_rc != LGSF_SUCESS) {
    TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
    rc = -1;
  } else {
    rc = apipar.hdl_ret_code_out;
  }

  free(params_in_p);
  TRACE_LEAVE2("rc = %d",rc);
  return rc;
}

/**
 * Get data group GID
 * @return: -1 if not set
 *          gid of data group if set.
 */
gid_t lgs_get_data_gid(char *groupname) {
  osafassert(groupname != NULL);

  if (strcmp(groupname, "") == 0){
    return -1;
  } else {
    struct group *gr = getgrnam(groupname);
    if (gr){
      return gr->gr_gid;
    } else {
      LOG_ER("Could not get group struct for %s, %s", groupname, strerror(errno));
      return -1;
    }
  }
}

/**
 * Own all log files of given stream by the data group
 *
 * @param: stream[in] : stream to own log files
 * @return: 0 on success
 *         -1 on error
 */
int lgs_own_log_files_h(log_stream_t *stream, const char *groupname) {
  lgsf_apipar_t apipar;
  lgsf_retcode_t api_rc;
  int rc = 0, n, path_len;
  std::string dir_path;
  olfbgh_t *data_in = static_cast<olfbgh_t *>(malloc(sizeof(olfbgh_t)));

  TRACE_ENTER2("stream %s", stream->name.c_str());

  /* Set in parameter dir_path */
  const std::string logsv_root_dir = static_cast<const char *>(lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));

  /* Set in parameter file_name */
  data_in->file_name = const_cast<char *>(stream->fileName.c_str());
  dir_path = logsv_root_dir + "/" + stream->pathName;
  data_in->dir_path = const_cast<char *>(dir_path.c_str());
  path_len = strlen(data_in->file_name) + strlen(data_in->dir_path);
  if (path_len > PATH_MAX) {
    LOG_WA("Path to log files > PATH_MAX");
    rc = -1;
    goto done;
  }
  /* Set in parameter groupname */
  n = snprintf(data_in->groupname, UT_NAMESIZE, "%s", groupname);
  if (n > UT_NAMESIZE) {
    LOG_WA("Group name > UT_NAMESIZE");
    rc = -1;
    goto done;
  }

  /* Fill in API structure */
  apipar.req_code_in = LGSF_OWN_LOGFILES;
  apipar.data_in_size = sizeof(olfbgh_t);
  apipar.data_in = data_in;
  apipar.data_out_size = 0;
  apipar.data_out = NULL;

  api_rc = log_file_api(&apipar);
  if (api_rc != LGSF_SUCESS) {
    TRACE("%s - API error %s",__FUNCTION__,lgsf_retcode_str(api_rc));
    rc = -1;
  } else {
    rc = apipar.hdl_ret_code_out;
  }

done:
  free(data_in);
  TRACE_LEAVE2("rc = %d",rc);
  return rc;
}

/**
 * Initiate a timer:
 * Creates a timeout timer, set timeout time and starts the timer
 * returns a file descriptor to the timer that can be used with poll()
 * See also osaf_timerfd.h
 *
 * @param timeout_s[in] Timeout time in seconds
 * @return fd  File descriptor referring to the created timer
 */
int lgs_init_timer(time_t timeout_s) {
  int fd;
  struct itimerspec lgs_cltimer;

  fd = osaf_timerfd_create(CLOCK_MONOTONIC, 0);

  /* Set timeout time. Do not use as interval timer */
  lgs_cltimer.it_interval.tv_sec = 0;
  lgs_cltimer.it_interval.tv_nsec = 0;
  /* Set timeout in seconds */
  lgs_cltimer.it_value.tv_sec = timeout_s;
  lgs_cltimer.it_value.tv_nsec = 0;

  osaf_timerfd_settime(fd, 0, &lgs_cltimer, NULL);

  return fd;
}

/**
 * Close a timer created with lgs_init_timer()
 * See also osaf_timerfd.h
 *
 * @param ufd[in]
 */
void lgs_close_timer(int ufd) {
  osaf_timerfd_close(ufd);
}

/**
 * Validate if string contains special characters or not
 *
 * @param: str [in] input string for checking
 * @return: true if there is special character, otherwise false.
 */
bool lgs_has_special_char(const std::string &str) {
  int index = 0;
  char c;

  while ((c = str[index++]) != '\0') {
    switch (c) {
      /* Unix-like system does not support slash in filename */
      case '/':
        /**
         * Unix-like system supports these characters in filename
         * but it could not support in other platforms.
         *
         * If mounting the root log dir to other platforms (e.g: Windows)
         * These characters could consider to add in as restriction.
         *
         * But for now, to avoid NBC, these ones are allowed.
         *
         */
        /* case '|': */
        /* case ';': */
        /* case ',': */
        /* case '!': */
        /* case '@': */
        /* case '#': */
        /* case '$': */
        /* case '(': */
        /* case ')': */
        /* case '<': */
        /* case '>': */
        /* case '\\': */
        /* case '"': */
        /* case '\'': */
        /* case '`': */
        /* case '~': */
        /* case '{': */
        /* case '}': */
        /* case '[': */
        /* case ']': */
        /* case '+': */
        /* case '&': */
        /* case '^': */
        /* case '?': */
        /* case '*': */
        /* case '%': */
        return true;

      default:
        break;
    }
  }
  return false;
}


/**
 * Get the maximum length of filename
 *
 * @param:none
 * @return: the maximum length of filename
 */
static size_t lgs_pathconf() {
  static size_t name_max = 0;
  static bool invoked = false;
  static std::string old_path;

  std::string dir_path = static_cast<const char *>(
      lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));

  /* To avoid invoking pathconf() unncessarily */
  if ((invoked == true) && (dir_path == old_path)) {
    return name_max;
  }

  errno = 0;
  long max = pathconf(dir_path.c_str(), _PC_NAME_MAX);

  /* Get default value if the limit is not defined, or error */
  if (max == -1) {
    max = 255;
    invoked = false;
    LOG_WA("pathconf error: %s", strerror(errno));
  }

  /* Backup the values to avoid calling pathconf() */
  name_max = max;
  old_path = dir_path;

  invoked = true;
  return name_max;
}

/**
 * Get the maximum length of a <filename> which is inputed from users
 * exlcuded tail part.
 *
 * @param: none
 * @return: the maximum length of <filename>
 */
size_t lgs_max_nlength() {
  return (lgs_pathconf() - LOG_TAIL_MAX);
}

/**
 * Check if file length is valid.
 *
 * logsv will append the tail to fileName such as file extention, open time.
 * and <fileName> is input from user (immcfg, imm.xml, log Open API),
 * so once passing the checking, logsv does not need to be worry
 * about the file name is over the limitation.

 * @param: fileName - file name for checking (excluse the tail part)
 * @return: true if file is valid, false otherwise.
 *
 */
bool lgs_is_valid_filelength(const std::string &fileName) {
  size_t filelen = fileName.size() + 1;
  size_t maxsize = lgs_pathconf();

  return ((filelen + LOG_TAIL_MAX) < maxsize);
}

/**
 * Check if the length of the path to file is valid
 * Path to file format: rootDir/pathToDir/<fileName><tail>
 *
 * @param:
 * - path: the path to log file of the stream (excluse root dir).
 * - fileName: the <fileName> of the log stream (excluse the tail part)
 * @return: true if the path is valid, false otherwise.
 */
bool lgs_is_valid_pathlength(const std::string &path,
                             const std::string &fileName,
                             const std::string &rootPath) {
  std::string rootpath;
  size_t pathlen = path.size();
  size_t filelen = fileName.size();

  if (rootPath.empty() == true) {
    rootpath = static_cast<const char *>(
        lgs_cfg_get(LGS_IMM_LOG_ROOT_DIRECTORY));
  } else {
    rootpath = rootPath;
  }
  size_t rootlen = rootpath.size() + 1;

  return ((rootlen + pathlen + filelen + LOG_TAIL_MAX) < PATH_MAX);
}

/**
 * Check if the name is valid or not.
 */
bool lgs_is_extended_name_valid(const SaNameT* name) {
  if (name == NULL) return false;
  if (osaf_is_extended_name_valid(name) == false) return false;

  SaConstStringT str = osaf_extended_name_borrow(name);
  if (strlen(str) >= kOsafMaxDnLength) return false;

  return true;
}
