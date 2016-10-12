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

#ifndef logtest_h
#define logtest_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <saLog.h>
#include <assert.h>
#include <utest.h>
#include <util.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>

#include <osaf_time.h>
#include <logtrace.h>

#define SA_LOG_CONFIGURATION_OBJECT "logConfig=1,safApp=safLogService"
#define SA_LOG_STREAM_APPLICATION1 "safLgStr=saLogApplication1"
#define SA_LOG_STREAM_APPLICATION2 "safLgStr=saLogApplication2"
#define SA_LOG_STREAM_APPLICATION3 "safLgStr=saLogApplication3"
#define DEFAULT_APP_LOG_REC_SIZE 150
#define DEFAULT_APP_LOG_FILE_SIZE 1024 * 1024
#define DEFAULT_FORMAT_EXPRESSION "@Cr @Ch:@Cn:@Cs @Cm/@Cd/@CY @Sv @Sl \"@Cb\""
#define DEFAULT_ALM_LOG_REC_SIZE 1024
#define DEFAULT_ALM_LOG_BUFFER "Alarm stream test"
#define DEFAULT_NOT_LOG_REC_SIZE 1024
#define DEFAULT_NOT_LOG_BUFFER "Notification stream test"
#define DEFAULT_NOTIFYING_OBJECT "safSu=ntfyo,safSg=ntfyo,safApp=ntfyo"
#define DEFAULT_NOTIFICATION_OBJECT "safSu=ntfo,safSg=ntfo,safApp=ntfo"
#define DEFAULT_APP_FILE_PATH_NAME "saflogtest"
#define DEFAULT_APP_FILE_NAME "saLogApplication1"
#define DEFAULT_MAX_FILE_ROTATED 4

#define LOGTST_IMM_LOG_CONFIGURATION "logConfig=1,safApp=safLogService"
#define LOGTST_IMM_LOG_RUNTIME       "logConfig=currentConfig,safApp=safLogService"
#define LOGTST_IMM_LOG_GCFG          "opensafConfigId=opensafGlobalConfig,safApp=OpenSAF"

extern SaNameT systemStreamName;
extern SaNameT alarmStreamName;
extern SaNameT notificationStreamName;
extern SaNameT globalConfig;
extern SaNameT app1StreamName;
extern SaNameT notifyingObject;
extern SaNameT notificationObject;
extern SaNameT configurationObject;
extern SaNameT saNameT_Object_256;
extern SaNameT saNameT_appstream_name_256;
extern SaVersionT logVersion;
extern SaAisErrorT rc;
extern SaLogHandleT logHandle;
extern SaLogStreamHandleT logStreamHandle;
extern SaLogCallbacksT logCallbacks;
extern SaLogBufferT alarmStreamBuffer;
extern SaLogBufferT notificationStreamBuffer;
extern SaSelectionObjectT selectionObject;
extern SaNameT logSvcUsrName;
extern SaLogRecordT genLogRecord;
extern char log_root_path[];

/* Same as system() but returns WEXITSTATUS if not -1 */
int tet_system(const char *command);

/* Vebose mode. If set some test cases will print extra information */
bool verbose_flg;
void printf_v(const char *format, ...) __attribute__ ((format(printf, 1, 2)));

/* Silent mode. If set test cases printing information will be silent
 * Only affects stdout. Do not affect default printouts for PASS/FAIL info
 */
bool silent_flg;
void printf_s(const char *format, ...) __attribute__ ((format(printf, 1, 2)));

/* Tag mode. Same as silent mode except that a TAG is printed on stdout when
 * time to take an external action. E.g. TAG_ND means stop SC nodes.
 * The tag is printed on a separate line.
 */
bool tag_flg;
void print_t(const char *format, ...) __attribute__ ((format(printf, 1, 2)));

/* Extra test cases */
void add_suite_9(void);
void add_suite_10(void);
void add_suite_11(void);
void add_suite_12(void);
void add_suite_14();
void add_suite_15();
void add_suite_16();
int get_active_sc(void);
int get_attr_value(SaNameT *inObjName, char *inAttr, void *outValue);

/* Inline time measuring functions */
typedef struct {
  struct timespec start_time;
  struct timespec end_time;
  struct timespec diff_time;
} time_meas_t;
/**
 *
 * @param tm[out]
 */
static inline void time_meas_start(time_meas_t* tm) {
  osaf_clock_gettime(CLOCK_REALTIME, &tm->start_time);
}

static inline void time_meas_log(time_meas_t* tm, char *id) {
  osaf_clock_gettime(CLOCK_REALTIME, &tm->end_time);
  osaf_timespec_subtract(&tm->end_time, &tm->start_time, &tm->diff_time);
  LOG_NO("LLDTEST3 %s [%s]\t Elapsed time %ld sec, %ld nsec",
         __FUNCTION__, id,
         tm->diff_time.tv_sec, tm->diff_time.tv_nsec);
}


static inline void time_meas_print_v(time_meas_t* tm, char *id) {
  osaf_clock_gettime(CLOCK_REALTIME, &tm->end_time);
  osaf_timespec_subtract(&tm->end_time, &tm->start_time, &tm->diff_time);
  printf_v("\n%s. Elapsed time %ld.%09ld sec\n", id,
           tm->diff_time.tv_sec, tm->diff_time.tv_nsec);
}

// Common interfaces
const char *hostname(void);
bool is_test_done_on_pl(void);
void cond_check(void);

#endif
