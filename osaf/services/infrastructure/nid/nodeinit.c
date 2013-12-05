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
 * Author(s): Emerson Network Power
 *            Wind River Systems
 *
 */

/************************************************************************
*                                                                       *
*       Module Name:    nodeinit (Node Initialization Daemon)           *
*                                                                       *
*       Purpose:        opensafd reads following info from              *
*                       PKGSYSCONFDIR/nodeinit.conf file:               *
*                       * Application file name,with absolute path name.*
*                       * Application Name.                             *
*                       * Application Type.                             *
*                       * [OPTIONAL]: Cleanup application               *
*                       * Time-out for successful application initializ *
*                         ation.                                        *
*                       * [OPTIONAL]: Recovery action (respawn "N"      *
*                         times, Restart "X" times if respawn fails     *
*                         "N" times).                                   *
*                       * [OPTIONAL]: Input parameters for application. *
*                       * [OPTIONAL]: Input parameters for cleanup app. *
*                                                                       *
*            Spawns the services in the sequence listed in              *
*            this file,uses time-out against each service               *
*            spawned and spawn the next service only once               *
*            opensafd receives a successful initialization              *
*            notification from spawned servrice.                        *
*                                                                       *
*            opensafd invokes cleanup followed by recovery              *
*            if it receives initialize action error from                *
*            spawned service or time-out before it receives             *
*            any notification.                                          *
************************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <libgen.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <configmake.h>
#include <rda_papi.h>
#include <logtrace.h>

#include "nodeinit.h"

#define SETSIG(sa, sig, fun, flags) \
	do { \
		sa.sa_handler = fun; \
		sa.sa_flags = flags; \
		sigemptyset(&sa.sa_mask); \
		sigaction(sig,&sa,NULL); \
	} while(0)

#define NIDLOG PKGLOGDIR "/nid.log"

/* NID FIFO file descriptor */
int32_t select_fd = -1;

/* To track NID current priority */
int32_t nid_current_prio;

/* NIS FIFO handler, created by NIS. NIS is waiting for us to write DONE */
int32_t nis_fifofd = -1;

/* List to store the info of application to be spawned */
NID_CHILD_LIST spawn_list = { NULL, NULL, 0 };

/* Used to depict if we are ACTIVE/STDBY */
uint32_t role = 0;
char rolebuff[20];
char svc_name[NID_MAXSNAME];

static uint32_t spawn_wait(NID_SPAWN_INFO *servicie, char *strbuff);
int32_t fork_process(NID_SPAWN_INFO *service, char *app, char *args[], char *strbuff);
static int32_t fork_script(NID_SPAWN_INFO *service, char *app, char *args[], char *strbuff);
static int32_t fork_daemon(NID_SPAWN_INFO *service, char *app, char *args[], char *strbuff);
static void collect_param(char *, char *, char *args[]);
static char *gettoken(char **, uint32_t);
static void add2spawnlist(NID_SPAWN_INFO *);
static NID_APP_TYPE get_apptype(char *);
static uint32_t get_spawn_info(char *, NID_SPAWN_INFO *, char *);
static uint32_t parse_nodeinit_conf(char *strbuf);
static uint32_t check_process(NID_SPAWN_INFO *service);
static void cleanup(NID_SPAWN_INFO *service);
static uint32_t recovery_action(NID_SPAWN_INFO *, char *);
static uint32_t spawn_services(char *);
static void nid_sleep(uint32_t);

/* List of recovery strategies */
NID_FUNC recovery_funcs[] = { spawn_wait  };
NID_FORK_FUNC fork_funcs[] = { fork_process, fork_script, fork_daemon };

char *nid_recerr[NID_MAXREC][4] = {
	{"Trying To RESPAWN", "Could Not RESPAWN", "Succeeded To RESPAWN", "FAILED TO RESPAWN"},
	{"Trying To RESET", "Faild to RESET", "suceeded To RESET", "FAILED AFTER RESTART"}
};

/****************************************************************************
 * Name          : nid_sleep                                                *
 *                                                                          *
 * Description   : Select based sleep upto milli secs granularity           *
 *                                                                          *
 * Arguments     : time_in _msec- time to sleep for milli secs              *
 *                                                                          *
 ***************************************************************************/
void nid_sleep(uint32_t time_in_msec)
{
	struct timeval tv;

	TRACE_ENTER();

	tv.tv_sec = time_in_msec / 1000;
	tv.tv_usec = ((time_in_msec) % 1000) * 1000;

	while (select(0, 0, 0, 0, &tv) != 0)
		if (errno == EINTR)
			continue;

	TRACE_LEAVE();
}

/****************************************************************************
 * Name          : gettoken                                                 *
 *                                                                          *
 * Description   : Parse and return the string speperated by tok            *
 *                                                                          *
 * Arguments     : str - input string to be parsed                          *
 *                 tok - tokenizing charecter, in our case ':'              *
 *                                                                          *
 * Return Values : token string seperated by "tok" or NULL if nothing       *
 *                                                                          *
 ****************************************************************************/
char *gettoken(char **str, uint32_t tok)
{
	char *p, *q;

	TRACE_ENTER();

	if ((str == NULL) || (*str == 0) || (**str == '\n') || (**str == '\0'))
		return (NULL);

	p = q = *str;
	if (**str == ':') {
		*p++ = 0;
		*str = p;
		return (NULL);
	}

	while ((*p != tok) && (*p != '\n') && *p)
		p++;

	if ((*p == tok) || (*p == '\n')) {
		*p++ = 0;
		*str = p;
	}

	TRACE_LEAVE();

	return q;
}

/****************************************************************************
 * Name          : add2spawnlist                                            *
 *                                                                          *
 * Description   : Insert the childinfo into global list "spawn_list"       *
 *                 in FIFO order.                                           *
 *                                                                          *
 * Arguments     : childinfo - contains child info to be inserted into      *
 *                             spawn_list                                   *
 *                                                                          *
 ***************************************************************************/
void add2spawnlist(NID_SPAWN_INFO *childinfo)
{
	TRACE_ENTER();

	if (spawn_list.head == NULL) {
		spawn_list.head = spawn_list.tail = childinfo;
		childinfo->next = NULL;
		spawn_list.count++;
		return;
	}

	spawn_list.tail->next = childinfo;
	spawn_list.tail = childinfo;
	childinfo->next = NULL;
	spawn_list.count++;

	TRACE_LEAVE();
}

/****************************************************************************
 * Name          : get_apptype                                              *
 *                                                                          *
 * Description   : Given application type returns application internal code *
 *                                                                          *
 * Arguments     : p  - input parameter application type.                   *
 *                                                                          *
 * Return Values : Application type code/APPERR                             *
 *                                                                          *
 ***************************************************************************/
NID_APP_TYPE get_apptype(char *p)
{
	NID_APP_TYPE type = NID_APPERR;

	TRACE_ENTER();

	if (p == NULL)
		return type;

	if (*p == 'E')
		type = NID_EXEC;
	else if (*p == 'S')
		type = NID_SCRIPT;
	else if (*p == 'D')
		type = NID_DAEMON;

	TRACE_LEAVE();

	return type;
}

/****************************************************************************
 * Name          : get_spawn_info                                           *
 *                                                                          *
 * Description   : Parse one entry in PKGSYSCONFDIR/nodeinit.conf file and  *
 *                 extract the fields into "spawninfo".                     *
 *                                                                          *
 * Arguments     : srcstr - One entry in PKGSYSCONFDIR/nodeinit.conf to be  *
 *                 parsed.                                                  *
 *                 spawninfo - output buffer to fill with NID_SPAWN_INFO    *
 *                 sbuf - Buffer for returning error messages               *
 *                                                                          *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE                        *
 *                                                                          *
 ***************************************************************************/
uint32_t get_spawn_info(char *srcstr, NID_SPAWN_INFO *spawninfo, char *sbuf)
{
	char *p, *q;
	NID_PLATCONF_PARS parse_state = NID_PLATCONF_SFILE;

	TRACE_ENTER();

	p = srcstr;
	while (parse_state != NID_PLATCONF_END) {

		switch (parse_state) {
		case NID_PLATCONF_SFILE:
			if (p[0] == ':') {
				sprintf(sbuf, ": Missing application file name in file:" NID_PLAT_CONF);
				break;
			}
			q = gettoken(&p, ':');
			if (strlen(q) > NID_MAXSFILE) {
				sprintf(sbuf, ": App file name length exceeded max:%d in file"
					NID_PLAT_CONF, NID_MAXSFILE);
				break;
			}
			strcpy(spawninfo->s_name, q);
			if (spawninfo->s_name[0] != '/') {
				sprintf(sbuf, ": Not an absolute path: %s", spawninfo->s_name);
				break;
			}
			if ((p == NULL) || (*p == '\0')) {
				sprintf(sbuf, ": Missing app name in file:" NID_PLAT_CONF);
				break;
			}
			parse_state = NID_PLATCONF_SNAME;
			continue;

		case NID_PLATCONF_SNAME:
			if ((p[0] == ':') || (p[0] == '\n')) {
				sprintf(sbuf, ": Missing app name in file:" NID_PLAT_CONF);
				break;
			}
			q = gettoken(&p, ':');

			if ((q == NULL) || (*q == '\0')) {
				sprintf(sbuf, ": Null/Empty string  not a valid service Name");
				break;
			}
			if (strlen(q) > NID_MAX_SVC_NAME_LEN) {
				sprintf(sbuf, ": App name length exceeded max:%d in file", NID_MAX_SVC_NAME_LEN);
				break;
			}

			strcpy(spawninfo->serv_name, q);
			if ((p == NULL) || (*p == '\0')) {
				sprintf(sbuf, ": Missing file type in file:" NID_PLAT_CONF);
				break;
			}
			parse_state = NID_PLATCONF_APPTYPE;
			continue;

		case NID_PLATCONF_APPTYPE:
			if ((p[0] == ':') || (p[0] == '\n')) {
				sprintf(sbuf, ": Missing file type in file:" NID_PLAT_CONF);
				break;
			}
			q = gettoken(&p, ':');
			if (strlen(q) > NID_MAXAPPTYPE_LEN) {
				sprintf(sbuf, ": File type length exceeded max:%d in file"
					NID_PLAT_CONF, NID_MAXAPPTYPE_LEN);
				break;
			}
			spawninfo->app_type = get_apptype(q);
			if (spawninfo->app_type < 0) {
				sprintf(sbuf, ": Not an identified file type,\"%s\"", q);
				break;
			}
			if ((p == NULL) || (*p == '\0')) {
				if ((spawninfo->app_type == NID_SCRIPT))
					sprintf(sbuf, ": Missing cleanup script in file:" NID_PLAT_CONF);
				else if ((spawninfo->app_type == NID_EXEC) || (spawninfo->app_type == NID_DAEMON))
					sprintf(sbuf, ": Missing timeout value in file:" NID_PLAT_CONF);
				break;
			}

			parse_state = NID_PLATCONF_CLEANUP;
			continue;

		case NID_PLATCONF_CLEANUP:
			if ((p[0] == ':') || (p[0] == '\n')) {
				if ((spawninfo->app_type == NID_SCRIPT)) {
					sprintf(sbuf, ": Missing cleanup script in file:" NID_PLAT_CONF);
					break;
				} else if ((spawninfo->app_type == NID_EXEC) || (spawninfo->app_type == NID_DAEMON)) {
					spawninfo->cleanup_file[0] = '\0';
					gettoken(&p, ':');
					if ((p == NULL) || (*p == '\0')) {
						sprintf(sbuf, ": Missing timeout value in file:" NID_PLAT_CONF);
						break;
					}
					parse_state = NID_PLATCONF_TOUT;
					continue;
				}
			}
			q = gettoken(&p, ':');
			if (strlen(q) > NID_MAXSFILE) {
				sprintf(sbuf, ": Cleanup app file name length exceeded max:%d in file"
					NID_PLAT_CONF, NID_MAXSFILE);
				break;
			}

			strcpy(spawninfo->cleanup_file, q);
			if (spawninfo->cleanup_file[0] != '/') {
				sprintf(sbuf, ": Not an absolute path: %s", spawninfo->cleanup_file);
				break;
			}
			if ((p == NULL) || (*p == '\0')) {
				sprintf(sbuf, ": Missing timeout value in file:" NID_PLAT_CONF);
				break;
			}
			parse_state = NID_PLATCONF_TOUT;
			continue;

		case NID_PLATCONF_TOUT:
			if ((p[0] == ':') || (p[0] == '\n')) {
				sprintf(sbuf, ": Missing timeout value in file:" NID_PLAT_CONF);
				break;
			}
			q = gettoken(&p, ':');
			if (strlen(q) > NID_MAX_TIMEOUT_LEN) {
				sprintf(sbuf, ": Timeout field length exceeded max:%d in file"
					NID_PLAT_CONF, NID_MAX_TIMEOUT_LEN);
				break;
			}
			spawninfo->time_out = atoi(q);

			parse_state = NID_PLATCONF_PRIO;
			continue;

		case NID_PLATCONF_PRIO:
			q = gettoken(&p, ':');
			if (q == NULL) {
				spawninfo->priority = 0;
				parse_state = NID_PLATCONF_RSP;
				continue;
			} else {
				if (strlen(q) > NID_MAX_PRIO_LEN) {
					sprintf(sbuf, ": Priority field length exceeded max:%d in file"
						NID_PLAT_CONF, NID_MAX_PRIO_LEN);
					break;
				}
				spawninfo->priority = atoi(q);
				parse_state = NID_PLATCONF_RSP;
				continue;
			}

		case NID_PLATCONF_RSP:
			q = gettoken(&p, ':');
			if (q == NULL) {
				spawninfo->recovery_matrix[NID_RESPAWN].retry_count = 0;
				spawninfo->recovery_matrix[NID_RESPAWN].action = recovery_funcs[NID_RESPAWN];
				parse_state = NID_PLATCONF_RST;
				continue;
			} else {
				if (strlen(q) > NID_MAX_RESP_LEN) {
					sprintf(sbuf, ": Respawn field length exceeded max:%d in file"
						NID_PLAT_CONF, NID_MAX_RESP_LEN);
					break;
				}
				if ((*q < '0') || (*q > '9')) {
					sprintf(sbuf, ": Not a digit");
					break;
				}
				spawninfo->recovery_matrix[NID_RESPAWN].retry_count = atoi(q);
				spawninfo->recovery_matrix[NID_RESPAWN].action = recovery_funcs[NID_RESPAWN];
				parse_state = NID_PLATCONF_RST;
				continue;
			}

		case NID_PLATCONF_RST:
			q = gettoken(&p, ':');
			if (q == NULL) {
				spawninfo->recovery_matrix[NID_RESET].retry_count = 0;
				parse_state = NID_PLATCONF_SPARM;
				continue;
			} else {
				if (strlen(q) > NID_MAX_REST_LEN) {
					sprintf(sbuf, ": Restart field length exceeded max:%d in file"
						NID_PLAT_CONF, NID_MAX_REST_LEN);
					break;
				}
				if ((*q < '0') || (*q > '1')) {
					sprintf(sbuf, ": Not a valid digit");
					break;
				}
				spawninfo->recovery_matrix[NID_RESET].retry_count = atoi(q);
				parse_state = NID_PLATCONF_SPARM;
				continue;
			}

		case NID_PLATCONF_SPARM:
			q = gettoken(&p, ':');
			if (q == NULL) {
				strcpy(spawninfo->s_parameters, " ");
				spawninfo->serv_args[1] = NULL;
				spawninfo->serv_args[0] = spawninfo->s_name;
				parse_state = NID_PLATCONF_CLNPARM;
				continue;
			} else {
				if (strlen(q) > NID_MAXPARMS) {
					sprintf(sbuf, ": App param length exceeded max:%d in file"
						NID_PLAT_CONF, NID_MAXPARMS);
					break;
				}

				strncpy(spawninfo->s_parameters, q, NID_MAXPARMS);
				collect_param(spawninfo->s_parameters, spawninfo->s_name, spawninfo->serv_args);
				parse_state = NID_PLATCONF_CLNPARM;
				continue;
			}

		case NID_PLATCONF_CLNPARM:
			q = gettoken(&p, ':');
			if (q == NULL) {
				strcpy(spawninfo->cleanup_parms, " ");
				spawninfo->clnup_args[1] = NULL;
				spawninfo->clnup_args[0] = spawninfo->cleanup_file;
				parse_state = NID_PLATCONF_END;
				continue;
			} else {
				if (strlen(q) > NID_MAXPARMS) {
					sprintf(sbuf, ": App param length exceeded max:%d in file"
						NID_PLAT_CONF, NID_MAXPARMS);
					break;
				}
				strncpy(spawninfo->cleanup_parms, q, NID_MAXPARMS);
				collect_param(spawninfo->cleanup_parms, spawninfo->cleanup_file, spawninfo->clnup_args);
				parse_state = NID_PLATCONF_END;
				continue;
			}

		case NID_PLATCONF_END:
			break;
		}

		if (parse_state != NID_PLATCONF_END)
			return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : parse_nodeinit_conf                                       *
 *                                                                          *
 * Description   : Parse all the entries in PKGSYSCONFDIR/nodeinit.conf     *
 *                 file and return intermittently with lineno where parsing *
 *                 error was found.                                         *
 *                                                                          *
 * Arguments     : strbuf- To return the error message string               *
 *                                                                          *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE                        *
 *                                                                          *
 ***************************************************************************/
uint32_t parse_nodeinit_conf(char *strbuf)
{
	NID_SPAWN_INFO *childinfo;
	char buff[256], sbuf[200], *ch, *ch1;
	uint32_t lineno = 0, retry = 0;
	struct nid_resetinfo info = { {""}, -1 };
	NCS_OS_FILE plat_conf, plat_conf_close;

	TRACE_ENTER();

	plat_conf.info.open.i_file_name = NID_PLAT_CONF;
	plat_conf.info.open.i_read_write_mask = NCS_OS_FILE_PERM_READ;

	if (m_NCS_OS_FILE(&plat_conf, NCS_OS_FILE_OPEN) != NCSCC_RC_SUCCESS) {
		sprintf(strbuf, NID_PLAT_CONF " file not found\n");
		return NCSCC_RC_FAILURE;
	}

	while (fgets(buff, sizeof(buff), (FILE *)plat_conf.info.open.o_file_handle)) {
		lineno++;

		/* Skip Comments and tab spaces in the beginning */
		ch = buff;

		while (*ch == ' ' || *ch == '\t')
			ch++;

		if (*ch == '#' || *ch == '\n')
			continue;

		/* In case if we have # somewhere in this line lets truncate the string from there */
		if ((ch1 = strchr(ch, '#')) != NULL) {
			*ch1++ = '\n';
			*ch1 = '\0';
		}

		/* Allocate mem for new child info */
		while ((childinfo = (NID_SPAWN_INFO *)m_NCS_OS_MEMALLOC(sizeof(NID_SPAWN_INFO), NULL)) == NULL) {
			if (retry++ == 5) {
				sprintf(strbuf, "FAILURE: Out of memory\n");
				return NCSCC_RC_FAILURE;
			}
			nid_sleep(1000);
		}

		/* Clear the new child info struct */
		memset(childinfo, 0, sizeof(NID_SPAWN_INFO));

		/* Parse each entry in the nodeinit.conf file */
		if (get_spawn_info(ch, childinfo, sbuf) != NCSCC_RC_SUCCESS) {
			sprintf(strbuf, "%s, At: %d\n", sbuf, lineno);
			return NCSCC_RC_FAILURE;
		}

		if (strcmp(childinfo->serv_name, info.faild_serv_name) == 0)
			childinfo->recovery_matrix[NID_RESET].retry_count = info.count;

		/* Add the new child info to spawn_list */
		add2spawnlist(childinfo);
	}

	/* FIXME:Need to raise a bug  */
	/* close the node_init_conf file once it is read,
	   else this open fd will be passed to all its children */
	plat_conf_close.info.close.i_file_handle = plat_conf.info.open.o_file_handle;
	if (m_NCS_OS_FILE(&plat_conf_close, NCS_OS_FILE_CLOSE) != NCSCC_RC_SUCCESS) {
		sprintf(strbuf, "Failed to close " NID_PLAT_CONF "\n");
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : fork_daemon                                              *
 *                                                                          *
 * Description   : Creates a daemon out of noraml process                   *
 *                                                                          *
 * Arguments     : app - Application file name to be spawned                *
 *                 args - Application arguments                             *
 *                 strbuff - Return error message string, not used for now  *
 *                                                                          *
 * Return Values : Process ID of the daemon forked.                         *
 *                                                                          *
 ***************************************************************************/
int32_t fork_daemon(NID_SPAWN_INFO *service, char *app, char *args[], char *strbuff)
{
	int32_t pid = -1;
	int tmp_pid = -1;
	int32_t prio_stat = -1;
	sigset_t nmask, omask;
	struct sigaction sa;
	int i = 0, filedes[2];
	int32_t n;
	fd_set set;
	struct timeval tv;

	TRACE_ENTER();

	/* Block sigchild while forking */
	sigemptyset(&nmask);
	sigaddset(&nmask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &nmask, &omask);

	if(-1 == pipe(filedes))
		LOG_ER("Problem creating pipe: %s", strerror(errno));
	else
		fcntl(filedes[0], F_SETFL, fcntl(filedes[0], F_GETFL) | O_NONBLOCK);

	if ((pid = fork()) == 0) {
		if (nis_fifofd > 0)
			close(nis_fifofd);

		if ((tmp_pid = fork()) > 0) {
			exit(0);
		}

		/* We dont need reader open here */
		close(filedes[0]);

		SETSIG(sa, SIGPIPE, SIG_IGN, 0);

		tmp_pid = getpid();
		while (write(filedes[1], &tmp_pid, sizeof(int)) < 0) {
			if (errno == EINTR)
				continue;
			else if (errno == EPIPE) {
				LOG_ER("Reader not available to return my PID");
			} else {
				LOG_ER("Problem writing to pipe, err=%s", strerror(errno));
			}
			exit(2);
		}

		setsid();
		if(! freopen("/dev/null", "r", stdin))
			LOG_ER("freopen stdin: %s", strerror(errno));

		if(! freopen(NIDLOG, "a", stdout))
			LOG_ER("freopen stdout: %s", strerror(errno));

		if(! freopen(NIDLOG, "a", stderr))
			LOG_ER("freopen stderr: %s", strerror(errno));

		prio_stat = setpriority(PRIO_PROCESS, 0, service->priority);
		if (prio_stat < 0)
			LOG_ER("Failed setting priority for %s", service->serv_name);

		umask(022);

		/* Reset all the signals */
		for (i = 1; i < NSIG; i++)
			SETSIG(sa, i, SIG_DFL, SA_RESTART);

		execvp(app, args);

		/* Hope we never come here, incase if we are here, Lets rest in peace */
		LOG_ER("Failed to exec while creating daemon, err=%s", strerror(errno));
		exit(2);
	}

	/* We dont need writer open here */
	close(filedes[1]);

	/* Lets not block indefinitely for reading pid */
	FD_ZERO(&set);
	FD_SET(filedes[0], &set);
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	while ((n = select(filedes[0] + 1, &set, NULL, NULL, &tv)) <= 0) {
		if (n == 0) {
			LOG_ER("Writer couldn't return PID");
			close(filedes[0]);
			return tmp_pid;
		}
		if (errno == EINTR)
			continue;
		break;
	}

	while (read(filedes[0], &tmp_pid, sizeof(int)) < 0)
		if (errno == EINTR)
			continue;
		else
			break;

	close(filedes[0]);
	sigprocmask(SIG_SETMASK, &omask, NULL);

	TRACE_LEAVE();

	return tmp_pid;
}

/****************************************************************************
 * Name          : fork_script                                              *
 *                                                                          *
 * Description   : Spawns shell scripts with console on                     *
 *                                                                          *
 * Arguments     : app - Application file name to be spawned                *
 *                 args - Application arguments                             *
 *                 strbuff - Return error message string, not used for now  *
 *                                                                          *
 * Return Values : Process ID of the script forked.(not usedful).           *
 *                                                                          *
 ***************************************************************************/
int32_t fork_script(NID_SPAWN_INFO *service, char *app, char *args[], char *strbuff)
{
	int32_t pid = -1;
	int i = 0;
	int32_t prio_stat = -1;
	sigset_t nmask, omask;
	struct sigaction sa;

	TRACE_ENTER();

	/* Block sigchild while forking */
	sigemptyset(&nmask);
	sigaddset(&nmask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &nmask, &omask);

	if ((pid = fork()) == 0) {
		if (nid_is_ipcopen() == NCSCC_RC_SUCCESS)
			nid_close_ipc();

		if (nis_fifofd > 0)
			close(nis_fifofd);

		sigprocmask(SIG_SETMASK, &omask, NULL);
		setsid();
		if(! freopen("/dev/null", "r", stdin))
			LOG_ER("freopen stdin: %s", strerror(errno));

		if(! freopen(NIDLOG, "a", stdout))
			LOG_ER("freopen stdout: %s", strerror(errno));

		if(! freopen(NIDLOG, "a", stderr))
			LOG_ER("freopen stderr: %s", strerror(errno));


		prio_stat = setpriority(PRIO_PROCESS, 0, service->priority);
		if (prio_stat < 0)
			LOG_ER("Failed to set priority for %s", service->serv_name);

		/* Reset all the signals */
		for (i = 1; i < NSIG; i++)
			SETSIG(sa, i, SIG_DFL, SA_RESTART);

		SETSIG(sa, SIGPIPE, SIG_IGN, 0);

		execvp(app, args);

		/* Hope we never come here, incase if we are here, Lets rest in peace */
		LOG_ER("Failed to exec while forking script, err=%s", strerror(errno));
		exit(2);
	}

	sigprocmask(SIG_SETMASK, &omask, NULL);

	TRACE_LEAVE();

	return pid;
}

/****************************************************************************
 * Name          : fork_process                                             *
 *                                                                          *
 * Description   : Spawns shell normal unix                                 *
 *                                                                          *
 * Arguments     : app - Application file name to be spawned                *
 *                 args - Application arguments                             *
 *                 strbuff - Return error message string, not used for now  *
 *                                                                          *
 * Return Values : Process ID of the process forked.                        *
 *                                                                          *
 ***************************************************************************/
int32_t fork_process(NID_SPAWN_INFO *service, char *app, char *args[], char *strbuff)
{				/* DEL */
	int32_t pid = -1;
	int i;
	int32_t prio_stat = -1;
	sigset_t nmask, omask;
	struct sigaction sa;

	TRACE_ENTER();

	/* Block sigchild while forking. */
	sigemptyset(&nmask);
	sigaddset(&nmask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &nmask, &omask);

	if ((pid = fork()) == 0) {
		if (nid_is_ipcopen() == NCSCC_RC_SUCCESS)
			nid_close_ipc();

		if (nis_fifofd > 0)
			close(nis_fifofd);

		sigprocmask(SIG_SETMASK, &omask, NULL);
		if(! freopen("/dev/null", "r", stdin))
			LOG_ER("freopen stdin: %s", strerror(errno));

		if(! freopen(NIDLOG, "a", stdout))
			LOG_ER("freopen stdout: %s", strerror(errno));

		if(! freopen(NIDLOG, "a", stderr))
			LOG_ER("freopen stderr: %s", strerror(errno));

		if (service) {
			prio_stat = setpriority(PRIO_PROCESS, 0, service->priority);
			if (prio_stat < 0)
				LOG_ER("Failed to set priority for %s", service->serv_name);
		}

		/* Reset all the signals */
		for (i = 1; i < NSIG; i++)
			SETSIG(sa, i, SIG_DFL, SA_RESTART);

		execvp(app, args);

		/* Hope we never come here, incase if we are here, Lets rest in peace */
		LOG_ER("Failed to exec, err=%s", strerror(errno));
		exit(2);
	}
	sigprocmask(SIG_SETMASK, &omask, NULL);

	TRACE_LEAVE();

	return pid;
}

/****************************************************************************
 * Name          : collect_param                                            *
 *                                                                          *
 * Description   : Given a string of parameters, it seperates the parameters*
 *                 into array of strings.                                   *
 *                                                                          *
 * Arguments     : params - string of parameters                            *
 *                 s_name _ Application name.                               *
 *                 args   - To return an array of seperated parameter       *
 *                          strings                                         *
 *                                                                          *
 * Return Values : None.                                                    *
 *                                                                          *
 ***************************************************************************/
void collect_param(char *params, char *s_name, char *args[])
{
	uint32_t f;
	char *ptr;

	TRACE_ENTER();

	ptr = params;
	for (f = 1; f < NID_MAXARGS; f++) {
		/* Skip white space */
		while (*ptr == ' ' || *ptr == '\t')
			ptr++;
		args[f] = ptr;

		/* May be trailing space.. */
		if (*ptr == 0)
			break;

		/* Skip this `word' */
		while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '#')
			ptr++;

		/* If end-of-line, break */
		if (*ptr == '#' || *ptr == 0) {
			f++;
			*ptr = 0;
			break;
		}
		/* End word with \0 and continue */
		*ptr++ = 0;
	}

	args[f] = NULL;
	args[0] = s_name;

	TRACE_LEAVE();
}

/****************************************************************************
 * Name          : spawn_wait                                               *
 *                                                                          *
 * Description   : Spawns given service and waits for given time for        *
 *                 service to respond. Error processing is done based on    *
 *                 services response. Returns a failure if service doesent  *
 *                 respond before timeout.                                  *
 *                                                                          *
 * Arguments     : service - service details for spawning.                  *
 *                 strbuff - Buffer to return error message if any.         *
 *                                                                          *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.                       *
 *                                                                          *
 ***************************************************************************/
uint32_t spawn_wait(NID_SPAWN_INFO *service, char *strbuff)
{
	int32_t pid = -1, retry = 5;
	int32_t i = 0, n = 0;
	fd_set set;
	NCS_OS_FILE fd1;
	struct timeval tv;
	NID_FIFO_MSG reqmsg;
	char *magicno, *serv, *stat, *p;
	char buff1[100], magic_str[15];

	TRACE_ENTER();

	/* Clean previous messages in strbuff */
	strbuff[0] = '\0';

	/******************************************************
	*    Check if the service file exists only for the    *
	*    first time per service, as we may fail opening   *
	*    when we are here during recovery. because the    *
	*    process killed during cleanup might be still     *
	*    holding this file and we are trying to open here *
	*    for read and write which usually will not be     *
	*    allowed by OS reporting "file bussy" error.      *
	*    And Testing this opening for the first           *
	*    Time would serve following purposes:             *
	*    1. If the executable exists.                     *
	******************************************************/
	if (service->pid == 0) {
		fd1.info.open.i_file_name = service->s_name;
		fd1.info.open.i_read_write_mask = NCS_OS_FILE_PERM_READ;
		if (m_NCS_OS_FILE(&fd1, NCS_OS_FILE_OPEN) != NCSCC_RC_SUCCESS) {
			if (errno != ETXTBSY) {
				LOG_ER("Error while loading configuration, file=%s, serv=%s",
					NID_PLAT_CONF, service->serv_name);
				exit(EXIT_FAILURE);
			}
		} else {
			NCS_OS_FILE close_fd;
			close_fd.info.close.i_file_handle = fd1.info.open.o_file_handle;
			m_NCS_OS_FILE(&close_fd, NCS_OS_FILE_CLOSE);
		}
	}

	/* By now fifo should be open, try once in case, if its not */
	if (nid_is_ipcopen() != NCSCC_RC_SUCCESS) {
		if (nid_create_ipc(strbuff) != NCSCC_RC_SUCCESS)
			return NCSCC_RC_FAILURE;
		if (nid_open_ipc(&select_fd, strbuff) != NCSCC_RC_SUCCESS)
			return NCSCC_RC_FAILURE;
	}

	/* Fork based on the application type, executable, script or daemon */
	while (retry) {
		pid = (fork_funcs[service->app_type])
			(service, service->s_name, service->serv_args, strbuff);

		if (pid <= 0) {
			LOG_ER("Error forking %s, err=%s, pid=%d, retrying", service->s_name, strerror(errno), pid);
			retry--;
			nid_sleep(1000);
			continue;
		} else {
			break;
		}
	}

	if (retry == 0) {
		LOG_ER("Unable to bring up %s, err=:%s", service->s_name, strerror(errno));
		exit(EXIT_FAILURE);
	}

	service->pid = pid;

	/* IF Everything is fine till now, wait till service notifies its initializtion status */
	FD_ZERO(&set);
	FD_SET(select_fd, &set);

	/* it's in centi sec */
	tv.tv_sec = (service->time_out) / 100;
	tv.tv_usec = ((service->time_out) % 100) * 10000;

	while ((n = select(select_fd + 1, &set, NULL, NULL, &tv)) <= 0) {
		if ((errno == EINTR) && (n < 0)) {
			FD_ZERO(&set);
			FD_SET(select_fd, &set);
			continue;
		}
		if (n == 0) {
			LOG_ER("Timed-out for response from %s", service->serv_name);
			return NCSCC_RC_FAILURE;
		}
		break;
	}

	/* Read the message from FIFO and fill in structure. */
	while ((n = read(select_fd, buff1, sizeof(buff1))) <= 0) {
		if (errno == EINTR) {
			continue;
		} else {
			sprintf(strbuff, "Failed \nError reading NID FIFO: %d", errno);
			return NCSCC_RC_FAILURE;
		}
	}

	buff1[n] = '\0';
	p = buff1;
	if ((magicno = gettoken(&p, ':')) == NULL) {
		LOG_ER("Failed missing magicno");
		return NCSCC_RC_FAILURE;
	}
	if ((serv = gettoken(&p, ':')) == NULL) {
		LOG_ER("Failed missing service name");
		return NCSCC_RC_FAILURE;
	}
	if ((stat = p) == NULL) {
		LOG_ER("Failed missing status code");
		return NCSCC_RC_FAILURE;
	}

	sprintf(magic_str, "%x", NID_MAGIC);
	for (i = 0; magicno[i] != '\0'; i++)
		magicno[i] = (char)tolower(magicno[i]);

	if (strcmp(magic_str, magicno) == 0)
		reqmsg.nid_magic_no = NID_MAGIC;
	else
		reqmsg.nid_magic_no = -1;

	if (strcmp(serv, service->serv_name) != 0) {
		sprintf(strbuff, "Failed \nReceived invalid service name received :"
			" %s sent service->serv_name : %s", serv, service->serv_name);
		return NCSCC_RC_FAILURE;
	} else {
		strcpy(reqmsg.nid_serv_name, serv);
	}

	reqmsg.nid_stat_code = atoi(stat);
	if (reqmsg.nid_magic_no != NID_MAGIC) {
		sprintf(strbuff, "Failed \nReceived invalid message: %x", reqmsg.nid_magic_no);
		return NCSCC_RC_FAILURE;
	}

	/* LOOKS LIKE CORRECT RESPONSE LETS PROCESS */
	if (strcmp(reqmsg.nid_serv_name, service->serv_name) != 0) {
		sprintf(strbuff, "Failed \nService name  mismatch! Srvc spawned: %s, Srvc code received:%s",
			service->serv_name, reqmsg.nid_serv_name);
		return NCSCC_RC_FAILURE;
	} else if (reqmsg.nid_stat_code == NCSCC_RC_SUCCESS) {
		return NCSCC_RC_SUCCESS;
	}

	if ((reqmsg.nid_stat_code > NCSCC_RC_SUCCESS)) {
		sprintf(strbuff, "Failed \n DESC:%s", service->serv_name);
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : check_process                                            *
 *                                                                          *
 * Description   : Checks if the process is still running.                  *
 *                                                                          *
 * Arguments     : service - service details.                               *
 *                                                                          *
 * Return Values : 0 - process not running.                                 *
 *                 1 - process running.                                     *
 *                                                                          *
 ***************************************************************************/
uint32_t check_process(NID_SPAWN_INFO *service)
{
	struct stat sb;
	char buf[32];

	TRACE_ENTER();

	sprintf(buf, "/proc/%d", service->pid);
	if (stat(buf, &sb) != 0)
		return 0;
	else
		return 1;

	TRACE_LEAVE();
}

/****************************************************************************
 * Name          : cleanup                                                  *
 *                                                                          *
 * Description   : - Does cleanup of the process spawned as a previous retry*
 *                 - cleanup of unix daemons and noraml process is through  *
 *                 - cleanup of services invoked by scripts is done through *
 *                   corresponding cleanup scripts.                         *
 *                                                                          *
 * Arguments     : service - service details of the service to be cleaned.  *
 *                 strbuff - Buffer to return error message if any.         *
 *                                                                          *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.                       *
 *                                                                          *
 ***************************************************************************/
void cleanup(NID_SPAWN_INFO *service)
{
	char strbuff[256];

	TRACE_ENTER();

	/* Dont Allow anyone to write to this pipe till we start recovery action */
	nid_close_ipc();
	select_fd = -1;

	if (check_process(service)) {
		LOG_ER("Sending SIGKILL to %s, pid=%d", service->serv_name, service->pid);
		kill(service->pid, SIGKILL);
	}

	if (service->cleanup_file[0] != '\0') {
		(fork_funcs[service->app_type]) (service, service->cleanup_file, service->clnup_args, strbuff);
		nid_sleep(15000);
	}

	/*******************************************************
	*    YEPPP!!!! we need to slowdown before spawning,    *
	*    cleanup task may take time before its done with   *
	*    cleaning. Spawning before cleanup may really lead *
	*    to CCHHHHAAAAOOOOOSSSSS!!!!!!!!                   *
	*******************************************************/
	nid_sleep(100);

	TRACE_LEAVE();
}

/****************************************************************************
 * Name          : recovery_action                                          *
 *                                                                          *
 * Description   : Invokes all the recovery actions in sequence according   *
 *                 to the recovery options specified in PKGSYSCONFDIR/-     *
 *                 nodeinit.conf file                                       *
 *                 It invokes recovery action for the count specified in    *
 *                 PKGSYSCONFDIR/nodeinit.conf if the recovery failes.      *
 *                                                                          *
 * Arguments     : service - service details for spawning.                  *
 *           strbuff - Buffer to return error message if any.               *
 *                                                                          *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.                       *
 *                                                                          *
 ***************************************************************************/
uint32_t recovery_action(NID_SPAWN_INFO *service, char *strbuff)
{
	uint32_t count = 0;
	NID_RECOVERY_OPT opt = NID_RESPAWN;

	TRACE_ENTER();

	while (opt != NID_RESET) {
		count = service->recovery_matrix[opt].retry_count;
		while (service->recovery_matrix[opt].retry_count > 0) {
			LOG_ER("%s %s attempt #%d", nid_recerr[opt][0],
			      service->s_name, (count - service->recovery_matrix[opt].retry_count) + 1);

           		/* Just clean the stuff we created during prev retry */
			if (service->pid != 0)
				cleanup(service);

           		/* Done with cleanup so goahead with recovery */
			if ((service->recovery_matrix[opt].action) (service, strbuff) != NCSCC_RC_SUCCESS) {
				service->recovery_matrix[opt].retry_count--;
				LOG_ER("%s %s", nid_recerr[opt][1], service->serv_name);
				LOG_ER("%s", strbuff);
				continue;
			} else {
				return NCSCC_RC_SUCCESS;
			}

		}

		if (service->recovery_matrix[opt].retry_count == 0) {
			if (count != 0)
				LOG_ER("%s", nid_recerr[opt][3]);
			opt++;
			continue;
		}
	}

	TRACE_LEAVE();

	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : spawn_services                                           *
 *                                                                          *
 * Description   : Takes the global spawn list and calls spawn_wait for     *
 *                 each service in the list, spawn wait retrns only after   *
 *                 the service is spawned successfully.                     *
 *                                                                          *
 * Arguments     : strbuff - Buffer to return error message if any.         *
 *                                                                          *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.                       *
 *                                                                          *
 ***************************************************************************/
uint32_t spawn_services(char *strbuf)
{
	NID_SPAWN_INFO *service;
	NID_CHILD_LIST sp_list = spawn_list;
	char sbuff[100];
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER();

	if (sp_list.head == NULL) {
		sprintf(strbuf, "No services to spawn\n");
		return NCSCC_RC_FAILURE;
	}

	/* Create nid fifo */
	if (nid_create_ipc(strbuf) != NCSCC_RC_SUCCESS)
		return NCSCC_RC_FAILURE;

	/* Try to open FIFO */
	if (nid_open_ipc(&select_fd, strbuf) != NCSCC_RC_SUCCESS)
		return NCSCC_RC_FAILURE;

	while (sp_list.head != NULL) {
		service = sp_list.head;
		rc = spawn_wait(service, sbuff);

		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("%s", sbuff);
			LOG_ER("Going for recovery");
			if (recovery_action(service, sbuff) != NCSCC_RC_SUCCESS) {
				exit(EXIT_FAILURE);
			}
		}

		if (strlen(sbuff) > 0)
			LOG_NO("%s", sbuff);

		sp_list.head = sp_list.head->next;
	}

	TRACE_LEAVE();
	
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : main                                                     *
 *                                                                          *
 * Description   : daemonizes the opensafd.                                 *
 *                 parses the nodeinit.conf file.                           *
 *                 invokes the services in the order mentioned in conf file *
 *                                                                          *
 * Arguments     : argc - no of command line args                           *
 *                 argv- List of arguments.                                 *
 *                                                                          *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.                       *
 *                                                                          *
 ***************************************************************************/
int main(int argc, char *argv[])
{
	char sbuf[256];
	char tracefile[NAME_MAX];

	TRACE_ENTER();

#ifdef RLIMIT_RTPRIO
	struct rlimit mylimit;
	mylimit.rlim_max = mylimit.rlim_cur = sched_get_priority_max(SCHED_RR);
	if (setrlimit(RLIMIT_RTPRIO, &mylimit) == -1)
		syslog(LOG_WARNING, "Could not set RTPRIO - %s", strerror(errno));
#endif

	openlog(basename(argv[0]), LOG_PID, LOG_LOCAL0);
	snprintf(tracefile, sizeof(tracefile), PKGLOGDIR "/%s.log", basename(argv[0]));

	if (logtrace_init(basename(argv[0]), tracefile, 0) != 0) {
		LOG_ER("Failed to init logtrace, exiting");
		exit(EXIT_FAILURE);
	}

	if (parse_nodeinit_conf(sbuf) != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed to parse " NID_PLAT_CONF " file %s, , exiting", sbuf);
		exit(EXIT_FAILURE);
	}

	if (spawn_services(sbuf) != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed while spawning service, %s, exiting", sbuf);
		exit(EXIT_FAILURE);
	}

	TRACE_LEAVE();

	return 0;
}
