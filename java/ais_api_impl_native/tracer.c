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

#include "tracer.h"

void jTrace(char *file, int line, char *format, ...)
{
	char ___tracer_string[1024];
	va_list args;

	if (file != NULL) {
		sprintf(___tracer_string, "%s:%d\t %s", basename(file), line,
			format);
	} else {
		sprintf(___tracer_string, "%s", format);
	}

	va_start(args, format);
	vprintf(___tracer_string, args);
	va_end(args);

	fflush(stdout);
}
