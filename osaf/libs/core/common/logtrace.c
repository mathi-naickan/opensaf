/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008-2010 The OpenSAF Foundation
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
 *            Wind River Systems
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <syslog.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <configmake.h>

#include "logtrace.h"

static int trace_fd = -1;
static int category_mask;
static char *prefix_name[] = { "EM", "AL", "CR", "ER", "WA", "NO", "IN", "DB",
	"TR", "T1", "T2", "T3", "T4", "T5", "T6", "T7", "T8", ">>", "<<"
};

static const char *ident;
static const char *pathname;

/**
 * USR2 signal handler to enable/disable trace (toggle)
 * @param sig
 */
static void sigusr2_handler(int sig)
{
	unsigned int trace_mask;

	if (category_mask == 0)
		trace_mask = CATEGORY_ALL;
	else
		trace_mask = 0;

	trace_category_set(trace_mask);
}

static void output(const char *file, unsigned int line, int priority, int category, const char *format, va_list ap)
{
	int i;
	struct timeval tv;
	char preamble[512];
	char log_string[1024];

	assert(priority <= LOG_DEBUG && category < CAT_MAX);

	/* Create a nice syslog looking date string */
	gettimeofday(&tv, NULL);
	strftime(log_string, sizeof(log_string), "%b %e %k:%M:%S", localtime(&tv.tv_sec));
	i = snprintf(preamble, sizeof(preamble), "%s.%06ld %s ", log_string, tv.tv_usec, ident);

	snprintf(&preamble[i], sizeof(preamble) - i, "[%d:%s:%04u] %s %s",
		getpid(), file, line, prefix_name[priority + category], format);
	i = vsnprintf(log_string, sizeof(log_string), preamble, ap);

	/* Add line feed if not there already */
	if (log_string[i - 1] != '\n') {
		log_string[i] = '\n';
		log_string[i + 1] = '\0';
		i++;
	}

	/* If we got here without a file descriptor, trace was enabled in runtime, open the file */
	if (trace_fd == -1) {
		trace_fd = open(pathname, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (trace_fd < 0) {
			syslog(LOG_ERR, "logtrace: open failed, file=%s (%s)", pathname, strerror(errno));
			return;
		}
	}

write_retry:
	i = write(trace_fd, log_string, i);
	if (i == -1) {
		if (errno == EAGAIN)
			goto write_retry;
		else
			syslog(LOG_ERR, "logtrace: write failed, %s", strerror(errno));
	}
}

void _logtrace_log(const char *file, unsigned int line, int priority, const char *format, ...)
{
	va_list ap;
	va_list ap2;

	/* Uncondionally send to syslog */
	va_start(ap, format);
	va_copy(ap2, ap);
	vsyslog(priority, format, ap);

	/* Only output to file if configured to */
	if (!(category_mask & (1 << CAT_LOG)))
		goto done;

	output(file, line, priority, CAT_LOG, format, ap2);

done:
	va_end(ap);
	va_end(ap2);
}

void _logtrace_trace(const char *file, unsigned int line, unsigned int category, const char *format, ...)
{
	va_list ap;

	/* Filter on category */
	if (!(category_mask & (1 << category)))
		return;

	va_start(ap, format);
	output(file, line, LOG_DEBUG, category, format, ap);
	va_end(ap);
}

int logtrace_init(const char *_ident, const char *_pathname, unsigned int mask)
{
	ident = _ident;
	pathname = strdup(_pathname);
	category_mask = mask;

	if (mask != 0) {
		trace_fd = open(pathname, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (trace_fd < 0) {
			syslog(LOG_ERR, "logtrace: open failed, file=%s (%s)", pathname, strerror(errno));
			return -1;
		}

		syslog(LOG_INFO, "logtrace: trace enabled to file %s, mask=0x%x", pathname, category_mask);
	}

	/* Only install signal for opensaf daemons */
	if (strncmp(PKGLOGDIR, _pathname, strlen(PKGLOGDIR)) == 0) {
		if (signal(SIGUSR2, sigusr2_handler) == SIG_ERR) {
			syslog(LOG_ERR, "logtrace: registering trace toggle SIGUSR2 failed, (%s)", strerror(errno));
			return -1;
		}
	}

	return 0;
}

int trace_category_set(unsigned int mask)
{
	category_mask = mask;

	if (category_mask == 0) {
		if (trace_fd != -1) {
			(void) close(trace_fd);
			trace_fd = -1;
		}
		syslog(LOG_INFO, "logtrace: trace disabled");
	}
	else
		syslog(LOG_INFO, "logtrace: trace enabled to file %s, mask=0x%x", pathname, category_mask);

	return 0;
}

unsigned int trace_category_get(void)
{
	return category_mask;
}
