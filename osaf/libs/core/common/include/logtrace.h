
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

/**
 * This file defines APIs for logging and tracing.
 * 
 * Logging is enabled by default and level based. Current backend for logging is
 * syslog, in the future SAF log could be used for some services (but not all -
 * boot strapping problem). Filtering is done with the mechanisms of the back
 * end e.g. the syslog settings.
 * 
 * Tracing is disabled by default and category based. Categories are or-ed into
 * the current mask setting and and-ed with the mask during filtering. Current
 * backend for tracing is file, in the future syslog could be used.
 * Filtering is done by the file back end.
 */

#ifndef LOGTRACE_H
#define LOGTRACE_H

#include <syslog.h>
#include <ncsgl_defs.h>
#ifdef  __cplusplus
extern "C" {
#endif

/* Categories */
	enum logtrace_categories {
		CAT_LOG = 0,
		CAT_TRACE,
		CAT_TRACE1,
		CAT_TRACE2,
		CAT_TRACE3,
		CAT_TRACE4,
		CAT_TRACE5,
		CAT_TRACE6,
		CAT_TRACE7,
		CAT_TRACE8,
		CAT_TRACE_ENTER,
		CAT_TRACE_LEAVE,
		CAT_MAX
	};

#define CATEGORY_ALL    0xffffffff

/**
 * logtrace_init - Initialize the logtrace system.
 * 
 * @param ident An identity string to be prepended to every message. Typically
 * set to the program name.
 * @param pathname The pathname parameter should contain a valid
 * path name for a file if tracing is to be enabled. The user must have write
 * access to that file. If the file already exist, it is appended. If the file
 * name is not valid, no tracing is performed.
 * @param mask The initial trace mask. Should be set set to zero by
 *             default (trace disabled)
 * 
 * @return int - 0 if OK, -1 otherwise
 */
extern int logtrace_init(const char *ident, const char *pathname, unsigned int mask);

/**
 * logtrace_init_daemon - Initialize the logtrace system for daemons
 * 
 * @param ident An identity string to be prepended to every message. Typically
 * set to the program name.
 * @param pathname The pathname parameter should contain a valid
 * path name for a file if tracing is to be enabled. The user must have write
 * access to that file. If the file already exist, it is appended. If the file
 * name is not valid, no tracing is performed.
 * @param tracemask The initial trace mask. Should be set set to zero by
 *             default (trace disabled)
 * @param logmask The initial log level to be set for log filtering.
 * 
 * @return int - 0 if OK, -1 otherwise
 */
extern int logtrace_init_daemon(const char *ident, const char *pathname, unsigned int tracemask, int logmask);

/**
 * trace_category_set - Set the mask used for trace filtering.
 * 
 * In libraries the category mask is typically set (to all ones) after a
 * library specific environment variable has been examined for the name of a
 * output file. The variable will not exist in a production system hence no
 * tracing will be done.
 * 
 * In daemons there is no need to call this function during initialization,
 * tracing is disabled by default. In runtime, using e.g. a signal handler the
 * daemon could call this function to enable or change traced categories.
 * 
 * @param category_mask The mask to set, 0 indicates no tracing.
 * 
 * @return int - 0 if OK, -1 otherwise
 */
extern int trace_category_set(unsigned int category_mask);

/**
 * trace_category_get - Get the current mask used for trace filtering.
 * 
 * @return int - The filtering mask value
 */
extern unsigned int trace_category_get(void);

/* internal functions, do not use directly */
extern void _logtrace_log(const char *file, unsigned int line, int priority,
		  const char *format, ...) __attribute__ ((format(printf, 4, 5)));
extern void _logtrace_trace(const char *file, unsigned int line, unsigned int category,
		    const char *format, ...) __attribute__ ((format(printf, 4, 5)));

/* LOG API. Use same levels as syslog */
#define LOG_EM(format, args...) _logtrace_log(__FILE__, __LINE__, LOG_EMERG, (format), ##args)
#define LOG_AL(format, args...) _logtrace_log(__FILE__, __LINE__, LOG_ALERT, (format), ##args)
#define LOG_CR(format, args...) _logtrace_log(__FILE__, __LINE__, LOG_CRIT, (format), ##args)
#define LOG_ER(format, args...) _logtrace_log(__FILE__, __LINE__, LOG_ERR, (format), ##args)
#define LOG_WA(format, args...) _logtrace_log(__FILE__, __LINE__, LOG_WARNING, (format), ##args)
#define LOG_NO(format, args...) _logtrace_log(__FILE__, __LINE__, LOG_NOTICE, (format), ##args)
#define LOG_IN(format, args...) _logtrace_log(__FILE__, __LINE__, LOG_INFO, (format), ##args)

/* TRACE API. */
#define TRACE(format, args...)   _logtrace_trace(__FILE__, __LINE__, CAT_TRACE, (format), ##args)
#define TRACE_1(format, args...) _logtrace_trace(__FILE__, __LINE__, CAT_TRACE1, (format), ##args)
#define TRACE_2(format, args...) _logtrace_trace(__FILE__, __LINE__, CAT_TRACE2, (format), ##args)
#define TRACE_3(format, args...) _logtrace_trace(__FILE__, __LINE__, CAT_TRACE3, (format), ##args)
#define TRACE_4(format, args...) _logtrace_trace(__FILE__, __LINE__, CAT_TRACE4, (format), ##args)
#define TRACE_5(format, args...) _logtrace_trace(__FILE__, __LINE__, CAT_TRACE5, (format), ##args)
#define TRACE_6(format, args...) _logtrace_trace(__FILE__, __LINE__, CAT_TRACE6, (format), ##args)
#define TRACE_7(format, args...) _logtrace_trace(__FILE__, __LINE__, CAT_TRACE7, (format), ##args)
#define TRACE_8(format, args...) _logtrace_trace(__FILE__, __LINE__, CAT_TRACE8, (format), ##args)
#define TRACE_ENTER()                 _logtrace_trace(__FILE__, __LINE__, CAT_TRACE_ENTER, "%s ", __FUNCTION__)
#define TRACE_ENTER2(format, args...) _logtrace_trace(__FILE__, __LINE__, CAT_TRACE_ENTER, "%s: " format, __FUNCTION__, ##args)
#define TRACE_LEAVE()                 _logtrace_trace(__FILE__, __LINE__, CAT_TRACE_LEAVE, "%s ", __FUNCTION__)
#define TRACE_LEAVE2(format, args...) _logtrace_trace(__FILE__, __LINE__, CAT_TRACE_LEAVE, "%s: " format, __FUNCTION__, ##args)

#ifdef  __cplusplus
}
#endif

#endif
