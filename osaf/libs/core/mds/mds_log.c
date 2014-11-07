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

static char *lf = NULL;

static void log_mds(const char *str);
static char process_name[MDS_MAX_PROCESS_NAME_LEN];

/*****************************************************
 Function NAME: get_process_name()
 Returns : <process_name>[<pid> or <tipc_port_ref>]
*****************************************************/
static void get_process_name(void)
{
	char pid_path[1024];
	uint32_t process_id = getpid();
	char *token, *saveptr;
	char *pid_name;

	sprintf(pid_path, "/proc/%d/cmdline", process_id);
	FILE* f = fopen(pid_path,"r");
	if(f){
		size_t size;
		size = fread(pid_path, sizeof(char), 1024, f);
		if(size>0){
			if('\n' == pid_path[size-1])
				pid_path[size-1]='\0';
		}
		fclose(f);
	}
	token = strtok_r(pid_path, "/", &saveptr);
	while( token != NULL )
	{
		pid_name = token;
		token = strtok_r(NULL, "/", &saveptr);	
	}
	snprintf(process_name, MDS_MAX_PROCESS_NAME_LEN, "%s[%d]", pid_name, process_id);
	return;
}
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

uint32_t mds_log_init(char *log_file_name, char *line_prefix)
{
	FILE *fh;
	memset(process_name, 0, MDS_MAX_PROCESS_NAME_LEN);
	get_process_name();	

	if (lf != NULL)
		return NCSCC_RC_FAILURE;

	if (strlen(log_file_name) >= MAX_MDS_FNAME_LEN)
		return NCSCC_RC_FAILURE;

	strcpy(mds_log_fname, log_file_name);

	lf = mds_log_fname;

	if ((fh = fopen(lf, "a+")) != NULL) {
		fclose(fh);
		log_mds_notify("BEGIN MDS LOGGING| PID=<%s> | ARCHW=%x|64bit=%ld\n",
				process_name, MDS_SELF_ARCHWORD, (long)MDS_WORD_SIZE_TYPE);
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
	char str[MDS_MAX_PROCESS_NAME_LEN + 32];
	int i;
	va_list ap;

	i = snprintf(str, sizeof(str), "%s CRITICAL  |", process_name);
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
	char str[MDS_MAX_PROCESS_NAME_LEN + 32];
	int i;
	va_list ap;

	i = snprintf(str, sizeof(str), "%s ERR  |", process_name); 
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
	char str[MDS_MAX_PROCESS_NAME_LEN + 32];
	int i;
	va_list ap;

	i = snprintf(str, sizeof(str), "%s NOTIFY  |", process_name);
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
	char str[MDS_MAX_PROCESS_NAME_LEN + 32];
	int i;
	va_list ap;

	i = snprintf(str, sizeof(str), "%s INFO  |", process_name);
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
	char str[MDS_MAX_PROCESS_NAME_LEN + 32];
	int i;
	va_list ap;

	i = snprintf(str, sizeof(str), "%s DBG  |", process_name);
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
	struct tm *tstamp_data, tm_info;

	if (lf != NULL && ((fp = fopen(lf, "a+")) != NULL)) {
		struct timeval tv;
		char asc_tod[128];
		char log_string[512];
		int i;

		gettimeofday(&tv, NULL);
		tzset();
		tstamp_data = localtime_r(&tv.tv_sec, &tm_info);
		osafassert(tstamp_data);

		strftime(asc_tod, sizeof(asc_tod), "%b %e %k:%M:%S", tstamp_data);
		i = snprintf(log_string, sizeof(log_string), "%s.%06ld %s",
			     asc_tod, tv.tv_usec, str);

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
