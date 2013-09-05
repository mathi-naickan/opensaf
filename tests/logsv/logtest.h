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

#define SA_LOG_STREAM_APPLICATION1 "safLgStr=saLogApplication1"
#define SA_LOG_STREAM_APPLICATION2 "safLgStr=saLogApplication2"
#define SA_LOG_STREAM_APPLICATION3 "safLgStr=saLogApplication3"
#define DEFAULT_APP_LOG_REC_SIZE 128
#define DEFAULT_APP_LOG_FILE_SIZE 512
#define DEFAULT_FORMAT_EXPRESSION "@Cr @Ch:@Cn:@Cs @Cm/@Cd/@CY @Sv @Sl \"@Cb\""
//#define SA_LOG_STREAM_APPLICATION1 "safLgStr=saLogApplication1"
#define DEFAULT_ALM_LOG_REC_SIZE 1024  
#define DEFAULT_ALM_LOG_BUFFER "Alarm stream test" 
#define DEFAULT_NOT_LOG_REC_SIZE 1024  
#define DEFAULT_NOT_LOG_BUFFER "Notification stream test" 
#define DEFAULT_NOTIFYING_OBJECT "safSu=ntfyo,safSg=ntfyo,safApp=ntfyo" 
#define DEFAULT_NOTIFICATION_OBJECT "safSu=ntfo,safSg=ntfo,safApp=ntfo"
#define DEFAULT_APP_FILE_PATH_NAME "logtest"
#define DEFAULT_APP_FILE_NAME "app1"
#define DEFAULT_MAX_FILE_ROTATED 4

extern SaNameT systemStreamName;
extern SaNameT alarmStreamName;
extern SaNameT notificationStreamName;
extern SaNameT app1StreamName;
extern SaNameT notifyingObject;
extern SaNameT notificationObject;
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

#endif
