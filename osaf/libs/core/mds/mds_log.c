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
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************
..............................................................................
  DESCRIPTION:  MDS LOG APIs

******************************************************************************
*/

#include "mds_log.h"
#include "mds_dt2c.h"		/* Include for arch-word definitions */

static char log_line_prefix[40];
static char *lf = NULL;

static void log_mds(const char *str);

/*******************************************************************************
* Funtion Name   :    mds_log_init
*
* Purpose        :
*
* Return Value   :    None
*
*******************************************************************************/
#define MAX_MDS_FNAME_LEN  256
static char mds_log_fname[MAX_MDS_FNAME_LEN];

uns32 mds_log_init(char *log_file_name, char *line_prefix)
{
	FILE *fh;
	uns32 process_id = 0;

	process_id = (uns32)getpid();

	/* Copy the log-line-prefix */
	strncpy(log_line_prefix, line_prefix, sizeof(log_line_prefix) - 1);
	log_line_prefix[sizeof(log_line_prefix) - 1] = 0;	/* Terminate string */

	if (lf != NULL)
		return NCSCC_RC_FAILURE;

	if (strlen(log_file_name) >= MAX_MDS_FNAME_LEN)
		return NCSCC_RC_FAILURE;

	strcpy(mds_log_fname, log_file_name);

	lf = mds_log_fname;

	if ((fh = fopen(lf, "a+")) != NULL) {
		fclose(fh);
		log_mds_notify("BEGIN MDS LOGGING| PID=%d|ARCH=%d|64bit=%ld\n",
			       process_id, MDS_ARCH_TYPE, (long)MDS_WORD_SIZE_TYPE);
	}

	return NCSCC_RC_SUCCESS;
}

/*******************************************************************************
* Funtion Name   :    log_mds_critical
*
* Purpose        :
*
* Return Value   :    None
*
*******************************************************************************/
void log_mds_critical(char *fmt, ...)
{
	char str[200];
	int i;
	va_list ap;

	i = snprintf(str, sizeof(str), "CRITICAL    |");
	va_start(ap, fmt);
	vsnprintf(str + i, sizeof(str) - i, fmt, ap);
	va_end(ap);
	log_mds(str);
}

/*******************************************************************************
* Funtion Name   :    log_mds_err
*
* Purpose        :
*
* Return Value   :    None
*
*******************************************************************************/
void log_mds_err(char *fmt, ...)
{
	char str[200];
	int i;
	va_list ap;

	i = snprintf(str, sizeof(str), "ERR    |");
	va_start(ap, fmt);
	vsnprintf(str + i, sizeof(str) - i, fmt, ap);
	va_end(ap);
	log_mds(str);
}

/*******************************************************************************
* Funtion Name   :    log_mds_notify
*
* Purpose        :
*
* Return Value   :    None
*
*******************************************************************************/
void log_mds_notify(char *fmt, ...)
{
	char str[200];
	int i;
	va_list ap;

	i = snprintf(str, sizeof(str), "NOTIFY |");
	va_start(ap, fmt);
	vsnprintf(str + i, sizeof(str) - i, fmt, ap);
	va_end(ap);
	log_mds(str);
}

/*******************************************************************************
* Funtion Name   :    log_mds_info
*
* Purpose        :
*
* Return Value   :    None
*
*******************************************************************************/
void log_mds_info(char *fmt, ...)
{
	char str[200];
	int i;
	va_list ap;

	i = snprintf(str, sizeof(str), "INFO   |");
	va_start(ap, fmt);
	vsnprintf(str + i, sizeof(str) - i, fmt, ap);
	va_end(ap);
	log_mds(str);
}

/*******************************************************************************
* Funtion Name   :    log_mds_dbg
*
* Purpose        :
*
* Return Value   :    NCSCC_RC_SUCCESS
*                     NCSCC_RC_FAILURE
*
*******************************************************************************/
void log_mds_dbg(char *fmt, ...)
{
	char str[200];
	int i;
	va_list ap;

	i = snprintf(str, sizeof(str), "DBG    |");
	va_start(ap, fmt);
	vsnprintf(str + i, sizeof(str) - i, fmt, ap);
	va_end(ap);
	log_mds(str);
}

/*******************************************************************************
* Funtion Name   :    log_mds
*
* Purpose        :
*
* Return Value   :    None
*
*******************************************************************************/
static void log_mds(const char *str)
{
	FILE *fp;

	if (lf != NULL && ((fp = fopen(lf, "a+")) != NULL)) {
		struct timeval tv;
		char asc_tod[128];
		char log_string[512];
		int i;

		gettimeofday(&tv, NULL);
		strftime(asc_tod, sizeof(asc_tod), "%b %e %k:%M:%S", localtime(&tv.tv_sec));
		i = snprintf(log_string, sizeof(log_string), "%s.%06ld %s %s",
			     asc_tod, tv.tv_usec, log_line_prefix, str);

		if (i >= sizeof(log_string)) {
			i = sizeof(log_string);
			log_string[i - 1] = '\n';
		} else if (log_string[i - 1] != '\n') {
			log_string[i] = '\n';
			i++;
		}

		if (! fwrite(log_string, 1, i, fp)) {
			fclose(fp);
			return;
		}

		fclose(fp);
	}
}
