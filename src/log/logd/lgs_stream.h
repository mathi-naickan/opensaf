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

#ifndef LOG_LOGD_LGS_STREAM_H_
#define LOG_LOGD_LGS_STREAM_H_

#include <string>
#include "base/ncspatricia.h"
#include <time.h>
#include <limits.h>

#include "lgs_fmt.h"
#include "base/osaf_extended_name.h"

#define LGS_LOG_FILE_EXT ".log"
#define LGS_LOG_FILE_CONFIG_EXT ".cfg"

/**
 * Stream descriptor.
 * Contains both the IMM LOG Class attributes and implementation dependent
 * attributes like file descriptor.
 */
typedef struct log_stream {
  /* --- Corresponds to IMM Class SaLogStream/SaLogStreamConfig --- */
  std::string name;
  std::string fileName;
  std::string pathName;
  SaUint64T maxLogFileSize;
  SaUint32T fixedLogRecordSize;
  SaBoolT haProperty;     /* app log stream only */
  SaLogFileFullActionT logFullAction;
  SaUint32T logFullHaltThreshold; /* !app log stream */
  SaUint32T maxFilesRotated;
  char *logFileFormat;
  SaUint32T severityFilter;
  SaTimeT creationTimeStamp;
  SaUint32T numOpeners;
  SaUint64T filtered;     /* discarded by server due to filtering */
  /* --- end correspond to IMM Class --- */

  uint32_t streamId;      /* The unique stream id for this stream */
  int32_t fd_shared;      /* Checkpointed stream file descriptor for shared fs */
  int32_t fd_local;       /* Local stream file descriptor for split fs */
  int32_t *p_fd;      /* Points to shared or local fd depending on fs config */
  std::string logFileCurrent;     /* Current file name */
  uint32_t curFileSize;   /* Bytes written to current log file */
  uint32_t logRecordId;   /* log record indentifier increased for each record */
  SaBoolT twelveHourModeFlag; /* Not used. Can be removed? */
  logStreamTypeT streamType;
  /**
   * This info is cached locally at SCs node when stream is opened.
   * Means that it is not checkpointed to standby node (to keep BC).
   * Using this variable to avoid cost in sending request to IMM
   * to get info whether the app stream is configurable or runtime
   * when performing adm op or closing app stream.
   */
  SaBoolT isRtStream; // default value is false

  /*
   *  Checkpointed parameters used by standby in split file mode
   */
  /* Time when latest file close time stamp was created on active.
   * Seconds since Epoch.
   */
  time_t act_last_close_timestamp;

  /* Not checkpointed parameters. Used by standby in split file mode */
  uint32_t stb_logRecordId; /* Last written Id. For checking Id inconsistency */
  std::string stb_logFileCurrent; /* Current file name used on standby */
  std::string stb_prev_actlogFileCurrent; /* current file name on active when previous record was written */
  uint32_t stb_curFileSize;       /* Bytes written to current log file */
} log_stream_t;

extern uint32_t log_stream_init();
extern void log_stream_delete(log_stream_t **s);

#define STREAM_NEW -1
extern int lgs_populate_log_stream(
    const std::string &filename,
    const std::string &pathname,
    SaUint64T maxLogFileSize,
    SaUint32T fixedLogRecordSize,
    SaLogFileFullActionT logFullAction,
    SaUint32T maxFilesRotated,
    const char *logFileFormat,
    logStreamTypeT streamType,
    SaBoolT twelveHourModeFlag,
    uint32_t logRecordId,
    log_stream_t *const o_stream
                                   );

extern SaAisErrorT lgs_create_rt_appstream(log_stream_t *const rt);
extern log_stream_t *log_stream_new(const std::string &name, int stream_id);

extern void log_stream_open_fileinit(log_stream_t *stream);
extern void log_initiate_stream_files(log_stream_t *stream);

extern void log_stream_close(log_stream_t **stream, time_t *close_time);
extern int log_stream_file_close(log_stream_t *stream);
extern int log_stream_write_h(log_stream_t *stream, const char *buf, size_t count);
extern void log_stream_id_print();

#define LGS_STREAM_CREATE_FILES true
int log_stream_config_change(bool create_files_f,
                             const std::string &root_path,
                             log_stream_t *stream,
                             const std::string &current_logfile_name,
                             time_t *cur_time_in);
extern int log_file_open(const std::string &root_path,
                         log_stream_t *stream,
                         const std::string &filename,
                         int *errno_save);

/* Accessor functions */
extern void log_stream_print(log_stream_t *stream);
extern log_stream_t *log_stream_get_by_id(uint32_t id);
extern bool check_max_stream();
void log_free_stream_resources(log_stream_t *stream);
log_stream_t *iterate_all_streams(SaBoolT &end, SaBoolT jstart);
extern log_stream_t *log_stream_get_by_name(const std::string &name);

#endif  // LOG_LOGD_LGS_STREAM_H_
