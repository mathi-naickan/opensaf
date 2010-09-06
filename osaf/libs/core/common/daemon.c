/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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
 * Author(s): Wind River Systems
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>
#include <pwd.h>
#include <getopt.h>
#include <libgen.h>
#include <unistd.h>
#include <utmp.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <configmake.h>

#include "daemon.h"
#include "logtrace.h"

#include "ncsgl_defs.h"


#define DEFAULT_RUNAS_USERNAME	"opensaf"

static char __pidfile[NAME_MAX];
static char __tracefile[NAME_MAX];
static char __runas_username[UT_NAMESIZE];
static unsigned int tracemask;

static void __print_usage(const char* progname, FILE* stream, int exit_code)
{
	fprintf(stream, "Usage:  %s [OPTIONS]...\n", progname);
	fprintf(stream,
		"  -h, --help                  Display this usage information.\n"
		"  -m, --tracemask[=mask]      Set daemon TRACE mask.\n"
		"  -p, --pidfile[=filename]    Set the daemon PID as [filename].\n"
		"                              Will be stored under " PKGPIDDIR "\n"
		"  -T, --tracefile[=filename]  Set the daemon TRACE as [filename].\n"
		"  -U, --run-as[=username]     Execute the daemon as [username].\n"
		"  -v, --version               Print daemon version.\n");
	exit(exit_code);
}

static int __create_pidfile(const char* pidfile)
{
	FILE *file = NULL;
	int fd, pid, rc = 0;

	/* open the file and associate a stream with it */
	if ( ((fd = open(pidfile, O_RDWR|O_CREAT, 0644)) == -1)
			|| ((file = fdopen(fd, "r+")) == NULL) ) { 
		syslog(LOG_ERR, "open failed, pidfile=%s, errno=%s", pidfile, strerror(errno));
		rc = -1;
	}

	/* Lock the file */
	if (flock(fd, LOCK_EX|LOCK_NB) == -1) {
		syslog(LOG_ERR, "flock failed, pidfile=%s, errno=%s", pidfile, strerror(errno));
		fclose(file);
		rc = -1;
	}

	pid = getpid();
	if (!fprintf(file,"%d\n", pid)) {
		syslog(LOG_ERR, "fprintf failed, pidfile=%s, errno=%s", pidfile, strerror(errno));
		fclose(file);
		rc = -1;
	}
	fflush(file);

	if (flock(fd, LOCK_UN) == -1) {
		syslog(LOG_ERR, "flock failed, pidfile=%s, errno=%s", pidfile, strerror(errno));
		fclose(file);
		rc = -1;
	}
	fclose(file);

	return rc;
}

static void __set_default_options(const char *progname)
{
	/* Set the default option values */
	snprintf(__pidfile, sizeof(__pidfile), PKGPIDDIR "/%s.pid", progname);
	snprintf(__tracefile, sizeof(__tracefile), PKGLOGDIR "/%s", progname);
	snprintf(__runas_username, sizeof(__runas_username), DEFAULT_RUNAS_USERNAME);
}

static void __parse_options(int argc, char *argv[])
{
	int next_option;
	const char *progname = basename(argv[0]);

	/* Set the default option values before parsing args */
	__set_default_options(progname);

	/* A string listing valid short options letters */
	const char* const short_options = "hm:p:T:U:v";
	
	/* An array describing valid long options */
	const struct option long_options[] = {
		{ "help",      0, NULL, 'h' },
		{ "tracemask", 1, NULL, 'm' },
		{ "pidfile",   1, NULL, 'p' },
		{ "tracefile", 1, NULL, 'T' },
		{ "run-as",    1, NULL, 'U' },
		{ "version",   0, NULL, 'v' },
		{ NULL,        0, NULL,  0  }	/* Required at end of array */
	};

	do {
		next_option = getopt_long(argc, argv, short_options, long_options, NULL);
		
		switch(next_option) {
		case 'h':	/* -h or --help */
			__print_usage(progname, stdout, EXIT_SUCCESS);
		case 'm':	/* -m or --tracemask */
			tracemask = strtoul(optarg, NULL, 0);
			break;
		case 'p':	/* -p or --pidfile */
			snprintf(__pidfile, sizeof(__pidfile), PKGPIDDIR "/%s", optarg);
			break;
		case 'T':	/* -T or --tracefile */
			snprintf(__tracefile, sizeof(__tracefile), "%s", optarg);
			break;
		case 'U':	/* -U or --run-as */
			snprintf(__runas_username, sizeof(__runas_username), "%s", optarg);
			break;
		case 'v':	/* -v or --version */
			/* TODO */
			break;
		case '?':	/* The user specified an invalid option */
			__print_usage(progname, stderr, EXIT_FAILURE);
		case EOF:	/* Done with options. */
			break;
		default:	/* Something else: unexpected */
			abort();
		}
	} while (next_option != EOF);
}

void daemonize(int argc, char *argv[])
{
	pid_t pid, sid;

	if (argc > 0 && argv != NULL) {	
		__parse_options(argc, argv);
		openlog(basename(argv[0]), LOG_PID, LOG_LOCAL0);
	} else {
		syslog(LOG_ERR, "invalid top argc/argv[] passed to daemonize()");
		exit(EXIT_FAILURE);
	}

	/* Already a daemon */
	if (getppid() == 1) return;

	/* Fork off the parent process */
	pid = fork();
	if (pid < 0) {
		syslog(LOG_ERR, "fork daemon failed, pid=%d (%s)", pid, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* If we got a good PID, then we can exit the parent process */
	if (pid > 0) exit(EXIT_SUCCESS);

	/* Create a new SID for the child process */
	sid = setsid();
	if (sid < 0) {
		syslog(LOG_ERR, "create new session failed, sid=%d (%s)", sid, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Redirect standard files to /dev/null */
	if (freopen("/dev/null", "r", stdin) == NULL)
		syslog(LOG_ERR, "freopen stdin failed - %s", strerror(errno));
	if (freopen("/dev/null", "w", stdout) == NULL)
		syslog(LOG_ERR, "freopen stdout failed - %s", strerror(errno));
	if (freopen("/dev/null", "w", stderr) == NULL)
		syslog(LOG_ERR, "freopen stderr failed - %s", strerror(errno));

	/* Change the file mode mask to 0644 */
	umask(022);

	/*
	 * Change the current working directory.  This prevents the current
	 * directory from being locked; hence not being able to remove it.
	 */
	if (chdir("/") < 0) {
		syslog(LOG_ERR, "unable to change directory, dir=%s (%s)", "/", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Create the process PID file */
	if (__create_pidfile(__pidfile) != 0)
		exit(EXIT_FAILURE);

	/* Initialize the log/trace interface */
	if (logtrace_init(basename(argv[0]), __tracefile, tracemask) != 0)
		exit(EXIT_FAILURE);

	/* Cancel certain signals */
	signal(SIGCHLD, SIG_DFL);	/* A child process dies */
	signal(SIGTSTP, SIG_IGN);	/* Various TTY signals */
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGHUP, SIG_IGN);	/* Ignore hangup signal */
	signal(SIGTERM, SIG_DFL);	/* Die on SIGTERM */

	/* TODO: Drop user if there is one, and we were run as root */
	/*if (getuid() == 0 || geteuid() == 0) {
		struct passwd *pw = getpwnam(runas_username);
		if (pw) {
			if (setuid(pw->pw_uid) < 0) {
				LOG_ER("setuid failed, uid=%d (%s)", pw->pw_uid, strerror(errno));
				exit(EXIT_FAILURE);
			}
		} else {
			LOG_ER(invalid passwd, user=%s (%s)", runas_username, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}*/
}
