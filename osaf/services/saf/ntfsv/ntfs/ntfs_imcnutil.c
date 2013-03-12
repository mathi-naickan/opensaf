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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <logtrace.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <ncs_main_papi.h>
#include "osaf_utility.h"

#include "ntfs.h"
#include "ntfs_imcnutil.h"

#include "configmake.h"

typedef struct {
	SaAmfHAStateT ha_state;
	pid_t pid;
	pthread_t thread;
} init_params_t;

static init_params_t ipar;
pthread_mutex_t ntfimcn_mutex;

static pid_t create_imcnprocess(SaAmfHAStateT ha_state)
{
	char *start_args[3];
	pid_t i_pid = (pid_t) -1;

	TRACE_ENTER();

	/*
	 * Start the configuration notifier process. Make the process aware of
	 * current HA state.
	 */
	i_pid = fork();
	if (i_pid == -1) {
		LOG_ER("Failed to fork %s",strerror(errno));
		abort();
	}
	else if (i_pid == 0) {
		/* We are Child. Run as subprocess */
		start_args[0] = PKGLIBDIR "/" NTFIMCN_PROC_NAME;
		start_args[1] = (ha_state == SA_AMF_HA_ACTIVE)?"active":"standby";
		start_args[2] = 0;

		TRACE("Starting %s(%s %s)",start_args[0],start_args[0],start_args[1]);
		execvp(start_args[0], start_args);
		LOG_ER("Failed to start!");
		abort();
	}

	TRACE_LEAVE();
	return i_pid;
}

/**
 * Thread:
 * Start the cn process and wait for process to exit.
 * If the process exit then restart it.
 * 
 * @param _init_params[in]
 * @return
 */
void *cnsurvail_thread(void *_init_params)
{
	init_params_t *ipar = (init_params_t *) _init_params;
	pid_t pid = (pid_t) -1;
	int status = 0;
	pid_t rc;

	TRACE_ENTER();

	while(1) {
		osaf_mutex_lock_ordie(&ntfimcn_mutex);
		pid = create_imcnprocess(ipar->ha_state);
		ipar->pid = pid;
		osaf_mutex_unlock_ordie(&ntfimcn_mutex);
		
		/* Wait for child process to exit */
		do {
			rc = waitpid(ipar->pid, &status, 0);
		} while ((rc == -1) && (errno == EINTR));
		if ((rc == -1) && (errno == EINVAL)) {
			LOG_ER("waitpid returned an error %s",strerror(errno));
			abort();
		}

		int exit_rc = WIFEXITED(status);
		int exit_stat = WEXITSTATUS(status);
		TRACE("osafntfimcnd process terminated reason %s (%d)",exit_rc?"exit":"other",exit_stat);
	}
}

/**
 * Start cn process surveillance thread
 * 
 * @param ha_state[in]
 */
static void start_cnprocess(SaAmfHAStateT ha_state)
{
	int rc;

	TRACE_ENTER();

	rc = pthread_mutex_init(&ntfimcn_mutex, NULL);
	if (rc != 0) osaf_abort(rc);

	ipar.ha_state = ha_state;

	rc = pthread_create(&ipar.thread, NULL, cnsurvail_thread, (void *) &ipar);
	if (rc != 0) osaf_abort(rc);

	TRACE_LEAVE();
}


/**
 * Initialize the configuration change notifier
 *
 * @param ha_state[in]
 */
void init_ntfimcn(SaAmfHAStateT ha_state)
{
	start_cnprocess(ha_state);
}

/**
 * Kill the imcn process. Use the pid saved when the process was started
 * This will cause the imcn process to be restarted in the given state.
 * This is done only if the current state is Active or Standby and if
 * the state has changed
 *
 * @param ha_state[in]
 */
void handle_state_ntfimcn(SaAmfHAStateT ha_state)
{
	int status = 0;
	pid_t rc = (pid_t) -1;

	if ((ha_state == SA_AMF_HA_ACTIVE) ||
		(ha_state == SA_AMF_HA_STANDBY)) {
		osaf_mutex_lock_ordie(&ntfimcn_mutex);
		if (ha_state != ipar.ha_state) {
			ipar.ha_state = ha_state;
			TRACE("%s: Terminating osafntfimcnd process",__FUNCTION__);
			if (kill(ipar.pid,SIGTERM)) {
				LOG_ER("%s kill failed %s",__FUNCTION__,strerror(errno));
			}
			
			/* Wait for child process to terminate */
			do {
				rc = waitpid(ipar.pid, &status, 0);
			} while ((rc == -1) && (errno == EINTR));
			
			if ((rc == -1) && (errno == EINVAL)) {
				LOG_ER("%s: Waiting for termination of osafntfimcnd process fail: %s",
						__FUNCTION__, strerror(errno));
				abort();
			} else {
				LOG_NO("%s: osafntfimcnd process terminated. State change",
						__FUNCTION__);
			}
		} else {
			TRACE("%s: osafntfimcnd process not restarted. Already in correct state",
					__FUNCTION__);
		}
		osaf_mutex_unlock_ordie(&ntfimcn_mutex);
	}
}

/**
 * Cancel the surveillance trhead and kill the cn process.
 * Use the pid and thread id saved when the process was started
 * This will terminate the process permanently.
 *
 * @return -1 if error
 */
int stop_ntfimcn(void)
{
	void *join_ret;
	int rc = 0;
	int status = 0;
	pid_t pid = (pid_t) -1;

	rc = pthread_cancel(ipar.thread);
	if (rc != 0) osaf_abort(rc);
	rc = pthread_join(ipar.thread,&join_ret);
	if (rc != 0) osaf_abort(rc);
	rc = pthread_mutex_destroy(&ntfimcn_mutex);
	if (rc != 0) osaf_abort(rc);
	rc = kill(ipar.pid,SIGTERM);
	if (rc != 0) {
		LOG_ER("%s kill osafntfimcnd failed %s",__FUNCTION__,strerror(errno));
	}
	
	/* Wait for child process to terminate */
	do {
		pid = waitpid(ipar.pid, &status, 0);
	} while ((pid == -1) && (errno == EINTR));

	if ((pid == -1) && (errno == EINVAL)) {
		LOG_ER("%s Waiting for termination of osafntfimcnd process fail: %s",
				__FUNCTION__, strerror(errno));
		rc = -1;
	} else {
		LOG_NO("%s osafntfimcnd process terminated",__FUNCTION__);
	}
	
	return rc;
}
