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
 * Author(s): OptXware Research & Development LLC.
 */

#ifndef TRACER_H_
#define TRACER_H_

#include <stdio.h>
#include <libgen.h>
#include <stdarg.h>

#define NDEBUG

#ifndef NDEBUG

#define _TRACE2(format, ...)					\
	jTrace(__FILE__, __LINE__, format, ## __VA_ARGS__);

#define _TRACE_FUN_ENTER						\
	jTrace(__FILE__, __LINE__, "Entering %s()\n", __FUNCTION__);
#define _TRACE_FUN_EXIT							\
	jTrace(__FILE__, __LINE__, "Exiting %s()\n", __FUNCTION__);

// format is a char* and value is the variable or value to be printed out
// Example:
//      SaAisErrorT someFunction() {
//              _TRACE_FUN_ENTER;
//              _TRACE_FUN_EXIT_RESULT("%s", "SA_AIS_OK");
//              return SA_AIS_OK;
//      }
// The output is:
//              Entering someFunction()
//              Exiting someFunction() result=SA_AIS_OK
#define _TRACE_FUN_EXIT_RESULT(format, ...)				\
	jTrace(__FILE__, __LINE__, "Exiting %s() result=" #format "\n", __FUNCTION__, ## __VA_ARGS__);

#define _TRACE_INLINE(format, ...)			\
	jTrace(NULL, 0, format, ## __VA_ARGS__);
#else
#define _TRACE2(format, ...)
#define _TRACE_FUN_ENTER
#define _TRACE_FUN_EXIT
#define _TRACE_FUN_EXIT_RESULT(format, ...)
#define _TRACE_INLINE(format, ...)
#endif				// NDEBUG

void jTrace(char *file, int line, char *format, ...);

#endif				/*TRACER_H_ */
