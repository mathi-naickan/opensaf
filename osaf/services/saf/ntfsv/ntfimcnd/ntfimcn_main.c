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

#include "ntfimcn_main.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>

#include "os_defs.h"
#include "ncs_osprm.h"
#include "logtrace.h"
#include "saf_error.h"

#include "ntfimcn_notifier.h"
#include "ntfimcn_imm.h"
#include "configmake.h"

#define NTFIMCN_DEFAULT_LOG "osafntfimcn"

enum {
	FD_IMM = 0,
	FD_TERM,
	SIZE_FDS
} NTFIMCN_FDS;

static NCS_SEL_OBJ term_sel_obj; /* Selection object for TERM signal events */

/*
 * Global parameters
 */
ntfimcn_cb_t ntfimcn_cb;

static struct pollfd fds[SIZE_FDS];
static nfds_t nfds = SIZE_FDS;
static unsigned int category_mask=0;

static void imcn_exit(void)
{
#if 0
	/* For test only */
	TRACE("Execution halted!");
	while(1) {
		sleep(10000);
	}
#else
	_Exit(EXIT_FAILURE);
#endif
}

/**
 * USR2 signal handler to enable/disable trace (toggle)
 * @param sig[in]
 */
static void sigusr2_handler(int sig)
{
	if (category_mask == 0) {
		category_mask = CATEGORY_ALL;
	} else {
		category_mask = 0;
	}

	if (trace_category_set(category_mask) == -1)
		LOG_ER("trace_category_set failed");

	if (category_mask != 0) {
		LOG_NO("osafntfimcn trace enabled");
	}
}

/**
 * TERM signal handler
 *
 * @param sig[in]
 */
static void sigterm_handler(int sig)
{
	(void) sig;
	signal(SIGTERM, SIG_IGN);
	ncs_sel_obj_ind(term_sel_obj);
}

/**
 * TERM event handler
 */
static void handle_sigterm_event(void)
{
	LOG_NO("exiting on signal %d", SIGTERM);
	exit(EXIT_SUCCESS);
}

/*
 * Exit if anything fails. This will cause ntfs to restart ntfimcn
 */
int main(int argc, char** argv)
{
	const char* logPath;
	const char* trace_label = "osafntfimcnd";
	SaAisErrorT ais_error = SA_AIS_OK;

	/*
	 * Activate Log Trace
	 */
	if ((logPath = getenv("NTFSCN_TRACE_PATHNAME"))) {
		category_mask = 0xffffffff;
	} else {
		logPath = PKGLOGDIR "/" NTFIMCN_DEFAULT_LOG;
	}

	if (logtrace_init(trace_label, logPath, category_mask) == -1)
	{
		syslog(LOG_ERR, "osafntfimcnd logtrace_init FAILED");
		/* We allow to execute anyway. */
	}
	
	/*
	 * Initiate HA state
	 */
	if (strcmp(argv[1],"active") == 0) {
		ntfimcn_cb.haState = SA_AMF_HA_ACTIVE;
	} else {
		ntfimcn_cb.haState = SA_AMF_HA_STANDBY;
	}

	/*
	 * Initiate external functionallity
	 * Exit on fail. This will cause ntfs to restart ntfimcn
	 */
	if (ntfimcn_ntf_init(&ntfimcn_cb) == NTFIMCN_INTERNAL_ERROR) {
		LOG_ER("ntfimcn_ntf_init() Fail");
		imcn_exit();
	}

	if (ntfimcn_send_lost_cm_notification()) {
		LOG_ER("send_lost_cm_notification() Fail");
		imcn_exit();
	}

	if (ntfimcn_imm_init(&ntfimcn_cb) == NTFIMCN_INTERNAL_ERROR) {
		LOG_ER("ntfimcn_imm_init() Fail");
		imcn_exit();
	}

	/* Signal for TRACE on/off */
	if (signal(SIGUSR2, sigusr2_handler) == SIG_ERR) {
		LOG_ER("signal USR2 failed: %s", strerror(errno));
		/* We allow to execute anyway. */
	}

	/* Termination signal with handler */
	if (ncs_sel_obj_create(&term_sel_obj) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_sel_obj_create failed");
		_Exit(EXIT_FAILURE);
	}

	if (signal(SIGTERM, sigterm_handler) == SIG_ERR) {
		LOG_ER("signal TERM failed: %s", strerror(errno));
		_Exit(EXIT_FAILURE);
	}

	/*
	 * Initiate polling
	 */
	fds[FD_IMM].fd = ntfimcn_cb.immSelectionObject;
	fds[FD_IMM].events = POLLIN;
	fds[FD_TERM].fd = term_sel_obj.rmv_obj;
	fds[FD_TERM].events = POLLIN;

	LOG_NO("Started");

	while (1) {
		if (poll(fds, nfds, -1) == (-1)) {
			if (errno == EINTR) {
				continue;
			}

			LOG_ER("poll Fail - %s", strerror(errno));
			imcn_exit();
		}

		if (fds[FD_TERM].revents & POLLIN) {
			handle_sigterm_event();
		}

		if (fds[FD_IMM].revents & POLLIN) {
			ais_error = saImmOiDispatch(ntfimcn_cb.immOiHandle, SA_DISPATCH_ALL);
			if (ais_error != SA_AIS_OK) {
				LOG_ER("saImmOiDispatch() Fail %s",saf_error(ais_error));
				imcn_exit();
			}
		}
	}

	return 0; /* Dummy */
}

